#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; LAB="$ROOT/vm-tests"
IMAGE="$LAB/windows-media.img"; STAGE="$LAB/windows-refind"
mkdir -p /mnt/refind-windows-iso
mountpoint -q /mnt/refind-windows-iso || mount -o loop,ro "${WINDOWS_ISO:-/mnt/c/Users/12991/Downloads/Win11_25H2_Chinese_Simplified_x64_v2.iso}" /mnt/refind-windows-iso
mkdir -p "$STAGE/EFI/BOOT/themes/finn-term"
cp -r "$ROOT/theme/." "$STAGE/EFI/BOOT/themes/finn-term/"
sbsign --key "$LAB/keys/test.key" --cert "$LAB/keys/test.crt" \
 --output "$STAGE/EFI/BOOT/BOOTX64.EFI" "$ROOT/test-esp/EFI/BOOT/BOOTX64.EFI"
cat > "$STAGE/EFI/BOOT/refind.conf" <<'CONF'
timeout 5
default_selection "Windows,Microsoft"
scanfor manual
showtools reboot shutdown about firmware
hideui hints,arrows,badges
use_nvram false
include themes/finn-term/theme.conf
menuentry "Windows 11 25H2 Setup - Microsoft EFI loader" {
 loader /EFI/Microsoft/Boot/bootmgfw.efi
 icon /EFI/BOOT/themes/finn-term/icons/os_win.png
}
CONF
sed -i 's/^resolution .*/resolution 1920 1080/' "$STAGE/EFI/BOOT/themes/finn-term/theme.conf"
mcopy -o -i "$IMAGE@@1048576" /mnt/refind-windows-iso/efi/boot/bootx64.efi ::/EFI/Microsoft/Boot/bootmgfw.efi
mcopy -o -s -i "$IMAGE@@1048576" "$STAGE/EFI/BOOT" ::/EFI/
cp "$LAB/vars-on.fd" "$LAB/vars-windows.fd"
echo 'Windows media now starts signed rEFInd, then Microsoft boot manager.'
