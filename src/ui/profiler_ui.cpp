#include "runscope/ui/profiler_ui.hpp"
#include "runscope/analysis/statistics.hpp"
#include "runscope/platform/process_info.hpp"
#include "runscope/runscope_v2.hpp"
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include <cstring>
#include <iostream>

using namespace runscope::ui;

struct ProfilerUI::Impl
{
    analysis::StatisticsAnalyzer stats_analyzer;
    std::unique_ptr<platform::ProcessAttacher> attacher;
    
    bool show_live_dashboard_{true};
    bool show_timeline_{true};
    bool show_flamegraph_{true};
    bool show_call_tree_{false};
    bool show_hot_spots_{true};
    bool show_thread_view_{false};
    bool show_statistics_{true};
    bool show_function_details_{false};
    bool show_process_selector_{false};
    bool show_attachment_dialog_{false};
    bool show_details_panel_{false};
    bool show_export_dialog_{false};
    bool show_settings_dialog_{false};
    bool show_memory_profiler_{false};
    bool show_cpu_monitor_{false};
    
    std::optional<size_t> selected_entry_;
    std::string filter_text_;
    std::string highlighted_function_;
    float timeline_zoom_{1.0f};
    float timeline_offset_{0.0f};
    bool auto_zoom_{true};
    
    std::vector<platform::ProcessInfo> process_list_;
    core::ProcessId selected_pid_{0};
    std::vector<core::ProfileEntry> cached_entries_;
    runscope::export_format::Exporter exporter_;
    runscope::core::ProfilerSession current_session_;
    
    Impl() : attacher(std::make_unique<platform::ProcessAttacher>()), current_session_("UI Session")
    {
    }
};

ProfilerUI::ProfilerUI() : impl_(std::make_unique<Impl>()) {}
ProfilerUI::~ProfilerUI() = default;

void ProfilerUI::render(const std::vector<core::ProfileEntry>& entries) const
{
    // Cache entries for details panel
    impl_->cached_entries_ = entries;
    
    show_menu_bar();
    
    if (impl_->show_live_dashboard_)
    {
        show_live_dashboard();
    }
    
    if (impl_->show_timeline_)
    {
        show_timeline_view(entries);
    }
    
    if (impl_->show_flamegraph_)
    {
        show_flamegraph_view(entries);
    }
    
    if (impl_->show_call_tree_)
    {
        show_call_tree_view(entries);
    }
    
    if (impl_->show_hot_spots_)
    {
        show_hot_spots_view(entries);
    }
    
    if (impl_->show_thread_view_)
    {
        show_thread_view(entries);
    }
    
    if (impl_->show_statistics_)
    {
        show_statistics_view(entries);
    }
    
    if (impl_->show_details_panel_ && impl_->selected_entry_.has_value())
    {
        show_function_details();
    }
    
    if (impl_->show_process_selector_)
    {
        show_process_selector();
    }
    
    if (impl_->show_attachment_dialog_)
    {
        show_process_attachment();
    }

    if (impl_->show_export_dialog_)
    {
        show_export_dialog();
    }

    if (impl_->show_settings_dialog_)
    {
        show_settings_dialog();
    }

    if (impl_->show_memory_profiler_)
    {
        show_memory_profiler();
    }

    if (impl_->show_cpu_monitor_)
    {
        show_cpu_monitor();
    }
}

