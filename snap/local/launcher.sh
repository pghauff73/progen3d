#!/bin/sh
set -eu

cd "$SNAP"
exec "$SNAP/progen3d" "$@"
