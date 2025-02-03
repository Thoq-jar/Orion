cmake_minimum_required(VERSION 3.15)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /EHsc)
endif()

add_definitions(-DUNICODE -D_UNICODE)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/MainWindow.hpp
)

add_executable(OrionWindows WIN32 ${SOURCES} ${HEADERS})

target_include_directories(OrionWindows PRIVATE
    include
    "${CMAKE_SOURCE_DIR}/../OrionKit/Sources/OrionKit/include"
    "$ENV{WindowsSdkDir}Include/um"
    "$ENV{WindowsSdkDir}Include/shared"
    "$ENV{WindowsSdkDir}Include/winrt"
)

target_link_directories(OrionWindows PRIVATE 
    "$ENV{WindowsSdkDir}Lib/$ENV{WindowsTargetPlatformVersion}/um/x64"
)

target_link_libraries(OrionWindows PRIVATE
    "${CMAKE_SOURCE_DIR}/../OrionKit/build/OrionKit.lib"
    dwmapi
    uxtheme
    comctl32.lib
    Shlwapi.lib
    Shell32.lib
    Ole32.lib
)
