cmake_minimum_required(VERSION 3.16)

project(sf_libs LANGUAGES CXX)

set(SOURCES_SF "sf_libs.cpp" "stb_image.h" "stb_image_write.h" "stb_perlin.h" "tinyexr.h" "CLI11.hpp")
add_library(sf_libs STATIC ${SOURCES_SF})

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
endif()

