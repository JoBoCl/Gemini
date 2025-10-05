cmake_minimum_required(VERSION 3.10)
project(Gemini VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(PLUGIN_NAME 4msCompany)
set(ADDITIONAL_PLUGIN_DISTRIBUTABLES res LICENSE.md README.md)

include(RackSDK.cmake)


message(STATUS "Installing into '${CMAKE_INSTALL_PREFIX}'")

target_compile_options(${RACK_PLUGIN_LIB} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)

add_compile_definitions(VCVRACK)

add_library(Gemini SHARED src/)
set_target_properties(Gemini PROPERTIES LINKER_LANGUAGE CXX)

# Change path from /src if needed, or add more directories
file(GLOB_RECURSE sources
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "/usr/include/vcvrack/**/*.hpp"
        "${CMAKE_SOURCE_DIR/DSPFilters/shared/DSPFiltesr/include/DspFilters/*.h"
        )

target_include_directories(Gemini PUBLIC "${RACK_DIR}/src")
