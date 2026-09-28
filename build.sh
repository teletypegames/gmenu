#!/bin/sh
# Builds GMENUPRG.COM from GMENU.ASM (requires NASM: brew install nasm)
set -e
cd "$(dirname "$0")"

if ! command -v nasm >/dev/null 2>&1; then
    echo "Error: nasm not found. Install it with: brew install nasm (macOS) / apt install nasm (Linux)" >&2
    exit 1
fi

nasm -f bin -o GMENUPRG.COM GMENU.ASM
echo "OK: GMENUPRG.COM ($(wc -c < GMENUPRG.COM | tr -d ' ') bytes)"
