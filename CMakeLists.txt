cmake_minimum_required(VERSION 3.1)
project(dancing)

find_package(Threads)

enable_testing()

# Load Check package (copied and trimmed from FindCheck.cmake example file).
INCLUDE( FindPkgConfig )
PKG_SEARCH_MODULE( CHECK check )
MARK_AS_ADVANCED( CHECK_INCLUDE_DIR CHECK_LIBRARIES )

include_directories("src/include")

add_subdirectory(src)
