#include "runscope/platform/process_attacher.hpp"
#include "runscope/core/clock.hpp"
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef __linux__
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <dirent.h>
#include <elf.h>
#include <link.h>
#include <sys/uio.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/thread_info.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <libproc.h>
#include <mach-o/dyld.h>
#include <mach/mach_vm.h>
#endif

using namespace runscope::platform;
using namespace runscope::core;

class ProcessAttacher::Impl
{
public:
    Impl() = default;
    ~Impl()
    {
        if (sampling_)
        {
            stop_sampling();
        }
        if (attached_)
        {
            detach();
        }
    }
    
    bool attach(core::ProcessId pid)
    {
        if (attached_)
        {
            last_error_ = "Already attached to a process";
            return false;
        }
        
#ifdef __linux__
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1)
        {
            last_error_ = "Failed to attach to process";
            status_ = core::AttachmentStatus::Failed;
            return false;
        }
        
        int status;
        waitpid(pid, &status, 0);
        
        if (ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
        {
            last_error_ = "Failed to continue process after attach";
            ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
            status_ = core::AttachmentStatus::Failed;
            return false;
        }
#elif __APPLE__
        mach_port_t task;
        kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS)
        {
            last_error_ = "Failed to get task port. May need entitlements or sudo.";
            status_ = core::AttachmentStatus::Failed;
            return false;
        }
        task_port_ = task;
#else
        last_error_ = "Process attachment not supported on this platform";
        status_ = core::AttachmentStatus::Failed;
        return false;
#endif
        
        attached_ = true;
        attached_pid_ = pid;
        status_ = core::AttachmentStatus::Attached;
        return true;
    }
    
    bool detach()
    {
        if (!attached_)
        {
            return false;
        }
        
#ifdef __linux__
        ptrace(PTRACE_DETACH, attached_pid_, nullptr, nullptr);
#elif __APPLE__
        if (task_port_ != MACH_PORT_NULL)
        {
            mach_port_deallocate(mach_task_self(), task_port_);
            task_port_ = MACH_PORT_NULL;
        }
#endif
        
        attached_ = false;
        attached_pid_ = 0;
        status_ = core::AttachmentStatus::Detached;
        return true;
    }
    
    bool is_attached() const noexcept { return attached_; }
    core::ProcessId attached_pid() const noexcept { return attached_pid_; }
    core::AttachmentStatus status() const noexcept { return status_; }
    
    void set_sample_rate(int rate) { sample_rate_ = rate; }
    int sample_rate() const noexcept { return sample_rate_; }
    
    void start_sampling()
    {
        if (!attached_)
        {
            return;
        }
        
        if (sampling_)
        {
            return; // Already sampling
        }
        
        sampling_ = true;
        sampling_thread_ = std::thread(&Impl::sampling_loop, this);
    }
    
    void stop_sampling()
    {
        if (!sampling_)
        {
            return;
        }
        
        sampling_ = false;
        
        if (sampling_thread_.joinable())
        {
            sampling_thread_.join();
        }
    }
    
    bool is_sampling() const noexcept { return sampling_; }
    
    void set_sample_callback(SampleCallback callback)
    {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        sample_callback_ = std::move(callback);
    }
    
    std::vector<core::ProfileEntry> get_sampled_entries() const
    {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        return sampled_entries_;
    }
    
    std::string last_error() const { return last_error_; }
    
private:
    void sampling_loop()
    {
        auto interval = std::chrono::milliseconds(1000 / sample_rate_);
        
        while (sampling_ && attached_)
        {
            auto start_time = std::chrono::steady_clock::now();
            sample_process();
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed < interval)
            {
                std::this_thread::sleep_for(interval - elapsed);
            }
        }
    }
    
    std::vector<pid_t> get_thread_ids() const
    {
        std::vector<pid_t> tids;
        
#ifdef __linux__
        const std::string task_path = "/proc/" + std::to_string(attached_pid_) + "/task";
        DIR* dir = opendir(task_path.c_str());
        if (!dir)
        {
            return tids;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_type == DT_DIR)
            {
                std::string tid_str = entry->d_name;
                if (!tid_str.empty() && std::all_of(tid_str.begin(), tid_str.end(), ::isdigit))
                {
                    tids.push_back(std::stoi(tid_str));
                }
            }
        }
        closedir(dir);
#elif __APPLE__
        thread_act_array_t thread_list;
        mach_msg_type_number_t thread_count;
        
        if (task_threads(task_port_, &thread_list, &thread_count) == KERN_SUCCESS)
        {
            for (mach_msg_type_number_t i = 0; i < thread_count; i++)
            {
                tids.push_back(thread_list[i]);
            }
            
            vm_deallocate(mach_task_self(), (vm_address_t)thread_list, thread_count * sizeof(thread_act_t));
        }
