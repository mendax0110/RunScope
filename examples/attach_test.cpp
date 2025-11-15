#include "runscope/platform/process_attacher.hpp"
#include "runscope/core/profiler_engine.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(const int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <pid>" << std::endl;
        return 1;
    }
    
    const int pid = std::atoi(argv[1]);
    
    std::cout << "Attempting to attach to PID " << pid << "..." << std::endl;
    
    runscope::platform::ProcessAttacher attacher;
    
    if (!attacher.attach(pid))
    {
        std::cerr << "Failed to attach: " << attacher.last_error() << std::endl;
        std::cerr << "Note: You may need to run with sudo or adjust ptrace_scope" << std::endl;
        std::cerr << "Try: sudo sysctl -w kernel.yama.ptrace_scope=0" << std::endl;
        return 1;
    }
    
    std::cout << "Successfully attached to PID " << pid << std::endl;
    std::cout << "Starting sampling..." << std::endl;
    
    attacher.set_sample_rate(10);
    attacher.start_sampling();

    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "Stopping sampling..." << std::endl;
    attacher.stop_sampling();

    const auto entries = attacher.get_sampled_entries();
    
    std::cout << "\nCollected " << entries.size() << " sample entries" << std::endl;
    std::cout << "\nFirst 10 samples:" << std::endl;
    
    int count = 0;
    for (const auto& entry : entries)
    {
        if (count++ >= 10) break;
        
        std::cout << "  " << entry.name 
                  << " (depth=" << entry.depth 
                  << ", duration=" << entry.duration_ms() << "ms";
        
        if (!entry.children.empty())
        {
            std::cout << ", " << entry.children.size() << " children";
        }
        
        std::cout << ")" << std::endl;
        
        for (const auto& child : entry.children)
        {
            std::cout << "    -> " << child->name << std::endl;
        }
    }
    
    std::cout << "\nDetaching..." << std::endl;
    attacher.detach();
    
    std::cout << "Done!" << std::endl;
    
    return 0;
}
