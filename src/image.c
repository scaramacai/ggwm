/**
 * @file image.c
 * @author Joe Wingbermuehle
 * @date 2005-2007
 *
 * @brief Functions to load images.
 *
 */

/**
 * Modified 2024 Scaramacai
 * to remove dependencies on libpng, libjpeg, and librsvng
 * and use instead stb_image.h (https://github.com/nothings/stb)
 * and nanosvg(raster).h (https://github.com/memononen/nanosvg)
 *
 */

/**
 * I spent a lot of time in figuring out how to use stb_image.h
 * and ended up with an implementation that is very similar to
 * another one, that I was not aware of, and found later on github.
 * That is due to technosaurus (https://github.com/technosaurus).
 * Technosaururus also started implementing nanosvg.
 * To my knowledge the more recent version of image.c by technosaurus can
 * be found at https://github.com/Miteam/jwm/blob/master/src/image.c
 *
 */
 

#include "ggwm.h"

#include "image.h"
#include "main.h"
#include "error.h"
#include "color.h"
#include "misc.h"

/* Use anyway nanosvg hence icons are always available */
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrastFLTK.h"

#include <stdint.h> //for uint32_t
typedef ImageNode *(*ImageLoader)(const char *fileName,
                                  int rwidth, int rheight,
                                  char preserveAspect);

static ImageNode *LoadNSVGImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);

/* include stb_image.h */
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

static ImageNode *LoadSTBImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);

static ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#ifdef USE_XBM
static ImageNode *LoadXBMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif

static ImageNode *CreateImageFromXImages(XImage *image, XImage *shape);

/* File extension to image loader mapping. */
static const struct {
   const char *extension;
   ImageLoader loader;
} IMAGE_LOADERS[] = {
   {".svg",       LoadNSVGImage      },
   {".png",       LoadSTBImage      },
   {".jpg",       LoadSTBImage     },
   {".jpeg",      LoadSTBImage     },
   {".xpm",       LoadXPMImage      },
#ifdef USE_XBM
   {".xbm",       LoadXBMImage      },
#endif
};
static const unsigned IMAGE_LOADER_COUNT = ARRAY_LENGTH(IMAGE_LOADERS);

/** Load an image from the specified file. */
ImageNode *LoadImage(const char *fileName, int rwidth, int rheight,
                     char preserveAspect)
{
   unsigned i;
   unsigned name_length;
   ImageNode *result = NULL;

   /* Make sure we have a reasonable file name. */
   if(!fileName) {
      return result;
   }
   name_length = strlen(fileName);
   if(JUNLIKELY(name_length == 0)) {
      return result;
   }

   /* Make sure the file exists. */
   if(access(fileName, R_OK) < 0) {
      return result;
   }

   /* First we attempt to use the extension to determine the type
    * to avoid trying all loaders. */
   for(i = 0; i < IMAGE_LOADER_COUNT; i++) {
      const char *ext = IMAGE_LOADERS[i].extension;
      const unsigned ext_length = strlen(ext);
      if(JLIKELY(name_length >= ext_length)) {
         const unsigned offset = name_length - ext_length;
         if(!StrCmpNoCase(&fileName[offset], ext)) {
            const ImageLoader loader = IMAGE_LOADERS[i].loader;
            result = (loader)(fileName, rwidth, rheight, preserveAspect);
            if(JLIKELY(result)) {
               return result;
            }
            break;
         }
      }
   }

   /* We were unable to load by extension, so try everything. */
   for(i = 0; i < IMAGE_LOADER_COUNT; i++) {
      const ImageLoader loader = IMAGE_LOADERS[i].loader;
      result = (loader)(fileName, rwidth, rheight, preserveAspect);
      if(result) {
         /* We were able to load the image, so it must have either the
          * wrong extension or an extension we don't recognize. */
         Warning(_("unrecognized extension for \"%s\", expected \"%s\""),
                 fileName, IMAGE_LOADERS[i].extension);
         return result;
      }
   }

   /* No image could be loaded. */
   return result;
}

/** Load an image from a pixmap. */

static void swap_channels(ImageNode * image)
{
   uint32_t i;
   uint32_t num_pixels;
   uint32_t * data_pointer;

   if(!image) return;

   num_pixels = image->width * image->height;
   data_pointer = (uint32_t  *) image->data;

   for(i=0;i < num_pixels; i++)
    //  data_pointer[i] = ((data_pointer[i] << 24) | (data_pointer[i] >> 8));
    //  depends on LSB first or MSB first, endianess, etc.
    //  the line below works
      data_pointer[i] = ((data_pointer[i] << 8) | (data_pointer[i] >> 24));

   return;
}

   
   
