// Copyright (C) 2004-2022 Artifex Software, Inc.
//
// This file is part of MuPDF.
//
// MuPDF is free software: you can redistribute it and/or modify it under the
// terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// MuPDF is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with MuPDF. If not, see <https://www.gnu.org/licenses/agpl-3.0.en.html>
//
// Alternative licensing terms are available from the licensor.
// For commercial licensing, see <https://www.artifex.com/> or contact
// Artifex Software, Inc., 1305 Grant Avenue - Suite 200, Novato,
// CA 94945, U.S.A., +1(415)492-9861, for further information.

#include "mupdf/fitz.h"

#include "image-imp.h"
#include "pixmap-imp.h"

#if FZ_ENABLE_JXL

#include <limits.h>
#include <jxl/decode.h>

struct jxl_info
{
	int width, height;
	int channels, alpha;
	uint8_t orientation;
	fz_colorspace *cs;
};

struct jxl_alloc
{
	fz_context *ctx;
	int failed;
};

static void *
jxl_malloc(void *opaque, size_t size)
{
	struct jxl_alloc *alloc = opaque;
	void *p = fz_malloc_no_throw(alloc->ctx, size);
	if (!p && size)
		alloc->failed = 1;
	return p;
}

static void
jxl_free(void *opaque, void *p)
{
	struct jxl_alloc *alloc = opaque;
	fz_free(alloc->ctx, p);
}

static void
check_jxl_status(fz_context *ctx, struct jxl_alloc *alloc, JxlDecoderStatus status)
{
	if (status != JXL_DEC_SUCCESS)
		fz_throw(ctx, alloc->failed ? FZ_ERROR_SYSTEM : FZ_ERROR_FORMAT, "failed decoding JPEG XL image");
}

static fz_colorspace *
load_jxl_icc_profile(fz_context *ctx, JxlDecoder *decoder, struct jxl_alloc *alloc, int channels)
{
	fz_colorspace *cs = NULL;
#if FZ_ENABLE_ICC
	fz_buffer *buffer = NULL;

	fz_var(cs);
	fz_var(buffer);

	fz_try(ctx)
	{
		size_t size;
		unsigned char *data;
		check_jxl_status(ctx, alloc, JxlDecoderGetICCProfileSize(decoder, JXL_COLOR_PROFILE_TARGET_DATA, &size));
		if (size)
		{
			buffer = fz_new_buffer(ctx, size);
			fz_append_byte_n(ctx, buffer, 0, size);
			fz_buffer_storage(ctx, buffer, &data);
			check_jxl_status(ctx, alloc, JxlDecoderGetColorAsICCProfile(decoder, JXL_COLOR_PROFILE_TARGET_DATA, data, size));
			/* Match the profile to the decoded pixels, including grayscale output. */
			cs = fz_new_icc_colorspace(ctx, channels == 1 ? FZ_COLORSPACE_GRAY : FZ_COLORSPACE_RGB, 0, NULL, buffer);
		}
	}
	fz_always(ctx)
		fz_drop_buffer(ctx, buffer);
	fz_catch(ctx)
	{
		fz_rethrow_if(ctx, FZ_ERROR_SYSTEM);
		fz_report_error(ctx);
		fz_warn(ctx, "ignoring embedded ICC profile in JPEG XL");
	}
#else
	(void)ctx;
	(void)decoder;
	(void)alloc;
	(void)channels;
#endif
	return cs;
}

