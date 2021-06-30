cmake_minimum_required(VERSION 3.1)
project(dancing)

find_package(Threads)

include_directories("src/include")

add_subdirectory(src)
