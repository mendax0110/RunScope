#pragma once

#include "types.hpp"
#include <cstdint>

namespace runscope::core
{
    class Clock
    {
    public:
        static TimePoint now() noexcept;
        static int64_t now_nanoseconds() noexcept;
        static int64_t now_microseconds() noexcept;
        static double now_milliseconds() noexcept;
        static double now_seconds() noexcept;

        static int64_t to_nanoseconds(const TimePoint& tp) noexcept;
        static int64_t to_microseconds(const TimePoint& tp) noexcept;
        static double to_milliseconds(const TimePoint& tp) noexcept;
        static double to_seconds(const TimePoint& tp) noexcept;

        static int64_t duration_nanoseconds(const TimePoint& start, const TimePoint& end) noexcept;
        static double duration_milliseconds(const TimePoint& start, const TimePoint& end) noexcept;
    };
}