#endif
        
        return tids;
    }
    
    static std::string demangle_symbol(const char* mangled)
    {
        if (!mangled)
            return "??";
            
        int status;
        char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
        
        if (status == 0 && demangled)
        {
            std::string result(demangled);
            free(demangled);
            return result;
        }
        
        return mangled;
    }
    
    static std::string resolve_address(void* addr)
    {
#ifdef __linux__
        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname)
        {
            return demangle_symbol(info.dli_sname);
        }
#endif

#if __APPLE__
        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname)
        {
            return demangle_symbol(info.dli_sname);
        }
#endif
        std::ostringstream oss;
        oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(addr);
        return oss.str();
    }
    
    struct ThreadState
    {
        std::string name;
        std::string state;
        uint64_t utime{};
        uint64_t stime{};
    };
    
    static ThreadState read_thread_state(pid_t pid, pid_t tid)
    {
        ThreadState state;
#ifdef __linux__
        std::string stat_path = "/proc/" + std::to_string(pid) + "/task/" + 
                               std::to_string(tid) + "/stat";
        std::ifstream stat_file(stat_path);
        if (!stat_file.is_open())
        {
            state.name = "unknown";
            state.state = "?";
            return state;
        }
        
        std::string line;
        std::getline(stat_file, line);

        size_t name_start = line.find('(');
        size_t name_end = line.find(')');
        if (name_start != std::string::npos && name_end != std::string::npos)
        {
            state.name = line.substr(name_start + 1, name_end - name_start - 1);

            if (name_end + 2 < line.length())
            {
                state.state = line[name_end + 2];
            }
        }
#elif __APPLE__
        thread_basic_info_data_t basic_info;
        mach_msg_type_number_t info_count = THREAD_BASIC_INFO_COUNT;
        
        if (thread_info(tid, THREAD_BASIC_INFO, (thread_info_t)&basic_info, &info_count) == KERN_SUCCESS)
        {
            switch (basic_info.run_state)
            {
                case TH_STATE_RUNNING: state.state = "R"; break;
                case TH_STATE_STOPPED: state.state = "T"; break;
                case TH_STATE_WAITING: state.state = "S"; break;
                case TH_STATE_UNINTERRUPTIBLE: state.state = "D"; break;
                case TH_STATE_HALTED: state.state = "Z"; break;
                default: state.state = "?";
            }

            char thread_name[256];
            pthread_t pthread_id = pthread_from_mach_thread_np(tid);
            if (pthread_id && pthread_getname_np(pthread_id, thread_name, sizeof(thread_name)) == 0)
            {
                state.name = thread_name;
            }
            else
            {
                state.name = "Thread-" + std::to_string(tid);
            }
        }
        else
        {
            state.name = "Thread-" + std::to_string(tid);
            state.state = "?";
        }
#else
        state.name = "Thread-" + std::to_string(tid);
        state.state = "?";
#endif
        
        return state;
    }
    
    std::string get_process_exe_name() const
    {
#ifdef __APPLE__
        std::string exe_path = "/proc/" + std::to_string(attached_pid_) + "/path/a.out";
        char path[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(attached_pid_, path, sizeof(path)) > 0)
        {
            std::string full_path(path);
            size_t last_slash = full_path.find_last_of('/');
            if (last_slash != std::string::npos)
            {
                return full_path.substr(last_slash + 1);
            }
            return full_path;
        }
#elif __linux__
        const std::string exe_path = "/proc/" + std::to_string(attached_pid_) + "/exe";
        char path[1024];
        const ssize_t len = readlink(exe_path.c_str(), path, sizeof(path) - 1);
        if (len != -1)
        {
            path[len] = '\0';
            std::string full_path(path);
            // Get just the filename
            const size_t last_slash = full_path.find_last_of('/');
            if (last_slash != std::string::npos)
            {
                return full_path.substr(last_slash + 1);
            }
            return full_path;
        }
#endif
        return "unknown";
    }
    
    static std::vector<void*> read_stack_trace(pid_t tid)
    {
        std::vector<void*> stack_frames;
        
#ifdef __linux__
        // Create stack frames based on actual thread state
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, tid, nullptr, &regs) == -1)
        {
            return stack_frames;
        }
        
        //readng the instr ptr..
        #ifdef __x86_64__
            void* ip = reinterpret_cast<void*>(regs.rip);
            void* bp = reinterpret_cast<void*>(regs.rbp);
        #elif defined(__i386__)
            void* ip = reinterpret_cast<void*>(regs.eip);
            void* bp = reinterpret_cast<void*>(regs.ebp);
        #elif defined(__aarch64__)
            void* ip = reinterpret_cast<void*>(regs.pc);
            void* bp = reinterpret_cast<void*>(regs.regs[29]);
        #else
            return stack_frames;
        #endif
        
        stack_frames.push_back(ip);
        
        //unwind the stack by reading frame pointers
        uintptr_t current_bp = reinterpret_cast<uintptr_t>(bp);
        constexpr int max_frames = 32;
        
        for (int i = 0; i < max_frames && current_bp != 0; ++i)
        {
            const long ret_addr = ptrace(PTRACE_PEEKDATA, tid, current_bp + sizeof(void*), nullptr);
            if (ret_addr == -1)
            {
                break;
            }
            
            stack_frames.push_back(reinterpret_cast<void*>(ret_addr));

            const long next_bp = ptrace(PTRACE_PEEKDATA, tid, current_bp, nullptr);
            if (next_bp == -1 || static_cast<uintptr_t>(next_bp) <= current_bp)
            {
                break;
            }
            
            current_bp = next_bp;
        }
        
