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

add_library(simulation STATIC Simulation/planet_system.cpp)
target_compile_features(simulation PUBLIC cxx_std_17)
target_include_directories(simulation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Simulation" "${CMAKE_CURRENT_SOURCE_DIR}/Rendering")

add_library(simulation_setup STATIC Simulation/setup.cpp)
target_link_libraries(simulation_setup PUBLIC simulation)

add_executable(vectors main.cpp Rendering/renderer.cpp Rendering/setup_menu.cpp)
target_link_libraries(vectors PRIVATE simulation_setup SFML::Graphics)
target_compile_definitions(vectors PRIVATE SIM_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Rendering/tuffy.ttf")

include(CTest)
if(BUILD_TESTING)
    add_executable(grid_cache_tests tests/grid_cache.cpp)
    target_link_libraries(grid_cache_tests PRIVATE simulation SFML::Graphics)
    add_test(NAME grid_cache COMMAND grid_cache_tests)
    set_tests_properties(grid_cache PROPERTIES TIMEOUT 15)

    add_executable(trajectory_preview_tests tests/trajectory_preview.cpp)
    target_link_libraries(trajectory_preview_tests PRIVATE simulation)
    add_test(NAME trajectory_preview COMMAND trajectory_preview_tests)
    set_tests_properties(trajectory_preview PROPERTIES TIMEOUT 15)

    add_executable(input_validation_tests tests/input_validation.cpp)
    target_compile_features(input_validation_tests PRIVATE cxx_std_17)
    target_link_libraries(input_validation_tests PRIVATE simulation_setup)
    add_test(NAME input_validation COMMAND input_validation_tests)
    set_tests_properties(input_validation PROPERTIES TIMEOUT 15)
    add_executable(simulation_timing_tests tests/simulation_timing.cpp)
    target_compile_features(simulation_timing_tests PRIVATE cxx_std_17)
    target_link_libraries(simulation_timing_tests PRIVATE simulation_setup)
    add_test(NAME simulation_timing COMMAND simulation_timing_tests)
    set_tests_properties(simulation_timing PROPERTIES TIMEOUT 15)
    add_executable(collision_tests tests/collisions.cpp)
    target_compile_features(collision_tests PRIVATE cxx_std_17)
    target_link_libraries(collision_tests PRIVATE simulation_setup)
    add_test(NAME collisions COMMAND collision_tests)
    set_tests_properties(collisions PROPERTIES TIMEOUT 15)
    add_executable(orbital_accuracy_tests tests/orbital_accuracy.cpp)
    target_compile_features(orbital_accuracy_tests PRIVATE cxx_std_17)
    target_link_libraries(orbital_accuracy_tests PRIVATE simulation_setup)
    add_test(NAME orbital_accuracy COMMAND orbital_accuracy_tests)
    set_tests_properties(orbital_accuracy PROPERTIES TIMEOUT 15)
    add_executable(simulation_controls_tests tests/simulation_controls.cpp)
    target_link_libraries(simulation_controls_tests PRIVATE simulation)
    add_test(NAME simulation_controls COMMAND simulation_controls_tests)
    set_tests_properties(simulation_controls PROPERTIES TIMEOUT 15)
endif()
