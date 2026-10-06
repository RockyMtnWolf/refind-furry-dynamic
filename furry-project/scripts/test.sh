#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$ROOT/refind"; [[ -d "$SOURCE/refind" ]] || SOURCE="$ROOT/.."
mkdir -p "$ROOT/evidence/phase2"
gcc -DEFI_FUNCTION_WRAPPER -D__MAKEWITH_GNUEFI -DEFIX64 -fshort-wchar \
 -ffunction-sections -fdata-sections -fsanitize=address,undefined -g \
 -I/usr/include/efi -I/usr/include/efi/x86_64 -I"$SOURCE/include" \
 -I"$SOURCE/refind" -I"$SOURCE/libeg" "$ROOT/scripts/test-smbios.c" \
 -Wl,--gc-sections -lefi -lgnuefi -o /tmp/refind-test-smbios
/tmp/refind-test-smbios | tee "$ROOT/evidence/phase2/parser-tests.txt"
