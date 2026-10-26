# Working on vm75/mupdf

This fork maintains WebP/JPEG XL image input, image-index documents, and
optional generic reflow settings on top of Artifex MuPDF. Read [README.md](README.md) for extension behavior, build options, and the
upstream rebase workflow; preserve the original upstream [README](README).

## Repository map

- `source/fitz/load-webp.c`, `load-jxl.c`: decoders and metadata handling.
- `source/fitz/image.c`, `image-imp.h`: image recognition and dispatch.
- `include/mupdf/fitz/`: codec types and feature configuration.
- `source/cbz/`: standalone image and CBZ registration.
- `thirdparty/libjxl/`: pinned JPEG XL source and decoder dependencies.
- `platform/jxl/`: host CMake integration for the pinned JPEG XL decoder.
- `Makelists`, `Makerules`, `Makethird`, `platform/`: build integration.
- `source/tests/test-webp.c`, `test-jxl.c`, `test-document-options.c`: regressions.
- `scripts/build-host-jxl.py`: Make's pinned JPEG XL decoder build.
- `scripts/build-android-jxl.py`, `platform/android/jxl/`: Android decoder SDK.
- [Document options](docs/guide/document-options.md): APIs, image indexes, overlay migration.
- [Android codecs](docs/guide/android-codecs.md): native setup and embedding.
- `scripts/rebase-fork.py`: backed-up upstream rebase and validation.

## Maintenance rules

- Keep fork commits on top of the chosen upstream base on `main`; use rebases.
- Require a clean, committed worktree before running the rebase helper.
- Preserve backups and resolve conflicts against current upstream APIs.
- Preserve RGB/gray channel validation, premultiplied alpha, bounded metadata
  parsing, and exception/resource cleanup when adapting the decoders.
- Keep both codecs enabled by default, with independent `FZ_ENABLE_WEBP=0`
  and `FZ_ENABLE_JXL=0` switches that remove their build dependencies.
- Keep the libwebp and libjxl pins, source lists, platform integration, and
  dependency documentation consistent. At each dependency update, pin both
  codecs to their respective latest stable upstream releases, and document the
  verified versions and build requirements together.
- Keep optional presentation behavior generic, independent, and off by default.
  Preserve context initialization/cloning and standard CSS behavior outside the
  specific opted-in property. Keep filesystem image lookup limited to indexes.
- Follow [upstream C conventions](docs/reference/c/coding-style.md).
- Use KISS and YAGNI. Search first and read only task-relevant files. Preserve
  unrelated changes and existing conventions; update docs whose behavior changed.

## Validation

- Default codec configuration: `make build=debug test-webp test-jxl test-document-options`.
- Disabled configurations: repeat with `FZ_ENABLE_WEBP=0`, with
  `FZ_ENABLE_JXL=0`, and with both flags set to `0`.
- Rebase helper: `python3 scripts/test-rebase-fork.py`.
- Codec build switches: `python3 scripts/test-codec-config.py`.
- Android codec selection: `python3 scripts/test-android-codecs.py`.
- Use a fresh `OUT` directory for changed toolchains or compiler flags.
- Validate affected behavior before finishing. Inspect native platform source
  lists when build integration changes; distinguish inspection from native builds.

Keep README.md accurate for capabilities, setup, and maintenance workflow.
Update this file only when the repository map, commands, or fork rules change.
