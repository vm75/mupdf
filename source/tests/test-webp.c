/* Run from the repository root with: make build=debug test-webp */
#include "mupdf/fitz.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(ctx, condition) do { \
	++checks; \
	if (!(condition)) \
		fz_throw(ctx, FZ_ERROR_GENERIC, "WebP regression failed at line %d: %s", __LINE__, #condition); \
} while (0)

/* 2 by 2 fixtures encoded with libwebp 1.6.0. */
static const unsigned char webp_rgb[] = {
	0x52, 0x49, 0x46, 0x46, 0x2c, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50,
	0x56, 0x50, 0x38, 0x4c, 0x1f, 0x00, 0x00, 0x00, 0x2f, 0x01, 0x40, 0x00,
	0x00, 0x1f, 0x20, 0x10, 0x48, 0xde, 0x1f, 0x3a, 0x8d, 0xf9, 0x17, 0x10,
	0x14, 0xfc, 0x1f, 0xdd, 0xfc, 0x47, 0x64, 0x0f, 0xe0, 0x06, 0x0c, 0x11,
	0xfd, 0x0f, 0x01, 0x00,
};

static const unsigned char webp_rgba[] = {
	0x52, 0x49, 0x46, 0x46, 0x38, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50,
	0x56, 0x50, 0x38, 0x4c, 0x2b, 0x00, 0x00, 0x00, 0x2f, 0x01, 0x40, 0x00,
	0x10, 0x1f, 0x20, 0x10, 0x48, 0xd2, 0x9e, 0x7e, 0x0d, 0x05, 0x69, 0x1b,
	0x30, 0x9b, 0x7f, 0xc7, 0x7f, 0xa0, 0x04, 0x02, 0x84, 0x75, 0x67, 0xce,
	0x04, 0xc0, 0x3d, 0x99, 0x06, 0x25, 0x01, 0x02, 0xa0, 0x28, 0x23, 0x11,
	0xfd, 0x0f, 0x06, 0x00,
};

static const unsigned char webp_lossy[] = {
	0x52, 0x49, 0x46, 0x46, 0x3c, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50,
	0x56, 0x50, 0x38, 0x20, 0x30, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x9d,
	0x01, 0x2a, 0x02, 0x00, 0x02, 0x00, 0x01, 0x00, 0x1c, 0x25, 0xa0, 0x02,
	0x74, 0xba, 0x01, 0xf8, 0x01, 0xf8, 0x00, 0x03, 0xc8, 0x00, 0xfe, 0xee,
	0x0a, 0x67, 0xfe, 0xca, 0xbe, 0x13, 0x23, 0xff, 0x6a, 0xbf, 0xfe, 0xb2,
	0xab, 0xe4, 0xa8, 0xfd, 0xb3, 0x00, 0x00, 0x00,
};

static const unsigned char webp_lossy_alpha[] = {
	0x52, 0x49, 0x46, 0x46, 0x66, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50,
	0x56, 0x50, 0x38, 0x58, 0x0a, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x41, 0x4c, 0x50, 0x48, 0x05, 0x00,
	0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x40, 0x00, 0x56, 0x50, 0x38, 0x20,
	0x3a, 0x00, 0x00, 0x00, 0x70, 0x02, 0x00, 0x9d, 0x01, 0x2a, 0x02, 0x00,
	0x02, 0x00, 0x01, 0x00, 0x1c, 0x25, 0xa0, 0x02, 0x74, 0xba, 0x01, 0xf8,
	0x01, 0xf8, 0x00, 0x03, 0x44, 0x77, 0xa3, 0x40, 0x00, 0xfe, 0x6c, 0xdc,
	0x54, 0x87, 0xd7, 0x5b, 0x6c, 0x0a, 0x42, 0xc5, 0xff, 0xfe, 0xbd, 0x10,
	0x78, 0x83, 0x7e, 0xb4, 0x1f, 0xfd, 0x83, 0xa9, 0xb7, 0xaf, 0x7e, 0x3c,
	0x00, 0x00,
};



