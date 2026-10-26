#!/usr/bin/env python3
"""Exercise Android.mk's codec selection without requiring an Android toolchain."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent


class AndroidCodecTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mupdf-android-config-")
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        (self.directory / "clear.mk").write_text(
            "LOCAL_MODULE :=\nLOCAL_SRC_FILES :=\nLOCAL_C_INCLUDES :=\n"
            "LOCAL_CFLAGS :=\nLOCAL_STATIC_LIBRARIES :=\nLOCAL_EXPORT_CFLAGS :=\n"
            "LOCAL_EXPORT_C_INCLUDES :=\nLOCAL_EXPORT_LDLIBS :=\nLOCAL_EXPORT_LDFLAGS :=\n")
        (self.directory / "module.mk").write_text(
            "$(info MODULE $(LOCAL_MODULE) $(LOCAL_STATIC_LIBRARIES))\n")
        (self.directory / "shared.mk").write_text(
            "$(info SHARED $(LOCAL_MODULE))\n$(info MODULE $(LOCAL_MODULE) $(LOCAL_STATIC_LIBRARIES))\n")
        (self.directory / "Makefile").write_text(
            "my-dir = " + str(ROOT / "platform/java") + "\n"
            "TARGET_CC := cc\nTARGET_ARCH_ABI := arm64-v8a\n"
            "CLEAR_VARS := " + str(self.directory / "clear.mk") + "\n"
            "BUILD_STATIC_LIBRARY := " + str(self.directory / "module.mk") + "\n"
            "BUILD_SHARED_LIBRARY := " + str(self.directory / "shared.mk") + "\n"
            "PREBUILT_STATIC_LIBRARY := $(BUILD_STATIC_LIBRARY)\n"
            "include " + str(ROOT / "platform/java/Android.mk") + "\n"
            "all: ; @:\n")
        self.environment = os.environ.copy()
        for key in ("MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES", "APP_CFLAGS",
                    "MUPDF_EXTRA_CFLAGS", "FZ_ENABLE_WEBP", "FZ_ENABLE_JXL"):
            self.environment.pop(key, None)

    def config(self, *args, sdk=True, ok=True):
        if sdk:
            for suffix in ("include/jxl/decode.h", "lib/libmupdf-jxl.a"):
                path = self.directory / "arm64-v8a" / suffix
                path.parent.mkdir(parents=True, exist_ok=True)
                path.touch()
        result = subprocess.run(["make", "--no-print-directory", "-s",
                                 "MUPDF_JXL_ROOT=" + str(self.directory), *args],
                                cwd=self.directory, env=self.environment,
                                text=True, capture_output=True)
        self.assertEqual(result.returncode == 0, ok, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def test_independent_switches(self):
        for webp, jxl in ((1, 1), (0, 1), (1, 0), (0, 0)):
            with self.subTest(webp=webp, jxl=jxl):
                output = self.config("FZ_ENABLE_WEBP=" + str(webp), "FZ_ENABLE_JXL=" + str(jxl))
                self.assertEqual("MODULE mupdf_thirdparty_libwebp" in output, bool(webp))
                self.assertEqual("MODULE mupdf_thirdparty_libjxl" in output, bool(jxl))

    def test_default_both_enabled(self):
        output = self.config()
        self.assertIn("MODULE mupdf_thirdparty_libwebp", output)
        self.assertIn("MODULE mupdf_thirdparty_libjxl", output)

    def test_disabled_jxl_needs_no_sdk(self):
        self.config("FZ_ENABLE_JXL=0", sdk=False)

    def test_missing_jxl_sdk_fails_actionably(self):
        self.assertIn("JPEG XL requires an Android SDK", self.config(sdk=False, ok=False))

    def test_compiler_defines_are_honored(self):
        output = self.config("MUPDF_EXTRA_CFLAGS=-DFZ_ENABLE_WEBP=0 -DFZ_ENABLE_JXL=0", sdk=False)
        self.assertNotIn("MODULE mupdf_thirdparty_libwebp", output)
        self.assertNotIn("MODULE mupdf_thirdparty_libjxl", output)

    def test_static_jni_reuses_the_dependency_modules(self):
        self.assertIn("SHARED mupdf_java", self.config())
        output = self.config("MUPDF_JNI_STATIC=yes")
        self.assertNotIn("SHARED mupdf_java", output)
        self.assertIn("MODULE mupdf_java", output)
        self.assertIn("mupdf_thirdparty_libjxl", output)

    def test_bad_compiler_fails_explicitly(self):
        self.assertIn("Cannot read Android codec configuration", self.config("TARGET_CC=false", ok=False))


if __name__ == "__main__":
    unittest.main()
