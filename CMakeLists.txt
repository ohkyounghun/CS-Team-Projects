cmake_minimum_required(VERSION 3.27)
project(CSTP C)

set(CMAKE_C_STANDARD 11)

# Build each program as its own executable to avoid multiple 'main' conflicts.
add_executable(CSTP main.c)    # optional: remove if you don't have main.c

add_executable(prob1 prob1.c)
add_executable(prob2 prob2.c)
add_executable(prob3 prob3.c)
