cmake_minimum_required(VERSION 3.10)
project(Orion)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(ORIONKIT_ROOT ${CMAKE_SOURCE_DIR}/../OrionKit)
set(ORIONKIT_BUILD ${ORIONKIT_ROOT}/.build/debug)
set(ORIONKIT_INCLUDE ${ORIONKIT_ROOT}/Sources/COrionKit/include)

if(APPLE)
    set(ORIONKIT_LIB "libOrionKit.dylib")
elseif(UNIX)
    set(ORIONKIT_LIB "libOrionKit.so")
endif()

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${ORIONKIT_INCLUDE})

link_directories(${GTK3_LIBRARY_DIRS})
link_directories(${ORIONKIT_BUILD})

add_executable(Orion src/main.cpp src/window.cpp)

target_link_libraries(Orion ${GTK3_LIBRARIES})
target_link_libraries(Orion ${ORIONKIT_BUILD}/${ORIONKIT_LIB})

add_compile_options(${GTK3_CFLAGS_OTHER})

add_custom_target(
    OrionKitBuild
    COMMAND cd ${ORIONKIT_ROOT} && swift build -c debug
    COMMENT "Building OrionKit..."
)

add_dependencies(Orion OrionKitBuild)