#if FZ_ENABLE_WEBP
static const unsigned char rgb_samples[] = { 255,0,0, 0,255,0, 0,0,255, 255,255,255 };
static const unsigned char rgba_samples[] = { 255,0,0,255, 0,128,0,128, 0,0,0,0, 63,30,20,64 };
static const unsigned char lossy_samples[] = { 40,120,200, 40,120,200, 40,120,200, 40,120,200 };

#endif

struct test_state
{
	size_t fail_size;
	int allocation_failed;
	int unhandled;
};

static void *
test_malloc(void *user, size_t size)
{
	struct test_state *state = user;
	if (state->fail_size && state->fail_size == size)
	{
		state->allocation_failed = 1;
		return NULL;
	}
	return malloc(size);
}

static void *
test_realloc(void *user, void *ptr, size_t size)
{
	struct test_state *state = user;
	if (state->fail_size && state->fail_size == size)
	{
		state->allocation_failed = 1;
		return NULL;
	}
	return realloc(ptr, size);
}

static void
test_free(void *user, void *ptr)
{
	(void)user;
	free(ptr);
}

static void
test_message(void *user, const char *message)
{
	struct test_state *state = user;
	if (strstr(message, "UNHANDLED EXCEPTION"))
		state->unhandled = 1;
}

#if FZ_ENABLE_WEBP

static void
put16(unsigned char *p, unsigned int value, int big_endian)
{
	p[big_endian ? 1 : 0] = value;
	p[big_endian ? 0 : 1] = value >> 8;
}

static void
put32(unsigned char *p, uint32_t value, int big_endian)
{
	int i;
	for (i = 0; i < 4; ++i)
		p[big_endian ? 3 - i : i] = value >> (8 * i);
}

static void
append_chunk(fz_context *ctx, fz_buffer *buffer, const char *tag, const unsigned char *data, size_t size)
{
	unsigned char length[4];
	put32(length, (uint32_t)size, 0);
	fz_append_data(ctx, buffer, tag, 4);
	fz_append_data(ctx, buffer, length, 4);
	fz_append_data(ctx, buffer, data, size);
	if (size & 1)
		fz_append_byte(ctx, buffer, 0);
}

static fz_buffer *
new_test_webp(fz_context *ctx, int alpha, const unsigned char *icc, size_t icc_size, const unsigned char *exif, size_t exif_size)
{
	const unsigned char *sample = alpha ? webp_rgba : webp_rgb;
	size_t sample_size = alpha ? sizeof(webp_rgba) : sizeof(webp_rgb);
	unsigned char vp8x[10] = { 0 };
	fz_buffer *buffer = fz_new_buffer(ctx, 128);

	vp8x[0] = (icc ? 32 : 0) | (exif ? 8 : 0) | (alpha ? 16 : 0);
	vp8x[4] = vp8x[7] = 1; /* 2 by 2 canvas */
	fz_try(ctx)
	{
		unsigned char *data;
		size_t size;
		fz_append_data(ctx, buffer, "RIFF\0\0\0\0WEBP", 12);
		append_chunk(ctx, buffer, "VP8X", vp8x, sizeof(vp8x));
		if (icc)
			append_chunk(ctx, buffer, "ICCP", icc, icc_size);
		fz_append_data(ctx, buffer, sample + 12, sample_size - 12);
		if (exif)
			append_chunk(ctx, buffer, "EXIF", exif, exif_size);
		size = fz_buffer_storage(ctx, buffer, &data);
		put32(data + 4, (uint32_t)(size - 8), 0);
	}
	fz_catch(ctx)
	{
		fz_drop_buffer(ctx, buffer);
		fz_rethrow(ctx);
	}
	return buffer;
}

