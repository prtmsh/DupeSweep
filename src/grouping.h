#pragma once

#include "dupesweep/types.h"

namespace dupesweep {
    class Grouping {
    public: 
        
        //group files by size
        //each group has files of the same size
        static SizeGroup groupFilesBySize(const FileList& files);

        //filter out size groups with only one file
        //(no duplicates are possible)
        static SizeGroup filterPotentialDuplicates(const SizeGroup& sizeGroups);
    };
}