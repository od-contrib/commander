#!/usr/bin/env bash

set -euo pipefail
source ./targets.sh

usage() {
  echo "Usage: package-opk.sh <target> [build dir] [output OPK path]"
  usage_target
}

if ! check_target "$@"; then
  usage
  exit 64
fi

declare -r TARGET="${1}"
declare -r BUILD_DIR="${2:-"build-${TARGET}"}"
declare -r OUT="${3:-"$BUILD_DIR/commander-${TARGET}.opk"}"

main() {
  local ext="$TARGET"
  if [[ $TARGET == rg350 ]]; then
    ext=gcw0
  elif [[ $TARGET == lepus ]]; then
    ext=lepus
  fi
  local -a files=(
    "opkg/default.$ext.desktop"
    "opkg/readme.$ext.txt"
    opkg/commander.png
    res/*.png
    res/*.ttf
    "$BUILD_DIR/commander"
  )
  if [[ $TARGET == rs90 ]]; then
    # rg99 uses the same binary as the rs90 but a different config
    files+=(
      opkg/default.rg99.desktop
      opkg/commander.rg99.cfg
      opkg/readme.retrofw.txt
    )
  fi

  set -x
  mksquashfs "${files[@]}" "$OUT" \
    -all-root -no-xattrs -noappend -no-exports
}

main
