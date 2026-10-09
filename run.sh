#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
exec ./build/custom_daw_artefacts/Release/custom-daw "$@"
