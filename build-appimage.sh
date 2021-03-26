#!/usr/bin/env bash

set -euo pipefail

BUILD_DIR=build-appimage-x86_64
LINUXDEPLOY_DIR=linuxdeploy
LINUXDEPLOY_BIN=linuxdeploy-x86_64.AppImage
APP_DIR="${LINUXDEPLOY_DIR}/AppDir"

maybe_dl_linuxdeploy() {
  if [[ -f "${LINUXDEPLOY_DIR}/${LINUXDEPLOY_BIN}" ]]; then
    return
  fi
  mkdir -p "$LINUXDEPLOY_DIR"
  cd "$LINUXDEPLOY_DIR"
  wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
  chmod +x "$LINUXDEPLOY_BIN"
  cd -
}

set -x

cmake -S. -B"$BUILD_DIR" \
  -DCMAKE_INSTALL_PREFIX="${APP_DIR}/usr" \
  -DRES_DIR='"usr/share/commander/res/"' \
  -DCMAKE_BUILD_TYPE=Release -DPPU_X=2 -DPPU_Y=2 "$@"
cmake --build "$BUILD_DIR" -j $(getconf _NPROCESSORS_ONLN)

rm -rf "$APP_DIR"
cmake --install "$BUILD_DIR"

maybe_dl_linuxdeploy

"${LINUXDEPLOY_DIR}/${LINUXDEPLOY_BIN}" --appimage-extract-and-run \
  --appdir="$APP_DIR" \
  --custom-apprun=packaging/appimage/AppRun \
  --desktop-file=packaging/commander.desktop \
  --icon-file="${APP_DIR}/usr/share/icons/hicolor/32x32/apps/commander.png" \
  --output=appimage
rm -f release/commander_x86_64.appimage # Remove to clear cached mount if any
mv Commander-*.AppImage release/commander_x86_64.appimage
