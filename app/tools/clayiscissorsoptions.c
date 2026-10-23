/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "clayiscissorstool.h"
#include "clayiscissorsoptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_INTERACTIVE
};


static void   clay_iscissors_options_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   clay_iscissors_options_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);


G_DEFINE_TYPE (ClayIscissorsOptions, clay_iscissors_options,
               CLAY_TYPE_SELECTION_OPTIONS)

#define parent_class clay_iscissors_options_parent_class


static void
clay_iscissors_options_class_init (ClayIscissorsOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_iscissors_options_set_property;
  object_class->get_property = clay_iscissors_options_get_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_INTERACTIVE,
                            "interactive",
                            _("Interactive boundary"),
                            _("Display future selection segment "
                              "as you drag a control node"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_iscissors_options_init (ClayIscissorsOptions *options)
{
}

static void
clay_iscissors_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayIscissorsOptions *options = CLAY_ISCISSORS_OPTIONS (object);

  switch (property_id)
    {
    case PROP_INTERACTIVE:
      options->interactive = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_iscissors_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayIscissorsOptions *options = CLAY_ISCISSORS_OPTIONS (object);

  switch (property_id)
    {
    case PROP_INTERACTIVE:
      g_value_set_boolean (value, options->interactive);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_iscissors_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config  = G_OBJECT (tool_options);
  GtkWidget *vbox    = clay_selection_options_gui (tool_options);
  GtkWidget *button;

  button = clay_prop_check_button_new (config, "interactive", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  return vbox;
}
