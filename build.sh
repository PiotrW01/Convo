#!/bin/bash
arg="$1"

mkdir -p build
cmake -B build -S . -DCMAKE_C_COMPILER=/usr/bin/gcc-12 -DCMAKE_CXX_COMPILER=/usr/bin/g++-12


if [ "$arg" = "client" ]; then
    cmake --build build --target client
elif [ "$arg" = "server" ]; then
    cmake --build build --target server
else
    cmake --build build
fi