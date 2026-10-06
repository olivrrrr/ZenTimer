#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$project_dir/build/simulator"
xcrun clang++ -std=c++17 -Wall -Wextra -Wpedantic -fobjc-arc \
  "$project_dir/simulator/main.mm" -framework Cocoa \
  -o "$project_dir/build/simulator/ZenTimerSimulator"
