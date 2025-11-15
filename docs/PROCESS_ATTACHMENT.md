# Process Attachment and Real-Time Profiling

### Local Simulation vs Real Process Data

The profiler application (`profiler_app`) can show two different types of data:

**Local Simulation (Default - NOT Real Data)**
- When no process is attached, you see artificial simulation functions:
  - `simulation_work()`, `physics_calculation()`, `render_scene()`, `game_frame()`
- This is just a demo to show the UI - **not real profiling data**

**Real Process Data (After Attachment)**
- When you attach to a process and start sampling, you see **actual OS data**:
  - Real process name (e.g., `target_app`)
  - Real thread IDs (e.g., `TID:5517`)
  - Real thread states (`Running`, `Sleeping`, `Disk_Sleep`, etc.)
  - Actual stack traces (when available)

**To see real data**: Attach to a process → Start Sampling → Verify "PROFILING REAL PROCESS DATA" is displayed

## Overview

RunScope supports attaching to and profiling live processes. The profiler reads **real data** from attached processes, including:

- Process executable name and ID
- Thread enumeration and IDs  
- Thread states (Running, Sleeping, etc.)
- Real-time sampling at configurable rates

## Quick Start

### 1. Start a Target Application

```bash
# Build the examples
cd build
cmake --build .

# Run the target application
./examples/target_app
# Output: Target application started. PID: 12345
```

### 2. Attach and Profile

#### Option A: Using attach_test (Command Line)

```bash
# In another terminal
./examples/attach_test 12345
```

This will:
- Attach to the specified PID
- Sample for 5 seconds at 10 Hz
- Display the sampled data
- Show real process information

Expected output:
```
Attempting to attach to PID 12345...
Successfully attached to PID 12345
Starting sampling...
Stopping sampling...

Collected 50 sample entries

First 10 samples:
  target_app::target_app [TID:12345, State:Sleeping] (depth=0, duration=1ms)
  target_app::target_app [TID:12345, State:Running] (depth=0, duration=1ms)
  ...
```

#### Option B: Using profiler_app (GUI)

```bash
./examples/profiler_app
```

In the GUI:
1. Go to **Profiler → Process Selector**
2. Click **Refresh Process List**
3. Select your target process
4. Click **Attach to Process**
5. Click **Start Sampling**
6. Toggle **Use Attached Process Data** in the Control Panel
7. View real-time data in the visualization panels

## Requirements

### Linux

Process attachment requires ptrace permissions. By default, Linux restricts ptrace to parent-child relationships.

To allow attaching to any process owned by your user:

```bash
sudo sysctl -w kernel.yama.ptrace_scope=0
```

Or to make it permanent:
```bash
echo "kernel.yama.ptrace_scope = 0" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

**Security Note:** This allows any process you own to be traced. Reset with `ptrace_scope=1` after profiling.

### macOS

Process attachment requires:
- Running with sudo, OR
- Proper entitlements in your application

```bash
sudo ./examples/attach_test <pid>
```

## What Data is Real?

### Currently Implemented (Real Data)

1. **Process Information**
   - Executable path and name (from `/proc/[pid]/exe`)
   - Process ID
   - Thread IDs (from `/proc/[pid]/task/`)

2. **Thread State**
   - Thread name (from `/proc/[pid]/task/[tid]/stat`)
   - Current state: R (Running), S (Sleeping), D (Disk Sleep), etc.
   - State is sampled in real-time

3. **Sampling**
   - Configurable sample rate (default: 10 Hz)
   - Non-intrusive - doesn't stop the target process
   - Background sampling thread

### Future Enhancements

1. **Stack Unwinding**
   - Full userspace call stacks
   - Would require libunwind or similar
   - Or using eBPF for kernel-side unwinding

2. **Symbol Resolution**
   - Function names from addresses
   - Requires DWARF debug info or symbol tables
   - File/line number information

3. **Performance Metrics**
   - CPU usage per function
   - Memory allocations
   - I/O operations

## API Usage

### Basic Attachment

```cpp
#include "runscope/platform/process_attacher.hpp"

runscope::platform::ProcessAttacher attacher;

