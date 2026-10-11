#!/usr/bin/env bash
# Install the custom build, never the distribution's stock rEFInd binary.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$ROOT/refind"; [[ -f "$SOURCE/refind/dynamic_info.c" ]] || SOURCE="$ROOT/.."
ESP=; STAGE=; BINARY=; DEPS=1; ACTIVATE=1
LABEL='rEFInd Furry Dynamic'; FOLDER=refind-furry
STATE=/var/lib/refind-furry
die() { echo "错误：$*" >&2; exit 1; }
usage() {
 cat <<'HELP'
Ubuntu x86_64 UEFI 一键安装：sudo bash scripts/install-ubuntu.sh
  --esp /boot/efi     指定已挂载的 FAT32 EFI System Partition
  --no-activate       安装并注册启动项，保留原 BootOrder
  --stage DIRECTORY  仅生成 EFI 安装包，不读写固件或系统 ESP
  --binary FILE      使用已有的自定义 EFI 二进制，跳过源码构建
  --no-deps          跳过 apt 依赖安装
  --help             显示帮助
Secure Boot 开启时：首次申请 MOK 注册，重启确认后再次运行完成安装。
HELP
}
while (($#)); do
 case "$1" in
  --esp|--stage|--binary) (($#>=2)) || die "$1 缺少参数";
   case "$1" in --esp) ESP="$2";; --stage) STAGE="$2";; --binary) BINARY="$2";; esac; shift 2;;
  --no-deps) DEPS=0; shift;; --no-activate) ACTIVATE=0; shift;;
  --help|-h) usage; exit 0;; *) die "未知参数 $1";;
 esac
done
[[ "$(uname -m)" == x86_64 ]] || die '当前仅支持 x86_64 Ubuntu。'
[[ -f /etc/os-release ]] || die '未找到操作系统信息。'
# shellcheck disable=SC1091
. /etc/os-release
[[ "${ID:-}" == ubuntu ]] || die '此安装脚本针对 Ubuntu。'
[[ -d "$ROOT/theme" && -f "$ROOT/theme/background.png" ]] || die '主题资源不完整。'
[[ -n "$STAGE" || $EUID -eq 0 ]] || die '请通过 sudo 运行。'
SECURE=0
if [[ -z "$STAGE" ]]; then
 [[ -d /sys/firmware/efi ]] || die '当前不是 UEFI 启动的 Ubuntu；不能在 WSL/Legacy 模式安装。'
 if [[ -z "$ESP" ]]; then
  for candidate in /boot/efi /efi /boot; do
   if mountpoint -q "$candidate" && [[ "$(findmnt -nro FSTYPE --target "$candidate")" == vfat ]]; then
    ESP="$candidate"; break
   fi
  done
 fi
 [[ -n "$ESP" ]] || die '没有找到已挂载的 ESP；请挂载后用 --esp 指定。'
 ESP="$(realpath -e "$ESP")"
 mountpoint -q "$ESP" || die "$ESP 不是独立挂载点。"
 [[ "$(findmnt -nro FSTYPE --target "$ESP")" == vfat ]] || die 'ESP 必须是 FAT 文件系统。'
 DEVICE="$(findmnt -nro SOURCE --target "$ESP")"
 [[ -b "$DEVICE" ]] || die 'ESP 不是块设备分区。'
 PARTTYPE="$(lsblk -dnro PARTTYPE "$DEVICE")"
 [[ "${PARTTYPE,,}" == c12a7328-f81f-11d2-ba4b-00a0c93ec93b ]] || die '目标不是 GPT EFI System Partition。'
 PARENT="$(lsblk -dnro PKNAME "$DEVICE")"; PART="$(lsblk -dnro PARTN "$DEVICE")"
 GUID="$(lsblk -dnro PARTUUID "$DEVICE")"
 [[ -n "$PARENT" && "$PART" =~ ^[0-9]+$ && "$GUID" =~ ^[[:xdigit:]-]{36}$ ]] || die '不能确定 ESP 的磁盘/分区号/GPT UUID。'
 [[ -d "$ESP/EFI" ]] || die 'ESP 缺少 EFI 目录，请核实所选分区。'
 SBVAR=/sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c
 [[ -r "$SBVAR" ]] || die '无法读取 Secure Boot 状态，未继续安装。'
 SECURE="$(od -An -j4 -N1 -tu1 "$SBVAR" | tr -d '[:space:]')"
 [[ "$SECURE" == 0 || "$SECURE" == 1 ]] || die 'Secure Boot 变量无效。'
 mkdir -p "$STATE"; chmod 700 "$STATE"
 exec 9>"$STATE/install.lock"; flock -n 9 || die '另一个安装进程正在运行。'
