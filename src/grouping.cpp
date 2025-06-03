#include "grouping.h"
#include <algorithm>

namespace dupesweep {

SizeGroup Grouping::groupFilesBySize(const FileList& files) {
    SizeGroup sizeGroups;

    for(const auto& [path, size]: files) {
        sizeGroups[size].push_back(path);
    }

    return sizeGroups;
}

SizeGroup Grouping:: filterPotentialDuplicates(const SizeGroup& sizeGroups) {
    SizeGroup filteredGroups;

    for(const auto& [size, paths]: sizeGroups) {
        if(paths.size() > 1) {
            filteredGroups[size] = paths;
        }
    }

    return filteredGroups;
}

}