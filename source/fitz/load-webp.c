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

#include <limits.h>
#include <string.h>

#include <webp/decode.h>
#include <webp/demux.h>

struct webp_info
{
	int width, height;
	int xres, yres;
	uint8_t orientation;
	fz_colorspace *cs;
};

static uint32_t
read_webp_exif_value(const unsigned char *data, int bytes, int big_endian)
{
	uint32_t value = 0;
	if (!big_endian)
		data += bytes;
	for (; bytes > 0; --bytes)
		value = (value << 8) | (big_endian ? *data++ : *--data);
	return value;
}

static int
webp_exif_resolution(uint32_t numerator, uint32_t denominator, int unit)
{
	uint64_t resolution;

	if (!denominator)
		return 0;
	if (unit == 2)
		resolution = (uint64_t)numerator / denominator;
	else if (unit == 3)
		resolution = (uint64_t)numerator * 254 / ((uint64_t)denominator * 100);
	else
		return 0;

	return resolution > 0 && resolution <= INT_MAX ? (int)resolution : 0;
}

static void
read_webp_exif(const WebPData *chunk, struct webp_info *info)
{
	static const uint8_t orientations[9] = { 0, 1, 5, 3, 7, 6, 4, 8, 2 };
	const unsigned char *data = chunk->bytes;
	size_t size = chunk->size, offset;
	uint32_t xnum = 0, xden = 0, ynum = 0, yden = 0;
	int big_endian, count, unit = 0, xres, yres;

	/* WebP EXIF chunks may contain a TIFF header directly or an Exif prefix. */
	if (size >= 6 && !memcmp(data, "Exif\0\0", 6))
	{
		data += 6;
		size -= 6;
	}
	if (size < 8)
		return;
	if (!memcmp(data, "II", 2))
		big_endian = 0;
	else if (!memcmp(data, "MM", 2))
		big_endian = 1;
	else
		return;
	if (read_webp_exif_value(data + 2, 2, big_endian) != 42)
		return;

	offset = read_webp_exif_value(data + 4, 4, big_endian);
	if (offset < 8 || offset > size - 2)
		return;
	count = read_webp_exif_value(data + offset, 2, big_endian);
	offset += 2;
	for (; count > 0 && offset <= size && size - offset >= 12; --count, offset += 12)
	{
		int tag = read_webp_exif_value(data + offset, 2, big_endian);
		int type = read_webp_exif_value(data + offset + 2, 2, big_endian);
		uint32_t n = read_webp_exif_value(data + offset + 4, 4, big_endian);
		size_t value_offset = read_webp_exif_value(data + offset + 8, 4, big_endian);

		if (n != 1)
			continue;
		if (tag == 0x112 && type == 3)
		{
			unsigned int orientation = read_webp_exif_value(data + offset + 8, 2, big_endian);
			if (orientation >= 1 && orientation <= 8)
				info->orientation = orientations[orientation];
		}
		else if (tag == 0x128 && type == 3)
			unit = read_webp_exif_value(data + offset + 8, 2, big_endian);
		else if ((tag == 0x11A || tag == 0x11B) && type == 5 && value_offset <= size - 8)
		{
			uint32_t numerator = read_webp_exif_value(data + value_offset, 4, big_endian);
			uint32_t denominator = read_webp_exif_value(data + value_offset + 4, 4, big_endian);
			if (tag == 0x11A)
			{
				xnum = numerator;
				xden = denominator;
			}
			else
			{
				ynum = numerator;
				yden = denominator;
			}
		}
	}

	xres = webp_exif_resolution(xnum, xden, unit);
	yres = webp_exif_resolution(ynum, yden, unit);
	if (xres && yres)
	{
		info->xres = xres;
		info->yres = yres;
	}
}

static fz_colorspace *
load_webp_icc_profile(fz_context *ctx, const WebPData *chunk)
{
	fz_colorspace *cs = NULL;
#if FZ_ENABLE_ICC
	fz_buffer *buf = NULL;

	fz_var(buf);
	fz_var(cs);

	fz_try(ctx)
	{
		buf = fz_new_buffer_from_copied_data(ctx, chunk->bytes, chunk->size);
		/* libwebp always decodes to RGB, regardless of the embedded profile. */
		cs = fz_new_icc_colorspace(ctx, FZ_COLORSPACE_RGB, 0, NULL, buf);
	}
	fz_always(ctx)
		fz_drop_buffer(ctx, buf);
	fz_catch(ctx)
	{
		fz_rethrow_if(ctx, FZ_ERROR_SYSTEM);
		fz_report_error(ctx);
		fz_warn(ctx, "ignoring embedded ICC profile in WebP");
	}
#else
	(void)ctx;
	(void)chunk;
#endif
	return cs;
}

