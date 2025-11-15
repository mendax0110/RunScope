#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <memory>


namespace runscope::core
{
    struct ProfileEntry
    {
        std::string name;
        std::string file;
        int line;
        int64_t start_ns;
        int64_t end_ns;
        ThreadId thread_id;
        int depth;
        uint64_t memory_used;
        double cpu_usage;
        std::vector<std::shared_ptr<ProfileEntry>> children;

        ProfileEntry()
            : line(0)
            , start_ns(0)
            , end_ns(0)
            , depth(0)
            , memory_used(0)
            , cpu_usage(0.0)
        {

        }

        [[nodiscard]] int64_t duration_ns() const noexcept
        {
            return end_ns - start_ns;
        }

        [[nodiscard]] double duration_us() const noexcept
        {
            return duration_ns() / 1000.0;
        }

        [[nodiscard]] double duration_ms() const noexcept
        {
            return duration_ns() / 1000000.0;
        }

        [[nodiscard]] double duration_s() const noexcept
        {
            return duration_ns() / 1000000000.0;
        }

        [[nodiscard]] bool has_children() const noexcept
        {
            return !children.empty();
        }

        [[nodiscard]] size_t child_count() const noexcept
        {
            return children.size();
        }
    };

    struct ThreadInfo
    {
        ThreadId id;
        std::string name;
        uint64_t total_time_ns;
        size_t entry_count;

        ThreadInfo()
            : total_time_ns(0)
            , entry_count(0)
        {

        }
    };
}

