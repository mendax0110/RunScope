#include "runscope/export/exporter.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace runscope::export_format;

std::string Exporter::thread_id_to_string(const core::ThreadId& id)
{
    std::ostringstream oss;
    oss << id;
    return oss.str();
}

bool Exporter::export_to_json(const std::vector<core::ProfileEntry>& entries, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }
    
    file << "{\n";
    file << "  \"entries\": [\n";
    
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& entry = entries[i];
        file << "    {\n";
        file << "      \"name\": \"" << entry.name << "\",\n";
        file << "      \"file\": \"" << entry.file << "\",\n";
        file << "      \"line\": " << entry.line << ",\n";
        file << "      \"start_ns\": " << entry.start_ns << ",\n";
        file << "      \"end_ns\": " << entry.end_ns << ",\n";
        file << "      \"duration_ns\": " << entry.duration_ns() << ",\n";
        file << "      \"duration_ms\": " << entry.duration_ms() << ",\n";
        file << "      \"thread_id\": \"" << thread_id_to_string(entry.thread_id) << "\",\n";
        file << "      \"depth\": " << entry.depth << ",\n";
        file << "      \"memory_used\": " << entry.memory_used << ",\n";
        file << "      \"cpu_usage\": " << entry.cpu_usage << "\n";
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

bool Exporter::export_to_csv(const std::vector<core::ProfileEntry>& entries, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }
    
    file << "Name,File,Line,Start(ns),End(ns),Duration(ns),Duration(ms),ThreadID,Depth,Memory,CPU\n";
    
    for (const auto& entry : entries)
    {
        file << entry.name << ","
             << entry.file << ","
             << entry.line << ","
             << entry.start_ns << ","
             << entry.end_ns << ","
             << entry.duration_ns() << ","
             << std::fixed << std::setprecision(6) << entry.duration_ms() << ","
             << thread_id_to_string(entry.thread_id) << ","
             << entry.depth << ","
             << entry.memory_used << ","
             << entry.cpu_usage << "\n";
    }
    
    return true;
}

bool Exporter::export_to_chrome_trace(const std::vector<core::ProfileEntry>& entries, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }
    
    file << "[\n";
    
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& entry = entries[i];
        
        file << "  {\n";
        file << "    \"name\": \"" << entry.name << "\",\n";
        file << "    \"cat\": \"function\",\n";
        file << "    \"ph\": \"X\",\n";
        file << "    \"ts\": " << (entry.start_ns / 1000) << ",\n";
        file << "    \"dur\": " << (entry.duration_ns() / 1000) << ",\n";
        file << "    \"pid\": 1,\n";
        file << "    \"tid\": \"" << thread_id_to_string(entry.thread_id) << "\",\n";
        file << "    \"args\": {\n";
        file << "      \"file\": \"" << entry.file << "\",\n";
        file << "      \"line\": " << entry.line << "\n";
        file << "    }\n";
        file << "  }";
        if (i < entries.size() - 1)
        {
            file << ",";
        }
        file << "\n";
    }
    
    file << "]\n";
    
    return true;
}

bool Exporter::import_from_json(const std::string& filename, std::vector<core::ProfileEntry>& entries)
{
    entries.clear();
    return false;
}