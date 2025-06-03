#include "cli.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <thread>
#include <sstream>

namespace dupesweep {

CLI::Options CLI::parseArgs(int argc, char* argv[]) {
    Options options;

    // set default root directory to current directory
    options.rootDir = fs::current_path();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            showUsage(argv[0]);
            exit(0);
        } 
        else if (arg == "--delete") {
            options.dryRun = false;
        } 
        else if (arg == "--threads" || arg == "-t") {
            if (i + 1 < argc) {
                try {
                    options.numThreads = std::stoi(argv[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "invalid thread count: " << argv[i] << std::endl;
                    exit(1);
                }
            }
        } 
        else if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        } 
        else if (arg == "--non-interactive") {
            options.interactive = false;
        } 
        else if (arg == "--include-hidden") {
            options.includeHidden = true;
        } 
        else if (arg == "--format") {
            if (i + 1 < argc) {
                options.outputFormat = argv[++i];
                std::transform(options.outputFormat.begin(), options.outputFormat.end(),
                              options.outputFormat.begin(),
                              [](unsigned char c){ return std::tolower(c); });

                if (options.outputFormat != "text" &&
                    options.outputFormat != "json" &&
                    options.outputFormat != "csv") {
                    std::cerr << "invalid output format: " << options.outputFormat << std::endl;
                    exit(1);
                }
            }
        } else if (arg[0] != '-') {
            // assume it's the directory path
            options.rootDir = arg;
        } else {
            std::cerr << "unknown option: " << arg << std::endl;
            showUsage(argv[0]);
            exit(1);
        }
    }

    // validate the root directory
    if (!fs::exists(options.rootDir)) {
        std::cerr << "error: Directory does not exist: " << options.rootDir << std::endl;
        exit(1);
    }

    if (!fs::is_directory(options.rootDir)) {
        std::cerr << "error: Not a directory: " << options.rootDir << std::endl;
        exit(1);
    }

    // set default thread count if not specified
    if (options.numThreads <= 0) {
        options.numThreads = std::thread::hardware_concurrency();
        if (options.numThreads == 0) {
            // default to 4 threads if detection fails
            options.numThreads = 4;
        }
    }

    return options;
}

void CLI::showUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [directory]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                Show this help message" << std::endl;
    std::cout << "  --delete                  Delete duplicate files (default is dry-run)" << std::endl;
    std::cout << "  -t, --threads <num>       Number of threads to use" << std::endl;
    std::cout << "  -v, --verbose             Enable verbose output" << std::endl;
    std::cout << "  --non-interactive         Disable interactive mode" << std::endl;
    std::cout << "  --include-hidden          Include hidden files in scan" << std::endl;
    std::cout << "  --format <format>         Output format: text, json, csv" << std::endl;
    std::cout << std::endl;
    std::cout << "If directory is not specified, the current directory is used." << std::endl;
}

void CLI::displayDuplicates(const DuplicateList& duplicates) {
    if (duplicates.empty()) {
        std::cout << "no duplicate files found." << std::endl;
        return;
    }

    int groupCount = 0;
    for (const auto& group : duplicates) {
        groupCount++;
        std::cout << "Duplicate group #" << groupCount
                  << " (Size: " << formatSize(group.fileSize)
                  << ", Hash: " << group.hash << ")" << std::endl;

        int fileCount = 0;
        for (const auto& file : group.files) {
            fileCount++;
            std::cout << "  " << fileCount << ". " << file.string() << std::endl;
        }
        std::cout << std::endl;
    }
}

void CLI::displaySummary(const DuplicateList& duplicates) {
    if (duplicates.empty()) {
        std::cout << "No duplicate files found." << std::endl;
        return;
    }

    // count duplicate groups and files
    int totalGroups = duplicates.size();
    int totalFiles = 0;
    FileSize totalWasted = 0;

    for (const auto& group : duplicates) {
        totalFiles += group.files.size();
        totalWasted += group.wastedSpace();
    }

    std::cout << "Summary:" << std::endl;
    std::cout << "  Duplicate groups: " << totalGroups << std::endl;
    std::cout << "  Duplicate files: " << totalFiles << std::endl;
    std::cout << "  Total files (including originals): " << totalFiles << std::endl;
    std::cout << "  Wasted space: " << formatSize(totalWasted) << std::endl;
}

void CLI::handleDuplicateDeletion(DuplicateList& duplicates, bool dryRun, bool interactive) {
    if (duplicates.empty()) {
        return;
    }

    if (dryRun) {
        std::cout << "Dry run mode - no files will be deleted." << std::endl;
        return;
    }

    if (!interactive) {
        // delete all duplicates 
        // (keeping only the first file in each group)
        int deletedFiles = 0;
        FileSize freedSpace = 0;

        for (auto& group : duplicates) {
            // keep the first file, delete the rest
            for (size_t i = 1; i < group.files.size(); i++) {
                try {
                    fs::remove(group.files[i]);
                    deletedFiles++;
                    freedSpace += group.fileSize;
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "error deleting file " << group.files[i]
                              << ": " << e.what() << std::endl;
                }
            }
        }

        std::cout << "Deleted " << deletedFiles << " files, freed "
                  << formatSize(freedSpace) << " of space." << std::endl;

        return;
    }

    // interactive deletion
    std::cout << "Interactive deletion mode:" << std::endl;
    std::cout << "For each group, you can:" << std::endl;
    std::cout << "  - Choose which files to keep (space-separated numbers)" << std::endl;
    std::cout << "  - Type 'all' to keep all files" << std::endl;
    std::cout << "  - Type 'skip' to skip this group" << std::endl;
    std::cout << "  - Type 'quit' to exit" << std::endl;

    int groupIndex = 0;
    int deletedFiles = 0;
    FileSize freedSpace = 0;

    for (auto& group : duplicates) {
        groupIndex++;

        std::cout << std::endl;
        std::cout << "Group " << groupIndex << "/" << duplicates.size()
                  << " (Size: " << formatSize(group.fileSize) << ")" << std::endl;

        for (size_t i = 0; i < group.files.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << group.files[i].string() << std::endl;
        }

        std::cout << "Enter files to KEEP (1-" << group.files.size() << "): ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "quit" || input == "q") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        if (input == "skip" || input == "s") {
            std::cout << "Skipping group..." << std::endl;
            continue;
        }

        if (input == "all" || input == "a") {
            std::cout << "Keeping all files in this group." << std::endl;
            continue;
        }

        // parse input to get which files to keep
        std::vector<int> filesToKeep;
        std::stringstream ss(input);
        int fileIndex;

        while (ss >> fileIndex) {
            if (fileIndex >= 1 && fileIndex <= static_cast<int>(group.files.size())) {
                filesToKeep.push_back(fileIndex - 1);
            }
        }

        // delete files not in the keep list
        for (size_t i = 0; i < group.files.size(); i++) {
            if (std::find(filesToKeep.begin(), filesToKeep.end(), i) == filesToKeep.end()) {
                try {
                    std::cout << "Deleting: " << group.files[i].string() << std::endl;
                    fs::remove(group.files[i]);
                    deletedFiles++;
                    freedSpace += group.fileSize;
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "error deleting file " << group.files[i]
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    std::cout << std::endl;
    std::cout << "Deleted " << deletedFiles << " files, freed "
              << formatSize(freedSpace) << " of space." << std::endl;
}

std::string CLI::formatSize(FileSize size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double adjustedSize = static_cast<double>(size);

    while (adjustedSize >= 1024.0 && unitIndex < 4) {
        adjustedSize /= 1024.0;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << adjustedSize << " " << units[unitIndex];
    return ss.str();
}

}