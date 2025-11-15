#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <chrono>


namespace runscope::core
{
    using ThreadId = std::thread::id;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;
    using ProcessId = uint32_t;

    enum class ProfilerMode
    {
        Instrumentation,
        Sampling,
        Both
    };

    enum class AttachmentStatus
    {
        Detached,
        Attaching,
        Attached,
        Failed
    };

    struct SystemInfo
    {
        uint32_t cpu_count;
        uint64_t total_memory;
        std::string os_name;
        std::string os_version;
    };
}

