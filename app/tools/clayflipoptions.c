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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "widgets/claywidgets-utils.h"

#include "clayflipoptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_FLIP_TYPE
};


static void   clay_flip_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   clay_flip_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (ClayFlipOptions, clay_flip_options,
               CLAY_TYPE_TRANSFORM_OPTIONS)


static void
clay_flip_options_class_init (ClayFlipOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_flip_options_set_property;
  object_class->get_property = clay_flip_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FLIP_TYPE,
                         "flip-type",
                         _("Flip Type"),
                         _("Direction of flipping"),
                         CLAY_TYPE_ORIENTATION_TYPE,
                         CLAY_ORIENTATION_HORIZONTAL,
                         CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_flip_options_init (ClayFlipOptions *options)
{
}

static void
clay_flip_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayFlipOptions *options = CLAY_FLIP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FLIP_TYPE:
      options->flip_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_flip_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayFlipOptions *options = CLAY_FLIP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FLIP_TYPE:
      g_value_set_enum (value, options->flip_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_flip_options_gui (ClayToolOptions *tool_options)
{
  GObject              *config     = G_OBJECT (tool_options);
  ClayFlipOptions      *options    = CLAY_FLIP_OPTIONS (tool_options);
  ClayTransformOptions *tr_options = CLAY_TRANSFORM_OPTIONS (tool_options);
  GtkWidget            *vbox;
  GtkWidget            *frame;
  GtkWidget            *combo;
  gchar                *str;
  GtkListStore         *clip_model;
  GdkModifierType       toggle_mask;

  vbox = clay_transform_options_gui (tool_options, FALSE, FALSE, FALSE);

  toggle_mask = clay_get_toggle_behavior_mask ();

  /*  tool toggle  */
  str = g_strdup_printf (_("Direction  (%s)"),
                         clay_get_mod_string (toggle_mask));

  frame = clay_prop_enum_radio_frame_new (config, "flip-type",
                                          str,
                                          CLAY_ORIENTATION_HORIZONTAL,
                                          CLAY_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  g_free (str);

  options->direction_frame = frame;

  /*  the clipping menu  */
  clip_model = clay_enum_store_new_with_range (CLAY_TYPE_TRANSFORM_RESIZE,
                                               CLAY_TRANSFORM_RESIZE_ADJUST,
                                               CLAY_TRANSFORM_RESIZE_CLIP);

  combo = clay_prop_enum_combo_box_new (config, "clip", 0, 0);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (clip_model));
  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo), tr_options->clip);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Clipping"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  g_object_unref (clip_model);

  return vbox;
}
