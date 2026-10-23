/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "widgets/claypropwidgets.h"

#include "claycoloroptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED,
  PROP_SAMPLE_AVERAGE,
  PROP_AVERAGE_RADIUS
};


static void   clay_color_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   clay_color_options_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


G_DEFINE_TYPE (ClayColorOptions, clay_color_options,
               CLAY_TYPE_TOOL_OPTIONS)


static void
clay_color_options_class_init (ClayColorOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_color_options_set_property;
  object_class->get_property = clay_color_options_get_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            _("Use merged color value from "
                              "all composited visible layers"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                            "sample-average",
                            _("Sample average"),
                            _("Use averaged color value from "
                              "nearby pixels"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_AVERAGE_RADIUS,
                           "average-radius",
                           _("Radius"),
                           _("Color Picker Average Radius"),
                           1.0, 300.0, 3.0,
                           CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_color_options_init (ClayColorOptions *options)
{
}

static void
clay_color_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayColorOptions *options = CLAY_COLOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_AVERAGE:
      options->sample_average = g_value_get_boolean (value);
      break;
    case PROP_AVERAGE_RADIUS:
      options->average_radius = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayColorOptions *options = CLAY_COLOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value, options->sample_average);
      break;
    case PROP_AVERAGE_RADIUS:
      g_value_set_double (value, options->average_radius);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_color_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_tool_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *scale;

  /*  the sample average options  */
  scale = clay_prop_spin_scale_new (config, "average-radius",
                                    1.0, 10.0, 0);

  frame = clay_prop_expanding_frame_new (config, "sample-average", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  /* The Sample merged checkbox. */
  button = clay_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  return vbox;
}
