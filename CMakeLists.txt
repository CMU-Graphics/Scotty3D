
cmake_minimum_required(VERSION 3.16)

# define project

project(Scotty3D VERSION 1.1
                 DESCRIPTION "CMU 15-462/662 - Computer Graphics"
                 LANGUAGES CXX)

set(SCOTTY3D_BUILD_REF false)

if(SCOTTY3D_BUILD_REF)
    add_definitions(-DSCOTTY3D_BUILD_REF)
endif()

# define sources

set(SOURCES_SCOTTY3D_GUI
                    "src/gui/manager.cpp"
                    "src/gui/manager.h"
                    "src/gui/model.cpp"
                    "src/gui/model.h"
                    "src/gui/layout.cpp"
                    "src/gui/layout.h"
                    "src/gui/animate.cpp"
                    "src/gui/animate.h"
                    "src/gui/widgets.cpp"
                    "src/gui/widgets.h"
                    "src/gui/rig.cpp"
                    "src/gui/rig.h"
                    "src/gui/simulate.cpp"
                    "src/gui/simulate.h"
                    "src/gui/render.cpp"
                    "src/gui/render.h")
set(SOURCES_SCOTTY3D_GEOM
                    "src/geometry/halfedge.cpp"
                    "src/geometry/halfedge.h"
                    "src/geometry/util.cpp"
                    "src/geometry/util.h"
                    "src/geometry/spline.h"
                    "src/geometry/spline.inl")
set(SOURCES_SCOTTY3D_RAYS
                    "src/rays/pathtracer.cpp"
                    "src/rays/pathtracer.h"
                    "src/rays/light.cpp"
                    "src/rays/light.h"
                    "src/rays/bsdf.h"
                    "src/rays/env_light.h"
                    "src/rays/bvh.h"
                    "src/rays/list.h"
                    "src/rays/object.h"
                    "src/rays/samplers.h"
                    "src/rays/tri_mesh.h"
                    "src/rays/shapes.h")
set(SOURCES_SCOTTY3D_UTIL
                    "src/util/hdr_image.cpp"
                    "src/util/hdr_image.h"
                    "src/util/camera.cpp"
                    "src/util/camera.h"
                    "src/util/thread_pool.cpp"
                    "src/util/thread_pool.h"
                    "src/util/rand.h"
                    "src/util/rand.cpp")
set(SOURCES_SCOTTY3D_PLATFORM
                    "src/platform/gl.cpp"
                    "src/platform/platform.cpp"
                    "src/platform/gl.h"
                    "src/platform/platform.h"
                    "deps/imgui/imgui_impl_opengl3.cpp"
                    "deps/imgui/imgui_impl_opengl3.h"
                    "deps/imgui/imgui_impl_sdl.cpp"
                    "deps/imgui/imgui_impl_sdl.h")
set(SOURCES_SCOTTY3D_SCENE
                    "src/scene/undo.cpp"
                    "src/scene/undo.h"
                    "src/scene/renderer.cpp"
                    "src/scene/renderer.h"
                    "src/scene/scene.cpp"
                    "src/scene/scene.h"
                    "src/scene/pose.cpp"
                    "src/scene/pose.h"
                    "src/scene/light.cpp"
                    "src/scene/light.h"
                    "src/scene/skeleton.cpp"
                    "src/scene/skeleton.h"
                    "src/scene/particles.cpp"
                    "src/scene/particles.h"
                    "src/scene/material.cpp"
                    "src/scene/material.h"
                    "src/scene/object.cpp"
                    "src/scene/object.h")
set(SOURCES_SCOTTY3D_LIB
                    "src/lib/bbox.h"
                    "src/lib/line.h"
                    "src/lib/log.h"
                    "src/lib/mat4.h"
                    "src/lib/mathlib.h"
                    "src/lib/plane.h"
                    "src/lib/quat.h"
                    "src/lib/ray.h"
                    "src/lib/spectrum.h"
                    "src/lib/vec2.h"
                    "src/lib/vec3.h"
                    "src/lib/vec4.h")
if(SCOTTY3D_BUILD_REF)
    set(SOURCES_SCOTTY3D_STUDENT
                    "src/reference/meshedit.cpp"
                    "src/reference/camera.cpp"
                    "src/reference/pathtracer.cpp"
                    "src/reference/samplers.cpp"
                    "src/reference/shapes.cpp"
                    "src/reference/bsdf.cpp"
                    "src/reference/bbox.cpp"
                    "src/reference/particles.cpp"
                    "src/reference/bvh.inl"
                    "src/reference/spline.inl"
                    "src/reference/skeleton.cpp"
                    "src/reference/env_light.cpp"
                    "src/reference/tri_mesh.cpp")