ImageNode *LoadImageFromDrawable(Drawable pmap, Pixmap mask)
{
   ImageNode *result = NULL;
   XImage *mask_image = NULL;
   XImage *icon_image = NULL;
   Window rwindow;
   int x, y;
   unsigned int width, height;
   unsigned int border_width;
   unsigned int depth;

   JXGetGeometry(display, pmap, &rwindow, &x, &y, &width, &height,
                 &border_width, &depth);
   icon_image = JXGetImage(display, pmap, 0, 0, width, height,
                           AllPlanes, ZPixmap);
   if(mask != None) {
      mask_image = JXGetImage(display, mask, 0, 0, width, height, 1, ZPixmap);
   }
   if(icon_image) {
      result = CreateImageFromXImages(icon_image, mask_image);
      JXDestroyImage(icon_image);
   }
   if(mask_image) {
      JXDestroyImage(mask_image);
   }
   return result;
}

/* Load a (PNG, JPEG) image from the given file name,
 * using the stb_image.h library. */
static ImageNode *LoadSTBImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{

   ImageNode *result = NULL;
   int img_channels;

//   Assert(fileName);
// We already checked that filename is Ok

   result = CreateImage(0, 0, 0);

   /* use stbi to load the image and always
    * create 4 interleaved channels (RGBA) */
   if (!(result->data = stbi_load(fileName, &result->width, &result->height,
                                  &img_channels, 4 )))  {
      Warning(_("could not decode image: %s"), fileName);
      DestroyImage(result);
      return NULL;
   }

  /* It looks like jwm uses abgr hence we need to swap channels */

   swap_channels(result);

   /* end of channel swap ------------------------------------------ */
   
   return result;
}


static ImageNode *LoadNSVGImage(const char *fileName, int rwidth, int rheight,
                                char preserveAspect)
{
   float xscale, yscale;
   ImageNode *result = NULL;
   NSVGimage *image = nsvgParseFromFile(fileName, "px", 96.0f);

   if (image) {
       if(rwidth == 0 || rheight == 0) {
          rwidth = (int) image->width;
          rheight = (int) image->height;
          xscale = 1.0;
          yscale = 1.0;
       } else if(preserveAspect) {
          if(abs((int) image->width - rwidth) < abs((int) image->height - rheight)) {
             xscale = (float)rwidth / image->width;
             rheight = image->height * xscale;
          } else {
             xscale = (float)rheight / image->height;
             rwidth = image->width * xscale;
          }
          yscale = xscale;
       } else {
          xscale = (float)rwidth / image->width;
          yscale = (float)rheight / image->height;
       }

       result = CreateImage(rwidth,rheight,0);

       NSVGrasterizer *rast = nsvgCreateRasterizer();
       nsvgRasterizeXY(rast, image, 0,0, xscale, yscale,
                       result->data, rwidth, rheight, rwidth*4);
       nsvgDeleteRasterizer(rast);
       nsvgDelete(image);

       if (result->data) {
           swap_channels(result);
        } else {
           DestroyImage(result);
           result = NULL;
        }
     }
  return result;
}

/** Load an XPM image from the specified file. */

/* xpixmap.c:
 *
 * XPixMap format file read and identify routines.  these can handle any
 * "format 1" XPixmap file with up to a memory limited number of chars
 * per pixel. It also handles XPM2 C and XPM3 format files.
 * It is not nearly as picky as it might be.
 *
 * unlike most image loading routines, this is X specific since it
 * requires X color name parsing.  to handle this we have global X
 * variables for display and screen.  it's ugly but it keeps the rest
 * of the image routines clean.
 *
 * Copyright 1989 Jim Frost.  See Copyright for complete
 * copyright information.
 *
 * Modified 16/10/92 by GWG to add version 2C and 3 support.
 * 
 * Modified 2025 by Scaramacai for use in ggwm
 */
/*
 * Copyright 1989, 1990, 1991 Jim Frost
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The author makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <strings.h>

static void freeCtable(char **ctable, int ncolors)
		/* color table */

{
	int i;

	if (ctable == NULL)
		return;
	for (i = 0; i < ncolors; i++)
		if (ctable[i] != NULL)
			free(ctable[i]);
	free(ctable);
}

/*
 * put the next word from the string s into b,
 * and return a pointer beyond the word returned.
 * Return NULL if no word is found.
 */