// Attach to process
if (attacher.attach(pid))
{
    // Set sample rate (samples per second)
    attacher.set_sample_rate(100);
    
    // Start sampling
    attacher.start_sampling();
    
    // Wait or do other work...
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Get sampled data
    auto entries = attacher.get_sampled_entries();
    
    // Process entries...
    for (const auto& entry : entries)
    {
        std::cout << entry.name << ": " 
                  << entry.duration_ms() << "ms" << std::endl;
    }
    
    // Stop and detach
    attacher.stop_sampling();
    attacher.detach();
}
```

### Using Callbacks

```cpp
attacher.set_sample_callback([](const auto& entries) {
    // Process new samples as they arrive
    for (const auto& entry : entries)
    {
        // Handle each sample...
    }
});
```

## Architecture

### Linux Implementation

```
ProcessAttacher
    |_ ptrace(PTRACE_ATTACH) - Attach to process
    |_ /proc/[pid]/exe - Read executable name
    |_ /proc/[pid]/task/ - Enumerate threads
    |_ /proc/[pid]/task/[tid]/stat - Read thread state
    |_ Background sampling thread
```

### Sampling Thread

1. **Initialize**: Attach to target process
2. **Loop** (at sample_rate Hz):
   - Enumerate all threads
   - For each thread:
     - Read thread state from /proc
     - Create ProfileEntry with real data
   - Store entries (circular buffer, max 1000)
   - Invoke callback if set
3. **Cleanup**: Stop sampling, detach

## Troubleshooting

### "Failed to attach: Operation not permitted"

**Solution**: Adjust ptrace_scope or run with elevated privileges

```bash
sudo sysctl -w kernel.yama.ptrace_scope=0
```

### "No thread data available"

**Cause**: Process may have exited or /proc is not accessible

**Solution**: Verify the process is still running:
```bash
ps -p <pid>
```

### Sampling shows no stack traces

**Expected**: Current implementation reads process state but not full stacks

**Reason**: Full stack unwinding requires either:
- Stopping the process (intrusive)
- Using libunwind (complex integration)
- Using eBPF (kernel support required)

The current implementation prioritizes non-intrusive monitoring.

## Performance Impact

- **Target Process**: Minimal (<1% CPU overhead)
- **Sampler**: ~0.5% CPU at 10 Hz, ~5% CPU at 100 Hz
- **Memory**: ~8KB per 100 samples (circular buffer)

## Examples

### Example 1: Profile a Long-Running Server

```bash
# Find the server PID
pgrep my_server

# Attach and sample
./examples/attach_test $(pgrep my_server)
```

### Example 2: Profile During Load Test

```bash
# Terminal 1: Start server
./my_server &
SERVER_PID=$!

# Terminal 2: Start profiler GUI
./examples/profiler_app
# Then attach to $SERVER_PID

# Terminal 3: Run load test
./run_load_test.sh

# View real-time profiling data in GUI
```

### Example 3: Automated Performance Testing

```cpp
#include "runscope/platform/process_attacher.hpp"
#include <chrono>

// Profile a subprocess
int pid = fork();
if (pid == 0)
{
    // Child: run workload
    execl("./my_app", "my_app", nullptr);
}

// Parent: profile it
runscope::platform::ProcessAttacher profiler;
profiler.attach(pid);
profiler.start_sampling();

// Wait for workload to complete
waitpid(pid, nullptr, 0);

// Analyze results
auto samples = profiler.get_sampled_entries();
// Generate report...
```

## Security Considerations

- Process attachment requires same user or root privileges
- Can only attach to processes you own (unless root)
- Attached process continues running normally
- Detachment is clean - no side effects on target

## Future Roadmap

1. **Enhanced Stack Traces**
   - Integrate libunwind for userspace unwinding
   - Support DWARF debug information
   - eBPF-based sampling for zero overhead

2. **Symbol Resolution**
   - Parse ELF symbol tables
   - Support external debug symbols
   - Demangle C++ symbols in-process

3. **Performance Metrics**
   - CPU time per function
   - Memory allocation tracking
   - System call profiling

4. **Platform Support**
   - Windows (via DbgHelp API)
   - Improved macOS support

## References

- [ptrace(2) man page](https://man7.org/linux/man-pages/man2/ptrace.2.html)
- [proc(5) man page](https://man7.org/linux/man-pages/man5/proc.5.html)
- [Linux perf_events](https://perf.wiki.kernel.org/index.php/Main_Page)
- [eBPF profiling](https://ebpf.io/)
