#!/usr/bin/env python3
"""Build a relocatable static JPEG XL decoder SDK for ndk-build, per Android ABI."""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent
ABIS = ("armeabi-v7a", "arm64-v8a", "x86", "x86_64")


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def build(args, abi):
    work = args.output / ".build" / abi
    sdk = args.output / abi
    run("cmake", "-S", ROOT / "platform/android/jxl", "-B", work,
        "-DCMAKE_TOOLCHAIN_FILE=" + str(args.ndk / "build/cmake/android.toolchain.cmake"),
        "-DANDROID_ABI=" + abi, "-DANDROID_PLATFORM=android-" + str(args.api),
        "-DANDROID_STL=c++_static", "-DCMAKE_BUILD_TYPE=Release",
        "-DMUPDF_LIBJXL_SOURCE_DIR=" + str(args.source))
    header = work / "libjxl/lib/include/jxl/version.h"
    version = tuple(int(re.search(r"#define JPEGXL_" + part + r"_VERSION\s+(\d+)",
                                  header.read_text()).group(1))
                    for part in ("MAJOR", "MINOR", "PATCH"))
    if version < (0, 12, 0):
        raise ValueError("libjxl 0.12.0 or newer is required")
    run("cmake", "--build", work, "--target", "jxl_dec", "jxl_cms",
        "--parallel", args.jobs)
    archives = (work / "mupdf-jxl-archives.txt").read_text().splitlines()
    # Paths are passed to llvm-ar's MRI language, which does not quote whitespace.
    # Running in a directory of numbered symlinks avoids interpreting source paths.
    merge = work / "merge"
    merge.mkdir(exist_ok=True)
    for i, archive in enumerate(archives):
        link = merge / f"{i}.a"
        if link.is_symlink():
            link.unlink()
        link.symlink_to(archive)
    cache = (work / "CMakeCache.txt").read_text()
    ar = re.search(r"^CMAKE_AR:FILEPATH=(.+)$", cache, re.MULTILINE).group(1)
    mri = "CREATE libmupdf-jxl.a\n" + "".join(f"ADDLIB {i}.a\n" for i in range(len(archives)))
    run(ar, "-M", input=mri + "SAVE\nEND\n", text=True, cwd=merge)
    run(ar, "s", merge / "libmupdf-jxl.a")
    (sdk / "lib").mkdir(parents=True, exist_ok=True)
    shutil.copy2(merge / "libmupdf-jxl.a", sdk / "lib/libmupdf-jxl.a")
    include = sdk / "include/jxl"
    include.mkdir(parents=True, exist_ok=True)
    for directory in (args.source / "lib/include/jxl", header.parent):
        for path in directory.glob("*.h"):
            shutil.copy2(path, include / path.name)
    # Keep redistribution notices with the SDK, including the embedded dependencies.
    licenses = sdk / "licenses"
    licenses.mkdir(exist_ok=True)
    for name, path in {
        "libjxl": args.source / "LICENSE",
        "highway": args.source / "third_party/highway/LICENSE",
        "brotli": args.source / "third_party/brotli/LICENSE",
        "skcms": args.source / "third_party/skcms/LICENSE",
    }.items():
        shutil.copy2(path, licenses / (name + ".txt"))
    manifest = {"abi": abi, "api": args.api, "libjxl": ".".join(map(str, version)),
                "ndk": (args.ndk / "source.properties").read_text()}
    (sdk / "build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("JPEG XL SDK:", sdk, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="libjxl 0.12.0+ checkout with its submodules initialized")
    parser.add_argument("--ndk", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "build/android-jxl")
    parser.add_argument("--abis", nargs="+", choices=ABIS, default=list(ABIS))
    parser.add_argument("--api", type=int, default=24)
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 4))
    args = parser.parse_args()
    args.source = args.source.resolve()
    args.ndk = args.ndk.resolve()
    args.output = args.output.resolve()
    if args.api < 21 or args.jobs < 1:
        parser.error("--api must be at least 21 and --jobs must be positive")
    if not (args.ndk / "build/cmake/android.toolchain.cmake").is_file():
        parser.error("--ndk must name an Android NDK installation")
    try:
        for abi in args.abis:
            build(args, abi)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
