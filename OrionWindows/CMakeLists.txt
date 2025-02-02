cmake_minimum_required(VERSION 3.20)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WindowsAppSDK PATHS "C:/Program Files (x86)/Microsoft SDKs/Windows Kits/10/ExtensionSDKs/Microsoft.WindowsAppRuntime.1.5")

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

# Add Windows App SDK package reference for Visual Studio
set_property(TARGET OrionWindows PROPERTY
    VS_PACKAGE_REFERENCES "Microsoft.WindowsAppSDK_1.5.240227000"
)

target_include_directories(OrionWindows PRIVATE 
    include
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/um"
    "$ENV{WindowsSdkDir}Include/$ENV{WindowsTargetPlatformVersion}/winrt"
)

target_link_directories(OrionWindows PRIVATE 
    "$ENV{WindowsSdkDir}Lib/$ENV{WindowsTargetPlatformVersion}/um/${CMAKE_SYSTEM_PROCESSOR}"
)

target_link_libraries(OrionWindows PRIVATE
    WindowsApp.lib
)
