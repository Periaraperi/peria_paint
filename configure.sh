#!/bin/bash

if [ ! -d build ]; then
    echo creating build directory
    mkdir -p ./build/{debug,release}
fi

echo configuring Debug build...
cmake -DCMAKE_BUILD_TYPE=Debug -B build/debug/ -S .

echo configuring Release build...
cmake -DCMAKE_BUILD_TYPE=Release -B build/release -S .
