cmake_minimum_required(VERSION 3.28)
project(VectorsPractice LANGUAGES CXX)

include(FetchContent)

# Build only the SFML modules needed by Graphics, and avoid SFML runtime DLLs.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Build SFML audio")
set(SFML_BUILD_NETWORK OFF CACHE BOOL "Build SFML network")

FetchContent_Declare(SFML
    URL https://github.com/SFML/SFML/archive/refs/tags/3.0.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(SFML)

add_executable(vectors vectors.cpp)
target_compile_features(vectors PRIVATE cxx_std_17)
target_link_libraries(vectors PRIVATE SFML::Graphics)
