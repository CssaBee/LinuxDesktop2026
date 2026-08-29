#include "audacity_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

struct test_failure : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw test_failure(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-audacity-flavor";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw test_failure("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void init_creates_missing_config_file_for_read_write_probe()
{
    const auto root = test_root();
    flavor_tests::audacity::FileConfig config(root / "Audacity" / "audacity.cfg");

    require(config.Init(), "Audacity config init should create a writable local file");
    require(std::filesystem::exists(config.localFilename()), "Audacity local config file should exist after probe");
}

void flush_replaces_config_and_removes_dirty_state()
{
    const auto root = test_root();
    const auto target = root / "Audacity" / "audacity.cfg";
    std::filesystem::create_directories(target.parent_path());
    std::ofstream(target) << "[Directories]\nTempDir=old\n";

    flavor_tests::audacity::FileConfig config(target);
    require(config.Init(), "Audacity config init should succeed");
    const auto report = config.Flush("[Directories]\nTempDir=new\n");

    require(report.ok, "Audacity config flush should succeed");
    require(report.backup_path.has_value(), "Audacity config flush should keep backup");
    require(!config.dirty(), "Audacity dirty flag should clear after successful flush");
    require(read_file(target).find("TempDir=new") != std::string::npos,
        "Audacity config should contain new content");
    require(read_file(*report.backup_path).find("TempDir=old") != std::string::npos,
        "Audacity backup should contain old config");
}

} // namespace

int main()
{
    try {
        init_creates_missing_config_file_for_read_write_probe();
        flush_replaces_config_and_removes_dirty_state();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
    return 0;
}
