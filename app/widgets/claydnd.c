/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <math.h>

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claybuffer.h"
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claydatafactory.h"
#include "core/claydrawable.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/clayimagefile.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claypalette.h"
#include "core/claypattern.h"
#include "core/claytemplate.h"
#include "core/claytoolitem.h"

#include "path/claypath.h"

#include "text/clayfont.h"

#include "claydnd.h"
#include "claydnd-xds.h"
#include "claypixbuf.h"
#include "clayselectiondata.h"
#include "clayview.h"
#include "clayviewrendererimage.h"

#include "clay-log.h"
#include "clay-intl.h"


#define DRAG_PREVIEW_SIZE  CLAY_VIEW_SIZE_LARGE
#define DRAG_ICON_OFFSET   -8


typedef GtkWidget * (* ClayDndGetIconFunc)  (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data);
typedef void        (* ClayDndDragDataFunc) (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data,
                                             GtkSelectionData *selection);
typedef gboolean    (* ClayDndDropDataFunc) (GtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             GCallback         set_data_func,
                                             gpointer          set_data_data,
                                             GtkSelectionData *selection);


typedef struct _ClayDndDataDef ClayDndDataDef;

struct _ClayDndDataDef
{
  GtkTargetEntry       target_entry;

  const gchar         *get_data_func_name;
  const gchar         *get_data_data_name;

  const gchar         *set_data_func_name;
  const gchar         *set_data_data_name;

  ClayDndGetIconFunc   get_icon_func;
  ClayDndDragDataFunc  get_data_func;
  ClayDndDropDataFunc  set_data_func;
};


static GtkWidget * clay_dnd_get_viewable_list_icon (GtkWidget      *widget,
                                                    GdkDragContext *context,
                                                    GCallback       get_viewable_list_func,
                                                    gpointer        get_viewable_list_data);
static GtkWidget * clay_dnd_get_viewable_icon   (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_viewable_func,
                                                 gpointer          get_viewable_data);
static GtkWidget * clay_dnd_get_component_icon  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_comp_func,
                                                 gpointer          get_comp_data);
static GtkWidget * clay_dnd_get_color_icon      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_color_func,
                                                 gpointer          get_color_data);

