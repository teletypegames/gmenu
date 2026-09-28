#!/bin/sh
# Run the DOSBox screen tests (SOURCE/test/run.py) when this version has them;
# versions before 1.0.0 have none, and only their build is checked.
set -eu

if [ -f SOURCE/test/run.py ]; then
  python3 SOURCE/test/run.py
else
  echo "[ci-test] this version has no screen tests"
fi
