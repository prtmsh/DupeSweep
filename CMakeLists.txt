cmake_minimum_required(VERSION 3.10)
project(DupeSweep VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Collect source files
file(GLOB SOURCES "src/*.cpp")

# Add executable
add_executable(dupesweep ${SOURCES})

# Link with OpenSSL
target_link_libraries(dupesweep OpenSSL::Crypto)

# Link with filesystem library if needed (gcc < 9.0)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dupesweep stdc++fs)
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Install target
install(TARGETS dupesweep DESTINATION bin)