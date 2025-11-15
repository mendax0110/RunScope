#pragma once

#include "types.hpp"
#include "profile_entry.hpp"
#include "profiler_engine.hpp"
#include "clock.hpp"
#include <string>
#include <string_view>


namespace runscope::core
{
    class ScopeProfiler
    {
    public:
        explicit ScopeProfiler(std::string_view name, const char* file = "", int line = 0);
        ~ScopeProfiler();

        ScopeProfiler(const ScopeProfiler&) = delete;
        ScopeProfiler& operator=(const ScopeProfiler&) = delete;
        ScopeProfiler(ScopeProfiler&&) = delete;
        ScopeProfiler& operator=(ScopeProfiler&&) = delete;

    private:
        static int& depth_ref();
        static int get_depth();
        static void increment_depth();
        static void decrement_depth();

        std::string name_;
        std::string file_;
        int line_;
        TimePoint start_time_;
        int depth_;
    };
}


#define RUNSCOPE_CONCAT_IMPL(x, y) x##y
#define RUNSCOPE_CONCAT(x, y) RUNSCOPE_CONCAT_IMPL(x, y)

#define RUNSCOPE_PROFILE_SCOPE(name) \
    ::runscope::core::ScopeProfiler RUNSCOPE_CONCAT(__profiler_, __LINE__)(name, __FILE__, __LINE__)

#define RUNSCOPE_PROFILE_FUNCTION() \
    RUNSCOPE_PROFILE_SCOPE(__FUNCTION__)
