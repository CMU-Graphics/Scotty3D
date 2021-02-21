cmake_minimum_required(VERSION 3.16)

project(imgui LANGUAGES CXX)

set(SOURCES_IMGUI "imgui.cpp"
                  "imgui.h"
                  "imconfig.h"
                  "imgui_internal.h"
                  "imstb_rectpack.h"
                  "imstb_textedit.h"
                  "imstb_truetype.h"
                  "imgui_draw.cpp"
                  "imgui_widgets.cpp")

add_library(imgui STATIC ${SOURCES_IMGUI})

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
endif()

set_target_properties(imgui PROPERTIES
            		  CXX_STANDARD 17
            		  CXX_EXTENSIONS OFF)