static void
check_image(fz_context *ctx, fz_buffer *buffer, const unsigned char *expected, int alpha, int tolerance, int xres, int yres, int orientation, int device)
{
	fz_image *image = NULL;
	fz_pixmap *pixmap = NULL;

	fz_var(image);
	fz_var(pixmap);
	fz_try(ctx)
	{
		int x, y, k;
		image = fz_new_image_from_buffer(ctx, buffer);
		CHECK(ctx, image->w == 2 && image->h == 2);
		if (orientation >= 0)
			CHECK(ctx, fz_image_orientation(ctx, image) == orientation);
		if (xres >= 0)
			CHECK(ctx, image->xres == xres && image->yres == yres);
		pixmap = fz_get_unscaled_pixmap_from_image(ctx, image);
		CHECK(ctx, pixmap->w == 2 && pixmap->h == 2);
		CHECK(ctx, pixmap->n == 3 + alpha && pixmap->alpha == alpha);
		CHECK(ctx, fz_colorspace_type(ctx, pixmap->colorspace) == FZ_COLORSPACE_RGB);
		if (device >= 0)
			CHECK(ctx, fz_colorspace_is_device(ctx, pixmap->colorspace) == device);
		if (xres >= 0)
			CHECK(ctx, pixmap->xres == xres && pixmap->yres == yres);
		if (alpha)
			for (y = 0; y < 2; ++y)
				for (x = 0; x < 2; ++x)
				{
					unsigned char *pixel = pixmap->samples + y * pixmap->stride + x * 4;
					CHECK(ctx, pixel[3] == rgba_samples[(y * 2 + x) * 4 + 3]);
					CHECK(ctx, pixel[0] <= pixel[3] && pixel[1] <= pixel[3] && pixel[2] <= pixel[3]);
				}
		if (expected)
			for (y = 0; y < 2; ++y)
				for (x = 0; x < 2; ++x)
					for (k = 0; k < pixmap->n; ++k)
						CHECK(ctx, abs(pixmap->samples[y * pixmap->stride + x * pixmap->n + k] - expected[(y * 2 + x) * pixmap->n + k]) <= tolerance);
	}
	fz_always(ctx)
	{
		fz_drop_pixmap(ctx, pixmap);
		fz_drop_image(ctx, image);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

static void
check_sample(fz_context *ctx, const unsigned char *data, size_t size, const unsigned char *expected, int alpha, int tolerance)
{
	fz_buffer *buffer = fz_new_buffer_from_copied_data(ctx, data, size);
	fz_try(ctx)
		check_image(ctx, buffer, expected, alpha, tolerance, 72, 72, 0, 1);
	fz_always(ctx)
		fz_drop_buffer(ctx, buffer);
	fz_catch(ctx)
		fz_rethrow(ctx);
}

static void
check_icc(fz_context *ctx, const char *path, int rgb, int alpha)
{
	fz_buffer *profile = NULL;
	fz_buffer *buffer = NULL;
	fz_var(profile);
	fz_var(buffer);
	fz_try(ctx)
	{
		unsigned char *data;
		size_t size;
		profile = fz_read_file(ctx, path);
		size = fz_buffer_storage(ctx, profile, &data);
		buffer = new_test_webp(ctx, alpha, data, size, NULL, 0);
		check_image(ctx, buffer, alpha ? rgba_samples : rgb_samples, alpha, 0, 72, 72, 0, !(rgb && FZ_ENABLE_ICC));
	}
	fz_always(ctx)
	{
		fz_drop_buffer(ctx, buffer);
		fz_drop_buffer(ctx, profile);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

static void
check_icc_allocation_failure(fz_context *ctx, struct test_state *state)
{
#if FZ_ENABLE_ICC
	fz_buffer *profile = NULL;
	fz_buffer *buffer = NULL;
	fz_image *image = NULL;
	int caught = 0;
	fz_var(profile);
	fz_var(buffer);
	fz_var(image);
	fz_var(caught);
	fz_try(ctx)
	{
		unsigned char *data;
		size_t size;
		profile = fz_read_file(ctx, "resources/icc/rgb.icc");
		size = fz_buffer_storage(ctx, profile, &data);
		buffer = new_test_webp(ctx, 0, data, size, NULL, 0);
		state->fail_size = size;
		fz_try(ctx)
			image = fz_new_image_from_buffer(ctx, buffer);
		fz_always(ctx)
			state->fail_size = 0;
		fz_catch(ctx)
		{
			caught = fz_caught(ctx);
			fz_report_error(ctx);
		}
		CHECK(ctx, state->allocation_failed);
		CHECK(ctx, caught == FZ_ERROR_SYSTEM);
		/* A fatal ICC error must not poison the next ordinary decode. */
		check_image(ctx, buffer, rgb_samples, 0, 0, 72, 72, 0, 0);
	}
	fz_always(ctx)
	{
		state->fail_size = 0;
		fz_drop_image(ctx, image);
		fz_drop_buffer(ctx, buffer);
		fz_drop_buffer(ctx, profile);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
#else
	(void)ctx;
	(void)state;
#endif
}

static void
make_exif(unsigned char data[78], int big_endian, uint32_t numerator, uint32_t denominator, int unit)
{
	static const unsigned int tags[] = { 0x112, 0x11A, 0x11B, 0x128 };
	int i;
	memset(data, 0, 78);
	memcpy(data, big_endian ? "MM" : "II", 2);
	put16(data + 2, 42, big_endian);
	put32(data + 4, 8, big_endian);
	put16(data + 8, 4, big_endian);
	for (i = 0; i < 4; ++i)
	{
		unsigned char *entry = data + 10 + 12 * i;
		put16(entry, tags[i], big_endian);
		put16(entry + 2, i == 0 || i == 3 ? 3 : 5, big_endian);
		put32(entry + 4, 1, big_endian);
	}
	put16(data + 18, 6, big_endian);
	put32(data + 30, 62, big_endian);
	put32(data + 42, 70, big_endian);
	put16(data + 54, unit, big_endian);
	put32(data + 62, numerator, big_endian);
	put32(data + 66, denominator, big_endian);
	put32(data + 70, numerator, big_endian);
	put32(data + 74, denominator, big_endian);
}

static void
check_exif(fz_context *ctx, const unsigned char *data, size_t size, int xres, int yres, int orientation)
{
	fz_buffer *buffer = new_test_webp(ctx, 0, NULL, 0, data, size);
	fz_try(ctx)
		check_image(ctx, buffer, rgb_samples, 0, 0, xres, yres, orientation, 1);
	fz_always(ctx)
		fz_drop_buffer(ctx, buffer);
	fz_catch(ctx)
		fz_rethrow(ctx);
}

static void
check_bad_icc_and_image(fz_context *ctx)
{
	const unsigned char invalid_icc[] = { 1, 2, 3, 4 };
	fz_buffer *buffer = new_test_webp(ctx, 0, invalid_icc, sizeof(invalid_icc), NULL, 0);
	fz_image *image = NULL;
	int caught = 0;
	fz_var(image);
	fz_var(caught);
	fz_try(ctx)
	{
		check_image(ctx, buffer, rgb_samples, 0, 0, 72, 72, 0, 1);
		/* Recovery from a bad profile must consume its pending exception. */
		fz_try(ctx)
			fz_throw(ctx, FZ_ERROR_FORMAT, "expected regression-test exception");
		fz_catch(ctx)
			fz_report_error(ctx);
		fz_clear_buffer(ctx, buffer);
		fz_append_data(ctx, buffer, webp_rgb, 20);
		fz_try(ctx)
			image = fz_new_image_from_buffer(ctx, buffer);
		fz_catch(ctx)
		{
			caught = fz_caught(ctx);
			fz_report_error(ctx);
		}
		CHECK(ctx, caught == FZ_ERROR_FORMAT);
	}
	fz_always(ctx)
	{
		fz_drop_image(ctx, image);
		fz_drop_buffer(ctx, buffer);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

#else

static void
check_disabled(fz_context *ctx, const unsigned char *data, size_t size)
{
	fz_buffer *buffer = fz_new_buffer_from_copied_data(ctx, data, size);
	fz_image *image = NULL;
	int caught = 0;
	fz_var(image);
	fz_var(caught);
	fz_try(ctx)
		image = fz_new_image_from_buffer(ctx, buffer);
	fz_always(ctx)
	{
		fz_drop_image(ctx, image);
		fz_drop_buffer(ctx, buffer);
	}
	fz_catch(ctx)
	{
		caught = fz_caught(ctx);
		fz_report_error(ctx);
	}
	CHECK(ctx, caught == FZ_ERROR_UNSUPPORTED);
}

#endif

int
main(void)
{
	struct test_state state = { 0 };
	fz_alloc_context alloc = { &state, test_malloc, test_realloc, test_free };
	fz_context *ctx = fz_new_context(&alloc, NULL, FZ_STORE_DEFAULT);
	int failed = 0;

	fz_var(state);
	fz_var(failed);
	if (!ctx)
		return 1;
	fz_set_warning_callback(ctx, test_message, &state);
	fz_set_error_callback(ctx, test_message, &state);
	fz_try(ctx)
	{
#if FZ_ENABLE_WEBP
		unsigned char exif[78], prefixed[84];
		size_t i;
#endif
		CHECK(ctx, fz_lookup_image_type("webp") == FZ_IMAGE_WEBP);
		CHECK(ctx, !strcmp(fz_image_type_name(FZ_IMAGE_WEBP), "webp"));
#if FZ_ENABLE_WEBP
		check_sample(ctx, webp_rgb, sizeof(webp_rgb), rgb_samples, 0, 0);
		check_sample(ctx, webp_rgba, sizeof(webp_rgba), rgba_samples, 1, 0);
		check_sample(ctx, webp_lossy, sizeof(webp_lossy), lossy_samples, 0, 5);
		check_sample(ctx, webp_lossy_alpha, sizeof(webp_lossy_alpha), NULL, 1, 0);
		check_icc(ctx, "resources/icc/rgb.icc", 1, 0);
		check_icc(ctx, "resources/icc/gray.icc", 0, 0);
		check_icc(ctx, "resources/icc/lab.icc", 0, 0);
		check_icc(ctx, "resources/icc/cmyk.icc", 0, 0);
		check_icc(ctx, "resources/icc/cmyk.icc", 0, 1);
		check_bad_icc_and_image(ctx);
		check_icc_allocation_failure(ctx, &state);
		make_exif(exif, 0, 300, 1, 2);
		check_exif(ctx, exif, sizeof(exif), 300, 300, 4);
		put32(exif + 70, 600, 0);
		check_exif(ctx, exif, sizeof(exif), 300, 600, 4);
		make_exif(exif, 1, 100, 1, 3);
		check_exif(ctx, exif, sizeof(exif), 254, 254, 4);
		memcpy(prefixed, "Exif\0\0", 6);
		memcpy(prefixed + 6, exif, sizeof(exif));
		check_exif(ctx, prefixed, sizeof(prefixed), 254, 254, 4);
		make_exif(exif, 0, 1000000000, 1, 3);
		check_exif(ctx, exif, sizeof(exif), 72, 72, 4);
		make_exif(exif, 0, INT_MAX, 1, 2);
		check_exif(ctx, exif, sizeof(exif), INT_MAX, INT_MAX, 4);
		make_exif(exif, 0, UINT32_MAX, 1, 2);
		check_exif(ctx, exif, sizeof(exif), 72, 72, 4);
		make_exif(exif, 1, UINT32_MAX, UINT32_MAX, 2);
		check_exif(ctx, exif, sizeof(exif), 1, 1, 4);
		make_exif(exif, 0, 300, 0, 2);
		check_exif(ctx, exif, sizeof(exif), 72, 72, 4);
		make_exif(exif, 1, 300, 1, 2);
		put32(exif + 30, UINT32_MAX, 1);
		check_exif(ctx, exif, sizeof(exif), 72, 72, 4);
		make_exif(exif, 0, 300, 1, 2);
		for (i = 0; i < sizeof(exif); ++i)
			check_exif(ctx, exif, i, 72, 72, -1);
#else
		check_disabled(ctx, webp_rgb, sizeof(webp_rgb));
		check_disabled(ctx, webp_rgba, sizeof(webp_rgba));
#endif
		CHECK(ctx, !state.unhandled);
	}
	fz_catch(ctx)
	{
		fprintf(stderr, "%s\n", fz_caught_message(ctx));
		fz_report_error(ctx);
		failed = 1;
	}
	fz_drop_context(ctx);
	if (!failed)
		printf("WebP %s tests passed (%d checks)\n", FZ_ENABLE_WEBP ? "decoder" : "disabled-build", checks);
	return failed;
}
