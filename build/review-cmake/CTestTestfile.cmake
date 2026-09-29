# CMake generated Testfile for 
# Source directory: C:/Users/Adins/Downloads/Learning-cpp/Practice
# Build directory: C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("grid_cache" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/grid_cache_tests.exe")
set_tests_properties("grid_cache" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;32;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("trajectory_preview" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/trajectory_preview_tests.exe")
set_tests_properties("trajectory_preview" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;37;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("input_validation" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/input_validation_tests.exe")
set_tests_properties("input_validation" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;43;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("simulation_timing" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/simulation_timing_tests.exe")
set_tests_properties("simulation_timing" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;48;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("collisions" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/collision_tests.exe")
set_tests_properties("collisions" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;53;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("orbital_accuracy" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/orbital_accuracy_tests.exe")
set_tests_properties("orbital_accuracy" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;58;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("simulation_controls" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build/review-cmake/simulation_controls_tests.exe")
set_tests_properties("simulation_controls" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;62;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
subdirs("_deps/sfml-build")