static void        clay_dnd_get_uri_list_data   (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_uri_list_func,
                                                 gpointer          get_uri_list_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_uri_list_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_uri_list_func,
                                                 gpointer          set_uri_list_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_xds_data        (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_image_func,
                                                 gpointer          get_image_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_color_data      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_color_func,
                                                 gpointer          get_color_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_color_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_color_func,
                                                 gpointer          set_color_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_stream_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_stream_func,
                                                 gpointer          get_stream_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_stream_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_stream_func,
                                                 gpointer          set_stream_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_pixbuf_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_pixbuf_func,
                                                 gpointer          get_pixbuf_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_pixbuf_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_pixbuf_func,
                                                 gpointer          set_pixbuf_data,
                                                 GtkSelectionData *selection);
static void        clay_dnd_get_component_data  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_comp_func,
                                                 gpointer          get_comp_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_component_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_comp_func,
                                                 gpointer          set_comp_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_image_data      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_image_func,
                                                 gpointer          get_image_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_image_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_image_func,
                                                 gpointer          set_image_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_item_data       (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_item_func,
                                                 gpointer          get_item_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_item_data       (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_item_func,
                                                 gpointer          set_item_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_item_list_data  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_item_func,
                                                 gpointer          get_item_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_item_list_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_item_func,
                                                 gpointer          set_item_data,
                                                 GtkSelectionData *selection);

static void        clay_dnd_get_object_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_object_func,
                                                 gpointer          get_object_data,
                                                 GtkSelectionData *selection);

static gboolean    clay_dnd_set_brush_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_brush_func,
                                                 gpointer          set_brush_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_pattern_data    (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_pattern_func,
                                                 gpointer          set_pattern_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_gradient_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_gradient_func,
                                                 gpointer          set_gradient_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_palette_data    (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_palette_func,
                                                 gpointer          set_palette_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_font_data       (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_font_func,
                                                 gpointer          set_font_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_buffer_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_buffer_func,
                                                 gpointer          set_buffer_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_imagefile_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_imagefile_func,
                                                 gpointer          set_imagefile_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_template_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_template_func,
                                                 gpointer          set_template_data,
                                                 GtkSelectionData *selection);
static gboolean    clay_dnd_set_tool_item_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_tool_item_func,
                                                 gpointer          set_tool_item_data,
                                                 GtkSelectionData *selection);



static ClayDndDataDef dnd_data_defs[] =
{
  {
    { NULL, 0, -1 },

    NULL,
    NULL,

    NULL,
    NULL,
    NULL
  },

  {
    CLAY_TARGET_URI_LIST,

    "clay-dnd-get-uri-list-func",
    "clay-dnd-get-uri-list-data",

    "clay-dnd-set-uri-list-func",
    "clay-dnd-set-uri-list-data",

    NULL,
    clay_dnd_get_uri_list_data,
    clay_dnd_set_uri_list_data
  },

  {
    CLAY_TARGET_TEXT_PLAIN,

    NULL,
    NULL,

    "clay-dnd-set-uri-list-func",
    "clay-dnd-set-uri-list-data",

    NULL,
    NULL,
    clay_dnd_set_uri_list_data
  },

  {
    CLAY_TARGET_NETSCAPE_URL,

    NULL,
    NULL,

    "clay-dnd-set-uri-list-func",
    "clay-dnd-set-uri-list-data",

    NULL,
    NULL,
    clay_dnd_set_uri_list_data
  },

  {
    CLAY_TARGET_XDS,

    "clay-dnd-get-xds-func",
    "clay-dnd-get-xds-data",

    NULL,
    NULL,

    clay_dnd_get_viewable_icon,
    clay_dnd_get_xds_data,
    NULL
  },

  {
    CLAY_TARGET_COLOR,

    "clay-dnd-get-color-func",
    "clay-dnd-get-color-data",

    "clay-dnd-set-color-func",
    "clay-dnd-set-color-data",

    clay_dnd_get_color_icon,
    clay_dnd_get_color_data,
    clay_dnd_set_color_data
  },

  {
    CLAY_TARGET_SVG,

    "clay-dnd-get-svg-func",
    "clay-dnd-get-svg-data",

    "clay-dnd-set-svg-func",
    "clay-dnd-set-svg-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_stream_data,
    clay_dnd_set_stream_data
  },

  {
    CLAY_TARGET_SVG_XML,

    "clay-dnd-get-svg-xml-func",
    "clay-dnd-get-svg-xml-data",

    "clay-dnd-set-svg-xml-func",
    "clay-dnd-set-svg-xml-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_stream_data,
    clay_dnd_set_stream_data
  },

  {
    CLAY_TARGET_PIXBUF,

    "clay-dnd-get-pixbuf-func",
    "clay-dnd-get-pixbuf-data",

    "clay-dnd-set-pixbuf-func",
    "clay-dnd-set-pixbuf-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_pixbuf_data,
    clay_dnd_set_pixbuf_data
  },

  {
    CLAY_TARGET_IMAGE,

    "clay-dnd-get-image-func",
    "clay-dnd-get-image-data",

    "clay-dnd-set-image-func",
    "clay-dnd-set-image-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_image_data,
    clay_dnd_set_image_data,
  },

  {
    CLAY_TARGET_COMPONENT,

    "clay-dnd-get-component-func",
    "clay-dnd-get-component-data",

    "clay-dnd-set-component-func",
    "clay-dnd-set-component-data",

    clay_dnd_get_component_icon,
    clay_dnd_get_component_data,
    clay_dnd_set_component_data,
  },

  {
    CLAY_TARGET_LAYER,

    "clay-dnd-get-layer-func",
    "clay-dnd-get-layer-data",

    "clay-dnd-set-layer-func",
    "clay-dnd-set-layer-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_item_data,
    clay_dnd_set_item_data,
  },

  {
    CLAY_TARGET_CHANNEL,

    "clay-dnd-get-channel-func",
    "clay-dnd-get-channel-data",

    "clay-dnd-set-channel-func",
    "clay-dnd-set-channel-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_item_data,
    clay_dnd_set_item_data,
  },

  {
    CLAY_TARGET_LAYER_MASK,

    "clay-dnd-get-layer-mask-func",
    "clay-dnd-get-layer-mask-data",

    "clay-dnd-set-layer-mask-func",
    "clay-dnd-set-layer-mask-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_item_data,
    clay_dnd_set_item_data,
  },

  {
    CLAY_TARGET_PATH,

    "clay-dnd-get-path-func",
    "clay-dnd-get-path-data",

    "clay-dnd-set-path-func",
    "clay-dnd-set-path-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_item_data,
    clay_dnd_set_item_data,
  },

  {
    CLAY_TARGET_BRUSH,

    "clay-dnd-get-brush-func",
    "clay-dnd-get-brush-data",

    "clay-dnd-set-brush-func",
    "clay-dnd-set-brush-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_brush_data
  },

  {
    CLAY_TARGET_PATTERN,

    "clay-dnd-get-pattern-func",
    "clay-dnd-get-pattern-data",

    "clay-dnd-set-pattern-func",
    "clay-dnd-set-pattern-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_pattern_data
  },

  {
    CLAY_TARGET_GRADIENT,

    "clay-dnd-get-gradient-func",
    "clay-dnd-get-gradient-data",

    "clay-dnd-set-gradient-func",
    "clay-dnd-set-gradient-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_gradient_data
  },

  {
    CLAY_TARGET_PALETTE,

    "clay-dnd-get-palette-func",
    "clay-dnd-get-palette-data",

    "clay-dnd-set-palette-func",
    "clay-dnd-set-palette-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_palette_data
  },

  {
    CLAY_TARGET_FONT,

    "clay-dnd-get-font-func",
    "clay-dnd-get-font-data",

    "clay-dnd-set-font-func",
    "clay-dnd-set-font-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_font_data
  },

  {
    CLAY_TARGET_BUFFER,

    "clay-dnd-get-buffer-func",
    "clay-dnd-get-buffer-data",

    "clay-dnd-set-buffer-func",
    "clay-dnd-set-buffer-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_buffer_data
  },

  {
    CLAY_TARGET_IMAGEFILE,

    "clay-dnd-get-imagefile-func",
    "clay-dnd-get-imagefile-data",

    "clay-dnd-set-imagefile-func",
    "clay-dnd-set-imagefile-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_imagefile_data
  },

  {
    CLAY_TARGET_TEMPLATE,

    "clay-dnd-get-template-func",
    "clay-dnd-get-template-data",

    "clay-dnd-set-template-func",
    "clay-dnd-set-template-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_template_data
  },

  {
    CLAY_TARGET_TOOL_ITEM,

    "clay-dnd-get-tool-item-func",
    "clay-dnd-get-tool-item-data",

    "clay-dnd-set-tool-item-func",
    "clay-dnd-set-tool-item-data",

    clay_dnd_get_viewable_icon,
    clay_dnd_get_object_data,
    clay_dnd_set_tool_item_data
  },

  {
    CLAY_TARGET_NOTEBOOK_TAB,

    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL
  },

  {
    CLAY_TARGET_LAYER_LIST,

    "clay-dnd-get-layer-list-func",
    "clay-dnd-get-layer-list-data",

    "clay-dnd-set-layer-list-func",
    "clay-dnd-set-layer-list-data",

    clay_dnd_get_viewable_list_icon,
    clay_dnd_get_item_list_data,
    clay_dnd_set_item_list_data,
  },

  {
    CLAY_TARGET_CHANNEL_LIST,

    "clay-dnd-get-channel-list-func",
    "clay-dnd-get-channel-list-data",

    "clay-dnd-set-channel-list-func",
    "clay-dnd-set-channel-list-data",

    clay_dnd_get_viewable_list_icon,
    clay_dnd_get_item_list_data,
    clay_dnd_set_item_list_data,
  },

  {
    CLAY_TARGET_PATH_LIST,

    "clay-dnd-get-path-list-func",
    "clay-dnd-get-path-list-data",

    "clay-dnd-set-path-list-func",
    "clay-dnd-set-path-list-data",

    clay_dnd_get_viewable_list_icon,
    clay_dnd_get_item_list_data,
    clay_dnd_set_item_list_data,
  },

};


static Clay *the_dnd_clay = NULL;


void
clay_dnd_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (the_dnd_clay == NULL);

  the_dnd_clay = clay;

#ifdef PLATFORM_OSX
  /* on macOS, GDK turns any custom target containing a "/" to a UTI-like
     format, but not the other way around. So, rewrite "/" to "-" to work */
  for (gint i = 0; i < G_N_ELEMENTS (dnd_data_defs); i++)
    {
      const gchar *quartz_target = dnd_data_defs[i].target_entry.target;

      if (quartz_target &&
          (g_str_has_prefix (quartz_target, "application/x-clay-") ||
           g_strcmp0 (quartz_target, "application/x-geglcolor") == 0))
        {
          gchar *quartz_name = g_strdup (quartz_target);

          g_strdelimit (quartz_name, "/", '-');
          dnd_data_defs[i].target_entry.target = quartz_name;
        }
    }
#endif
}


/**********************/
/*  helper functions  */
/**********************/

static void
clay_dnd_target_list_add (GtkTargetList        *list,
                          const GtkTargetEntry *entry)
{
  GdkAtom atom = gdk_atom_intern (entry->target, FALSE);
  guint   info;

  if (! gtk_target_list_find (list, atom, &info) || info != entry->info)
    {
      gtk_target_list_add (list, atom, entry->flags, entry->info);
    }
}


/********************************/
/*  general data dnd functions  */
/********************************/

static void
clay_dnd_data_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context,
                          gpointer        data)
{
  const ClayDndDataDef *dnd_data;
  ClayDndType           data_type;
  GCallback             get_data_func = NULL;
  gpointer              get_data_data = NULL;
  GtkWidget            *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "clay-dnd-get-data-type"));

  CLAY_LOG (DND, "data type %d", data_type);

  if (! data_type)
    return;

  dnd_data = dnd_data_defs + data_type;

  if (dnd_data->get_data_func_name)
    get_data_func = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_func_name);

  if (dnd_data->get_data_data_name)
    get_data_data = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_data_name);

  if (! get_data_func)
    return;

  icon_widget = dnd_data->get_icon_func (widget,
                                         context,
                                         get_data_func,
                                         get_data_data);

  if (icon_widget)
    {
      GtkWidget *frame;
      GtkWidget *window;

      window = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DND);
      gtk_window_set_screen (GTK_WINDOW (window),
                             gtk_widget_get_screen (widget));

      gtk_widget_realize (window);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (window), frame);
      gtk_widget_set_visible (frame, TRUE);

      gtk_container_add (GTK_CONTAINER (frame), icon_widget);
      gtk_widget_set_visible (icon_widget, TRUE);

      g_object_set_data_full (G_OBJECT (widget), "clay-dnd-data-widget",
                              window, (GDestroyNotify) gtk_widget_destroy);