static char *nword(char *s, char *b)
{
	while (*s != '\0' && (*s == ' ' || *s == '\t'))
		s++;
	if (*s == '\0')
		return NULL;
	while (*s != '\0' && *s != ' ' && *s != '\t')
		*b++ = *s++;
	*b = '\0';
	return s;
}

#define XPM_FORMAT1 1
#define XPM_FORMAT2C 2
#define XPM_FORMAT3 3

/* xpm colour map key */
#define XPMKEY_NONE 0
#define XPMKEY_S    1		/* Symbolic (not supported) */
#define XPMKEY_M    2		/* Mono */
#define XPMKEY_G4   3		/* 4-level greyscale */
#define XPMKEY_G    4		/* >4 level greyscale */
#define XPMKEY_C    5		/* color */

char *xpmkeys[6] =
{"", "s", "m", "g4", "g", "c"};

/*
 * Return TRUE if we think this is an XPM file 
 */

static int
isXpixmap(FILE * xpm_file, unsigned int *w, unsigned int *h, unsigned int *cpp, unsigned int *ncolors, int *format)
			 /* image dimensions */
			 /* chars per pixel */
			 /* number of colors */
			 /* XPM format type */

{
	char buf[BUFSIZ];
	char what[BUFSIZ];
	char *p;
	unsigned int value;
	int maxlines = 8;	/* 8 lines to find #define etc. */
	unsigned int b;
	int c;


	/* read #defines until we have all that are necessary or until we
	 * get an error
	 */

	*format = *w = *h = *ncolors = 0;
	for (; maxlines-- > 0;) {
		if (!fgets((unsigned char *) buf, BUFSIZ - 1, xpm_file)) {
			return 0;
		}
		if (!strncmp(buf, "#define", 7)) {
			if (sscanf(buf, "#define %s %d", what, &value) != 2) {
				return 0;
			}
			if (!(p = strrchr(what, '_')))
				p = what;
			else
				p++;
			if (!strcmp(p, "format"))
				*format = value;
			else if (!strcmp(p, "width"))
				*w = value;
			else if (!strcmp(p, "height"))
				*h = value;
			else if (!strcmp(p, "ncolors"))
				*ncolors = value;

			/* this one is ugly
			 */

			else if (!strcmp(p, "pixel")) {		/* this isn't pretty but it works */
				if (p == what)
					continue;
				*(--p) = '\0';
				if (!(p = strrchr(what, '_')) || (p == what) || strcmp(++p, "per"))
					continue;
				*(--p) = '\0';
				if (!(p = strrchr(what, '_')))
					p = what;
				if (strcmp(++p, "chars"))
					continue;
				*cpp = value;
			}
		} else if ((sscanf(buf, "static char * %s", what) == 1) &&
			   (p = strrchr(what, '_')) && (!strcmp(p + 1, "colors[]") || !strcmp(p + 1, "mono[]"))) {
			ungetc(strlen(buf), xpm_file);	/* stuff it back so we can read it again */
			break;
		} else if ((sscanf(buf, "/* %s C */", what) == 1) &&
			   !strcmp(what, "XPM2")) {
			*format = XPM_FORMAT2C;
			break;
		} else if ((sscanf(buf, "/* %s */", what) == 1) &&
			   !strcmp(what, "XPM")) {
			*format = XPM_FORMAT3;
			break;
		}
	}

	if (maxlines <= 0) {
		return 0;
	}
	if (*format == XPM_FORMAT2C || *format == XPM_FORMAT3) {
		for (; maxlines-- > 0;) {
			if (!fgets((unsigned char *) buf, BUFSIZ - 1, xpm_file)) {
				return 0;
			}
			if (sscanf(buf, "static char * %s", what) == 1) {
				if (strlen(what) >= (unsigned) 2 && what[strlen(what) - 2] == '['
				    && what[strlen(what) - 1] == ']')
					what[strlen(what) - 2] = '\000';	/* remove "[]" */
				break;
			}
		}
		if (maxlines <= 0) {
			return 0;
		}
		/* get to the first string in the array */
		while (((c = getc(xpm_file)) != EOF) && (c != '"'));
		if (c == EOF) {
			return 0;
		}
		/* put the string in the buffer */
		for (b = 0; ((c = getc(xpm_file)) != EOF) && (c != '"') && b < (BUFSIZ - 1); b++) {
			if (c == '\\')
				c = getc(xpm_file);
			if (c == EOF) {
				return 0;
			}
			buf[b] = (char) c;
		}
		buf[b] = '\0';
		if (sscanf(buf, "%d %d %d %d", w, h, ncolors, cpp) != 4) {
			return 0;
		}
	} else if ((p = strrchr(what, '_'))) {
		/* get the name in the image if there is one */
		*p = '\0';
	}
	if (!*format || !*w || !*h || !*ncolors || !*cpp) {
		return 0;
	}
	return 1;
}

ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{
	FILE *xpm_file;
	char buf[BUFSIZ];
	char what[BUFSIZ];
	char *p;
	unsigned int value;
	unsigned int w, h;	/* image dimensions */
	unsigned int cpp;	/* chars per pixel */
	unsigned int ncolors;	/* number of colors */
	unsigned int depth;	/* depth of image */
	int format;		/* XPM format type */
	char **ctable = NULL;	/* temp color table */
	int *clookup;		/* working color table */
	int cmin, cmax;		/* min/max color string index numbers */
	ImageNode *result = NULL;
	uint32_t *color_array;
	unsigned char alpha; /* add an alpha channel to image */
	XColor xcolor;
	unsigned int a, b, x, y;
	int c;
	uint32_t *dptr;
	int colkey = XPMKEY_C;
	int gotkey = XPMKEY_NONE;
	int donecmap = 0;

	if (!(xpm_file = fopen(fileName, "rb"))) {
		fprintf(stderr,"xpixmapLoad: could not open file %s\n", fileName);
		return (NULL);
	} {
		unsigned int tw, th;
		unsigned int tcpp;
		unsigned int tncolors;
		if (!isXpixmap(xpm_file, &tw, &th, &tcpp, &tncolors, &format)) {
			fclose(xpm_file);
			return (NULL);
		}
		w = tw;
		h = th;
		cpp = tcpp;
		ncolors = tncolors;
	}

	for (depth = 1, value = 2; value < ncolors; value <<= 1, depth++);
        color_array = (uint32_t *) malloc(ncolors * sizeof(uint32_t));

//	fprintf(stderr, "%s is a %dx%d X Pixmap image (Version %d) with %d colors, of depth %d\n",
//	       fileName, w, h, format, ncolors, depth);

	/*
	 * decide which version of the xpm file to read in
	 */

	for (donecmap = 0; !donecmap;) {	/* until we have read a colormap */
		if (format == XPM_FORMAT1) {
			for (;;) {	/* keep reading lines */
				if (!fgets((unsigned char *) buf, BUFSIZ - 1, xpm_file)) {
					fprintf(stderr, "xpixmapLoad: %s - unable to find a colormap\n", fileName);
					freeCtable(ctable, ncolors);
					fclose(xpm_file);
					return NULL;
				}
				if ((sscanf(buf, "static char * %s", what) == 1) &&
				    (p = strrchr(what, '_')) && (!strcmp(p + 1, "colors[]")
					      || !strcmp(p + 1, "mono[]")
					|| !strcmp(p + 1, "pixels[]"))) {
					if (!strcmp(p + 1, "pixels[]")) {
						if (gotkey == XPMKEY_NONE) {
							fprintf(stderr, "xpixmapLoad: %s - colormap is missing\n", fileName);
							freeCtable(ctable, ncolors);
							fclose(xpm_file);
							return NULL;
						}
						donecmap = 1;
						break;
					} else if (!strcmp(p + 1, "colors[]")) {
						if (gotkey == XPMKEY_M && colkey == XPMKEY_M) {
							continue;	/* good enough already - look for pixels */
						}
						gotkey = XPMKEY_C;
						break;
					} else {	/* must be m */
						if (gotkey == XPMKEY_C && colkey != XPMKEY_M) {
							continue;	/* good enough already - look for pixels */
						}
						gotkey = XPMKEY_M;
						break;
					}
				}
			}
		}
		if (donecmap)
			break;

		/* read the colors array and build the image colormap
		 */

		ctable = (char **) malloc(sizeof(char *) * ncolors);
		bzero(ctable, sizeof(char *) * ncolors);
		xcolor.flags = DoRed | DoGreen | DoBlue;
		for (a = 0; a < ncolors; a++) {

			/* read pixel value
			 */

			*(ctable + a) = (char *) malloc(sizeof(char *) * ncolors);
			/* Find the start of the next string */
			while (((c = getc(xpm_file)) != EOF) && (c != '"'));
			if (c == EOF) {
				fprintf(stderr, "xpixmapLoad: %s - file ended in the colormap\n", fileName);
				freeCtable(ctable, ncolors);
				fclose(xpm_file);
				return NULL;
			}
			/* Read cpp characters in as the color symbol */
			for (b = 0; b < cpp; b++) {
				if ((c = getc(xpm_file)) == '\\')
					c = getc(xpm_file);
				if (c == EOF) {
					fprintf(stderr, "xpixmapLoad: %s - file ended in the colormap\n", fileName);
					freeCtable(ctable, ncolors);
					fclose(xpm_file);
					return NULL;
				}
				*(*(ctable + a) + b) = (char) c;
			}

			/* Locate the end of this string */
			if (format == XPM_FORMAT1) {
				if (((c = getc(xpm_file)) == EOF) || (c != '"')) {
					fprintf(stderr, "xpixmapLoad: %s - file ended in the colormap\n", fileName);
					freeCtable(ctable, ncolors);
					fclose(xpm_file);
					return NULL;
				}
			}
			/* read color definition and parse it
			 */

			/* Locate the start of the next string */
			if (format == XPM_FORMAT1) {
				while (((c = getc(xpm_file)) != EOF) && (c != '"'));
				if (c == EOF) {
					fprintf(stderr, "xpixmapLoad: %s - file ended in the colormap\n", fileName);
					freeCtable(ctable, ncolors);
					fclose(xpm_file);
					return NULL;
				}
			}
			/* load the rest of this string into the buffer */
			for (b = 0; ((c = getc(xpm_file)) != EOF) && (c != '"') && b < (BUFSIZ - 1); b++) {
				if (c == '\\')
					c = getc(xpm_file);
				if (c == EOF) {
					fprintf(stderr, "xpixmapLoad: %s - file ended in the colormap\n", fileName);
					freeCtable(ctable, ncolors);
					fclose(xpm_file);
					return NULL;
				}
				buf[b] = (char) c;
			}
			buf[b] = '\0';

			/* locate the colour to use */
			if (format != XPM_FORMAT1) {
				for (p = buf; p != NULL;) {
					if ((p = nword(p, what)) != NULL
					    && !strcmp(what, xpmkeys[colkey])) {
						if (nword(p, what)) {
							p = what;
							break;	/* found a color definition */
						}
					}
				}
				if (p == NULL) {	/* failed to find that color key type */
					for (b = XPMKEY_C; b >= XPMKEY_M && p == NULL; b--) {	/* try all the rest */
						for (p = buf; p != NULL;) {
							if ((p = nword(p, what)) != NULL
							    && !strcmp(what, xpmkeys[b])) {
								if (nword(p, what)) {
									p = what;
									fprintf(stderr, "xpixmapLoad: %s - couldn't find color key '%s', switching to '%s'\n",
										fileName, xpmkeys[colkey], xpmkeys[b]);
									colkey = b;
									break;	/* found a color definition */
								}
							}
						}
					}
				}
				if (p == NULL) {
					fprintf(stderr, "xpixmapLoad: %s - file is corrupted\n", fileName);
					freeCtable(ctable, ncolors);
					free(color_array);
					fclose(xpm_file);
					return NULL;
				}
				donecmap = 1;	/* There is only 1 color map for new xpm files */
			} else
				p = buf;

			alpha = 0xff;
			if( strcmp(p, "None") == 0 ) {
					xcolor.red = xcolor.green = xcolor.blue = 0;
					alpha = 0;
			} else if (!XParseColor(display, rootColormap, p, &xcolor)) {
				fprintf(stderr, "xpixmapLoad: %s - Bad color name '%s'\n", fileName, p);
				xcolor.red = xcolor.green = xcolor.blue = 0;
				alpha = 0;
			}
			/* Why is this the right one? I don't know :( */
			color_array[a] = alpha <<24 |(xcolor.blue >> 8) << 16 | (xcolor.green >> 8) << 8 | (xcolor.red >> 8);
		}
	}

	/* convert the linear search color table into a lookup table
	 * for better speed (but could use a lot of memory!).
	 */

	cmin = 1 << (sizeof(int) * 8 - 2);	/* Hmm. should use maxint */
	cmax = 0;
	for (a = 0; a < ncolors; a++) {
		int val;
		for (b = 0, val = 0; b < cpp; b++)
			val = val * 96 + ctable[a][b] - 32;
		if (val < cmin)
			cmin = val;
		if (val > cmax)
			cmax = val;
	}
	cmax++;			/* point one past cmax */
	clookup = (int *) malloc(sizeof(int) * cmax - cmin);
	bzero(clookup, sizeof(int) * cmax - cmin);
	for (a = 0; a < ncolors; a++) {
		int val;
		for (b = 0, val = 0; b < cpp; b++)
			val = val * 96 + ctable[a][b] - 32;
		clookup[val - cmin] = a;
	}
	freeCtable(ctable, ncolors);

	/*
	 * Now look for the pixel array
	 */

	/* read in image data
	 */

	result = CreateImage(w, h, 0);

	dptr = (uint32_t *) result->data;

	for (y = 0; y < h; y++) {
		while (((c = getc(xpm_file)) != EOF) && (c != '"'));
		for (x = 0; x < w; x++) {
			int val;
			for (b = 0, val = 0; b < cpp; b++) {
				if ((c = getc(xpm_file)) == '\\')
					c = getc(xpm_file);
				if (c == EOF) {
					fprintf(stderr, "xpixmapLoad: %s - Short read of X Pixmap\n", fileName);
					fclose(xpm_file);
					free(clookup);
					free(color_array);
					return (result);
				}
				val = val * 96 + (int) c - 32;
			}
			if (val < cmin || val >= cmax) {
				fprintf(stderr, "xpixmapLoad: %s - Pixel data doesn't match color data\n", fileName);
				a = 0;
			}
			a = clookup[val - cmin]; /* a is the index of color_array */
			dptr[y*w+x] = color_array[a];
		}
		if ((c = getc(xpm_file)) != '"') {
			fprintf(stderr, "xpixmapLoad: %s - Short read of X Pixmap\n", fileName);
			free(clookup);
			free(color_array);
			fclose(xpm_file);
			return (result);
		}
	}
	free(clookup);	
	free(color_array);
	fclose(xpm_file);
        swap_channels(result);
	return (result);
}

