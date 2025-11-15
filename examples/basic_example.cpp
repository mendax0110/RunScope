#include "runscope/runscope.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

void fibonacci(const int n)
{
    RUNSCOPE_PROFILE_FUNCTION();
    
    if (n <= 1) return;
    
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i)
    {
        const int temp = a + b;
        a = b;
        b = temp;
    }
}

void matrix_multiply(const int size)
{
    RUNSCOPE_PROFILE_FUNCTION();

    const std::vector a(size, std::vector(size, 1));
    const std::vector b(size, std::vector(size, 1));
    std::vector c(size, std::vector(size, 0));
    
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            for (int k = 0; k < size; ++k)
            {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void sort_data(const int size)
{
    RUNSCOPE_PROFILE_FUNCTION();
    
    std::vector<int> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (int& val : data)
    {
        val = dis(gen);
    }
    
    std::ranges::sort(data);
}

void nested_function_1()
{
    RUNSCOPE_PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void nested_function_2()
{
    RUNSCOPE_PROFILE_FUNCTION();
    nested_function_1();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void complex_computation()
{
    RUNSCOPE_PROFILE_FUNCTION();
    
    fibonacci(1000);
    matrix_multiply(50);
    sort_data(10000);
    nested_function_2();
}

void worker_thread(int id)
{
    RUNSCOPE_PROFILE_SCOPE("worker_thread_" + std::to_string(id));
    
    for (int i = 0; i < 5; ++i)
    {
        RUNSCOPE_PROFILE_SCOPE("iteration");
        fibonacci(500);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main()
{
    std::cout << "RunScope Basic Example\n";
    std::cout << "======================\n\n";
    
    auto& profiler = runscope::Profiler::getInstance();
    auto& process_mgr = runscope::ProcessManager::getInstance();
    
    process_mgr.register_process("fibonacci");
    process_mgr.register_process("matrix_multiply");
    process_mgr.register_process("sort_data");
    process_mgr.register_process("complex_computation");
    
    profiler.begin_session("BasicExample");
    
    std::cout << "Running single-threaded tests...\n";
    {
        RUNSCOPE_PROFILE_SCOPE("single_threaded");
        
        for (int i = 0; i < 3; ++i)
        {
            complex_computation();
        }
    }
    
    std::cout << "Running multi-threaded tests...\n";
    {
        RUNSCOPE_PROFILE_SCOPE("multi_threaded");
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i)
        {
            threads.emplace_back(worker_thread, i);
        }
        
        for (auto& t : threads)
        {
            t.join();
        }
    }
    
    profiler.end_session();

    const auto entries = profiler.get_entries();
    std::cout << "\nProfiler captured " << entries.size() << " entries\n\n";
    
    for (const auto& entry : entries)
    {
        process_mgr.update_statistics(entry.name, entry.duration_ms());
    }
    
    std::cout << "Exporting results...\n";
    if (runscope::Exporter::export_to_json(entries, "profile_results.json"))
    {
        std::cout << "  Exported to profile_results.json\n";
    }
    
    if (runscope::Exporter::export_to_csv(entries, "profile_results.csv"))
    {
        std::cout << "  Exported to profile_results.csv\n";
    }
    
    std::cout << "\nStatistics:\n";
    std::cout << "-----------\n";
    auto processes = process_mgr.get_all_processes();
    for (const auto& [name, info] : processes)
    {
        if (info.call_count > 0)
        {
            std::cout << name << ":\n";
            std::cout << "  Calls: " << info.call_count << "\n";
            std::cout << "  Total: " << info.total_time_ms << " ms\n";
            std::cout << "  Avg: " << info.avg_time_ms << " ms\n";
            std::cout << "  Min/Max: " << info.min_time_ms << " / " 
                     << info.max_time_ms << " ms\n\n";
        }
    }
    
    return 0;
}
