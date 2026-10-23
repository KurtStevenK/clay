/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core/claydisplay.h"


#define CLAY_TYPE_DISPLAY_IMPL            (clay_display_impl_get_type ())
#define CLAY_DISPLAY_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DISPLAY_IMPL, ClayDisplayImpl))
#define CLAY_DISPLAY_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DISPLAY_IMPL, ClayDisplayImplClass))
#define CLAY_IS_DISPLAY_IMPL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DISPLAY_IMPL))
#define CLAY_IS_DISPLAY_IMPL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DISPLAY_IMPL))
#define CLAY_DISPLAY_IMPL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DISPLAY_IMPL, ClayDisplayImplClass))


typedef struct _ClayDisplayImpl        ClayDisplayImpl;
typedef struct _ClayDisplayImplClass   ClayDisplayImplClass;
typedef struct _ClayDisplayImplPrivate ClayDisplayImplPrivate;


struct _ClayDisplayImpl
{
  ClayDisplay             parent_instance;

  ClayDisplayImplPrivate *priv;

};

struct _ClayDisplayImplClass
{
  ClayDisplayClass  parent_class;
};


GType              clay_display_impl_get_type   (void);

ClayDisplay      * clay_display_new             (Clay              *clay,
                                                 ClayImage         *image,
                                                 ClayUnit          *unit,
                                                 gdouble            scale,
                                                 ClayUIManager     *popup_manager,
                                                 ClayDialogFactory *dialog_factory,
                                                 GdkMonitor        *monitor);
void               clay_display_delete          (ClayDisplay       *display);
void               clay_display_close           (ClayDisplay       *display);

gchar            * clay_display_get_action_name (ClayDisplay       *display);

ClayImage        * clay_display_get_image       (ClayDisplay       *display);
void               clay_display_set_image       (ClayDisplay       *display,
                                                 ClayImage         *image);

gint               clay_display_get_instance    (ClayDisplay       *display);

ClayDisplayShell * clay_display_get_shell       (ClayDisplay       *display);

void               clay_display_empty           (ClayDisplay       *display);
void               clay_display_fill            (ClayDisplay       *display,
                                                 ClayImage         *image,
                                                 ClayUnit          *unit,
                                                 gdouble            scale);

void               clay_display_update_bounding_box
                                                (ClayDisplay       *display);

void               clay_display_update_area     (ClayDisplay       *display,
                                                 gboolean           now,
                                                 gint               x,
                                                 gint               y,
                                                 gint               w,
                                                 gint               h);

void               clay_display_flush           (ClayDisplay       *display);
void               clay_display_flush_now       (ClayDisplay       *display);
