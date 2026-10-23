/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * claylayermodecombobox.c
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

#include "claylayermodecombobox.h"


/**
 * SECTION: claylayermodecombobox
 * @title: ClayLayerModeComboBox
 * @short_description: A #ClayEnumComboBox subclass for selecting a layer mode.
 *
 * A #GtkComboBox subclass for selecting a layer mode
 **/


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_LAYER_MODE,
  PROP_GROUP
};


struct _ClayLayerModeComboBoxPrivate
{
  ClayLayerModeContext context;
  ClayLayerMode        layer_mode;
  ClayLayerModeGroup   group;
};


static void     clay_layer_mode_combo_box_constructed      (GObject               *object);
static void     clay_layer_mode_combo_box_set_property     (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static void     clay_layer_mode_combo_box_get_property     (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);

static void     clay_layer_mode_combo_box_changed          (GtkComboBox           *gtk_combo);

static void     clay_layer_mode_combo_box_update_model     (ClayLayerModeComboBox *combo,
                                                            gboolean               change_mode);
static gboolean clay_layer_mode_combo_box_separator_func   (GtkTreeModel          *model,
                                                            GtkTreeIter           *iter,
                                                            gpointer               data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayLayerModeComboBox, clay_layer_mode_combo_box,
                            CLAY_TYPE_ENUM_COMBO_BOX)

#define parent_class clay_layer_mode_combo_box_parent_class


static void
clay_layer_mode_combo_box_class_init (ClayLayerModeComboBoxClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *combo_class  = GTK_COMBO_BOX_CLASS (klass);

  object_class->constructed  = clay_layer_mode_combo_box_constructed;
  object_class->set_property = clay_layer_mode_combo_box_set_property;
  object_class->get_property = clay_layer_mode_combo_box_get_property;

  combo_class->changed       = clay_layer_mode_combo_box_changed;

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

  g_object_class_install_property (object_class, PROP_GROUP,
                                   g_param_spec_enum ("group",
                                                      NULL, NULL,
                                                      CLAY_TYPE_LAYER_MODE_GROUP,
                                                      CLAY_LAYER_MODE_GROUP_DEFAULT,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
clay_layer_mode_combo_box_init (ClayLayerModeComboBox *combo)
{
  combo->priv = clay_layer_mode_combo_box_get_instance_private (combo);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo),
                                        clay_layer_mode_combo_box_separator_func,
                                        GINT_TO_POINTER (CLAY_LAYER_MODE_SEPARATOR),
                                        NULL);
}

static void
clay_layer_mode_combo_box_constructed (GObject *object)
{
  ClayLayerModeComboBox *combo = CLAY_LAYER_MODE_COMBO_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_layer_mode_combo_box_update_model (combo, FALSE);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo),
                                 combo->priv->layer_mode);
}

static void
clay_layer_mode_combo_box_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClayLayerModeComboBox *combo = CLAY_LAYER_MODE_COMBO_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      clay_layer_mode_combo_box_set_context (combo, g_value_get_flags (value));
      break;

    case PROP_LAYER_MODE:
      clay_layer_mode_combo_box_set_mode (combo, g_value_get_enum (value));
      break;

    case PROP_GROUP:
      clay_layer_mode_combo_box_set_group (combo, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_layer_mode_combo_box_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClayLayerModeComboBox *combo = CLAY_LAYER_MODE_COMBO_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_flags (value, combo->priv->context);
      break;

    case PROP_LAYER_MODE:
      g_value_set_enum (value, combo->priv->layer_mode);
      break;

    case PROP_GROUP:
      g_value_set_enum (value, combo->priv->group);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_layer_mode_combo_box_changed (GtkComboBox *gtk_combo)
{
  ClayLayerModeComboBox *combo = CLAY_LAYER_MODE_COMBO_BOX (gtk_combo);
  ClayLayerMode          mode;

  if (clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo),
                                     (gint *) &mode))
    {
      combo->priv->layer_mode = mode;

      g_object_notify (G_OBJECT (combo), "layer-mode");
    }
}


/**
 * clay_layer_mode_combo_box_new:
 * Foo.
 *
 * Returns: a new #ClayLayerModeComboBox.
 **/
GtkWidget *
clay_layer_mode_combo_box_new (ClayLayerModeContext context)
{
  return g_object_new (CLAY_TYPE_LAYER_MODE_COMBO_BOX,
                       "context", context,
                       NULL);
}

void
clay_layer_mode_combo_box_set_context (ClayLayerModeComboBox *combo,
                                       ClayLayerModeContext   context)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo));

  if (context != combo->priv->context)
    {
      g_object_freeze_notify (G_OBJECT (combo));

      combo->priv->context = context;
      g_object_notify (G_OBJECT (combo), "context");

      clay_layer_mode_combo_box_update_model (combo, TRUE);

      g_object_thaw_notify (G_OBJECT (combo));
    }
}

ClayLayerModeContext
clay_layer_mode_combo_box_get_context (ClayLayerModeComboBox *combo)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo),
                        CLAY_LAYER_MODE_CONTEXT_ALL);

  return combo->priv->context;
}

