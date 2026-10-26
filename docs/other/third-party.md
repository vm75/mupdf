# Third Party Libraries

These are the third party libraries used by MuPDF.

|Library|Version|Function|License|
|-|-|-|-|
|**Required**|
|[freetype](http://www.freetype.org/)|2.14.3|Font scaling and rendering|BSD-style|
|[harfbuzz](http://www.harfbuzz.org/)|6.0.0 with patches|Text shaping|MIT-style|
|[libjpeg](http://www.ijg.org/)|10.0 with patches|JPEG decoding|BSD-style|
|[libwebp](https://www.webmproject.org/)|1.6.0|WebP decoding|BSD-style|
|[libjxl](https://github.com/libjxl/libjxl)|0.12.0 or later (external)|JPEG XL decoding|BSD-style|
|[Incompatible fork of lcms2](http://cgit.ghostscript.com/cgi-bin/cgit.cgi/thirdparty-lcms2.git/)|2.16 with patches|Color management|MIT-style|
|[openjpeg](http://www.openjpeg.org/)|2.5.4 with upstream and local patches|JPEG 2000 decoding|BSD-style|
|[zlib](http://www.zlib.net/)|1.3.2|Deflate compression|zlib License|
|[gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser)|0.13.2 with upstream and local patches|HTML5 parser|Apache 2.0|
|[brotli](https://brotli.org/)|1.1.0 with upstream and local patches|Brotli compression|MIT-style|
|[cmark-gfm](https://github.com/github/cmark-gfm)|0.29.0.gfm.13 with patches|Markdown conversion to HTML|MIT-style|
|**Optional**|
|[FreeGLUT](http://freeglut.sourceforge.net/)|3.0.0 with patches|OpenGL API for UI|MIT-style|
|[curl](http://curl.haxx.se/)|7.66.0 with patches|HTTP data transfer|MIT-style|
|[JPEG-XR reference](https://www.itu.int/rec/T-REC-T.835/)|1.32 with patches|JPEG-XR decoding|special|
|[Tesseract](https://tesseract-ocr.github.io/)|5.5.2 with patches|OCR|Apache 2.0|
|[Leptonica](https://github.com/DanBloomberg/leptonica)|1.87.0 with patches|Tesseract dependency|BSD-style|
|[Zint](https://www.zint.org.uk/)|2.13.0.9|Zxing-cpp dependency|BSD-style|
|[Zxing-cpp](https://github.com/zxing-cpp/zxing-cpp)|2.3.0 with patches|Barcode decoding/encoding|Apache 2.0|

WebP and JPEG XL input are enabled by default. JPEG XL requires the `libjxl`
development package (version 0.12.0 or later), discoverable by `pkg-config`.
Disable either codec with `FZ_ENABLE_WEBP=0` or `FZ_ENABLE_JXL=0`; Make accepts
these variables and reads the same switches from compiler flags or `config.h`.
Disabled codecs do not require their dependencies in Make builds.
Custom installations can set `SYS_LIBJXL_CFLAGS` and `SYS_LIBJXL_LIBS`.
Android builds can use the [decoder SDK helper](../guide/android-codecs.md),
which bundles the libjxl decoder, skcms color management, Highway and Brotli
into a static archive per ABI and preserves their license notices. Other build
systems must supply the same headers and libraries for the default configuration,
or disable JPEG XL with `FZ_ENABLE_JXL=0`. JPEG XL supports `.jxl`
image and CBZ input, displaying the first coalesced frame as 8-bit grayscale or
RGB, with alpha and ICC color profiles.

Note:
[jbig2dec](https://jbig2dec.com/?utm_source=rtd-mupdf&utm_medium=rtd&utm_content=inline-link)
and
[MuJS](https://mujs.com/?utm_source=rtd-mupdf&utm_medium=rtd&utm_content=inline-link)
are included in "thirdparty" but are copyright Artifex Software Inc.
