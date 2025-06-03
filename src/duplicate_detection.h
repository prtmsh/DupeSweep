#pragma once

#include "dupesweep/types.h"
#include <functional>

namespace dupesweep {
    class DuplicateDetection {
    public:
        // find all duplicate files in the given directory
        static DuplicateList findDuplicates(
            const FilePath& directory,
            int numThreads = 0,
            const std::function<void(const std::string&, int, int)>& progressCallback = [](const std::string&, int, int) {}
        );

        // calculate total wasted space from duplicate files
        static FileSize calculateWastedSpace(const DuplicateList& duplicates);

        // convert HashGroup to DuplicateList
        static DuplicateList hashGroupToDuplicateList(const HashGroup& hashGroup, const SizeGroup& sizeGroups);
    };
}