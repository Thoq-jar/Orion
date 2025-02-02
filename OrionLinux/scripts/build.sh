#!/bin/bash

set -e

echo "Building OrionKit..."
cd ../OrionKit
swift build -c debug

echo "Building Linux app..."
cd ../OrionLinux
mkdir -p build
cd build
cmake ..
make
