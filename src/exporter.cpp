#include "runscope/exporter.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <functional>

using namespace runscope;

bool Exporter::export_to_json(const std::vector<ProfileEntry>& entries, const std::string_view filename)
{
    std::ofstream file(filename.data());
    if (!file.is_open())
    {
        return false;
    }

    file << "{\n";
    file << "  \"traceEvents\": [\n";

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& entry = entries[i];
        
        file << "    {\n";
        file << "      \"name\": \"" << entry.name << "\",\n";
        file << "      \"cat\": \"function\",\n";
        file << "      \"ph\": \"X\",\n";
        file << "      \"ts\": " << (entry.start_ns / 1000) << ",\n";
        file << "      \"dur\": " << (entry.duration_ns() / 1000) << ",\n";
        file << "      \"pid\": 0,\n";
        file << "      \"tid\": " << std::hash<std::thread::id>{}(entry.thread_id) << ",\n";
        file << "      \"args\": {\"depth\": " << entry.depth << "}\n";
        file << "    }";
        
        if (i < entries.size() - 1)
        {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    return true;
}

bool Exporter::export_to_csv(const std::vector<ProfileEntry>& entries, const std::string_view filename)
{
    std::ofstream file(filename.data());
    if (!file.is_open())
    {
        return false;
    }

    file << "Name,Start(ns),End(ns),Duration(ns),Duration(ms),Thread,Depth\n";

    for (const auto& entry : entries)
    {
        file << "\"" << entry.name << "\","
             << entry.start_ns << ","
             << entry.end_ns << ","
             << entry.duration_ns() << ","
             << std::fixed << std::setprecision(3) << entry.duration_ms() << ","
             << std::hash<std::thread::id>{}(entry.thread_id) << ","
             << entry.depth << "\n";
    }

    return true;
}
