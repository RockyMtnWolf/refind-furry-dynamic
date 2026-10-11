#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=$(mktemp -d); trap 'rm -rf -- "$WORK"' EXIT
# Link real production objects; only stop initialisation before any hardware scan.
gcc -DGNU_EFI_3_0_COMPAT -DGNU_EFI_USE_MS_ABI -D__MAKEWITH_GNUEFI -DEFIX64 -fshort-wchar \
 -I/usr/include/efi -I/usr/include/efi/x86_64 -I"$ROOT/refind" -I"$ROOT/include" \
 "$ROOT/furry-project/scripts/test-entry.c" "$ROOT/refind/"*.o \
 "$ROOT/libeg/libeg.a" "$ROOT/mok/libmok.a" "$ROOT/EfiLib/libEfiLib.a" "$ROOT/gzip/libgzip.a" \
 -Wl,--wrap=InitializeLib,--wrap=InitRefitLib -lefi -lgnuefi -o "$WORK/test-entry"
"$WORK/test-entry"
