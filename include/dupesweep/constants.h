#pragma once

#include <cstddef>

namespace dupesweep {

    /*
        buffer size for quick hash,
        ie hash these many bytes first. 
        if they don't match, file is obviously not a duplicate
    */
    constexpr size_t QUICK_HASH_BYTES = 1024;

    // similar 4MB buffer for full hash
    constexpr size_t HASH_BUFFER_SIZE = 4 * 1024 * 1024;

    constexpr int DEFAULT_THREAD_COUNT = 4;

    // xxHash seed for consistent results
    constexpr unsigned int XXHASH_SEED = 0;
}