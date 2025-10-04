#!/bin/bash

build_type=$1

if [ "$build_type" = "--debug" ]; then
    cmake --build build/debug/ --config Debug
elif [ "$build_type" = "--release" ]; then
    cmake --build build/release/ --config Release
else
    echo must specify build type
fi

