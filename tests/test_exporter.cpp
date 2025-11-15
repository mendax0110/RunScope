#include <gtest/gtest.h>
#include "runscope/exporter.hpp"
#include <fstream>
#include <filesystem>

using namespace runscope;

class ExporterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = "/tmp/runscope_test";
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override
    {
        std::filesystem::remove_all(test_dir_);
    }
    
    std::string test_dir_;
};

TEST_F(ExporterTest, ExportToJSON)
{
    std::vector<ProfileEntry> entries;
    entries.push_back({"function1", 1000000, 2000000, std::this_thread::get_id(), 0});
    entries.push_back({"function2", 2000000, 3000000, std::this_thread::get_id(), 1});
    
    std::string filename = test_dir_ + "/test.json";
    EXPECT_TRUE(Exporter::export_to_json(entries, filename));
    
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    std::ifstream file(filename);
    EXPECT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("traceEvents"), std::string::npos);
    EXPECT_NE(content.find("function1"), std::string::npos);
    EXPECT_NE(content.find("function2"), std::string::npos);
}

TEST_F(ExporterTest, ExportToCSV)
{
    std::vector<ProfileEntry> entries;
    entries.push_back({"function1", 1000000, 2000000, std::this_thread::get_id(), 0});
    entries.push_back({"function2", 2000000, 3000000, std::this_thread::get_id(), 1});
    
    std::string filename = test_dir_ + "/test.csv";
    EXPECT_TRUE(Exporter::export_to_csv(entries, filename));
    
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    std::ifstream file(filename);
    EXPECT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    EXPECT_NE(content.find("Name"), std::string::npos);
    EXPECT_NE(content.find("Duration"), std::string::npos);
    EXPECT_NE(content.find("function1"), std::string::npos);
    EXPECT_NE(content.find("function2"), std::string::npos);
}

TEST_F(ExporterTest, EmptyEntries)
{
    constexpr std::vector<ProfileEntry> entries;
    
    const std::string json_file = test_dir_ + "/empty.json";
    EXPECT_TRUE(Exporter::export_to_json(entries, json_file));
    EXPECT_TRUE(std::filesystem::exists(json_file));
    
    const std::string csv_file = test_dir_ + "/empty.csv";
    EXPECT_TRUE(Exporter::export_to_csv(entries, csv_file));
    EXPECT_TRUE(std::filesystem::exists(csv_file));
}

TEST_F(ExporterTest, InvalidPath)
{
    std::vector<ProfileEntry> entries;
    entries.push_back({"function1", 1000000, 2000000, std::this_thread::get_id(), 0});
    
    EXPECT_FALSE(Exporter::export_to_json(entries, "/invalid/path/test.json"));
    EXPECT_FALSE(Exporter::export_to_csv(entries, "/invalid/path/test.csv"));
}

TEST_F(ExporterTest, LargeDataset)
{
    std::vector<ProfileEntry> entries;
    
    for (int i = 0; i < 1000; ++i)
    {
        entries.push_back({
            "function_" + std::to_string(i),
            i * 1000000,
            (i + 1) * 1000000,
            std::this_thread::get_id(),
            i % 10
        });
    }
    
    const std::string json_file = test_dir_ + "/large.json";
    EXPECT_TRUE(Exporter::export_to_json(entries, json_file));
    EXPECT_TRUE(std::filesystem::exists(json_file));
    
    const std::string csv_file = test_dir_ + "/large.csv";
    EXPECT_TRUE(Exporter::export_to_csv(entries, csv_file));
    EXPECT_TRUE(std::filesystem::exists(csv_file));
}
