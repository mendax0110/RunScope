#include <gtest/gtest.h>
#include "runscope/process_manager.hpp"
#include <thread>

using namespace runscope;

class ProcessManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ProcessManager::getInstance().clear();
    }

    void TearDown() override
    {
        ProcessManager::getInstance().clear();
    }
};

TEST_F(ProcessManagerTest, RegisterProcess)
{
    auto& manager = ProcessManager::getInstance();
    
    manager.register_process("test_process");
    
    auto processes = manager.get_all_processes();
    EXPECT_EQ(processes.size(), 1);
    EXPECT_NE(processes.find("test_process"), processes.end());
}

TEST_F(ProcessManagerTest, EnableDisableProcess)
{
    auto& manager = ProcessManager::getInstance();
    
    manager.register_process("test_process");
    EXPECT_TRUE(manager.is_process_enabled("test_process"));
    
    manager.set_process_enabled("test_process", false);
    EXPECT_FALSE(manager.is_process_enabled("test_process"));
    
    manager.set_process_enabled("test_process", true);
    EXPECT_TRUE(manager.is_process_enabled("test_process"));
}

TEST_F(ProcessManagerTest, UpdateStatistics)
{
    auto& manager = ProcessManager::getInstance();
    
    manager.register_process("test_process");
    manager.update_statistics("test_process", 10.5);
    manager.update_statistics("test_process", 20.3);
    manager.update_statistics("test_process", 15.7);
    
    auto processes = manager.get_all_processes();
    const auto& info = processes["test_process"];
    
    EXPECT_EQ(info.call_count, 3);
    EXPECT_NEAR(info.total_time_ms, 46.5, 0.01);
    EXPECT_NEAR(info.avg_time_ms, 15.5, 0.01);
    EXPECT_NEAR(info.min_time_ms, 10.5, 0.01);
    EXPECT_NEAR(info.max_time_ms, 20.3, 0.01);
}

TEST_F(ProcessManagerTest, ClearStatistics)
{
    auto& manager = ProcessManager::getInstance();
    
    manager.register_process("test_process");
    manager.update_statistics("test_process", 10.5);
    manager.update_statistics("test_process", 20.3);
    
    manager.clear_statistics();
    
    auto processes = manager.get_all_processes();
    const auto& info = processes["test_process"];
    
    EXPECT_EQ(info.call_count, 0);
    EXPECT_EQ(info.total_time_ms, 0.0);
    EXPECT_EQ(info.avg_time_ms, 0.0);
}

TEST_F(ProcessManagerTest, MultipleProcesses)
{
    auto& manager = ProcessManager::getInstance();
    
    for (int i = 0; i < 10; ++i)
    {
        manager.register_process("process_" + std::to_string(i));
    }
    
    const auto processes = manager.get_all_processes();
    EXPECT_EQ(processes.size(), 10);
}

TEST_F(ProcessManagerTest, ThreadSafety)
{
    auto& manager = ProcessManager::getInstance();
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&manager, i]()
        {
            const std::string process_name = "process_" + std::to_string(i);
            manager.register_process(process_name);
            
            for (int j = 0; j < 100; ++j)
            {
                manager.update_statistics(process_name, j * 0.1);
            }
        });
    }
    
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    auto processes = manager.get_all_processes();
    EXPECT_EQ(processes.size(), 10);
    
    for (int i = 0; i < 10; ++i)
    {
        std::string process_name = "process_" + std::to_string(i);
        EXPECT_EQ(processes[process_name].call_count, 100);
    }
}

TEST_F(ProcessManagerTest, UnregisteredProcess)
{
    auto& manager = ProcessManager::getInstance();
    
    EXPECT_FALSE(manager.is_process_enabled("nonexistent"));
    
    manager.set_process_enabled("nonexistent", false);
    EXPECT_FALSE(manager.is_process_enabled("nonexistent"));
}

TEST_F(ProcessManagerTest, DuplicateRegistration)
{
    auto& manager = ProcessManager::getInstance();
    
    manager.register_process("test_process");
    manager.update_statistics("test_process", 10.0);
    
    manager.register_process("test_process");
    
    auto processes = manager.get_all_processes();
    EXPECT_EQ(processes.size(), 1);
    EXPECT_EQ(processes["test_process"].call_count, 1);
}
