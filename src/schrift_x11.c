/*
 * Structures and functions to deal with libschrift and X11
 *
 * @author Scaramacai
 * @date 2024-2025
 * @licence MIT
 *
 */

#define _GNU_SOURCE // stupido gcc che altrimenti non riconosce realpath

#include <stdio.h>
#include <string.h>
#include <limits.h> //for realpath
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include <stdint.h>
#include "schrift_x11.h"

#define ABORT(cp, m) do { fprintf(stderr, "codepoint 0x%04X %s\n", cp, m); return -1; } while (0)

static int utf8_to_utf32(const uint8_t *utf8, uint32_t *utf32, int max)
{
	unsigned int c;
	int i = 0;
	--max;
	while (*utf8) {
		if (i >= max)
			return 0;
		if (!(*utf8 & 0x80U)) {
			utf32[i++] = *utf8++;
		} else if ((*utf8 & 0xe0U) == 0xc0U) {
			c = (*utf8++ & 0x1fU) << 6;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			utf32[i++] = c + (*utf8++ & 0x3fU);
		} else if ((*utf8 & 0xf0U) == 0xe0U) {
			c = (*utf8++ & 0x0fU) << 12;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			c += (*utf8++ & 0x3fU) << 6;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			utf32[i++] = c + (*utf8++ & 0x3fU);
		} else if ((*utf8 & 0xf8U) == 0xf0U) {
			c = (*utf8++ & 0x07U) << 18;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			c += (*utf8++ & 0x3fU) << 12;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			c += (*utf8++ & 0x3fU) << 6;
			if ((*utf8 & 0xc0U) != 0x80U) return 0;
			c += (*utf8++ & 0x3fU);
			if ((c & 0xFFFFF800U) == 0xD800U) return 0;
            utf32[i++] = c;
		} else return 0;
	}
	utf32[i] = 0;
	return i;
}


/**
 * Encode a code point using UTF-8
 * 
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @license MIT
 * 
 * @param out - output buffer (min 5 characters), will be 0-terminated
 * @param utf - code point 0-0x10FFFF
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which uses 3 bytes)
 */
/* original name: utf8_encode.c */
/* used for debugging only */

int codepoint_to_utf8(char *out, uint32_t utf)
{
  if (utf <= 0x7F) {
    // Plain ASCII
    out[0] = (char) utf;
    out[1] = 0;
    return 1;
  }
  else if (utf <= 0x07FF) {
    // 2-byte unicode
    out[0] = (char) (((utf >> 6) & 0x1F) | 0xC0);
    out[1] = (char) (((utf >> 0) & 0x3F) | 0x80);
    out[2] = 0;
    return 2;
  }
  else if (utf <= 0xFFFF) {
    // 3-byte unicode
    out[0] = (char) (((utf >> 12) & 0x0F) | 0xE0);
    out[1] = (char) (((utf >>  6) & 0x3F) | 0x80);
    out[2] = (char) (((utf >>  0) & 0x3F) | 0x80);
    out[3] = 0;
    return 3;
  }
  else if (utf <= 0x10FFFF) {
    // 4-byte unicode
    out[0] = (char) (((utf >> 18) & 0x07) | 0xF0);
    out[1] = (char) (((utf >> 12) & 0x3F) | 0x80);
    out[2] = (char) (((utf >>  6) & 0x3F) | 0x80);
    out[3] = (char) (((utf >>  0) & 0x3F) | 0x80);
    out[4] = 0;
    return 4;
  }
  else { 
    // error - use replacement character
    out[0] = (char) 0xEF;  
    out[1] = (char) 0xBF;
    out[2] = (char) 0xBD;
    out[3] = 0;
    return 0;
  }
}

static double font_scale(double scale)
{
  return 2.0833*scale; //This is totally arbitrary and based on my perception on the screen
}

