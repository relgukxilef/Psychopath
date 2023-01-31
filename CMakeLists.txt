cmake_minimum_required(VERSION 3.5)

project(Psychopath LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    Psychopath test.cpp
    source/utility.h source/utility.cpp
    source/montecarlo_tree_search.h source/montecarlo_tree_search.cpp
)

target_link_options(Psychopath PRIVATE -static)
