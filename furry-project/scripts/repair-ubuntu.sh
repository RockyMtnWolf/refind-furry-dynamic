#!/usr/bin/env bash
# Deploy the repaired build without activating it in BootOrder.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
[[ $EUID -eq 0 ]] || { echo '请使用 sudo bash furry-project/scripts/repair-ubuntu.sh'; exit 1; }
MODE="${1:-}"
[[ -z "$MODE" || "$MODE" == --safe || "$MODE" == --devices || "$MODE" == --full ]] || { echo '可选参数：--safe（关闭动态信息）、--devices（GPU/磁盘）或 --full（再开启 SMBIOS）'; exit 1; }
ESP=/boot/efi
mountpoint -q "$ESP"
[[ $(findmnt -nro FSTYPE --target "$ESP") == vfat ]]
BACKUP="/var/lib/refind-furry/backups/repair-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$BACKUP"
chmod 700 "$BACKUP"
efibootmgr -v > "$BACKUP/efibootmgr-before.txt"
cp -a "$ESP/EFI/refind-furry" "$BACKUP/"
echo "修复前备份：$BACKUP"
echo '正在完整重建修复版本（避免旧对象文件或旧二进制）…'
(cd "$ROOT"; make clean > "$BACKUP/clean.log" 2>&1; make gnuefi > "$BACKUP/build.log" 2>&1) || {
 tail -n 60 "$BACKUP/build.log" >&2
 echo '构建失败，未替换 EFI 文件。' >&2
 exit 1
}
echo '验证 GNU-EFI 入口参数传递…'
bash "$ROOT/furry-project/scripts/test-entry.sh" | tee "$BACKUP/entry-test.log"
bash "$ROOT/furry-project/scripts/install-ubuntu.sh" --no-deps --no-activate --esp "$ESP" --binary "$ROOT/refind/refind_x64.efi"
CONF="$ESP/EFI/refind-furry/refind.conf"
THEME="$ESP/EFI/refind-furry/themes/finn-term/theme.conf"
sed -i '1i# Physical-machine repair diagnostics\nlog_level 3' "$CONF"
sed -i 's/^timeout .*/timeout 5/' "$CONF"
# Auto scaling stays enabled even when --safe disables the overlay.
grep -q '^theme_auto_scale true$' "$THEME"
if [[ "$MODE" == --safe ]]; then
 sed -i 's/^dynamic_info .*/dynamic_info false/' "$THEME"
elif [[ "$MODE" == --devices || "$MODE" == --full ]]; then
 sed -i 's/^dynamic_device_probe .*/dynamic_device_probe true/' "$THEME"
fi
if [[ "$MODE" == --full ]]; then
 sed -i 's/^dynamic_smbios .*/dynamic_smbios true/' "$THEME"
fi
cp "$CONF" "$BACKUP/refind-after.conf"
cp "$THEME" "$BACKUP/theme-after.conf"
sync
echo '修复文件已部署，原 BootOrder 保持不变。请从固件启动菜单选择 rEFInd 测试。'
echo '如果仍卡住，返回 Ubuntu 后用 --safe 再运行本脚本，关闭全部动态信息。'
echo '日志位于 EFI/refind-furry/refind.log（或 EFI 分区根目录）。'
echo '已安装配置：'
grep -E '^(log_level|timeout|dont_scan_dirs|include|menuentry)' "$CONF"
grep -E '^(resolution|theme_auto_scale|dynamic_)' "$THEME"
sha256sum "$ESP/EFI/refind-furry/grubx64.efi" 2>/dev/null || sha256sum "$ESP/EFI/refind-furry/refind_x64.efi"
