cmake_minimum_required(VERSION 3.22)
project(ColonySim VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COLONY_BUILD_TESTS "Build GoogleTest targets" ON)
option(COLONY_BUILD_PYTHON "Build pybind11 Python bindings" ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

add_subdirectory(core)

if(COLONY_BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()

if(COLONY_BUILD_PYTHON)
    add_subdirectory(bindings/colony_py)
endif()