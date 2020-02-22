#!/usr/bin/env bash

set -xeuo pipefail

mkdir -p build
cd build
cmake .. -DPPU_X=2 -DPPU_Y=2 "$@"
cmake --build . -j $(getconf _NPROCESSORS_ONLN)
cd -
ln -sf build/commander commander
