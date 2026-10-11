#!/usr/bin/env bash
# Some mokutil versions report enrolled certificates with exit status 1.
# Require an explicit enrollment result in the C locale; pending != enrolled.
mok_key_enrolled() {
 local certificate="$1" result
 result=$(LC_ALL=C mokutil --test-key "$certificate" 2>&1) || true
 grep -Fxq -- "$certificate is already enrolled" <<< "$result" ||
  grep -Fxq -- "$certificate is already in db" <<< "$result"
}
