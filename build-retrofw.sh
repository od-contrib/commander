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
    -DCMAKE_TOOLCHAIN_FILE="$BUILDROOT/output/host/usr/share/buildroot/toolchainfile.cmake"
  cmake --build . -j $(getconf _NPROCESSORS_ONLN)
  cd -
}

package_opk() {
  cd build-retrofw
  cp -r ../ipkg ipkg
  mv commander.dge ipkg/home/retrofw/apps/commander/commander.dge
  cp -R ../res ipkg/home/retrofw/apps/commander/
  rm ipkg/home/retrofw/apps/commander/*.1.png
  cd ipkg

  tar -czvf control.tar.gz control
  tar -czvf data.tar.gz home
  ar rv commander.ipk control.tar.gz data.tar.gz debian-binary
  mv commander.ipk ..

  cd ..
  rm -rf ipkg
  cd ..
}

main() {
  check_buildroot
  set -x
  make_buildroot
  build
  package_opk
}

main