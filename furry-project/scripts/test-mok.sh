#!/usr/bin/env bash
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
source "$ROOT/check-mok.sh"
WORK=$(mktemp -d); trap 'rm -rf -- "$WORK"' EXIT
cat > "$WORK/mokutil" <<'MOCK'
#!/usr/bin/env bash
printf '%s %s\n' "$2" "$MOK_TEST_MESSAGE"
exit "$MOK_TEST_STATUS"
MOCK
chmod +x "$WORK/mokutil"
export PATH="$WORK:$PATH"
for status in 0 1; do
 export MOK_TEST_STATUS=$status MOK_TEST_MESSAGE='is already enrolled'
 mok_key_enrolled /tmp/certificate.der
 export MOK_TEST_MESSAGE='is already in db'
 mok_key_enrolled /tmp/certificate.der
done
for message in 'is not enrolled' 'is already in the enrollment request' 'is blocked' 'could not be checked'; do
 export MOK_TEST_STATUS=1 MOK_TEST_MESSAGE="$message"
 if mok_key_enrolled /tmp/certificate.der; then echo "FAIL: $message"; exit 1; fi
done
echo 'PASS: enrolled exit 0/1, firmware db trust, pending/not-enrolled/errors rejected'
