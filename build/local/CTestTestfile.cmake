# CMake generated Testfile for 
# Source directory: C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator
# Build directory: C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[trajectory_preview]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/trajectory_preview_tests.exe")
set_tests_properties([=[trajectory_preview]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;32;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
add_test([=[input_validation]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/input_validation_tests.exe")
set_tests_properties([=[input_validation]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;38;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
add_test([=[simulation_timing]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/simulation_timing_tests.exe")
set_tests_properties([=[simulation_timing]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;43;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
add_test([=[collisions]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/collision_tests.exe")
set_tests_properties([=[collisions]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;48;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
add_test([=[orbital_accuracy]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/orbital_accuracy_tests.exe")
set_tests_properties([=[orbital_accuracy]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;53;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
add_test([=[simulation_controls]=] "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/build/local/simulation_controls_tests.exe")
set_tests_properties([=[simulation_controls]=] PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;57;add_test;C:/Users/Adins/OneDrive/Documents/VScode_Projects/Gravity Sim/Gravity-Simulator/CMakeLists.txt;0;")
subdirs("_deps/sfml-build")
