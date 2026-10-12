"""Validate/build with isolated synthetic secrets; never use production secrets."""
import base64
import os
from pathlib import Path
import subprocess
import sys

import yaml

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / ".ci-build"
BUILD.mkdir(exist_ok=True)
source = (ROOT / "bilderrahmen.yaml").read_text()
remote = """      type: git
      url: https://github.com/danhae/esphome-gxepd2-esp8266
      ref: v1.1.0-beta.1
      path: components"""
if source.count(remote) != 1:
    raise SystemExit("Expected exactly one pinned external component source")
config = source.replace(remote, "      type: local\n      path: ../components")
# Regression: dependency discovery must also work without the YAML option.
config = config.replace("  platformio_options:\n    lib_ldf_mode: chain+\n", "")
(BUILD / "test.yaml").write_text(config)
secrets = {
    "wifi_ssid": "build-validation-only",
    "wifi_password": base64.b64encode(os.urandom(18)).decode(),
    "api_encryption_key": base64.b64encode(os.urandom(32)).decode(),
}
(BUILD / "secrets.yaml").write_text(yaml.safe_dump(secrets))
os.chmod(BUILD / "secrets.yaml", 0o600)
# Source generation only: firmware compilation is performed by the user.
for args in (["config"], ["compile", "--only-generate"]):
    subprocess.run(
        [sys.executable, "-m", "esphome", *args, str(BUILD / "test.yaml")],
        cwd=ROOT, check=True,
    )
