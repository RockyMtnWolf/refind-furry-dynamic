#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
for file in "$ROOT/scripts/"*.sh; do bash -n "$file"; done
python3 - "$ROOT" <<'PY'
from pathlib import Path
import sys
for file in (Path(sys.argv[1])/'scripts').glob('*.py'):
    compile(file.read_text(encoding='utf-8-sig'),str(file),'exec')
print('Shell/Python syntax OK')
PY
