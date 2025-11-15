#pragma once

#include "profile_data.hpp"
#include <vector>
#include <string>
#include <string_view>

namespace runscope
{
    class Exporter
    {
    public:
        static bool export_to_json(const std::vector<ProfileEntry>& entries, std::string_view filename);

        static bool export_to_csv(const std::vector<ProfileEntry>& entries, std::string_view filename);
    };
}
