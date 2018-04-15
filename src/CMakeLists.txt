# (C) Copyright Shou Hao Ho   2018
# Distributed under the MIT Software License (See accompanying LICENSE file)

cmake_minimum_required(VERSION 3.1)

add_executable (run_placer random_netlist.cpp chip.cpp iterative_placement.cpp plan.cpp analytical_placement.cpp run_placer.cpp)