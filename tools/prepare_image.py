#!/usr/bin/env python3
"""Convert a local image to the bounded ESP8266 EPD1 wire format."""
import argparse
import os
from pathlib import Path
import struct
import tempfile
import zlib
from PIL import Image, ImageEnhance, ImageOps

WIDTH, HEIGHT = 800, 480
HEADER = struct.Struct("<4sHHII")

def encode(image, *, fit="contain", dither=True, invert=False, contrast=1.0):
    image = ImageOps.exif_transpose(image).convert("RGBA")
    background = Image.new("RGBA", image.size, "white")
    background.alpha_composite(image)
    image = background.convert("L")
    if fit == "cover":
        image = ImageOps.fit(image, (WIDTH, HEIGHT), method=Image.Resampling.LANCZOS)
    elif fit == "stretch":
        image = image.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    else:
        resized = ImageOps.contain(image, (WIDTH, HEIGHT), method=Image.Resampling.LANCZOS)
        image = Image.new("L", (WIDTH, HEIGHT), 255)
        image.paste(resized, ((WIDTH-resized.width)//2, (HEIGHT-resized.height)//2))
    image = ImageEnhance.Contrast(image).enhance(contrast)
    if invert:
        image = ImageOps.invert(image)
    preview = image.convert("1", dither=Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE)
    payload = preview.tobytes()
    assert len(payload) == 48000
    return HEADER.pack(b"EPD1", WIDTH, HEIGHT, len(payload), zlib.crc32(payload)) + payload, preview

def validate(data):
    if len(data) != 48016:
        raise ValueError("EPD1 file must contain exactly 48016 bytes")
    magic, width, height, size, crc = HEADER.unpack(data[:16])
    if (magic, width, height, size) != (b"EPD1", WIDTH, HEIGHT, 48000):
        raise ValueError("Invalid EPD1 header")
    if zlib.crc32(data[16:]) != crc:
        raise ValueError("CRC32 mismatch")

def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".epaper-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary, 0o644)
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--preview", type=Path)
    parser.add_argument("--fit", choices=["contain", "cover", "stretch"], default="contain")
    parser.add_argument("--no-dither", action="store_true")
    parser.add_argument("--invert", action="store_true")
    parser.add_argument("--contrast", type=float, default=1.0)
    args = parser.parse_args()
    if args.contrast <= 0:
        parser.error("--contrast must be greater than zero")
    if args.input.resolve() == args.output.resolve():
        parser.error("Input and output must be different files")
    try:
        with Image.open(args.input) as image:
            data, preview = encode(image, fit=args.fit, dither=not args.no_dither,
                                   invert=args.invert, contrast=args.contrast)
        validate(data)
        atomic_write(args.output, data)
        if args.preview:
            args.preview.parent.mkdir(parents=True, exist_ok=True)
            preview.save(args.preview)
    except (OSError, ValueError, Image.DecompressionBombError) as error:
        parser.exit(1, f"Image conversion failed: {error}\n")
    print(f"800x480 monochrome image written: {args.output} (48016 bytes)")

if __name__ == "__main__":
    main()
