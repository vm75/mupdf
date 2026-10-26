/* Regression tests for image indexes and per-context reflow options. */
#include "mupdf/fitz.h"
#include "../html/html-imp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) fz_throw(ctx, FZ_ERROR_GENERIC, "line %d: %s", __LINE__, #c); } while (0)

static void lock(void *user, int n) { (void)user; (void)n; }

static void
check_document(fz_context *ctx, const char *directory, const char *name, int count, int width, int height)
{
	char filename[2048];
	fz_document *doc = NULL;
	fz_pixmap *pix = NULL;
	fz_var(doc);
	fz_var(pix);
	fz_snprintf(filename, sizeof filename, "%s/%s", directory, name);
	fz_try(ctx)
	{
		doc = fz_open_document(ctx, filename);
		CHECK(fz_count_pages(ctx, doc) == count);
		if (count)
		{
			pix = fz_new_pixmap_from_page_number(ctx, doc, 0, fz_identity, fz_device_rgb(ctx), 0);
			CHECK(pix->w == width && pix->h == height);
			CHECK(pix->samples[0] == 0 && pix->samples[1] == 255 && pix->samples[2] == 0);
		}
	}
	fz_always(ctx)
	{
		fz_drop_pixmap(ctx, pix);
		fz_drop_document(ctx, doc);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

static void
check_invalid_with_error(fz_context *ctx, const char *directory, const char *name, int expected_error)
{
	char filename[2048];
	fz_document *doc = NULL;
	int rejected = 0;
	fz_var(doc);
	fz_var(rejected);
	fz_snprintf(filename, sizeof filename, "%s/%s", directory, name);
	fz_try(ctx)
		doc = fz_open_document(ctx, filename);
	fz_always(ctx)
		fz_drop_document(ctx, doc);
	fz_catch(ctx)
	{
		int error = fz_caught(ctx);
		fz_rethrow_if(ctx, FZ_ERROR_SYSTEM);
		fz_ignore_error(ctx);
		CHECK(!expected_error || error == expected_error);
		rejected = 1;
	}
	CHECK(rejected);
}

static void
check_invalid(fz_context *ctx, const char *directory, const char *name)
{
	check_invalid_with_error(ctx, directory, name, 0);
}

static void
check_scale_api(fz_context *ctx)
{
	int rejected = 0;
	fz_var(rejected);
	fz_set_reflow_image_scale(ctx, 1, 2);
	CHECK(ctx->is_image_scale == 1 && ctx->image_scale == 2);
	fz_try(ctx)
		fz_set_reflow_image_scale(ctx, 1, NAN);
	fz_catch(ctx)
	{
		CHECK(fz_caught(ctx) == FZ_ERROR_ARGUMENT);
		rejected = 1;
		fz_ignore_error(ctx);
	}
	CHECK(rejected && ctx->is_image_scale == 1 && ctx->image_scale == 2);
	fz_set_reflow_image_scale(ctx, 0, 0);
	CHECK(ctx->is_image_scale == 0 && ctx->image_scale == 1);
}

static void
check_css(fz_context *ctx)
{
	fz_html_font_set *set = fz_new_html_font_set(ctx);
	fz_css_match match = { 0 };
	fz_css_style style;
	fz_css_value font = { CSS_LENGTH, "24pt", NULL, NULL };
	fz_css_value width = { CSS_LENGTH, "100px", NULL, NULL };
	fz_css_value margin = { CSS_LENGTH, "10px", NULL, NULL };
	fz_css_value padding = { CSS_LENGTH, "8px", NULL, NULL };
	fz_css_value border = { CSS_KEYWORD, "dotted", NULL, NULL };
	match.value[PRO_FONT_SIZE] = &font;
	match.value[PRO_WIDTH] = &width;
	match.value[PRO_MARGIN_LEFT] = &margin;
	match.value[PRO_PADDING_RIGHT] = &padding;
	match.value[PRO_BORDER_TOP_STYLE] = &border;
	fz_try(ctx)
	{
		fz_apply_css_style(ctx, set, &style, &match);
		CHECK(style.font_size.unit == N_LENGTH && style.font_size.value == 24);
		fz_set_reflow_options(ctx, FZ_REFLOW_RELATIVE_FONT_SIZES);
		fz_apply_css_style(ctx, set, &style, &match);
		CHECK(style.font_size.unit == N_SCALE && style.font_size.value == 2);
		CHECK(style.width.unit == N_LENGTH && style.width.value == 100);
		CHECK(style.margin[3].value == 10 && style.padding[1].value == 8);
		CHECK(style.border_style_0 == BS_DOTTED);
		fz_set_reflow_options(ctx, FZ_REFLOW_COMPACT_SPACING);
		fz_apply_css_style(ctx, set, &style, &match);
		CHECK(style.font_size.unit == N_LENGTH && style.font_size.value == 24);
		CHECK(style.margin[3].value == 5 && style.padding[1].value == 4);
		CHECK(style.border_style_0 == BS_DOTTED);
		fz_set_reflow_options(ctx, FZ_REFLOW_SOLID_BORDERS);
		fz_apply_css_style(ctx, set, &style, &match);
		CHECK(style.margin[3].value == 10 && style.padding[1].value == 8);
		CHECK(style.border_style_0 == BS_SOLID);
		ctx->is_image_scale = 1;
		fz_apply_css_style(ctx, set, &style, &match);
		CHECK(style.width.unit == N_LENGTH && style.width.value == 100);
	}
	fz_always(ctx)
	{
		ctx->is_image_scale = 0;
		fz_set_reflow_options(ctx, 0);
		fz_drop_html_font_set(ctx, set);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

typedef struct
{
	fz_device super;
	float width, height;
	int images;
	int strokes;
	float stroke_width;
	float font_size;
	char text[256];
	int text_len;
} image_device;

static void
capture_image(fz_context *ctx, fz_device *dev_, fz_image *image, fz_matrix ctm, float alpha, fz_color_params params)
{
	image_device *dev = (image_device*)dev_;
	dev->width = hypotf(ctm.a, ctm.b);
	dev->height = hypotf(ctm.c, ctm.d);
	++dev->images;
}

static void
capture_stroke(fz_context *ctx, fz_device *dev_, const fz_path *path, const fz_stroke_state *stroke,
	fz_matrix ctm, fz_colorspace *cs, const float *color, float alpha, fz_color_params params)
{
	image_device *dev = (image_device*)dev_;
	++dev->strokes;
	dev->stroke_width = stroke->linewidth;
}

static void
capture_text(fz_context *ctx, fz_device *dev_, const fz_text *text, fz_matrix ctm,
	fz_colorspace *cs, const float *color, float alpha, fz_color_params params)
{
	image_device *dev = (image_device*)dev_;
	fz_text_span *span;
	int i;
	for (span = text->head; span; span = span->next)
	{
		dev->font_size = fz_max(dev->font_size, hypotf(span->trm.a, span->trm.b));
		for (i = 0; i < span->len && dev->text_len < sizeof(dev->text) - 1; ++i)
			if (span->items[i].ucs > 0 && span->items[i].ucs < 128)
				dev->text[dev->text_len++] = span->items[i].ucs;
	}
	dev->text[dev->text_len] = 0;
}

static image_device
render_fixture(fz_context *ctx, const char *directory, const char *name, unsigned int options)
{
	char filename[2048];
	fz_document *doc = NULL;
	fz_page *page = NULL;
	image_device *dev = NULL;
	image_device result = {0};
	fz_var(doc);
	fz_var(page);
	fz_var(dev);
	fz_set_reflow_options(ctx, options);
	fz_snprintf(filename, sizeof filename, "%s/%s", directory, name);
	fz_try(ctx)
	{
		doc = fz_open_document(ctx, filename);
		fz_layout_document(ctx, doc, 600, 800, 12);
		page = fz_load_page(ctx, doc, 0);
		dev = fz_new_derived_device(ctx, image_device);
		dev->super.fill_image = capture_image;
		dev->super.stroke_path = capture_stroke;
		dev->super.fill_text = capture_text;
		fz_run_page(ctx, page, (fz_device*)dev, fz_identity, NULL);
		fz_close_device(ctx, (fz_device*)dev);
		result = *dev;
	}
	fz_always(ctx)
	{
		fz_set_reflow_options(ctx, 0);
		fz_drop_device(ctx, (fz_device*)dev);
		fz_drop_page(ctx, page);
		fz_drop_document(ctx, doc);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
	return result;
}

static fz_buffer *(*saved_archive_read)(fz_context *, fz_archive *, const char *);
static int injected_archive_error;

static fz_buffer *
read_with_navigation_error(fz_context *ctx, fz_archive *archive, const char *name)
{
	if (!strcmp(name, "toc.xml"))
		fz_throw(ctx, injected_archive_error, "injected navigation read failure");
	return saved_archive_read(ctx, archive, name);
}

static void
check_navigation_error(fz_context *ctx, const char *directory, const char *name, int expected)
{
	char filename[2048];
	fz_archive *archive = NULL;
	fz_document *doc = NULL;
	int error = 0;
	fz_var(archive);
	fz_var(doc);
	fz_var(error);
	fz_snprintf(filename, sizeof filename, "%s/%s", directory, name);
	fz_set_reflow_options(ctx, FZ_REFLOW_IGNORE_INVALID_NAVIGATION);
	fz_try(ctx)
	{
		archive = fz_open_archive(ctx, filename);
		saved_archive_read = archive->read_entry;
		injected_archive_error = expected;
		archive->read_entry = read_with_navigation_error;
		doc = fz_open_document_with_stream_and_dir(ctx, "epub", NULL, archive);
	}
	fz_always(ctx)
	{
		fz_set_reflow_options(ctx, 0);
		fz_drop_document(ctx, doc);
		fz_drop_archive(ctx, archive);
	}
	fz_catch(ctx)
	{
		error = fz_caught(ctx);
		fz_ignore_error(ctx);
	}
	CHECK(error == expected);
}

static void
check_render_options(fz_context *ctx, const char *directory)
{
	image_device standard, changed;
	standard = render_fixture(ctx, directory, "heading.xhtml", 0);
	changed = render_fixture(ctx, directory, "heading.xhtml", FZ_REFLOW_NO_DEFAULT_CSS);
	CHECK(standard.font_size > changed.font_size && changed.font_size > 0);
	changed = render_fixture(ctx, directory, "publisher.fb2", FZ_REFLOW_NO_DEFAULT_CSS);
	CHECK(changed.font_size == 30);
	standard = render_fixture(ctx, directory, "underline.xhtml", 0);
	changed = render_fixture(ctx, directory, "underline.xhtml", FZ_REFLOW_THICK_DECORATIONS);
	CHECK(standard.strokes > 0 && changed.strokes > 0);
	CHECK(standard.stroke_width == 1 && changed.stroke_width == 3);
	standard = render_fixture(ctx, directory, "missing.xhtml", 0);
	changed = render_fixture(ctx, directory, "missing.xhtml", FZ_REFLOW_HIDE_MISSING_IMAGES);
	CHECK(strstr(standard.text, "[image]") != NULL);
	CHECK(strstr(changed.text, "[image]") == NULL);
	standard = render_fixture(ctx, directory, "extended.xhtml", 0);
	changed = render_fixture(ctx, directory, "extended.xhtml", FZ_REFLOW_EXTENDED_IMAGE_REFERENCES);
	CHECK(standard.images == 0 && changed.images == 1);
	check_invalid(ctx, directory, "broken-ncx.epub");
	check_invalid(ctx, directory, "broken-nav.epub");
	changed = render_fixture(ctx, directory, "broken-ncx.epub", FZ_REFLOW_IGNORE_INVALID_NAVIGATION);
	CHECK(changed.images == 1);
	changed = render_fixture(ctx, directory, "broken-nav.epub", FZ_REFLOW_IGNORE_INVALID_NAVIGATION);
	CHECK(changed.images == 1);
	changed = render_fixture(ctx, directory, "deep-nav.epub", FZ_REFLOW_IGNORE_INVALID_NAVIGATION);
	CHECK(changed.images == 1);
	check_navigation_error(ctx, directory, "broken-ncx.epub", FZ_ERROR_SYSTEM);
	check_navigation_error(ctx, directory, "broken-ncx.epub", FZ_ERROR_TRYLATER);
	check_navigation_error(ctx, directory, "broken-ncx.epub", FZ_ERROR_LIMIT);
	check_navigation_error(ctx, directory, "broken-nav.epub", FZ_ERROR_SYSTEM);
	check_navigation_error(ctx, directory, "broken-nav.epub", FZ_ERROR_TRYLATER);
	check_navigation_error(ctx, directory, "broken-nav.epub", FZ_ERROR_LIMIT);
}

static void
check_scaling(fz_context *ctx, const char *directory, float scale, float expected_width)
{
	char filename[2048];
	fz_document *doc = NULL;
	fz_page *page = NULL;
	image_device *dev = NULL;
	fz_var(doc);
	fz_var(page);
	fz_var(dev);
	ctx->is_image_scale = 1;
	ctx->image_scale = scale;
	fz_snprintf(filename, sizeof filename, "%s/image.html", directory);
	fz_try(ctx)
	{
		doc = fz_open_document(ctx, filename);
		fz_layout_document(ctx, doc, 600, 800, 12);
		page = fz_load_page(ctx, doc, 0);
		dev = fz_new_derived_device(ctx, image_device);
		dev->super.fill_image = capture_image;
		fz_run_page(ctx, page, (fz_device*)dev, fz_identity, NULL);
		fz_close_device(ctx, (fz_device*)dev);
		CHECK(dev->images == 1);
		CHECK(fabsf(dev->width - expected_width) < 0.01f);
		CHECK(fabsf(dev->height - expected_width / 2) < 0.01f);
	}
	fz_always(ctx)
	{
		ctx->is_image_scale = 0;
		ctx->image_scale = 1;
		fz_drop_device(ctx, (fz_device*)dev);
		fz_drop_page(ctx, page);
		fz_drop_document(ctx, doc);
	}
	fz_catch(ctx)
		fz_rethrow(ctx);
}

int
main(int argc, char **argv)
{
	fz_locks_context locks = { NULL, lock, lock };
	fz_context *ctx = fz_new_context(NULL, &locks, FZ_STORE_DEFAULT);
	fz_context *clone = NULL;
	int failed = 0;
	if (!ctx || argc != 2)
		return 1;
	fz_var(clone);
	fz_var(failed);
	fz_try(ctx)
	{
		CHECK(ctx->image_scale == 1 && ctx->is_image_scale == 0);
		ctx->image_scale = 2;
		ctx->is_image_scale = 1;
		fz_set_reflow_options(ctx, FZ_REFLOW_RELATIVE_FONT_SIZES);
		clone = fz_clone_context(ctx);
		CHECK(clone && clone->image_scale == 2 && clone->is_image_scale == 1);
		fz_set_reflow_options(ctx, FZ_REFLOW_COMPACT_SPACING);
		CHECK(fz_reflow_options(clone) == FZ_REFLOW_RELATIVE_FONT_SIZES);
		fz_set_reflow_options(ctx, 0);
		ctx->image_scale = 1;
		ctx->is_image_scale = 0;
		fz_register_document_handlers(ctx);
		check_document(ctx, argv[1], "relative.imagedir", 2, 4, 2);
		check_document(ctx, argv[1], "relative.ldir", 2, 4, 2);
		check_document(ctx, argv[1], "absolute.ldir", 2, 4, 2);
		check_document(ctx, argv[1], "uri.ldir", 1, 4, 2);
		check_document(ctx, argv[1], "utf16.ldir", 1, 4, 2);
		check_document(ctx, argv[1], "empty.ldir", 0, 0, 0);
		check_document(ctx, argv[1], "hidden.cbz", 2, 4, 2);
		check_invalid(ctx, argv[1], "bad-root.ldir");
		check_invalid(ctx, argv[1], "missing-path.ldir");
		check_invalid(ctx, argv[1], "bad-xml.ldir");
		check_scale_api(ctx);
		check_css(ctx);
		check_render_options(ctx, argv[1]);
		check_scaling(ctx, argv[1], 2, 3);
		fz_set_reflow_options(ctx, FZ_REFLOW_NO_DEFAULT_CSS | FZ_REFLOW_COMPACT_SPACING);
		check_scaling(ctx, argv[1], 2, 3);
		fz_set_reflow_options(ctx, 0);
		check_scaling(ctx, argv[1], NAN, 1.5f);
		check_scaling(ctx, argv[1], -1, 1.5f);
	}
	fz_catch(ctx)
	{
		fz_report_error(ctx);
		failed = 1;
	}
	fz_drop_context(clone);
	fz_drop_context(ctx);
	if (!failed)
		puts("Document option tests passed");
	return failed;
}
