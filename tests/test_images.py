import importlib.util
import base64
import io
from unittest.mock import patch
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
    def test_download_png_and_convert(self):
        png=io.BytesIO()
        Image.new("RGB",(800,480),"black").save(png,format="PNG")
        class Response(io.BytesIO):
            status=200
            def geturl(self): return "http://ha/api/eink_dashboard/id/image.png"
        with patch.object(prepare,"urlopen",return_value=Response(png.getvalue())) as opened:
            with Image.open(prepare.download_image("http://ha/api/eink_dashboard/id/image.png")) as image:
                data,_=prepare.encode(image,dither=False)
            prepare.validate(data)
            self.assertEqual(data[16:],b"\x00"*48000)
            self.assertEqual(opened.call_args.kwargs["timeout"],15)
    def test_download_rejects_bad_url_status_empty_and_oversize(self):
        for url in ["file:///etc/passwd", "http://user:pass@ha/image.png", "http://ha/image.png\n"]:
            with self.assertRaises(ValueError): prepare.download_image(url)
        class Response(io.BytesIO):
            status=200
            def geturl(self): return "http://ha/image.png"
        for body,status in [(b"",200),(b"x"*(prepare.MAX_DOWNLOAD+1),200),(b"x",404)]:
            response=Response(body);response.status=status
            with patch.object(prepare,"urlopen",return_value=response):
                with self.assertRaises(ValueError): prepare.download_image("http://ha/image.png")
    def test_base64_cli_download_error_preserves_existing_frame(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/"frame.epd";p.write_bytes(b"old-frame")
            args=["prepare_image.py","--url-base64",base64.b64encode(b"http://ha/image.png").decode(),"--output",str(p)]
            with patch("sys.argv",args), patch.object(prepare,"urlopen",side_effect=OSError("simulated network failure")):
                with self.assertRaises(SystemExit) as failure: prepare.main()
            self.assertEqual(failure.exception.code,1)
            self.assertEqual(p.read_bytes(),b"old-frame")
    def test_atomic_replace(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/"frame.epd"
            p.write_bytes(b"previous")
            prepare.atomic_write(p,b"new")
            self.assertEqual(p.read_bytes(),b"new")
            self.assertEqual(list(p.parent.glob(".epaper-*")),[])

if __name__ == "__main__": unittest.main()
