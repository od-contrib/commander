#!/usr/bin/env bash

set -euo pipefail
source ./targets.sh

usage() {
  echo "Usage: build.sh [target]"
  usage_target
}

if ! check_target "$@"; then
  usage
  exit 64
fi

declare -r TARGET="${1}"

check_buildroot() {
  if ! [[ -d $BUILDROOT ]]; then
    echo "Please set the BUILDROOT environment variable"
    exit 1
  fi
}

make_buildroot() {
  cd "$BUILDROOT"
  # Check dependencies manually as it's much faster than `make`.
  local -a deps=()
  if ! [[ -f output/staging/usr/include/SDL/SDL.h ]]; then
    deps+=(sdl)
  fi
  if ! [[ -f output/staging/usr/include/SDL/SDL_image.h ]]; then
    deps+=(sdl_image)
  fi
  if ! [[ -d output/staging/usr/include/freetype2/ ]]; then
    deps+=(freetype)
  fi
  if ! [[ -f output/host/usr/share/buildroot/toolchainfile.cmake ]]; then
    deps+=(toolchain)
  fi
  if (( ${#deps[@]} )); then
    make "${deps[@]}" BR2_JLEVEL=0
  fi
  cd -
}

build() {
  mkdir -p "build-$TARGET"
  cd "build-$TARGET"
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DTARGET_PLATFORM="$TARGET" \
    -DCMAKE_TOOLCHAIN_FILE="$BUILDROOT/output/host/usr/share/buildroot/toolchainfile.cmake"
  cmake --build . -j $(getconf _NPROCESSORS_ONLN)
  cd -
}

package_opk() {
  ./package-opk.sh "$TARGET"
}

main() {
  check_buildroot
  set -x
  make_buildroot
  build
  package_opk
}

main
