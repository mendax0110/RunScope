#include "runscope/core/profiler_session.hpp"
#include "runscope/core/clock.hpp"
#include <algorithm>

#include "runscope/platform/process_attacher.hpp"

using namespace runscope::core;

ProfilerSession::ProfilerSession(std::string name)
    : name_(std::move(name))
    , start_time_(Clock::now())
    , active_(true)
{

}

ProfilerSession::~ProfilerSession()
{
    if (active_)
    {
        end();
    }
}

void ProfilerSession::add_entry(ProfileEntry entry)
{
    if (active_)
    {
        entries_.push_back(std::move(entry));
    }
}

void ProfilerSession::add_entry_mt(ProfileEntry entry)
{
    if (active_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back(std::move(entry));
    }
}

std::vector<ProfileEntry> ProfilerSession::get_entries() const
{
    return entries_;
}

std::vector<ProfileEntry> ProfilerSession::get_entries_mt() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

std::map<ThreadId, ThreadInfo> ProfilerSession::get_thread_info() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<ThreadId, ThreadInfo> thread_map;
    
    for (const auto& entry : entries_)
    {
        auto& info = thread_map[entry.thread_id];
        info.id = entry.thread_id;
        info.total_time_ns += entry.duration_ns();
        info.entry_count++;
    }
    
    return thread_map;
}

std::map<std::string, uint64_t> ProfilerSession::get_memory_usage() const
{
#if defined(__linux__) || defined(__APPLE__)
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, uint64_t> memory_map;

    for (const auto& entry : entries_)
    {
        memory_map[entry.name] += entry.memory_used;
    }

    return memory_map;
#endif
    return {};
}

std::map<std::string, double> ProfilerSession::get_cpu_usage() const
{
#if defined(__linux__) || defined(__APPLE__)
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, double> cpu_map;

    for (const auto& entry : entries_)
    {
        cpu_map[entry.name] += entry.cpu_usage;
    }

    return cpu_map;
#endif
    return {};
}

void ProfilerSession::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

size_t ProfilerSession::entry_count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void ProfilerSession::end()
{
    active_ = false;
    end_time_ = Clock::now();
}
