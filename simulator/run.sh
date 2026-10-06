#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sh "$project_dir/simulator/build.sh"
exec "$project_dir/build/simulator/ZenTimerSimulator" "$@"
