#include "runscope/imgui_visualizer.hpp"
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ranges>

using namespace runscope;

void ImGuiVisualizer::render(const std::vector<ProfileEntry>& entries)
{
    render_menu_bar();
    
    auto filtered = filter_entries(entries);
    
    if (show_process_menu_)
    {
        render_process_selection_menu();
    }
    
    if (show_live_view_)
    {
        render_live_view(filtered);
    }
    
    if (show_timeline_)
    {
        render_timeline(filtered);
    }
    
    if (show_flamegraph_)
    {
        render_flamegraph(filtered);
    }
    
    if (show_statistics_)
    {
        render_statistics(filtered);
    }
    
    if (selected_entry_.has_value())
    {
        render_details_panel(entries);
    }
}

void ImGuiVisualizer::render_menu_bar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Live View", nullptr, &show_live_view_);
            ImGui::MenuItem("Timeline", nullptr, &show_timeline_);
            ImGui::MenuItem("Flame Graph", nullptr, &show_flamegraph_);
            ImGui::MenuItem("Statistics", nullptr, &show_statistics_);
            ImGui::MenuItem("Process Menu", nullptr, &show_process_menu_);
            ImGui::Separator();
            ImGui::MenuItem("Auto Zoom", nullptr, &auto_zoom_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Export"))
        {
            if (ImGui::MenuItem("Export to JSON"))
            {

            }
            if (ImGui::MenuItem("Export to CSV"))
            {

            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Clear Selection"))
            {
                selected_entry_ = std::nullopt;
                highlighted_function_ = "";
            }
            if (ImGui::MenuItem("Reset Zoom"))
            {
                zoom_level_ = 1.0f;
                pan_offset_ = 0.0f;
            }
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("##filter", filter_text_, sizeof(filter_text_));
        
        ImGui::EndMainMenuBar();
    }
}

void ImGuiVisualizer::render_process_selection_menu()
{
    ImGui::Begin("Process Selection", &show_process_menu_);
    
    ImGui::Text("Select processes to profile:");
    ImGui::Separator();
    
    auto processes = ProcessManager::getInstance().get_all_processes();
    
    if (processes.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No processes registered yet");
    }
    else
    {
        if (ImGui::BeginTable("ProcessTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Enabled");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Calls");
            ImGui::TableSetupColumn("Total (ms)");
            ImGui::TableSetupColumn("Avg (ms)");
            ImGui::TableSetupColumn("Min/Max (ms)");
            ImGui::TableHeadersRow();
            
            for (auto& [name, info] : processes)
            {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                bool enabled = info.enabled;
                if (ImGui::Checkbox(("##" + name).c_str(), &enabled))
                {
                    ProcessManager::getInstance().set_process_enabled(name, enabled);
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", name.c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", info.call_count);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.3f", info.total_time_ms);
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.3f", info.avg_time_ms);
                
                ImGui::TableSetColumnIndex(5);
                if (info.call_count > 0)
                {
                    ImGui::Text("%.3f / %.3f", info.min_time_ms, info.max_time_ms);
                }
                else
                {
                    ImGui::Text("-");
                }
            }
            
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Clear Statistics"))
        {
            ProcessManager::getInstance().clear_statistics();
        }
        ImGui::SameLine();
        if (ImGui::Button("Enable All"))
        {
            for (const auto &name: processes | std::views::keys)
            {
                ProcessManager::getInstance().set_process_enabled(name, true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Disable All"))
        {
            for (const auto &name: processes | std::views::keys)
            {
                ProcessManager::getInstance().set_process_enabled(name, false);
            }
        }
    }
    
    ImGui::End();
}

void ImGuiVisualizer::render_timeline(const std::vector<ProfileEntry>& entries)
{
    ImGui::Begin("Timeline View", &show_timeline_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    ImGui::SliderFloat("Zoom", &zoom_level_, 0.1f, 10.0f);
    
    auto thread_groups = group_by_thread(entries);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
    
    ImGui::InvisibleButton("canvas", canvas_size);
    
    int64_t min_time = entries.front().start_ns;
    int64_t max_time = entries.front().end_ns;
    
    for (const auto& entry : entries)
    {
        min_time = std::min(min_time, entry.start_ns);
        max_time = std::max(max_time, entry.end_ns);
    }
    
    int64_t time_range = max_time - min_time;
    if (time_range == 0) time_range = 1;

    constexpr float row_height = 25.0f;
    float y_offset = canvas_pos.y;
    
    for (size_t i = 0; i < thread_groups.size(); ++i)
    {
        const auto& thread_group = thread_groups[i];
        
        std::stringstream ss;
        ss << "Thread " << std::hash<std::thread::id>{}(thread_group.thread_id);
        draw_list->AddText(ImVec2(canvas_pos.x, y_offset), 
                          IM_COL32(200, 200, 200, 255), ss.str().c_str());
        y_offset += 20.0f;
        
        int max_depth = 0;
        for (const auto* entry : thread_group.entries)
        {
            max_depth = std::max(max_depth, entry->depth);
        }
        
        for (size_t j = 0; j < thread_group.entries.size(); ++j)
        {
            const auto* entry = thread_group.entries[j];
            
            size_t global_idx = 0;
            for (size_t k = 0; k < entries.size(); ++k)
            {
                if (&entries[k] == entry)
                {
                    global_idx = k;
                    break;
                }
            }
            
            const float x_start = ((entry->start_ns - min_time) / static_cast<float>(time_range)) * canvas_size.x * zoom_level_;
            const float x_end = ((entry->end_ns - min_time) / static_cast<float>(time_range)) * canvas_size.x * zoom_level_;
            const float width = std::max(x_end - x_start, 2.0f);
            
            const float y_pos = y_offset + entry->depth * row_height;
            
            const bool is_selected = selected_entry_.has_value() && selected_entry_.value() == global_idx;
            const bool is_highlighted = !highlighted_function_.empty() && entry->name == highlighted_function_;
            
            draw_entry_rect(*entry, global_idx, canvas_pos.x + x_start, y_pos, width, row_height - 2.0f, entry->depth, is_selected, is_highlighted);
        }
        
        y_offset += (max_depth + 1) * row_height + 10.0f;
    }
    
    ImGui::End();
}

void ImGuiVisualizer::render_flamegraph(const std::vector<ProfileEntry>& entries)
{
    ImGui::Begin("Flame Graph", &show_flamegraph_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    
    if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
    
    ImGui::InvisibleButton("flamegraph_canvas", canvas_size);
    
    std::map<int, std::vector<const ProfileEntry*>> depth_map;
    int64_t total_time = 0;
    
    for (const auto& entry : entries)
    {
        depth_map[entry.depth].push_back(&entry);
        if (entry.depth == 0)
        {
            total_time += entry.duration_ns();
        }
    }
    
    const int max_depth = depth_map.empty() ? 0 : depth_map.rbegin()->first;
    const float row_height = std::min(30.0f, canvas_size.y / (max_depth + 1));
    
    for (int depth = max_depth; depth >= 0; --depth)
    {
        const float y_pos = canvas_pos.y + canvas_size.y - (depth + 1) * row_height;
        float x_offset = 0.0f;
        
        for (const auto* entry : depth_map[depth])
        {
            size_t global_idx = 0;
            for (size_t k = 0; k < entries.size(); ++k)
            {
                if (&entries[k] == entry)
                {
                    global_idx = k;
                    break;
                }
            }
            
            float width = (entry->duration_ns() / static_cast<float>(total_time)) * canvas_size.x;
            width = std::max(width, 2.0f);
            
            const bool is_selected = selected_entry_.has_value() && selected_entry_.value() == global_idx;
            const bool is_highlighted = !highlighted_function_.empty() && entry->name == highlighted_function_;
            
            draw_entry_rect(*entry, global_idx, canvas_pos.x + x_offset, y_pos, width, row_height - 1.0f, depth, is_selected, is_highlighted);
            x_offset += width;
        }
    }
    
    ImGui::End();
}

void ImGuiVisualizer::render_statistics(const std::vector<ProfileEntry>& entries)
{
    ImGui::Begin("Statistics", &show_statistics_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    std::map<std::string, std::vector<double>> function_times;
    
    for (const auto& entry : entries)
    {
        function_times[entry.name].push_back(entry.duration_ms());
    }
    
    if (ImGui::BeginTable("StatsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable))
    {
        ImGui::TableSetupColumn("Function");
        ImGui::TableSetupColumn("Calls");
        ImGui::TableSetupColumn("Total (ms)");
        ImGui::TableSetupColumn("Avg (ms)");
        ImGui::TableSetupColumn("Min (ms)");
        ImGui::TableSetupColumn("Max (ms)");
        ImGui::TableHeadersRow();
        
        for (const auto& [name, times] : function_times)
        {
            double total = 0.0;
            double min_time = times[0];
            double max_time = times[0];
            
            for (double time : times)
            {
                total += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            double avg = total / times.size();
            
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", times.size());
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", total);
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.3f", avg);
            
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", min_time);
            
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.3f", max_time);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

std::vector<ImGuiVisualizer::ThreadEntries> ImGuiVisualizer::group_by_thread(const std::vector<ProfileEntry>& entries)
{
    std::map<std::thread::id, std::vector<const ProfileEntry*>> thread_map;
    
    for (const auto& entry : entries)
    {
        thread_map[entry.thread_id].push_back(&entry);
    }
    
    std::vector<ThreadEntries> result;
    for (auto& [thread_id, thread_entries] : thread_map)
    {
        result.push_back({thread_id, std::move(thread_entries)});
    }
    
    return result;
}

bool ImGuiVisualizer::draw_entry_rect(const ProfileEntry& entry, size_t entry_idx,
                                        const float x, const float y,
                                        const float width, const float height,
                                        const int depth, const bool is_selected, const bool is_highlighted)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    constexpr  ImU32 colors[] = {
        IM_COL32(100, 150, 255, 255),
        IM_COL32(255, 150, 100, 255),
        IM_COL32(150, 255, 100, 255),
        IM_COL32(255, 100, 150, 255),
        IM_COL32(100, 255, 150, 255),
        IM_COL32(150, 100, 255, 255),
    };
    
    ImU32 color = colors[depth % 6];
    
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
        std::stringstream ss;
        ss << entry.name << " (" << std::fixed << std::setprecision(2) << entry.duration_ms() << " ms)";

        const ImVec2 text_size = ImGui::CalcTextSize(ss.str().c_str());
        if (text_size.x < width - 4.0f)
        {
            draw_list->AddText(ImVec2(x + 2.0f, y + (height - text_size.y) / 2.0f), IM_COL32(255, 255, 255, 255), ss.str().c_str());
        }
    }
    
    bool clicked = false;
    const bool is_hovered = ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + width, y + height));
    
    if (is_hovered)
    {
        ImGui::BeginTooltip();
        ImGui::Text("Function: %s", entry.name.c_str());
        ImGui::Text("Duration: %.3f ms (%.0f us)", entry.duration_ms(), entry.duration_us());
        ImGui::Text("Start: %.3f ms", entry.start_ns / 1'000'000.0);
        ImGui::Text("Depth: %d", entry.depth);
        ImGui::Text("Thread: %zu", std::hash<std::thread::id>{}(entry.thread_id));
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Click to select");
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Double-click to highlight all");
        ImGui::EndTooltip();
        
        if (ImGui::IsMouseClicked(0))
        {
            clicked = true;
            selected_entry_ = entry_idx;
        }
        
        if (ImGui::IsMouseDoubleClicked(0))
        {
            highlighted_function_ = entry.name;
        }
    }
    
    return clicked;
}

void ImGuiVisualizer::render_live_view(const std::vector<ProfileEntry>& entries)
{
    ImGui::Begin("Live Performance View", &show_live_view_);
    
    if (entries.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No profiling data available");
        ImGui::End();
        return;
    }
    
    int64_t total_duration = 0;
    int64_t min_start = entries.empty() ? 0 : entries.front().start_ns;
    int64_t max_end = entries.empty() ? 0 : entries.front().end_ns;
    
    std::map<std::string, int64_t> function_durations;
    
    for (const auto& entry : entries)
    {
        total_duration += entry.duration_ns();
        min_start = std::min(min_start, entry.start_ns);
        max_end = std::max(max_end, entry.end_ns);
        function_durations[entry.name] += entry.duration_ns();
    }
    
    int64_t session_duration = max_end - min_start;
    
    ImGui::Text("Session Duration: %.2f ms", session_duration / 1'000'000.0);
    ImGui::Text("Total Entries: %zu", entries.size());
    ImGui::Text("Cumulative Time: %.2f ms", total_duration / 1'000'000.0);
    
    ImGui::Separator();
    ImGui::Text("Top Functions by Time:");
    
    std::vector<std::pair<std::string, int64_t>> sorted_functions(
        function_durations.begin(), function_durations.end());
    std::ranges::sort(sorted_functions, [](const auto& a, const auto& b)
    {
        return a.second > b.second;
    });
    
    render_mini_timeline(entries);
    
    ImGui::Separator();
    
    int count = 0;
    for (const auto& [name, duration] : sorted_functions)
    {
        if (count++ >= 10) break;
        
        const float percentage = (duration / static_cast<float>(total_duration)) * 100.0f;
        
        ImGui::Text("%s", name.c_str());
        ImGui::SameLine(300);
        ImGui::Text("%.2f ms (%.1f%%)", duration / 1'000'000.0, percentage);
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
        ImGui::ProgressBar(percentage / 100.0f, ImVec2(-1, 0));
        ImGui::PopStyleColor();
    }
    
    ImGui::End();
}

void ImGuiVisualizer::render_details_panel(const std::vector<ProfileEntry>& entries)
{
    if (!selected_entry_.has_value() || selected_entry_.value() >= entries.size())
    {
        return;
    }
    
    const auto& entry = entries[selected_entry_.value()];
    
    ImGui::Begin("Details Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Selected Entry");
    ImGui::Separator();
    
    ImGui::Text("Function: %s", entry.name.c_str());
    ImGui::Text("Duration: %.3f ms", entry.duration_ms());
    ImGui::Text("Duration: %.0f us", entry.duration_us());
    ImGui::Text("Duration: %lld ns", static_cast<long long>(entry.duration_ns()));
    ImGui::Text("Start Time: %.3f ms", entry.start_ns / 1'000'000.0);
    ImGui::Text("End Time: %.3f ms", entry.end_ns / 1'000'000.0);
    ImGui::Text("Depth: %d", entry.depth);
    ImGui::Text("Thread ID: %zu", std::hash<std::thread::id>{}(entry.thread_id));
    
    ImGui::Separator();
    
    int same_function_count = 0;
    int64_t total_time = 0;
    
    for (const auto& e : entries)
    {
        if (e.name == entry.name)
        {
            same_function_count++;
            total_time += e.duration_ns();
        }
    }
    
    ImGui::Text("Same function called: %d times", same_function_count);
    ImGui::Text("Total time in function: %.3f ms", total_time / 1'000'000.0);
    
    ImGui::Separator();
    
    if (ImGui::Button("Highlight All Instances"))
    {
        highlighted_function_ = entry.name;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close"))
    {
        selected_entry_ = std::nullopt;
    }
    
    ImGui::End();
}

void ImGuiVisualizer::render_mini_timeline(const std::vector<ProfileEntry>& entries)
{
    if (entries.empty()) return;
    
    ImGui::Separator();
    ImGui::Text("Timeline Overview:");
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    const auto canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 60.0f);
    
    ImGui::InvisibleButton("mini_timeline", canvas_size);
    
    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));
    
    int64_t min_time = entries.front().start_ns;
    int64_t max_time = entries.front().end_ns;
    
    for (const auto& entry : entries)
    {
        min_time = std::min(min_time, entry.start_ns);
        max_time = std::max(max_time, entry.end_ns);
    }
    
    int64_t time_range = max_time - min_time;
    if (time_range == 0) time_range = 1;
    
    for (const auto& entry : entries)
    {
        const float x_start = ((entry.start_ns - min_time) / static_cast<float>(time_range)) * canvas_size.x;
        const float x_end = ((entry.end_ns - min_time) / static_cast<float>(time_range)) * canvas_size.x;
        
        const float y_pos = canvas_pos.y + (entry.depth % 5) * (canvas_size.y / 5);
        const float height = canvas_size.y / 5 - 2;
        
        const ImU32 color = IM_COL32(100 + (entry.depth * 30) % 155, 150 - (entry.depth * 20) % 100, 200, 200);
        
        draw_list->AddRectFilled(ImVec2(canvas_pos.x + x_start, y_pos), ImVec2(canvas_pos.x + x_end, y_pos + height), color);
    }
    
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(200, 200, 200, 255));
}

std::vector<ProfileEntry> ImGuiVisualizer::filter_entries(const std::vector<ProfileEntry>& entries) const
{
    if (filter_text_[0] == '\0')
    {
        return entries;
    }
    
    std::vector<ProfileEntry> filtered;
    std::string filter_str(filter_text_);
    
    for (const auto& entry : entries)
    {
        if (entry.name.find(filter_str) != std::string::npos)
        {
            filtered.push_back(entry);
        }
    }
    
    return filtered;
}
