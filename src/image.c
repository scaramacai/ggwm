/**
 * @file image.c
 * @author Joe Wingbermuehle
 * @date 2005-2007
 *
 * @brief Functions to load images.
 *
 */

#include "jwm.h"

#ifndef MAKE_DEPEND

#  ifdef USE_XPM
#     include <X11/xpm.h>
#  endif
#  ifdef USE_CAIRO
#     include <cairo.h>
#     include <cairo-svg.h>
#  endif
#  ifdef USE_RSVG
#     include <librsvg/rsvg.h>
#  endif
#endif /* MAKE_DEPEND */

#include "image.h"
#include "main.h"
#include "error.h"
#include "color.h"
#include "misc.h"


typedef ImageNode *(*ImageLoader)(const char *fileName,
                                  int rwidth, int rheight,
                                  char preserveAspect);

#if defined(USE_PNG) || defined(USE_JPEG)
/* include stb_image.h */
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#ifdef USE_JPEG
#define STBI_ONLY_JPEG
#endif
#ifdef USE_PNG
#define STBI_ONLY_PNG
#endif
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

static ImageNode *LoadSTBImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif /* USE_PNG || USE_JPEG */

#ifdef USE_CAIRO
#ifdef USE_RSVG
static ImageNode *LoadSVGImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif
#endif

#ifdef USE_XPM
static ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif
#ifdef USE_XBM
static ImageNode *LoadXBMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif
#ifdef USE_ICONS
static ImageNode *CreateImageFromXImages(XImage *image, XImage *shape);
#endif

#ifdef USE_XPM
static int AllocateColor(Display *d, Colormap cmap, char *name,
                         XColor *c, void *closure);
static int FreeColors(Display *d, Colormap cmap, Pixel *pixels, int n,
                      void *closure);
#endif

