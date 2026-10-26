# Working on vm75/mupdf

This fork maintains WebP and optional JPEG XL image input on top of Artifex
MuPDF. Read [README.md](README.md) for extension behavior, build options, and the
upstream rebase workflow; preserve the original upstream [README](README).

## Repository map

- `source/fitz/load-webp.c`, `load-jxl.c`: decoders and metadata handling.
- `source/fitz/image.c`, `image-imp.h`: image recognition and dispatch.
- `include/mupdf/fitz/`: codec types and feature configuration.
- `source/cbz/`: standalone image and CBZ registration.
- `Makelists`, `Makerules`, `Makethird`, `platform/`: build integration.
- `source/tests/test-webp.c`, `test-jxl.c`: codec regressions.
- `scripts/rebase-fork.py`: backed-up upstream rebase and validation.

## Maintenance rules

- Keep fork commits on top of the chosen upstream base on `main`; use rebases.
- Require a clean, committed worktree before running the rebase helper.
- Preserve backups and resolve conflicts against current upstream APIs.
- Preserve RGB/gray channel validation, premultiplied alpha, bounded metadata
  parsing, and exception/resource cleanup when adapting the decoders.
- Keep the libwebp pin, source lists, and dependency documentation consistent.
- Follow [upstream C conventions](docs/reference/c/coding-style.md).
- Use KISS and YAGNI. Search first and read only task-relevant files. Preserve
  unrelated changes and existing conventions; update docs whose behavior changed.

## Validation

- Default codec configuration: `make build=debug test-webp test-jxl`.
- Both codecs: `make build=debug jxl=yes test-webp test-jxl`.
- Rebase helper: `python3 scripts/test-rebase-fork.py`.
- Use a fresh `OUT` directory for changed toolchains or compiler flags.
- Validate affected behavior before finishing. Inspect native platform source
  lists when build integration changes; distinguish inspection from native builds.

Keep README.md accurate for capabilities, setup, and maintenance workflow.
Update this file only when the repository map, commands, or fork rules change.
