#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
source ./build_options.sh "$@"
configure_if_needed
cmake --build "$build_directory" --target custom_daw
exec "./$application" "${arguments[@]}"
