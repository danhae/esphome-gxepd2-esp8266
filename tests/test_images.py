import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("prepare_image", ROOT / "tools/prepare_image.py")
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)

class ImageTests(unittest.TestCase):
    def test_white_black_polarity_and_crc(self):
        for color, byte in [("white", 255), ("black", 0)]:
            data, preview = prepare.encode(Image.new("RGB", (800,480), color), dither=False)
            prepare.validate(data)
            self.assertEqual(data[16:], bytes([byte])*48000)
            self.assertEqual(preview.size, (800,480))
            self.assertEqual(struct.unpack("<4sHHII",data[:16])[-1],zlib.crc32(data[16:]))
    def test_transparent_pixels_are_white(self):
        data,_ = prepare.encode(Image.new("RGBA",(800,480),(0,0,0,0)),dither=False)
        self.assertEqual(data[16:],b"\xff"*48000)
    def test_contain_preserves_aspect_ratio(self):
        _,preview = prepare.encode(Image.new("RGB",(100,100),"black"),dither=False)
        self.assertEqual(preview.getpixel((0,240)),255)
        self.assertEqual(preview.getpixel((400,240)),0)
    def test_corruption_and_truncation_rejected(self):
        data,_ = prepare.encode(Image.new("RGB",(800,480),"white"))
        for invalid in [data[:-1], data+b"x", data[:20]+bytes([data[20]^1])+data[21:], b"FAIL"+data[4:]]:
            with self.assertRaises(ValueError): prepare.validate(invalid)
    def test_atomic_replace(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/"frame.epd"
            p.write_bytes(b"previous")
            prepare.atomic_write(p,b"new")
            self.assertEqual(p.read_bytes(),b"new")
            self.assertEqual(list(p.parent.glob(".epaper-*")),[])

if __name__ == "__main__": unittest.main()