static fz_pixmap *
load_webp_image(fz_context *ctx, struct webp_info *info, const unsigned char *data, size_t size, int only_metadata)
{
	fz_pixmap *image = NULL;
	WebPDemuxer *demux = NULL;
	WebPBitstreamFeatures features;
	WebPData webp_data;
	WebPChunkIterator chunk;

	fz_var(image);
	fz_var(demux);

	fz_try(ctx)
	{
		if (WebPGetFeatures(data, size, &features) != VP8_STATUS_OK)
			fz_throw(ctx, FZ_ERROR_FORMAT, "unable to extract WebP features");

		info->width = features.width;
		info->height = features.height;
		info->xres = 72;
		info->yres = 72;
		info->cs = fz_keep_colorspace(ctx, fz_device_rgb(ctx));

		webp_data.bytes = data;
		webp_data.size = size;
		demux = WebPDemux(&webp_data);
		if (demux)
		{
			if (WebPDemuxGetChunk(demux, "EXIF", 1, &chunk))
			{
				read_webp_exif(&chunk.chunk, info);
				WebPDemuxReleaseChunkIterator(&chunk);
			}
			if (WebPDemuxGetChunk(demux, "ICCP", 1, &chunk))
			{
				fz_try(ctx)
				{
					fz_colorspace *cs = load_webp_icc_profile(ctx, &chunk.chunk);
					if (cs)
					{
						fz_drop_colorspace(ctx, info->cs);
						info->cs = cs;
					}
				}
				fz_always(ctx)
					WebPDemuxReleaseChunkIterator(&chunk);
				fz_catch(ctx)
					fz_rethrow(ctx);
			}
		}

		if (!only_metadata)
		{
			unsigned char *samples;
			image = fz_new_pixmap(ctx, info->cs, info->width, info->height, NULL, features.has_alpha);
			fz_set_pixmap_resolution(ctx, image, info->xres, info->yres);

			/* The external-buffer API validates the size and stride before writing. */
			if (features.has_alpha)
				samples = WebPDecodeRGBAInto(data, size, image->samples, (size_t)image->stride * image->h, (int)image->stride);
			else
				samples = WebPDecodeRGBInto(data, size, image->samples, (size_t)image->stride * image->h, (int)image->stride);
			if (!samples)
				fz_throw(ctx, FZ_ERROR_FORMAT, "failed decoding WebP image");
			if (features.has_alpha)
				fz_premultiply_pixmap(ctx, image);
		}
	}
	fz_always(ctx)
		WebPDemuxDelete(demux);
	fz_catch(ctx)
	{
		fz_drop_pixmap(ctx, image);
		fz_rethrow(ctx);
	}

	return image;
}

fz_pixmap *
fz_load_webp(fz_context *ctx, const unsigned char *data, size_t size)
{
	fz_pixmap *image = NULL;
	struct webp_info info = { 0 };

	fz_var(image);
	fz_var(info);

	fz_try(ctx)
		image = load_webp_image(ctx, &info, data, size, 0);
	fz_always(ctx)
		fz_drop_colorspace(ctx, info.cs);
	fz_catch(ctx)
		fz_rethrow(ctx);

	return image;
}

void
fz_load_webp_info(fz_context *ctx, const unsigned char *data, size_t size, int *w, int *h, int *xres, int *yres, fz_colorspace **cs, uint8_t *orientation)
{
	struct webp_info info = { 0 };

	fz_var(info);

	fz_try(ctx)
		load_webp_image(ctx, &info, data, size, 1);
	fz_catch(ctx)
	{
		fz_drop_colorspace(ctx, info.cs);
		fz_rethrow(ctx);
	}

	*w = info.width;
	*h = info.height;
	*xres = info.xres;
	*yres = info.yres;
	*cs = info.cs;
	if (orientation)
		*orientation = info.orientation;
}
