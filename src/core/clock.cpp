#include "runscope/core/clock.hpp"
#include <chrono>

using namespace runscope::core;

TimePoint Clock::now() noexcept
{
    return std::chrono::high_resolution_clock::now();
}

int64_t Clock::now_nanoseconds() noexcept
{
    return to_nanoseconds(now());
}

int64_t Clock::now_microseconds() noexcept
{
    return to_microseconds(now());
}

double Clock::now_milliseconds() noexcept
{
    return to_milliseconds(now());
}

double Clock::now_seconds() noexcept
{
    return to_seconds(now());
}

int64_t Clock::to_nanoseconds(const TimePoint& tp) noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

int64_t Clock::to_microseconds(const TimePoint& tp) noexcept
{
    return std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count();
}

double Clock::to_milliseconds(const TimePoint& tp) noexcept
{
    return std::chrono::duration<double, std::milli>(tp.time_since_epoch()).count();
}

double Clock::to_seconds(const TimePoint& tp) noexcept
{
    return std::chrono::duration<double>(tp.time_since_epoch()).count();
}

int64_t Clock::duration_nanoseconds(const TimePoint& start, const TimePoint& end) noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

double Clock::duration_milliseconds(const TimePoint& start, const TimePoint& end) noexcept
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}
