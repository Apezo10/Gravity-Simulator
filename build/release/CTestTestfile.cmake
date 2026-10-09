# CMake generated Testfile for 
# Source directory: C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim
# Build directory: C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("render_cache" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/render_cache_tests.exe")
set_tests_properties("render_cache" PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;33;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("grid_cache" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/grid_cache_tests.exe")
set_tests_properties("grid_cache" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;38;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("trajectory_preview" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/trajectory_preview_tests.exe")
set_tests_properties("trajectory_preview" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;43;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("input_validation" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/input_validation_tests.exe")
set_tests_properties("input_validation" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;49;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("simulation_timing" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/simulation_timing_tests.exe")
set_tests_properties("simulation_timing" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;54;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("collisions" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/collision_tests.exe")
set_tests_properties("collisions" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;59;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("orbital_accuracy" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/orbital_accuracy_tests.exe")
set_tests_properties("orbital_accuracy" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;64;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
add_test("simulation_controls" "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/build/release/simulation_controls_tests.exe")
set_tests_properties("simulation_controls" PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;68;add_test;C:/Users/Adins/OneDrive/Documents/VScode Projects/Gravity Sim/CMakeLists.txt;0;")
subdirs("_deps/sfml-build")