void ProfilerUI::show_menu_bar() const
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Export JSON"))
            {
                show_export_dialog();
            }
            if (ImGui::MenuItem("Export CSV"))
            {
                show_export_dialog();
            }
            if (ImGui::MenuItem("Export Chrome Trace"))
            {
                throw std::runtime_error("Export Chrome Trace not implemented yet");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Session"))
            {
                throw std::runtime_error("Import session not implemented yet");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                if (impl_->attacher->is_attached())
                {
                    impl_->attacher->detach();
                }
                exit(0);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Live Dashboard", nullptr, &impl_->show_live_dashboard_);
            ImGui::MenuItem("Timeline", nullptr, &impl_->show_timeline_);
            ImGui::MenuItem("Flame Graph", nullptr, &impl_->show_flamegraph_);
            ImGui::MenuItem("Call Tree", nullptr, &impl_->show_call_tree_);
            ImGui::MenuItem("Hot Spots", nullptr, &impl_->show_hot_spots_);
            ImGui::MenuItem("Thread View", nullptr, &impl_->show_thread_view_);
            ImGui::MenuItem("Statistics", nullptr, &impl_->show_statistics_);
            ImGui::Separator();
            ImGui::MenuItem("Function Details", nullptr, &impl_->show_details_panel_);
            ImGui::MenuItem("Auto Zoom", nullptr, &impl_->auto_zoom_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Profiler"))
        {
            if (ImGui::MenuItem("Start Recording"))
            {
                if (!impl_->attacher->is_sampling())
                {
                    impl_->attacher->start_sampling();
                }
            }
            if (ImGui::MenuItem("Stop Recording"))
            {
                if (impl_->attacher->is_sampling())
                {
                    impl_->attacher->stop_sampling();
                }
            }
            if (ImGui::MenuItem("Clear Data"))
            {
                impl_->selected_entry_ = std::nullopt;
                impl_->highlighted_function_ = "";
                impl_->show_details_panel_ = false;
            }
            ImGui::Separator();
            ImGui::MenuItem("Process Selector", nullptr, &impl_->show_process_selector_);
            ImGui::MenuItem("Attach to Process", nullptr, &impl_->show_attachment_dialog_);
            ImGui::Separator();
            if (ImGui::MenuItem("Settings"))
            {
                show_settings_dialog();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Clear Selection"))
            {
                impl_->selected_entry_ = std::nullopt;
                impl_->highlighted_function_ = "";
                impl_->show_details_panel_ = false;
            }
            if (ImGui::MenuItem("Reset Zoom"))
            {
                impl_->timeline_zoom_ = 1.0f;
                impl_->timeline_offset_ = 0.0f;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Memory Profiler"))
            {
                impl_->show_memory_profiler_ = true;
            }
            if (ImGui::MenuItem("CPU Monitor"))
            {
                impl_->show_cpu_monitor_ = true;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                throw std::runtime_error("About dialog not implemented yet");
            }
            if (ImGui::MenuItem("Documentation"))
            {
                throw std::runtime_error("Documentation link not implemented yet");
            }
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        char filter_buffer[256];
        strncpy(filter_buffer, impl_->filter_text_.c_str(), sizeof(filter_buffer) - 1);
        filter_buffer[sizeof(filter_buffer) - 1] = '\0';
        if (ImGui::InputText("##filter", filter_buffer, sizeof(filter_buffer)))
        {
            impl_->filter_text_ = filter_buffer;
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ProfilerUI::show_live_dashboard() const
{
    ImGui::Begin("Live Performance Dashboard", &impl_->show_live_dashboard_);
    
    const auto& entries = impl_->cached_entries_;
    
    ImGui::Text("Session Active: Yes");
    ImGui::Text("Recording: Yes");
    ImGui::Separator();
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    // Calculate metrics
    int64_t total_duration = 0;
    int64_t min_start = entries.front().start_ns;
    int64_t max_end = entries.front().end_ns;
    std::map<std::string, int64_t> function_durations;
    std::set<core::ThreadId> unique_threads;
    
    for (const auto& entry : entries)
    {
        total_duration += entry.duration_ns();
        min_start = std::min(min_start, entry.start_ns);
        max_end = std::max(max_end, entry.end_ns);
        function_durations[entry.name] += entry.duration_ns();
        unique_threads.insert(entry.thread_id);
    }
    
    int64_t session_duration = max_end - min_start;
    double fps = session_duration > 0 ? (1000000000.0 / session_duration) : 0.0;
    
    ImGui::Text("Session Duration: %.2f ms", session_duration / 1000000.0);
    ImGui::Text("Total Entries: %zu", entries.size());
    ImGui::Text("Cumulative Time: %.2f ms", total_duration / 1000000.0);
    ImGui::Text("Active Threads: %zu", unique_threads.size());
    if (session_duration > 0)
    {
        ImGui::Text("Estimated FPS: %.1f", fps);
    }
    
    ImGui::Separator();
    ImGui::Text("Top Functions by Time:");

    std::vector<std::pair<std::string, int64_t>> sorted_functions(function_durations.begin(), function_durations.end());
    std::ranges::sort(sorted_functions,[](const auto& a, const auto& b)
    {
        return a.second > b.second;
    });
    
    if (ImGui::BeginTable("TopFunctions", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Function");
        ImGui::TableSetupColumn("Time (ms)");
        ImGui::TableSetupColumn("% Total");
        ImGui::TableHeadersRow();
        
        int count = 0;
        for (const auto& [name, duration] : sorted_functions)
        {
            if (count++ >= 10) break;
            
            const float percentage = total_duration > 0 ? (duration / static_cast<float>(total_duration)) * 100.0f : 0.0f;
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", duration / 1000000.0);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.1f%%", percentage);
        }
        
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Timeline Overview:");
    
    if (!entries.empty())
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        const auto canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 60.0f);
        
        ImGui::InvisibleButton("mini_timeline", canvas_size);
        
        draw_list->AddRectFilled(canvas_pos,ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));
        
        int64_t time_range = max_end - min_start;
        if (time_range == 0) time_range = 1;
        
        for (const auto& entry : entries)
        {
            const float x_start = ((entry.start_ns - min_start) / static_cast<float>(time_range)) * canvas_size.x;
            const float x_end = ((entry.end_ns - min_start) / static_cast<float>(time_range)) * canvas_size.x;
            
            const float y_pos = canvas_pos.y + (entry.depth % 5) * (canvas_size.y / 5);
            const float height = canvas_size.y / 5 - 2;
            
            const ImU32 color = IM_COL32(100 + (entry.depth * 30) % 155, 150 - (entry.depth * 20) % 100, 200, 200);
            
            draw_list->AddRectFilled(ImVec2(canvas_pos.x + x_start, y_pos), ImVec2(canvas_pos.x + x_end, y_pos + height), color);
        }
        
        draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(200, 200, 200, 255));
    }
    
    ImGui::End();
}

void ProfilerUI::show_timeline_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Timeline View", &impl_->show_timeline_);
    
    ImGui::SliderFloat("Zoom", &impl_->timeline_zoom_, 0.1f, 10.0f);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }

    std::vector<core::ProfileEntry> filtered_entries;
    if (!impl_->filter_text_.empty())
    {
        for (const auto& entry : entries)
        {
            if (entry.name.find(impl_->filter_text_) != std::string::npos)
            {
                filtered_entries.push_back(entry);
            }
        }
    }
    else
    {
        filtered_entries = entries;
    }
    
    if (filtered_entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No matching entries");
        ImGui::End();
        return;
    }
    
    int64_t min_time = filtered_entries.front().start_ns;
    int64_t max_time = filtered_entries.front().end_ns;
    for (const auto& entry : filtered_entries)
    {
        min_time = std::min(min_time, entry.start_ns);
        max_time = std::max(max_time, entry.end_ns);
    }
    int64_t time_range = max_time - min_time;
    if (time_range == 0) time_range = 1;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
    
    ImGui::InvisibleButton("timeline_canvas", canvas_size);

    std::map<core::ThreadId, std::vector<size_t>> thread_entries;
    for (size_t i = 0; i < filtered_entries.size(); ++i)
    {
        thread_entries[filtered_entries[i].thread_id].push_back(i);
    }

    float y_offset = canvas_pos.y;
    
    for (const auto& [thread_id, indices] : thread_entries)
    {
        constexpr float row_height = 25.0f;
        std::ostringstream ss;
        ss << "Thread " << thread_id;
        draw_list->AddText(ImVec2(canvas_pos.x, y_offset), IM_COL32(200, 200, 200, 255), ss.str().c_str());
        y_offset += 20.0f;

        int max_depth = 0;
        for (const size_t idx : indices)
        {
            max_depth = std::max(max_depth, filtered_entries[idx].depth);
        }

        for (const size_t idx : indices)
        {
            render_timeline_entry(filtered_entries[idx], row_height, time_range, min_time, canvas_pos, canvas_size, y_offset, idx);
        }
        
        y_offset += (max_depth + 1) * row_height + 10.0f;
    }
    
    ImGui::End();
}

void ProfilerUI::show_flamegraph_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Flame Graph", &impl_->show_flamegraph_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
    
    ImGui::InvisibleButton("flamegraph_canvas", canvas_size);

    std::map<int, std::vector<size_t>> depth_map;
    int64_t total_time = 0;
    
    for (size_t i = 0; i < entries.size(); ++i)
    {
        depth_map[entries[i].depth].push_back(i);
        if (entries[i].depth == 0)
        {
            total_time += entries[i].duration_ns();
        }
    }
    
    if (total_time == 0) total_time = 1;
    
    int max_depth = depth_map.empty() ? 0 : depth_map.rbegin()->first;
    float row_height = std::min(30.0f, canvas_size.y / (max_depth + 1));

    for (int depth = max_depth; depth >= 0; --depth)
    {
        float y_pos = canvas_pos.y + canvas_size.y - (depth + 1) * row_height;
        float x_offset = 0.0f;
        
        for (size_t idx : depth_map[depth])
        {
            const auto& entry = entries[idx];
            
            float width = (entry.duration_ns() / static_cast<float>(total_time)) * canvas_size.x;
            width = std::max(width, 2.0f);
            
            render_flamegraph_node(entry, canvas_pos.x + x_offset, y_pos, width, row_height - 1.0f, idx);
            
            x_offset += width;
        }
    }
    
    ImGui::End();
}

void ProfilerUI::show_call_tree_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Call Tree", &impl_->show_call_tree_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    for (const auto& entry : entries)
    {
        if (entry.depth == 0)
        {
            render_call_tree_node(entry, 0);
        }
    }
    
    ImGui::End();
}

