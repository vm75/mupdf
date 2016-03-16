#include "mupdf/fitz.h"

fz_bitmap *
fz_new_bitmap(fz_context *ctx, int w, int h, int n, int xres, int yres)
{
	fz_bitmap *bit;

	bit = fz_malloc_struct(ctx, fz_bitmap);
	bit->refs = 1;
	bit->w = w;
	bit->h = h;
	bit->n = n;
	bit->xres = xres;
	bit->yres = yres;
	/* Span is 32 bit aligned. We may want to make this 64 bit if we
	 * use SSE2 etc. */
	bit->stride = ((n * w + 31) & ~31) >> 3;

	bit->samples = fz_malloc_array(ctx, h, bit->stride);

	return bit;
}

fz_bitmap *
fz_keep_bitmap(fz_context *ctx, fz_bitmap *bit)
{
	if (bit)
		bit->refs++;
	return bit;
}

void
fz_drop_bitmap(fz_context *ctx, fz_bitmap *bit)
{
	if (bit && --bit->refs == 0)
	{
		fz_free(ctx, bit->samples);
		fz_free(ctx, bit);
	}
}

void
fz_clear_bitmap(fz_context *ctx, fz_bitmap *bit)
{
	memset(bit->samples, 0, bit->stride * bit->h);
}

void
fz_write_pbm_header(fz_context *ctx, fz_output *out, int w, int h)
{
	fz_printf(ctx, out, "P4\n%d %d\n", w, h);
}

void
fz_write_bitmap_as_pbm(fz_context *ctx, fz_output *out, fz_bitmap *bitmap)
{
	fz_write_pbm_header(ctx, out, bitmap->w, bitmap->h);

	fz_write_pbm_band(ctx, out, bitmap);
}

void
fz_write_pbm_band(fz_context *ctx, fz_output *out, fz_bitmap *bitmap)
{
	unsigned char *p;
	int h, bytestride;

	if (bitmap->n != 1)
		fz_throw(ctx, FZ_ERROR_GENERIC, "too many color components in bitmap");

	p = bitmap->samples;
	h = bitmap->h;
	bytestride = (bitmap->w + 7) >> 3;
	while (h--)
	{
		fz_write(ctx, out, p, bytestride);
		p += bitmap->stride;
	}
}

void
fz_save_bitmap_as_pbm(fz_context *ctx, fz_bitmap *bitmap, char *filename)
{
	fz_output *out = fz_new_output_with_path(ctx, filename, 0);
	fz_try(ctx)
		fz_write_bitmap_as_pbm(ctx, out, bitmap);
	fz_always(ctx)
		fz_drop_output(ctx, out);
	fz_catch(ctx)
		fz_rethrow(ctx);
}

void fz_bitmap_details(fz_bitmap *bit, int *w, int *h, int *n, int *stride)
{
	if (!bit)
	{
		if (w)
			*w = 0;
		if (h)
			*h = 0;
		if (n)
			*n = 0;
		if (stride)
			*stride = 0;
		return;
	}
	if (w)
		*w = bit->w;
	if (h)
		*h = bit->h;
	if (n)
		*n = bit->n;
	if (stride)
		*stride = bit->stride;
}
