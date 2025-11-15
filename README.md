# RunScope - Professional C++ Profiler

A comprehensive, professional-grade C++20 profiler with advanced visualization capabilities using Dear ImGui. RunScope provides low-overhead performance profiling with thread-safe data collection, process attachment, statistical analysis, and an intuitive graphical interface.

## Features

### Core Profiling
- **RAII-based scope profiling** with minimal overhead
- **Thread-safe data collection** with lock-free timing
- **Session management** with start/stop controls
- **Nested scope tracking** with automatic depth calculation
- **High-resolution timing** using std::chrono::high_resolution_clock
- **Multiple profiling modes**: Instrumentation and Sampling (framework ready)

### Process Management
- **Process enumeration** for discovering running applications
- **Process attachment** to profile external applications
  - Linux: ptrace-based attachment
  - macOS: task_for_pid-based attachment (requires entitlements or sudo)
- **Real-time sampling** from attached processes

### Data Analysis
- **Statistical analysis** with function-level metrics
- **Hot spots detection** identifying performance bottlenecks
- **Call count tracking** and aggregation
- **Time distribution** (min, max, average, total)
- **Thread-level analysis** with per-thread metrics

### Visualization (Dear ImGui)
- **Live Performance Dashboard** with real-time metrics
- **Timeline View** with zoom and pan controls
- **Flame Graph** for hierarchical call visualization
- **Call Tree View** showing function relationships
- **Hot Spots Analysis** with sortable tables
- **Thread Activity View** for multi-threaded analysis
- **Statistics Panel** with detailed function metrics
- **Process Selector** with filterable process list
- **Function Details** panel with in-depth information

### Export & Import
- **JSON format** with full profile data
- **CSV format** for spreadsheet analysis
- **Chrome Trace format** for chrome://tracing
- **Session save/load** (framework ready)

### Cross-Platform Support
- **Ubuntu/Linux** with native process enumeration
- **macOS** with proper OpenGL 3.2+ support
- **Platform-specific optimizations** where applicable

## Requirements

- **CMake 3.20+**
- **C++20 compatible compiler**
  - GCC 10+
  - Clang 11+
  - MSVC 2019+
- **OpenGL** (for ImGui visualization)
- **X11 development libraries** (Linux only)

## Building

### Install Dependencies

```bash
chmod +x install_deps.sh
./install_deps.sh
```

The script automatically installs required dependencies for your platform:
- **Ubuntu/Debian**: build-essential, cmake, OpenGL, X11 libraries
- **macOS**: cmake, glfw via Homebrew

### Build the Project

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Build Options

```bash
cmake -DRUNSCOPE_BUILD_TESTS=ON \      # Build unit tests (default: ON)
      -DRUNSCOPE_BUILD_EXAMPLES=ON \    # Build examples (default: ON)
      -DRUNSCOPE_BUILD_IMGUI=ON \       # Build ImGui UI (default: ON)
      ..
```

## Usage

### Basic Profiling

```cpp
#include "runscope/runscope_v2.hpp"

void expensive_function() {
    RUNSCOPE_PROFILE_FUNCTION();
    // Your code here
}

void custom_scope() {
    RUNSCOPE_PROFILE_SCOPE("custom_operation");
    // Code to profile
}

int main() {
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    profiler.begin_session("MySession");
    
    expensive_function();
    custom_scope();
    
    profiler.end_session();
    
    // Export results
    auto entries = profiler.get_entries();
    runscope::export_format::Exporter::export_to_json(entries, "profile.json");
    runscope::export_format::Exporter::export_to_csv(entries, "profile.csv");
    
    return 0;
}
```

### Process Attachment

```cpp
#include "runscope/runscope_v2.hpp"

int main() {
    // Enumerate processes
    auto processes = runscope::platform::ProcessEnumerator::enumerate_processes();
    
    for (const auto& proc : processes) {
        std::cout << "PID: " << proc.pid << ", Name: " << proc.name << "\n";
    }
    
    // Attach to a process
    runscope::platform::ProcessAttacher attacher;
    if (attacher.attach(1234)) {
        std::cout << "Attached successfully\n";
        attacher.start_sampling();
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        attacher.stop_sampling();
        attacher.detach();
    } else {
        std::cerr << "Failed: " << attacher.last_error() << "\n";
    }
    
    return 0;
}
```

### Statistical Analysis

```cpp
#include "runscope/runscope_v2.hpp"

int main() {
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    profiler.begin_session("Analysis");
    
    // ... run your code ...
    
    auto entries = profiler.get_entries();
    
    runscope::analysis::StatisticsAnalyzer analyzer;
    analyzer.analyze(entries);
    
    // Get top 10 functions by time
    auto hotspots = analyzer.get_hotspots(10);
    
    for (const auto& func : hotspots) {
        std::cout << func.name << ": " 
                  << func.total_time_ns / 1e6 << " ms, "
                  << func.call_count << " calls\n";
    }
    
    profiler.end_session();
    return 0;
}
```

### ImGui Profiler Application

```cpp
#include "runscope/runscope_v2.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

int main() {
    // Initialize GLFW and ImGui
    // (See examples/profiler_app.cpp for complete setup)
    
    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    profiler.begin_session("MainSession");
    
    runscope::ui::ProfilerUI ui;
    
    while (!glfwWindowShouldClose(window)) {
        // Poll events and start ImGui frame
        
        auto entries = profiler.get_entries();
        ui.render(entries);
        
        // Render ImGui
    }
    
    return 0;
}
```

## Running Examples

### Basic Example
Demonstrates core profiling with statistics and export:
```bash
./build/examples/basic_example
```

