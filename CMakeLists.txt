cmake_minimum_required(VERSION 3.10)
project(Gemini)
add_library(Gemini SHARED src/)
set_target_properties(Gemini PROPERTIES LINKER_LANGUAGE CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Change path from /src if needed, or add more directories
file(GLOB_RECURSE sources
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        )

target_include_directories(Gemini PUBLIC "${RACK_DIR}/src")
