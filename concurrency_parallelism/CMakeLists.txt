cmake_minimum_required(VERSION 3.15)
project(concurrency C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(concurrency main.c)
target_compile_options(concurrency PRIVATE -W -fverbose-asm)
target_link_libraries(concurrency pthread)
