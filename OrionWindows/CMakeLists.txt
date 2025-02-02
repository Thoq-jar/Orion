cmake_minimum_required(VERSION 3.20)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WINDOWSAPPSDK_PATH "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/include/winrt")
set(WINDOWSAPPSDK_LIB_PATH "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/lib/x64")

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/MainWindow.hpp
)

add_executable(OrionWindows WIN32 ${SOURCES} ${HEADERS})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OrionWindows)

set_target_properties(OrionWindows PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"
)

target_include_directories(OrionWindows PRIVATE 
    include
    "${WINDOWSAPPSDK_PATH}"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/um"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/winrt"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/shared"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/cppwinrt"
)

target_link_directories(OrionWindows PRIVATE 
    "${WINDOWSAPPSDK_LIB_PATH}"
    "$ENV{WindowsSdkDir}Lib/$ENV{WindowsTargetPlatformVersion}/um/x64"
)

target_link_libraries(OrionWindows PRIVATE
    WindowsApp.lib
    Shlwapi.lib
    Shell32.lib
)