#ifndef PLATFORM_OSX
      /* Gtk-WARNING: gtk_drag_set_icon_widget is not supported on Mac OS X */
      gtk_drag_set_icon_widget (context, window,
                                DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
#endif

      /*  remember for which drag context the widget was made  */
      g_object_set_data (G_OBJECT (window), "clay-gdk-drag-context", context);
    }
}

static void
clay_dnd_data_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  ClayDndType  data_type;
  GtkWidget   *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "clay-dnd-get-data-type"));

  CLAY_LOG (DND, "data type %d", data_type);

  icon_widget = g_object_get_data (G_OBJECT (widget), "clay-dnd-data-widget");

  if (icon_widget)
    {
      /*  destroy the icon_widget only if it was made for this drag
       *  context. See bug #139337.
       */
      if (g_object_get_data (G_OBJECT (icon_widget),
                             "clay-gdk-drag-context") ==
          (gpointer) context)
        {
          g_object_set_data (G_OBJECT (widget), "clay-dnd-data-widget", NULL);
        }
    }
}

static void
clay_dnd_data_drag_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  GCallback    get_data_func = NULL;
  gpointer     get_data_data = NULL;
  ClayDndType  data_type;

  CLAY_LOG (DND, "data type %d", info);

  for (data_type = CLAY_DND_TYPE_NONE + 1;
       data_type <= CLAY_DND_TYPE_LAST;
       data_type++)
    {
      const ClayDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          CLAY_LOG (DND, "target %s", dnd_data->target_entry.target);

          if (dnd_data->get_data_func_name)
            get_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_func_name);

          if (dnd_data->get_data_data_name)
            get_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_data_name);

          if (! get_data_func)
            return;

          dnd_data->get_data_func (widget,
                                   context,
                                   get_data_func,
                                   get_data_data,
                                   selection_data);

          return;
        }
    }
}

static void
clay_dnd_data_drop_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           gint              x,
                           gint              y,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  ClayDndType data_type;

  CLAY_LOG (DND, "data type %d", info);

  if (gtk_selection_data_get_length (selection_data) <= 0)
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  for (data_type = CLAY_DND_TYPE_NONE + 1;
       data_type <= CLAY_DND_TYPE_LAST;
       data_type++)
    {
      const ClayDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          GCallback set_data_func = NULL;
          gpointer  set_data_data = NULL;

          CLAY_LOG (DND, "target %s", dnd_data->target_entry.target);

          if (dnd_data->set_data_func_name)
            set_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_func_name);

          if (dnd_data->set_data_data_name)
            set_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_data_name);

          if (set_data_func &&
              dnd_data->set_data_func (widget, x, y,
                                       set_data_func,
                                       set_data_data,
                                       selection_data))
            {
              gtk_drag_finish (context, TRUE, FALSE, time);
              return;
            }

          gtk_drag_finish (context, FALSE, FALSE, time);
          return;
        }
    }
}

static void
clay_dnd_data_source_add (ClayDndType  data_type,
                          GtkWidget   *widget,
                          GCallback    get_data_func,
                          gpointer     get_data_data)
{
  const ClayDndDataDef *dnd_data;
  gboolean              drag_connected;

  dnd_data = dnd_data_defs + data_type;

  /*  set a default drag source if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-site-data"))
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                         NULL, 0,
                         GDK_ACTION_COPY | GDK_ACTION_MOVE);

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "clay-dnd-drag-connected"));

  if (! drag_connected)
    {
      g_signal_connect (widget, "drag-begin",
                        G_CALLBACK (clay_dnd_data_drag_begin),
                        NULL);
      g_signal_connect (widget, "drag-end",
                        G_CALLBACK (clay_dnd_data_drag_end),
                        NULL);
      g_signal_connect (widget, "drag-data-get",
                        G_CALLBACK (clay_dnd_data_drag_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "clay-dnd-drag-connected",
                         GINT_TO_POINTER (TRUE));
    }

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name,
                     get_data_func);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name,
                     get_data_data);

  /*  remember the first set source type for drag view creation  */
  if (! g_object_get_data (G_OBJECT (widget), "clay-dnd-get-data-type"))
    g_object_set_data (G_OBJECT (widget), "clay-dnd-get-data-type",
                       GINT_TO_POINTER (data_type));

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          clay_dnd_target_list_add (target_list, &dnd_data->target_entry);
        }
      else
        {
          target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

          gtk_drag_source_set_target_list (widget, target_list);
          gtk_target_list_unref (target_list);
        }
    }
}

static gboolean
clay_dnd_data_source_remove (ClayDndType  data_type,
                             GtkWidget   *widget)
{
  const ClayDndDataDef *dnd_data;
  gboolean              drag_connected;
  gboolean              list_changed = FALSE;

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "clay-dnd-drag-connected"));

  if (! drag_connected)
    return FALSE;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name, NULL);

  /*  remove the dnd type remembered for the dnd icon  */
  if (data_type ==
      GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                          "clay-dnd-get-data-type")))
    g_object_set_data (G_OBJECT (widget), "clay-dnd-get-data-type", NULL);

  if (dnd_data->target_entry.target)
    {
      /* Don't just remove the target from the existing list, create a
       * new list without the target and replace the old list. The
       * source's target list is part of a drag operation's state, but
       * only by reference, it's not copied. So when we change the
       * list, we would change the state of that ongoing drag, making
       * it impossible to drop anything. See bug #676522.
       */
      GtkTargetList *target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          GtkTargetList  *new_list;
          GtkTargetEntry *targets;
          gint            n_targets_old;
          gint            n_targets_new;
          gint            i;

          targets = gtk_target_table_new_from_list (target_list, &n_targets_old);

          new_list = gtk_target_list_new (NULL, 0);

          for (i = 0; i < n_targets_old; i++)
            {
              if (targets[i].info != data_type)
                {
                  gtk_target_list_add (new_list,
                                       gdk_atom_intern (targets[i].target, FALSE),
                                       targets[i].flags,
                                       targets[i].info);
                }
            }

          gtk_target_table_free (targets, n_targets_old);

          targets = gtk_target_table_new_from_list (new_list, &n_targets_new);
          gtk_target_table_free (targets, n_targets_new);

          if (n_targets_old != n_targets_new)
            {
              list_changed = TRUE;

              if (n_targets_new > 0)
                gtk_drag_source_set_target_list (widget, new_list);
              else
                gtk_drag_source_set_target_list (widget, NULL);
            }

          gtk_target_list_unref (new_list);
        }
    }

  return list_changed;
}

