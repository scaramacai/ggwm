/**
 * @file font.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle fonts.
 *
 */

#include "jwm.h"
#include "font.h"
#include "main.h"
#include "error.h"
#include "misc.h"
#include "schrift_x11.h"
#include "sds.h"

#ifdef USE_ICONV
#  ifdef HAVE_LANGINFO_H
#     include <langinfo.h>
#  endif
#  ifdef HAVE_ICONV_H
#     include <iconv.h>
#  endif
#endif
   SFT_X * sft_x1 = NULL;
   SFT_X * sft_x3 = NULL;

#ifdef USE_XRENDER
static const char *DEFAULT_FONT = "alegreya-sans-v25-latin_latin-ext-regular";
static const double DEFAULT_SIZE = 13.0;
static const char *SIZE_SEPARATOR = "::"; //It is assumed that the file name itself does not contain the SIZE_SEPARATOR substring!
static const char * const FONT_DIRS_HOME[] = {
    ".config/ggwm/fonts/",
    ".local/share/fonts/",
    NULL,
};
static const char * const FONT_DIRS_XDG[] = {
    "ggwm/fonts/",
    NULL,
};
static const char * const FONT_DIRS_SYS[] = {
    "/usr/share/fonts/", 
    "/usr/share/fonts/truetype/", 
    "/usr/local/share/fonts/truetype/", 
    NULL,
};

typedef struct _NameSize {
    sds name;
    double size;
} NameSize;

static unsigned int get_font_dirs_dim(const char * const * dir)
{
  unsigned int dim;
  
  dim = 0;
  while(dir[dim]) dim++;
fprintf(stderr, "Dimensione array: %d\n", dim);
  return dim;
}
//static sds *font_expanded_paths(const char * const * dir)
static sds *font_expanded_paths(void)
{
  unsigned int i;
  unsigned int j;
  unsigned int dim_home;
  unsigned int dim_xdg;
  unsigned int dim_sys;
  unsigned int dim = 0;
  sds * result = NULL;
  char * path = NULL;
  sds home = NULL;
  sds xdg = NULL;

  fprintf(stderr,"FONTS_DIR HOME  ");
  dim_home = get_font_dirs_dim(FONT_DIRS_HOME);
  fprintf(stderr,"FONTS_DIR XDG  ");
  dim_xdg = get_font_dirs_dim(FONT_DIRS_XDG);
  fprintf(stderr,"FONTS_DIR SYS  ");
  dim_sys = get_font_dirs_dim(FONT_DIRS_SYS);

  if(getenv("HOME")) {
     home = sdsnew(getenv("HOME"));
     dim += dim_home;
  fprintf(stderr,"$HOME presente! HOME=%s\n", home);
  }

  if(getenv("XDG_CONFIG_HOME")) {
     xdg = sdsnew(getenv("XDG_CONFIG_HOME"));
     dim += dim_xdg;
  fprintf(stderr,"$XDG_CONFIG_HOME presente! XDG_CONFIG_HOME=%s\n", home);
  }
  dim += dim_sys;
  fprintf(stderr,"Dimensioni totali array result: %d\n", dim);

  result = (sds *) malloc((dim+1) * sizeof(sds));
  result[dim] = NULL;
  fprintf(stderr,"Result ha dimensioni %ld\n", (dim * sizeof(sds)));

//  for (i = 0; i < dim_home; i++) {
  j = 0;
  if(home) {
     i = 0;
     while(FONT_DIRS_HOME[i]) {
  fprintf(stderr,"In font_expanded_path: FONT_DIRS_HOME[%d] = %s\n", i, FONT_DIRS_HOME[i]); fflush(stderr);
        result[j] = sdsdup(home);
        result[j] = sdscatfmt(result[j], "%s%s","/",FONT_DIRS_HOME[i]);
  fprintf(stderr,"In font_expanded_path: result[%d] = %s\n", j, result[j]); fflush(stderr);
//     free(path);
  fprintf(stderr,"In font_expanded_path FONT_DIRS_HOME: iterazione %d\n", i); fflush(stderr);
        i++;
        j++;
     }
     sdsfree(home);   
  }   
  if(xdg) {
     i = 0;
     while(FONT_DIRS_XDG[i]) {
  fprintf(stderr,"In font_expanded_path: FONT_DIRS_XDG[%d] = %s\n", i, FONT_DIRS_XDG[i]); fflush(stderr);
        result[j] = sdsdup(home);
        result[j] = sdscatfmt(result[j], "%s%s","/",FONT_DIRS_XDG[i]);
  fprintf(stderr,"In font_expanded_path: result[%d] = %s\n", j, result[j]); fflush(stderr);
//     free(path);
  fprintf(stderr,"In font_expanded_path FONT_DIRS_XDG: iterazione %d\n", i); fflush(stderr);
        i++;
        j++;
     }
     sdsfree(xdg);   
  }   
  i = 0;
  while(FONT_DIRS_SYS[i]) {
  fprintf(stderr,"In font_expanded_path: FONT_DIRS_SYS[%d] = %s\n", i, FONT_DIRS_SYS[i]); fflush(stderr);
     result[j] = sdsnew(FONT_DIRS_SYS[i]);
  fprintf(stderr,"In font_expanded_path: result[%d] = %s\n", j, result[j]); fflush(stderr);
//     free(path);
  fprintf(stderr,"In font_expanded_path FONT_DIRS_SYS: iterazione %d\n", i); fflush(stderr);
     i++;
     j++;
  }


  return result;
}

