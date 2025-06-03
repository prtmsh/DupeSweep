#pragma once

#include "dupesweep/types.h"
#include <string>
#include <vector>

namespace dupesweep {
    class CLI {
    public:
        struct Options {
            FilePath rootDir;
            bool dryRun = true;
            int numThreads = 0;
            bool verbose = false;
            bool interactive = true;
            bool includeHidden = false;
            std::string outputFormat = "text";
        };

        // parse cli arguments
        static Options parseArgs(int argc, char* argv[]);

        // display usage info
        static void showUsage(const char* programName);

        // displau duplicate files
        static void displayDuplicates(const DuplicateList& duplicates);

        // display summary info
        static void displaySummary(const DuplicateList& duplicates);

        // delete duplicate files interactively
        static void handleDuplicateDeletion(DuplicateList& duplicates, bool dryRun, bool interactive);
        
        // format file size in human-readable format
        static std::string formatSize(FileSize size);
    };
}