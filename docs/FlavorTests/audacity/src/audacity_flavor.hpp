#pragma once

#include "linuxdesktop/settings.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace flavor_tests::audacity {

struct FlushResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
};

class FileConfig {
public:
    explicit FileConfig(std::filesystem::path local_filename);

    bool Init();
    FlushResult Flush(std::string content);
    bool dirty() const { return dirty_; }
    const std::filesystem::path& localFilename() const { return local_filename_; }

private:
    void Warn();

    std::filesystem::path local_filename_;
    bool dirty_ = true;
    int warnings_ = 0;
};

} // namespace flavor_tests::audacity
