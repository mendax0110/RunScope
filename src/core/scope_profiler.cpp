#include "runscope/core/scope_profiler.hpp"
#include <thread>

using namespace runscope::core;

ScopeProfiler::ScopeProfiler(const std::string_view name, const char* file, const int line)
    : name_(name)
    , file_(file)
    , line_(line)
    , start_time_(Clock::now())
    , depth_(get_depth())
{
    increment_depth();
}

ScopeProfiler::~ScopeProfiler()
{
    const auto end_time = Clock::now();
    
    ProfileEntry entry;
    entry.name = name_;
    entry.file = file_;
    entry.line = line_;
    entry.start_ns = Clock::to_nanoseconds(start_time_);
    entry.end_ns = Clock::to_nanoseconds(end_time);
    entry.thread_id = std::this_thread::get_id();
    entry.depth = depth_;
    entry.memory_used = 0;
    entry.cpu_usage = 0.0;

    ProfilerEngine::getInstance().record_entry(std::move(entry));
    decrement_depth();
}

int& ScopeProfiler::depth_ref()
{
    thread_local int depth = 0;
    return depth;
}

int ScopeProfiler::get_depth()
{
    return depth_ref();
}

void ScopeProfiler::increment_depth()
{
    ++depth_ref();
}

void ScopeProfiler::decrement_depth()
{
    --depth_ref();
}