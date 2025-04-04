ggwm window manager
==============================================================================
**NOTE:** This is the mirror of my private fossil repository

**ggwm** is my personal fork of the excellent window manager [JWM](https://github.com/joewing/jwm) by Joe Wingbermuehle.

Why? First of all this is for fun, but let me say that I was strongly disappointed when jwm moved to pango/cairo for font rendering, since this implies that jwm depends on a lot of heavy external libraries.

A related reason is that I want a wm that is more light-weight than jwm, but still enough user-friendly to be also usable by a non skilled person.

There are many good small window managers, dwm and cwm being maybe the most famous. However, if you want to add something more than dmenu to its wm tools, again the problem is that most of such tools are apparently small, but require a lot of libraries for working.
 
So said, jwm was a good starting point for my goals. It has a taskbar and a menu, can manage virtual desktops, it is easy to configure and so on. Furthermore its code is enough well structured and easy to understand and hack.

At the beginning I was mainly interested in removing the dependence on pango, hence I took the source code of jwm 2.4.6 and reverted back the font system at the last git commit before pango/cairo. Further I realized that the whole code can be made smaller by using some "alternative" pieces of code, so I decided to change the name of the project, because it was coming to something too different from the original.

The goals of the project are mainly focused on reducing dependecies on external libraries and possibly reduce the size of the executable itself. 

Obviously this would also imply that some of the feature one finds in jwm, are not available in ggwm, but the trade-off is good to me. I removed the (already optional)  dependencies on libXext and libXmu, since I'm not interested to have shapes and rounded corners on a window manager.

The first step in the project was about icon and images. At present, ggwm natively supports svg images and icons thanks to [nanosvg](https://github.com/memononen/nanosvg). This amount to have some svg that cannot be rendered or that can only partially rendered, but in practice I never found a svg icon that fails with nanosvg. 

The canonical libraries for png and jpeg formats have been removed as well, in favour of the single file
 image loader [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h).

Xpm images are managed by using a modified version of the routines by Jim Frost, taken from [xli](https://github.com/openSUSE/xli), hence libXpm is not needed.

A big step in the project was using [libschrift](https://github.com/tomolt/libschrift) by Thomas Holtmann, to deal with True Type fonts.  
This is still at an alpha state, but things are going in the right direction and now ggwm with libschrift can be used for everyday work.  
I removed the dependecies on libXft and FontConfig, while libXrender is still needed to render strings with ttf fonts.
However  I plan to remove this dependency in the future, so that ggwm could be used also on servers without the XRender extension.   

At present ggwm is usable, but there is a strange bug I need to investigate, so that only some configurations of fonts works. **It must be noted, however, that all is fine when ggwm with the same config files is runned inside valgrind!** Any help on this issue will be appreciated very much.

I'm also considering using [stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) instead of libschrift. Furthermore, for complex languages, [hamza](https://github.com/saidwho12/hamza) is looking promising.

Anyway, for the configurations where ggwm already works, it works well, and I'm using it every day.   
For details on the configuration, please see the file README.schrift.md in this directory.

I also included in the code the ["Simple Dynamic Strings library for C"](https://github.com/antirez/sds) by Salvatore Sanfilippo, to simplify manipulating strings. For now, its use was limited to the files I changed or added, but it would be nice to replace with sds code here and there, where strings are used.

To build ggwm you will need a recent C compiler (gcc works), X11, and the
"development headers" for X11 and Xlib.
If available and not disabled at compile time, ggwm  will also use
the following libraries:

 - libXrender for the render extension. In this case, True Type fonts will be used, via libschrift.
 - fribidi for bi-directional text support.
 - libXinerama for multiple head support.


------------------------------------------------------------------------------
The original README.md of JWM is in README.jwm.md
