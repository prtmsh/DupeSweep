#include "hashing.h"
#include "dupesweep/constants.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <atomic>
#include <iostream>

#include <xxhash.h>

namespace dupesweep {

std::string Hashing::quickHash(const FilePath& path) {
    unsigned char buffer[QUICK_HASH_BYTES];

    // open file and read the first QUICK_HASH_BYTES bytes
    std::ifstream file(path, std::ios::binary);
    if(!file) {
        throw std::runtime_error("Cannot open file for quick hashing: " + path.string());
    }

    file.read(reinterpret_cast<char*>(buffer), QUICK_HASH_BYTES);
    size_t bytesRead = file.gcount();

    // calculate xxhash
    XXH64_hash_t hash = XXH64(buffer, bytesRead, XXHASH_SEED);

    // convert hash to hex string
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string Hashing::fullHash(const FilePath& path) {
    unsigned char buffer[HASH_BUFFER_SIZE];

    // open the file
    std::ifstream file(path, std::ios::binary);
    if(!file) {
        throw std::runtime_error("cannot open file for full hashing: " + path.string());
    }

    // initialize xxhash state
    XXH64_state_t* state = XXH64_createState();
    if (state == nullptr) {
        throw std::runtime_error("failed to create xxHash state");
    }

    XXH64_reset(state, XXHASH_SEED);

    // read and update hash in chunks
    while(file) {
        file.read(reinterpret_cast<char*>(buffer), HASH_BUFFER_SIZE);
        size_t bytesRead = file.gcount();
        if(bytesRead > 0) {
            XXH64_update(state, buffer, bytesRead);
        }
        if(bytesRead < HASH_BUFFER_SIZE) {
            break;
        }
    }

    XXH64_hash_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    // convert hash to hex string
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

HashGroup Hashing::groupByQuickHash(const std::vector<FilePath>& files) {
    HashGroup quickHashGroups;

    for(const auto& path: files) {
        try {
            std::string hash = quickHash(path);
            quickHashGroups[hash].push_back(path);
        } catch (const std::exception& e) {
            std::cerr << "error hashing file " << path << ": " << e.what() << "\n";
        }
    }

    return quickHashGroups;
}

HashGroup Hashing::groupByFullHash(const std::vector<FilePath>& files) {
    HashGroup fullHashGroups;

    for(const auto& path: files) {
        try {
            std::string hash = fullHash(path);
            fullHashGroups[hash].push_back(path);
        } catch (const std::exception& e) {
            std::cerr << "error hashing file " << path << ": " << e.what() << "\n";
        }
    }

    return fullHashGroups;
}

HashGroup Hashing::processFilesParallel(
    const std::vector<FilePath>& files,
    const std::function<std::string(const FilePath&)>& hashFunction,
    int numThreads,
    std::atomic<int>& processedFiles,
    const std::function<void(int, int)>& progressCallback,
    int totalFiles
) {
    HashGroup hashGroups;
    std::mutex hashGroupsMutex;

    // determine the number of threads to use
    int threadCount = numThreads > 0 ? numThreads : std::thread::hardware_concurrency();

    if(threadCount == 0) {
        threadCount = DEFAULT_THREAD_COUNT;
    }

    // create a vector of futures
    std::vector<std::future<void>> futures;

    // calculate batch size
    size_t batchSize = files.size() / threadCount;
    if(batchSize==0) batchSize = 1;

    // launch the threadds
    for(size_t i=0; i<files.size(); i+=batchSize) {
        size_t end = std::min(i+batchSize, files.size());

        futures.push_back(std::async(std::launch::async, [&, i, end]() {
            HashGroup localGroups;

            for(size_t j=i; j<end; j++) {
                try {
                    std::string hash = hashFunction(files[j]);
                    localGroups[hash].push_back(files[j]);

                    //update progress
                    int processed = ++processedFiles;
                    progressCallback(processed, totalFiles);
                } catch(const std::exception& e) {
                    std::cerr << "Error hashing file " << files[j] << ": " << e.what() << "\n";
                }
            }

            // merge local results with global
            std::lock_guard<std::mutex> lock(hashGroupsMutex);
            for(const auto& [hash, paths]: localGroups) {
                hashGroups[hash].insert(hashGroups[hash].end(), paths.begin(), paths.end());
            }
        }));
    }

    // wait for all threads to complete
    for(auto& future: futures) {
        future.wait();
    }

    return hashGroups;
}

HashGroup Hashing::findDuplicates(
    const SizeGroup& sizeGroups,
    int numThreads,
    const std::function<void(int, int)>& progressCallback
) {
    HashGroup duplicates;

    // count total files for progress reporting
    int totalFiles = 0;
    for(const auto& [size, paths]: sizeGroups) {
        totalFiles += paths.size();
    }

    std::atomic<int> processedFiles(0);

    // process each size group
    for(const auto& [size, paths]: sizeGroups) {
        //skip singleton groups
        if(paths.size() <= 1) continue;

        // group by quick hash first
        HashGroup quickHashGroups = groupByQuickHash(paths);

        // for each quick hash group, calculate full hash
        for(const auto& [quickHash, quickHashPaths]: quickHashGroups) {
            // skip singleton groups
            if(quickHashPaths.size() <= 1) continue;
            
            // calculate full hashes in parallel
            HashGroup fullHashGroups = processFilesParallel(
                quickHashPaths,
                fullHash,
                numThreads,
                processedFiles,
                progressCallback,
                totalFiles
            );

            // add duplicate groups to results
            for(const auto& [fullHash, fullHashPaths]: fullHashGroups) {
                if(fullHashPaths.size() > 1) {
                    duplicates[fullHash] = fullHashPaths;
                }
            }
        }
    }

    return duplicates;
}

}