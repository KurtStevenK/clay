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

#pragma once


#define CLAY_TARGET_URI_LIST \
        { "text/uri-list", 0, CLAY_DND_TYPE_URI_LIST }

#define CLAY_TARGET_TEXT_PLAIN \
        { "text/plain", 0, CLAY_DND_TYPE_TEXT_PLAIN }

#define CLAY_TARGET_NETSCAPE_URL \
        { "_NETSCAPE_URL", 0, CLAY_DND_TYPE_NETSCAPE_URL }

#define CLAY_TARGET_XDS \
        { "XdndDirectSave0", 0, CLAY_DND_TYPE_XDS }

#define CLAY_TARGET_COLOR \
        { "application/x-geglcolor", 0, CLAY_DND_TYPE_COLOR }

#define CLAY_TARGET_SVG \
        { "image/svg", 0, CLAY_DND_TYPE_SVG }

#define CLAY_TARGET_SVG_XML \
        { "image/svg+xml", 0, CLAY_DND_TYPE_SVG_XML }

/* just here for documentation purposes, the actual list of targets
 * is created dynamically from available GdkPixbuf loaders
 */
#define CLAY_TARGET_PIXBUF \
        { NULL, 0, CLAY_DND_TYPE_PIXBUF }

#define CLAY_TARGET_IMAGE \
        { "application/x-clay-image-id", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_IMAGE }

#define CLAY_TARGET_COMPONENT \
        { "application/x-clay-component", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_COMPONENT }

#define CLAY_TARGET_LAYER \
        { "application/x-clay-layer-id", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_LAYER }

#define CLAY_TARGET_CHANNEL \
        { "application/x-clay-channel-id", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_CHANNEL }

#define CLAY_TARGET_LAYER_MASK \
        { "application/x-clay-layer-mask-id", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_LAYER_MASK }

#define CLAY_TARGET_PATH \
        { "application/x-clay-path-id", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_PATH }

#define CLAY_TARGET_BRUSH \
        { "application/x-clay-brush-name", 0, CLAY_DND_TYPE_BRUSH }

#define CLAY_TARGET_PATTERN \
        { "application/x-clay-pattern-name", 0, CLAY_DND_TYPE_PATTERN }

#define CLAY_TARGET_GRADIENT \
        { "application/x-clay-gradient-name", 0, CLAY_DND_TYPE_GRADIENT }

#define CLAY_TARGET_PALETTE \
        { "application/x-clay-palette-name", 0, CLAY_DND_TYPE_PALETTE }

#define CLAY_TARGET_FONT \
        { "application/x-clay-font-name", 0, CLAY_DND_TYPE_FONT }

#define CLAY_TARGET_BUFFER \
        { "application/x-clay-buffer-name", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_BUFFER }

#define CLAY_TARGET_IMAGEFILE \
        { "application/x-clay-imagefile-name", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_IMAGEFILE }

#define CLAY_TARGET_TEMPLATE \
        { "application/x-clay-template-name", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_TEMPLATE }

#define CLAY_TARGET_TOOL_ITEM \
        { "application/x-clay-tool-item-name", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_TOOL_ITEM }

#define CLAY_TARGET_NOTEBOOK_TAB \
        { "GTK_NOTEBOOK_TAB", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_NOTEBOOK_TAB }

#define CLAY_TARGET_LAYER_LIST \
        { "application/x-clay-layer-list", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_LAYER_LIST }

#define CLAY_TARGET_CHANNEL_LIST \
        { "application/x-clay-channel-list", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_CHANNEL_LIST }

#define CLAY_TARGET_PATH_LIST \
        { "application/x-clay-path-list", GTK_TARGET_SAME_APP, CLAY_DND_TYPE_PATH_LIST }


/*  dnd initialization  */

void  clay_dnd_init (Clay *clay);


/*  uri list dnd functions  */

typedef GList * (* ClayDndDragUriListFunc) (GtkWidget *widget,
                                            gpointer   data);
typedef void    (* ClayDndDropUriListFunc) (GtkWidget *widget,
                                            gint       x,
                                            gint       y,
                                            GList     *uri_list,
                                            gpointer   data);

void  clay_dnd_uri_list_source_add    (GtkWidget              *widget,
                                       ClayDndDragUriListFunc  get_uri_list_func,
                                       gpointer                data);
void  clay_dnd_uri_list_source_remove (GtkWidget              *widget);

void  clay_dnd_uri_list_dest_add      (GtkWidget              *widget,
                                       ClayDndDropUriListFunc  set_uri_list_func,
                                       gpointer                data);
void  clay_dnd_uri_list_dest_remove   (GtkWidget              *widget);


/*  color dnd functions  */

typedef void (* ClayDndDragColorFunc) (GtkWidget     *widget,
                                       GeglColor    **color,
                                       gpointer       data);
typedef void (* ClayDndDropColorFunc) (GtkWidget     *widget,
                                       gint           x,
                                       gint           y,
                                       GeglColor     *color,
                                       gpointer       data);

void  clay_dnd_color_source_add    (GtkWidget            *widget,
                                    ClayDndDragColorFunc  get_color_func,
                                    gpointer              data);
void  clay_dnd_color_source_remove (GtkWidget            *widget);

void  clay_dnd_color_dest_add      (GtkWidget            *widget,
                                    ClayDndDropColorFunc  set_color_func,
                                    gpointer              data);
void  clay_dnd_color_dest_remove   (GtkWidget            *widget);


/*  stream dnd functions  */

typedef guchar * (* ClayDndDragStreamFunc) (GtkWidget    *widget,
                                            gsize        *stream_len,
                                            gpointer      data);
