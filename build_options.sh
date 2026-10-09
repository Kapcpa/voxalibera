build_directory=build
build_type=Release
sanitizers=OFF

if [ "${1:-}" = "--debug" ]; then
	build_directory=build-debug
	build_type=Debug
	sanitizers=ON
	shift
fi

arguments=("$@")
application="$build_directory/custom_daw_artefacts/$build_type/custom-daw"
tests="$build_directory/tests/custom_daw_tests_artefacts/$build_type/custom_daw_tests"
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"

configure_if_needed() {
	if [ ! -f "$build_directory/CMakeCache.txt" ]; then
		cmake -B "$build_directory" -G Ninja -DCMAKE_BUILD_TYPE="$build_type" -DCUSTOM_DAW_SANITIZERS="$sanitizers"
	fi
}
