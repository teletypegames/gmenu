#!/bin/sh
# Builds ../GMENUPRG.EXE and ../GMEDIT.EXE with Open Watcom v2
# (https://github.com/open-watcom/open-watcom-v2), as 16-bit DOS programs.
# WATCOM points to the Open Watcom folder (default: ~/.local/opt/watcom).
# ./build.sh test builds the test versions (the keys come from TESTKEYS.TXT, see
# test/run.py) into build/.
set -e
cd "$(dirname "$0")"

WATCOM=${WATCOM:-$HOME/.local/opt/watcom}
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) BIN=armo64 ;;
  Darwin-*) BIN=bino64 ;;
  Linux-aarch64) BIN=arml64 ;;
  *) BIN=binl64 ;;
esac
export WATCOM PATH="$WATCOM/$BIN:$PATH"
if ! command -v wpp >/dev/null 2>&1; then
  echo "Error: Open Watcom not found in $WATCOM (set WATCOM=<folder>)" >&2
  exit 1
fi

OBJ=build
OUT=..
DEFS=
if [ "$1" = test ]; then
  OUT=$OBJ
  DEFS=-dTEST
fi
rm -rf "$OBJ"
mkdir -p "$OBJ"

# gmenu.ini: collect_dir = default game directory (used when GMENU.BAT gives none)
INI_FILE="../gmenu.ini"
COLLECT_DIR='\COLLECT'
if [ -f "$INI_FILE" ]; then
  v=$(sed -n 's/^collect_dir[[:space:]]*=[[:space:]]*//p' "$INI_FILE" | tr -d '\r' | head -n 1)
  [ -n "$v" ] && COLLECT_DIR=$v
fi
case $COLLECT_DIR in
  *\"*) echo "Error: collect_dir must not contain a quote" >&2; exit 1 ;;
esac
COLLECT_C=$(printf '%s' "$COLLECT_DIR" | sed 's/\\/\\\\/g')

CFLAGS="-0 -ml -os -s -ei -we -zq -bt=dos -i=. -i=$WATCOM/h $DEFS"
MENU="GMENU.CPP catalog.cpp cache.cpp filter.cpp favorites.cpp stats.cpp session.cpp sort.cpp
      launcher.cpp ui.cpp browser.cpp common.cpp"
for f in $MENU GMEDIT.CPP; do
  wpp "$f" $CFLAGS "-dCOLLECT=\"$COLLECT_C\"" -fo="$OBJ/${f%.*}.obj"
done
objs() { for f in "$@"; do printf '%s,' "$OBJ/${f%.*}.obj"; done; }
LINK="system dos option quiet option eliminate option stack=4096"
wlink $LINK name "$OUT/GMENUPRG.EXE" file "$(objs $MENU | sed 's/,$//')"
wlink $LINK name "$OUT/GMEDIT.EXE" file "$(objs GMEDIT.CPP common.cpp | sed 's/,$//')"
rm -f "$OBJ"/*.obj
ls -l "$OUT/GMENUPRG.EXE" "$OUT/GMEDIT.EXE"
