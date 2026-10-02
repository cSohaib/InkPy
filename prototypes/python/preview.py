"""Rotate the native-panel host console capture into portrait."""
from pathlib import Path
from PIL import Image

for name in ("console", "console-menu", "file-menu", "browser-console", "editor", "editor-menu", "created-script"):
    source = Path(__file__).parent / "build" / f"{name}.pbm"
    if source.exists():
        Image.open(source).transpose(Image.Transpose.ROTATE_90).save(source.with_suffix(".png"))
