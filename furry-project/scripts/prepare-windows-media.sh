#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; LAB="$ROOT/vm-tests"; ISO=/mnt/refind-windows-iso
mkdir -p "$ISO"
mountpoint -q "$ISO" || mount -o loop,ro "${WINDOWS_ISO:-/mnt/c/Users/12991/Downloads/Win11_25H2_Chinese_Simplified_x64_v2.iso}" "$ISO"
mkdir -p "$LAB/windows-files/sources" "$LAB/windows-files/EFI/refind/themes/finn-term"
# Only a new image under this lab is formatted. Never use /dev/sd* or a host ESP.
# Random FAT sector updates on /mnt/c are very slow. Build on WSL ext4,
# then export in one sequential copy. ISO files are still read-only inputs.
WORK="${WINDOWS_WORK:-/var/tmp/refind-furry-windows-media}"
mkdir -p "$WORK"
IMAGE="$WORK/windows-media.img"
if [[ ! -f "$IMAGE" ]]; then
 qemu-img create -f raw "$IMAGE" 12G
 parted -s "$IMAGE" mklabel gpt mkpart ESP fat32 1MiB 100% set 1 esp on
 # Leave the GPT backup header outside the filesystem.
 mkfs.vfat -F 32 -n WIN11TEST --offset 2048 "$IMAGE" 12581871 >/dev/null
fi
if [[ ! -f "$LAB/windows-files/sources/split.complete" ]]; then
 wimlib-imagex split "$ISO/sources/install.wim" "$LAB/windows-files/sources/install.swm" 3800 > "$ROOT/evidence/phase2/wim-split.log" 2>&1
 touch "$LAB/windows-files/sources/split.complete"
fi
# The Microsoft EFI loader and WinPE remain intact; only install.wim is split for FAT32.
MTOOLS_SKIP_CHECK=1 mcopy -o -s -i "$IMAGE@@1048576" "$ISO/efi" "$ISO/boot" "$ISO/setup.exe" ::/
MTOOLS_SKIP_CHECK=1 mmd -i "$IMAGE@@1048576" ::/sources 2>/dev/null || true
find "$ISO/sources" -maxdepth 1 -type f ! -name install.wim -print0 | while IFS= read -r -d '' file; do
 MTOOLS_SKIP_CHECK=1 mcopy -o -i "$IMAGE@@1048576" "$file" ::/sources/
done
find "$ISO/sources" -mindepth 1 -maxdepth 1 -type d -print0 | while IFS= read -r -d '' folder; do
 MTOOLS_SKIP_CHECK=1 mcopy -o -s -i "$IMAGE@@1048576" "$folder" ::/sources/
done
MTOOLS_SKIP_CHECK=1 mcopy -o -i "$IMAGE@@1048576" "$LAB/windows-files/sources/"*.swm ::/sources/
cp "$IMAGE" "$LAB/windows-media.img"
echo 'Prepared Windows FAT32 EFI test media.'
