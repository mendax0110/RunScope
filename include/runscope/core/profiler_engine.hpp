#pragma once

#include "types.hpp"
#include "profile_entry.hpp"
#include "profiler_session.hpp"
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace runscope::core
{
    class ProfilerEngine
    {
    public:
        static ProfilerEngine& getInstance();

        void begin_session(const std::string& name, ProfilerMode mode = ProfilerMode::Instrumentation);
        void end_session() const;

        void record_entry(ProfileEntry entry) const;

        bool is_active() const noexcept;
        ProfilerMode mode() const noexcept;

        std::shared_ptr<ProfilerSession> current_session();
        std::vector<ProfileEntry> get_entries() const;

        void clear() const;

        void set_enabled(bool enabled) noexcept;
        bool is_enabled() const noexcept;

    private:
        ProfilerEngine() = default;
        ~ProfilerEngine() = default;
        ProfilerEngine(const ProfilerEngine&) = delete;
        ProfilerEngine& operator=(const ProfilerEngine&) = delete;

        std::shared_ptr<ProfilerSession> current_session_;
        mutable std::mutex mutex_;
        std::atomic<bool> enabled_{true};
        ProfilerMode mode_{ProfilerMode::Instrumentation};
    };
}

