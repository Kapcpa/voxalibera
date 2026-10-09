#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if [ ! -f build/CMakeCache.txt ]; then
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
fi

cmake --build build
exec ./run.sh "$@"