fi
if ((DEPS)); then
 [[ $EUID -eq 0 ]] || die '--stage 非 root 运行时请使用 --no-deps。'
 apt-get update
 DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  build-essential gnu-efi efibootmgr mokutil openssl sbsigntool shim-signed
fi
KEY=; CERT=; SHIM=; MM=
if ((SECURE)); then
 for path in "$ESP/EFI/ubuntu/shimx64.efi" /usr/lib/shim/shimx64.efi.signed.latest /usr/lib/shim/shimx64.efi.signed; do
  [[ ! -f "$path" ]] || { SHIM="$path"; break; }
 done
 for path in "$ESP/EFI/ubuntu/mmx64.efi" /usr/lib/shim/mmx64.efi.signed /usr/lib/shim/mmx64.efi; do
  [[ ! -f "$path" ]] || { MM="$path"; break; }
 done
 [[ -n "$SHIM" && -n "$MM" ]] || die '未找到 Ubuntu shim/MokManager，请检查 shim-signed。'
 mkdir -p "$STATE/keys"; chmod 700 "$STATE/keys"
 KEY="$STATE/keys/MOK.key"; CERT="$STATE/keys/MOK.crt"; DER="$STATE/keys/MOK.der"
 if [[ ! -f "$KEY" && ! -f "$CERT" ]]; then
  (umask 077; openssl req -new -x509 -newkey rsa:3072 -nodes -sha256 -days 3650 \
   -subj '/CN=rEFInd Furry machine owner/' -keyout "$KEY" -out "$CERT")
 fi
 [[ -f "$KEY" && -f "$CERT" ]] || die '签名密钥不完整；请检查 /var/lib/refind-furry/keys。'
 openssl x509 -in "$CERT" -outform DER -out "$DER"
 # Do not infer enrollment solely from mokutil exit status.
 source "$ROOT/scripts/check-mok.sh"
 if ! mok_key_enrolled "$DER"; then
  echo 'Secure Boot 已开启。需要先注册本机专用 MOK，原启动顺序保持不变。'
  openssl x509 -in "$CERT" -noout -fingerprint -sha256
  echo '请设置临时注册密码；重启进入 Ubuntu shim 的 Enroll MOK → Continue → Yes，输入此密码。'
  mokutil --import "$DER"
  echo '注册申请已提交。重启完成 MOK 确认后，再运行同一安装命令。'
  exit 2
 fi
fi
WORK="$(mktemp -d)"; trap 'rm -rf -- "$WORK"' EXIT
if [[ -z "$BINARY" ]]; then
 [[ -f "$SOURCE/refind/dynamic_info.c" && -f "$SOURCE/refind/dynamic_devices.c" ]] || die '未找到自定义 rEFInd 源码；请使用完整工程包。'
 echo '正在完整构建自定义 rEFInd…'
 (cd "$SOURCE"; make clean >"$WORK/clean.log" 2>&1; make gnuefi >"$WORK/build.log" 2>&1) || {
  tail -n 60 "$WORK/build.log" >&2; die '构建失败，未安装。'
 }
 BINARY="$SOURCE/refind/refind_x64.efi"
fi
[[ -s "$BINARY" ]] || die 'EFI 二进制不存在或为空。'
# The normal build is authoritative; supplied binaries must at least be x64 EFI apps.
objdump -p "$BINARY" >"$WORK/pe.txt" || die '不能读取 EFI PE 头。'
grep -q 'pei-x86-64' "$WORK/pe.txt" || die '二进制不是 x86_64 PE。'
grep -Eq 'Subsystem[[:space:]]+0000000a' "$WORK/pe.txt" || die '二进制不是 EFI application。'
PAYLOAD="$WORK/$FOLDER"; mkdir -p "$PAYLOAD/themes/finn-term"
cp -r "$ROOT/theme/." "$PAYLOAD/themes/finn-term/"
# Normalize a theme copied from a Windows checkout for Linux deployment.
sed -i 's/\r$//' "$PAYLOAD/themes/finn-term/theme.conf"
if ((SECURE)); then
 sbsign --key "$KEY" --cert "$CERT" --output "$PAYLOAD/grubx64.efi" "$BINARY"
 sbverify --cert "$CERT" "$PAYLOAD/grubx64.efi"
 cp "$SHIM" "$PAYLOAD/shimx64.efi"; cp "$MM" "$PAYLOAD/mmx64.efi"
 LOADER="\\EFI\\$FOLDER\\shimx64.efi"
else
 cp "$BINARY" "$PAYLOAD/refind_x64.efi"
 LOADER="\\EFI\\$FOLDER\\refind_x64.efi"