static void
clay_dnd_data_dest_add (ClayDndType  data_type,
                        GtkWidget   *widget,
                        gpointer     set_data_func,
                        gpointer     set_data_data)
{
  const ClayDndDataDef *dnd_data;
  gboolean              drop_connected;

  /*  set a default drag dest if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);

  drop_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "clay-dnd-drop-connected"));

  if (set_data_func && ! drop_connected)
    {
      g_signal_connect (widget, "drag-data-received",
                        G_CALLBACK (clay_dnd_data_drop_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "clay-dnd-drop-connected",
                         GINT_TO_POINTER (TRUE));
    }

  dnd_data = dnd_data_defs + data_type;

  if (set_data_func)
    {
      g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name,
                         set_data_func);
      g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name,
                         set_data_data);
    }

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_dest_get_target_list (widget);

      if (target_list)
        {
          clay_dnd_target_list_add (target_list, &dnd_data->target_entry);
        }
      else
        {
          target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

          gtk_drag_dest_set_target_list (widget, target_list);
          gtk_target_list_unref (target_list);
        }
    }
}

static void
clay_dnd_data_dest_remove (ClayDndType  data_type,
                           GtkWidget   *widget)
{
  const ClayDndDataDef *dnd_data;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name, NULL);

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_dest_get_target_list (widget);

      if (target_list)
        {
          GdkAtom atom = gdk_atom_intern (dnd_data->target_entry.target, TRUE);

          if (atom != GDK_NONE)
            gtk_target_list_remove (target_list, atom);
        }
    }
}


/****************************/
/*  uri list dnd functions  */
/****************************/

static void
clay_dnd_get_uri_list_data (GtkWidget        *widget,
                            GdkDragContext   *context,
                            GCallback         get_uri_list_func,
                            gpointer          get_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list;

  uri_list = (* (ClayDndDragUriListFunc) get_uri_list_func) (widget,
                                                             get_uri_list_data);

  CLAY_LOG (DND, "uri_list %p", uri_list);

  if (uri_list)
    {
      clay_selection_data_set_uri_list (selection, uri_list);

      g_list_free_full (uri_list, (GDestroyNotify) g_free);
    }
}

static gboolean
clay_dnd_set_uri_list_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_uri_list_func,
                            gpointer          set_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list = clay_selection_data_get_uri_list (selection);

  CLAY_LOG (DND, "uri_list %p", uri_list);

  if (! uri_list)
    return FALSE;

  (* (ClayDndDropUriListFunc) set_uri_list_func) (widget, x, y, uri_list,
                                                  set_uri_list_data);

  g_list_free_full (uri_list, (GDestroyNotify) g_free);

  return TRUE;
}

void
clay_dnd_uri_list_source_add (GtkWidget              *widget,
                              ClayDndDragUriListFunc  get_uri_list_func,
                              gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_URI_LIST, widget,
                            G_CALLBACK (get_uri_list_func),
                            data);
}

void
clay_dnd_uri_list_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_remove (CLAY_DND_TYPE_URI_LIST, widget);
}

void
clay_dnd_uri_list_dest_add (GtkWidget              *widget,
                            ClayDndDropUriListFunc  set_uri_list_func,
                            gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /*  Set a default drag dest if not already done. Explicitly set
   *  COPY and MOVE for file drag destinations. Some file managers
   *  such as Konqueror only offer MOVE by default.
   */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_ALL, NULL, 0,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  clay_dnd_data_dest_add (CLAY_DND_TYPE_URI_LIST, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  clay_dnd_data_dest_add (CLAY_DND_TYPE_TEXT_PLAIN, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  clay_dnd_data_dest_add (CLAY_DND_TYPE_NETSCAPE_URL, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
}

void
clay_dnd_uri_list_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_remove (CLAY_DND_TYPE_URI_LIST, widget);
  clay_dnd_data_dest_remove (CLAY_DND_TYPE_TEXT_PLAIN, widget);
  clay_dnd_data_dest_remove (CLAY_DND_TYPE_NETSCAPE_URL, widget);
}


/******************************/
/* Direct Save Protocol (XDS) */
/******************************/

static void
clay_dnd_get_xds_data (GtkWidget        *widget,
                       GdkDragContext   *context,
                       GCallback         get_image_func,
                       gpointer          get_image_data,
                       GtkSelectionData *selection)
{
  ClayImage   *image;
  ClayContext *clay_context;

  image = g_object_get_data (G_OBJECT (context), "clay-dnd-viewable");

  if (! image)
    image = (ClayImage *)
      (* (ClayDndDragViewableFunc) get_image_func) (widget, &clay_context,
                                                    get_image_data);

  CLAY_LOG (DND, "image %p", image);

  if (image)
    clay_dnd_xds_save_image (context, image, selection);
}

static void
clay_dnd_xds_drag_begin (GtkWidget      *widget,
                         GdkDragContext *context)
{
  const ClayDndDataDef *dnd_data = dnd_data_defs + CLAY_DND_TYPE_XDS;
  GCallback             get_data_func;
  gpointer              get_data_data;

  get_data_func = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_func_name);
  get_data_data = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_data_name);

  if (get_data_func)
    {
      ClayImage   *image;
      ClayContext *clay_context;

      image = (ClayImage *)
        (* (ClayDndDragViewableFunc) get_data_func) (widget, &clay_context,
                                                     get_data_data);

      CLAY_LOG (DND, "image %p", image);

      clay_dnd_xds_source_set (context, image);
    }
}

static void
clay_dnd_xds_drag_end (GtkWidget      *widget,
                       GdkDragContext *context)
{
  clay_dnd_xds_source_set (context, NULL);
}

void
clay_dnd_xds_source_add (GtkWidget               *widget,
                         ClayDndDragViewableFunc  get_image_func,
                         gpointer                 data)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_XDS, widget,
                            G_CALLBACK (get_image_func),
                            data);

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "clay-dnd-xds-drag-begin"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-begin",
                                  G_CALLBACK (clay_dnd_xds_drag_begin),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "clay-dnd-xds-drag-begin",
                         GUINT_TO_POINTER (handler));
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "clay-dnd-xds-drag-end"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-end",
                                  G_CALLBACK (clay_dnd_xds_drag_end),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "clay-dnd-xds-drag-end",
                         GUINT_TO_POINTER (handler));
    }
}

void
clay_dnd_xds_source_remove (GtkWidget *widget)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "clay-dnd-xds-drag-begin"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "clay-dnd-xds-drag-begin", NULL);
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "clay-dnd-xds-drag-end"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "clay-dnd-xds-drag-end", NULL);
    }

  clay_dnd_data_source_remove (CLAY_DND_TYPE_XDS, widget);
}


/*************************/
/*  color dnd functions  */
/*************************/

static GtkWidget *
clay_dnd_get_color_icon (GtkWidget      *widget,
                         GdkDragContext *context,
                         GCallback       get_color_func,
                         gpointer        get_color_data)
{
  GtkWidget *color_area;
  GeglColor *color = NULL;

  (* (ClayDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  CLAY_LOG (DND, "called");

  g_object_set_data_full (G_OBJECT (context),
                          "clay-dnd-color", color,
                          (GDestroyNotify) g_object_unref);

  color_area = clay_color_area_new (color, CLAY_COLOR_AREA_SMALL_CHECKS, 0);
  clay_color_area_set_color_config (CLAY_COLOR_AREA (color_area),
                                    the_dnd_clay->config->color_management);
  gtk_widget_set_size_request (color_area,
                               DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);

  return color_area;
}

static void
clay_dnd_get_color_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_color_func,
                         gpointer          get_color_data,
                         GtkSelectionData *selection)
{
  GeglColor *color = NULL;

  color = g_object_get_data (G_OBJECT (context), "clay-dnd-color");

  if (color)
    color = g_object_ref (color);
  else
    (* (ClayDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  CLAY_LOG (DND, "called");

  clay_selection_data_set_color (selection, color);
  g_object_unref (color);
}

static gboolean
clay_dnd_set_color_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_color_func,
                         gpointer          set_color_data,
                         GtkSelectionData *selection)
{
  GeglColor *color;

  CLAY_LOG (DND, "called");

  if (! (color = clay_selection_data_get_color (selection)))
    return FALSE;

  (* (ClayDndDropColorFunc) set_color_func) (widget, x, y, color,
                                             set_color_data);

  return TRUE;
}

void
clay_dnd_color_source_add (GtkWidget            *widget,
                           ClayDndDragColorFunc  get_color_func,
                           gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_COLOR, widget,
                            G_CALLBACK (get_color_func),
                            data);
}

void
clay_dnd_color_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_remove (CLAY_DND_TYPE_COLOR, widget);
}

