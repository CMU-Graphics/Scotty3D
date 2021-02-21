cmake_minimum_required(VERSION 3.16)

project(glad LANGUAGES CXX)

set(SOURCES_GLAD "glad.cpp"
                 "glad.h"
                 "khrplatform.h")

add_library(glad STATIC ${SOURCES_GLAD})

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
endif()

if(LINUX)
    target_link_libraries(glad ${CMAKE_DL_LIBS})
endif()

