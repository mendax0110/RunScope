# RunScope User Guide

## Table of Contents
1. [Getting Started](#getting-started)
2. [Basic Profiling](#basic-profiling)
3. [Advanced Features](#advanced-features)
4. [Using the UI](#using-the-ui)
5. [Process Attachment](#process-attachment)
6. [Data Analysis](#data-analysis)
7. [Export and Import](#export-and-import)
8. [Troubleshooting](#troubleshooting)

## Getting Started

### Installation

1. **Clone the repository**
```bash
git clone https://github.com/mendax0110/RunScope.git
cd RunScope
```

2. **Install dependencies**
```bash
chmod +x install_deps.sh
./install_deps.sh
```

3. **Build the project**
```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Quick Start

The fastest way to get started is to run one of the provided examples:

```bash
# Basic profiling example
./build/examples/basic_example

# Interactive visualization
./build/examples/imgui_example

# Full profiler application
./build/examples/profiler_app
```

## Basic Profiling

### Profiling a Function

The simplest way to profile a function is using the `RUNSCOPE_PROFILE_FUNCTION()` macro:

```cpp
#include "runscope/runscope_v2.hpp"

void my_function() {
    RUNSCOPE_PROFILE_FUNCTION();
    // Your code here
}
```

This automatically profiles the entire function from entry to exit.

### Profiling a Scope

For more granular control, use `RUNSCOPE_PROFILE_SCOPE()`:

```cpp
void complex_function() {
    {
        RUNSCOPE_PROFILE_SCOPE("initialization");
        // initialization code
    }
    
    {
        RUNSCOPE_PROFILE_SCOPE("processing");
        // processing code
    }
    
    {
        RUNSCOPE_PROFILE_SCOPE("cleanup");
        // cleanup code
    }
}
```

### Session Management

Always wrap your profiling in a session:

```cpp
int main() {
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    
    // Start a profiling session
    profiler.begin_session("MyApplication");
    
    // Run your code
    my_function();
    
    // End the session
    profiler.end_session();
    
    return 0;
}
```

### Getting Results

After profiling, retrieve the results:

```cpp
auto entries = profiler.get_entries();

for (const auto& entry : entries) {
    std::cout << entry.name << ": " 
              << entry.duration_ms() << " ms\n";
}
```

## Advanced Features

### Multi-threaded Profiling

RunScope automatically handles multi-threaded code:

```cpp
void worker_thread(int id) {
    RUNSCOPE_PROFILE_FUNCTION();
    // Thread work
}

int main() {
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    profiler.begin_session("MultithreadedApp");
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    profiler.end_session();
    
    // Analyze per-thread activity
    auto session = profiler.current_session();
    auto thread_info = session->get_thread_info();
    
    for (const auto& [tid, info] : thread_info) {
        std::cout << "Thread " << tid << ": " 
                  << info.entry_count << " entries\n";
    }
    
    return 0;
}
```

### Nested Scope Profiling

RunScope automatically tracks nesting depth:

```cpp
void outer() {
    RUNSCOPE_PROFILE_FUNCTION();
    inner();
}

void inner() {
    RUNSCOPE_PROFILE_FUNCTION();
    // Will show as nested under outer()
}
```

The depth information is automatically captured and can be visualized in the flame graph.

### Conditional Profiling

Enable or disable profiling at runtime:

```cpp
auto& profiler = runscope::core::ProfilerEngine::getInstance();

// Disable profiling
profiler.set_enabled(false);

// This won't be profiled
expensive_function();

// Re-enable profiling
profiler.set_enabled(true);

// This will be profiled
another_function();
```

### Statistical Analysis

Use the StatisticsAnalyzer for detailed insights:

```cpp
#include "runscope/runscope_v2.hpp"

int main() {
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    profiler.begin_session("Analysis");
    
    // Run your code
    for (int i = 0; i < 100; ++i) {
        my_function();
    }
    
    profiler.end_session();
    
    // Analyze
    auto entries = profiler.get_entries();
    runscope::analysis::StatisticsAnalyzer analyzer;
    analyzer.analyze(entries);
    
    // Get top 10 hot spots
    auto hotspots = analyzer.get_hotspots(10);
    
    std::cout << "Top 10 Hot Spots:\n";
    for (const auto& func : hotspots) {
        std::cout << func.name << ":\n"
                  << "  Calls: " << func.call_count << "\n"
                  << "  Total: " << func.total_time_ns / 1e6 << " ms\n"
                  << "  Avg: " << func.avg_time_ns / 1e6 << " ms\n"
                  << "  Min: " << func.min_time_ns / 1e6 << " ms\n"
                  << "  Max: " << func.max_time_ns / 1e6 << " ms\n";
    }
    
    return 0;
}
```

## Using the UI

### Main Window

The profiler UI provides several panels:

1. **Menu Bar**: Access all profiler functions
2. **Live Dashboard**: Real-time performance metrics
3. **Timeline View**: Horizontal timeline of function calls
4. **Flame Graph**: Hierarchical call visualization
5. **Call Tree**: Tree view of function relationships
6. **Hot Spots**: Functions sorted by time consumption
7. **Thread View**: Per-thread activity
8. **Statistics**: Detailed function statistics

### Navigation

- **Zoom**: Use the zoom slider in the timeline view
- **Pan**: Drag in the timeline to pan horizontally
- **Select**: Click on any entry to view details
- **Filter**: Use the filter box in the menu bar
- **Sort**: Click column headers in tables to sort

### Menu Options

**File Menu**
- Export JSON: Save complete profile data
- Export CSV: Save for spreadsheet analysis
- Export Chrome Trace: Open in chrome://tracing
- Import Session: Load previous session

**View Menu**
- Toggle visibility of all panels
- Show/hide specific views
- Reset zoom and pan

**Profiler Menu**
- Start/Stop Recording
- Clear Data: Reset all profiling data
- Process Selector: Choose processes to profile
- Attach to Process: Profile external applications
- Settings: Configure profiler options

**Tools Menu**
- Clear Selection: Deselect all entries
- Reset Zoom: Return to default view
- Memory Profiler: (Coming soon)
- CPU Monitor: (Coming soon)

### Keyboard Shortcuts

- `Ctrl+E`: Export JSON
- `Ctrl+C`: Clear data
- `Ctrl+F`: Focus filter box
- `Ctrl+R`: Reset zoom
- `Escape`: Clear selection

## Process Attachment

### Enumerating Processes

List all running processes:

```cpp
#include "runscope/runscope_v2.hpp"

int main() {
    auto processes = runscope::platform::ProcessEnumerator::enumerate_processes();
    
    std::cout << "Running Processes:\n";
    for (const auto& proc : processes) {
        std::cout << "PID: " << proc.pid 
                  << ", Name: " << proc.name
                  << ", Path: " << proc.executable_path << "\n";
    }
    
    return 0;
}
```

### Attaching to a Process

Attach to an external process for profiling:

```cpp
#include "runscope/runscope_v2.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <pid>\n";
        return 1;
    }
    
    runscope::core::ProcessId pid = std::stoi(argv[1]);
    
    runscope::platform::ProcessAttacher attacher;
    
    // Attach to the process
    if (!attacher.attach(pid)) {
        std::cerr << "Failed to attach: " << attacher.last_error() << "\n";
        return 1;
    }
    
    std::cout << "Attached to PID " << pid << "\n";
    
    // Configure sampling
    attacher.set_sample_rate(100);  // 100 samples per second
    attacher.start_sampling();
    
    std::cout << "Sampling for 10 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    attacher.stop_sampling();
    attacher.detach();
    
    std::cout << "Detached\n";
    
    return 0;
}
```

### Platform Notes

**Linux**
- Requires `ptrace` capability
- May need `sudo` or `CAP_SYS_PTRACE`
- Works with any process you have permission to trace

**macOS**
- Requires entitlements or `sudo`
- Uses `task_for_pid` Mach call
- May need to disable System Integrity Protection (SIP) for some system processes

## Data Analysis

### Function Statistics

Get detailed statistics for all functions:

```cpp
runscope::analysis::StatisticsAnalyzer analyzer;
analyzer.analyze(entries);

auto all_stats = analyzer.get_function_stats();

for (const auto& [name, stats] : all_stats) {
    std::cout << name << ":\n"
              << "  Call Count: " << stats.call_count << "\n"
              << "  Total Time: " << stats.total_time_ns / 1e6 << " ms\n"
              << "  Average: " << stats.avg_time_ns / 1e6 << " ms\n"
              << "  Min: " << stats.min_time_ns / 1e6 << " ms\n"
              << "  Max: " << stats.max_time_ns / 1e6 << " ms\n\n";
}
```

### Finding Hot Spots

Identify performance bottlenecks:

```cpp
auto hotspots = analyzer.get_hotspots(10);

double total_time = analyzer.total_profiled_time_ns();

std::cout << "Performance Hot Spots:\n";
for (const auto& func : hotspots) {
    double percentage = (func.total_time_ns / total_time) * 100.0;
    std::cout << func.name << ": " 
              << percentage << "% of total time\n";
}
```

### Thread Analysis

Analyze per-thread performance:

```cpp
auto session = profiler.current_session();
auto thread_info = session->get_thread_info();

for (const auto& [tid, info] : thread_info) {
    std::cout << "Thread " << tid << ":\n"
              << "  Entry Count: " << info.entry_count << "\n"
              << "  Total Time: " << info.total_time_ns / 1e6 << " ms\n";
}
```

## Export and Import

### JSON Export

Export complete profile data:

```cpp
auto entries = profiler.get_entries();
bool success = runscope::export_format::Exporter::export_to_json(
    entries, "profile.json"
);

if (success) {
    std::cout << "Exported to profile.json\n";
}
```

JSON format includes:
- Function names
- File and line information
- Start/end timestamps
- Duration in nanoseconds and milliseconds
- Thread IDs
- Nesting depth
- Memory and CPU usage (when available)

### CSV Export

Export for spreadsheet analysis:

```cpp
bool success = runscope::export_format::Exporter::export_to_csv(
    entries, "profile.csv"
);
```

CSV columns:
- Name
- File
- Line
- Start (ns)
- End (ns)
- Duration (ns)
- Duration (ms)
- Thread ID
- Depth
- Memory
- CPU

### Chrome Trace Format

Export for visualization in Chrome DevTools:

```cpp
bool success = runscope::export_format::Exporter::export_to_chrome_trace(
    entries, "profile.trace.json"
);
```

Open in Chrome:
1. Navigate to `chrome://tracing`
2. Click "Load"
3. Select your trace file

## Troubleshooting

### Build Issues

**Problem**: CMake can't find OpenGL
```
Solution: Install OpenGL development files
Ubuntu: sudo apt-get install libgl1-mesa-dev
macOS: OpenGL is included in Xcode Command Line Tools
```

**Problem**: GLFW build fails on Linux
```
Solution: Install X11 development libraries
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

**Problem**: OpenGL version error on macOS
```
Solution: This is fixed in the new implementation (OpenGL 3.2+)
Make sure you're using the updated examples
```

### Runtime Issues

**Problem**: Process attachment fails
```
Solution: 
- Linux: Run with sudo or add CAP_SYS_PTRACE capability
- macOS: Run with sudo or add proper entitlements
```

**Problem**: UI window doesn't open
```
Solution:
- Check that you have a display server running
- Verify OpenGL support: glxinfo | grep "OpenGL version"
- Try running with --verbose flag for debug output
```

**Problem**: High profiling overhead
```
Solution:
- Profile larger scopes instead of very small functions
- Use sampling mode instead of instrumentation
- Disable profiling in release builds with compiler flags
```

For more information, see:
- [README.md](../README.md) - Quick start and API reference
- [examples/](../examples/) - Code examples
