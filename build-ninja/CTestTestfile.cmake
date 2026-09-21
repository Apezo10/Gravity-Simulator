# CMake generated Testfile for 
# Source directory: C:/Users/Adins/Downloads/Learning-cpp/Practice
# Build directory: C:/Users/Adins/Downloads/Learning-cpp/Practice/build-ninja
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("input_validation" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build-ninja/input_validation_tests.exe")
set_tests_properties("input_validation" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;33;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("simulation_timing" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build-ninja/simulation_timing_tests.exe")
set_tests_properties("simulation_timing" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;38;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("collisions" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build-ninja/collision_tests.exe")
set_tests_properties("collisions" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;43;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
add_test("orbital_accuracy" "C:/Users/Adins/Downloads/Learning-cpp/Practice/build-ninja/orbital_accuracy_tests.exe")
set_tests_properties("orbital_accuracy" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;48;add_test;C:/Users/Adins/Downloads/Learning-cpp/Practice/CMakeLists.txt;0;")
subdirs("_deps/sfml-build")
