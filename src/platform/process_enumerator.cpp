#include "runscope/platform/process_info.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif __APPLE__
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <libproc.h>
#include <unistd.h>
#endif

using namespace runscope::platform;

#ifdef __linux__

std::vector<ProcessInfo> ProcessEnumerator::enumerate_processes()
{
    std::vector<ProcessInfo> processes;
    DIR* dir = opendir("/proc");
    if (!dir)
    {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type != DT_DIR) continue;
        
        std::string pid_str = entry->d_name;
        if (pid_str.empty() || !std::ranges::all_of(pid_str, ::isdigit))
        {
            continue;
        }

        const core::ProcessId pid = std::stoul(pid_str);
        
        try
        {
            ProcessInfo info = get_process_info(pid);
            processes.push_back(info);
        }
        catch (...)
        {

        }
    }
    
    closedir(dir);
    return processes;
}

ProcessInfo ProcessEnumerator::get_process_info(core::ProcessId pid)
{
    ProcessInfo info;
    info.pid = pid;
    info.memory_usage = 0;
    info.cpu_usage = 0.0;
    info.is_64bit = (sizeof(void*) == 8);
    
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(status_path);
    if (status_file.is_open())
    {
        std::string line;
        while (std::getline(status_file, line))
        {
            if (line.substr(0, 5) == "Name:")
            {
                info.name = line.substr(6);
                break;
            }
        }
    }
    
    std::string exe_path = "/proc/" + std::to_string(pid) + "/exe";
    char path[1024];
    ssize_t len = readlink(exe_path.c_str(), path, sizeof(path) - 1);
    if (len != -1)
    {
        path[len] = '\0';
        info.executable_path = path;
    }
    
    return info;
}

#elif __APPLE__

std::vector<ProcessInfo> ProcessEnumerator::enumerate_processes()
{
    std::vector<ProcessInfo> processes;
    
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    
    if (sysctl(mib, 4, nullptr, &size, nullptr, 0) < 0)
    {
        return processes;
    }
    
    std::vector<struct kinfo_proc> proc_list(size / sizeof(struct kinfo_proc));
    
    if (sysctl(mib, 4, proc_list.data(), &size, nullptr, 0) < 0)
    {
        return processes;
    }
    
    size_t count = size / sizeof(struct kinfo_proc);
    for (size_t i = 0; i < count; ++i)
    {
        ProcessInfo info;
        info.pid = proc_list[i].kp_proc.p_pid;
        info.name = proc_list[i].kp_proc.p_comm;
        info.memory_usage = 0;
        info.cpu_usage = 0.0;
        info.is_64bit = true;
        
        char path[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(info.pid, path, sizeof(path)) > 0)
        {
            info.executable_path = path;
        }
        
        processes.push_back(info);
    }
    
    return processes;
}

ProcessInfo ProcessEnumerator::get_process_info(core::ProcessId pid)
{
    ProcessInfo info;
    info.pid = pid;
    info.memory_usage = 0;
    info.cpu_usage = 0.0;
    info.is_64bit = true;
    
    struct proc_bsdinfo proc{};
    if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, sizeof(proc)) > 0)
    {
        info.name = proc.pbi_comm;
    }
    
    char path[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, path, sizeof(path)) > 0)
    {
        info.executable_path = path;
    }
    
    return info;
}

#else

std::vector<ProcessInfo> ProcessEnumerator::enumerate_processes()
{
    return {};
}

ProcessInfo ProcessEnumerator::get_process_info(core::ProcessId pid)
{
    ProcessInfo info;
    info.pid = pid;
    return info;
}

#endif

bool ProcessEnumerator::is_process_running(core::ProcessId pid)
{
#ifdef __linux__
    const std::string path = "/proc/" + std::to_string(pid);
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#elif __APPLE__
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(pid)};
    struct kinfo_proc proc{};
    size_t size = sizeof(proc);
    return sysctl(mib, 4, &proc, &size, nullptr, 0) == 0;
#else
    return false;
#endif
}

std::string ProcessEnumerator::get_process_name(core::ProcessId pid)
{
    return get_process_info(pid).name;
}
