Schrift library and fonts declaration for ggwm
==============================================================================

A big step in the ggwm development is in use of [libschrift](https://github.com/tomolt/libschrift) by Thomas Holtmann, to deal with True Type fonts.  
This is still at an alpha state, but things are going in the right direction and now ggwm with libschrift can be used for everyday work.  
With libschrift the dependecies on libXft and FontConfig are gone.   
Lib Xrender, instead, is still needed to render strings, but I hope to be able to remove  remove this dependency in the future, so that ggwm could be used also on servers without the XRender extension.   

At present ggwm is usable, but there is a strange bug I need to investigate, so that only some configurations of fonts works. On the other hand, when ggwm works, it works well, and I'm using it every day.   

ggwm implements an hardcoded font used by default, however, as per
the bug I mentioned before, for some unknown reasons, some of the items are non drawn (or not refreshed, or what?), depending on the configuration of the fonts in ggwmrc.

## How to prepare ggwmrc to have a functional ggwm
After the last modification (github 0d1c368, fossil commit d34bbcff31) you will have a full functional ggwm when you explicitly declare a font in each Style section of ggwmrc,
that is  WindowStyle, MenuStyle, TrayStyle and so on.

-    **You MUST explicitly declare FONT in WindowStyle, MenuStyle, TrayStyle, PopupStyle, ClockStyle, TaskListStyle, PagerStyle and TrayButtonStyle**.
-    Other configurations could work for you, but are not tested.

If you don't follow these directives on your configuration file ggwmrc, weird things happen!   
In many cases, you miss the labels on the pager, or the popups, but there are also configurations where the menu font disappears.

Anyway, since there are configurations making ggwm happy, and working well, I think it is time to publish it,
 and hope that someone can help to fix this disappointing bug.

### Font declaration in ggwmrc

Libschrift works with the most of ttf files, while it is not featured to deal with otf or Type1 fonts.

Hence you can choose only among True Type (.ttf) fonts. Today, however, there is a large choice of very good fonts.

I'll add some configuration examples and some fonts for latin alphabet that work well.
If you are not on the latin side, you can try [Noto fonts](https://fonts.google.com/noto) or [FiraGO](https://github.com/bBoxType/FiraGO).

Since ggwm does not use fontconfig, font names used in ggwmrc are different from the ones we are used with Xft and so on.

***In ggwmrc you must indicate the name of the font file, not the font family as it is in fontconfig***

- A font file is expected to end with **.ttf**;
- In ggwmrc you should provide the name of the file **without** the suffix .ttf;
  - contrary of fontconfig way, a file name denotes one style only (usually Regular), hence if you want bold or italic you must give the name of the file that contains bold/italic; 
- If you want a size different from the default you can add it at the end of the name, separated by a separator that at the moment is hardcoded to **::** (colon colon);
  - size is of type _double_, hence you can give a non integer size;
- If a font file begins with a **/** it is assumed to be an absolute path, otherwise it will be searched in predefined (hardcoded) directories.

The searched directories are (in the order of search):

-  $HOME/.config/ggwm/fonts/
-  $HOME/.local/share/fonts/
-  $XDG_CONFIG_HOME/ggwm/fonts/
-  /usr/share/fonts/ 
-  /usr/share/fonts/truetype/ 
-  /usr/local/share/fonts/truetype/ 

where $HOME stands for the home directory and $XDG_CONFIG_HOME is the base directory for user-specific configuration files,
 according to the [specifications](https://specifications.freedesktop.org/basedir-spec/latest/) of freedesktop.org.

### Examples

#### example 1 (using font files stored in system directories)

Say that you have a font file, e.g.  /usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular.ttf, you want to add to your configuration. You can specify it with:

~~~
<FONT>/usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular</FONT>
~~~

Since the above is an absolute path, it is taken as is, and font configuration directories are not searched.
However, since the first part of the path, /usr/share/fonts/, is among the searched directories, you can also specify the font as jetbrains-mono/JetBrainsMono-Regular, that is you write, in your ggwmrc file:

~~~ 
<FONT>jetbrains-mono/JetBrainsMono-Regular</FONT>
~~~

In any case, the name will be expanded, at runtime, to the absolute path name:
 
**/usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular.ttf**

#### example 2 (using font files in config directories)

Now you decide you want to use another font file, say AlegreyaSans-Regular.ttf, stored in your .config/ggwm/fonts/ directory. In this case you can simply write:

~~~ 
<FONT>AlegreyaSans-Regular</FONT>
~~~

If you want the font at another size, for example 13.5, you write:

~~~ 
<FONT>AlegreyaSans-Regular::13.5</FONT>
~~~

Also in these cases, the name will be expanded to the absolute path name, say:
 
**/home/_your-home-directory-name_/.config/ggwm/fonts/AlegreyaSans-Regular.ttf**

while the size, if any, will be extracted and used to duly configure the font at runtime.

#### example 3 (using symbolic links in config directories)

Note the you can use symbolic links instead of real path names. Say you are tired to search the right font inside the font directories. You can link the font file with a symbolic name in one of the config directories. For example, from your home directory:

~~~
~ $ cd .config/ggwm/fonts
~ $ ln -s /usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular.ttf JetBrains.ttf
~~~

Now, if you want the font in the font file /usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular.ttf, at size 16, you can simply write:

~~~ 
<FONT>JetBrains::16</FONT>
~~~

Again, the font name is expanded to the real path:
 
**/usr/share/fonts/jetbrains-mono/JetBrainsMono-Regular.ttf**

--------------------------------------------------------------------------
An example of configuration file is in dot_files/ggwm/ggwmrc
