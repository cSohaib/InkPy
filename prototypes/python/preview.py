"""Rotate the native-panel host console capture into portrait."""
from pathlib import Path
from PIL import Image

source = Path(__file__).parent / "build" / "console.pbm"
Image.open(source).transpose(Image.Transpose.ROTATE_90).save(source.with_suffix(".png"))
