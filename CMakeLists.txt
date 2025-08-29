cmake_minimum_required(VERSION 3.30)
project(Scotty3D)

set(CMAKE_CXX_STANDARD 17)

include_directories(deps/)

if (APPLE)
    set(PLATFORM "macos")
elseif (UNIX AND NOT APPLE)
    set(PLATFORM "linux")
elseif (WIN32)
    set(PLATFORM "windows")
endif ()

include_directories(nest-libs/${PLATFORM}/freetype/include)
include_directories(nest-libs/${PLATFORM}/glm/include)
include_directories(nest-libs/${PLATFORM}/harfbuzz/include)
include_directories(nest-libs/${PLATFORM}/libogg/include)
include_directories(nest-libs/${PLATFORM}/libogg/include/ogg)
include_directories(nest-libs/${PLATFORM}/libopus/include)
include_directories(nest-libs/${PLATFORM}/libopusenc/include)
include_directories(nest-libs/${PLATFORM}/libpng/include)
include_directories(nest-libs/${PLATFORM}/opusfile/include)
include_directories(nest-libs/${PLATFORM}/SDL2/include/SDL2)
include_directories(nest-libs/${PLATFORM}/zlib/include)

add_executable(Scotty3D
    src/geometry/halfedge-global.cpp
    src/geometry/halfedge-local.cpp
    src/geometry/halfedge-utility.cpp
    src/geometry/halfedge.h
    src/geometry/indexed.cpp
    src/geometry/indexed.h
    src/geometry/meshedit.cpp
    src/geometry/spline.cpp
    src/geometry/spline.h
    src/geometry/util.cpp
    src/geometry/util.h
    src/gui/animate.cpp
    src/gui/animate.h
    src/gui/manager.cpp
    src/gui/manager.h
    src/gui/model.cpp
    src/gui/model.h
    src/gui/modifiers.h
    src/gui/render.cpp
    src/gui/render.h
    src/gui/rig.cpp
    src/gui/rig.h
    src/gui/simulate.cpp
    src/gui/simulate.h
    src/gui/widgets.cpp
    src/gui/widgets.h
    src/lib/bbox.h
    src/lib/line.h
    src/lib/log.h
    src/lib/mat4.h
    src/lib/mathlib.h
    src/lib/plane.h
    src/lib/quat.h
    src/lib/ray.h
    src/lib/spectrum.h
    src/lib/vec2.h
    src/lib/vec3.h
    src/lib/vec4.h
    src/pathtracer/aggregate.h
    src/pathtracer/aperture_shape.cpp
    src/pathtracer/aperture_shape.h
    src/pathtracer/bvh.cpp
    src/pathtracer/bvh.h
    src/pathtracer/instance.h
    src/pathtracer/list.h
    src/pathtracer/pathtracer.cpp
    src/pathtracer/pathtracer.h
    src/pathtracer/samplers.cpp
    src/pathtracer/samplers.h
    src/pathtracer/trace.h
    src/pathtracer/tri_mesh.cpp
    src/pathtracer/tri_mesh.h
    src/platform/font.dat
    src/platform/gl.cpp
    src/platform/gl.h
    src/platform/icon.res
    src/platform/platform.cpp
    src/platform/platform.h
    src/platform/renderer.cpp
    src/platform/renderer.h
    src/rasterizer/framebuffer.cpp
    src/rasterizer/framebuffer.h
    src/rasterizer/pipeline.cpp
    src/rasterizer/pipeline.h
    src/rasterizer/programs.h
    src/rasterizer/rasterizer.cpp
    src/rasterizer/rasterizer.h
    src/rasterizer/sample_pattern.cpp
    src/rasterizer/sample_pattern.h
    src/scene/animator.cpp
    src/scene/animator.h
    src/scene/camera.cpp
    src/scene/camera.h
    src/scene/delta_light.cpp
    src/scene/delta_light.h
    src/scene/env_light.cpp
    src/scene/env_light.h
    src/scene/instance.h
    src/scene/introspect.h
    src/scene/io.cpp
    src/scene/io.h
    src/scene/load-save-json.cpp
    src/scene/load-save.cpp
    src/scene/material.cpp
    src/scene/material.h
    src/scene/particles.cpp
    src/scene/particles.h
    src/scene/scene-step.cpp
    src/scene/scene.cpp
    src/scene/scene.h
    src/scene/shape.cpp
    src/scene/shape.h
    src/scene/skeleton.cpp
    src/scene/skeleton.h
    src/scene/texture.cpp
    src/scene/texture.h
    src/scene/transform.cpp
    src/scene/transform.h
    src/scene/undo.cpp
    src/scene/undo.h
    src/util/hdr_image.cpp
    src/util/hdr_image.h
    src/util/rand.cpp
    src/util/rand.h
    src/util/thread_pool.cpp
    src/util/thread_pool.h
    src/util/timer.cpp
    src/util/timer.h
    src/util/to_json.cpp
    src/util/to_json.h
    src/util/viewer.cpp
    src/util/viewer.h
    src/app.cpp
    src/app.h
    src/main.cpp)
