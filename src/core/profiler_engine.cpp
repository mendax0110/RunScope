#include "runscope/core/profiler_engine.hpp"

using namespace runscope::core;

ProfilerEngine& ProfilerEngine::getInstance()
{
    static ProfilerEngine engine;
    return engine;
}

void ProfilerEngine::begin_session(const std::string& name, ProfilerMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = mode;
    current_session_ = std::make_shared<ProfilerSession>(name);
}

void ProfilerEngine::end_session() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_session_)
    {
        current_session_->end();
    }
}

void ProfilerEngine::record_entry(ProfileEntry entry) const
{
    if (!enabled_.load(std::memory_order_acquire))
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_session_ && current_session_->is_active())
    {
        current_session_->add_entry(std::move(entry));
    }
}

bool ProfilerEngine::is_active() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_session_ && current_session_->is_active();
}

ProfilerMode ProfilerEngine::mode() const noexcept
{
    return mode_;
}

std::shared_ptr<ProfilerSession> ProfilerEngine::current_session()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_session_;
}

std::vector<ProfileEntry> ProfilerEngine::get_entries() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_session_)
    {
        return current_session_->get_entries();
    }
    return {};
}

void ProfilerEngine::clear() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_session_)
    {
        current_session_->clear();
    }
}

void ProfilerEngine::set_enabled(bool enabled) noexcept
{
    enabled_.store(enabled, std::memory_order_release);
}

bool ProfilerEngine::is_enabled() const noexcept
{
    return enabled_.load(std::memory_order_acquire);
}