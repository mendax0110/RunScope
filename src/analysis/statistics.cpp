#include "runscope/analysis/statistics.hpp"
#include <algorithm>
#include <ranges>

using namespace runscope::analysis;

void StatisticsAnalyzer::analyze(const std::vector<core::ProfileEntry>& entries)
{
    function_stats_.clear();
    total_time_ns_ = 0;
    
    for (const auto& entry : entries)
    {
        auto& stats = function_stats_[entry.name];
        stats.name = entry.name;
        stats.call_count++;
        
        int64_t duration = entry.duration_ns();
        stats.total_time_ns += duration;
        stats.min_time_ns = std::min(stats.min_time_ns, duration);
        stats.max_time_ns = std::max(stats.max_time_ns, duration);
        stats.inclusive_time_ns += duration;
        
        total_time_ns_ += duration;
    }
    
    for (auto &stats: function_stats_ | std::views::values)
    {
        if (stats.call_count > 0)
        {
            stats.avg_time_ns = static_cast<double>(stats.total_time_ns) / stats.call_count;
        }
        stats.self_time_ns = stats.total_time_ns;
    }
}

std::map<std::string, FunctionStats> StatisticsAnalyzer::get_function_stats() const
{
    return function_stats_;
}

std::vector<FunctionStats> StatisticsAnalyzer::get_top_functions(size_t count) const
{
    std::vector<FunctionStats> top;
    top.reserve(function_stats_.size());
    
    for (const auto &stats: function_stats_ | std::views::values)
    {
        top.push_back(stats);
    }
    
    std::ranges::sort(top, [](const FunctionStats& a, const FunctionStats& b)
    {
        return a.total_time_ns > b.total_time_ns;
    });
    
    if (top.size() > count)
    {
        top.resize(count);
    }
    
    return top;
}

std::vector<FunctionStats> StatisticsAnalyzer::get_hotspots(size_t count) const
{
    return get_top_functions(count);
}

FunctionStats StatisticsAnalyzer::get_stats_for_function(const std::string& name) const
{
    auto it = function_stats_.find(name);
    if (it != function_stats_.end())
    {
        return it->second;
    }
    return FunctionStats();
}

size_t StatisticsAnalyzer::total_functions() const
{
    return function_stats_.size();
}

int64_t StatisticsAnalyzer::total_profiled_time_ns() const
{
    return total_time_ns_;
}

void StatisticsAnalyzer::clear()
{
    function_stats_.clear();
    total_time_ns_ = 0;
}

