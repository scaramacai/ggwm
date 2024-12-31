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
 

#include "jwm.h"

#ifndef MAKE_DEPEND

#  ifdef USE_XPM
#     include <X11/xpm.h>
#  endif
#endif /* MAKE_DEPEND */

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

#ifdef USE_XPM
static ImageNode *LoadXPMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif
#ifdef USE_XBM
static ImageNode *LoadXBMImage(const char *fileName, int rwidth, int rheight,
                               char preserveAspect);
#endif

static ImageNode *CreateImageFromXImages(XImage *image, XImage *shape);

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
   {".svg",       LoadNSVGImage      },
#ifdef USE_PNG
   {".png",       LoadSTBImage      },
#endif
#ifdef USE_JPEG
   {".jpg",       LoadSTBImage     },
   {".jpeg",      LoadSTBImage     },
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
#if defined(USE_PNG) || defined(USE_JPEG)
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


#endif /* USE_PNG || USE_JPEG */

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
