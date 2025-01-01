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
The original README.md of JWMs in [here](/dir/README.jwm.md)
