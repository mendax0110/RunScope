#pragma once

#include "timer.hpp"
#include "profile_data.hpp"
#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <memory>
#include <atomic>

namespace runscope
{
    class Profiler
    {
    public:
        static Profiler& getInstance()
        {
            static Profiler profiler;
            return profiler;
        }

        void begin_session(const std::string_view name)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            session_name_ = name;
            entries_.clear();
            session_start_ = Timer::Clock::now();
            active_ = true;
        }

        void end_session()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_ = false;
        }

        void record_entry(ProfileEntry entry)
        {
            if (!active_) return;

            std::lock_guard<std::mutex> lock(mutex_);
            entries_.push_back(std::move(entry));
        }

        [[nodiscard]] std::vector<ProfileEntry> get_entries() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return entries_;
        }

        [[nodiscard]] std::string get_session_name() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return session_name_;
        }

        [[nodiscard]] bool is_active() const noexcept
        {
            return active_.load(std::memory_order_acquire);
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            entries_.clear();
        }

        [[nodiscard]] int64_t get_session_start_ns() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return std::chrono::duration_cast<std::chrono::nanoseconds>(session_start_.time_since_epoch()).count();
        }

    private:
        Profiler() = default;
        ~Profiler() = default;
        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;

        mutable std::mutex mutex_;
        std::vector<ProfileEntry> entries_;
        std::string session_name_;
        Timer::TimePoint session_start_;
        std::atomic<bool> active_{false};
    };

    class ScopeProfiler
    {
    public:
        explicit ScopeProfiler(const std::string_view name)
            : name_(name)
            , timer_()
            , depth_(get_depth())
        {
            increment_depth();
            start_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(timer_.start_time().time_since_epoch()).count();
        }

        ~ScopeProfiler()
        {
            const auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Timer::Clock::now().time_since_epoch()).count();

            ProfileEntry entry{
                std::string(name_),
                start_ns_,
                end_ns,
                std::this_thread::get_id(),
                depth_
            };

            Profiler::getInstance().record_entry(std::move(entry));
            decrement_depth();
        }

        ScopeProfiler(const ScopeProfiler&) = delete;
        ScopeProfiler& operator=(const ScopeProfiler&) = delete;

    private:
        static int& get_depth_ref()
        {
            thread_local int depth = 0;
            return depth;
        }

        static int get_depth()
        {
            return get_depth_ref();
        }

        static void increment_depth()
        {
            ++get_depth_ref();
        }

        static void decrement_depth()
        {
            --get_depth_ref();
        }

        std::string_view name_;
        Timer timer_;
        int64_t start_ns_;
        int depth_;
    };
}

#define RUNSCOPE_CONCAT_IMPL(x, y) x##y
#define RUNSCOPE_CONCAT(x, y) RUNSCOPE_CONCAT_IMPL(x, y)
#define RUNSCOPE_PROFILE_SCOPE(name) \
    ::runscope::ScopeProfiler RUNSCOPE_CONCAT(__profiler_, __LINE__)(name)
#define RUNSCOPE_PROFILE_FUNCTION() RUNSCOPE_PROFILE_SCOPE(__FUNCTION__)