#elif __APPLE__
        #ifdef __x86_64__
            x86_thread_state64_t state;
            mach_msg_type_number_t state_count = x86_THREAD_STATE64_COUNT;
            
            if (thread_get_state(tid, x86_THREAD_STATE64, (thread_state_t)&state, &state_count) == KERN_SUCCESS)
            {
                void* ip = reinterpret_cast<void*>(state.__rip);
                void* bp = reinterpret_cast<void*>(state.__rbp);
                stack_frames.push_back(ip);

                // Note: This requires proper frame pointer preservation (-fno-omit-frame-pointer)
                uintptr_t current_bp = reinterpret_cast<uintptr_t>(bp);
                const int max_frames = 16;
                
                for (int i = 0; i < max_frames && current_bp != 0; ++i)
                {
                    break; // TODO: Implement with mach_vm_read_overwrite
                }
            }
        #elif defined(__arm64__) || defined(__aarch64__)
            arm_thread_state64_t state;
            mach_msg_type_number_t state_count = ARM_THREAD_STATE64_COUNT;
            
            if (thread_get_state(tid, ARM_THREAD_STATE64, (thread_state_t)&state, &state_count) == KERN_SUCCESS)
            {
                //acces programcounter, frame pointer, link register
                void* ip = reinterpret_cast<void*>(arm_thread_state64_get_pc(state));
                void* fp = reinterpret_cast<void*>(arm_thread_state64_get_fp(state));
                void* lr = reinterpret_cast<void*>(arm_thread_state64_get_lr(state));
                
                stack_frames.push_back(ip);
                
                // Add link reg the return address
                if (lr != nullptr)
                {
                    stack_frames.push_back(lr);
                }

                // On ARM64, frame layout: [FP-1] = LR, [FP] = previous FP
                uintptr_t current_fp = reinterpret_cast<uintptr_t>(fp);
                const int max_frames = 16;
                
                for (int i = 0; i < max_frames && current_fp != 0; ++i)
                {
                    auto remoteProcessMemoryRead = [](mach_port_t task, uintptr_t address, void* buffer, size_t size) -> bool
                    {
                        if (task == MACH_PORT_NULL)
                            return false;
                        mach_vm_size_t outSize = 0;
                        kern_return_t kr = mach_vm_read_overwrite(task, address, size, reinterpret_cast<mach_vm_address_t>(buffer), &outSize);
                        return (kr == KERN_SUCCESS && outSize == size);
                    };

                    remoteProcessMemoryRead(MACH_PORT_NULL, current_fp - sizeof(void*), &lr, sizeof(void*));
                    if (lr != nullptr)
                    {
                        stack_frames.push_back(lr);
                    }
                    uintptr_t next_fp = 0;
                    remoteProcessMemoryRead(MACH_PORT_NULL, current_fp, &next_fp, sizeof(void*));
                    if (next_fp == 0 || next_fp <= current_fp)
                    {
                        break;
                    }
                    current_fp = next_fp;
                }
            }
        #endif