else()
    set(SOURCES_SCOTTY3D_STUDENT
                    "src/student/meshedit.cpp"
                    "src/student/camera.cpp"
                    "src/student/pathtracer.cpp"
                    "src/student/samplers.cpp"
                    "src/student/shapes.cpp"
                    "src/student/bsdf.cpp"
                    "src/student/particles.cpp"
                    "src/student/bbox.cpp"
                    "src/student/debug.h"
                    "src/student/debug.cpp"
                    "src/student/bvh.inl"
                    "src/student/spline.inl"
                    "src/student/env_light.cpp"
                    "src/student/skeleton.cpp"
                    "src/student/tri_mesh.cpp")
endif()

set(SOURCES_SCOTTY3D ${SOURCES_SCOTTY3D_UTIL}
                     ${SOURCES_SCOTTY3D_GUI}
                     ${SOURCES_SCOTTY3D_GEOM}
                     ${SOURCES_SCOTTY3D_RAYS}
                     ${SOURCES_SCOTTY3D_PLATFORM}
                     ${SOURCES_SCOTTY3D_STUDENT}
                     ${SOURCES_SCOTTY3D_SCENE}
                     ${SOURCES_SCOTTY3D_LIB}
                     "src/app.cpp"
                     "src/app.h"
                     "src/main.cpp")


# setup OS-specific options

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "-framework AppKit")
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS}/..)
	include_directories(${SDL2_INCLUDE_DIRS})
	link_directories(${SDL2_LIBRARY_DIRS})
	add_definitions(${SDL2_CFLAGS_OTHER})
endif()

if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    include_directories(${SDL2_INCLUDE_DIRS})
    link_directories(${SDL2_LIBRARY_DIRS})
    add_definitions(${SDL2_CFLAGS_OTHER})
endif()







# define executable

if(WIN32)
    add_executable(Scotty3D ${SOURCES_SCOTTY3D})
else()
    add_executable(Scotty3D ${SOURCES_SCOTTY3D})
endif()

set_target_properties(Scotty3D PROPERTIES
                      CXX_STANDARD 17
                      CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(Scotty3D PRIVATE /MP /W4 /WX /wd4201 /wd4840 /wd4100 /fp:fast)
else()
    target_compile_options(Scotty3D PRIVATE -Wall -Wextra -Werror -Wno-reorder -Wno-unused-parameter)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Scotty3D PRIVATE -fno-omit-frame-pointer)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(Scotty3D PRIVATE Threads::Threads)



# define include paths

target_include_directories(Scotty3D PRIVATE "deps/" "deps/assimp/include")
target_include_directories(Scotty3D PRIVATE "${CMAKE_BINARY_DIR}/deps/assimp/include")
include_directories("${Scotty3D_SOURCE_DIR}/deps/")
include_directories("${Scotty3D_SOURCE_DIR}/src/")

source_group(lib FILES ${SOURCES_SCOTTY3D_LIB})
source_group(scene FILES ${SOURCES_SCOTTY3D_SCENE})
source_group(platform FILES ${SOURCES_SCOTTY3D_PLATFORM})




# build dependencies

add_subdirectory("deps/imgui/")
add_subdirectory("deps/glad/")
add_subdirectory("deps/nfd/")
add_subdirectory("deps/sf_libs/")

set(ASSIMP_BUILD_COLLADA_IMPORTER TRUE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_PLY_IMPORTER TRUE)
set(ASSIMP_BUILD_FBX_IMPORTER TRUE)
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE)
set(ASSIMP_BUILD_3DS_IMPORTER TRUE)
set(ASSIMP_BUILD_STL_IMPORTER TRUE)
set(ASSIMP_BUILD_BLEND_IMPORTER TRUE)
set(ASSIMP_BUILD_COLLADA_EXPORTER TRUE)
add_subdirectory("deps/assimp/")
include_directories(${ASSIMP_INCLUDE_DIRS})




# link libraries

if(WIN32)
    target_include_directories(Scotty3D PRIVATE "deps/win")
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \"${CMAKE_CURRENT_SOURCE_DIR}/src/platform/icon.res\" /IGNORE:4098 /IGNORE:4099")
    endif()
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    target_link_libraries(Scotty3D PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/win/SDL2/SDL2main.lib")
    target_link_libraries(Scotty3D PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/win/SDL2/SDL2.lib")
    target_link_libraries(Scotty3D PRIVATE Winmm)
    target_link_libraries(Scotty3D PRIVATE Version)
    target_link_libraries(Scotty3D PRIVATE Setupapi)
    target_link_libraries(Scotty3D PRIVATE Shcore)
endif()

if(LINUX)
    target_link_libraries(Scotty3D PRIVATE SDL2)
endif()

if(APPLE)
	target_link_libraries(Scotty3D PRIVATE ${SDL2_LIBRARIES})
endif()

target_link_libraries(Scotty3D PRIVATE assimp)
target_link_libraries(Scotty3D PRIVATE nfd)
target_link_libraries(Scotty3D PRIVATE sf_libs)
target_link_libraries(Scotty3D PRIVATE imgui)
target_link_libraries(Scotty3D PRIVATE glad)