static void free_expanded_paths(sds *path)
{
  unsigned int i = 0;
  while(path[i++]) sdsfree(path[i]);
  free(path);
}
  
static NameSize * get_font_name_size(char * name)
{
   NameSize * name_size = NULL;
   sds *splitted = NULL;
   double size;
   int count;
   if (!name) return NULL;
   splitted = sdssplitlen(name, strlen(name), SIZE_SEPARATOR, strlen(SIZE_SEPARATOR), &count);
   if (count == 1) {
      size = DEFAULT_SIZE;
   } else {
      size = atof(splitted[1]);
   }
   splitted[0] = sdscat(splitted[0],".ttf");

   name_size = (NameSize *) malloc(sizeof(NameSize));
   name_size->name = sdsdup((const sds) splitted[0]);
   name_size->size = size;
   sdsfreesplitres(splitted, count);
   fprintf(stderr,"Requested font %s at size %g\n", name_size->name, name_size->size); fflush(stderr);
   return name_size;
}

static void FreeNameSize(NameSize * ns)
{
   if(!ns) return;
   sdsfree(ns->name);
   free(ns);
   ns = NULL;
}

static SFT_X * search_font_in_default_paths(sds name, double size, sds * paths)
{
  unsigned int i;
  sds path;
  SFT_X * font = NULL;
  i = 0;
  while(paths[i]) {
    path = sdsdup(paths[i]);
    path = sdscatsds(path, name);
    font = SFT_X_create_from_file(path, size, 1.0);
    sdsfree(path);
    if(font) break; 
    i++;
  }
  return font;
}
  
#else /* no USE_XRENDER */
static const char *DEFAULT_FONT = "fixed";
#endif

static const struct {
   const FontType src;
   const FontType dest;
} INHERITED_FONTS[] = {
   { FONT_TRAY, FONT_PAGER       },
   { FONT_TRAY, FONT_CLOCK       },
   { FONT_TRAY, FONT_TASKLIST    },
   { FONT_TRAY, FONT_TRAYBUTTON  }
};

static char *GetUTF8String(const char *str);
static void ReleaseUTF8String(char *utf8String);

static char *fontNames[FONT_COUNT];

#ifdef USE_ICONV
static const char *UTF8_CODESET = "UTF-8";
static iconv_t fromUTF8 = (iconv_t)-1;
static iconv_t toUTF8 = (iconv_t)-1;
#endif

#ifdef USE_XRENDER
static SFT_X *fonts[FONT_COUNT];
#else
static XFontStruct *fonts[FONT_COUNT];
#endif

/** Initialize font data. */
void InitializeFonts(void)
{
#ifdef USE_ICONV
   const char *codeset;
#endif
   unsigned int x;

   for(x = 0; x < FONT_COUNT; x++) {
      fonts[x] = NULL;
      fontNames[x] = NULL;
   }

   /* Allocate a conversion descriptor if we're not using UTF-8. */
#ifdef USE_ICONV
   codeset = nl_langinfo(CODESET);
   if(strcmp(codeset, UTF8_CODESET)) {
      toUTF8 = iconv_open(UTF8_CODESET, codeset);
      fromUTF8 = iconv_open(codeset, UTF8_CODESET);
   } else {
      toUTF8 = (iconv_t)-1;
      fromUTF8 = (iconv_t)-1;
   }
#endif

}