void ProfilerUI::show_hot_spots_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Hot Spots", &impl_->show_hot_spots_);
    
    impl_->stats_analyzer.analyze(entries);
    const auto top_functions = impl_->stats_analyzer.get_top_functions(20);
    
    if (ImGui::BeginTable("HotSpots", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable))
    {
        ImGui::TableSetupColumn("Function");
        ImGui::TableSetupColumn("Calls");
        ImGui::TableSetupColumn("Total Time (ms)");
        ImGui::TableSetupColumn("Avg Time (ms)");
        ImGui::TableSetupColumn("% Total");
        ImGui::TableHeadersRow();

        const int64_t total_time = impl_->stats_analyzer.total_profiled_time_ns();
        
        for (const auto& func : top_functions)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", func.name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", func.call_count);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", func.total_time_ns / 1000000.0);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", func.avg_time_ns / 1000000.0);
            ImGui::TableSetColumnIndex(4);
            double percentage = (total_time > 0) ? (100.0 * func.total_time_ns / total_time) : 0.0;
            ImGui::Text("%.2f%%", percentage);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void ProfilerUI::show_thread_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Thread Activity", &impl_->show_thread_view_);
    
    std::map<core::ThreadId, std::vector<core::ProfileEntry>> thread_entries;
    for (const auto& entry : entries)
    {
        thread_entries[entry.thread_id].push_back(entry);
    }
    
    for (const auto& [tid, entries] : thread_entries)
    {
        std::ostringstream oss;
        oss << "Thread " << tid;
        if (ImGui::CollapsingHeader(oss.str().c_str()))
        {
            ImGui::Text("Entries: %zu", entries.size());
        }
    }
    
    ImGui::End();
}

