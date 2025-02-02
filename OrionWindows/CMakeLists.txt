cmake_minimum_required(VERSION 3.20)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY VS_DISABLE_PACKAGE_RESTORE ON)

add_definitions(-DUNICODE -D_UNICODE)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/MainWindow.hpp
)

add_executable(OrionWindows WIN32 ${SOURCES} ${HEADERS})

set_property(TARGET OrionWindows PROPERTY VS_PACKAGE_REFERENCES "")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OrionWindows)

set_target_properties(OrionWindows PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
    VS_GLOBAL_DisableNuGetRestore "True"
    VS_GLOBAL_EnableNuGetPackageRestore "false"
)

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
