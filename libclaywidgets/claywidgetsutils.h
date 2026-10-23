/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claywidgetsutils.h
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_WIDGETS_UTILS_H__
#define __CLAY_WIDGETS_UTILS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean             clay_event_triggers_context_menu (const GdkEvent   *event,
                                                       gboolean          on_release);

GtkWidget          * clay_grid_attach_aligned        (GtkGrid           *grid,
                                                      gint               left,
                                                      gint               top,
                                                      const gchar       *label_text,
                                                      gfloat             label_xalign,
                                                      gfloat             label_yalign,
                                                      GtkWidget         *widget,
                                                      gint               widget_columns);

void                 clay_label_set_attributes       (GtkLabel          *label,
                                                      ...);

GdkMonitor         * clay_widget_get_monitor         (GtkWidget         *widget);
GdkMonitor         * clay_get_monitor_at_pointer     (void);

void                 clay_widget_track_monitor       (GtkWidget         *widget,
                                                      GCallback          monitor_changed_callback,
                                                      gpointer           user_data,
                                                      GDestroyNotify     user_data_destroy);

ClayColorProfile   * clay_monitor_get_color_profile  (GdkMonitor        *monitor);
ClayColorProfile   * clay_widget_get_color_profile   (GtkWidget         *widget);

ClayColorTransform * clay_widget_get_color_transform (GtkWidget         *widget,
                                                      ClayColorConfig   *config,
                                                      ClayColorProfile  *src_profile,
                                                      const Babl        *src_format,
                                                      const Babl        *dest_format,
                                                      ClayColorProfile  *softproof_profile,
                                                      ClayColorRenderingIntent proof_intent,
                                                      gboolean           proof_bpc);
const Babl         * clay_widget_get_render_space    (GtkWidget       *widget,
                                                      ClayColorConfig *config);

void                 clay_widget_set_native_handle   (GtkWidget        *widget,
                                                      GBytes          **window_handle);
void                 clay_widget_free_native_handle  (GtkWidget        *widget,
                                                      GBytes          **window_handle);

gboolean             clay_widget_animation_enabled   (void);

/* Internal use */

G_GNUC_INTERNAL void _clay_widget_get_profiles       (GtkWidget         *widget,
                                                      ClayColorConfig   *config,
                                                      ClayColorProfile **proof_profile,
                                                      ClayColorProfile **dest_profile);


G_END_DECLS

#endif /* __CLAY_WIDGETS_UTILS_H__ */
