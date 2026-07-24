#!/bin/bash

mkdir -p "cmake-build-release"
cmake -B "cmake-build-release" -S . -DCMAKE_BUILD_TYPE=Release -DEXT_BUILD_TESTS=OFF
cmake --build "cmake-build-release"
sudo cmake --install "cmake-build-release"
