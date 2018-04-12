cmake_minimum_required(VERSION 3.1)

add_executable (test_placer random_netlist.cpp chip.cpp iterative_placement.cpp plan.cpp test_placer.cpp)
