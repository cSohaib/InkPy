"""Package an application image, SD math resources, notices and buildable sources."""
from pathlib import Path
import hashlib
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
firmware, output = map(lambda s: Path(s).resolve(), sys.argv[1:])
if output.exists():
    raise SystemExit("Choose a new output directory; release artifacts are never overwritten")
if firmware.read_bytes()[:1] != b"\xe9":
    raise SystemExit("Not an ESP application image")
math = ROOT / "prototypes/math/.deps/MicroTeX"
dependencies = [ROOT / "prototypes/math/.deps" / name
                for name in ("MicroTeX", "freetype", "tinyxml2", "md4c")]
dependencies.append(ROOT / "prototypes/python/.deps/micropython")
if not all((path / ".git").exists() for path in dependencies):
    raise SystemExit("Run the firmware build first to fetch pinned dependencies")
output.mkdir(parents=True)
shutil.copyfile(firmware, output / "firmware.bin")
with tempfile.TemporaryDirectory() as tmp:
    stage = Path(tmp) / "sd"
    subprocess.run([sys.executable, str(ROOT / "scripts/prepare-math-sd.py"),
                    str(math), str(stage)], check=True)
    shutil.copyfile(math / "LICENSE", stage / "inkpy/math/MicroTeX-LICENSE.txt")
    (stage / "README.txt").write_text(
        "Copy the inkpy folder to microSD root. These are math glyph resources.\n"
        "Keep the included font and MicroTeX license notices.\n")
    with zipfile.ZipFile(output / "inkpy-sd-resources.zip", "w",
                         zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file() and not path.name.startswith("."):
                archive.write(path, path.relative_to(stage))
# Original tree plus fetched dependencies, without build outputs or Git metadata.
tracked = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).split(b"\0")
with tarfile.open(output / "inkpy-source.tar.gz", "w:gz") as archive:
    for entry in tracked:
        if entry:
            relative = Path(entry.decode())
            archive.add(ROOT / relative, arcname=str(Path("InkPy") / relative))
    for dependency in dependencies:
        entries = subprocess.check_output(["git", "ls-files", "-z"], cwd=dependency).split(b"\0")
        for entry in entries:
            if entry:
                path = dependency / entry.decode()
                archive.add(path, arcname=str(Path("InkPy") / path.relative_to(ROOT)))
    (output / "SOURCE-REVISION.txt").write_text(
        subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT).decode())
    archive.add(output / "SOURCE-REVISION.txt", arcname="InkPy/SOURCE-REVISION.txt")
(output / "SOURCE-REVISION.txt").unlink()
# Include notices separately, so binary users need not unpack all the sources.
with zipfile.ZipFile(output / "licenses.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    paths = [ROOT / "LICENSE", ROOT / "THIRD_PARTY.md",
             ROOT / "components/ink_layout/vendor/fribidi/COPYING",
             ROOT / "components/ink_browser/FONT-LICENSE.txt"]
    paths.extend(sorted((ROOT / "third_party/notices").glob("*")))
    for path in paths:
        archive.write(path, path.relative_to(ROOT))
lines = [hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name
         for path in sorted(output.iterdir()) if path.is_file()]
(output / "SHA256SUMS").write_text("\n".join(lines) + "\n")
print("Packaged release:", output)
