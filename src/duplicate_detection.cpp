#include "duplicate_detection.h"
#include "file_traversal.h"
#include "grouping.h"
#include "hashing.h"
#include <iostream>

namespace dupesweep {

DuplicateList DuplicateDetection::findDuplicates(
    const FilePath& directory,
    int numThreads,
    const std::function<void(const std::string&, int, int)>& progressCallback
) {
    // step 1: collect all files
    progressCallback("scanning directory... ", 0, 0);
    FileList files = FileTraversal::collectFiles(
        directory,
        [&progressCallback](const FilePath& path) {
            progressCallback("scanning: " + path.filename().string(), 0, 0);
        }
    );

    progressCallback("found " + std::to_string(files.size()) + " files", 0, 0);
    if(files.empty()) {
        return {};
    }

    // step 2: groups files by size
    progressCallback("grouping files by size...", 0 , 0);
    SizeGroup sizeGroups = Grouping::groupFilesBySize(files);
    SizeGroup potentialDuplicates = Grouping::filterPotentialDuplicates(sizeGroups);

    // count files with potential duplicates
    int potentialDuplicatesCount = 0;
    for(const auto& [size, paths]: potentialDuplicates) {
        potentialDuplicatesCount += paths.size();
    }

    progressCallback("found " + std::to_string(potentialDuplicatesCount) + " potential duplicates", 0, 0);

    if(potentialDuplicatesCount == 0){
        return {};
    }

    // step 3: find duplicates using quickHash + fullHash
    progressCallback("calculating file hashes...", 0, potentialDuplicatesCount);
    HashGroup duplicateHashGroups = Hashing::findDuplicates(
        potentialDuplicates,
        numThreads,
        [&progressCallback](int processed, int total) {
            progressCallback("hashing files... ", processed, total);
        }
    );

    // step 4: convert to duplicate list
    DuplicateList duplicates = hashGroupToDuplicateList(duplicateHashGroups, sizeGroups);
    progressCallback("found " + std::to_string(duplicates.size()) + " duplicate groups", 0, 0);

    return duplicates;
}

FileSize DuplicateDetection::calculateWastedSpace(const DuplicateList& duplicates) {
    FileSize totalWasted = 0;

    for(const auto& group: duplicates) {
        totalWasted += group.wastedSpace();
    }

    return totalWasted;
}

DuplicateList DuplicateDetection::hashGroupToDuplicateList(
    const HashGroup& hashGroup,
    const SizeGroup& sizeGroups
) {
    DuplicateList duplicates;

    for(const auto& [hash, paths]: hashGroup) {
        if(paths.size() > 1) {
            DuplicateGroup group;
            group.hash = hash;
            group.files = paths;

            // find the file size
            // get from the first file
            if(!paths.empty()) {
                try {
                    group.fileSize = fs::file_size(paths[0]);
                } catch(const fs::filesystem_error& e) {
                    std::cerr << "error getting file size: " << e.what() << "\n";
                    group.fileSize = 0;
                }
            }

            duplicates.push_back(group);
        }
    }

    return duplicates;
}

}