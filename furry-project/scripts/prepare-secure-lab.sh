#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LAB="$ROOT/vm-tests"
mkdir -p "$LAB/keys" "$ROOT/evidence/phase2"
exec 9>"$LAB/.lock"; flock -n 9 || { echo 'Quit the running lab before preparing it.' >&2; exit 1; }
if [[ ! -f "$LAB/keys/test.key" ]]; then
  openssl req -new -x509 -newkey rsa:2048 -nodes -sha256 -days 3650 \
    -subj '/CN=rEFInd isolated VM test key/' -keyout "$LAB/keys/test.key" -out "$LAB/keys/test.crt" > "$ROOT/evidence/phase2/keygen.log" 2>&1
fi
mkdir -p "$LAB/esp-on"; cp -r "$ROOT/test-esp/." "$LAB/esp-on/"
sbsign --key "$LAB/keys/test.key" --cert "$LAB/keys/test.crt" --output "$LAB/esp-on/EFI/BOOT/BOOTX64.EFI" "$ROOT/test-esp/EFI/BOOT/BOOTX64.EFI"
sbsign --key "$LAB/keys/test.key" --cert "$LAB/keys/test.crt" --output "$LAB/esp-on/EFI/demo/demo.efi" "$ROOT/test-esp/EFI/demo/demo.efi"
virt-fw-vars -i /usr/share/OVMF/OVMF_VARS_4M.fd -o "$LAB/vars-on.fd" --enroll-cert "$LAB/keys/test.crt" --add-db 49d3a619-7234-47bb-9647-19b420243030 "$LAB/keys/test.crt" --sb
virt-fw-vars -i "$LAB/vars-on.fd" -p > "$ROOT/evidence/phase2/enrolled-vars.txt"
sbverify --cert "$LAB/keys/test.crt" "$LAB/esp-on/EFI/BOOT/BOOTX64.EFI" > "$ROOT/evidence/phase2/signature.txt"
mkdir -p "$LAB/esp-off"; cp -r "$ROOT/test-esp/." "$LAB/esp-off/"
cp /usr/share/OVMF/OVMF_VARS_4M.fd "$LAB/vars-off.fd"
mkdir -p "$LAB/esp-reject"; cp -r "$LAB/esp-on/." "$LAB/esp-reject/"
cp "$ROOT/test-esp/EFI/demo/demo.efi" "$LAB/esp-reject/EFI/demo/demo.efi"
cp "$LAB/vars-on.fd" "$LAB/vars-reject.fd"
cp "$LAB/vars-on.fd" "$LAB/vars-seed-on.fd"
