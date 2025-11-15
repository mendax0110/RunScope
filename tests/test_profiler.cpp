#include <gtest/gtest.h>
#include "runscope/profiler.hpp"
#include <thread>
#include <chrono>

using namespace runscope;

class ProfilerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Profiler::getInstance().clear();
    }
    
    void TearDown() override
    {
        Profiler::getInstance().end_session();
    }
};

TEST_F(ProfilerTest, SessionManagement)
{
    auto& profiler = Profiler::getInstance();
    
    EXPECT_FALSE(profiler.is_active());
    
    profiler.begin_session("test_session");
    EXPECT_TRUE(profiler.is_active());
    EXPECT_EQ(profiler.get_session_name(), "test_session");
    
    profiler.end_session();
    EXPECT_FALSE(profiler.is_active());
}

TEST_F(ProfilerTest, RecordEntry)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");

    const ProfileEntry entry{
        "test_function",
        1000000,
        2000000,
        std::this_thread::get_id(),
        0
    };
    
    profiler.record_entry(entry);
    
    const auto entries = profiler.get_entries();
    EXPECT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].name, "test_function");
    EXPECT_EQ(entries[0].duration_ns(), 1000000);
}

TEST_F(ProfilerTest, MultipleEntries)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");
    
    for (int i = 0; i < 10; ++i)
    {
        const ProfileEntry entry{
            "function_" + std::to_string(i),
            i * 1000000,
            (i + 1) * 1000000,
            std::this_thread::get_id(),
            0
        };
        profiler.record_entry(entry);
    }
    
    const auto entries = profiler.get_entries();
    EXPECT_EQ(entries.size(), 10);
}

TEST_F(ProfilerTest, ClearEntries)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");
    
    const ProfileEntry entry{"test", 0, 1000000, std::this_thread::get_id(), 0};
    profiler.record_entry(entry);
    
    EXPECT_EQ(profiler.get_entries().size(), 1);
    
    profiler.clear();
    EXPECT_EQ(profiler.get_entries().size(), 0);
}

TEST_F(ProfilerTest, ThreadSafety)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&profiler, i]()
        {
            for (int j = 0; j < 100; ++j)
            {
                const ProfileEntry entry{
                    "thread_" + std::to_string(i) + "_entry_" + std::to_string(j),
                    j * 1000000,
                    (j + 1) * 1000000,
                    std::this_thread::get_id(),
                    0
                };
                profiler.record_entry(entry);
            }
        });
    }
    
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    const auto entries = profiler.get_entries();
    EXPECT_EQ(entries.size(), 1000);
}

void test_function_1()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void test_function_2()
{
    RUNSCOPE_PROFILE_FUNCTION();
    test_function_1();
}

TEST_F(ProfilerTest, ScopeProfiler)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");
    
    {
        RUNSCOPE_PROFILE_SCOPE("test_scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto entries = profiler.get_entries();
    EXPECT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].name, "test_scope");
    EXPECT_GE(entries[0].duration_ms(), 10.0);
}

TEST_F(ProfilerTest, NestedScopes)
{
    auto& profiler = Profiler::getInstance();
    profiler.begin_session("test");
    
    {
        RUNSCOPE_PROFILE_SCOPE("outer");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        {
            RUNSCOPE_PROFILE_SCOPE("inner");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    
    const auto entries = profiler.get_entries();
    EXPECT_EQ(entries.size(), 2);
    
    bool found_outer = false;
    bool found_inner = false;
    
    for (const auto& entry : entries)
    {
        if (entry.name == "outer")
        {
            found_outer = true;
            EXPECT_EQ(entry.depth, 0);
        }
        if (entry.name == "inner")
        {
            found_inner = true;
            EXPECT_EQ(entry.depth, 1);
        }
    }
    
    EXPECT_TRUE(found_outer);
    EXPECT_TRUE(found_inner);
}

TEST_F(ProfilerTest, InactiveProfiler)
{
    auto& profiler = Profiler::getInstance();
    
    const ProfileEntry entry{"test", 0, 1000000, std::this_thread::get_id(), 0};
    profiler.record_entry(entry);
    
    EXPECT_EQ(profiler.get_entries().size(), 0);
}