void ProfilerUI::show_statistics_view(const std::vector<core::ProfileEntry>& entries) const
{
    ImGui::Begin("Statistics", &impl_->show_statistics_);
    
    impl_->stats_analyzer.analyze(entries);
    
    ImGui::Text("Total Functions: %zu", impl_->stats_analyzer.total_functions());
    ImGui::Text("Total Entries: %zu", entries.size());
    ImGui::Text("Total Time: %.3f ms", impl_->stats_analyzer.total_profiled_time_ns() / 1000000.0);
    
    ImGui::Separator();
    
    auto all_stats = impl_->stats_analyzer.get_function_stats();
    
    if (ImGui::BeginTable("AllStats", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Function");
        ImGui::TableSetupColumn("Calls");
        ImGui::TableSetupColumn("Total (ms)");
        ImGui::TableSetupColumn("Avg (ms)");
        ImGui::TableSetupColumn("Min (ms)");
        ImGui::TableSetupColumn("Max (ms)");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        
        for (const auto& [name, stats] : all_stats)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", stats.call_count);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", stats.total_time_ns / 1000000.0);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", stats.avg_time_ns / 1000000.0);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", stats.min_time_ns / 1000000.0);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.3f", stats.max_time_ns / 1000000.0);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void ProfilerUI::show_function_details() const
{
    ImGui::Begin("Function Details", &impl_->show_details_panel_);
    
    if (!impl_->selected_entry_.has_value() || 
        impl_->selected_entry_.value() >= impl_->cached_entries_.size())
    {
        ImGui::Text("No entry selected");
        ImGui::End();
        return;
    }
    
    const auto& entry = impl_->cached_entries_[impl_->selected_entry_.value()];
    
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Selected Entry");
    ImGui::Separator();
    
    ImGui::Text("Function: %s", entry.name.c_str());
    ImGui::Text("Duration: %.3f ms", entry.duration_ms());
    ImGui::Text("Duration: %.0f us", entry.duration_us());
    ImGui::Text("Duration: %lld ns", static_cast<long long>(entry.duration_ns()));
    ImGui::Text("Start Time: %.3f ms", entry.start_ns / 1000000.0);
    ImGui::Text("End Time: %.3f ms", entry.end_ns / 1000000.0);
    ImGui::Text("Depth: %d", entry.depth);
    ImGui::Text("Thread ID: %zu", std::hash<core::ThreadId>{}(entry.thread_id));
    
    ImGui::Separator();
    
    // Count same function calls
    int same_function_count = 0;
    int64_t total_time = 0;
    int64_t min_time = entry.duration_ns();
    int64_t max_time = entry.duration_ns();
    
    for (const auto& e : impl_->cached_entries_)
    {
        if (e.name == entry.name)
        {
            same_function_count++;
            total_time += e.duration_ns();
            min_time = std::min(min_time, e.duration_ns());
            max_time = std::max(max_time, e.duration_ns());
        }
    }
    
    ImGui::Text("Same function called: %d times", same_function_count);
    ImGui::Text("Total time in function: %.3f ms", total_time / 1000000.0);
    if (same_function_count > 0)
    {
        ImGui::Text("Average time: %.3f ms", (total_time / same_function_count) / 1000000.0);
        ImGui::Text("Min time: %.3f ms", min_time / 1000000.0);
        ImGui::Text("Max time: %.3f ms", max_time / 1000000.0);
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Highlight All Instances"))
    {
        impl_->highlighted_function_ = entry.name;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close"))
    {
        impl_->selected_entry_ = std::nullopt;
        impl_->show_details_panel_ = false;
    }
    
    ImGui::End();
}

void ProfilerUI::show_process_selector() const
{
    ImGui::Begin("Process Selector", &impl_->show_process_selector_);
    
    if (ImGui::Button("Refresh Process List"))
    {
        impl_->process_list_ = platform::ProcessEnumerator::enumerate_processes();
    }
    
    ImGui::Separator();
    
    if (ImGui::BeginTable("ProcessList", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Path");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        
        for (const auto& proc : impl_->process_list_)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(std::to_string(proc.pid).c_str(), impl_->selected_pid_ == proc.pid, ImGuiSelectableFlags_SpanAllColumns))
            {
                impl_->selected_pid_ = proc.pid;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", proc.name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", proc.executable_path.c_str());
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void ProfilerUI::show_process_attachment() const
{
    ImGui::Begin("Attach to Process", &impl_->show_attachment_dialog_);
    
    ImGui::Text("PID: %u", impl_->selected_pid_);
    
    if (impl_->attacher->is_attached())
    {
        ImGui::Text("Status: Attached to PID %u", impl_->attacher->attached_pid());
        if (ImGui::Button("Detach"))
        {
            impl_->attacher->detach();
        }
    }
    else
    {
        if (ImGui::Button("Attach") && impl_->selected_pid_ > 0)
        {
            if (!impl_->attacher->attach(impl_->selected_pid_))
            {
                ImGui::Text("Error: %s", impl_->attacher->last_error().c_str());
            }
        }
    }
    
    ImGui::End();
}

void ProfilerUI::show_memory_profiler() const
{
    auto memory = impl_->current_session_.get_memory_usage();
    ImGui::Begin("Memory Profiler", &impl_->show_memory_profiler_);
    if (ImGui::BeginTable("MemoryUsageTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Function");
        ImGui::TableSetupColumn("Memory Used (bytes)");
        ImGui::TableHeadersRow();

        for (const auto& [func, mem] : memory)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", func.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%llu", static_cast<unsigned long long>(mem));
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void ProfilerUI::show_cpu_monitor() const
{
    auto cpu_usage = impl_->current_session_.get_cpu_usage();
    ImGui::Begin("CPU Monitor", &impl_->show_cpu_monitor_);
    if (ImGui::BeginTable("CPUUsageTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Process");
        ImGui::TableSetupColumn("CPU Usage (%)");
        ImGui::TableHeadersRow();

        for (const auto& [proc, cpu] : cpu_usage)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", proc.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", cpu);
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void ProfilerUI::render_timeline_entry(const core::ProfileEntry& entry,
                                        const float row_height,
                                        const int64_t time_range_ns,
                                        const int64_t min_time_ns,
                                        const ImVec2 canvas_pos,
                                        const ImVec2 canvas_size,
                                        const float base_y_offset, size_t entry_idx) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const float x_start = ((entry.start_ns - min_time_ns) / static_cast<float>(time_range_ns)) * canvas_size.x * impl_->timeline_zoom_;
    const float x_end = ((entry.end_ns - min_time_ns) / static_cast<float>(time_range_ns)) * canvas_size.x * impl_->timeline_zoom_;
    const float width = std::max(x_end - x_start, 2.0f);
    
    const float y_pos = base_y_offset + entry.depth * row_height;

    static constexpr ImU32 colors[] = {
        IM_COL32(100, 150, 255, 255),
        IM_COL32(255, 150, 100, 255),
        IM_COL32(150, 255, 100, 255),
        IM_COL32(255, 100, 150, 255),
        IM_COL32(100, 255, 150, 255),
        IM_COL32(150, 100, 255, 255),
    };
    
    const bool is_selected = impl_->selected_entry_.has_value() && impl_->selected_entry_.value() == entry_idx;
    const bool is_highlighted = !impl_->highlighted_function_.empty() && entry.name == impl_->highlighted_function_;
    
    ImU32 color = colors[entry.depth % 6];
    if (is_selected)
    {
        color = IM_COL32(255, 255, 0, 255);
    }
    else if (is_highlighted)
    {
        color = IM_COL32(255, 200, 0, 255);
    }

    draw_list->AddRectFilled(
        ImVec2(canvas_pos.x + x_start, y_pos),
        ImVec2(canvas_pos.x + x_start + width, y_pos + row_height - 2.0f),
        color);

    const ImU32 border_color = is_selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
    const float border_width = is_selected ? 2.0f : 1.0f;
    draw_list->AddRect(
        ImVec2(canvas_pos.x + x_start, y_pos), 
        ImVec2(canvas_pos.x + x_start + width, y_pos + row_height - 2.0f), 
        border_color, 0.0f, 0, border_width);

    if (width > 50.0f)
    {
        std::ostringstream ss;
        ss << entry.name << " (" << std::fixed << std::setprecision(2) << entry.duration_ms() << " ms)";
        
        const ImVec2 text_size = ImGui::CalcTextSize(ss.str().c_str());
        if (text_size.x < width - 4.0f)
        {
            draw_list->AddText(
                ImVec2(canvas_pos.x + x_start + 2.0f, y_pos + (row_height - text_size.y) / 2.0f), 
                IM_COL32(255, 255, 255, 255), 
                ss.str().c_str());
        }
    }

    const bool is_hovered = ImGui::IsMouseHoveringRect(
        ImVec2(canvas_pos.x + x_start, y_pos), 
        ImVec2(canvas_pos.x + x_start + width, y_pos + row_height - 2.0f));
    
    if (is_hovered)
    {
        ImGui::BeginTooltip();
        ImGui::Text("Function: %s", entry.name.c_str());
        ImGui::Text("Duration: %.3f ms (%.0f us)", entry.duration_ms(), entry.duration_us());
        ImGui::Text("Start: %.3f ms", entry.start_ns / 1000000.0);
        ImGui::Text("Depth: %d", entry.depth);
        ImGui::Text("Thread: %zu", std::hash<core::ThreadId>{}(entry.thread_id));
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Click to select");
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Double-click to highlight all");
        ImGui::EndTooltip();
        
        if (ImGui::IsMouseClicked(0))
        {
            impl_->selected_entry_ = entry_idx;
            impl_->show_details_panel_ = true;
        }
        
        if (ImGui::IsMouseDoubleClicked(0))
        {
            impl_->highlighted_function_ = entry.name;
        }
    }
}

void ProfilerUI::render_flamegraph_node(const core::ProfileEntry& entry,
                                        const float x,
                                        const float y,
                                        const float width,
                                        const float height,
                                        size_t entry_idx) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    constexpr ImU32 colors[] = {
        IM_COL32(100, 150, 255, 255),
        IM_COL32(255, 150, 100, 255),
        IM_COL32(150, 255, 100, 255),
        IM_COL32(255, 100, 150, 255),
        IM_COL32(100, 255, 150, 255),
        IM_COL32(150, 100, 255, 255),
    };
    
    const bool is_selected = impl_->selected_entry_.has_value() && impl_->selected_entry_.value() == entry_idx;
    const bool is_highlighted = !impl_->highlighted_function_.empty() && entry.name == impl_->highlighted_function_;
    
    ImU32 color = colors[entry.depth % 6];
    if (is_selected)
    {
        color = IM_COL32(255, 255, 0, 255);
    }
    else if (is_highlighted)
    {
        color = IM_COL32(255, 200, 0, 255);
    }

    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), color);

    const ImU32 border_color = is_selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
    const float border_width = is_selected ? 2.0f : 1.0f;
    draw_list->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), border_color, 0.0f, 0, border_width);

    if (width > 50.0f)
    {
        std::ostringstream ss;
        ss << entry.name << " (" << std::fixed << std::setprecision(2) << entry.duration_ms() << " ms)";
        
        const ImVec2 text_size = ImGui::CalcTextSize(ss.str().c_str());
        if (text_size.x < width - 4.0f)
        {
            draw_list->AddText(
                ImVec2(x + 2.0f, y + (height - text_size.y) / 2.0f), 
                IM_COL32(255, 255, 255, 255), 
                ss.str().c_str());
        }
    }

    const auto is_hovered = ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + width, y + height));
    
    if (is_hovered)
    {
        ImGui::BeginTooltip();
        ImGui::Text("Function: %s", entry.name.c_str());
        ImGui::Text("Duration: %.3f ms", entry.duration_ms());
        ImGui::Text("Depth: %d", entry.depth);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Click to select");
        ImGui::EndTooltip();
        
        if (ImGui::IsMouseClicked(0))
        {
            impl_->selected_entry_ = entry_idx;
            impl_->show_details_panel_ = true;
        }
        
        if (ImGui::IsMouseDoubleClicked(0))
        {
            impl_->highlighted_function_ = entry.name;
        }
    }
}

