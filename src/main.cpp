#include "cli.h"
#include "duplicate_detection.h"
#include <iostream>
#include <chrono>
#include <ctime>

using namespace dupesweep;

int main(int argc, char* argv[]) {
    // parse command line arguments
    CLI::Options options = CLI::parseArgs(argc, argv);

    std::cout << "DupeSweep - Duplicate File Finder" << std::endl;
    std::cout << "Scanning directory: " << options.rootDir.string() << std::endl;
    std::cout << "Using " << options.numThreads << " threads" << std::endl;

    // record start time
    auto startTime = std::chrono::steady_clock::now();

    // find duplicates and report progress
    DuplicateList duplicates = DuplicateDetection::findDuplicates(
        options.rootDir,
        options.numThreads,
        [&options](const std::string& message, int current, int total) {
            // only show detailed progress in verbose mode
            if (options.verbose || current == 0) {
                std::cout << message;
                if (total > 0) {
                    std::cout << " (" << current << "/" << total << ", "
                              << (current * 100 / total) << "%)";
                }
                std::cout << std::endl;
            } 
            else if (total > 0 && current % (total / 100 + 1) == 0) {
                // show progress bar
                std::cout << "\\rProgress: " << (current * 100 / total) << "% completed..." << std::flush;
            }
        }
    );

    // record end time and calculate duration
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    std::cout << std::endl;
    std::cout << "Scan completed in " << duration << " seconds." << std::endl;
    std::cout << std::endl;

    // display results
    CLI::displayDuplicates(duplicates);
    CLI::displaySummary(duplicates);

    // handle duplicate deletion if needed
    if (!options.dryRun || options.interactive) {
        CLI::handleDuplicateDeletion(duplicates, options.dryRun, options.interactive);
    }

    return 0;
}
