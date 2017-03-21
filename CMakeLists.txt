cmake_minimum_required(VERSION 3.6)
project(cachelab_handout)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES library.c library.h csim.c)
add_library(cachelab_handout ${SOURCE_FILES})