void ProfilerUI::render_call_tree_node(const core::ProfileEntry& entry, const int depth)
{
    const std::string label = entry.name + " (" + std::to_string(entry.duration_ms()) + " ms)";
    if (ImGui::TreeNode(label.c_str()))
    {
        for (const auto& child : entry.children)
        {
            render_call_tree_node(*child, depth + 1);
        }
        ImGui::TreePop();
    }
}

void ProfilerUI::set_selected_entry(size_t index) const
{
    impl_->selected_entry_ = index;
}

std::optional<size_t> ProfilerUI::selected_entry() const
{
    return impl_->selected_entry_;
}

void ProfilerUI::show_export_dialog() const
{
    ImGui::Begin("Export Profiling Data", &impl_->show_export_dialog_);

    if (ImGui::Button("Export to CSV"))
    {
        const auto export_entries = impl_->cached_entries_;
        if (impl_->exporter_.export_to_csv(export_entries, "profiler_export.csv"))
        {
            std::cout << "Exported to profiler_export.csv\n";
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Export to JSON"))
    {
        const auto export_entries = impl_->cached_entries_;
        if (impl_->exporter_.export_to_json(export_entries, "profiler_export.json"))
        {
            std::cout << "Exported to profiler_export.json\n";
        }
    }

    ImGui::End();;
}

void ProfilerUI::show_settings_dialog() const
{
    ImGui::Begin("Profiler Settings", &impl_->show_settings_dialog_);

    ImGui::Text("Sampling Interval (ms):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    float interval_ms = impl_->attacher->get_sampled_entries().empty() ? 1.0f : impl_->attacher->get_sampled_entries().front().duration_ms();
    if (ImGui::InputFloat("##sampling_interval", &interval_ms))
    {
        impl_->attacher->set_sample_rate(static_cast<int>(interval_ms));
    }

    ImGui::Separator();

    ImGui::Text("Auto Zoom:");
    ImGui::SameLine();
    ImGui::Checkbox("##auto_zoom", &impl_->auto_zoom_);

    ImGui::Separator();

    if (ImGui::Button("Close"))
    {
        impl_->show_settings_dialog_ = false;
    }

    ImGui::End();
}

runscope::platform::ProcessAttacher& ProfilerUI::get_process_attacher() const
{
    return *impl_->attacher;
}

void runscope::ui::ProfilerUI::update_statistics(const std::vector<core::ProfileEntry> &entries) const
{
    impl_->stats_analyzer.analyze(entries);
    if (impl_->show_statistics_)
    {
        show_statistics_view(entries);
    }
}
