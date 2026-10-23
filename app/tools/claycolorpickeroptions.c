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

#include "widgets/claywidgets-utils.h"

#include "claycolorpickeroptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_AVERAGE, /* overrides a ClayColorOptions property */
  PROP_PICK_TARGET,
  PROP_USE_INFO_WINDOW,
  PROP_FRAME1_MODE,
  PROP_FRAME2_MODE
};


static void   clay_color_picker_options_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void   clay_color_picker_options_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);


G_DEFINE_TYPE (ClayColorPickerOptions, clay_color_picker_options,
               CLAY_TYPE_COLOR_OPTIONS)


static void
clay_color_picker_options_class_init (ClayColorPickerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_color_picker_options_set_property;
  object_class->get_property = clay_color_picker_options_get_property;

  /* override a ClayColorOptions property to get a different default value */
  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                            "sample-average",
                            _("Sample average"),
                            _("Use averaged color value from "
                              "nearby pixels"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_PICK_TARGET,
                         "pick-target",
                         _("Pick Target"),
                         _("Choose what the color picker will do"),
                         CLAY_TYPE_COLOR_PICK_TARGET,
                         CLAY_COLOR_PICK_TARGET_FOREGROUND,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                            "use-info-window",
                            _("Use info window"),
                            _("Open a floating dialog to view picked "
                              "color values in various color models"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FRAME1_MODE,
                         "frame1-mode",
                         "Frame 1 Mode", NULL,
                         CLAY_TYPE_COLOR_PICK_MODE,
                         CLAY_COLOR_PICK_MODE_PIXEL,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FRAME2_MODE,
                         "frame2-mode",
                         "Frame 2 Mode", NULL,
                         CLAY_TYPE_COLOR_PICK_MODE,
                         CLAY_COLOR_PICK_MODE_RGB_PERCENT,
                         CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_color_picker_options_init (ClayColorPickerOptions *options)
{
}

static void
clay_color_picker_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClayColorPickerOptions *options = CLAY_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      CLAY_COLOR_OPTIONS (options)->sample_average = g_value_get_boolean (value);
      break;
    case PROP_PICK_TARGET:
      options->pick_target = g_value_get_enum (value);
      break;
    case PROP_USE_INFO_WINDOW:
      options->use_info_window = g_value_get_boolean (value);
      break;
    case PROP_FRAME1_MODE:
      options->frame1_mode = g_value_get_enum (value);
      break;
    case PROP_FRAME2_MODE:
      options->frame2_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_picker_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClayColorPickerOptions *options = CLAY_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value,
                           CLAY_COLOR_OPTIONS (options)->sample_average);
      break;
    case PROP_PICK_TARGET:
      g_value_set_enum (value, options->pick_target);
      break;
    case PROP_USE_INFO_WINDOW:
      g_value_set_boolean (value, options->use_info_window);
      break;
    case PROP_FRAME1_MODE:
      g_value_set_enum (value, options->frame1_mode);
      break;
    case PROP_FRAME2_MODE:
      g_value_set_enum (value, options->frame2_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_color_picker_options_gui (ClayToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = clay_color_options_gui (tool_options);
  GtkWidget       *button;
  GtkWidget       *frame;
  gchar           *str;
  GdkModifierType  extend_mask = clay_get_extend_selection_mask ();
  GdkModifierType  toggle_mask = clay_get_toggle_behavior_mask ();

  /*  the pick FG/BG frame  */
  str = g_strdup_printf (_("Pick Target  (%s)"),
                         clay_get_mod_string (toggle_mask));
  frame = clay_prop_enum_radio_frame_new (config, "pick-target", str, -1, -1);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  g_free (str);

  /*  the use_info_window toggle button  */
  str = g_strdup_printf (_("Use info window  (%s)"),
                         clay_get_mod_string (extend_mask));
  button = clay_prop_check_button_new (config, "use-info-window", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_free (str);

  return vbox;
}
