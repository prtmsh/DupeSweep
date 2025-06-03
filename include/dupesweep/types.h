#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace dupesweep {
    using FileSize = uintmax_t;
    using FilePath = fs::path;
    using FileInfo = std::pair<FilePath, FileSize>;
    using FileList = std::vector<FileInfo>;
    using SizeGroup = std::unordered_map<FileSize, std::vector<FilePath>>;
    using HashGroup = std::unordered_map<std::string, std::vector<FilePath>>;

    struct DuplicateGroup{
        std::string hash;
        std::vector<FilePath> files;
        FileSize fileSize;

        FileSize wastedSpace() const{
            return fileSize * (files.size() - 1);
        }
    };

    using DuplicateList = std::vector<DuplicateGroup>;
}