cmake_minimum_required(VERSION 3.20)
project(OrionWindows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WINDOWS_APP_SDK_VERSION "1.4.0")
set(WINDOWS_APP_SDK_RELEASE "-preview1")
set(WINDOWS_APP_SDK_RUNTIME_VERSION "4000.1000.1000.0")

include(FetchContent)
FetchContent_Declare(
    WindowsAppSDK
    URL "https://www.nuget.org/api/v2/package/Microsoft.WindowsAppSDK/${WINDOWS_APP_SDK_VERSION}${WINDOWS_APP_SDK_RELEASE}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(WindowsAppSDK)

set(WINDOWS_APP_SDK_INCLUDE_DIR "${windowsappsdk_SOURCE_DIR}/include")
set(WINDOWS_APP_SDK_LIB_DIR "${windowsappsdk_SOURCE_DIR}/lib/win10-${CMAKE_SYSTEM_PROCESSOR}")

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
    ${WINDOWS_APP_SDK_INCLUDE_DIR}
)

target_link_directories(OrionWindows PRIVATE ${WINDOWS_APP_SDK_LIB_DIR})
target_link_libraries(OrionWindows PRIVATE
    Microsoft.WindowsAppRuntime.lib
    WindowsApp.lib
)

add_custom_command(TARGET OrionWindows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WINDOWS_APP_SDK_LIB_DIR}/Microsoft.WindowsAppRuntime.Bootstrap.dll"
        "${WINDOWS_APP_SDK_LIB_DIR}/Microsoft.WindowsAppRuntime.dll"
        $<TARGET_FILE_DIR:OrionWindows>
)
