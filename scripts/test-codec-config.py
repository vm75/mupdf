#!/usr/bin/env python3
"""Check Make's effective codec switches without installing codec libraries."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
PRINT = "fork-print-codecs: ; @echo CODECS $(USE_WEBP) $(USE_JXL) $(OUT)"


class CodecConfigTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mupdf-codec-config-")
        self.addCleanup(self.temp.cleanup)
        self.env = os.environ.copy()
        for key in ["CFLAGS", "XCFLAGS", "MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES",
                    "FZ_ENABLE_WEBP", "FZ_ENABLE_JXL", "SYS_LIBJXL_LIBS", "SYS_LIBJXL_CFLAGS"]:
            self.env.pop(key, None)
        self.env["PKG_CONFIG_PATH"] = ""
        self.env["PKG_CONFIG_LIBDIR"] = self.temp.name

    def config(self, *args, system_library=False, ok=True):
        command = ["make", "--no-print-directory", "-s", "--eval=" + PRINT]
        if system_library:
            command += ["USE_SYSTEM_LIBJXL=yes", "SYS_LIBJXL_CFLAGS=", "SYS_LIBJXL_LIBS=-ljxl"]
        result = subprocess.run(command + list(args) + ["fork-print-codecs"], cwd=ROOT,
                                env=self.env, text=True, capture_output=True)
        if not ok:
            self.assertNotEqual(result.returncode, 0)
            return result.stderr
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        fields = result.stdout.strip().split()
        self.assertEqual(fields[0], "CODECS")
        return fields[1:]

    def test_default_both_enabled(self):
        self.assertEqual(self.config(), ["yes", "yes", "build/release"])

    def test_webp_can_be_disabled_independently(self):
        self.assertEqual(self.config("FZ_ENABLE_WEBP=0"),
                         ["no", "yes", "build/release-no-webp"])

    def test_jxl_can_be_disabled_without_its_dependency(self):
        self.assertEqual(self.config("FZ_ENABLE_JXL=0"),
                         ["yes", "no", "build/release-no-jxl"])

    def test_both_can_be_disabled(self):
        self.assertEqual(self.config("FZ_ENABLE_WEBP=0", "FZ_ENABLE_JXL=0"),
                         ["no", "no", "build/release-no-webp-no-jxl"])

    def test_compiler_flags_are_honored(self):
        self.assertEqual(self.config("XCFLAGS=-DFZ_ENABLE_WEBP=0 -DFZ_ENABLE_JXL=0"),
                         ["no", "no", "build/release-no-webp-no-jxl"])

    def test_config_header_settings_are_honored(self):
        directory = Path(self.temp.name) / "include/mupdf/fitz"
        directory.mkdir(parents=True)
        header = (ROOT / "include/mupdf/fitz/config.h").read_text()
        header = header.replace("#define FZ_ENABLE_WEBP 1", "#define FZ_ENABLE_WEBP 0")
        header = header.replace("#define FZ_ENABLE_JXL 1", "#define FZ_ENABLE_JXL 0")
        (directory / "config.h").write_text(header)
        self.assertEqual(self.config("XCFLAGS=-I" + str(Path(self.temp.name) / "include")),
                         ["no", "no", "build/release-no-webp-no-jxl"])

    def test_bundled_jxl_does_not_require_a_system_dependency(self):
        self.assertEqual(self.config(),
                         ["yes", "yes", "build/release"])

    def test_system_jxl_can_be_selected_explicitly(self):
        self.assertEqual(self.config(system_library=True),
                         ["yes", "yes", "build/release"])

    def test_opted_in_system_jxl_requires_its_development_package(self):
        error = self.config("USE_SYSTEM_LIBJXL=yes", ok=False)
        self.assertIn("System JPEG XL requires libjxl", error)

    def test_bad_compiler_does_not_silently_disable_codecs(self):
        self.assertIn("Cannot read codec configuration", self.config("CC=false", ok=False))


if __name__ == "__main__":
    unittest.main()
