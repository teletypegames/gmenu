#!/bin/sh
# Install what the pipeline needs into a Debian image: Open Watcom v2 (the compiler),
# DOSBox (Debian's DOSBox 0.74: SOURCE/test/run.py runs the screen tests in it) and
# Python. The Open Watcom download is a pinned release.
#
#   OW_RELEASE   Open Watcom release tag   (default: 2026-10-01-Build)
#
# Open Watcom goes to /opt/watcom (only the Linux binaries, the headers and the 16-bit
# libraries).
set -eu

OW_RELEASE="${OW_RELEASE:-2026-10-01-Build}"

say() { echo "[ci-toolchain] $*"; }

apt-get update
apt-get install -y --no-install-recommends ca-certificates curl xz-utils dosbox python3 >/dev/null

say "Open Watcom $OW_RELEASE"
mkdir -p /opt/watcom
curl -fsSL "https://github.com/open-watcom/open-watcom-v2/releases/download/$OW_RELEASE/ow-snapshot.tar.xz" |
  tar -xJ -C /opt/watcom ./binl64 ./h ./lib286
