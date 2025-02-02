cmake_minimum_required(VERSION 3.20)
project(Orion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Microsoft.WindowsAppSDK REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/MainWindow.hpp
)

add_executable(OrionWindows WIN32 ${SOURCES} ${HEADERS})

target_include_directories(OrionWindows PRIVATE include)

target_link_libraries(OrionWindows PRIVATE
    Microsoft::WindowsAppSDK
) 