fi
cat >"$PAYLOAD/refind.conf" <<'CONF'
# Production installation: scan real EFI OS loaders, no QEMU demo entries.
timeout 5
default_selection "Windows,Microsoft"
scanfor internal,external,optical,manual
scan_all_linux_kernels false
dont_scan_dirs +,EFI/refind-furry,EFI/BOOT
dont_scan_files +,mmx64.efi,MokManager.efi
showtools reboot,shutdown,about,firmware
hideui hints,arrows,badges
use_nvram false
include themes/finn-term/theme.conf
CONF
if [[ -n "$ESP" && -f "$ESP/EFI/ubuntu/shimx64.efi" ]]; then
 # Keep one explicit shim entry; avoid auto-scanning Ubuntu GRUB/shim duplicates.
 sed -i 's/^dont_scan_dirs .*/&,EFI\/ubuntu/' "$PAYLOAD/refind.conf"
 cat >>"$PAYLOAD/refind.conf" <<'CONF'
menuentry "Ubuntu" {
    loader /EFI/ubuntu/shimx64.efi
    icon /EFI/refind-furry/themes/finn-term/icons/os_ubuntu.png
}
CONF
fi
if [[ -n "$ESP" && -f "$ESP/EFI/Microsoft/Boot/bootmgfw.efi" ]]; then
 sed -i 's/^dont_scan_dirs .*/&,EFI\/Microsoft\/Boot/' "$PAYLOAD/refind.conf"
 cat >>"$PAYLOAD/refind.conf" <<'CONF'
menuentry "Windows" {
    loader /EFI/Microsoft/Boot/bootmgfw.efi
    icon /EFI/refind-furry/themes/finn-term/icons/os_win.png
}
CONF
fi
if [[ -n "$STAGE" ]]; then
 mkdir -p "$STAGE/EFI/$FOLDER"
 cp -r "$PAYLOAD/." "$STAGE/EFI/$FOLDER/"
 echo "仅生成安装包：$(realpath "$STAGE")/EFI/$FOLDER（未写入 ESP/NVRAM）"
 exit 0
fi
TARGET="$ESP/EFI/$FOLDER"
BACKUP="$STATE/backups/$(date +%Y%m%d-%H%M%S)-$$"; mkdir -p "$BACKUP"
efibootmgr -v >"$BACKUP/efibootmgr-before.txt" || die '固件变量不可写/不可读，未安装。'
OLDORDER="$(sed -n 's/^BootOrder: //p' "$BACKUP/efibootmgr-before.txt")"
[[ "$OLDORDER" =~ ^[[:xdigit:]]{4}(,[[:xdigit:]]{4})*$ ]] || die '无法读取现有 BootOrder。'
printf '%s\n' "$OLDORDER" >"$BACKUP/boot-order.txt"
[[ ! -d "$TARGET" ]] || cp -r "$TARGET" "$BACKUP/$FOLDER"
cp "$WORK/"*.log "$BACKUP/" 2>/dev/null || true
mkdir -p "$TARGET"; cp -r "$PAYLOAD/." "$TARGET/"
sync
find_entry() {
 # ENVIRON preserves literal UEFI backslashes; awk -v would interpret \r.
 efibootmgr -v | MATCH_LOADER="$LOADER" awk -v label="$LABEL" -v guid="$GUID" \
  'index($0,label) && index(tolower($0),tolower(guid)) && index(tolower($0),tolower(ENVIRON["MATCH_LOADER"])) { print substr($1,5,4); exit }'
}
ENTRY="$(find_entry)"
if [[ -z "$ENTRY" ]]; then
 # --create-only avoids changing BootOrder before registration is confirmed.
 efibootmgr --create-only --disk "/dev/$PARENT" --part "$PART" --label "$LABEL" --loader "$LOADER" || die "启动项注册失败；原启动顺序保留，备份在 $BACKUP。"
 ENTRY="$(find_entry)"
fi
[[ "$ENTRY" =~ ^[[:xdigit:]]{4}$ ]] || die "未找到安装后的启动项，备份在 $BACKUP。"
if ((ACTIVATE)); then
 ORDER="${ENTRY^^}"; IFS=',' read -ra previous <<< "$OLDORDER"
 for item in "${previous[@]}"; do [[ "${item^^}" == "${ENTRY^^}" ]] || ORDER+=",$item"; done
 efibootmgr --bootorder "$ORDER" || die "设定默认启动项失败；可在固件菜单选择 $LABEL。"
fi
efibootmgr -v >"$BACKUP/efibootmgr-after.txt"
echo "安装完成：$TARGET；启动项 Boot$ENTRY；备份：$BACKUP"
echo "恢复原启动顺序：sudo efibootmgr --bootorder $OLDORDER"
echo '请自行重启查看效果；低分辨率固件会自动缩放界面。'
