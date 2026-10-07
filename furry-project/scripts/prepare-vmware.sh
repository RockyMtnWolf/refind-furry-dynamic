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
# VMware's firmware GOP exposes 1080p here, but not the requested 4K mode.
# Keep the 4K background and let the dynamic layout scale to the actual GOP.
CONFIG="$(mktemp)"; trap 'rm -f -- "$CONFIG"' EXIT
sed 's/^resolution .*/resolution 1920 1080/' "$ROOT/theme/theme.conf" > "$CONFIG"
mcopy -o -i "$LAB/esp.img@@1048576" "$CONFIG" ::/EFI/BOOT/themes/finn-term/theme.conf
qemu-img convert -f raw -O vmdk -o subformat=monolithicSparse "$LAB/esp.img" "$LAB/esp.vmdk"
cp "$ROOT/scripts/vmware-test.vmx" "$LAB/refind-test.vmx"
