#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LAB="$ROOT/vm-tests"
MODE="${MODE:-on}"
ESP="$LAB/esp-$MODE"
[[ "$MODE" != windows ]] || ESP="$LAB/windows-refind"
exec 9>"$LAB/.lock"; flock -n 9 || { echo 'Another lab operation is running.' >&2; exit 1; }
WIDTH="${WIDTH:-3840}"; HEIGHT="${HEIGHT:-2160}"
RUNTIME="/tmp/refind-furry-lab-${UID}-${MODE}"; mkdir -p "$RUNTIME"
cp "$LAB/vars-$MODE.fd" "$RUNTIME/vars.fd"
trap 'cp "$RUNTIME/vars.fd" "$LAB/vars-$MODE.fd"; [[ ! -f "$RUNTIME/serial.log" ]] || cp "$RUNTIME/serial.log" "$ROOT/evidence/phase2/serial-$MODE.log"' EXIT
sed -i -E "s/^resolution .*/resolution $WIDTH $HEIGHT/" "$ESP/EFI/BOOT/themes/finn-term/theme.conf"
args=(-machine q35,smm=on -accel "${ACCEL:-kvm}" -cpu "${CPU:-host}" -m "${RAM:-4096}" -smp "${SMP:-1}"
 -global driver=cfi.pflash01,property=secure,value=on
 -device VGA,vgamem_mb=64,xres="$WIDTH",yres="$HEIGHT"
 -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.secboot.fd
 -drive if=pflash,format=raw,file="$RUNTIME/vars.fd"
 -display none -net none -vnc 127.0.0.1:10
 -qmp tcp:127.0.0.1:4450,server=on,wait=off
 -serial file:"$RUNTIME/serial.log" -name "refind-furry-lab-$MODE")
DISK="fat:rw:$ESP"; [[ "$MODE" != windows ]] || DISK="$LAB/windows-media.img"
[[ "$MODE" != ubuntu ]] || DISK="$LAB/ubuntu-esp.img"
if [[ "$MODE" == ubuntu || "$MODE" == windows ]]; then
 mcopy -o -i "$DISK@@1048576" "$ESP/EFI/BOOT/themes/finn-term/theme.conf" ::/EFI/BOOT/themes/finn-term/theme.conf
 # Sequential staging avoids thousands of slow /mnt/c sector reads.
 cp --sparse=always "$DISK" "$RUNTIME/disk.img"
 DISK="$RUNTIME/disk.img"
fi
args+=(-drive "if=none,id=esp,format=raw,file=$DISK")
if [[ "${DISK_BUS:-nvme}" == sata ]]; then
 args+=(-device ide-hd,drive=esp,bus=ide.0,bootindex=1)
else
 args+=(-device nvme,drive=esp,serial=FINNTERM-LAB-0001,bootindex=1)
fi
[[ -z "${SMBIOS_CPU:-}" ]] || args+=(-smbios "type=4,version=$SMBIOS_CPU")
[[ -z "${SMBIOS_FILE:-}" ]] || args+=(-smbios file="$SMBIOS_FILE")
if [[ -n "${ISO:-}" ]]; then
 MEDIA=/var/tmp/refind-furry-media; mkdir -p "$MEDIA"
 LOCAL_ISO="$MEDIA/$(basename "$ISO")"
 if [[ ! -f "$LOCAL_ISO" || "$ISO" -nt "$LOCAL_ISO" ]] ||
    [[ "$(stat -c %s "$LOCAL_ISO" 2>/dev/null || echo 0)" != "$(stat -c %s "$ISO")" ]]; then
  echo "Caching installation ISO in WSL: $LOCAL_ISO"
  cp "$ISO" "$LOCAL_ISO.partial"
  mv -f "$LOCAL_ISO.partial" "$LOCAL_ISO"
 fi
 args+=(-drive "file=$LOCAL_ISO,media=cdrom,readonly=on")
fi
qemu-system-x86_64 "${args[@]}"
