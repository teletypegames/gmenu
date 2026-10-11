#!/bin/sh
# Build GMENUPRG.EXE and GMEDIT.EXE into the repository root (SOURCE/build.sh), with the
# Open Watcom that scripts/ci-toolchain.sh installed. (The versions before 2.0.0 were
# written in NASM: they build with the scripts of their own tag.)
set -eu

say() { echo "[ci-build] $*"; }
die() { echo "[ci-build] error: $*" >&2; exit 1; }

rm -f GMENUPRG.EXE GMEDIT.EXE
WATCOM="${WATCOM:-/opt/watcom}" sh SOURCE/build.sh
for f in GMENUPRG.EXE GMEDIT.EXE; do
  [ -s "$f" ] || die "$f was not built"
  say "$f: $(wc -c < "$f" | tr -d ' ') bytes"
done
