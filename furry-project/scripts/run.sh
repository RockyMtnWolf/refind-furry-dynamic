#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RAM="${RAM:-2048}"
PORT="${QMP_PORT:-4444}"
mkdir -p "$ROOT/firmware" "$ROOT/evidence"
CODE=/usr/share/OVMF/OVMF_CODE_4M.secboot.fd
VARS=/usr/share/OVMF/OVMF_VARS_4M.fd
[[ -f "$ROOT/firmware/OVMF_CODE_4M.secboot.fd" ]] || cp "$CODE" "$ROOT/firmware/OVMF_CODE_4M.secboot.fd"
[[ -f "$ROOT/firmware/OVMF_VARS_4M.fd" ]] || cp "$VARS" "$ROOT/firmware/OVMF_VARS_4M.fd"
exec qemu-system-x86_64 -machine q35,smm=on -accel tcg -cpu "${CPU:-max}" -m "$RAM" \
  -global driver=cfi.pflash01,property=secure,value=on -device VGA,vgamem_mb=64,xres=3840,yres=2160 \
  -drive if=pflash,format=raw,readonly=on,file="$ROOT/firmware/OVMF_CODE_4M.secboot.fd" \
  -drive if=pflash,format=raw,file="$ROOT/firmware/OVMF_VARS_4M.fd" \
  -drive format=raw,file="fat:rw:$ROOT/test-esp" -net none \
  -display "${DISPLAY_BACKEND:-none}" -vnc 127.0.0.1:9 \
  -qmp tcp:127.0.0.1:"$PORT",server=on,wait=off \
  -serial file:"$ROOT/evidence/serial.log" -name 'rEFInd furry dynamic - test ESP'
