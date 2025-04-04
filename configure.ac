############################################################################
# GGWM autoconf.
############################################################################

CFLAGS="-Os"
AC_INIT([ggwm],[0.1.0],[scaramacai])
AC_PREREQ([2.72])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_SRCDIR([po])
AC_CONFIG_HEADERS([config.h])
AC_LANG(C)

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MKDIR_P

PACKAGE=ggwm

############################################################################
# Check if we need _XOPEN_SOURCE
############################################################################
AC_MSG_CHECKING([if _XOPEN_SOURCE should be defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#define _XOPEN_SOURCE 600L
#include <unistd.h>
]])], [use_xopen_source="yes"], [use_xopen_source="no"])
AC_MSG_RESULT([$use_xopen_source])

if test $use_xopen_source = "yes"; then

  AC_DEFINE(_XOPEN_SOURCE, 600L, [Define for single UNIX conformance])

  # Needed for IRIX 6.2 so that struct timeval is declared.
  AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, [Define for timeval on IRIX 6.2])

  # Needed for Solaris 2.5.1 so that struct timeval is declared.
  AC_DEFINE(__EXTENSIONS__, 1, [Define for timeval on Solaris 2.5.1])

fi

############################################################################
# Check for X11
############################################################################
AC_PATH_X

if test ! "$no_x" = "yes" ; then
   if test ! x"$x_libraries" = x ; then
      LDFLAGS="$LDFLAGS -L$x_libraries"
   fi
   if test ! x"$x_includes" = x ; then
      CFLAGS="$CFLAGS -I$x_includes"
   fi
else
   AC_MSG_ERROR([Could not find X11])
fi

AC_CHECK_LIB([X11], XOpenDisplay,
   [ LDFLAGS="$LDFLAGS -lX11" ],
   [ AC_MSG_ERROR([libX11 not found]) ])

AC_CHECK_LIB([X11], Xutf8TextPropertyToTextList,
   [ AC_DEFINE(USE_XUTF8, 1, [Define to use Xutf8TextPropertyToTextList]) ],
   [ AC_MSG_WARN([Xutf8TextPropertyToTextList not found in libX11]) ])

############################################################################
# Check for necessary include files.
############################################################################
AC_CHECK_HEADERS([stdarg.h stdio.h stdlib.h ctype.h], [],
   [ AC_MSG_ERROR([one or more necessary header files not found]) ])

AC_CHECK_HEADERS([sys/select.h signal.h unistd.h time.h sys/wait.h sys/time.h])

AC_CHECK_HEADERS([langinfo.h iconv.h])

AC_CHECK_HEADERS([locale.h libintl.h])

AC_CHECK_HEADERS([X11/Xlib.h], [],
   [ AC_MSG_ERROR([Xlib.h could not be found]) ],
   [
#include <X11/X.h>
   ])

AC_CHECK_HEADERS([X11/Xutil.h X11/cursorfont.h X11/Xproto.h \
   X11/Xatom.h X11/keysym.h X11/Xresource.h], [], [],
   [
#include <X11/Xlib.h>
   ])

AC_CHECK_FUNCS([unsetenv putenv setlocale])
AC_FUNC_ALLOCA()

############################################################################
# Check for pkg-config.
############################################################################

AC_DEFUN([GGWM_PKGCONFIG_EXISTS],[AC_PATH_TOOL(PKGCONFIG,pkg-config)])

AC_DEFUN([GGWM_PKGCONFIG],
[
   AC_REQUIRE([GGWM_PKGCONFIG_EXISTS])
   if test "x$PKGCONFIG" != "x" ; then
      AC_MSG_CHECKING([if pkg-config knows about $2])
      if `$PKGCONFIG $2` ; then
         $1="yes"
      else
         $1="no"
      fi
      AC_MSG_RESULT($$1)
   else
      $1="no"
   fi
])

GGWM_PKGCONFIG([use_pkgconfig_xrender], [xrender])
GGWM_PKGCONFIG([use_pkgconfig_fribidi], [fribidi])

############################################################################
# Check if support for the XRENDER extension was requested and available.
# If XRENDER is available, automatically libschrift is used
############################################################################
AC_ARG_ENABLE(xrender,
   AS_HELP_STRING([--disable-xrender],[disable XRender]) )
if test "$enable_xrender" != "no"; then

   if test "$use_pkgconfig_xrender" = "yes" ; then
      XRENDER_CFLAGS=`$PKGCONFIG --cflags xrender`
      XRENDER_LDFLAGS=`$PKGCONFIG --libs xrender`
   else
      XRENDER_LDFLAGS="-lXrender"
   fi

   AC_CHECK_HEADERS([X11/extensions/Xrender.h], [],
      [
         enable_xrender="no";
         AC_MSG_WARN([unable to use X11/extensions/Xrender.h])
      ], [
#include <X11/Xlib.h>
      ])

fi
if test "$enable_xrender" != "no" ; then
   AC_CHECK_LIB(Xrender, XRenderComposite,
      [ LDFLAGS="$LDFLAGS $XRENDER_LDFLAGS"
        CFLAGS="$CFLAGS $XRENDER_CFLAGS"
        enable_xrender="yes"
        AC_DEFINE(USE_XRENDER, 1, [Define to enable the XRender extension]) ],
      [ enable_xrender="no"
        AC_MSG_WARN([unable to use the XRender extension]) ],
      [ $XRENDER_LDFLAGS ])
fi

############################################################################
# Check if FriBidi support was requested and available.
############################################################################
AC_ARG_ENABLE(fribidi,
   AS_HELP_STRING([--disable-fribidi],[disable bi-directional unicode support]) )
