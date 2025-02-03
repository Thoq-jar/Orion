# Orion Windows

A Windows version of the Orion file search application using WinUI 3.

## Prerequisites

1. Visual Studio 2022 with the following components:
   - C++ Desktop Development workload
   - Windows App SDK
   - Windows SDK 10.0.19041.0 or later
   - CMake build tools

2. Windows App SDK (latest version)
   - Download from: https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/downloads

## Building

1. Run `scripts/build.cmd` inside a Developer Command Prompt for VS 2022 window.
2. Run out/OrionWindows.exe or rename it to Orion.exe and run it.

## Features

- Fast file search with progress indication
- File extension filtering
- Asynchronous search with cancellation support
- Modern Windows UI using WinUI 3
- Double-click to open files in Explorer

## Usage

1. Enter your search query in the search box
2. (Optional) Enter a file extension to filter results (e.g., "txt" or ".txt")
3. Click "Browse" to select a directory to search in, or enter the path manually
4. Click "Search" to start searching
5. Use "Cancel" to stop an ongoing search
6. Double-click on results to open them in File Explorer

## Development

The project uses:
- C++17 features
- Windows App SDK for modern UI
- CMake for build configuration
- Asynchronous operations for responsive UI
- Standard Windows file dialogs for directory selection 