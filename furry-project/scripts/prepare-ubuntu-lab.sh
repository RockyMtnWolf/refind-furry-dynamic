#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; LAB="$ROOT/vm-tests"
ISO="${UBUNTU_ISO:-/mnt/c/Users/12991/Downloads/ubuntu-26.04.1-desktop-amd64.iso}"
MOUNT=/mnt/refind-ubuntu-iso
mkdir -p "$MOUNT"
mountpoint -q "$MOUNT" || mount -o loop,ro "$ISO" "$MOUNT"
mkdir -p "$LAB/esp-ubuntu/EFI/ubuntu"
cp -r "$LAB/esp-on/." "$LAB/esp-ubuntu/"
sbsign --key "$LAB/keys/test.key" --cert "$LAB/keys/test.crt" --output "$LAB/esp-ubuntu/EFI/BOOT/BOOTX64.EFI" "$ROOT/test-esp/EFI/BOOT/BOOTX64.EFI"
cp --remove-destination "$MOUNT/EFI/boot/bootx64.efi" "$LAB/esp-ubuntu/EFI/ubuntu/shimx64.efi"
cp --remove-destination "$MOUNT/EFI/boot/grubx64.efi" "$MOUNT/EFI/boot/mmx64.efi" "$LAB/esp-ubuntu/EFI/ubuntu/"
cat > "$LAB/esp-ubuntu/EFI/ubuntu/grub.cfg" <<'GRUB'
search --no-floppy --file --set=root /casper/vmlinuz
set timeout=0
menuentry 'Ubuntu live ISO' {
 linux /casper/vmlinuz boot=casper console=tty0 console=ttyS0,115200 ---
 initrd /casper/initrd
}
GRUB
cat > "$LAB/esp-ubuntu/EFI/BOOT/refind.conf" <<'CONF'
timeout 0
scanfor manual
scan_delay 1
showtools reboot shutdown about firmware
hideui hints,arrows,badges
use_nvram false
include themes/finn-term/theme.conf
menuentry "Ubuntu 26.04.1 live ISO - signed shim" {
 loader /EFI/ubuntu/shimx64.efi
 icon /EFI/BOOT/themes/finn-term/icons/os_ubuntu.png
}
CONF
mkdir -p "$LAB/esp-ubuntu/boot/grub" "$LAB/esp-ubuntu/.disk"
cp --remove-destination "$MOUNT/.disk/info" "$LAB/esp-ubuntu/.disk/info"
cp "$LAB/esp-ubuntu/EFI/ubuntu/grub.cfg" "$LAB/esp-ubuntu/boot/grub/grub.cfg"
cp "$LAB/vars-on.fd" "$LAB/vars-ubuntu.fd"
bash "$ROOT/scripts/make-esp-image.sh" "$LAB/esp-ubuntu" "$LAB/ubuntu-esp.img"
echo 'Ubuntu shim, GRUB and kernel remain vendor signed.'
