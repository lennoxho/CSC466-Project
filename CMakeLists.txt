cmake_minimum_required(VERSION 3.1)
project(CSC466-project)

set(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}")
if (NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR "EIGEN3_INCLUDE_DIR not set!")
endif()
include_directories(SYSTEM "${EIGEN3_INCLUDE_DIR}")

find_package(Boost REQUIRED)
include_directories("${Boost_INCLUDE_DIRS}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if (MSVC)
    ADD_DEFINITIONS("/EHsc")
endif (MSVC)

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(src)