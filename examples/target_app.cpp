// Target application to be profiled by profiler_app
// This simulates a real application with various function calls

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <vector>

#if defined(__APPLE__)
#include <unistd.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

void compute_physics(const int iterations)
{
    double sum = 0.0;
    for (int i = 0; i < iterations; ++i)
    {
        sum += std::sin(i * 0.1) * std::cos(i * 0.2);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void render_graphics()
{
    std::vector<double> pixels(1000);
    for (size_t i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = std::sqrt(i) * 3.14159;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
}

void process_input()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

void update_game_logic()
{
    compute_physics(10000);
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
}

void game_loop()
{
    process_input();
    update_game_logic();
    render_graphics();
}

size_t get_current_pid()
{
#ifdef __APPLE__
    return static_cast<size_t>(::getpid());
#else // linux
    return static_cast<size_t>(::getpid());
#endif
}

[[noreturn]] int main()
{
    std::cout << "Target application started. PID: " << get_current_pid() << std::endl;
    std::cout << "This application can be profiled by attaching profiler_app to PID " 
              << get_current_pid() << std::endl;
    std::cout << "Running game loop..." << std::endl;
    
    int frame_count = 0;
    while (true)
    {
        game_loop();
        frame_count++;
        
        if (frame_count % 60 == 0)
        {
            std::cout << "Frame: " << frame_count << std::endl;
        }
        
        // Target ~30 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
