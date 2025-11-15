#pragma once

#include <chrono>
#include <cstdint>

namespace runscope
{
    class Timer
    {
    public:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::nanoseconds;

        Timer() noexcept : start_time_(Clock::now()) {}

        void reset() noexcept
        {
            start_time_ = Clock::now();
        }

        [[nodiscard]] Duration elapsed() const noexcept
        {
            return std::chrono::duration_cast<Duration>(Clock::now() - start_time_);
        }

        [[nodiscard]] double elapsed_seconds() const noexcept
        {
            return std::chrono::duration<double>(elapsed()).count();
        }

        [[nodiscard]] double elapsed_milliseconds() const noexcept
        {
            return std::chrono::duration<double, std::milli>(elapsed()).count();
        }

        [[nodiscard]] int64_t elapsed_microseconds() const noexcept
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(elapsed()).count();
        }

        [[nodiscard]] int64_t elapsed_nanoseconds() const noexcept
        {
            return elapsed().count();
        }

        [[nodiscard]] TimePoint start_time() const noexcept
        {
            return start_time_;
        }

    private:
        TimePoint start_time_;
    };
}
