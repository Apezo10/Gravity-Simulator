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
target_compile_definitions(vectors PRIVATE SIM_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/tuffy.ttf")

include(CTest)
if(BUILD_TESTING)
    add_executable(input_validation_tests tests/input_validation.cpp)
    target_compile_features(input_validation_tests PRIVATE cxx_std_17)
    target_link_libraries(input_validation_tests PRIVATE SFML::Graphics)
    add_test(NAME input_validation COMMAND input_validation_tests)
    set_tests_properties(input_validation PROPERTIES TIMEOUT 15)
    add_executable(simulation_timing_tests tests/simulation_timing.cpp)
    target_compile_features(simulation_timing_tests PRIVATE cxx_std_17)
    target_link_libraries(simulation_timing_tests PRIVATE SFML::Graphics)
    add_test(NAME simulation_timing COMMAND simulation_timing_tests)
    set_tests_properties(simulation_timing PROPERTIES TIMEOUT 15)
    add_executable(collision_tests tests/collisions.cpp)
    target_compile_features(collision_tests PRIVATE cxx_std_17)
    target_link_libraries(collision_tests PRIVATE SFML::Graphics)
    add_test(NAME collisions COMMAND collision_tests)
    set_tests_properties(collisions PROPERTIES TIMEOUT 15)
endif()