/* File extension to image loader mapping. */
static const struct {
   const char *extension;
   ImageLoader loader;
} IMAGE_LOADERS[] = {
#ifdef USE_PNG
   {".png",       LoadSTBImage      },
#endif
#ifdef USE_JPEG
   {".jpg",       LoadSTBImage     },
   {".jpeg",      LoadSTBImage     },
#endif
#ifdef USE_CAIRO
#ifdef USE_RSVG
   {".svg",       LoadSVGImage      },
#endif
#endif
#ifdef USE_XPM
   {".xpm",       LoadXPMImage      },
#endif
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
#ifdef USE_ICONS
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
#endif

/* Load a (PNG, JPEG) image from the given file name, using the stb_image.h library. */
#if defined(USE_PNG) || defined(USE_JPEG)
ImageNode *LoadSTBImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{

   static ImageNode *result;
   int iw, ih;
   int img_channels;
   unsigned char *img_data = NULL;

   /* These should be uint32_t */
   unsigned  *rgba_pointer;
   unsigned  *argb_pointer;

   unsigned int width;
   unsigned int height;

   unsigned long number_of_pixels;

   unsigned long i;

   Assert(fileName);


   result = NULL;

   /* use stbi to load the image and always create 4 interleaved channels (RGBA) */
   if (!(img_data = stbi_load(fileName, &iw, &ih, &img_channels, 4 )))  {
      Warning(_("could not decode image: %s"), fileName);
      return NULL;
   }

   width = (unsigned int) iw;
   height = (unsigned int) ih;

   result = CreateImage(width, height, 0);

  /* It looks like jwm uses argb hence we need to do so */

   number_of_pixels = (unsigned long) width * height;

   rgba_pointer = (unsigned  *) img_data;
   argb_pointer = (unsigned  *) result->data;

   for(i=0;i < number_of_pixels; i++)
    //  argb_pointer[i] = ((rgba_pointer[i] << 24) | (rgba_pointer[i] >> 8));
    //  depends on big little endian?
    //  I cannot understand the line below but it works
      argb_pointer[i] = ((rgba_pointer[i] << 8) | (rgba_pointer[i] >> 24));

   /* end of alpha swap ------------------------------------------ */
   
   free(img_data);

   return result;

}

#endif /* USE_PNG || USE_JPEG */

#ifdef USE_CAIRO
#ifdef USE_RSVG
ImageNode *LoadSVGImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{

#if !GLIB_CHECK_VERSION(2, 35, 0)
   static char initialized = 0;
#endif
   ImageNode *result = NULL;
   RsvgHandle *rh;
   RsvgDimensionData dim;
   GError *e;
   cairo_surface_t *target;
   cairo_t *context;
   int stride;
   int i;
   float xscale, yscale;

   Assert(fileName);

#if !GLIB_CHECK_VERSION(2, 35, 0)
   if(!initialized) {
      initialized = 1;
      g_type_init();
   }
#endif

   /* Load the image from the file. */
   e = NULL;
   rh = rsvg_handle_new_from_file(fileName, &e);
   if(!rh) {
      g_error_free(e);
      return NULL;
   }

   rsvg_handle_get_dimensions(rh, &dim);
   if(rwidth == 0 || rheight == 0) {
      rwidth = dim.width;
      rheight = dim.height;
      xscale = 1.0;
      yscale = 1.0;
   } else if(preserveAspect) {
      if(abs(dim.width - rwidth) < abs(dim.height - rheight)) {
         xscale = (float)rwidth / dim.width;
         rheight = dim.height * xscale;
      } else {
         xscale = (float)rheight / dim.height;
         rwidth = dim.width * xscale;
      }
      yscale = xscale;
   } else {
      xscale = (float)rwidth / dim.width;
      yscale = (float)rheight / dim.height;
   }

   result = CreateImage(rwidth, rheight, 0);
   memset(result->data, 0, rwidth * rheight * 4);

   /* Create the target surface. */
   stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, rwidth);
   target = cairo_image_surface_create_for_data(result->data,
                                                CAIRO_FORMAT_ARGB32,
                                                rwidth, rheight, stride);
   context = cairo_create(target);
   cairo_scale(context, xscale, yscale);
   cairo_paint_with_alpha(context, 0.0);
   rsvg_handle_render_cairo(rh, context);
   cairo_destroy(context);
   cairo_surface_destroy(target);
   g_object_unref(rh);

   for(i = 0; i < 4 * rwidth * rheight; i += 4) {
      const unsigned int temp = *(unsigned int*)&result->data[i];
      const unsigned int alpha  = (temp >> 24) & 0xFF;
      const unsigned int red    = (temp >> 16) & 0xFF;
      const unsigned int green  = (temp >>  8) & 0xFF;
      const unsigned int blue   = (temp >>  0) & 0xFF;
      result->data[i + 0] = alpha;
      if(alpha > 0) {
         result->data[i + 1] = (red * 255) / alpha;
         result->data[i + 2] = (green * 255) / alpha;
         result->data[i + 3] = (blue * 255) / alpha;
      }
   }

   return result;

}
#endif /* USE_RSVG */
#endif /* USE_CAIRO */

/** Load an XPM image from the specified file. */
#ifdef USE_XPM
ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
                        char preserveAspect)
{

   ImageNode *result = NULL;

   XpmAttributes attr;
   XImage *image;
   XImage *shape;
   int rc;

   Assert(fileName);

   attr.valuemask = XpmAllocColor | XpmFreeColors | XpmColorClosure;
   attr.alloc_color = AllocateColor;
   attr.free_colors = FreeColors;
   attr.color_closure = NULL;
   rc = XpmReadFileToImage(display, (char*)fileName, &image, &shape, &attr);
   if(rc == XpmSuccess) {
      result = CreateImageFromXImages(image, shape);
      JXDestroyImage(image);
      if(shape) {
         JXDestroyImage(shape);
      }
   }

   return result;

}
#endif /* USE_XPM */

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
#ifdef USE_ICONS
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
#endif /* USE_ICONS */

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

/** Callback to allocate a color for libxpm. */
#ifdef USE_XPM
int AllocateColor(Display *d, Colormap cmap, char *name,
                  XColor *c, void *closure)
{
   if(name) {
      if(!JXParseColor(d, cmap, name, c)) {
         return -1;
      }
   }

   GetColor(c);
   return 1;
}
#endif /* USE_XPM */

/** Callback to free colors allocated by libxpm.
 * We don't need to do anything here since color.c takes care of this.
 */
#ifdef USE_XPM
int FreeColors(Display *d, Colormap cmap, Pixel *pixels, int n,
               void *closure)
{
   return 1;
}
#endif /* USE_XPM */
