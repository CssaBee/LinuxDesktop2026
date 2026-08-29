#include "audacity_flavor.hpp"

#include <fstream>
#include <utility>

namespace flavor_tests::audacity {

namespace ld = linuxdesktop::settings;

namespace {

bool config_stream_is_readable(const std::filesystem::path& path, std::string& message)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        message = "Audacity config stream could not be reopened after save";
        return false;
    }
    return true;
}

} // namespace

FileConfig::FileConfig(std::filesystem::path local_filename)
    : local_filename_(std::move(local_filename))
{
}

bool FileConfig::Init()
{
    for (int attempt = 0; attempt != 2; ++attempt) {
        std::ifstream readable(local_filename_, std::ios::binary);
        const bool can_read = static_cast<bool>(readable) || !std::filesystem::exists(local_filename_);
        readable.close();

        std::error_code ec;
        std::filesystem::create_directories(local_filename_.parent_path(), ec);
        std::ofstream writable(local_filename_, std::ios::binary | std::ios::app);
        const bool can_write = static_cast<bool>(writable);
        writable.close();

        if (can_read && can_write) {
            return true;
        }
        Warn();
    }
    return false;
}

FlushResult FileConfig::Flush(std::string content)
{
    auto report = ld::write_common_config({local_filename_, std::move(content), true}, config_stream_is_readable);
    if (report.ok) {
        dirty_ = false;
    }
    return {report.ok, report.backup_path};
}

void FileConfig::Warn()
{
    ++warnings_;
}

} // namespace flavor_tests::audacity
