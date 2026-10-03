#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/prepare-python-device.sh
bash scripts/fetch-math.sh
# Fixed product profile also applies to a cached Stage 22 sdkconfig.
python3 - <<'PYCONFIG'
from pathlib import Path
p=Path("build-browser/sdkconfig")
if p.exists():
    overrides=Path("sdkconfig.browser.defaults").read_text().splitlines()
    keys={line.split("=",1)[0] for line in overrides}
    lines=[line for line in p.read_text().splitlines() if line.split("=",1)[0] not in keys and not any(line=="# "+key+" is not set" for key in keys)]
    p.write_text("\n".join(lines+overrides)+"\n")
PYCONFIG
# Separate config avoids silently inheriting diagnostic/old stack defaults.
bash scripts/build.sh -B build-browser -DSDKCONFIG="$PWD/build-browser/sdkconfig" -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.browser.defaults' -DINKPY_PYTHON_DIAGNOSTIC=OFF -DINKPY_BROWSER=ON -DINKPY_MATH_DIAGNOSTIC=OFF
cp build-browser/inkpy.bin build-browser/firmware.bin
sha256sum build-browser/firmware.bin
