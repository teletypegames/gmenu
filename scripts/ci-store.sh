#!/bin/sh
# Publish the built menu to the DOSINST program store (dos/dosinst,
# ftp://ftp.teletypegames.org): the GMENU folder as it is installed, plus an INFO.INI
# with the version of the tag, so DOSINST can install and update it.
#
#   scripts/ci-store.sh              the tag from Woodpecker (CI_COMMIT_TAG)
#   TAG=v1.2.3 STORE=/tmp/store scripts/ci-store.sh
#
# STORE is the store's programs folder; in the pipeline /srv/stacks/dosinst/programs of
# the server is mounted there. The new folder is put together next to the old one and
# swapped in with two renames, so a download never sees it half written. The files get
# STORE_OWNER (the server's tasi user), so the store can be managed without sudo.
#
# GMENUPRG.COM must already be built (scripts/ci-build.sh, the pipeline's test step).
set -eu

TAG="${TAG:-${CI_COMMIT_TAG:-}}"
STORE="${STORE:-/store}"
STORE_OWNER="${STORE_OWNER:-1000:1000}"
NAME=GMENU

say() { echo "[ci-store] $*"; }
die() { echo "[ci-store] error: $*" >&2; exit 1; }

[ -n "$TAG" ] || die "cannot work out the tag — set TAG=v1.2.3"
[ -f GMENUPRG.COM ] || die "GMENUPRG.COM is not built — run scripts/ci-build.sh first"
[ -d "$STORE" ] || die "no store folder at $STORE"

VERSION="${TAG#v}"
NEW="$STORE/.$NAME.new"
OLD="$STORE/.$NAME.old"

rm -rf "$NEW" "$OLD"
mkdir "$NEW"
# GMENU.BAT keeps its CRLF line ends: it is copied, not regenerated.
cp GMENU.BAT GMENUPRG.COM README.md LICENSE.md "$NEW/"
[ -f GMEDIT.COM ] && cp GMEDIT.COM "$NEW/"
printf '[program]\r\nname=GMENU\r\ndescription=Game menu for DOS: lists the game folders with their title, genre and year, searches and filters them, and starts the game you pick. Edit GMENU.BAT to set the game directory.\r\nversion=%s\r\n' \
  "$VERSION" > "$NEW/INFO.INI"
chown -R "$STORE_OWNER" "$NEW" 2>/dev/null || say "could not chown to $STORE_OWNER, keeping the owner"
chmod -R a+rX "$NEW"

if [ -d "$STORE/$NAME" ]; then
  mv "$STORE/$NAME" "$OLD"
fi
mv "$NEW" "$STORE/$NAME"
rm -rf "$OLD"

say "published $NAME $VERSION:"
ls -l "$STORE/$NAME"
