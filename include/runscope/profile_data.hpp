#pragma once

#include <string>
#include <cstdint>
#include <thread>

namespace runscope
{
    struct ProfileEntry
    {
        std::string name;
        int64_t start_ns;
        int64_t end_ns;
        std::thread::id thread_id;
        int depth;

        [[nodiscard]] int64_t duration_ns() const noexcept
        {
            return end_ns - start_ns;
        }

        [[nodiscard]] double duration_ms() const noexcept
        {
            return duration_ns() / 1'000'000.0;
        }

        [[nodiscard]] double duration_us() const noexcept
        {
            return duration_ns() / 1'000.0;
        }
    };
}