/** Load an XBM image from the specified file. */
#ifdef USE_XBM
ImageNode *LoadXBMImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{
   ImageNode *result = NULL;
   unsigned char *data;
   unsigned width, height;
   int xhot, yhot;
   int rc;

   rc = XReadBitmapFileData(fileName, &width, &height, &data, &xhot, &yhot);
   if(rc == BitmapSuccess) {
      result = CreateImage(width, height, 1);
      memcpy(result->data, data, (width * height + 7) / 8);
      XFree(data);
   }

   return result;
}
#endif /* USE_XBM */

/** Create an image from XImages giving color and shape information. */
#define HASH_SIZE 16
ImageNode *CreateImageFromXImages(XImage *image, XImage *shape)
{
   XColor colors[HASH_SIZE];
   ImageNode *result;
   unsigned char *dest;
   int x, y;

   memset(colors, 0xFF, sizeof(colors));
   result = CreateImage(image->width, image->height, 0);
   dest = result->data;
   for(y = 0; y < image->height; y++) {
      for(x = 0; x < image->width; x++) {
         const unsigned long pixel = XGetPixel(image, x, y);
         *dest++ = (!shape || XGetPixel(shape, x, y)) ? 255 : 0;
         if(image->depth == 1) {
            const unsigned char value = pixel ? 0 : 255;
            *dest++ = value;
            *dest++ = value;
            *dest++ = value;
         } else{
            const unsigned index = pixel % HASH_SIZE;
            if(colors[index].pixel != pixel) {
               colors[index].pixel = pixel;
               JXQueryColor(display, rootColormap, &colors[index]);
            }
            *dest++ = (unsigned char)(colors[index].red   >> 8);
            *dest++ = (unsigned char)(colors[index].green >> 8);
            *dest++ = (unsigned char)(colors[index].blue  >> 8);
         }
      }
   }

   return result;
}
#undef HASH_SIZE

ImageNode *CreateImage(unsigned width, unsigned height, char bitmap)
{
   unsigned image_size;
   if(bitmap) {
      image_size = (width * height + 7) / 8;
   } else {
      image_size = 4 * width * height;
   }
   ImageNode *image = Allocate(sizeof(ImageNode));
   image->data = Allocate(image_size);
   image->next = NULL;
   image->bitmap = bitmap;
   image->width = width;
   image->height = height;
#ifdef USE_XRENDER
   image->render = haveRender;
#endif
   return image;
}

/** Destroy an image node. */
void DestroyImage(ImageNode *image) {
   while(image) {
      ImageNode *next = image->next;
      if(image->data) {
         Release(image->data);
      }
      Release(image);
      image = next;
   }
}
