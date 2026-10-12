"""Compile/run host-side mocks only; does not build ESP8266 firmware."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
compiler=os.environ.get("CXX") or shutil.which("clang++") or shutil.which("g++")
if not compiler: raise SystemExit("A host C++ compiler is needed")
with tempfile.TemporaryDirectory(prefix="epaper-host-tests-") as temp:
 exe=Path(temp)/"stream-tests"
 subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-I"+str(root/"tests/mocks"),
                 "-I"+str(root/"components/gxepd2_frame"),str(root/"tests/test_stream.cpp"),
                 str(root/"components/gxepd2_frame/gxepd2_frame.cpp"),"-o",str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
