#pragma once

#include "runscope/core/profile_entry.hpp"
#include "runscope/analysis/statistics.hpp"
#include "runscope/platform/process_info.hpp"
#include "runscope/platform/process_attacher.hpp"
#include <vector>
#include <string>
#include <memory>
#include <optional>

// Forward declaration for ImGui types
struct ImVec2;

namespace runscope::ui
{
    class ProfilerUI
    {
    public:
        ProfilerUI();
        ~ProfilerUI();

        void render(const std::vector<core::ProfileEntry>& entries) const;

        void show_main_window(bool* p_open = nullptr);
        void show_menu_bar() const;

        void show_live_dashboard() const;
        void show_timeline_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_flamegraph_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_call_tree_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_hot_spots_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_thread_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_statistics_view(const std::vector<core::ProfileEntry>& entries) const;
        void show_function_details() const;
        void show_process_selector() const;
        void show_process_attachment() const;
        void show_memory_profiler() const;
        void show_cpu_monitor() const;

        void show_export_dialog() const;

        void show_settings_dialog() const;

        void set_selected_entry(size_t index) const;
        [[nodiscard]] std::optional<size_t> selected_entry() const;
        
        // Get the internal process attacher for external use
        platform::ProcessAttacher& get_process_attacher() const;

    private:
        void render_timeline_entry(const core::ProfileEntry& entry, float row_height, int64_t time_range_ns, int64_t min_time_ns, ImVec2 canvas_pos, ImVec2 canvas_size, float base_y_offset, size_t entry_idx) const;
        void render_flamegraph_node(const core::ProfileEntry& entry, float x, float y, float width, float height, size_t entry_idx) const;

        static void render_call_tree_node(const core::ProfileEntry& entry, int depth);
        void update_statistics(const std::vector<core::ProfileEntry>& entries) const;

        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}