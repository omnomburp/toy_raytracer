cmake_minimum_required(VERSION 3.10)

project(toy-raytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
    add_compile_options($<$<CONFIG:Release>:/O2>)
else()
    add_compile_options($<$<CONFIG:Release>:-O3>)
endif()

add_executable(toy-raytracer
    src/main.cpp
)