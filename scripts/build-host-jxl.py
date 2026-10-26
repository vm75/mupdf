#!/usr/bin/env python3
"""Build the pinned host libjxl decoder archives for MuPDF's Make build."""

import argparse
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "thirdparty/libjxl")
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 4))
    parser.add_argument("--generator")
    parser.add_argument("--arch")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Release")
    args = parser.parse_args()
    args.source = args.source.resolve()
    args.build = args.build.resolve()
    if args.jobs < 1:
        parser.error("--jobs must be positive")

    version_source = (args.source / "lib/CMakeLists.txt").read_text()
    version = tuple(int(re.search(r"set\(JPEGXL_" + part + r"_VERSION\s+(\d+)\)", version_source).group(1))
                    for part in ("MAJOR", "MINOR", "PATCH"))
    if version != (0, 12, 0):
        parser.error("thirdparty/libjxl must be pinned to v0.12.0")

    configure = ["cmake", "-S", ROOT / "platform/jxl", "-B", args.build,
                 "-DMUPDF_LIBJXL_SOURCE_DIR=" + str(args.source),
                 "-DCMAKE_BUILD_TYPE=" + args.config]
    if args.generator:
        configure += ["-G", args.generator]
    if args.arch:
        configure += ["-A", args.arch]
    run(*configure)
    run("cmake", "--build", args.build, "--target", "jxl_dec", "jxl_cms",
        "--config", args.config, "--parallel", args.jobs)
    expected = (("jxl_dec", "jxl_cms", "hwy", "brotlidec", "brotlicommon")
                if os.name == "nt" else
                ("libjxl_dec", "libjxl_cms", "libhwy", "libbrotlidec", "libbrotlicommon"))
    extension = ".lib" if os.name == "nt" else ".a"
    missing = [args.build / "lib" / (name + extension) for name in expected
               if not (args.build / "lib" / (name + extension)).is_file()]
    if missing:
        parser.error("CMake did not produce expected archives: " + ", ".join(map(str, missing)))


if __name__ == "__main__":
    main()
