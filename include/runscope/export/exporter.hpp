#pragma once

#include "runscope/core/profile_entry.hpp"
#include <string>
#include <vector>

namespace runscope::export_format
{
    class Exporter
    {
    public:
        static bool export_to_json(const std::vector<core::ProfileEntry>& entries, const std::string& filename);

        static bool export_to_csv(const std::vector<core::ProfileEntry>& entries, const std::string& filename);

        static bool export_to_chrome_trace(const std::vector<core::ProfileEntry>& entries, const std::string& filename);

        static bool import_from_json(const std::string& filename, std::vector<core::ProfileEntry>& entries);

    private:
        static std::string thread_id_to_string(const core::ThreadId& id);

        static std::string extract_string(const std::string &src, const std::string &key);

        template<typename T>
        static T extract_number(const std::string& src, const std::string& key);
    };
}

