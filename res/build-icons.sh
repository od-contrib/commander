#!/usr/bin/env bash

set -euo pipefail

declare -r ICONS_DIR=/usr/share/icons/Humanity

set -x
rsvg-convert -h 28 "${ICONS_DIR}/places/32/folder.svg" > folder.png
rsvg-convert -h 28 "${ICONS_DIR}/mimes/32/image-x-generic.svg" > file-image.png
rsvg-convert -h 28 "${ICONS_DIR}/mimes/32/text-x-generic.svg" > file-text.png
rsvg-convert -h 28 "${ICONS_DIR}/actions/24/go-up.svg" > up.png
optipng -o9 *.png