static double font_scale_points(SFT_X * sft_x, double points)
{
/* This is mostly a try and guess */
	int dpi = 96; //points per inch of the screen
	double requested_points = points * dpi / 72; // 1 point = 1/72 inch 
//	double fheight = sft_x->ascent + sft_x->descent;
//	double fheight = (2*sft_x->ascent + sft_x->descent)/2; //Mean value of asc+desc and asc only
	double fheight = sft_x->ascent; //This seems to be the one we are more accustomed
	double scale = requested_points / fheight;
	return scale;
}

static int add_glyph_to_width(SFT_X *sft_x, unsigned cp, SFT_Glyph * previous, int * width)
{
	SFT * sft = SFT_X_get_sft(sft_x);

	SFT_Glyph gid;  //  unsigned long gid;
	if (sft_lookup(sft, cp, &gid) < 0)
		ABORT(cp, "missing");

	SFT_GMetrics mtx;
	if (sft_gmetrics(sft, gid, &mtx) < 0)
		ABORT(cp, "bad glyph metrics");

	SFT_Kerning kerning = {
		.xShift = 0,
		.yShift = 0,
	};
	if (sft_kerning(sft, *previous, gid, &kerning) < 0)
			ABORT(cp, "kerning failed");

	*width += (short) (mtx.advanceWidth); // This should be enough for normal LTR cases but what about RTL? 
	*previous = gid;

	return 0;
}

static int add_glyph(Display *dpy, GlyphSet glyphset, SFT_X *sft_x, unsigned cp, SFT_Glyph * previous, int * width)
{
	SFT * sft = SFT_X_get_sft(sft_x);

	SFT_Glyph gid;  //  unsigned long gid;
	if (sft_lookup(sft, cp, &gid) < 0)
		ABORT(cp, "missing");

	SFT_GMetrics mtx;
	if (sft_gmetrics(sft, gid, &mtx) < 0)
		ABORT(cp, "bad glyph metrics");

	SFT_Image img = {
		.width  = (mtx.minWidth + 3) & ~3,
		.height = mtx.minHeight,
	};
	
	char pixels[img.width * img.height];
	img.pixels = pixels;
	if (sft_render(sft, gid, img) < 0)
		ABORT(cp, "not rendered");

	SFT_Kerning kerning = {
		.xShift = 0,
		.yShift = 0,
	};
	if (sft_kerning(sft, *previous, gid, &kerning) < 0)
			ABORT(cp, "kerning failed");

	XGlyphInfo info = {
		.x      = (short) (-mtx.leftSideBearing - kerning.xShift),
		//.x      = (short) (-mtx.leftSideBearing),
		.y      = (short) -mtx.yOffset,
		.width  = (unsigned short) img.width,
		.height = (unsigned short) img.height,
		.xOff   = (short) (mtx.advanceWidth),
		.yOff   = 0
	};

	// Warning! We assume width is not NULL! Remenber to initialize the width variable in the caller! 

	*width += info.xOff; // This should be enough for normal LTR cases but what about RTL? 

	Glyph g = cp;
	XRenderAddGlyphs(dpy, glyphset, &g, &info, 1,
		img.pixels, (int) (img.width * img.height));

	*previous = gid;

	return 0;
}


SFT * SFT_create_from_file(const char * filename)
{
	SFT * sft = NULL;;

	if(!filename) return NULL;
	sft = (SFT *) malloc(sizeof(SFT));
	if(!sft) return NULL;
	sft->font = sft_loadfile(filename);
	if(!(sft->font)) {
		fprintf(stderr, "%s: TTF load failed", filename);
		free(sft);
		return NULL;
	}

	sft->xScale = 10.0; //set an arbitrary scale
	sft->yScale = sft->xScale;
	sft->flags  = SFT_DOWNWARD_Y;

	return sft;
}

