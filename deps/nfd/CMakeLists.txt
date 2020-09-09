cmake_minimum_required(VERSION 3.17)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(APPLE)

	project(nfd LANGUAGES C OBJC)

	set(SOURCES_NFD "nfd_common.c"
					"nfd_common.h"
					"nfd_cocoa.m")

	add_library(nfd STATIC ${SOURCES_NFD})

elseif(LINUX)

	project(nfd LANGUAGES C)

	set(SOURCES_NFD "nfd_common.c"
					"nfd_common.h"
					"nfd_gtk.c")
	
	find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    
	include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})

	add_library(nfd STATIC ${SOURCES_NFD})

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		set(CMAKE_C_FLAGS "-Wno-format-truncation ${CMAKE_C_FLAGS}")
	endif()

    target_link_directories(nfd PUBLIC ${GTK3_LIBRARY_DIRS})
    target_link_libraries(nfd ${GTK3_LIBRARIES})

else()
	project(nfd LANGUAGES C CXX)

	set(SOURCES_NFD "nfd_common.c"
					"nfd_common.h"	
					"nfd_win.cpp")

	add_library(nfd STATIC ${SOURCES_NFD})
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
endif()
