/* This file is part of schrift_x11.
 *
 * © 2025 Scaramacai
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef SCHRIFT_X11_H
#define SCHRIFT_X11_H 1

#ifdef __cplusplus
extern "C" {
#endif


#define _GNU_SOURCE // stupido gcc che altrimenti non riconosce realpath

#include <stdio.h>
#include <string.h>
#include <limits.h> //for realpath
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include <stdint.h>
#include "schrift.h"

/* This is needed to continue to use (more or less) the same font.[ch] jwm uses */
typedef struct _SFT_X
{
	char * name; //ggwm name
	char * filename; //filename (with complete path)
	double req_size;
	double xy_factor;
	SFT * sft;
	double ascent;
	double descent; //Positive (is minus the value given by SFT_Lmetrics)
} SFT_X;

SFT_X * SFT_X_create_from_file(const char * filename, double size, double xy_factor);

void SFT_X_set_points(SFT_X * sft_x, double size);

SFT * SFT_X_get_sft(SFT_X * sft_x);

int SFT_X_get_string_width(SFT_X * sft_x, char * text_string);

int SFT_X_draw_string32(Display * dpy, Drawable d, int x, int y, XRenderColor * fg, SFT_X * sft_x, char * text_string);

void SFT_X_free(SFT_X * sft_x);

/* These should be private and only be called if SFT_X_create_from_file is not used !!! */ 
SFT * SFT_create_from_file(const char * filename);

int SFT_scale_xy (SFT * sft, double size, double xy_factor);
/* ********************************************************************* */

#ifdef __cplusplus
}
#endif

#endif // SCHRIFT_X11_H
