/* xyz.kainzmayer.ClayDesign
 *
 * claycageoptions.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "claycageoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CAGE_MODE,
  PROP_FILL_PLAIN_COLOR
};


static void   clay_cage_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   clay_cage_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (ClayCageOptions, clay_cage_options,
               CLAY_TYPE_TOOL_OPTIONS)

#define parent_class clay_cage_options_parent_class


static void
clay_cage_options_class_init (ClayCageOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_cage_options_set_property;
  object_class->get_property = clay_cage_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CAGE_MODE,
                         "cage-mode",
                         NULL, NULL,
                         CLAY_TYPE_CAGE_MODE,
                         CLAY_CAGE_MODE_CAGE_CHANGE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_PLAIN_COLOR,
                            "fill-plain-color",
                            _("Fill the original position\n"
                              "of the cage with a color"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_cage_options_init (ClayCageOptions *options)
{
}

static void
clay_cage_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayCageOptions *options = CLAY_CAGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAGE_MODE:
      options->cage_mode = g_value_get_enum (value);
      break;
    case PROP_FILL_PLAIN_COLOR:
      options->fill_plain_color = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_cage_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayCageOptions *options = CLAY_CAGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAGE_MODE:
      g_value_set_enum (value, options->cage_mode);
      break;
    case PROP_FILL_PLAIN_COLOR:
      g_value_set_boolean (value, options->fill_plain_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_cage_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_tool_options_gui (tool_options);
  GtkWidget *mode;
  GtkWidget *button;

  mode = clay_prop_enum_radio_box_new (config, "cage-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);

  button = clay_prop_check_button_new (config, "fill-plain-color", NULL);
  gtk_box_pack_start (GTK_BOX (vbox),  button, FALSE, FALSE, 0);

  return vbox;
}
