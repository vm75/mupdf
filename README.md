# vm75/mupdf: WebP and JPEG XL extensions

This fork of [Artifex MuPDF](https://github.com/ArtifexSoftware/mupdf) adds WebP
and JPEG XL image input, including images inside CBZ archives. These extensions
are maintained in **vm75/mupdf** as a small sequence of commits on `main`, rebased
onto upstream. Upstream acceptance of the codec changes is not a prerequisite
for maintaining this fork.

Generic [image-index documents and per-context reflow options](docs/guide/document-options.md)
are also available to embedding applications; optional presentation behavior is
disabled by default.

For general MuPDF usage, see the original [upstream README](README) and
[MuPDF documentation](https://mupdf.readthedocs.io/).

## Extensions

| Input | Availability | Dependency | Behavior |
| --- | --- | --- | --- |
| WebP (`.webp`, `image/webp`) | Enabled by default; disable with `FZ_ENABLE_WEBP=0` | Bundled libwebp 1.6.0, or a system libwebp | Lossy/lossless images, alpha, RGB ICC profiles, EXIF orientation and resolution; standalone and CBZ input |
| JPEG XL (`.jxl`, `image/jxl`) | Enabled by default; disable with `FZ_ENABLE_JXL=0` | External libjxl 0.12.0+ development files | Raw/container codestreams, grayscale/RGB, alpha, ICC profiles, and orientation; standalone and CBZ input |

JPEG XL display output is 8-bit, including when the input has higher precision.
Animated JPEG XL displays the first coalesced frame. Animated WebP is not
supported by this decoder. These are input extensions; JPEG XL/WebP output
encoding and new PDF compression filters are outside their scope.

## Build and use

The normal [upstream build prerequisites](docs/guide/install.md) apply. The default build
also needs `pkg-config` and libjxl 0.12.0+ headers and libraries.

```sh
git clone --branch main https://github.com/vm75/mupdf.git
cd mupdf
git remote add upstream https://github.com/ArtifexSoftware/mupdf.git
git -c branch.main.remote=upstream submodule update --init --recursive

# Both decoders are enabled; output goes to build/release/.
make tools
build/release/mutool draw -o page.png image.jxl
build/release/mutool draw -o page-%d.png comic.cbz

# Disable either codec independently, or both.
make FZ_ENABLE_WEBP=0 tools
make FZ_ENABLE_JXL=0 tools
make FZ_ENABLE_WEBP=0 FZ_ENABLE_JXL=0 tools
```

The temporary `branch.main.remote` override resolves upstream's relative
submodule URLs against Artifex, even when `main` tracks this fork's `origin/main`.
The bundled libwebp URL is absolute and is unaffected.

For a custom libjxl installation, set `PKG_CONFIG_PATH` to its pkg-config
directory and configure the platform's runtime library search path. Alternatively,
supply `SYS_LIBJXL_CFLAGS` and `SYS_LIBJXL_LIBS` to Make. To use system WebP,
set `USE_SYSTEM_LIBWEBP=yes`; the corresponding overrides are
`SYS_LIBWEBP_CFLAGS` and `SYS_LIBWEBP_LIBS`.

Like JPX, both codecs default to enabled in `include/mupdf/fitz/config.h`.
Define `FZ_ENABLE_WEBP=0` or `FZ_ENABLE_JXL=0` to disable a codec in any build
system. Make accepts those switches as command-line variables and also honors
compiler defines in `XCFLAGS` or settings in `config.h`; disabled codecs do not
require their headers or libraries in Make builds. Make uses separate `-no-webp`
and `-no-jxl` output directories for disabled configurations.

WebP has Make, Android, and Visual Studio build integration. For Android JPEG XL,
use the [Android codec SDK helper and NDK integration](docs/guide/android-codecs.md).
Other build systems must supply libjxl headers and link libraries for the default
JPEG XL enabled configuration, or define `FZ_ENABLE_JXL=0`. Linux codec and
document-option tests are available; native Windows builds and Android device
execution have not been validated here.

## Keep the fork on top of upstream

Use `origin` for `vm75/mupdf` and `upstream` for `ArtifexSoftware/mupdf`. Maintain a
linear fork patch stack on `main`; rebase upstream updates instead of merging
them into it. Keep codec work, dependency updates, and fork maintenance changes
in focused commits so they remain reviewable and replayable.

The [rebase helper](scripts/rebase-fork.py) requires Python 3.9+, Git, Make, and
the codec build dependencies. Run it from a **clean `main` with committed fork
changes**. Commit staged extension work before the first run. The helper leaves
uncommitted work in place and refuses to start until the worktree is clean.

```sh
# Inspect the plan against already-fetched refs; this changes nothing.
python3 scripts/rebase-fork.py --dry-run

# Fetch upstream and replay the fork commits onto upstream/master.
python3 scripts/rebase-fork.py
```

`upstream/master` is the development branch. For a stable-release base, choose
an upstream release tag explicitly:

```sh
python3 scripts/rebase-fork.py --base 1.28.5
```

That tag is an example, not a moving "latest stable" alias. Select the intended
upstream release when updating. The helper's default remains `upstream/master`
on each invocation, so repeat `--base <tag>` when following stable releases.

Each real run creates a `backup/main-<UTC timestamp>` branch before rebasing.
It saves the plan in Git's local `fork-rebase.json`, replays only commits after
the previous upstream base with `git rebase --onto`, synchronizes submodules,
and runs all four enabled/disabled codec combinations. It then prints `git range-diff` for
review and records the new base in the local ref `refs/fork/upstream-base`.
A failed check keeps the plan so it can be retried.

On a new clone, the helper initially infers the old base from `upstream/master`.
If the fork was based on a stable release whose commits are absent from master,
supply the previous base explicitly on the first run:

```sh
python3 scripts/rebase-fork.py --from-base <previous-upstream-tag> --base <new-upstream-tag>
```

The helper rejects ambiguous inferred bases and merge commits in the fork stack.
Tags on fork commits can also require an explicit `--from-base` on a fresh clone.
Local base metadata and backup branches are not published automatically.

### Resolve conflicts and review

When a rebase stops, inspect `git status`, resolve each conflict while preserving
the extensions against the new upstream APIs, and stage the resolved files:

```sh
git status
git add <resolved-files>
python3 scripts/rebase-fork.py --continue
```

Repeat as needed. `--continue` also retries submodule synchronization and tests
after Git has finished rebasing. If tests expose an upstream API change, fix it,
commit the fix on `main`, and run `--continue` again. During an active Git rebase,
restore the original `main` with:

```sh
python3 scripts/rebase-fork.py --abort
```

After Git has finished, `--abort` does not reset branch history. The saved backup
remains available for inspection or recovery on a separate branch. The helper
can also cancel a pending plan if Git never moved `main`. It
accepts `--skip-tests` for a separate manual validation run; complete the checks
below before publishing.

Pay particular attention to format recognition, image type dispatch and metadata,
CBZ extensions, ICC channel counts, alpha premultiplication, exception cleanup,
Make dependency lists, and the Android/Windows source lists. Keep libwebp's
submodule pin and the third-party version documentation consistent.

### Validate and publish

The helper uses separate build directories for all four codec configurations.
The equivalent manual checks are:

```sh
make build=debug test-webp test-jxl test-document-options
make build=debug FZ_ENABLE_WEBP=0 test-webp test-jxl test-document-options
make build=debug FZ_ENABLE_JXL=0 test-webp test-jxl test-document-options
make build=debug FZ_ENABLE_WEBP=0 FZ_ENABLE_JXL=0 test-webp test-jxl test-document-options
```

The default checks both decoders, including pixel output, metadata, malformed
input, and allocation failures. The other configurations verify that each codec
can be disabled independently and reports an unsupported-codec error. Use a
fresh `OUT` directory when checking a new compiler or changed build flags.
For memory checks, use `build=sanitize` with a fresh `OUT`;
ASan, UBSan, and LeakSanitizer need a supported execution environment.

Review the range-diff and test results before publishing. For the first
publication of a new `main`, use `git push --set-upstream origin main`. After a
rebase of an already-published branch, review any remote changes, then use:

```sh
git push --force-with-lease origin main:main
```

The helper updates the local branch; publishing remains a manual step. Follow
this README and the helper's codec checks for future upstream updates.

## License and issues

MuPDF's existing [license](COPYING) applies. Dependency information is in
[Third Party Libraries](docs/other/third-party.md). Report fork-specific WebP,
JPEG XL, or rebase-helper problems to
[vm75/mupdf issues](https://github.com/vm75/mupdf/issues).
