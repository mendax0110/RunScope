#pragma once

#include "profile_data.hpp"
#include "process_manager.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>
#include <optional>

namespace runscope
{
    class ImGuiVisualizer
    {
    public:
        ImGuiVisualizer()
            : show_timeline_(true)
            , show_flamegraph_(false)
            , show_statistics_(true)
            , show_process_menu_(true)
            , show_live_view_(true)
            , zoom_level_(1.0f)
            , pan_offset_(0.0f)
            , selected_entry_(std::nullopt)
            , auto_zoom_(true)
            , filter_text_("")
        {

        }

        void render(const std::vector<ProfileEntry>& entries);

        void render_menu_bar();
        void render_process_selection_menu();
        void render_timeline(const std::vector<ProfileEntry>& entries);
        void render_flamegraph(const std::vector<ProfileEntry>& entries);
        void render_statistics(const std::vector<ProfileEntry>& entries);
        void render_live_view(const std::vector<ProfileEntry>& entries);
        void render_details_panel(const std::vector<ProfileEntry>& entries);

        void set_show_timeline(const bool show) { show_timeline_ = show; }
        void set_show_flamegraph(const bool show) { show_flamegraph_ = show; }
        void set_show_statistics(const bool show) { show_statistics_ = show; }
        void set_show_process_menu(const bool show) { show_process_menu_ = show; }
        void set_show_live_view(const bool show) { show_live_view_ = show; }

    private:
        bool show_timeline_;
        bool show_flamegraph_;
        bool show_statistics_;
        bool show_process_menu_;
        bool show_live_view_;
        float zoom_level_;
        float pan_offset_;
        std::optional<size_t> selected_entry_;
        std::string highlighted_function_;
        bool auto_zoom_;
        char filter_text_[256];

        struct ThreadEntries
        {
            std::thread::id thread_id;
            std::vector<const ProfileEntry*> entries;
        };

        [[nodiscard]] static std::vector<ThreadEntries> group_by_thread(const std::vector<ProfileEntry>& entries);
        [[nodiscard]] std::vector<ProfileEntry> filter_entries(const std::vector<ProfileEntry>& entries) const;
        bool draw_entry_rect(const ProfileEntry& entry, size_t entry_idx, float x, float y,
                            float width, float height, int depth, bool is_selected, bool is_highlighted);

        static void render_mini_timeline(const std::vector<ProfileEntry>& entries);
    };
}
