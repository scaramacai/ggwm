ggwm
==============================================================================

**ggwm** is my personal fork of the excellent window manager [JWM](https://github.com/joewing/jwm) by Joe Wingbermuehle.

Why? First of all this is for fun, but let me say that I was strongly disappointed when jwm moved to pango/cairo for font rendering, since this implies that jwm depends on a lot of heavy external libraries.

A related reason is that I want a wm that is more light-weight than jwm, but still enough user-friendly to be also usable by a non skilled person.

There are many good small window managers, dwm and cwm being maybe the most famous. However, if you want to add something more than dmenu to its wm tools, again the problem is that most of them are apparently small, but require a lot of libraries for working.
 
So said, jwm was a good starting point for my goals. It has a taskbar and a menu, can manage virtual desktops, it is easy to configure and so on. Furthermore its code is well structured and easy to understand and hack.

At the beginning I was mainly interested in removing the dependence on pango, hence I took the source code of jwm 2.4.6 and reverted back the font system at the last git commit before pango/cairo. Further I realized that the whole code can be made smaller by using some "alternative" pieces of code, so I decided to change the name of the project, because it was coming to something too different from the original.

The goals of the project are mainly focused on reducing dependecies on external libraries and possibly reduce the size of the executable itself. 

Obviously this would also imply that some of the feature one finds in jwm, are not available in ggwm, but the trade-off is good to me.

At present, apart from the change in managing Xft fonts, ggwm natively supports svg images and icons thanks to [nanosvg](https://github.com/memononen/nanosvg). This amount to have some svg that cannot be rendered or that can only partially renedered, but in practice I never found a svg icon that fails with nanosvg. 

The canonical libraries for png and jpeg formats have been removed as well, in favour of the single file
 image loader [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h).

If available and not disabled at compile time, ggwm  will also use
the following libraries:

 - fribidi for bi-directional text support.
 - libXext for the shape extension.
 - libXrender for the render extension.
 - libXmu for rounded corners.
 - libXft for anti-aliased and true type fonts.
 - libXinerama for multiple head support.
 - libXpm for XPM icons and backgrounds.


------------------------------------------------------------------------------
The following is the original README.md of JWM

------------------------------------------------------------------------------
JWM (Joe's Window Manager)
==============================================================================

JWM is a light-weight window manager for the X11 Window System.

Requirements
------------------------------------------------------------------------------
To build JWM you will need a C compiler (gcc works), X11, and the
"development headers" for X11 and Xlib.
If available and not disabled at compile time, JWM will also use
the following libraries:

 - cairo and librsvg2 for SVG icons and backgrounds.
 - fribidi for bi-directional text support.
 - libjpeg for JPEG icons and backgrounds.
 - libpng for PNG icons and backgrounds.
 - libXext for the shape extension.
 - libXrender for the render extension.
 - libXmu for rounded corners.
 - libXft for anti-aliased and true type fonts.
 - libXinerama for multiple head support.
 - libXpm for XPM icons and backgrounds.

Installation
------------------------------------------------------------------------------

 0. For building from the git repository, run "./autogen.sh".
 1. Run "./configure --help" for configuration options.
 2. Run "./configure [options]"
 3. Run "make" to build JWM.
 4. Run "make install" to install JWM.  Depending on where you are installing
    JWM, you may need to perform this step as root ("sudo make install").

License
------------------------------------------------------------------------------
See LICENSE for license information.

For more information see http://joewing.net/projects/jwm/
