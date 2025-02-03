cmake_minimum_required(VERSION 3.20)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/um"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/shared"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/winrt"
)

target_link_directories(OrionWindows PRIVATE 
    "$ENV{WindowsSdkDir}Lib/$ENV{WindowsTargetPlatformVersion}/um/x64"
)

target_link_libraries(OrionWindows PRIVATE
    comctl32.lib
    Shlwapi.lib
    Shell32.lib
    Ole32.lib
)
