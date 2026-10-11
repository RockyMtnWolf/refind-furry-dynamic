#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK=$(mktemp -d); trap 'rm -rf -- "$WORK"' EXIT
FLAGS=(-DAllocatePool=TestAllocatePool -DFreePool=TestFreePool -DGNU_EFI_3_0_COMPAT -DGNU_EFI_USE_MS_ABI -DEFI_FUNCTION_WRAPPER -D__MAKEWITH_GNUEFI -DEFIX64 -fshort-wchar -ffunction-sections -fdata-sections -fsanitize=address,undefined -g -I/usr/include/efi -I/usr/include/efi/x86_64 -I"$ROOT/include" -I"$ROOT/refind" -I"$ROOT/libeg")
gcc "${FLAGS[@]}" "$ROOT/furry-project/scripts/test-ui.c" "$ROOT/libeg/image.c" -Wl,--gc-sections -lefi -lgnuefi -o "$WORK/test-ui"
ASAN_OPTIONS=detect_leaks=0 "$WORK/test-ui"
gcc "${FLAGS[@]}" "$ROOT/furry-project/scripts/test-gop.c" -Wl,--gc-sections -lefi -lgnuefi -o "$WORK/test-gop"
ASAN_OPTIONS=detect_leaks=0 "$WORK/test-gop"
gcc "${FLAGS[@]}" -DStrDuplicate=TestStrDuplicate "$ROOT/furry-project/scripts/test-menu.c" "$ROOT/libeg/image.c" "$ROOT/refind/mystrings.c" -Wl,--gc-sections -lefi -lgnuefi -o "$WORK/test-menu"
ASAN_OPTIONS=detect_leaks=0 "$WORK/test-menu"

gcc "${FLAGS[@]}" "$ROOT/furry-project/scripts/test-devices.c" -Wl,--gc-sections -lefi -lgnuefi -o "$WORK/test-devices"
(cd "$ROOT"; ASAN_OPTIONS=detect_leaks=0 "$WORK/test-devices")
