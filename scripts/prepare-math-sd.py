"""Prepare a new staging tree, never write directly to an inserted SD card.
Usage: python3 scripts/prepare-math-sd.py MICROTeX_ROOT NEW_OUTPUT_DIRECTORY
"""
from pathlib import Path
import hashlib
import json
import shutil
import sys

source, output = map(Path, sys.argv[1:])
# Permit repeat runs only with our marker, never overwrite an arbitrary tree.
marker = output / '.inkpy-math-staging'
if output.exists() and not marker.is_file():
    raise SystemExit('Output exists without InkPy staging marker; choose a new directory')
output.mkdir(parents=True, exist_ok=True)
marker.write_text('Generated math diagnostic resources; not a device path.\n')
root = output / 'inkpy' / 'math'
root.mkdir(parents=True, exist_ok=True)
manifest = []
for path in sorted((source / 'res/fonts').rglob('*')):
    if not path.is_file() or path.name == 'dsrom10.ttf' or 'euler' in path.parts:
        continue
    relative = path.relative_to(source / 'res')
    dest = root / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(path, dest)
    manifest.append({'path': str(relative), 'bytes': path.stat().st_size,
                     'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
(root / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Prepared {len(manifest)} resources; copy the inkpy directory to microSD manually.')