void
clay_dnd_color_dest_add (GtkWidget            *widget,
                         ClayDndDropColorFunc  set_color_func,
                         gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_add (CLAY_DND_TYPE_COLOR, widget,
                          G_CALLBACK (set_color_func),
                          data);
}

void
clay_dnd_color_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_remove (CLAY_DND_TYPE_COLOR, widget);
}


/**************************/
/*  stream dnd functions  */
/**************************/

static void
clay_dnd_get_stream_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_stream_func,
                          gpointer          get_stream_data,
                          GtkSelectionData *selection)
{
  guchar *stream;
  gsize   stream_length;

  stream = (* (ClayDndDragStreamFunc) get_stream_func) (widget, &stream_length,
                                                        get_stream_data);

  CLAY_LOG (DND, "stream %p, length %" G_GSIZE_FORMAT, stream, stream_length);

  if (stream)
    {
      clay_selection_data_set_stream (selection, stream, stream_length);
      g_free (stream);
    }
}

static gboolean
clay_dnd_set_stream_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_stream_func,
                          gpointer          set_stream_data,
                          GtkSelectionData *selection)
{
  const guchar *stream;
  gsize         stream_length;

  stream = clay_selection_data_get_stream (selection, &stream_length);

  CLAY_LOG (DND, "stream %p, length %" G_GSIZE_FORMAT, stream, stream_length);

  if (! stream)
    return FALSE;

  (* (ClayDndDropStreamFunc) set_stream_func) (widget, x, y,
                                               stream, stream_length,
                                               set_stream_data);

  return TRUE;
}

void
clay_dnd_svg_source_add (GtkWidget             *widget,
                         ClayDndDragStreamFunc  get_svg_func,
                         gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_SVG, widget,
                            G_CALLBACK (get_svg_func),
                            data);
  clay_dnd_data_source_add (CLAY_DND_TYPE_SVG_XML, widget,
                            G_CALLBACK (get_svg_func),
                            data);
}

void
clay_dnd_svg_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_remove (CLAY_DND_TYPE_SVG, widget);
  clay_dnd_data_source_remove (CLAY_DND_TYPE_SVG_XML, widget);
}

void
clay_dnd_svg_dest_add (GtkWidget             *widget,
                       ClayDndDropStreamFunc  set_svg_func,
                       gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_add (CLAY_DND_TYPE_SVG, widget,
                          G_CALLBACK (set_svg_func),
                          data);
  clay_dnd_data_dest_add (CLAY_DND_TYPE_SVG_XML, widget,
                          G_CALLBACK (set_svg_func),
                          data);
}

void
clay_dnd_svg_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_remove (CLAY_DND_TYPE_SVG, widget);
  clay_dnd_data_dest_remove (CLAY_DND_TYPE_SVG_XML, widget);
}


/**************************/
/*  pixbuf dnd functions  */
/**************************/

static void
clay_dnd_get_pixbuf_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_pixbuf_func,
                          gpointer          get_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  pixbuf = (* (ClayDndDragPixbufFunc) get_pixbuf_func) (widget,
                                                        get_pixbuf_data);

  CLAY_LOG (DND, "pixbuf %p", pixbuf);

  if (pixbuf)
    {
      clay_set_busy (the_dnd_clay);

      gtk_selection_data_set_pixbuf (selection, pixbuf);
      g_object_unref (pixbuf);

      clay_unset_busy (the_dnd_clay);
    }
}

static gboolean
clay_dnd_set_pixbuf_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_pixbuf_func,
                          gpointer          set_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  clay_set_busy (the_dnd_clay);

  pixbuf = gtk_selection_data_get_pixbuf (selection);

  clay_unset_busy (the_dnd_clay);

  CLAY_LOG (DND, "pixbuf %p", pixbuf);

  if (! pixbuf)
    return FALSE;

  (* (ClayDndDropPixbufFunc) set_pixbuf_func) (widget, x, y,
                                               pixbuf,
                                               set_pixbuf_data);

  g_object_unref (pixbuf);

  return TRUE;
}

void
clay_dnd_pixbuf_source_add (GtkWidget             *widget,
                            ClayDndDragPixbufFunc  get_pixbuf_func,
                            gpointer               data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_PIXBUF, widget,
                            G_CALLBACK (get_pixbuf_func),
                            data);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  clay_pixbuf_targets_add (target_list, CLAY_DND_TYPE_PIXBUF, TRUE);

  gtk_drag_source_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
clay_dnd_pixbuf_source_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_remove (CLAY_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    clay_pixbuf_targets_remove (target_list);
}

void
clay_dnd_pixbuf_dest_add (GtkWidget              *widget,
                          ClayDndDropPixbufFunc   set_pixbuf_func,
                          gpointer                data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_add (CLAY_DND_TYPE_PIXBUF, widget,
                          G_CALLBACK (set_pixbuf_func),
                          data);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  clay_pixbuf_targets_add (target_list, CLAY_DND_TYPE_PIXBUF, FALSE);

  gtk_drag_dest_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
clay_dnd_pixbuf_dest_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_remove (CLAY_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    clay_pixbuf_targets_remove (target_list);
}


/*****************************/
/*  component dnd functions  */
/*****************************/

static GtkWidget *
clay_dnd_get_component_icon (GtkWidget      *widget,
                             GdkDragContext *context,
                             GCallback       get_comp_func,
                             gpointer        get_comp_data)
{
  GtkWidget       *view;
  ClayImage       *image;
  ClayContext     *clay_context;
  ClayChannelType  channel;

  image = (* (ClayDndDragComponentFunc) get_comp_func) (widget, &clay_context,
                                                        &channel,
                                                        get_comp_data);

  CLAY_LOG (DND, "image %p, component %d", image, channel);

  if (! image)
    return NULL;

  g_object_set_data_full (G_OBJECT (context),
                          "clay-dnd-viewable", g_object_ref (image),
                          (GDestroyNotify) g_object_unref);
  g_object_set_data (G_OBJECT (context),
                     "clay-dnd-component", GINT_TO_POINTER (channel));

  view = clay_view_new (clay_context, CLAY_VIEWABLE (image),
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  CLAY_VIEW_RENDERER_IMAGE (CLAY_VIEW (view)->renderer)->channel = channel;

  return view;
}

static void
clay_dnd_get_component_data (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GCallback         get_comp_func,
                             gpointer          get_comp_data,
                             GtkSelectionData *selection)
{
  ClayImage       *image;
  ClayContext     *clay_context;
  ClayChannelType  channel = 0;

  image = g_object_get_data (G_OBJECT (context), "clay-dnd-viewable");
  channel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                                "clay-dnd-component"));

  if (! image)
    image = (* (ClayDndDragComponentFunc) get_comp_func) (widget, &clay_context,
                                                          &channel,
                                                          get_comp_data);

  CLAY_LOG (DND, "image %p, component %d", image, channel);

  if (image)
    clay_selection_data_set_component (selection, image, channel);
}

