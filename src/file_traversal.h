#pragma once

#include "dupesweep/types.h"
#include <functional>

namespace dupesweep {
    class FileTraversal {
        public:
            //recursively collect all regular files from a directory
            static FileList collectFiles(const FilePath& rootDir);

            //collect files with progress callback
            static FileList collectFiles(
                const FilePath& rootDir,
                const std::function<void(const FilePath&)>& progressCallback
            );

            //check if we should process this file
            static bool shouldProcessFile(const FilePath& path);
    };
}