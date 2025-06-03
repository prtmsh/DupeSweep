#include "cli.h"
#include "duplicate_detection.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>

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
                    std::cout << " (" << current << "/" << total;
                    if (current > 0) {
                         std::cout << ", " << (current * 100 / total) << "%";
                    }
                    std::cout << ")";
                }
                std::cout << std::endl;
            }
            else if (!options.verbose && total > 0 && current > 0 ) {
                static int last_percentage = -1;
                int current_percentage = (current * 100 / total);
                if (current_percentage > last_percentage || current == total) {
                    std::cout << "\rProgress: " << current_percentage << "% completed (" << current << "/" << total << ")..." << std::flush;
                    last_percentage = current_percentage;
                }
                if (current == total) {
                    std::cout << std::endl;
                }
            }
            else if (!options.verbose && total == 0) {
                 std::cout << message << std::endl;
            }
        }
    );

    auto endTime = std::chrono::steady_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double seconds_taken_val = duration_ms.count() / 1000.0;

    std::ostringstream time_message_stream;
    time_message_stream << "Time taken: " << std::fixed << std::setprecision(2) << seconds_taken_val << " seconds.";
    std::string time_taken_message = time_message_stream.str();


    if (!options.verbose && !duplicates.empty()) { 
        std::cout << "\r" << std::string(100, ' ') << "\r" << std::flush;
    }

    std::cout << std::endl;
    std::cout << "Scan completed." << std::endl;
    std::cout << std::endl;

    // display results
    CLI::displayDuplicates(duplicates);
    CLI::displaySummary(duplicates);
    
    if (!duplicates.empty()) {
        std::cout << std::endl;
    }

    if (!options.dryRun) {
        CLI::handleDuplicateDeletion(duplicates, options.dryRun /*which is false here*/, options.interactive);
    } else {
        if (!duplicates.empty()) {
             std::cout << "Dry run mode - no files were deleted." << std::endl;
        }
    }
    
    std::cout << std::endl;


    std::cout << time_taken_message << std::endl;

    return 0;
}