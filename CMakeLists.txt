cmake_minimum_required(VERSION 3.5)

project(Psychopath LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    Psychopath main.cpp

    game_tree.cpp
    montecarlo_tree_search.cpp

    game_tree.h
    montecarlo_tree_search.h
)
