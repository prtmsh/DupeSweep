#pragma once

#include "dupesweep/types.h"
#include <functional>
#include <mutex>
#include <future>

namespace dupesweep {
    class Hashing {
    public: 

        //calculate quick hash (first few bytes) for a file
        static std::string quickHash(const FilePath& path);

        //calculate full hash (sha-256) for a file
        static std::string fullHash(const FilePath& path);

        //group files by quick hash within a size group
        static HashGroup groupByQuickHash(const std::vector<FilePath>& files);

        //group files by full hash within a quick hash group
        static HashGroup groupByFullHash(const std::vector<FilePath>& files);

        //perform full duplicate detection and report progress
        static HashGroup findDuplicates(
            const SizeGroup& sizeGroups,
            int numThreads = 0,
            const std::function<void(int, int)>& progressCallbacl = [](int, int) {}
        );

    private:
        //process a batch of files in parallel
        static HashGroup processFilesParallel(\
            const std::vector<FilePath>& files,
            const std::function<std::string(const FilePath&)>& hashFunction,
            int numThreads,
            std::atomic<int>& processedFiles,
            const std::function<void(int, int)>& progressCallback,
            int totalFiles
        );
    };
}