static gboolean
clay_dnd_set_component_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_comp_func,
                             gpointer          set_comp_data,
                             GtkSelectionData *selection)
{
  ClayImage       *image;
  ClayChannelType  channel = 0;

  image = clay_selection_data_get_component (selection, the_dnd_clay,
                                             &channel);

  CLAY_LOG (DND, "image %p, component %d", image, channel);

  if (! image)
    return FALSE;

  (* (ClayDndDropComponentFunc) set_comp_func) (widget, x, y,
                                                image, channel,
                                                set_comp_data);

  return TRUE;
}

void
clay_dnd_component_source_add (GtkWidget                *widget,
                               ClayDndDragComponentFunc  get_comp_func,
                               gpointer                  data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_add (CLAY_DND_TYPE_COMPONENT, widget,
                            G_CALLBACK (get_comp_func),
                            data);
}

void
clay_dnd_component_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_source_remove (CLAY_DND_TYPE_COMPONENT, widget);
}

void
clay_dnd_component_dest_add (GtkWidget                 *widget,
                             ClayDndDropComponentFunc   set_comp_func,
                             gpointer                   data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_add (CLAY_DND_TYPE_COMPONENT, widget,
                          G_CALLBACK (set_comp_func),
                          data);
}

void
clay_dnd_component_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  clay_dnd_data_dest_remove (CLAY_DND_TYPE_COMPONENT, widget);
}


/*******************************************/
/*  ClayViewable (by GType) dnd functions  */
/*******************************************/

static GtkWidget *
clay_dnd_get_viewable_icon (GtkWidget      *widget,
                            GdkDragContext *context,
                            GCallback       get_viewable_func,
                            gpointer        get_viewable_data)
{
  ClayViewable *viewable;
  ClayContext  *clay_context;
  GtkWidget    *view;
  gchar        *desc;

  viewable = (* (ClayDndDragViewableFunc) get_viewable_func) (widget,
                                                              &clay_context,
                                                              get_viewable_data);

  CLAY_LOG (DND, "viewable %p", viewable);

  if (! viewable)
    return NULL;

  g_object_set_data_full (G_OBJECT (context),
                          "clay-dnd-viewable", g_object_ref (viewable),
                          (GDestroyNotify) g_object_unref);

  view = clay_view_new (clay_context, viewable,
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  desc = clay_viewable_get_description (viewable, NULL);

  if (desc)
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
      gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
      gtk_widget_set_visible (view, TRUE);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label",           desc,
                            "xalign",          0.0,
                            "yalign",          0.5,
                            "max-width-chars", 30,
                            "width-chars",     MIN (strlen (desc), 10),
                            "ellipsize",       PANGO_ELLIPSIZE_END,
                            NULL);

      g_free (desc);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_set_visible (label, TRUE);

      return hbox;
    }

  return view;
}

static ClayDndType
clay_dnd_data_type_get_by_g_type (GType    type,
                                  gboolean list)
{
  ClayDndType dnd_type = CLAY_DND_TYPE_NONE;

  if (g_type_is_a (type, CLAY_TYPE_IMAGE) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_IMAGE;
    }
  else if (g_type_is_a (type, CLAY_TYPE_LAYER))
    {
      dnd_type = list ? CLAY_DND_TYPE_LAYER_LIST : CLAY_DND_TYPE_LAYER;
    }
  else if (g_type_is_a (type, CLAY_TYPE_LAYER_MASK) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_LAYER_MASK;
    }
  else if (g_type_is_a (type, CLAY_TYPE_CHANNEL))
    {
      dnd_type = list ? CLAY_DND_TYPE_CHANNEL_LIST : CLAY_DND_TYPE_CHANNEL;
    }
  else if (g_type_is_a (type, CLAY_TYPE_PATH))
    {
      dnd_type = list ? CLAY_DND_TYPE_PATH_LIST : CLAY_DND_TYPE_PATH;
    }
  else if (g_type_is_a (type, CLAY_TYPE_BRUSH) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_BRUSH;
    }
  else if (g_type_is_a (type, CLAY_TYPE_PATTERN) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_PATTERN;
    }
  else if (g_type_is_a (type, CLAY_TYPE_GRADIENT) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_GRADIENT;
    }
  else if (g_type_is_a (type, CLAY_TYPE_PALETTE) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_PALETTE;
    }
  else if (g_type_is_a (type, CLAY_TYPE_FONT) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_FONT;
    }
  else if (g_type_is_a (type, CLAY_TYPE_BUFFER) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_BUFFER;
    }
  else if (g_type_is_a (type, CLAY_TYPE_IMAGEFILE) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_IMAGEFILE;
    }
  else if (g_type_is_a (type, CLAY_TYPE_TEMPLATE) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_TEMPLATE;
    }
  else if (g_type_is_a (type, CLAY_TYPE_TOOL_ITEM) && ! list)
    {
      dnd_type = CLAY_DND_TYPE_TOOL_ITEM;
    }

  return dnd_type;
}

gboolean
clay_dnd_drag_source_set_by_type (GtkWidget       *widget,
                                  GdkModifierType  start_button_mask,
                                  GType            type,
                                  GdkDragAction    actions)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  gtk_drag_source_set (widget, start_button_mask,
                       &dnd_data_defs[dnd_type].target_entry, 1,
                       actions);

  return TRUE;
}

gboolean
clay_dnd_drag_dest_set_by_type (GtkWidget       *widget,
                                GtkDestDefaults  flags,
                                GType            type,
                                gboolean         list_accepted,
                                GdkDragAction    actions)
{
  GtkTargetEntry target_entries[2];
  ClayDndType    dnd_type;
  gint           target_entries_n = 0;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  if (list_accepted)
    {
      dnd_type = clay_dnd_data_type_get_by_g_type (type, TRUE);

      if (dnd_type != CLAY_DND_TYPE_NONE)
        {
          target_entries[target_entries_n] = dnd_data_defs[dnd_type].target_entry;
          target_entries_n++;
        }
    }

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);
  if (dnd_type != CLAY_DND_TYPE_NONE)
    {
      target_entries[target_entries_n] = dnd_data_defs[dnd_type].target_entry;
      target_entries_n++;
    }

  if (target_entries_n == 0)
    return FALSE;

  gtk_drag_dest_set (widget, flags,
                     (const GtkTargetEntry *) &target_entries,
                     target_entries_n,
                     actions);

  return TRUE;
}

/**
 * clay_dnd_viewable_source_add:
 * @widget:
 * @type:
 * @get_viewable_func:
 * @data:
 *
 * Sets up @widget as a drag source for a #ClayViewable object, as
 * returned by @get_viewable_func on @widget and @data.
 *
 * @type must be a list type for drag operations.
 */
gboolean
clay_dnd_viewable_source_add (GtkWidget               *widget,
                              GType                    type,
                              ClayDndDragViewableFunc  get_viewable_func,
                              gpointer                 data)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (get_viewable_func != NULL, FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_source_add (dnd_type, widget,
                            G_CALLBACK (get_viewable_func),
                            data);

  return TRUE;
}

gboolean
clay_dnd_viewable_source_remove (GtkWidget *widget,
                                 GType      type)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  return clay_dnd_data_source_remove (dnd_type, widget);
}

