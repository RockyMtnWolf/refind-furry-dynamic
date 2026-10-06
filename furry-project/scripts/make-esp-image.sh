#!/usr/bin/env bash
set -euo pipefail
SOURCE="${1:?ESP directory required}"; IMAGE="${2:?new image required}"
[[ -d "$SOURCE/EFI" ]] || exit 1
if [[ ! -f "$IMAGE" ]]; then
 qemu-img create -f raw "$IMAGE" 512M
 parted -s "$IMAGE" mklabel gpt mkpart ESP fat32 1MiB 511MiB set 1 esp on
 mkfs.vfat -F32 --offset 2048 "$IMAGE" 522240
fi
mcopy -o -s -i "$IMAGE@@1048576" "$SOURCE/EFI" ::/
[[ ! -d "$SOURCE/boot" ]] || mcopy -o -s -i "$IMAGE@@1048576" "$SOURCE/boot" ::/
[[ ! -d "$SOURCE/.disk" ]] || mcopy -o -s -i "$IMAGE@@1048576" "$SOURCE/.disk" ::/
