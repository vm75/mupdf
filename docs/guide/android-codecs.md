# Android codec builds

This fork's `platform/java/Android.mk` builds bundled libwebp 1.6.0 and can link
an Android JPEG XL decoder SDK. Both codecs are enabled by default. The Make
variables `FZ_ENABLE_WEBP=0` and `FZ_ENABLE_JXL=0` disable them independently;
compiler definitions in `APP_CFLAGS`/`MUPDF_EXTRA_CFLAGS` and overrides in
`include/mupdf/fitz/config.h` are also honored. Disabled codecs do not import
their native dependency modules. Cross builds never discover host libraries
through `pkg-config`.

## Build the JPEG XL SDK

Install Python 3, CMake, Make or Ninja, and an Android NDK. Supply a libjxl
0.12.0+ source checkout with its submodules initialized. For example:

```sh
git clone --branch v0.12.0 --recurse-submodules https://github.com/libjxl/libjxl.git /path/to/libjxl
python3 scripts/build-android-jxl.py \
    --source /path/to/libjxl \
    --ndk "$ANDROID_NDK_HOME" \
    --output "$PWD/build/android-jxl" \
    --api 24
```

The helper builds `armeabi-v7a`, `arm64-v8a`, `x86`, and `x86_64` by default.
Use `--abis arm64-v8a` to select a subset and `--jobs` to limit parallelism.
Use an API level no higher than the embedding app's minimum supported API;
the helper requires API 21 or later and defaults to 24. Its static libraries
use position-independent code and `c++_static`.

Each ABI directory contains `include/jxl/`, `lib/libmupdf-jxl.a`, dependency
license notices, and `build.json` with the ABI, API level, libjxl version, and
NDK revision. The archive combines the decoder, color management, Highway,
and Brotli decoder/common dependencies. Color management uses bundled skcms,
so it does not introduce a second lcms2 dependency into MuPDF. Intermediate
CMake builds live under the output directory's `.build/` directory.

## Build MuPDF

Run from this repository root after initializing MuPDF's submodules. Generate
the resources, then invoke the NDK directly:

```sh
make FZ_ENABLE_JXL=0 generate
"$ANDROID_NDK_HOME/ndk-build" -j4 \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=platform/java/Android.mk \
    APP_PLATFORM=android-24 \
    'APP_ABI=armeabi-v7a arm64-v8a x86 x86_64' \
    APP_STL=c++_static \
    MUPDF_JXL_ROOT="$PWD/build/android-jxl" \
    NDK_OUT=build/android/obj \
    NDK_LIBS_OUT=build/android/libs
```

`MUPDF_JXL_ROOT` defaults to `build/android-jxl` in this repository. The build
selects the matching ABI subdirectory. A missing SDK produces an explicit error
with setup instructions; it does not silently disable JPEG XL. To build without
the SDK, pass `FZ_ENABLE_JXL=0` to `ndk-build`. The resource-generation command's
switch only avoids a host libjxl requirement while generating resources; it does
not change the subsequent Android codec selection.

Use the fork's bundled WebP module consistently. An embedding makefile should
not also import a separate older libwebp implementation. Link through the NDK
module dependency graph so the C++ runtime and `libatomic` requirements are
preserved.

## Embed in another JNI library

Pass `MUPDF_JNI_STATIC=yes` to build `mupdf_java` as a static NDK module. An
embedding module can then name `mupdf_java` in `LOCAL_STATIC_LIBRARIES` while
reusing its transitive codec and third-party modules. The standalone archive
does not flatten those dependencies; keep them in the same NDK build graph.
The default remains the shared `libmupdf_java.so` library. Existing
`ADD_SOURCE_FILES`, `ADD_C_FLAGS` and `ADD_C_INCLUDES` hooks remain available.

## Validation

```sh
python3 scripts/test-android-codecs.py
```

These checks exercise makefile selection, independent codec switches,
compiler-defined switches, missing-SDK diagnostics and static JNI selection.
They complement native cross-compilation; they do not execute Android binaries.
The SDK helper supports all four ABIs. Cross-build the full JNI library for
each target ABI, then validate Android device execution in the embedding
application.