gboolean
clay_dnd_viewable_dest_add (GtkWidget               *widget,
                            GType                    type,
                            ClayDndDropViewableFunc  set_viewable_func,
                            gpointer                 data)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_dest_add (dnd_type, widget,
                          G_CALLBACK (set_viewable_func),
                          data);

  return TRUE;
}

gboolean
clay_dnd_viewable_dest_remove (GtkWidget *widget,
                               GType      type)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_dest_remove (dnd_type, widget);

  return TRUE;
}

ClayViewable *
clay_dnd_get_drag_viewable (GtkWidget *widget)
{
  const ClayDndDataDef    *dnd_data;
  ClayDndType              data_type;
  ClayDndDragViewableFunc  get_data_func = NULL;
  gpointer                 get_data_data = NULL;
  ClayContext             *context;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "clay-dnd-get-data-type"));

  if (! data_type)
    return NULL;

  dnd_data = dnd_data_defs + data_type;

  if (dnd_data->get_data_func_name)
    get_data_func = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_func_name);

  if (dnd_data->get_data_data_name)
    get_data_data = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_data_name);

  if (! get_data_func)
    return NULL;

  return (ClayViewable *) (* get_data_func) (widget, &context, get_data_data);
}


/*************************************************/
/*  ClayViewable (by GType) GList dnd functions  */
/*************************************************/

static GtkWidget *
clay_dnd_get_viewable_list_icon (GtkWidget      *widget,
                                 GdkDragContext *context,
                                 GCallback       get_list_func,
                                 gpointer        get_list_data)
{
  GList        *viewables;
  ClayViewable *viewable;
  ClayContext  *clay_context;
  GtkWidget    *view;
  gchar        *desc;
  gboolean      desc_use_markup = FALSE;
  gfloat        desc_yalign     = 0.5f;
  gint          desc_width_chars;

  viewables = (* (ClayDndDragViewableListFunc) get_list_func) (widget,
                                                               &clay_context,
                                                               get_list_data);

  g_object_set_data_full (G_OBJECT (widget),
                          "clay-dnd-viewables",  viewables,
                          (GDestroyNotify) g_list_free);

  if (! viewables)
    return NULL;

  viewable = viewables->data;

  CLAY_LOG (DND, "viewable %p", viewable);

  g_object_set_data_full (G_OBJECT (context),
                          "clay-dnd-viewable", g_object_ref (viewable),
                          (GDestroyNotify) g_object_unref);

  view = clay_view_new (clay_context, viewable,
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  if (g_list_length (viewables) > 1)
    {
      /* When dragging multiple viewable, just show the first of them in the
       * icon box, and the number of viewables being dragged in the
       * description label (instead of the viewable name).
       */
      desc = g_strdup_printf ("<sup><i>(%d)</i></sup>", g_list_length (viewables));
      desc_use_markup  = TRUE;
      desc_yalign      = 0.0f;
      desc_width_chars = (gint) log10 (g_list_length (viewables)) + 3;
    }
  else
    {
      desc = clay_viewable_get_description (viewable, NULL);
      desc_width_chars =  MIN (strlen (desc), 10);
    }

  if (desc)
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
      gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
      gtk_widget_set_visible (view, TRUE);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label",           desc,
                            "use-markup",      desc_use_markup,
                            "xalign",          0.0,
                            "yalign",          desc_yalign,
                            "max-width-chars", 30,
                            "width-chars",     desc_width_chars,
                            "ellipsize",       PANGO_ELLIPSIZE_END,
                            NULL);

      g_free (desc);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_set_visible (label, TRUE);

      return hbox;
    }

  return view;
}

/**
 * clay_dnd_viewable_list_source_add:
 * @widget:
 * @type:
 * @get_viewable_func:
 * @data:
 *
 * Sets up @widget as a drag source for a #GList of #ClayViewable
 * object, as returned by @get_viewable_func on @widget and @data.
 *
 * @type must be a list type for drag operations (only ClayLayer so
 * far).
 */
gboolean
clay_dnd_viewable_list_source_add (GtkWidget                   *widget,
                                   GType                        type,
                                   ClayDndDragViewableListFunc  get_viewable_list_func,
                                   gpointer                     data)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (get_viewable_list_func != NULL, FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_source_add (dnd_type, widget,
                            G_CALLBACK (get_viewable_list_func),
                            data);

  return TRUE;
}

gboolean
clay_dnd_viewable_list_source_remove (GtkWidget *widget,
                                      GType      type)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  return clay_dnd_data_source_remove (dnd_type, widget);
}

gboolean
clay_dnd_viewable_list_dest_add (GtkWidget                   *widget,
                                 GType                        type,
                                 ClayDndDropViewableListFunc  set_viewable_func,
                                 gpointer                     data)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_dest_add (dnd_type, widget,
                          G_CALLBACK (set_viewable_func),
                          data);

  return TRUE;
}

gboolean
clay_dnd_viewable_list_dest_remove (GtkWidget *widget,
                                    GType      type)
{
  ClayDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = clay_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == CLAY_DND_TYPE_NONE)
    return FALSE;

  clay_dnd_data_dest_remove (dnd_type, widget);

  return TRUE;
}

GList *
clay_dnd_get_drag_list (GtkWidget *widget)
{
  ClayDndType  data_type;
  GList       *viewables;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "clay-dnd-get-data-type"));

  if (! data_type)
    return NULL;

  /* Note: do not use the stored get_data_func_name at the end of a
   * paste, only at drag-begin (handled inside the get_icon_func() call)
   * for item lists, because the content of item container may change
   * during the paste, in particular when moving from one image tab to
   * another. Cf. #7497.
   */
  viewables = g_object_get_data (G_OBJECT (widget), "clay-dnd-viewables");

  return g_list_copy (viewables);
}


/*****************************/
/*  ClayImage dnd functions  */
/*****************************/

static void
clay_dnd_get_image_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_image_func,
                         gpointer          get_image_data,
                         GtkSelectionData *selection)
{
  ClayImage   *image;
  ClayContext *clay_context;

  image = g_object_get_data (G_OBJECT (context), "clay-dnd-viewable");

  if (! image)
    image = (ClayImage *)
      (* (ClayDndDragViewableFunc) get_image_func) (widget, &clay_context,
                                                    get_image_data);

  CLAY_LOG (DND, "image %p", image);

  if (image)
    clay_selection_data_set_image (selection, image);
}

static gboolean
clay_dnd_set_image_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_image_func,
                         gpointer          set_image_data,
                         GtkSelectionData *selection)
{
  ClayImage *image = clay_selection_data_get_image (selection, the_dnd_clay);

  CLAY_LOG (DND, "image %p", image);

  if (! image)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_image_func) (widget, x, y,
                                                CLAY_VIEWABLE (image),
                                                set_image_data);

  return TRUE;
}


/****************************/
/*  ClayItem dnd functions  */
/****************************/

static void
clay_dnd_get_item_data (GtkWidget        *widget,
                        GdkDragContext   *context,
                        GCallback         get_item_func,
                        gpointer          get_item_data,
                        GtkSelectionData *selection)
{
  ClayItem    *item;
  ClayContext *clay_context;

  item = g_object_get_data (G_OBJECT (context), "clay-dnd-viewable");

  if (! item)
    item = (ClayItem *)
      (* (ClayDndDragViewableFunc) get_item_func) (widget, &clay_context,
                                                   get_item_data);

  CLAY_LOG (DND, "item %p", item);

  if (item)
    clay_selection_data_set_item (selection, item);
}

