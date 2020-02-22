#!/usr/bin/env bash

set -euo pipefail

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
    deps+=(SDL)
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
  mkdir -p build-retrofw
  cd build-retrofw
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DRETROFW=1 \
    -DPPU_Y=2 \
    -DRES_DIR='' \
    -DCMAKE_TOOLCHAIN_FILE="$BUILDROOT/output/host/usr/share/buildroot/toolchainfile.cmake"
  cmake --build . -j $(getconf _NPROCESSORS_ONLN)
  cd -
}

package_opk() {
  cd build-retrofw
  mksquashfs \
    ../opkg/default.retrofw.desktop \
    ../opkg/readme.retrofw.txt \
    ../opkg/commander.png \
    ../res/*.png \
    ../res/wy_scorpio.ttf \
    commander \
    commander.opk \
    -all-root -no-xattrs -noappend -no-exports
  cd -
}

main() {
  check_buildroot
  set -x
  make_buildroot
  build
  package_opk
}

main
