/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * claylayermodebox.c
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "claylayermodebox.h"
#include "claylayermodecombobox.h"

#include "clay-intl.h"


/**
 * SECTION: claylayermodebox
 * @title: ClayLayerModeBox
 * @short_description: A #GtkBox subclass for selecting a layer mode.
 *
 * A #GtkBox subclass for selecting a layer mode
 **/


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_LAYER_MODE
};


struct _ClayLayerModeBoxPrivate
{
  ClayLayerModeContext  context;
  ClayLayerMode         layer_mode;

  GtkWidget            *mode_combo;
  GtkWidget            *group_combo;
};


static void   clay_layer_mode_box_constructed  (GObject      *object);
static void   clay_layer_mode_box_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   clay_layer_mode_box_get_property (GObject      *object,
                                                guint         prop_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (ClayLayerModeBox, clay_layer_mode_box, GTK_TYPE_BOX)

#define parent_class clay_layer_mode_box_parent_class


static void
clay_layer_mode_box_class_init (ClayLayerModeBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_layer_mode_box_constructed;
  object_class->set_property = clay_layer_mode_box_set_property;
  object_class->get_property = clay_layer_mode_box_get_property;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_flags ("context",
                                                       NULL, NULL,
                                                       CLAY_TYPE_LAYER_MODE_CONTEXT,
                                                       CLAY_LAYER_MODE_CONTEXT_ALL,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LAYER_MODE,
                                   g_param_spec_enum ("layer-mode",
                                                      NULL, NULL,
                                                      CLAY_TYPE_LAYER_MODE,
                                                      CLAY_LAYER_MODE_NORMAL,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
clay_layer_mode_box_init (ClayLayerModeBox *box)
{
  box->priv = clay_layer_mode_box_get_instance_private (box);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (box), 4);
}

static void
clay_layer_mode_box_constructed (GObject *object)
{
  ClayLayerModeBox *box = CLAY_LAYER_MODE_BOX (object);
  GtkWidget        *mode_combo;
  GtkWidget        *group_combo;
  GtkTreeModel     *group_model;
  gint              i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  box->priv->mode_combo = mode_combo =
    clay_layer_mode_combo_box_new (box->priv->context);
  gtk_box_pack_start (GTK_BOX (box), mode_combo, TRUE, TRUE, 0);
  gtk_widget_set_visible (mode_combo, TRUE);

  g_object_bind_property (object,                "context",
                          G_OBJECT (mode_combo), "context",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (object,                "layer-mode",
                          G_OBJECT (mode_combo), "layer-mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  box->priv->group_combo = group_combo =
    clay_prop_enum_combo_box_new (G_OBJECT (mode_combo),
                                  "group", 0, 0);
  clay_int_combo_box_set_layout (CLAY_INT_COMBO_BOX (group_combo),
                                 CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY);
  gtk_box_pack_start (GTK_BOX (box), group_combo, FALSE, FALSE, 0);

  clay_help_set_help_data (group_combo,
                           _("Switch to another group of modes"),
                           NULL);

  group_model = gtk_combo_box_get_model (GTK_COMBO_BOX (group_combo));

  for (i = 0; i < 2; i++)
    {
      static const gchar *icons[] =
      {
        "clay-reset",
        "clay-wilber-eek"
      };

      GtkTreeIter iter;

      if (clay_int_store_lookup_by_value (group_model, i, &iter))
        gtk_list_store_set (GTK_LIST_STORE (group_model), &iter,
                            CLAY_INT_STORE_ICON_NAME, icons[i],
                            -1);
    }
}

static void
clay_layer_mode_box_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayLayerModeBox *box = CLAY_LAYER_MODE_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      clay_layer_mode_box_set_context (box, g_value_get_flags (value));
      break;

    case PROP_LAYER_MODE:
      clay_layer_mode_box_set_mode (box, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_layer_mode_box_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayLayerModeBox *box = CLAY_LAYER_MODE_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_flags (value, box->priv->context);
      break;

    case PROP_LAYER_MODE:
      g_value_set_enum (value, box->priv->layer_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * clay_layer_mode_box_new:
 * Foo.
 *
 * Returns: a new #ClayLayerModeBox.
 **/
GtkWidget *
clay_layer_mode_box_new (ClayLayerModeContext context)
{
  return g_object_new (CLAY_TYPE_LAYER_MODE_BOX,
                       "context", context,
                       NULL);
}

void
clay_layer_mode_box_set_context (ClayLayerModeBox     *box,
                                 ClayLayerModeContext  context)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_BOX (box));

  if (context != box->priv->context)
    {
      box->priv->context = context;

      g_object_notify (G_OBJECT (box), "context");
    }
}

ClayLayerModeContext
clay_layer_mode_box_get_context (ClayLayerModeBox *box)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MODE_BOX (box),
                        CLAY_LAYER_MODE_CONTEXT_ALL);

  return box->priv->context;
}

void
clay_layer_mode_box_set_mode (ClayLayerModeBox *box,
                              ClayLayerMode     mode)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_BOX (box));

  if (mode != box->priv->layer_mode)
    {
      if (mode == -1)
        {
          ClayLayerModeComboBox *combo_box;

          combo_box = CLAY_LAYER_MODE_COMBO_BOX (box->priv->mode_combo);

          /* Directly call clay_layer_mode_combo_box_set_mode() instead of
           * changing the property because -1 is not accepted as a valid
           * value for the property.
           */
          clay_layer_mode_combo_box_set_mode (combo_box, -1);
        }
      else
        {
          box->priv->layer_mode = mode;
          g_object_notify (G_OBJECT (box), "layer-mode");
        }
    }
}

ClayLayerMode
clay_layer_mode_box_get_mode (ClayLayerModeBox *box)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MODE_BOX (box),
                        CLAY_LAYER_MODE_NORMAL);

  return box->priv->layer_mode;
}

void
clay_layer_mode_box_set_label (ClayLayerModeBox *box,
                               const gchar      *label)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_BOX (box));

  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (box->priv->mode_combo),
                                label);
}

void
clay_layer_mode_box_set_ellipsize (ClayLayerModeBox   *box,
                                   PangoEllipsizeMode  mode)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_BOX (box));

  g_object_set (box->priv->mode_combo,
                "ellipsize", mode,
                NULL);
}
