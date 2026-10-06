#!/usr/bin/env bash
# Re-colors the icons from awanwar/rEFInd-fallout (green -> purple/pink).
# The repo already ships recolored icons, you only need this for a different hue.
# Usage: ./recolor-icons.sh <path-to-fallout-icons-dir>
set -euo pipefail

SRC="${1:?usage: $0 <path-to-fallout-icons-dir>}"
DST="$(cd "$(dirname "$0")/.." && pwd)/icons"

mkdir -p "$DST"
for f in "$SRC"/*.png; do
    magick "$f" -modulate 150,75,181 "$DST/$(basename "$f")"
done
echo "recolored $(ls "$SRC"/*.png | wc -l) icons -> $DST"
