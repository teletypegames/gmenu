#!/bin/sh
# Build GMENUPRG.COM (and GMEDIT.COM, since the versions that have it) into the
# repository root, whatever the layout of this version: since 0.2.1 the source is in
# SOURCE/ and SOURCE/build.sh writes ../GMENUPRG.COM, before that GMENU.ASM was in the root.
set -eu

say() { echo "[ci-build] $*"; }
die() { echo "[ci-build] error: $*" >&2; exit 1; }

rm -f GMENUPRG.COM GMEDIT.COM
if [ -f SOURCE/build.sh ]; then
  sh SOURCE/build.sh
elif [ -f GMENU.ASM ]; then
  nasm -f bin -o GMENUPRG.COM GMENU.ASM
else
  die "no source found (SOURCE/build.sh or GMENU.ASM)"
fi
[ -s GMENUPRG.COM ] || die "GMENUPRG.COM was not built"
say "GMENUPRG.COM: $(wc -c < GMENUPRG.COM | tr -d ' ') bytes"
if [ -f SOURCE/GMEDIT.ASM ]; then
  [ -s GMEDIT.COM ] || die "GMEDIT.COM was not built"
  say "GMEDIT.COM: $(wc -c < GMEDIT.COM | tr -d ' ') bytes"
fi
