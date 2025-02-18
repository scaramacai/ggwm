/**
 * @file font.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 * @author Scaramacai
 * @date 2024-2025
 *
 * @brief Functions to handle fonts.
 *
 */

#include "ggwm.h"
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

#ifdef USE_XRENDER
static const char *DEFAULT_FONT = "alegreya-sans-v25-latin_latin-ext-regular";
static const double DEFAULT_SIZE = 12.0;
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

static unsigned int get_font_dirs_numel(const char * const * dir)
{
  unsigned int numel;
  
  numel = 0;
  while(dir[numel]) numel++;
  return numel;
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

  dim_home = get_font_dirs_numel(FONT_DIRS_HOME);
  dim_xdg = get_font_dirs_numel(FONT_DIRS_XDG);
  dim_sys = get_font_dirs_numel(FONT_DIRS_SYS);

  if(getenv("HOME")) {
     home = sdsnew(getenv("HOME"));
     dim += dim_home;
  }

  if(getenv("XDG_CONFIG_HOME")) {
     xdg = sdsnew(getenv("XDG_CONFIG_HOME"));
     dim += dim_xdg;
  }

  dim += dim_sys;

  /* create an array of sds terminated by a NULL element */
  result = (sds *) malloc((dim+1) * sizeof(sds));
  result[dim] = NULL;

//  for (i = 0; i < dim_home; i++) {
  j = 0;
  if(home) {
     i = -1;
     while(FONT_DIRS_HOME[++i]) {
        result[j] = sdsdup(home);
        result[j] = sdscatfmt(result[j], "%s%s","/",FONT_DIRS_HOME[i]);
        j++;
     }
     sdsfree(home);   
  }   
  if(xdg) {
     i = -1;
     while(FONT_DIRS_XDG[++i]) {
        result[j] = sdsdup(home);
        result[j] = sdscatfmt(result[j], "%s%s","/",FONT_DIRS_XDG[i]);
        j++;
     }
     sdsfree(xdg);   
  }   
  i = -1;
  while(FONT_DIRS_SYS[++i]) {
     result[j] = sdsnew(FONT_DIRS_SYS[i]);
     j++;
  }


  return result;
}

