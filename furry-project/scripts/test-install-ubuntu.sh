#!/usr/bin/env bash
# Exercise the real installer without touching the ESP, firmware or apt.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="$ROOT/refind"; [[ -d "$SOURCE/refind" ]] || SOURCE="$ROOT/.."
BINARY="${1:-$SOURCE/refind/refind_x64.efi}"
WORK="$(mktemp -d)"; trap 'rm -rf -- "$WORK"' EXIT
for repeat in 1 2; do
 echo "Testing package pass $repeat"
 bash "$ROOT/scripts/install-ubuntu.sh" --no-deps --binary "$BINARY" --stage "$WORK/package" >/dev/null
 cmp "$BINARY" "$WORK/package/EFI/refind-furry/refind_x64.efi"
 cmp "$ROOT/theme/background.png" "$WORK/package/EFI/refind-furry/themes/finn-term/background.png"
 grep -q '^timeout 5$' "$WORK/package/EFI/refind-furry/refind.conf"
 grep -q '^default_selection "Windows,Microsoft"$' "$WORK/package/EFI/refind-furry/refind.conf"
 grep -q '^scanfor internal,external,optical,manual$' "$WORK/package/EFI/refind-furry/refind.conf"
 grep -q '^dynamic_info true$' "$WORK/package/EFI/refind-furry/themes/finn-term/theme.conf"
 if grep -qiE '^[[:space:]]*loader.*(demo|/EFI/BOOT)' "$WORK/package/EFI/refind-furry/refind.conf"; then
  echo 'FAIL: demo/fallback loader in production configuration' >&2; exit 1
 fi
done
printf 'not an EFI binary\n' >"$WORK/bad.efi"
if bash "$ROOT/scripts/install-ubuntu.sh" --no-deps --binary "$WORK/bad.efi" --stage "$WORK/rejected" >/dev/null 2>&1; then
 echo 'FAIL: invalid binary was accepted' >&2; exit 1
fi
[[ ! -d "$WORK/rejected" ]]
echo 'PASS: production EFI layout, exact binary/background, dynamic theme, repeat install package, invalid PE rejection.'
