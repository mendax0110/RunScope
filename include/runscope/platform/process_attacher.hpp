#pragma once

#include "runscope/core/types.hpp"
#include "runscope/core/profile_entry.hpp"
#include "process_info.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace runscope::platform
{
    // Callback type for receiving sampled profile entries
    using SampleCallback = std::function<void(const std::vector<core::ProfileEntry>&)>;

    class ProcessAttacher
    {
    public:
        ProcessAttacher();
        ~ProcessAttacher();

        bool attach(core::ProcessId pid) const;
        bool detach() const;

        [[nodiscard]] bool is_attached() const noexcept;
        [[nodiscard]] core::ProcessId attached_pid() const noexcept;
        [[nodiscard]] core::AttachmentStatus status() const noexcept;

        void set_sample_rate(int samples_per_second) const;
        [[nodiscard]] int sample_rate() const noexcept;

        void start_sampling() const;
        void stop_sampling() const;
        [[nodiscard]] bool is_sampling() const noexcept;
        void set_sample_callback(SampleCallback callback) const;

        [[nodiscard]] std::vector<core::ProfileEntry> get_sampled_entries() const;

        [[nodiscard]] std::string last_error() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