typedef void     (* ClayDndDropStreamFunc) (GtkWidget    *widget,
                                            gint          x,
                                            gint          y,
                                            const guchar *stream,
                                            gsize         stream_len,
                                            gpointer      data);

void  clay_dnd_svg_source_add    (GtkWidget              *widget,
                                  ClayDndDragStreamFunc   get_svg_func,
                                  gpointer                data);
void  clay_dnd_svg_source_remove (GtkWidget              *widget);

void  clay_dnd_svg_dest_add      (GtkWidget              *widget,
                                  ClayDndDropStreamFunc   set_svg_func,
                                  gpointer                data);
void  clay_dnd_svg_dest_remove   (GtkWidget              *widget);


/*  pixbuf dnd functions  */

typedef GdkPixbuf * (* ClayDndDragPixbufFunc) (GtkWidget    *widget,
                                               gpointer      data);
typedef void        (* ClayDndDropPixbufFunc) (GtkWidget    *widget,
                                               gint          x,
                                               gint          y,
                                               GdkPixbuf    *pixbuf,
                                               gpointer      data);

void  clay_dnd_pixbuf_source_add    (GtkWidget              *widget,
                                     ClayDndDragPixbufFunc   get_pixbuf_func,
                                     gpointer                data);
void  clay_dnd_pixbuf_source_remove (GtkWidget              *widget);

void  clay_dnd_pixbuf_dest_add      (GtkWidget              *widget,
                                     ClayDndDropPixbufFunc   set_pixbuf_func,
                                     gpointer                data);
void  clay_dnd_pixbuf_dest_remove   (GtkWidget              *widget);


/*  component dnd functions  */

typedef ClayImage * (* ClayDndDragComponentFunc) (GtkWidget       *widget,
                                                  ClayContext    **context,
                                                  ClayChannelType *channel,
                                                  gpointer         data);
typedef void        (* ClayDndDropComponentFunc) (GtkWidget       *widget,
                                                  gint             x,
                                                  gint             y,
                                                  ClayImage       *image,
                                                  ClayChannelType  channel,
                                                  gpointer         data);

void  clay_dnd_component_source_add    (GtkWidget                 *widget,
                                        ClayDndDragComponentFunc   get_comp_func,
                                        gpointer                   data);
void  clay_dnd_component_source_remove (GtkWidget                 *widget);

void  clay_dnd_component_dest_add      (GtkWidget                 *widget,
                                        ClayDndDropComponentFunc   set_comp_func,
                                        gpointer                   data);
void  clay_dnd_component_dest_remove   (GtkWidget                 *widget);


/*  ClayViewable (by GType) dnd functions  */

typedef ClayViewable * (* ClayDndDragViewableFunc) (GtkWidget     *widget,
                                                    ClayContext  **context,
                                                    gpointer       data);
typedef void           (* ClayDndDropViewableFunc) (GtkWidget     *widget,
                                                    gint           x,
                                                    gint           y,
                                                    ClayViewable  *viewable,
                                                    gpointer       data);


gboolean clay_dnd_drag_source_set_by_type (GtkWidget               *widget,
                                           GdkModifierType          start_button_mask,
                                           GType                    type,
                                           GdkDragAction            actions);
gboolean clay_dnd_viewable_source_add     (GtkWidget               *widget,
                                           GType                    type,
                                           ClayDndDragViewableFunc  get_viewable_func,
                                           gpointer                 data);
gboolean clay_dnd_viewable_source_remove  (GtkWidget               *widget,
                                           GType                    type);

gboolean clay_dnd_drag_dest_set_by_type   (GtkWidget               *widget,
                                           GtkDestDefaults          flags,
                                           GType                    type,
                                           gboolean                 list_accepted,
                                           GdkDragAction            actions);

gboolean clay_dnd_viewable_dest_add       (GtkWidget               *widget,
                                           GType                    type,
                                           ClayDndDropViewableFunc  set_viewable_func,
                                           gpointer                 data);
gboolean clay_dnd_viewable_dest_remove    (GtkWidget               *widget,
                                           GType                    type);

ClayViewable * clay_dnd_get_drag_viewable (GtkWidget               *widget);

/*  ClayViewable (by GType) GList dnd functions  */

typedef GList * (* ClayDndDragViewableListFunc) (GtkWidget     *widget,
                                                 ClayContext  **context,
                                                 gpointer       data);
typedef void    (* ClayDndDropViewableListFunc) (GtkWidget     *widget,
                                                 gint           x,
                                                 gint           y,
                                                 GList         *viewables,
                                                 gpointer       data);

gboolean   clay_dnd_viewable_list_source_add    (GtkWidget                   *widget,
                                                 GType                        type,
                                                 ClayDndDragViewableListFunc  get_viewable_list_func,
                                                 gpointer                     data);
gboolean   clay_dnd_viewable_list_source_remove (GtkWidget                   *widget,
                                                 GType                        type);
gboolean   clay_dnd_viewable_list_dest_add      (GtkWidget                   *widget,
                                                 GType                        type,
                                                 ClayDndDropViewableListFunc  set_viewable_func,
                                                 gpointer                     data);
gboolean   clay_dnd_viewable_list_dest_remove   (GtkWidget                   *widget,
                                                 GType                        type);
GList    * clay_dnd_get_drag_list               (GtkWidget                   *widget);

/*  Direct Save Protocol (XDS)  */

void  clay_dnd_xds_source_add    (GtkWidget               *widget,
                                  ClayDndDragViewableFunc  get_image_func,
                                  gpointer                 data);
void  clay_dnd_xds_source_remove (GtkWidget               *widget);
