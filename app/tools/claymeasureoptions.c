/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymeasuretool.c
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "widgets/claywidgets-utils.h"

#include "claymeasureoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_USE_INFO_WINDOW
};


static void   clay_measure_options_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   clay_measure_options_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE (ClayMeasureOptions, clay_measure_options,
               CLAY_TYPE_TRANSFORM_OPTIONS)


static void
clay_measure_options_class_init (ClayMeasureOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_measure_options_set_property;
  object_class->get_property = clay_measure_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         _("Orientation"),
                         _("Orientation against which the angle is measured"),
                         CLAY_TYPE_COMPASS_ORIENTATION,
                         CLAY_COMPASS_ORIENTATION_AUTO,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                            "use-info-window",
                            _("Use info window"),
                            _("Open a floating dialog to view details "
                              "about measurements"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_measure_options_init (ClayMeasureOptions *options)
{
}

static void
clay_measure_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayMeasureOptions *options = CLAY_MEASURE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      options->orientation = g_value_get_enum (value);
      break;
    case PROP_USE_INFO_WINDOW:
      options->use_info_window = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_measure_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayMeasureOptions *options = CLAY_MEASURE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, options->orientation);
      break;
    case PROP_USE_INFO_WINDOW:
      g_value_set_boolean (value, options->use_info_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_measure_options_gui (ClayToolOptions *tool_options)
{
  GObject            *config  = G_OBJECT (tool_options);
  ClayMeasureOptions *options = CLAY_MEASURE_OPTIONS (tool_options);
  GtkWidget          *vbox    = clay_tool_options_gui (tool_options);
  GtkWidget          *frame;
  GtkWidget          *button;
  GtkWidget          *vbox2;
  gchar              *str;
  GdkModifierType     toggle_mask = clay_get_toggle_behavior_mask ();

  /*  the orientation frame  */
  str = g_strdup_printf (_("Orientation  (%s)"),
                         clay_get_mod_string (toggle_mask));
  frame = clay_prop_enum_radio_frame_new (config, "orientation", str, -1, -1);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /*  the use_info_window toggle button  */
  button = clay_prop_check_button_new (config, "use-info-window", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  /*  the straighten frame  */
  frame = clay_frame_new (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /*  the transform options  */
  vbox2 = clay_transform_options_gui (tool_options, FALSE, TRUE, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  /*  the straighten button  */
  button = gtk_button_new_with_label (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  clay_help_set_help_data (button,
                           _("Rotate the active layer, selection or path "
                             "by the measured angle"),
                           NULL);
  gtk_widget_set_visible (button, TRUE);

  options->straighten_button = button;

  return vbox;
}