SFT_X * SFT_X_create_from_file(const char * filename, double size, double xy_factor)
{
	SFT_LMetrics v_metrics;
	SFT_X * sft_x = NULL;
	SFT * sft = SFT_create_from_file(filename);
	if(!sft) return NULL;
	sft_x = (SFT_X *) malloc(sizeof(SFT_X));
	if(!sft_x) {
		fprintf(stderr, "SFT_X_create_from_file: malloc failed\n");
		sft_freefont(sft->font);
		free(sft);
		return NULL;
	}
	sft_x->sft = sft;
	sft_lmetrics(sft, &v_metrics);
	sft_x->ascent = v_metrics.ascender;
	sft_x->descent = - v_metrics.descender;
	sft_x->name = NULL;
	sft_x->filename = realpath(filename, NULL);
	sft_x->req_size = size;
	sft_x->xy_factor = xy_factor;

	double scale = font_scale_points(sft_x, size);
	sft->yScale *= scale;
	sft->xScale = sft->yScale;
	sft->yScale *= xy_factor; // need to move before

/* We need to recompute lmetrics now! */
	sft_lmetrics(sft, &v_metrics);
	sft_x->ascent = v_metrics.ascender;
	sft_x->descent = - v_metrics.descender;

	return sft_x;
}

void SFT_X_set_points(SFT_X * sft_x, double size)
{
	SFT_LMetrics v_metrics;
	SFT * sft = sft_x->sft;
	double scale = font_scale_points(sft_x, size);
	sft->yScale *= scale;
	sft->xScale = sft->yScale;
	sft->yScale *= sft_x->xy_factor;
	sft_x->req_size = size;

/* We need to recompute lmetrics now! */
	sft_lmetrics(sft, &v_metrics);
	sft_x->ascent = v_metrics.ascender;
	sft_x->descent = - v_metrics.descender;

	return; // sft_x;
}

SFT * SFT_X_get_sft (SFT_X * sft_x)
{
	if(!sft_x) return NULL;
	return sft_x->sft;
}


int SFT_scale_xy (SFT * sft, double size, double xy_factor)
{
	if(!sft) return -1;
	sft->xScale = font_scale(size);
	sft->yScale = xy_factor * sft->xScale;
	return 0;
}

int SFT_X_get_string_width(SFT_X * sft_x, char * text_string)
{
	int n = strlen(text_string) + 1; // for terminating \0
	unsigned codepoints[n];
	SFT_Glyph previous = 0;
	int width = 0;
	n = utf8_to_utf32((unsigned char *) text_string, codepoints, strlen(text_string) + 1);  // (const uint8_t *)

	for (int i = 0; i < n; i++) {
		add_glyph_to_width(sft_x, codepoints[i], &previous, &width);
	}

	return width;
}

/* ***************************************************
 * SFT_X_draw_string32
 *
 * Based on "A simple application that shows how to
 * use libschrift with X11 via XRender."
 * contributed to libschrift by Andor Badi.
 *
 */

