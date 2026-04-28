#!/usr/bin/env bash
# Build the BGP simulator to WebAssembly.
# Requires Emscripten (emcc) to be installed and activated.
# Install: https://emscripten.org/docs/getting_started/downloads.html
#
# Usage: bash build.sh
# Output: web/wasm/bgp_simulator.{js,wasm}  (commit these for Cloudflare Pages)
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$REPO/wasm/build"
OUT_DIR="$REPO/web/wasm"

if ! command -v emcc &>/dev/null; then
    echo "Error: emcc not found. Activate Emscripten first:" >&2
    echo "  source /path/to/emsdk/emsdk_env.sh" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR" "$OUT_DIR"

echo "Configuring..."
emcmake cmake -S "$REPO/wasm" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build "$BUILD_DIR" -- -j"$JOBS"

echo "Copying artifacts..."
cp "$BUILD_DIR/bgp_simulator.js"   "$OUT_DIR/"
cp "$BUILD_DIR/bgp_simulator.wasm" "$OUT_DIR/"

echo "Done. Commit web/wasm/ and push — Cloudflare Pages will auto-deploy."
