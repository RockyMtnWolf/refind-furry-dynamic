#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; LAB="$ROOT/vm-tests/vmware"
mkdir -p "$LAB"
if [[ ! -f "$LAB/esp.img" ]]; then
 qemu-img create -f raw "$LAB/esp.img" 512M
 parted -s "$LAB/esp.img" mklabel gpt mkpart ESP fat32 1MiB 511MiB set 1 esp on
 mkfs.vfat -F32 --offset 2048 "$LAB/esp.img" 522240
fi
mcopy -o -s -i "$LAB/esp.img@@1048576" "$ROOT/test-esp/EFI" ::/
qemu-img convert -f raw -O vmdk -o subformat=monolithicSparse "$LAB/esp.img" "$LAB/esp.vmdk"