#endif
        
        return stack_frames;
    }
    
    void sample_process()
    {
        if (!attached_)
        {
            return;
        }
        
        std::vector<core::ProfileEntry> entries;
        int64_t sample_time = core::Clock::now_nanoseconds();
        
        ++sample_count_;

        auto thread_ids = get_thread_ids();
        std::string exe_name = get_process_exe_name();
        
        if (thread_ids.empty())
        {
            core::ProfileEntry entry;
            entry.name = "[Attached to: " + exe_name + " (PID:" + std::to_string(attached_pid_) + ")]";
            entry.start_ns = sample_time;
            entry.end_ns = sample_time + 1000000; // 1ms
            entry.thread_id = std::this_thread::get_id();
            entry.depth = 0;
            entries.push_back(entry);
        }
        else
        {
            for (auto tid : thread_ids)
            {
                ThreadState thread_state = read_thread_state(attached_pid_, tid);
                core::ProfileEntry entry;
                std::string state_str;
                switch (thread_state.state[0])
                {
                    case 'R': state_str = "Running"; break;
                    case 'S': state_str = "Sleeping"; break;
                    case 'D': state_str = "Disk_Sleep"; break;
                    case 'Z': state_str = "Zombie"; break;
                    case 'T': state_str = "Stopped"; break;
                    default: state_str = "Unknown";
                }

                entry.name = exe_name + "::" + thread_state.name + " [TID:" +std::to_string(tid) + ", State:" + state_str + "]";
                
                entry.start_ns = sample_time;
                entry.end_ns = sample_time + 1000000; // 1ms sample
                entry.thread_id = std::this_thread::get_id();
                entry.depth = 0;

                auto stack_frames = read_stack_trace(tid);
                
                if (!stack_frames.empty())
                {
                    core::ProfileEntry* parent = &entry;
                    int depth = 1;
                    
                    for (size_t i = 0; i < std::min(stack_frames.size(), static_cast<size_t>(5)); ++i)
                    {
                        auto child = std::make_shared<core::ProfileEntry>();
                        child->name = resolve_address(stack_frames[i]);
                        child->start_ns = sample_time;
                        child->end_ns = sample_time + 800000;
                        child->thread_id = entry.thread_id;
                        child->depth = depth++;
                        parent->children.push_back(child);
                    }
                }
                
                entries.push_back(entry);
            }
        }

        {
            std::lock_guard<std::mutex> lock(sample_mutex_);
            sampled_entries_.insert(sampled_entries_.end(), entries.begin(), entries.end());

            if (sampled_entries_.size() > 1000)
            {
                sampled_entries_.erase(sampled_entries_.begin(), sampled_entries_.begin() + (sampled_entries_.size() - 1000));
            }
        }

        if (sample_callback_)
        {
            sample_callback_(entries);
        }
    }

    bool attached_{false};
    core::ProcessId attached_pid_{0};
    core::AttachmentStatus status_{core::AttachmentStatus::Detached};
    int sample_rate_{100};
    std::atomic<bool> sampling_{false};
    std::string last_error_;
    std::thread sampling_thread_;
    mutable std::mutex sample_mutex_;
    SampleCallback sample_callback_;
    std::vector<core::ProfileEntry> sampled_entries_;
    std::atomic<uint64_t> sample_count_{0};
    
#ifdef __APPLE__
    mach_port_t task_port_{MACH_PORT_NULL};
#endif
};

ProcessAttacher::ProcessAttacher() : impl_(std::make_unique<Impl>()) {}
ProcessAttacher::~ProcessAttacher() = default;

bool ProcessAttacher::attach(core::ProcessId pid) const
{
    return impl_->attach(pid);
}

bool ProcessAttacher::detach() const
{
    return impl_->detach();
}

bool ProcessAttacher::is_attached() const noexcept
{
    return impl_->is_attached();
}

ProcessId ProcessAttacher::attached_pid() const noexcept
{
    return impl_->attached_pid();
}

AttachmentStatus ProcessAttacher::status() const noexcept
{
    return impl_->status();
}

void ProcessAttacher::set_sample_rate(const int samples_per_second) const
{
    impl_->set_sample_rate(samples_per_second);
}

int ProcessAttacher::sample_rate() const noexcept
{
    return impl_->sample_rate();
}

void ProcessAttacher::start_sampling() const
{
    impl_->start_sampling();
}

void ProcessAttacher::stop_sampling() const
{
    impl_->stop_sampling();
}

bool ProcessAttacher::is_sampling() const noexcept
{
    return impl_->is_sampling();
}

void ProcessAttacher::set_sample_callback(SampleCallback callback) const
{
    impl_->set_sample_callback(std::move(callback));
}

std::vector<runscope::core::ProfileEntry> ProcessAttacher::get_sampled_entries() const
{
    return impl_->get_sampled_entries();
}

std::string ProcessAttacher::last_error() const
{
    return impl_->last_error();
}