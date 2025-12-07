cmake_minimum_required(VERSION 3.1)
Project(test_storeprop)

# support C++17 features used by LLVM
set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(store_prop)  # Use your pass name here.
