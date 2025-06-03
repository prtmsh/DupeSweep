cmake_minimum_required(VERSION 3.10)
project(DupeSweep VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
if(APPLE)
    message(STATUS "macOS detected. Attempting to locate OpenSSL in common Homebrew paths.")
    message(STATUS "If OpenSSL is not found, ensure it's installed (e.g., via 'brew install openssl@3' or 'openssl@1.1')")
    message(STATUS "and you may need to set OPENSSL_ROOT_DIR, e.g.,")
    message(STATUS "  export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)")
    message(STATUS "or pass it to CMake: -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)")

    # Add common Homebrew paths for OpenSSL.
    # CMake will search these in addition to standard paths and OPENSSL_ROOT_DIR.
    # Check for common Homebrew installation paths.
    # The order here can matter if multiple versions are somehow symlinked.
    # Specific version paths are generally more reliable.
    set(POTENTIAL_OPENSSL_PATHS
        "/opt/homebrew/opt/openssl@3"   # Apple Silicon Homebrew for openssl@3
        "/usr/local/opt/openssl@3"      # Intel Homebrew for openssl@3
        "/opt/homebrew/opt/openssl@1.1" # Apple Silicon Homebrew for openssl@1.1
        "/usr/local/opt/openssl@1.1"    # Intel Homebrew for openssl@1.1
        "/opt/homebrew/opt/openssl"     # Apple Silicon Homebrew for generic 'openssl'
        "/usr/local/opt/openssl"        # Intel Homebrew for generic 'openssl'
    )

    foreach(P_PATH ${POTENTIAL_OPENSSL_PATHS})
        if(EXISTS "${P_PATH}/include/openssl/ssl.h" AND NOT DEFINED ENV{OPENSSL_ROOT_DIR} AND NOT OPENSSL_ROOT_DIR)
            # Only set OPENSSL_ROOT_DIR if it's not already set by user (env or CMake var)
            # and if the path actually looks like an OpenSSL installation.
            # This is a softer way than directly appending to CMAKE_PREFIX_PATH for this specific case.
            # find_package(OpenSSL) often prefers OPENSSL_ROOT_DIR.
            # Alternatively, one could append to CMAKE_PREFIX_PATH and let find_package do its work.
            # Let's try with CMAKE_PREFIX_PATH approach as it's more standard for find_package.
            break() # Found one, no need to set OPENSSL_ROOT_DIR from here.
        endif()
    endforeach()

    # Add Homebrew prefix paths to CMAKE_PREFIX_PATH, which find_package uses.
    # This is generally a more robust way for find_package.
    if(EXISTS "/opt/homebrew/bin/brew")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt") # For Apple Silicon
    endif()
    if(EXISTS "/usr/local/bin/brew")
        list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt") # For Intel
    endif()
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
    # For debugging, you can uncomment the next line to see the effect:
    # message(STATUS "CMAKE_PREFIX_PATH has been extended to: ${CMAKE_PREFIX_PATH}")
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect source files
file(GLOB SOURCES "src/*.cpp")

# Add executable
add_executable(dupesweep ${SOURCES})

# Link with OpenSSL and Threads
# Using PRIVATE as these are link dependencies for the executable, not its public interface.
target_link_libraries(dupesweep PRIVATE OpenSSL::Crypto Threads::Threads)

# Link with filesystem library if needed (gcc < 9.0)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dupesweep PRIVATE stdc++fs)
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Install target
install(TARGETS dupesweep DESTINATION bin)