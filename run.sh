#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
source ./build_options.sh "$@"
exec "./$application" "${arguments[@]}"