static void free_expanded_paths(sds *path)
{
  unsigned int i = -1;
  while(path[++i]) sdsfree(path[i]);
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
  i = -1;
  while(paths[++i]) {
    path = sdsdup(paths[i]);
    path = sdscatsds(path, name);
    font = SFT_X_create_from_file(path, size, 1.0);
    sdsfree(path);
    if(font) {
       fprintf(stderr, "Font found with file %s. SFT_X points to %p\n", font->filename, (void*) font);
       break;
    }
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

   for(x = 0; x < FONT_COUNT; x++) {
	   if (fontNames[x]) fprintf(stderr, "Font type %d has name %s\n", x, fontNames[x]);
	   else fprintf(stderr, "Font type %d still without name\n", x, fontNames[x]); }
   fprintf(stderr, "\nBegin Loop for assigning type to inherited fonts\n\n");
   /* Inherit unset fonts from the tray for tray items. */
   for(x = 0; x < ARRAY_LENGTH(INHERITED_FONTS); x++) {
      const FontType dest = INHERITED_FONTS[x].dest;
      if(!fontNames[dest]) {
         const FontType src = INHERITED_FONTS[x].src;
         fontNames[dest] = CopyString(fontNames[src]);
      }
   }

   fprintf(stderr, "\nEnded Loop for inherited fonts\n\n");
   for(x = 0; x < FONT_COUNT; x++) {
	   if (fontNames[x]) fprintf(stderr, "Font type %d has name %s\n", x, fontNames[x]);
	   else fprintf(stderr, "Font type %d still without name\n", x, fontNames[x]); }
   fprintf(stderr, "\nBegin Loop for searching font\n\n");
#ifdef USE_XRENDER
   sds * expanded_paths = NULL;
   sds default_name = NULL;
   unsigned int i;
   expanded_paths = font_expanded_paths();
   for(x = 0; x < FONT_COUNT; x++) {
      if(fontNames[x]) {
         NameSize * ns = get_font_name_size(fontNames[x]);
         if(ns->name[0] == '/') {  //We assume this is an absolute path
            fonts[x] = SFT_X_create_from_file(ns->name, ns->size, 1.0);
         } else { //search in FONT_DIRS
            fonts[x] = search_font_in_default_paths(ns->name, ns->size, expanded_paths);
         } 
         if(JUNLIKELY(!fonts[x])) {
            fprintf(stderr, "Could not load font %d with font name %s\n", x, fontNames[x]);
            Warning(_("could not load font: %s"), fontNames[x]);
         }
         FreeNameSize(ns);   
      }
      if(!fonts[x]) {
            fprintf(stderr, "Using default font for font type %d\n", x);
         default_name = sdsnew(DEFAULT_FONT);
         default_name = sdscat(default_name,".ttf");
         if(DEFAULT_FONT[0] == '/') {  //We assume this is an absolute path
            fonts[x] = SFT_X_create_from_file(default_name, DEFAULT_SIZE, 1.0);
         } else { //search in FONT_DIRS
            fonts[x] = search_font_in_default_paths(default_name, DEFAULT_SIZE, expanded_paths);
         } 
         sdsfree(default_name);
     } 
     if(JUNLIKELY(!fonts[x])) {
        FatalError(_("could not load the default font: %s"), DEFAULT_FONT);
     }
        
   }
   free_expanded_paths(expanded_paths);

   fprintf(stderr, "\nLoop for searching font ended\n\n");
   for(x = 0; x < FONT_COUNT; x++) {
	   if (fontNames[x]) fprintf(stderr, "Font type %d has name %s and points to %p\n", x, fontNames[x], fonts[x]);
	   else fprintf(stderr, "Font type %d still without name, pointing to %p\n", x, fonts[x]); }

/* for test only/
   for(x = 0; x < FONT_COUNT; x++) {
     if(fonts[x]) {
         SFT_X_free(fonts[x]);
     }
   }

   fonts[0]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   fonts[1]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/FiraGO-Regular.ttf", 15.5, 1.0);
   fonts[2]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   fonts[3]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   fonts[4]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   fonts[5]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/chivo-v18-latin_latin-ext-500.ttf", 13.0, 1.0);
   fonts[6]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   fonts[7]=SFT_X_create_from_file("/home/alpine/.config/ggwm/fonts/alegreya-sans-v25-latin_latin-ext-regular.ttf", 13.5, 1.0);
   return;
 end test only */

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
}

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
   result = SFT_X_get_string_width(fonts[ft], output);
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
#ifdef USE_XRENDER
   XRenderColor * sft_color = GetXRenderColor(color);
#else
   XGCValues gcValues;
   unsigned long gcMask;
   GC gc;
#endif
   char *utf8String;

   /* Early return for empty strings. */
   if(!str || !str[0] || width < 1) {
      return;
   }

   /* Convert to UTF-8 if necessary. */
   utf8String = GetUTF8String(str);

   /* Get the length of the UTF-8 string. */
   len = strlen(utf8String);

#ifdef USE_XRENDER
//   xd = XftDrawCreate(display, d, rootVisual, rootColormap);
#else
   gcMask = GCGraphicsExposures;
   gcValues.graphics_exposures = False;
   gc = JXCreateGC(display, d, gcMask, &gcValues);
#endif


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
#ifdef USE_XRENDER
   rect.width =  SFT_X_get_string_width(fonts[font], str);
#else
   rect.width = XTextWidth(fonts[font], output, len);
#endif
   rect.width = Min(rect.width, width) + 2;

   /* Combine the width bounds with the region to use. */
   renderRegion = XCreateRegion();
   XUnionRectWithRegion(&rect, renderRegion, renderRegion);

   /* Display the string. */
#ifdef USE_XRENDER
   SFT_X_draw_string32(display, d, x, y,  sft_color, fonts[font], str, width);
   free(sft_color);
#else
   JXSetForeground(display, gc, colors[color]);
   JXSetRegion(display, gc, renderRegion);
   JXSetFont(display, gc, fonts[font]->fid);
   JXDrawString(display, d, gc, x, y + fonts[font]->ascent, output, len);
#endif

   /* Free any memory used for UTF conversion. */
#ifdef USE_FRIBIDI
   ReleaseStack(temp_i);
   ReleaseStack(temp_o);
   ReleaseStack(output);
#endif
   ReleaseUTF8String(utf8String);

   XDestroyRegion(renderRegion);

#ifdef USE_XRENDER
//   XftDrawDestroy(xd);
#else
   JXFreeGC(display, gc);
#endif

}