static fz_pixmap *
load_jxl_image(fz_context *ctx, struct jxl_info *info, const unsigned char *data, size_t size, int only_metadata)
{
	static const uint8_t orientations[9] = { 0, 1, 5, 3, 7, 6, 4, 8, 2 };
	struct jxl_alloc alloc = { ctx, 0 };
	JxlMemoryManager memory = { &alloc, jxl_malloc, jxl_free };
	JxlPixelFormat format = { 0, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
	JxlDecoder *decoder = NULL;
	fz_pixmap *image = NULL;

	fz_var(alloc);
	fz_var(decoder);
	fz_var(image);

	fz_try(ctx)
	{
		decoder = JxlDecoderCreate(&memory);
		if (!decoder)
			fz_throw(ctx, FZ_ERROR_SYSTEM, "cannot allocate JPEG XL decoder");
		check_jxl_status(ctx, &alloc, JxlDecoderSetKeepOrientation(decoder, JXL_TRUE));
		check_jxl_status(ctx, &alloc, JxlDecoderSetUnpremultiplyAlpha(decoder, JXL_TRUE));
		check_jxl_status(ctx, &alloc, JxlDecoderSubscribeEvents(decoder,
			JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE));
		check_jxl_status(ctx, &alloc, JxlDecoderSetInput(decoder, data, size));
		JxlDecoderCloseInput(decoder);

		for (;;)
		{
			JxlDecoderStatus status = JxlDecoderProcessInput(decoder);
			if (status == JXL_DEC_BASIC_INFO)
			{
				JxlBasicInfo basic;
				check_jxl_status(ctx, &alloc, JxlDecoderGetBasicInfo(decoder, &basic));
				if (!basic.xsize || !basic.ysize || basic.xsize > INT_MAX || basic.ysize > INT_MAX ||
					(basic.num_color_channels != 1 && basic.num_color_channels != 3))
					fz_throw(ctx, FZ_ERROR_FORMAT, "invalid JPEG XL dimensions or channel count");
				info->width = basic.xsize;
				info->height = basic.ysize;
				info->channels = basic.num_color_channels;
				info->alpha = basic.alpha_bits != 0;
				if (basic.orientation >= 1 && basic.orientation <= 8)
					info->orientation = orientations[basic.orientation];
				info->cs = fz_keep_colorspace(ctx, info->channels == 1 ? fz_device_gray(ctx) : fz_device_rgb(ctx));
				format.num_channels = info->channels + info->alpha;
			}
			else if (status == JXL_DEC_COLOR_ENCODING)
			{
				fz_colorspace *cs = load_jxl_icc_profile(ctx, decoder, &alloc, info->channels);
				if (cs)
				{
					fz_drop_colorspace(ctx, info->cs);
					info->cs = cs;
				}
				if (only_metadata)
					break;
			}
			else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER)
			{
				size_t required;
				image = fz_new_pixmap(ctx, info->cs, info->width, info->height, NULL, info->alpha);
				fz_set_pixmap_resolution(ctx, image, 72, 72);
				check_jxl_status(ctx, &alloc, JxlDecoderImageOutBufferSize(decoder, &format, &required));
				if (required != (size_t)image->stride * image->h)
					fz_throw(ctx, FZ_ERROR_FORMAT, "unexpected JPEG XL output buffer size");
				check_jxl_status(ctx, &alloc, JxlDecoderSetImageOutBuffer(decoder, &format, image->samples, required));
			}
			else if (status == JXL_DEC_FULL_IMAGE)
			{
				if (!image)
					fz_throw(ctx, FZ_ERROR_FORMAT, "JPEG XL image has no pixel data");
				if (info->alpha)
					fz_premultiply_pixmap(ctx, image);
				/* Use the first coalesced frame, as for standalone GIF images. */
				break;
			}
			else
				fz_throw(ctx, alloc.failed ? FZ_ERROR_SYSTEM : FZ_ERROR_FORMAT, "incomplete or invalid JPEG XL image");
		}
	}
	fz_always(ctx)
		JxlDecoderDestroy(decoder);
	fz_catch(ctx)
	{
		fz_drop_pixmap(ctx, image);
		fz_rethrow(ctx);
	}

	return image;
}

fz_pixmap *
fz_load_jxl(fz_context *ctx, const unsigned char *data, size_t size)
{
	struct jxl_info info = { 0 };
	fz_pixmap *image = NULL;

	fz_var(info);
	fz_var(image);
	fz_try(ctx)
		image = load_jxl_image(ctx, &info, data, size, 0);
	fz_always(ctx)
		fz_drop_colorspace(ctx, info.cs);
	fz_catch(ctx)
		fz_rethrow(ctx);
	return image;
}

void
fz_load_jxl_info(fz_context *ctx, const unsigned char *data, size_t size, int *w, int *h, int *xres, int *yres, fz_colorspace **cs, uint8_t *orientation)
{
	struct jxl_info info = { 0 };

	fz_var(info);
	fz_try(ctx)
		load_jxl_image(ctx, &info, data, size, 1);
	fz_catch(ctx)
	{
		fz_drop_colorspace(ctx, info.cs);
		fz_rethrow(ctx);
	}
	*w = info.width;
	*h = info.height;
	*xres = *yres = 72;
	*cs = info.cs;
	if (orientation)
		*orientation = info.orientation;
}

#else

fz_pixmap *
fz_load_jxl(fz_context *ctx, const unsigned char *data, size_t size)
{
	fz_throw(ctx, FZ_ERROR_UNSUPPORTED, "JPEG XL support requires a build with libjxl (jxl=yes)");
}

void
fz_load_jxl_info(fz_context *ctx, const unsigned char *data, size_t size, int *w, int *h, int *xres, int *yres, fz_colorspace **cs, uint8_t *orientation)
{
	fz_throw(ctx, FZ_ERROR_UNSUPPORTED, "JPEG XL support requires a build with libjxl (jxl=yes)");
}

#endif
