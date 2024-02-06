#!/usr/bin/env bash

set -e

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

BUILD_PATH="$SCRIPT_DIR/build"
REAL_BUILD_PATH="/tmp/build/esp32_sound_detector"

if ! command -v idf.py &> /dev/null; then
	>&2 echo "Error: 'idf.py' not in PATH"
	>&2 echo "Error: Please source the esp-idf export script"
	exit 1
fi

mkdir -p "$REAL_BUILD_PATH"
if [ ! -L "$BUILD_PATH" ]; then
	ln -s "$REAL_BUILD_PATH" "$BUILD_PATH"
fi

cd "$SCRIPT_DIR"
idf.py build

sed -i "build/compile_commands.json" \
	-e "s/-mlongcalls//g" \
	-e "s/-fno-tree-switch-conversion//g" \
	-e "s/-fstrict-volatile-bitfields//g"

if [ ! -L "compile_commands.json" ]; then
	ln -s "build/compile_commands.json"
fi
