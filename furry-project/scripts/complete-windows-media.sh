#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ISO=/mnt/refind-windows-iso
mkdir -p "$ISO"
mountpoint -q "$ISO" || mount -o loop,ro "${WINDOWS_ISO:-/mnt/c/Users/12991/Downloads/Win11_25H2_Chinese_Simplified_x64_v2.iso}" "$ISO"
find "$ISO/sources" -mindepth 1 -maxdepth 1 -type d -print0 | while IFS= read -r -d '' folder; do
 mcopy -o -s -i "$ROOT/vm-tests/windows-media.img@@1048576" "$folder" ::/sources/
done
echo 'Windows sources subdirectories copied.'
