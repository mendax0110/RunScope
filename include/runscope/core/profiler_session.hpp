#pragma once

#include "types.hpp"
#include "profile_entry.hpp"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>


namespace runscope::core
{
    class ProfilerSession
    {
    public:
        explicit ProfilerSession(std::string name);
        ~ProfilerSession();

        const std::string& name() const noexcept { return name_; }
        TimePoint start_time() const noexcept { return start_time_; }
        TimePoint end_time() const noexcept { return end_time_; }

        bool is_active() const noexcept { return active_; }
        void set_active(bool active) noexcept { active_ = active; }

        void add_entry(ProfileEntry entry);
        void add_entry_mt(ProfileEntry entry);

        std::vector<ProfileEntry> get_entries() const;
        std::vector<ProfileEntry> get_entries_mt() const;

        std::map<ThreadId, ThreadInfo> get_thread_info() const;
        std::map<std::string, uint64_t> get_memory_usage() const;
        std::map<std::string, double> get_cpu_usage() const;

        void clear();
        size_t entry_count() const;

        void end();

    private:
        std::string name_;
        TimePoint start_time_;
        TimePoint end_time_;
        bool active_;

        std::vector<ProfileEntry> entries_;
        mutable std::mutex mutex_;
    };
}

