#pragma once

#include "runscope/core/profile_entry.hpp"
#include <string>
#include <map>
#include <vector>


namespace runscope::analysis
{
    struct FunctionStats
    {
        std::string name;
        size_t call_count;
        int64_t total_time_ns;
        int64_t min_time_ns;
        int64_t max_time_ns;
        double avg_time_ns;
        double self_time_ns;
        double inclusive_time_ns;

        FunctionStats()
            : call_count(0), total_time_ns(0)
            , min_time_ns(std::numeric_limits<int64_t>::max())
            , max_time_ns(0), avg_time_ns(0.0)
            , self_time_ns(0.0), inclusive_time_ns(0.0)
        {

        }
    };

    class StatisticsAnalyzer
    {
    public:
        StatisticsAnalyzer() = default;

        void analyze(const std::vector<core::ProfileEntry>& entries);

        [[nodiscard]] std::map<std::string, FunctionStats> get_function_stats() const;
        [[nodiscard]] std::vector<FunctionStats> get_top_functions(size_t count) const;
        [[nodiscard]] std::vector<FunctionStats> get_hotspots(size_t count) const;

        [[nodiscard]] FunctionStats get_stats_for_function(const std::string& name) const;

        [[nodiscard]] size_t total_functions() const;
        [[nodiscard]] int64_t total_profiled_time_ns() const;

        void clear();

    private:
        std::map<std::string, FunctionStats> function_stats_;
        int64_t total_time_ns_{0};
    };
}