int SFT_X_draw_string32(Display * dpy, Drawable d, int x, int y, XRenderColor * fg,
                                  SFT_X * sft_x, char * text_string, int max_width)
{
	XRectangle rect;
	Region r;
	int screen = DefaultScreen(dpy);
	int n = strlen(text_string) + 1; // for terminating \0
	unsigned codepoints[n];
	SFT_Glyph previous = 0;
	int width = 0;
	XRenderPictFormat *fmt = XRenderFindVisualFormat(dpy, DefaultVisual(dpy, screen));
	Picture topic =XRenderCreatePicture (dpy, d, fmt, 0, NULL);
	Pixmap fgpix = XCreatePixmap(dpy, d, 1, 1, 24);
//	Pixmap fgpix = XCreatePixmap(dpy, d, 1, 1, 32);
	XRenderPictureAttributes attr = { .repeat = True };
	fmt = XRenderFindStandardFormat(dpy, PictStandardRGB24);
//	fmt = XRenderFindStandardFormat(dpy, PictStandardARGB32);
	Picture fgpic = XRenderCreatePicture(dpy, fgpix, fmt, CPRepeat, &attr);
	XRenderFillRectangle(dpy, PictOpSrc, fgpic, fg, 0, 0, 200, 200);

	fmt = XRenderFindStandardFormat(dpy, PictStandardA8);
	GlyphSet glyphset = XRenderCreateGlyphSet(dpy, fmt);
	n = utf8_to_utf32((unsigned char *) text_string, codepoints, strlen(text_string) + 1);  // (const uint8_t *)

	for (int i = 0; i < n; i++) {
		add_glyph(dpy, glyphset, sft_x, codepoints[i], &previous, &width);
	}
	rect.x = x;
	rect.y = y;
	rect.width = width;
	if (rect.width > max_width) rect.width = max_width;
	rect.width += 2;
	rect.height = (int) ((sft_x->ascent+sft_x->descent));
	r = XCreateRegion();
	XUnionRectWithRegion(&rect, r , r);
    /* This is for debugging clip region: draw a rectangle around the region supposed to contain the string */
    //XRenderColor fg1 = { 0x00ff, 0xffaa, 0xccaa, 0xFFFF } ;//, bg = { 0, 0, 0, 0xffff};
    //XRenderFillRectangle(dpy, PictOpOver, topic, &fg1, x  , (int) (y + sft_x->ascent), width  , (int) (sft_x->ascent+sft_x->descent) + 1);
    //XRenderFillRectangle(dpy, PictOpOver, topic, &fg1, x  , (int) (y - sft_x->ascent), width  , (int) (sft_x->ascent+sft_x->descent) + 1);
    //XRenderColor fg1 = { 0x5555, 0xa4a4, 0x1616, 0xFFFF } ;//, bg = { 0, 0, 0, 0xffff};
    //XRenderFillRectangle(dpy, PictOpOver, topic, &fg1, x  , (int) (y), rect.width  , (int) (sft_x->ascent+sft_x->descent));
    /* End debugging clip region: draw a rectangle around the region supposed to contain the string */
	XRenderSetPictureClipRegion(dpy, topic,  r);
	XRenderCompositeString32(dpy, PictOpOver, fgpic, topic, NULL, glyphset, 0, 0, x, y + sft_x->ascent, codepoints, n);

	XRenderFreePicture(dpy, fgpic);
	XRenderFreePicture(dpy, topic);
	XRenderFreeGlyphSet (dpy, glyphset);
	XFreePixmap(dpy, fgpix);
	XDestroyRegion(r);
	return 0;
}

/* parses a string in the form #rgb or #rrggbb and fills an XRenderColor */
static XRenderColor * colorstring_to_xrendercolor(const char* str)
{
	unsigned int r=0, g=0, b=0;
	unsigned int len = strlen(str);
	XRenderColor * color = NULL;
	if(len == 7) {
		if (!(sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3) )		// 2 digit hex
			return NULL;
		else {
			r = r*257; /* r*256 + r */
			g = g*257;
			b = b*257;
		}
	}
	else if(len == 4) {
		if (!(sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) )		// 2 digit hex
			return NULL;
		else {
			r = r*4369; /* (r*16 + r) * (256 + 1) */
			g = g*4369;
			b = b*4369;
		}
	}
	else {
		/* len not compatible */
		return NULL;
	}
	color = (XRenderColor *) malloc(sizeof (XRenderColor));
	color->red = (uint16_t) r;
	color->green = (uint16_t) g;
	color->blue = (uint16_t) b;
	color->alpha = 0xffff; /* set alpha to full opacity; no need to premultiply */

	return color;
}

void SFT_X_free(SFT_X * sft_x)
{
	if (!sft_x) return;
	if (sft_x->sft) {
		if (sft_x->sft->font) sft_freefont(sft_x->sft->font);
		free(sft_x->sft);
	}
	if (sft_x->name) free(sft_x->name);
	if (sft_x->filename) free(sft_x->filename);
	free(sft_x);
	sft_x = NULL;
}
