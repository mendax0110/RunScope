#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <ranges>

namespace runscope
{
    struct ProcessInfo
    {
        std::string name;
        bool enabled;
        int call_count;
        double total_time_ms;
        double avg_time_ms;
        double min_time_ms;
        double max_time_ms;

        ProcessInfo()
            : enabled(true)
            , call_count(0)
            , total_time_ms(0.0)
            , avg_time_ms(0.0)
            , min_time_ms(std::numeric_limits<double>::max())
            , max_time_ms(0.0)
        {

        }

        explicit ProcessInfo(std::string n)
            : name(std::move(n))
            , enabled(true)
            , call_count(0)
            , total_time_ms(0.0)
            , avg_time_ms(0.0)
            , min_time_ms(std::numeric_limits<double>::max())
            , max_time_ms(0.0)
        {

        }
    };

    class ProcessManager
    {
    public:
        static ProcessManager& getInstance()
        {
            static ProcessManager manager;
            return manager;
        }

        void register_process(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (processes_.find(name) == processes_.end())
            {
                processes_[name] = ProcessInfo(name);
            }
        }

        void set_process_enabled(const std::string& name, bool enabled)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = processes_.find(name);
            if (it != processes_.end())
            {
                it->second.enabled = enabled;
            }
        }

        bool is_process_enabled(const std::string& name) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = processes_.find(name);
            return it != processes_.end() && it->second.enabled;
        }

        void update_statistics(const std::string& name, double duration_ms)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = processes_.find(name);
            if (it != processes_.end())
            {
                auto& info = it->second;
                info.call_count++;
                info.total_time_ms += duration_ms;
                info.avg_time_ms = info.total_time_ms / info.call_count;
                info.min_time_ms = std::min(info.min_time_ms, duration_ms);
                info.max_time_ms = std::max(info.max_time_ms, duration_ms);
            }
        }

        std::map<std::string, ProcessInfo> get_all_processes() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return processes_;
        }

        void clear_statistics()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto &info: processes_ | std::views::values)
            {
                info.call_count = 0;
                info.total_time_ms = 0.0;
                info.avg_time_ms = 0.0;
                info.min_time_ms = std::numeric_limits<double>::max();
                info.max_time_ms = 0.0;
            }
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            processes_.clear();
        }

    private:
        ProcessManager() = default;
        ~ProcessManager() = default;
        ProcessManager(const ProcessManager&) = delete;
        ProcessManager& operator=(const ProcessManager&) = delete;

        mutable std::mutex mutex_;
        std::map<std::string, ProcessInfo> processes_;
    };
}
