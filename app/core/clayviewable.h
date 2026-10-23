/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayviewable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "clayobject.h"


#define CLAY_VIEWABLE_MAX_PREVIEW_SIZE 8192
#define CLAY_VIEWABLE_MAX_POPUP_SIZE    256
#define CLAY_VIEWABLE_MAX_BUTTON_SIZE  CLAY_VIEW_SIZE_GIGANTIC
#define CLAY_VIEWABLE_MAX_MENU_SIZE      48


#define CLAY_TYPE_VIEWABLE (clay_viewable_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayViewable,
                          clay_viewable,
                          CLAY, VIEWABLE,
                          ClayObject)


struct _ClayViewableClass
{
  ClayObjectClass  parent_class;

  const gchar     *default_icon_name;
  const gchar     *default_name;
  const gchar     *name_changed_signal;
  gboolean         name_editable;

  /*  signals  */
  void            (* invalidate_preview) (ClayViewable  *viewable);
  void            (* size_changed)       (ClayViewable  *viewable);
  void            (* expanded_changed)   (ClayViewable  *viewable);
  void            (* ancestry_changed)   (ClayViewable  *viewable);

  /*  virtual functions  */
  gboolean        (* get_size)           (ClayViewable  *viewable,
                                          gint          *width,
                                          gint          *height);
  void            (* get_preview_size)   (ClayViewable  *viewable,
                                          gint           size,
                                          gboolean       is_popup,
                                          gboolean       dot_for_dot,
                                          gint          *width,
                                          gint          *height);
  gboolean        (* get_popup_size)     (ClayViewable  *viewable,
                                          gint           width,
                                          gint           height,
                                          gboolean       dot_for_dot,
                                          gint          *popup_width,
                                          gint          *popup_height);
  ClayTempBuf   * (* get_preview)        (ClayViewable  *viewable,
                                          ClayContext   *context,
                                          gint           width,
                                          gint           height,
                                          gint           scale_factor,
                                          GeglColor     *fg_color);
  ClayTempBuf   * (* get_new_preview)    (ClayViewable  *viewable,
                                          ClayContext   *context,
                                          gint           width,
                                          gint           height,
                                          gint           scale_factor,
                                          GeglColor     *fg_color);
  GdkPixbuf     * (* get_pixbuf)         (ClayViewable  *viewable,
                                          ClayContext   *context,
                                          gint           width,
                                          gint           height,
                                          gint           scale_factor,
                                          GeglColor     *fg_color);
  GdkPixbuf     * (* get_new_pixbuf)     (ClayViewable  *viewable,
                                          ClayContext   *context,
                                          gint           width,
                                          gint           height,
                                          gint           scale_factor,
                                          GeglColor     *fg_color);
  gchar         * (* get_description)    (ClayViewable  *viewable,
                                          gchar        **tooltip);

  gboolean        (* is_name_editable)   (ClayViewable  *viewable);

  void            (* preview_freeze)     (ClayViewable  *viewable);
  void            (* preview_thaw)       (ClayViewable  *viewable);

  ClayContainer * (* get_children)       (ClayViewable  *viewable);

  void            (* set_expanded)       (ClayViewable  *viewable,
                                          gboolean       expand);
  gboolean        (* get_expanded)       (ClayViewable  *viewable);
};


void            clay_viewable_invalidate_preview (ClayViewable  *viewable);
void            clay_viewable_size_changed       (ClayViewable  *viewable);
void            clay_viewable_expanded_changed   (ClayViewable  *viewable);

void            clay_viewable_calc_preview_size  (gint           aspect_width,
                                                  gint           aspect_height,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gdouble        xresolution,
                                                  gdouble        yresolution,
                                                  gint          *return_width,
                                                  gint          *return_height,
                                                  gboolean      *scaling_up);

gboolean        clay_viewable_has_preview        (ClayViewable  *viewable);
gboolean        clay_viewable_get_size           (ClayViewable  *viewable,
                                                  gint          *width,
                                                  gint          *height);
void            clay_viewable_get_preview_size   (ClayViewable  *viewable,
                                                  gint           size,
                                                  gboolean       popup,
                                                  gboolean       dot_for_dot,
                                                  gint          *width,
                                                  gint          *height);
gboolean        clay_viewable_get_popup_size     (ClayViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gboolean       dot_for_dot,
                                                  gint          *popup_width,
                                                  gint          *popup_height);

ClayTempBuf   * clay_viewable_get_preview        (ClayViewable  *viewable,
                                                  ClayContext   *context,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  GeglColor     *fg_color);
ClayTempBuf   * clay_viewable_get_new_preview    (ClayViewable  *viewable,
                                                  ClayContext   *context,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  GeglColor     *fg_color);

ClayTempBuf   * clay_viewable_get_dummy_preview  (ClayViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  const Babl    *format);

GdkPixbuf     * clay_viewable_get_pixbuf         (ClayViewable  *viewable,
                                                  ClayContext   *context,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  GeglColor     *fg_color);
GdkPixbuf     * clay_viewable_get_new_pixbuf     (ClayViewable  *viewable,
                                                  ClayContext   *context,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  GeglColor     *fg_color);

GdkPixbuf     * clay_viewable_get_dummy_pixbuf   (ClayViewable  *viewable,
                                                  gint           width,
                                                  gint           height,
                                                  gint           scale_factor,
                                                  gboolean       with_alpha);

gchar         * clay_viewable_get_description    (ClayViewable  *viewable,
                                                  gchar        **tooltip);

gboolean        clay_viewable_is_name_editable   (ClayViewable  *viewable);

const gchar   * clay_viewable_get_icon_name      (ClayViewable  *viewable);
void            clay_viewable_set_icon_name      (ClayViewable  *viewable,
                                                  const gchar   *icon_name);

void            clay_viewable_preview_freeze     (ClayViewable  *viewable);
void            clay_viewable_preview_thaw       (ClayViewable  *viewable);
gboolean        clay_viewable_preview_is_frozen  (ClayViewable  *viewable);

ClayViewable  * clay_viewable_get_parent         (ClayViewable  *viewable);
void            clay_viewable_set_parent         (ClayViewable  *viewable,
                                                  ClayViewable  *parent);

gint            clay_viewable_get_depth          (ClayViewable  *viewable);

ClayContainer * clay_viewable_get_children       (ClayViewable  *viewable);

gboolean        clay_viewable_get_expanded       (ClayViewable  *viewable);
void            clay_viewable_set_expanded       (ClayViewable  *viewable,
                                                  gboolean       expanded);

gboolean        clay_viewable_is_ancestor        (ClayViewable  *ancestor,
                                                  ClayViewable  *descendant);