static gboolean
clay_dnd_set_item_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_item_func,
                        gpointer          set_item_data,
                        GtkSelectionData *selection)
{
  ClayItem *item = clay_selection_data_get_item (selection, the_dnd_clay);

  CLAY_LOG (DND, "item %p", item);

  if (! item)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_item_func) (widget, x, y,
                                               CLAY_VIEWABLE (item),
                                               set_item_data);

  return TRUE;
}


/**********************************/
/*  ClayItem GList dnd functions  */
/**********************************/

static void
clay_dnd_get_item_list_data (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GCallback         get_item_func,
                             gpointer          get_item_data,
                             GtkSelectionData *selection)
{
  GList *items;

  /* Note: do not use get_item_func() during a paste, only at drag-begin
   * (handled inside the get_icon_func() call) for item lists, because
   * the content of item container may change during the paste, in
   * particular when moving from one image tab to another. Cf. #7497.
   */
  items = g_object_get_data (G_OBJECT (widget), "clay-dnd-viewables");

  if (items)
    clay_selection_data_set_item_list (selection, items);
}

static gboolean
clay_dnd_set_item_list_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_item_func,
                             gpointer          set_item_data,
                             GtkSelectionData *selection)
{
  GList *items = clay_selection_data_get_item_list (selection, the_dnd_clay);

  if (! items)
    return FALSE;

  (* (ClayDndDropViewableListFunc) set_item_func) (widget, x, y, items,
                                                   set_item_data);
  g_list_free (items);

  return TRUE;
}

/******************************/
/*  ClayObject dnd functions  */
/******************************/

static void
clay_dnd_get_object_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_object_func,
                          gpointer          get_object_data,
                          GtkSelectionData *selection)
{
  ClayObject  *object;
  ClayContext *clay_context;

  object = g_object_get_data (G_OBJECT (context), "clay-dnd-viewable");

  if (! object)
    object = (ClayObject *)
      (* (ClayDndDragViewableFunc) get_object_func) (widget, &clay_context,
                                                     get_object_data);

  CLAY_LOG (DND, "object %p", object);

  if (CLAY_IS_OBJECT (object))
    clay_selection_data_set_object (selection, object);
}


/*****************************/
/*  ClayBrush dnd functions  */
/*****************************/

static gboolean
clay_dnd_set_brush_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_brush_func,
                         gpointer          set_brush_data,
                         GtkSelectionData *selection)
{
  ClayBrush *brush = clay_selection_data_get_brush (selection, the_dnd_clay);

  CLAY_LOG (DND, "brush %p", brush);

  if (! brush)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_brush_func) (widget, x, y,
                                                CLAY_VIEWABLE (brush),
                                                set_brush_data);

  return TRUE;
}


/*******************************/
/*  ClayPattern dnd functions  */
/*******************************/

static gboolean
clay_dnd_set_pattern_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_pattern_func,
                           gpointer          set_pattern_data,
                           GtkSelectionData *selection)
{
  ClayPattern *pattern = clay_selection_data_get_pattern (selection,
                                                          the_dnd_clay);

  CLAY_LOG (DND, "pattern %p", pattern);

  if (! pattern)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_pattern_func) (widget, x, y,
                                                  CLAY_VIEWABLE (pattern),
                                                  set_pattern_data);

  return TRUE;
}


/********************************/
/*  ClayGradient dnd functions  */
/********************************/

static gboolean
clay_dnd_set_gradient_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_gradient_func,
                            gpointer          set_gradient_data,
                            GtkSelectionData *selection)
{
  ClayGradient *gradient = clay_selection_data_get_gradient (selection,
                                                             the_dnd_clay);

  CLAY_LOG (DND, "gradient %p", gradient);

  if (! gradient)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_gradient_func) (widget, x, y,
                                                   CLAY_VIEWABLE (gradient),
                                                   set_gradient_data);

  return TRUE;
}


/*******************************/
/*  ClayPalette dnd functions  */
/*******************************/

static gboolean
clay_dnd_set_palette_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_palette_func,
                           gpointer          set_palette_data,
                           GtkSelectionData *selection)
{
  ClayPalette *palette = clay_selection_data_get_palette (selection,
                                                          the_dnd_clay);

  CLAY_LOG (DND, "palette %p", palette);

  if (! palette)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_palette_func) (widget, x, y,
                                                  CLAY_VIEWABLE (palette),
                                                  set_palette_data);

  return TRUE;
}


/****************************/
/*  ClayFont dnd functions  */
/****************************/

static gboolean
clay_dnd_set_font_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_font_func,
                        gpointer          set_font_data,
                        GtkSelectionData *selection)
{
  ClayFont *font = clay_selection_data_get_font (selection, the_dnd_clay);

  CLAY_LOG (DND, "font %p", font);

  if (! font)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_font_func) (widget, x, y,
                                               CLAY_VIEWABLE (font),
                                               set_font_data);

  return TRUE;
}


/******************************/
/*  ClayBuffer dnd functions  */
/******************************/

static gboolean
clay_dnd_set_buffer_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_buffer_func,
                          gpointer          set_buffer_data,
                          GtkSelectionData *selection)
{
  ClayBuffer *buffer = clay_selection_data_get_buffer (selection, the_dnd_clay);

  CLAY_LOG (DND, "buffer %p", buffer);

  if (! buffer)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_buffer_func) (widget, x, y,
                                                 CLAY_VIEWABLE (buffer),
                                                 set_buffer_data);

  return TRUE;
}


/*********************************/
/*  ClayImagefile dnd functions  */
/*********************************/

static gboolean
clay_dnd_set_imagefile_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_imagefile_func,
                             gpointer          set_imagefile_data,
                             GtkSelectionData *selection)
{
  ClayImagefile *imagefile = clay_selection_data_get_imagefile (selection,
                                                                the_dnd_clay);

  CLAY_LOG (DND, "imagefile %p", imagefile);

  if (! imagefile)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_imagefile_func) (widget, x, y,
                                                    CLAY_VIEWABLE (imagefile),
                                                    set_imagefile_data);

  return TRUE;
}


/********************************/
/*  ClayTemplate dnd functions  */
/********************************/

static gboolean
clay_dnd_set_template_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_template_func,
                            gpointer          set_template_data,
                            GtkSelectionData *selection)
{
  ClayTemplate *template = clay_selection_data_get_template (selection,
                                                             the_dnd_clay);

  CLAY_LOG (DND, "template %p", template);

  if (! template)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_template_func) (widget, x, y,
                                                   CLAY_VIEWABLE (template),
                                                   set_template_data);

  return TRUE;
}


/*********************************/
/*  ClayToolEntry dnd functions  */
/*********************************/

static gboolean
clay_dnd_set_tool_item_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_tool_item_func,
                             gpointer          set_tool_item_data,
                             GtkSelectionData *selection)
{
  ClayToolItem *tool_item = clay_selection_data_get_tool_item (selection,
                                                               the_dnd_clay);

  CLAY_LOG (DND, "tool_item %p", tool_item);

  if (! tool_item)
    return FALSE;

  (* (ClayDndDropViewableFunc) set_tool_item_func) (widget, x, y,
                                                    CLAY_VIEWABLE (tool_item),
                                                    set_tool_item_data);

  return TRUE;
}
