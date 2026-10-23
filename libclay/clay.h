/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clay.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_H__
#define __CLAY_H__

#include <cairo.h>
#include <glib-object.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pango.h>

#include <libclaybase/claybase.h>
#include <libclaycolor/claycolor.h>
#include <libclayconfig/clayconfig.h>
#include <libclaymath/claymath.h>

#define __CLAY_H_INSIDE__

#include <libclay/clayenums.h>
#include <libclay/claytypes.h>

#include <libclay/claybatchprocedure.h>
#include <libclay/claybrush.h>
#include <libclay/claychannel.h>
#include <libclay/claycurve.h>
#include <libclay/claydisplay.h>
#include <libclay/claydrawable.h>
#include <libclay/claydrawablefilter.h>
#include <libclay/claydrawablefilterconfig.h>
#include <libclay/clayexportoptions.h>
#include <libclay/clayexportprocedure.h>
#include <libclay/clayfont.h>
#include <libclay/clayclayrc.h>
#include <libclay/claygradient.h>
#include <libclay/claygrouplayer.h>
#include <libclay/clayimage.h>
#include <libclay/clayimagecolorprofile.h>
#include <libclay/clayimagemetadata.h>
#include <libclay/clayimageprocedure.h>
#include <libclay/clayitem.h>
#include <libclay/claylayer.h>
#include <libclay/claylayermask.h>
#include <libclay/claylinklayer.h>
#include <libclay/clayloadprocedure.h>
#include <libclay/claypalette.h>
#include <libclay/clayparamspecs.h>
#include <libclay/claypath.h>
#include <libclay/claypattern.h>
#include <libclay/claypdb.h>
#include <libclay/clayplugin.h>
#include <libclay/clayprocedureconfig.h>
#include <libclay/clayprocedure-params.h>
#include <libclay/clayprogress.h>
#include <libclay/clayrasterizable.h>
#include <libclay/clayresource.h>
#include <libclay/clayselection.h>
#include <libclay/claytextlayer.h>
#include <libclay/claythumbnailprocedure.h>
#include <libclay/clayvectorlayer.h>
#include <libclay/clayvectorloadprocedure.h>

#include <libclay/clay_pdb_headers.h>

#undef __CLAY_H_INSIDE__

#ifdef G_OS_WIN32
#include <stdlib.h> /* For __argc and __argv */
#endif

G_BEGIN_DECLS


/**
 * CLAY_MAIN:
 * @plug_in_type: The #GType of the plug-in's #ClayPlugIn subclass
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code:
 *
 * ```C
 * CLAY_MAIN (MY_TYPE_PLUG_IN)
 * ```
 *
 * at the toplevel of your file. No semicolon should be used.
 *
 * Non-C plug-ins won't have this function and should use
 * [func@Clay.main] instead.
 *
 * Since: 3.0
 **/

#ifdef G_OS_WIN32

/* Define WinMain() because plug-ins are built as GUI applications. Also
 * define a main() in case some plug-in still is built as a console
 * application.
 */
#  ifdef __GNUC__
#    ifndef _stdcall
#      define _stdcall __attribute__((stdcall))
#    endif
#  endif

#  define CLAY_MAIN(plug_in_type)                       \
   struct HINSTANCE__;                                  \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow);                            \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow)                             \
   {                                                    \
     return clay_main (plug_in_type,                    \
                       __argc, __argv);                 \
   }                                                    \
                                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     /* Use __argc and __argv here, too, as they work   \
      * better with mingw-w64.                          \
      */                                                \
     return clay_main (plug_in_type,                    \
                       __argc, __argv);                 \
   }
#else
#  define CLAY_MAIN(plug_in_type)                       \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     return clay_main (plug_in_type,                    \
                       argc, argv);                     \
   }
#endif


/* The main procedure that must be called with the plug-in's
 * ClayPlugIn subclass type and the 'argc' and 'argv' that are passed
 * to "main".
 */
gint                clay_main                 (GType  plug_in_type,
                                               gint   argc,
                                               gchar *argv[]);

/* Return the ClayPlugIn singleton of this plug-in process
 */
ClayPlugIn        * clay_get_plug_in          (void);

/* Return the ClayPDB singleton of this plug-in process
 */
ClayPDB           * clay_get_pdb              (void);

CLAY_DEPRECATED
void                clay_quit                 (void) G_GNUC_NORETURN;

/* Return various constants given by the Clay core at plug-in config time.
 */
guint               clay_tile_width           (void) G_GNUC_CONST;
guint               clay_tile_height          (void) G_GNUC_CONST;
gboolean            clay_show_help_button     (void) G_GNUC_CONST;
gboolean            clay_export_color_profile (void) G_GNUC_CONST;
gboolean            clay_export_comment       (void) G_GNUC_CONST;
gboolean            clay_export_exif          (void) G_GNUC_CONST;
gboolean            clay_export_xmp           (void) G_GNUC_CONST;
gboolean            clay_export_iptc          (void) G_GNUC_CONST;
gboolean            clay_export_thumbnail     (void) G_GNUC_CONST;
gboolean            clay_update_metadata      (void) G_GNUC_CONST;
gint                clay_get_num_processors   (void) G_GNUC_CONST;
ClayCheckSize       clay_check_size           (void) G_GNUC_CONST;
ClayCheckType       clay_check_type           (void) G_GNUC_CONST;
const GeglColor   * clay_check_custom_color1  (void) G_GNUC_CONST;
const GeglColor   * clay_check_custom_color2  (void) G_GNUC_CONST;
ClayDisplay       * clay_default_display      (void) G_GNUC_CONST;
const gchar       * clay_wm_class             (void) G_GNUC_CONST;
const gchar       * clay_display_name         (void) G_GNUC_CONST;
gint                clay_monitor_number       (void) G_GNUC_CONST;
guint32             clay_user_time            (void) G_GNUC_CONST;
const gchar       * clay_icon_theme_dir       (void) G_GNUC_CONST;

const gchar       * clay_get_progname         (void) G_GNUC_CONST;


/* Forcefully causes the clay library to exit and
 * close down its connection to main clay application.
 * For internal use only.
 */
G_GNUC_INTERNAL
void               _clay_quit                 (void) G_GNUC_NORETURN;

G_END_DECLS

#endif /* __CLAY_H__ */
