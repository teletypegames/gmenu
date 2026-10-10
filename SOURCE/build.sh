#!/bin/sh
# Builds GMENUPRG.COM from GMENU.ASM and GMEDIT.COM from GMEDIT.ASM (requires NASM:
# brew install nasm). Extra arguments are passed to nasm (e.g. -DTEST "-DTESTKEYS=...").
# OUT=<file> overrides the menu's output file (default: ../GMENUPRG.COM), EDOUT=<file>
# the editor's (default: GMEDIT.COM next to the menu). ONLY=menu / ONLY=editor builds
# just one of them (the tests use it).
set -e
cd "$(dirname "$0")"
OUT=${OUT:-../GMENUPRG.COM}
EDOUT=${EDOUT:-$(dirname "$OUT")/GMEDIT.COM}

# gmenu.ini: collect_dir = default game directory (used when GMENU.BAT gives none)
INI_FILE="../gmenu.ini"
COLLECT_DIR='\COLLECT'
if [ -f "$INI_FILE" ]; then
    v=$(sed -n 's/^collect_dir[[:space:]]*=[[:space:]]*//p' "$INI_FILE" | tr -d '\r' | head -n 1)
    [ -n "$v" ] && COLLECT_DIR=$v
fi
case $COLLECT_DIR in
    *\'*) echo "Error: collect_dir must not contain a quote" >&2; exit 1 ;;
esac

if ! command -v nasm >/dev/null 2>&1; then
    echo "Error: nasm not found. Install it with: brew install nasm (macOS) / apt install nasm (Linux)" >&2
    exit 1
fi

if [ "$ONLY" != editor ]; then
MAP=$(mktemp)
trap 'rm -f "$MAP"' EXIT
nasm -f bin "-DCOLLECT='$COLLECT_DIR'" "-DMAPFILE=$MAP" "$@" -o "$OUT" GMENU.ASM

# code, data and BSS share the 64 KB segment (the stack has its own segment)
BSS_END=$(awk '$3 == "bss_end" { print $1 }' "$MAP")
if [ -z "$BSS_END" ]; then
    echo "Error: bss_end not found in the map file" >&2
    exit 1
fi
if [ $((0x$BSS_END)) -gt $((0xFFF0)) ]; then
    echo "Error: the program and its data end at $BSS_END, beyond the 64 KB segment (limit FFF0)" >&2
    rm -f "$OUT"
    exit 1
fi
echo "OK: $OUT ($(wc -c < "$OUT" | tr -d ' ') bytes, data end: $BSS_END)"
fi

if [ "$ONLY" != menu ]; then
nasm -f bin "$@" -o "$EDOUT" GMEDIT.ASM
echo "OK: $EDOUT ($(wc -c < "$EDOUT" | tr -d ' ') bytes)"
fi
