# Image indexes and reflow options

These extensions are generic library features. Reflow options are per-context,
independent, and disabled by default. Image-index documents reuse the comic
image renderer, including the WebP and JPEG XL decoders.

## Reflow options

Set options before opening and laying out documents. Keep the same options while
using those documents; parsed styles and layout caches are not invalidated by
changing a context setting. Reopen or explicitly restyle/re-layout a document
when changing presentation settings. Context clones inherit the settings by
value, so changing a clone does not change the original context.

```c
fz_set_reflow_options(ctx,
    FZ_REFLOW_RELATIVE_FONT_SIZES | FZ_REFLOW_COMPACT_SPACING);
fz_set_reflow_image_scale(ctx, 1, 1.5f);
/* Open, style and lay out the document using this context. */
```

`fz_reflow_options(ctx)` returns the flags. Passing `0` to
`fz_set_reflow_options` restores default presentation behavior.

| Flag | Behavior |
| --- | --- |
| `FZ_REFLOW_RELATIVE_FONT_SIZES` | Maps publisher font sizes to reader-relative sizes. `12pt` and `25px` correspond to `1em`; absolute point/pixel sizes have a `1em` minimum, and `em` sizes have a `0.5em` minimum. Margins, borders, colors, and other CSS units retain their normal interpretation. |
| `FZ_REFLOW_COMPACT_SPACING` | Halves horizontal margins and padding, and omits the additional soft break following an image. |
| `FZ_REFLOW_THICK_DECORATIONS` | Uses 3-point text-decoration strokes, places underlines 6 points below the baseline, and places line-through at `0.25em` above it. |
| `FZ_REFLOW_NO_DEFAULT_CSS` | Omits the built-in HTML, FB2, and MOBI stylesheets. User CSS and enabled publisher CSS still apply; this flag does not disable publisher CSS. |
| `FZ_REFLOW_EXTENDED_IMAGE_REFERENCES` | Accepts `href`/`l:href` for HTML image references, external FB2 image references, and FB2 roots detected in an otherwise generic HTML/XML input. |
| `FZ_REFLOW_IGNORE_INVALID_NAVIGATION` | Reports and skips malformed EPUB NCX or EPUB 3 navigation rather than rejecting the book. System/resource failures and deferred loading errors still propagate. |
| `FZ_REFLOW_SOLID_BORDERS` | Renders dotted CSS borders as solid borders. |
| `FZ_REFLOW_HIDE_MISSING_IMAGES` | Omits the `[image]` placeholder when an HTML image cannot be loaded. |

`fz_set_reflow_image_scale(ctx, enabled, scale)` overrides intrinsic image
sizes before fitting them to the page and ignores publisher width/height.
Enabled scales must be finite and positive; invalid input throws
`FZ_ERROR_ARGUMENT` without changing the previous setting. Disabling the override
restores publisher sizing. A new context starts with the override disabled and
scale `1`. The existing `image_scale` and `is_image_scale` context fields remain
available for source compatibility with embedding bridges; new callers should
use the setter.

## Image-index documents

An `.imagedir` document is an XML list of images, in the desired page order:

```xml
<container count="2">
  <item path="images/page2.webp"/>
  <item path="/books/images/page10.jxl"/>
</container>
```

Open it with `fz_open_document` after `fz_register_document_handlers`. The
handler is available when `FZ_ENABLE_CBZ` is enabled. Its MIME type is
`application/x-image-index+xml`; `.ldir` and `application/lxml` are accepted as
legacy aliases. Format metadata reports `image-index`.

Relative paths resolve against the index's containing directory. Absolute
filesystem paths and local `file:///...` URIs are also accepted. When opening
an index from a stream, use `fz_open_document_with_stream_and_dir` to provide
the archive/directory for relative references. XML escaping and encoding rules
apply to paths. Pages retain index order; the optional `count` attribute is
informational, and the actual item count determines the page count. Missing or
empty `path` attributes and malformed XML are rejected. Empty indexes are valid.

Only the explicit index handler reads filesystem paths; ordinary comic archives
continue to resolve pages from their archive. Both handlers use the existing
comic behavior of reporting an unreadable image and leaving its page blank.
Comic page enumeration skips dotfiles, hidden path components and root
`__MACOSX` resource-fork entries, while preserving natural page ordering.

## Embedding and previous source overlays

Rebuild embedding bridges against this fork's headers and library together.
The fork currently identifies as MuPDF 1.29.0 in
[`version.h`](../../include/mupdf/fitz/version.h); an older source bridge must be
compiled and checked against that API. This does not provide binary
compatibility with an independently built stock MuPDF library.

The previous `LibreraReader/Builder/jni/~mupdf-1.28.5` overlay was compared with
MuPDF's `1.28.5` tag before porting behavior to the current fork:

| Overlay area | Treatment in this fork |
| --- | --- |
| Image types, recognition, loading, and comic extensions | Keep this fork's WebP/JXL implementations and metadata handling. Do not replace these files with the older WebP-only versions. |
| Context image scaling, HTML layout and reader presentation | Exposed through the generic settings above, with initialized and cloned state. General CSS number parsing remains unchanged. |
| Image-folder XML documents and hidden comic files | Supported by the image-index handler and comic enumeration. Index allocation uses the actual item count. |
| EPUB navigation tolerance | Optional recovery for both NCX and EPUB 3 navigation; system and deferred errors propagate. |
| `directory.c`, `list-device.c`, `pdf-xref.c` | The overlay copies matched the 1.28.5 tag. Keep the current upstream implementations. |
| XML encoding conversion and EPUB container/encryption handling | Keep upstream conversion, prefixed-container support, and font deobfuscation. Do not port the older code that disabled these paths. |
| Missing ZIP entries | Keep upstream missing-entry behavior instead of manufacturing an uninitialized one-byte buffer. |
| SVG feature guards and HTML/XML allocation cleanup | Keep current feature guards and ownership rules. Extended image references are handled without double-loading images. |
| OpenJPEG `j2k.c` and `pi.c` | Keep the pinned upstream dependency. The overlay changes rejection/skipping of malformed MCT and component-range data; these changes are not required for codec input, image indexes, or the embedding API. |

Embedding build scripts must stop copying old whole-file overlays onto this
fork. Select the needed reflow flags through the C API, use one WebP dependency,
and link the complete Android codec dependencies as described in
[Android codec builds](android-codecs.md). Application file filters, MIME maps,
cover decoders and non-MuPDF rendering engines remain application concerns.

## Validation

```sh
make build=debug test-webp test-jxl test-document-options
```

Use separate `OUT` directories when repeating with `FZ_ENABLE_WEBP=0`,
`FZ_ENABLE_JXL=0`, and both disabled. The document-options suite creates temporary
indexes, comics, XHTML and EPUB fixtures, tests independent flags, renders
image scaling and text decorations, and checks context cloning and malformed
input. No application checkout is needed for these tests.
