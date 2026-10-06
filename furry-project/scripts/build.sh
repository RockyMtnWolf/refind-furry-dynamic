#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT/evidence" "$ROOT/test-esp/EFI/BOOT" "$ROOT/test-esp/EFI/demo"
SOURCE="$ROOT/refind"
[[ -d "$SOURCE/refind" ]] || SOURCE="$ROOT/.."
cd "$SOURCE"
make clean > "$ROOT/evidence/clean.log" 2>&1
make gnuefi > "$ROOT/evidence/build.log" 2>&1
cp refind/refind_x64.efi "$ROOT/test-esp/EFI/BOOT/BOOTX64.EFI"
mkdir -p "$ROOT/test-esp/EFI/BOOT/themes/finn-term"
cp -r "$ROOT/theme/"* "$ROOT/test-esp/EFI/BOOT/themes/finn-term/"
cp "$ROOT/refind.conf" "$ROOT/test-esp/EFI/BOOT/refind.conf"
gcc -I/usr/include/efi -I/usr/include/efi/x86_64 -fpic -fshort-wchar -mno-red-zone -fno-stack-protector -DEFI_FUNCTION_WRAPPER -c "$ROOT/scripts/demo.c" -o "$ROOT/evidence/demo.o"
ld -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic /usr/lib/crt0-efi-x86_64.o "$ROOT/evidence/demo.o" -L/usr/lib -lefi -lgnuefi -o "$ROOT/evidence/demo.so"
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 "$ROOT/evidence/demo.so" "$ROOT/test-esp/EFI/demo/demo.efi"
sha256sum "$ROOT/test-esp/EFI/BOOT/BOOTX64.EFI" > "$ROOT/evidence/binary.sha256"
echo 'Built EFI binary and test ESP.'