/** Startup font support. */
void StartupFonts(void)
{
   unsigned int x;

   /* Inherit unset fonts from the tray for tray items. */
   for(x = 0; x < ARRAY_LENGTH(INHERITED_FONTS); x++) {
      const FontType dest = INHERITED_FONTS[x].dest;
      if(!fontNames[dest]) {
         const FontType src = INHERITED_FONTS[x].src;
         fontNames[dest] = CopyString(fontNames[src]);
      }
   }

#ifdef USE_XRENDER

fprintf(stderr,"In StartupFonts\n"); fflush(stderr);
   sds * expanded_paths = NULL;
   sds default_name = NULL;
   unsigned int i;
fprintf(stderr,"Expanding dir paths\n"); fflush(stderr);
   expanded_paths = font_expanded_paths();
fprintf(stderr,"Printing dir paths\n"); fflush(stderr);
   i = 0;
   while(expanded_paths[i]) {
   fprintf(stderr, "expanded_paths[%d]: %s\n",i,expanded_paths[i]);
      i++;
   }
fprintf(stderr,"End Printing dir paths\n"); fflush(stderr);
// This is only for testing
fprintf(stderr,"FONT_COUNT %d\n", FONT_COUNT);
  SetFont( 1,  "sansita-v11-latin-regular::18");
//   fontNames[1] = "chivo-v18-latin_latin-ext-500::12";
//   fontNames[2] = "jetbrains-mono/JetBrainsMono-Medium::11";
//   fontNames[3] = "/usr/share/fonts/jetbrains-mono/JetBrainsMono-Medium::10";
////////////////////////////////////////////////////
   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
fprintf(stderr,"Got font name %d: %s\n", x, fontNames[x]); fflush(stderr);
         NameSize * ns = get_font_name_size(fontNames[x]);
fprintf(stderr,"Need font name: %s at size %g\n", ns->name, ns->size); fflush(stderr);
         if(ns->name[0] == '/') {  //We assume this is an absolute path
            fonts[x] = SFT_X_create_from_file(ns->name, ns->size, 1.0);
         } else { //search in FONT_DIRS
            fonts[x] = search_font_in_default_paths(ns->name, ns->size, expanded_paths);
         } 
         if(JUNLIKELY(!fonts[x])) {
            fprintf(stderr, "Warning: could not load font: %s\n", fontNames[x]);
         }
         if(!fonts[x]) {
            fprintf(stderr, "%s: replacing with default font %s at size %g\n", fontNames[x], DEFAULT_FONT, ns->size);
            default_name = sdsnew(DEFAULT_FONT);
            default_name = sdscat(default_name,".ttf");
            if(DEFAULT_FONT[0] == '/') {  //We assume this is an absolute path
               fonts[x] = SFT_X_create_from_file(DEFAULT_FONT, ns->size, 1.0);
            } else { //search in FONT_DIRS
               fonts[x] = search_font_in_default_paths(default_name, ns->size, expanded_paths);
            } 
            sdsfree(default_name);
        } 
        FreeNameSize(ns);   
        if(JUNLIKELY(!fonts[x])) {
           fprintf(stderr,"could not load the default font: %s\n", DEFAULT_FONT);
        }
        
      }
   }
   free_expanded_paths(expanded_paths);

   sft_x1 = SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/chivo-v18-latin_latin-ext-500.ttf",14.0,1.0); 
   sft_x3 = SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/chivo-v18-latin_latin-ext-500.ttf",12.0,1.0); 
}
#else /* no USE_XRENDER */

   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         fonts[x] = JXLoadQueryFont(display, fontNames[x]);
         if(JUNLIKELY(!fonts[x] && fontNames[x])) {
            Warning(_("could not load font: %s"), fontNames[x]);
         }
      }
      if(!fonts[x]) {
         fonts[x] = JXLoadQueryFont(display, DEFAULT_FONT);
      }
      if(JUNLIKELY(!fonts[x])) {
         FatalError(_("could not load the default font: %s"), DEFAULT_FONT);
      }
   }

#endif /* no USE_XRENDER */


/** Shutdown font support. */
void ShutdownFonts(void)
{
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
      if(fonts[x]) {
#ifdef USE_XRENDER
         SFT_X_free(fonts[x]);
#else
         JXFreeFont(display, fonts[x]);
#endif
         fonts[x] = NULL;
      }
   }
}

