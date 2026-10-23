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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "paint/claypaintoptions.h"

#include "clay.h"
#include "claypaintinfo.h"


static void    clay_paint_info_dispose         (GObject       *object);
static void    clay_paint_info_finalize        (GObject       *object);
static gchar * clay_paint_info_get_description (ClayViewable  *viewable,
                                                gchar        **tooltip);


G_DEFINE_TYPE (ClayPaintInfo, clay_paint_info, CLAY_TYPE_VIEWABLE)

#define parent_class clay_paint_info_parent_class


static void
clay_paint_info_class_init (ClayPaintInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->dispose           = clay_paint_info_dispose;
  object_class->finalize          = clay_paint_info_finalize;

  viewable_class->get_description = clay_paint_info_get_description;
}

static void
clay_paint_info_init (ClayPaintInfo *paint_info)
{
  paint_info->clay          = NULL;
  paint_info->paint_type    = G_TYPE_NONE;
  paint_info->blurb         = NULL;
  paint_info->paint_options = NULL;
}

static void
clay_paint_info_dispose (GObject *object)
{
  ClayPaintInfo *paint_info = CLAY_PAINT_INFO (object);

  if (paint_info->paint_options)
    {
      g_object_run_dispose (G_OBJECT (paint_info->paint_options));
      g_clear_object (&paint_info->paint_options);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_paint_info_finalize (GObject *object)
{
  ClayPaintInfo *paint_info = CLAY_PAINT_INFO (object);

  g_clear_pointer (&paint_info->blurb, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
clay_paint_info_get_description (ClayViewable  *viewable,
                                 gchar        **tooltip)
{
  ClayPaintInfo *paint_info = CLAY_PAINT_INFO (viewable);

  return g_strdup (paint_info->blurb);
}

ClayPaintInfo *
clay_paint_info_new (Clay        *clay,
                     GType        paint_type,
                     GType        paint_options_type,
                     const gchar *identifier,
                     const gchar *blurb,
                     const gchar *icon_name)
{
  ClayPaintInfo *paint_info;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  paint_info = g_object_new (CLAY_TYPE_PAINT_INFO,
                             "name",      identifier,
                             "icon-name", icon_name,
                             NULL);

  paint_info->clay               = clay;
  paint_info->paint_type         = paint_type;
  paint_info->paint_options_type = paint_options_type;
  paint_info->blurb              = g_strdup (blurb);

  paint_info->paint_options      = clay_paint_options_new (paint_info);

  return paint_info;
}

void
clay_paint_info_set_standard (Clay          *clay,
                              ClayPaintInfo *paint_info)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (! paint_info || CLAY_IS_PAINT_INFO (paint_info));

  g_set_object (&clay->standard_paint_info, paint_info);
}

ClayPaintInfo *
clay_paint_info_get_standard (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return clay->standard_paint_info;
}
