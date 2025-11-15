#include <gtest/gtest.h>
#include "runscope/timer.hpp"
#include <thread>
#include <chrono>

using namespace runscope;

TEST(TimerTest, Initialization)
{
    const Timer timer;
    EXPECT_GE(timer.elapsed_nanoseconds(), 0);
}

TEST(TimerTest, ElapsedTime)
{
    const Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_GE(timer.elapsed_milliseconds(), 10.0);
    EXPECT_GE(timer.elapsed_microseconds(), 10000);
    EXPECT_GE(timer.elapsed_nanoseconds(), 10000000);
}

TEST(TimerTest, Reset)
{
    Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    const auto elapsed1 = timer.elapsed_milliseconds();
    EXPECT_GE(elapsed1, 10.0);
    
    timer.reset();
    const auto elapsed2 = timer.elapsed_milliseconds();
    EXPECT_LT(elapsed2, elapsed1);
}

TEST(TimerTest, ElapsedSeconds)
{
    const Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const double seconds = timer.elapsed_seconds();
    EXPECT_GE(seconds, 0.1);
    EXPECT_LT(seconds, 1.0);
}

TEST(TimerTest, StartTime)
{
    const Timer timer;
    const auto start = timer.start_time();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(timer.start_time(), start);
}

TEST(TimerTest, MultipleTimers)
{
    const Timer timer1;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const Timer timer2;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_GT(timer1.elapsed_milliseconds(), timer2.elapsed_milliseconds());
}