/** Destroy font data. */
void DestroyFonts(void)
{
   unsigned int x;
   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         Release(fontNames[x]);
         fontNames[x] = NULL;
      }
   }
#ifdef USE_ICONV
   if(fromUTF8 != (iconv_t)-1) {
      iconv_close(fromUTF8);
      fromUTF8 = (iconv_t)-1;
   }
   if(toUTF8 != (iconv_t)-1) {
      iconv_close(toUTF8);
      toUTF8 = (iconv_t)-1;
   }
#endif
}

/** Convert a string from UTF-8. */
char *ConvertFromUTF8(char *str)
{
   char *result = (char*)str;
#ifdef USE_ICONV
   if(fromUTF8 != (iconv_t)-1) {
      ICONV_CONST char *inBuf = (ICONV_CONST char*)str;
      char *outBuf;
      size_t inLeft = strlen(str);
      size_t outLeft = 4 * inLeft;
      size_t rc;
      result = Allocate(outLeft + 1);
      outBuf = result;
      rc = iconv(fromUTF8, &inBuf, &inLeft, &outBuf, &outLeft);
      if(rc == (size_t)-1) {
         Warning("iconv conversion from UTF-8 failed");
         iconv_close(fromUTF8);
         fromUTF8 = (iconv_t)-1;
         Release(result);
         result = (char*)str;
      } else {
         Release(str);
         *outBuf = 0;
      }
   }
#endif
   return result;
}

/** Convert a string to UTF-8. */
char *GetUTF8String(const char *str)
{
   char *utf8String = (char*)str;
#ifdef USE_ICONV
   if(toUTF8 != (iconv_t)-1) {
      ICONV_CONST char *inBuf = (ICONV_CONST char*)str;
      char *outBuf;
      size_t inLeft = strlen(str);
      size_t outLeft = 4 * inLeft;
      size_t rc;
      utf8String = Allocate(outLeft + 1);
      outBuf = utf8String;
      rc = iconv(toUTF8, &inBuf, &inLeft, &outBuf, &outLeft);
      if(rc == (size_t)-1) {
         Warning("iconv conversion to UTF-8 failed");
         iconv_close(toUTF8);
         toUTF8 = (iconv_t)-1;
         Release(utf8String);
         utf8String = (char*)str;
      } else {
         *outBuf = 0;
      }
   }
#endif
   return utf8String;
}

/** Release a UTF-8 string. */
void ReleaseUTF8String(char *utf8String)
{
#ifdef USE_ICONV
   if(toUTF8 != (iconv_t)-1) {
      Release(utf8String);
   }
#endif
}

/** Get the width of a string. */
int GetStringWidth(FontType ft, const char *str)
{
#ifdef USE_XRENDER
int sft_w;
sft_w =  SFT_X_get_string_width(fonts[ft], str);
return sft_w;
   XGlyphInfo extents;
#endif
#ifdef USE_FRIBIDI
   FriBidiChar *temp_i;
   FriBidiChar *temp_o;
   FriBidiParType type = FRIBIDI_PAR_ON;
   int unicodeLength;
#endif
   int len;
   char *output;
   int result;
   char *utf8String;

   /* Convert to UTF-8 if necessary. */
   utf8String = GetUTF8String(str);

   /* Length of the UTF-8 string. */
   len = strlen(utf8String);

   /* Apply the bidi algorithm if requested. */
#ifdef USE_FRIBIDI
   temp_i = AllocateStack((len + 1) * sizeof(FriBidiChar));
   temp_o = AllocateStack((len + 1) * sizeof(FriBidiChar));
   unicodeLength = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8,
                                              utf8String, len, temp_i);
   fribidi_log2vis(temp_i, unicodeLength, &type, temp_o, NULL, NULL, NULL);
   output = AllocateStack(4 * len + 1);
   fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, temp_o, unicodeLength,
                              (char*)output);
   len = strlen(output);
#else
   output = utf8String;
#endif

   /* Get the width of the string. */
#ifdef USE_XRENDER
   result = extents.xOff;
#else
   result = XTextWidth(fonts[ft], output, len);
#endif

   /* Clean up. */
#ifdef USE_FRIBIDI
   ReleaseStack(temp_i);
   ReleaseStack(temp_o);
   ReleaseStack(output);
#endif
   ReleaseUTF8String(utf8String);

   return result;
}

/** Get the height of a string. */
int GetStringHeight(FontType ft)
{
   Assert(fonts[ft]);
   return fonts[ft]->ascent + fonts[ft]->descent;
}

