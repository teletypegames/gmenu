#!/bin/sh
# Package the built menu for a release: dist/GMENU-<tag>.zip holds the GMENU folder as it
# is installed (GMENU.BAT, GMENUPRG.COM, GMEDIT.COM, README.md, LICENSE.md), and
# dist/GMENUPRG.COM (and dist/GMEDIT.COM) are the programs alone, for updating an
# installed menu. Versions before GMEDIT.COM have only GMENUPRG.COM.
#
#   scripts/ci-package.sh            the tag from Woodpecker (CI_COMMIT_TAG)
#   TAG=v1.2.3 scripts/ci-package.sh
#
# GMENUPRG.COM must already be built (scripts/ci-build.sh, the pipeline's test step).
set -eu

TAG="${TAG:-${CI_COMMIT_TAG:-}}"
DIST="${DIST:-dist}"

say() { echo "[ci-package] $*"; }
die() { echo "[ci-package] error: $*" >&2; exit 1; }

[ -n "$TAG" ] || die "cannot work out the tag — set TAG=v1.2.3"
[ -f GMENUPRG.COM ] || die "GMENUPRG.COM is not built — run scripts/ci-build.sh first"

rm -rf "$DIST"
mkdir -p "$DIST/GMENU"
# GMENU.BAT keeps its CRLF line ends: it is copied, not regenerated.
cp GMENU.BAT GMENUPRG.COM README.md LICENSE.md "$DIST/GMENU/"
[ -f GMEDIT.COM ] && cp GMEDIT.COM "$DIST/GMENU/" && cp GMEDIT.COM "$DIST/"
(cd "$DIST" && zip -q -r "GMENU-$TAG.zip" GMENU)
cp GMENUPRG.COM "$DIST/"
rm -rf "$DIST/GMENU"

say "built:"
ls -l "$DIST"