### ImGui Example (Original)
Interactive visualization with the original UI:
```bash
./build/examples/imgui_example
```

### Profiler Application (New)
Comprehensive profiler with full UI and process attachment:
```bash
# Run with sudo for process attachment (ptrace permission required)
sudo ./build/examples/profiler_app
```

**Note**: By default, the profiler shows a local simulation (artificial data). To profile a real process:
1. Run a target application: `./build/examples/target_app`
2. In profiler_app, use **Profiler → Process Selector** to find and attach
3. Click **Start Sampling** to collect real process data
4. Verify "PROFILING REAL PROCESS DATA" status is shown

See [docs/PROCESS_ATTACHMENT.md](docs/PROCESS_ATTACHMENT.md) for details.

## Project Structure

```
RunScope/
|__ include/runscope/
|   |__ core/                    # Core profiling engine
|   |   |__ types.hpp           # Common types and enums
|   |   |__ clock.hpp           # High-resolution timing
|   |   |__ profile_entry.hpp   # Profile data structures
|   |   |__ profiler_session.hpp # Session management
|   |   |__ profiler_engine.hpp # Main profiler singleton
|   |   |__ scope_profiler.hpp  # RAII scope profiler
|   |__ platform/               # Platform-specific code
|   |   |__ process_info.hpp    # Process information
|   |   |__ process_attacher.hpp # Process attachment
|   |__ analysis/               # Analysis and statistics
|   |   |__ statistics.hpp      # Statistical analysis
|   |__ export/                 # Export formats
|   |   |__ exporter.hpp        # JSON, CSV, Chrome Trace
|   |__ ui/                     # ImGui user interface
|   |   |__ profiler_ui.hpp     # Comprehensive profiler UI
|   |__ runscope_v2.hpp         # New unified header
|   |__ runscope.hpp            # Legacy compatibility header
|__ src/                        # Implementation files
|   |__ core/
|   |__ platform/
|   |__ analysis/
|   |__ export/
|   |__ ui/
|__ examples/                   # Example applications
|   |__ basic_example.cpp
|   |__ imgui_example.cpp
|   |__ profiler_app.cpp        # New comprehensive app
|__ tests/                      # Unit tests
```

## UI Features

### Menu Bar
- **File**: Export (JSON, CSV, Chrome Trace), Import, Exit
- **View**: Toggle all visualization panels
- **Profiler**: Recording controls, Process selection, Attachment, Settings
- **Tools**: Selection management, Zoom controls, Memory profiler, CPU monitor
- **Help**: About, Documentation

### Visualization Panels
1. **Live Dashboard**: Real-time FPS, frame time, sample count, top functions
2. **Timeline View**: Horizontal timeline with zoom/pan, color-coded by function
3. **Flame Graph**: Hierarchical visualization of call stacks
4. **Call Tree**: Tree view of function calls with children
5. **Hot Spots**: Sortable table of functions by time consumption
6. **Thread View**: Per-thread activity and statistics
7. **Statistics**: Complete function statistics with filtering
8. **Function Details**: Detailed information about selected functions

### Process Management
- Process list with PID, name, and path
- Real-time process enumeration
- Attachment status and controls
- Error reporting and diagnostics

## Platform-Specific Notes

### macOS
- Uses OpenGL 3.2+ Core Profile (required by macOS)
- Process attachment requires entitlements or sudo privileges
- Uses `task_for_pid` for process attachment

### Linux
- Uses OpenGL 3.3+ by default
- Process attachment uses `ptrace`
- May require `CAP_SYS_PTRACE` capability or sudo for attachment

## Performance

RunScope is designed for minimal overhead:
- Header-only core API (zero linking overhead)
- RAII-based profiling (automatic start/stop)
- Lock-free high-resolution clock
- Thread-safe with minimal contention
- Efficient data structures for large-scale profiling

## Testing

Run all unit tests:
```bash
cd build
ctest --output-on-failure
```

Or run the test executable directly:
```bash
./build/tests/runscope_tests
```

## API Reference

### Core Profiling Macros
- `RUNSCOPE_PROFILE_FUNCTION()` - Profile current function
- `RUNSCOPE_PROFILE_SCOPE("name")` - Profile named scope

### ProfilerEngine
```cpp
auto& profiler = runscope::core::ProfilerEngine::getInstance();
profiler.begin_session("SessionName");
profiler.end_session();
profiler.get_entries();
profiler.clear();
profiler.is_active();
profiler.set_enabled(bool);
```

### ProcessEnumerator
```cpp
auto processes = runscope::platform::ProcessEnumerator::enumerate_processes();
auto info = runscope::platform::ProcessEnumerator::get_process_info(pid);
bool running = runscope::platform::ProcessEnumerator::is_process_running(pid);
```

### ProcessAttacher
```cpp
runscope::platform::ProcessAttacher attacher;
attacher.attach(pid);
attacher.detach();
attacher.start_sampling();
attacher.stop_sampling();
attacher.set_sample_rate(100);
```

### StatisticsAnalyzer
```cpp
runscope::analysis::StatisticsAnalyzer analyzer;
analyzer.analyze(entries);
auto stats = analyzer.get_function_stats();
auto hotspots = analyzer.get_hotspots(10);
```

### Exporter
```cpp
runscope::export_format::Exporter::export_to_json(entries, "file.json");
runscope::export_format::Exporter::export_to_csv(entries, "file.csv");
runscope::export_format::Exporter::export_to_chrome_trace(entries, "file.json");
```

## License

MIT License. See LICENSE file for details.
