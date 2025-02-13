/**
 * @file help.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for displaying information about JWM.
 *
 * Modified 2025 Scaramacai
 *
 */

#include "ggwm.h"
#include "help.h"

static void DisplayUsage(void);

/** Display program name, version, and compiled options . */
void DisplayAbout(void)
{
   printf("GGWM v" PACKAGE_VERSION " by Scaramacai\n");
   printf("Based on JWM v2.4.6 by Joe Wingbermuehle\n");
   printf("native icons svg xpm support\n");
   DisplayCompileOptions();
}

/** Display compiled options. */
void DisplayCompileOptions(void)
{
   printf("compiled options: "
#ifndef DISABLE_CONFIRM
          "confirm "
#endif
#ifdef DEBUG
          "debug "
#endif
#ifdef USE_FRIBIDI
          "fribidi "
#endif
#ifdef USE_JPEG
          "jpeg "
#endif
#ifdef ENABLE_NLS
          "nls "
#endif
#ifdef USE_PNG
          "png "
#endif
#ifdef USE_SHAPE
          "shape "
#endif
#ifdef USE_XBM
          "xbm "
#endif
#ifdef USE_XINERAMA
          "xinerama "
#endif
#ifdef USE_XRENDER
          "xrender "
#endif
          "\nsystem configuration: " SYSTEM_CONFIG "\n");
}

/** Display all help. */
void DisplayHelp(void)
{
   DisplayUsage();
   printf("  -display X  Set the X display to use\n"
          "  -exit       Exit JWM (send _JWM_EXIT to the root)\n"
          "  -f file     Use specified configuration file\n"
          "  -h          Display this help message\n"
          "  -p          Parse the configuration file and exit\n"
          "  -reload     Reload menu (send _JWM_RELOAD to the root)\n"
          "  -restart    Restart JWM (send _JWM_RESTART to the root)\n"
          "  -v          Display version information\n");
}

/** Display program usage information. */
void DisplayUsage(void)
{
   DisplayAbout();
   printf("usage: ggwm [ options ]\n");
}