/** Set the font to use for a component. */
void SetFont(FontType type, const char *value)
{
   if(JUNLIKELY(!value)) {
      Warning(_("empty Font tag"));
      return;
   }
   if(fontNames[type]) {
      Release(fontNames[type]);
   }
   fontNames[type] = CopyString(value);
}

/** Display a string. */
void RenderString(Drawable d, FontType font, ColorType color,
                  int x, int y, int width, const char *str)
{
//return;

   XRectangle rect;
   Region renderRegion;
   int len;
   char *output; 
#ifdef USE_FRIBIDI
   FriBidiChar *temp_i;
   FriBidiChar *temp_o;
   FriBidiParType type = FRIBIDI_PAR_ON;
   int unicodeLength;
#endif
//#ifdef USE_XRENDER
//   XftDraw *xd;
//   XGlyphInfo extents;
//#else
   XGCValues gcValues;
   unsigned long gcMask;
   GC gc;
//#endif
   char *utf8String;

   /* Early return for empty strings. */
   if(!str || !str[0]) {
      return;
   }

   /* Convert to UTF-8 if necessary. */
   utf8String = GetUTF8String(str);

   /* Get the length of the UTF-8 string. */
   len = strlen(utf8String);

//#ifdef USE_XRENDER
//   xd = XftDrawCreate(display, d, rootVisual, rootColormap);
//#else
   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   gc = JXCreateGC(display, d, gcMask, &gcValues);
//#endif


   /* Apply the bidi algorithm if requested. */
#ifdef USE_FRIBIDI
   temp_i = AllocateStack((len + 1) * sizeof(FriBidiChar));
   temp_o = AllocateStack((len + 1) * sizeof(FriBidiChar));
   unicodeLength = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8,
                                              utf8String, len, temp_i);
   fribidi_log2vis(temp_i, unicodeLength, &type, temp_o, NULL, NULL, NULL);
   output = AllocateStack(4 * len + 1);
   fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, temp_o, unicodeLength,
                              (char*)output);
   len = strlen(output);
#else
   output = utf8String;
#endif

   /* Get the bounds for the string based on the specified width. */
   rect.x = x;
   rect.y = y;
   rect.height = GetStringHeight(font);
//#ifdef USE_XRENDER
//   JXftTextExtentsUtf8(display, fonts[font], (const unsigned char*)output,
//                       len, &extents);
//   rect.width = extents.xOff;
//#else
//   rect.width = XTextWidth(fonts[font], output, len);
//#endif
rect.width =  SFT_X_get_string_width(fonts[font], str);
   rect.width = Min(rect.width, width) + 2;

   /* Combine the width bounds with the region to use. */
   renderRegion = XCreateRegion();
   XUnionRectWithRegion(&rect, renderRegion, renderRegion);

   /* Display the string. */
//#ifdef USE_XRENDER
//   JXftDrawSetClip(xd, renderRegion);
//   JXftDrawStringUtf8(xd, GetXftColor(color), fonts[font],
//                      x, y + fonts[font]->ascent,
//                      (const unsigned char*)output, len);
//   JXftDrawChange(xd, rootWindow);
//#else
   JXSetForeground(display, gc, colors[color]);
   JXSetRegion(display, gc, renderRegion);
XRenderColor * sft_color = GetXRenderColor(color);
fprintf(stderr,"Drawing string %s at %d,%d with font %s type %d\n", str,x,y, fonts[font]->filename,font);
if(font > 3) font=3;
if(font == 1)
SFT_X_draw_string32(display, d, x, y,  sft_color, sft_x1, str, width);
else if(font==3)
SFT_X_draw_string32(display, d, x, y,  sft_color, sft_x3, str, width);
else
SFT_X_draw_string32(display, d, x, y,  sft_color, fonts[font], str, width);
free(sft_color);
//   JXSetFont(display, gc, fonts[font]->fid);
//   JXDrawString(display, d, gc, x, y + fonts[font]->ascent, output, len);
//#endif

   /* Free any memory used for UTF conversion. */
#ifdef USE_FRIBIDI
   ReleaseStack(temp_i);
   ReleaseStack(temp_o);
   ReleaseStack(output);
#endif
   ReleaseUTF8String(utf8String);

   XDestroyRegion(renderRegion);

//#ifdef USE_XRENDER
//   XftDrawDestroy(xd);
//#else
   JXFreeGC(display, gc);
//#endif

}
