#!/bin/sh
# Build ZenTimer on macOS; no connected board required, no upload.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export PATH="$HOME/.local/bin:$PATH"
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli fehlt im PATH. Benoetigt: Arduino CLI und Seeeduino:nrf52 Core 1.1.13." >&2
  exit 1
fi
if ! command -v python >/dev/null 2>&1; then
  echo "python fehlt im PATH. Pruefe den vorhandenen Link ~/.local/bin/python auf Python 3." >&2
  exit 1
fi
mkdir -p "$project_dir/build/arduino"
echo "Baue ZenTimer; kein Flashen. Ausgabe: $project_dir/build/arduino"
exec arduino-cli compile \
  --fqbn Seeeduino:nrf52:xiaonRF52840Sense \
  --build-path "$project_dir/build/arduino" \
  "$project_dir"
