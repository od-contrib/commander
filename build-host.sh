#!/usr/bin/env bash

set -xeuo pipefail

mkdir -p build
cd build
cmake ..
cmake --build . -j $(getconf _NPROCESSORS_ONLN)
cd -
ln -sf build/commander commander
