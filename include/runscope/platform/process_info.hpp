#pragma once

#include "runscope/core/types.hpp"
#include <string>
#include <vector>

namespace runscope::platform
{
    struct ProcessInfo
    {
        core::ProcessId pid;
        std::string name;
        std::string executable_path;
        uint64_t memory_usage;
        double cpu_usage;
        bool is_64bit;
    };

    class ProcessEnumerator
    {
    public:
        static std::vector<ProcessInfo> enumerate_processes();
        static ProcessInfo get_process_info(core::ProcessId pid);
        static bool is_process_running(core::ProcessId pid);
        static std::string get_process_name(core::ProcessId pid);
    };
}