if test "$enable_fribidi" != "no" ; then
   if test "$use_pkgconfig_fribidi" = "yes" ; then
      FRIBIDI_CFLAGS=`$PKGCONFIG --cflags fribidi`
      FRIBIDI_LDFLAGS=`$PKGCONFIG --libs fribidi`
      enable_fribidi="yes"
   elif which fribidi-config >/dev/null ; then
      FRIBIDI_CFLAGS=`fribidi-config --cflags`
      FRIBIDI_LDFLAGS=`fribidi-config --libs`
      enable_fribidi="yes"
   else
      FRIBIDI_LDFLAGS="-lfribidi"
      AC_CHECK_LIB(fribidi, fribidi_charset_to_unicode,
         [ enable_fribidi="yes" ],
         [ enable_fribidi="no"
           AC_MSG_WARN([unable to use FriBidi]) ],
         [ $FRIBIDI_LDFLAGS ])
   fi
fi
if test "$enable_fribidi" = "yes" ; then
   LDFLAGS="$LDFLAGS $FRIBIDI_LDFLAGS"
   CFLAGS="$CFLAGS $FRIBIDI_CFLAGS"
   AC_DEFINE(USE_FRIBIDI, 1, [Define to use FriBidi])
fi

############################################################################
# Check if XBM support was requested and available.
############################################################################
AC_ARG_ENABLE(xbm,
    AS_HELP_STRING([--disable-xbm],[disable XBM images]) )
if test "$enable_xbm" != "no"; then
    AC_CHECK_LIB(X11,  XReadBitmapFileData,
        [ enable_xbm="yes"
          AC_DEFINE(USE_XBM, 1, [Define to enable XBM images]) ],
        [ enable_xbm="no"
          AC_MSG_WARN([unable to use XBM images]) ])
fi

############################################################################
# Check if support for Xinerama was requested and available.
############################################################################
AC_ARG_ENABLE(xinerama,
   AS_HELP_STRING([--disable-xinerama],[disable Xinerama support]) )
if test "$enable_xinerama" != "no"; then
   AC_CHECK_LIB(Xinerama, XineramaQueryExtension,
      [ LDFLAGS="$LDFLAGS -lXinerama"
        enable_xinerama="yes"
        AC_DEFINE(USE_XINERAMA, 1, [Define to enable Xinerama]) ],
      [ enable_xinerama="no"
        AC_MSG_WARN([unable to use Xinerama]) ])
fi

############################################################################
# Check if support for gettext was requested and available.
############################################################################
AM_ICONV
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.20])
LDFLAGS="$LDFLAGS $LIBINTL $LIBICONV"

############################################################################
# Check if debug mode was requested.
############################################################################
AC_ARG_ENABLE(debug,
   AS_HELP_STRING([--enable-debug],[use this to debug GGWM]) )
if test "$enable_debug" = "yes"; then
   AC_DEFINE(DEBUG, 1, [Define to debug GGWM])
   CFLAGS="$CFLAGS -Wall -g -DDEBUG"
   LDFLAGS="$LDFLAGS -g"
else
   enable_debug="no"
fi

############################################################################
# libm is needed
############################################################################
LDFLAGS="$LDFLAGS -lm"

############################################################################
# Create the output files.
############################################################################
if test "$prefix" = "NONE" ; then
   PREFIX="$ac_default_prefix"
   prefix="$ac_default_prefix"
else
   PREFIX="$prefix"
fi

if test "$exec_prefix" = "NONE" ; then
   exec_prefix="$PREFIX"
fi

if test "$sysconfdir" = "" ; then
   sysconfdir="$ac_default_sysconfdir"
fi

if test "$mandir" = "" ; then
   mandir="$ac_default_mandir"
fi

if test "$datadir" = "" ; then
    datadir="$ac_default_datadir"
fi

if test "$LOCALEDIR" = "" ; then
   localedir=`eval echo \""$datadir"/locale\"`
   localedir=`eval echo \""$localedir"\"`
   CFLAGS="$CFLAGS -DLOCALEDIR=\\\"$localedir\\\""
fi

BINDIR=`eval echo \""$bindir"\"`
SYSCONF=`eval echo \""$sysconfdir"\"`
MANDIR=`eval echo \""$mandir"\"`
MANDIR=`eval echo \""$MANDIR"\"`
DATADIR=`eval echo \""$datadir"\"`
DATADIR=`eval echo \""$DATADIR"\"`

AC_DEFINE_UNQUOTED(SYSTEM_CONFIG, "$SYSCONF/system.ggwmrc",
   [default system configuration path])

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(VERSION, "$PACKAGE_VERSION")
AC_SUBST(INSTVERSION, `echo $PACKAGE_VERSION | tr -d .`)
AC_SUBST(BINDIR, "$BINDIR")
AC_SUBST(MANDIR, "$MANDIR")
AC_SUBST(DATADIR, "$DATADIR")
AC_SUBST(DATE, `date -u -d "@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y-%m-%d`)
AC_SUBST(SYSCONF, "$SYSCONF")
AC_SUBST(PACKAGE, "$PACKAGE")

AC_CONFIG_FILES([po/Makefile.in
   Makefile
   src/Makefile
   ggwm.1
])
AC_OUTPUT

############################################################################
# Display the status.
############################################################################

echo "Compiler:      $CC"
echo "Compile flags: $CFLAGS"
echo "Link flags:    $LDFLAGS"
echo
echo "Options"
echo
echo "    XBM:      $enable_xbm"
echo "    XRender:  $enable_xrender"
echo "    FriBidi:  $enable_fribidi"
echo "    Xinerama: $enable_xinerama"
echo "    Debug:    $enable_debug"
echo

