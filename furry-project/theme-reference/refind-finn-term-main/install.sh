#!/usr/bin/env bash
# Installs finn-term into an existing rEFInd installation.
# Usage: sudo ./install.sh [path-to-refind-dir]
set -euo pipefail

REFIND="${1:-}"
if [[ -z "$REFIND" ]]; then
    for c in /boot/EFI/refind /boot/efi/EFI/refind /efi/EFI/refind; do
        if [[ -f "$c/refind.conf" ]]; then REFIND="$c"; break; fi
    done
fi
if [[ -z "$REFIND" || ! -f "$REFIND/refind.conf" ]]; then
    echo "could not find rEFInd, pass its directory: sudo ./install.sh /boot/EFI/refind" >&2
    exit 1
fi

SRC="$(cd "$(dirname "$0")" && pwd)"
DST="$REFIND/themes/finn-term"

mkdir -p "$DST"
cp -r "$SRC/icons" "$SRC/fonts" "$DST/"
cp "$SRC/background.png" "$SRC/selection_big.png" "$SRC/selection_small.png" \
   "$SRC/theme.conf" "$SRC/LICENSE-fallout-icons" "$DST/"

if grep -q "themes/finn-term/theme.conf" "$REFIND/refind.conf"; then
    echo "refind.conf already includes the theme"
else
    printf '\ninclude themes/finn-term/theme.conf\n' >> "$REFIND/refind.conf"
    echo "added 'include themes/finn-term/theme.conf' to $REFIND/refind.conf"
fi

echo "installed to $DST"
echo "note: theme.conf sets 'resolution 2560 1440', adjust it (and regenerate"
echo "the background, see README) if your display is different."
