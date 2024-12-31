ggwm
==============================================================================

**ggwm** is my personal fork of the excellent window manager [JWM](https://github.com/joewing/jwm) by Joe Wingbermuehle.

Why? Because I want a wm that is more light-weight than jwm, but still enough user-friendly to be also usable by a non skilled person.

So I decided that jwm was a good starting point. It has a taskbar and a menu, can manage virtual desktops, it is easy to configure and so on. Furthermore its code is well structured and easy to understand and hack.

The goals of the project are mainly focused on reducing dependecies on external libraries and possibly reduce the size of the executable itself. 

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