void
clay_layer_mode_combo_box_set_mode (ClayLayerModeComboBox *combo,
                                    ClayLayerMode          mode)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo));
  g_return_if_fail (mode == -1 || (clay_layer_mode_get_context (mode) & combo->priv->context));

  if (mode == -1)
    {
      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo), -1);
      combo->priv->layer_mode = mode;
    }
  else if (mode != combo->priv->layer_mode)
    {
      GtkTreeModel *model;
      GtkTreeIter   dummy;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

      g_object_freeze_notify (G_OBJECT (combo));

      if (! clay_int_store_lookup_by_value (model, mode, &dummy))
        {
          combo->priv->group = clay_layer_mode_get_group (mode);
          g_object_notify (G_OBJECT (combo), "group");

          clay_layer_mode_combo_box_update_model (combo, FALSE);
        }

      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo), mode);

      g_object_thaw_notify (G_OBJECT (combo));
    }
}

ClayLayerMode
clay_layer_mode_combo_box_get_mode (ClayLayerModeComboBox *combo)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo),
                        CLAY_LAYER_MODE_NORMAL);

  return combo->priv->layer_mode;
}

void
clay_layer_mode_combo_box_set_group (ClayLayerModeComboBox *combo,
                                     ClayLayerModeGroup     group)
{
  g_return_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo));

  if (group != combo->priv->group)
    {
      g_object_freeze_notify (G_OBJECT (combo));

      combo->priv->group = group;
      g_object_notify (G_OBJECT (combo), "group");

      clay_layer_mode_combo_box_update_model (combo, TRUE);

      g_object_thaw_notify (G_OBJECT (combo));
    }
}

ClayLayerModeGroup
clay_layer_mode_combo_box_get_group (ClayLayerModeComboBox *combo)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MODE_COMBO_BOX (combo),
                        CLAY_LAYER_MODE_GROUP_DEFAULT);

  return combo->priv->group;
}


/*  private functions  */

static void
clay_enum_store_add_value (GtkListStore *store,
                           GEnumClass   *enum_class,
                           GEnumValue   *value)
{
  GtkTreeIter  iter = { 0, };
  const gchar *desc;
  const gchar *abbrev;
  gchar       *stripped;

  desc   = clay_enum_value_get_desc   (enum_class, value);
  abbrev = clay_enum_value_get_abbrev (enum_class, value);

  /* no mnemonics in combo boxes */
  stripped = clay_strip_uline (desc);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      CLAY_INT_STORE_VALUE,  value->value,
                      CLAY_INT_STORE_LABEL,  stripped,
                      CLAY_INT_STORE_ABBREV, abbrev,
                      -1);

  g_free (stripped);
}

static void
clay_enum_store_add_separator (GtkListStore *store)
{
  GtkTreeIter  iter = { 0, };

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      CLAY_INT_STORE_VALUE, CLAY_LAYER_MODE_SEPARATOR,
                      -1);
}

static GtkListStore *
clay_enum_store_new_from_array (GType                 enum_type,
                                gint                  n_values,
                                const gint           *values,
                                ClayLayerModeContext  context)
{
  GtkListStore *store;
  GEnumClass   *enum_class;
  GEnumValue   *value;
  gboolean      first_item        = TRUE;
  gboolean      prepend_separator = FALSE;
  gint          i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);
  g_return_val_if_fail (values != NULL, NULL);

  store = g_object_new (CLAY_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  enum_class = g_type_class_ref (enum_type);

  for (i = 0; i < n_values; i++)
    {
      if (values[i] != CLAY_LAYER_MODE_SEPARATOR)
        {
          if (clay_layer_mode_get_context (values[i]) & context)
            {
              value = g_enum_get_value (enum_class, values[i]);

              if (value)
                {
                  if (prepend_separator)
                    {
                      clay_enum_store_add_separator (store);

                      prepend_separator = FALSE;
                    }

                  clay_enum_store_add_value (store, enum_class, value);

                  first_item = FALSE;
                }
            }
        }
      else
        {
          if (! first_item)
            prepend_separator = TRUE;
        }
    }

  g_type_class_unref (enum_class);

  return store;
}

static void
clay_layer_mode_combo_box_update_model (ClayLayerModeComboBox *combo,
                                        gboolean               change_mode)
{
  GtkListStore        *store;
  const ClayLayerMode *modes;
  gint                 n_modes;

  modes = clay_layer_mode_get_group_array (combo->priv->group, &n_modes);
  store = clay_enum_store_new_from_array (CLAY_TYPE_LAYER_MODE,
                                          n_modes, (gint *) modes,
                                          combo->priv->context);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
  g_object_unref (store);

  if (change_mode)
    {
      ClayLayerMode new_mode;

      if (clay_layer_mode_get_for_group (combo->priv->layer_mode,
                                         combo->priv->group,
                                         &new_mode) &&
          (clay_layer_mode_get_context (new_mode) & combo->priv->context))
        {
          clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo), new_mode);
        }
      else
        {
          GtkTreeIter iter;

          /*  switch to the first mode, which will be one of the "normal"  */
          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
        }
    }
}

static gboolean
clay_layer_mode_combo_box_separator_func (GtkTreeModel *model,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  gint value;

  gtk_tree_model_get (model, iter, CLAY_INT_STORE_VALUE, &value, -1);

  return value == GPOINTER_TO_INT (data);
}
