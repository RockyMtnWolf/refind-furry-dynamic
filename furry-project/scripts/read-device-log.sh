#!/usr/bin/env bash
set -Eeuo pipefail
found=false
for f in /boot/efi/EFI/refind-furry/refind.log /boot/efi/refind.log; do
 if [[ -f "$f" ]]; then
  found=true
  echo "LOG: $f"
  iconv -f UTF-16LE -t UTF-8 "$f" | tr -d '\r' | sed -n '/Disk probe:/p'
 fi
done
$found || { echo '未找到 rEFInd 日志；请先重启进入测试版，再返回 Ubuntu。' >&2; exit 1; }
