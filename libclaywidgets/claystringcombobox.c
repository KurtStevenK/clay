/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claystringcombobox.c
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "claywidgetstypes.h"

#include "claystringcombobox.h"


/**
 * SECTION: claystringcombobox
 * @title: ClayStringComboBox
 * @short_description: A #GtkComboBox subclass to select strings.
 *
 * A #GtkComboBox subclass to select strings.
 **/


enum
{
  PROP_0,
  PROP_ID_COLUMN,
  PROP_LABEL_COLUMN,
  PROP_ELLIPSIZE,
  PROP_VALUE
};


typedef struct _ClayStringComboBoxPrivate
{
  gint                       id_column;
  gint                       label_column;
  GtkCellRenderer           *text_renderer;

  ClayStringSensitivityFunc  sensitivity_func;
  gpointer                   sensitivity_data;
  GDestroyNotify             sensitivity_destroy;
} ClayStringComboBoxPrivate;


static void   clay_string_combo_box_constructed   (GObject                   *object);
static void   clay_string_combo_box_set_property  (GObject                   *object,
                                                   guint                      property_id,
                                                   const GValue              *value,
                                                   GParamSpec                *pspec);
static void   clay_string_combo_box_get_property  (GObject                   *object,
                                                   guint                      property_id,
                                                   GValue                    *value,
                                                   GParamSpec                *pspec);

static void  clay_string_combo_box_cell_data_func (GtkCellLayout             *cell_layout,
                                                   GtkCellRenderer           *cell,
                                                   GtkTreeModel              *tree_model,
                                                   GtkTreeIter               *iter,
                                                   ClayStringComboBoxPrivate *priv);


G_DEFINE_TYPE_WITH_PRIVATE (ClayStringComboBox, clay_string_combo_box,
                            GTK_TYPE_COMBO_BOX)

#define parent_class clay_string_combo_box_parent_class


static void
clay_string_combo_box_class_init (ClayStringComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_string_combo_box_constructed;
  object_class->set_property = clay_string_combo_box_set_property;
  object_class->get_property = clay_string_combo_box_get_property;

  /**
   * ClayStringComboBox:id-column:
   *
   * The column in the associated GtkTreeModel that holds unique
   * string IDs.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ID_COLUMN,
                                   g_param_spec_int ("id-column",
                                                     "ID Column",
                                                     "The model column that holds the ID",
                                                     0, G_MAXINT,
                                                     0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  /**
   * ClayStringComboBox:label-column:
   *
   * The column in the associated GtkTreeModel that holds strings to
   * be used as labels in the combo-box.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_LABEL_COLUMN,
                                   g_param_spec_int ("label-column",
                                                     "Label Column",
                                                     "The model column that holds the label",
                                                     0, G_MAXINT,
                                                     0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  /**
   * ClayStringComboBox:ellipsize:
   *
   * Specifies the preferred place to ellipsize text in the combo-box,
   * if the cell renderer does not have enough room to display the
   * entire string.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      "Ellipsize",
                                                      "Ellipsize mode for the text cell renderer",
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      CLAY_PARAM_READWRITE));

  /**
   * ClayStringComboBox:value:
   *
   * The active value (different from the "active" property of
   * GtkComboBox which is the active index).
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        "Value",
                                                        "Value of active item",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_EXPLICIT_NOTIFY));
}

static void
clay_string_combo_box_init (ClayStringComboBox *combo_box)
{
}

static void
clay_string_combo_box_constructed (GObject *object)
{
  ClayStringComboBox        *combo = CLAY_STRING_COMBO_BOX (object);
  ClayStringComboBoxPrivate *priv = clay_string_combo_box_get_instance_private (combo);
  GtkCellRenderer           *cell;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->text_renderer = cell = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), cell,
                                  "text", priv->label_column,
                                  NULL);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (object), priv->text_renderer,
                                      (GtkCellLayoutDataFunc) clay_string_combo_box_cell_data_func,
                                      priv, NULL);

  /* The "changed" signal of the GtkComboBox also triggers a "value" property
   * notification.
   */
  g_signal_connect (object, "changed",
                    G_CALLBACK (g_object_notify),
                    "value");
}

static void
clay_string_combo_box_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayStringComboBox        *combo = CLAY_STRING_COMBO_BOX (object);
  ClayStringComboBoxPrivate *priv = clay_string_combo_box_get_instance_private (combo);

  switch (property_id)
    {
    case PROP_ID_COLUMN:
      priv->id_column = g_value_get_int (value);
      break;

    case PROP_LABEL_COLUMN:
      priv->label_column = g_value_get_int (value);
      break;

    case PROP_ELLIPSIZE:
      g_object_set_property (G_OBJECT (priv->text_renderer),
                             pspec->name, value);
      break;
    case PROP_VALUE:
      clay_string_combo_box_set_active (CLAY_STRING_COMBO_BOX (object),
                                        g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_string_combo_box_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayStringComboBox        *combo = CLAY_STRING_COMBO_BOX (object);
  ClayStringComboBoxPrivate *priv = clay_string_combo_box_get_instance_private (combo);

  switch (property_id)
    {
    case PROP_ID_COLUMN:
      g_value_set_int (value, priv->id_column);
      break;

    case PROP_LABEL_COLUMN:
      g_value_set_int (value, priv->label_column);
      break;

    case PROP_ELLIPSIZE:
      g_object_get_property (G_OBJECT (priv->text_renderer),
                             pspec->name, value);
      break;
    case PROP_VALUE:
      {
        gchar *v;

        v = clay_string_combo_box_get_active (CLAY_STRING_COMBO_BOX (object));
        g_value_take_string (value, v);
      }
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_string_model_lookup (GtkTreeModel *model,
                          gint          column,
                          const gchar  *id,
                          GtkTreeIter  *iter)
{
  GValue    value = G_VALUE_INIT;
  gboolean  iter_valid;

  /*  This lookup could be backed up by a hash table or some other
   *  data structure instead of doing a list traversal. But since this
   *  is a GtkComboBox, there shouldn't be many entries anyway...
   */

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      const gchar *str;

      gtk_tree_model_get_value (model, iter, column, &value);

      str = g_value_get_string (&value);

      if (str && strcmp (str, id) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  return iter_valid;
}


/**
 * clay_string_combo_box_new:
 * @model:        a #GtkTreeModel
 * @id_column:    the model column of the ID
 * @label_column: the modl column of the label
 *
 * Returns: a new #ClayStringComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_string_combo_box_new (GtkTreeModel *model,
                           gint          id_column,
                           gint          label_column)
{
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), NULL);
  g_return_val_if_fail (gtk_tree_model_get_column_type (model,
                                                        id_column) == G_TYPE_STRING, NULL);
  g_return_val_if_fail (gtk_tree_model_get_column_type (model,
                                                        label_column) == G_TYPE_STRING, NULL);

  return g_object_new (CLAY_TYPE_STRING_COMBO_BOX,
                       "model",        model,
                       "id-column",    id_column,
                       "label-column", label_column,
                       NULL);
}

/**
 * clay_string_combo_box_set_active:
 * @combo_box: a #ClayStringComboBox
 * @id:        the ID of the item to select
 *
 * Looks up the item that belongs to the given @id and makes it the
 * selected item in the @combo_box.
 *
 * Returns: %TRUE on success or %FALSE if there was no item for
 *          this value.
 *
 * Since: 2.4
 **/
gboolean
clay_string_combo_box_set_active (ClayStringComboBox *combo_box,
                                  const gchar        *id)
{
  ClayStringComboBoxPrivate *priv;

  g_return_val_if_fail (CLAY_IS_STRING_COMBO_BOX (combo_box), FALSE);

  priv = clay_string_combo_box_get_instance_private (combo_box);

  if (id)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gint          column;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

      column = priv->id_column;

      if (clay_string_model_lookup (model, column, id, &iter))
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
          g_object_notify (G_OBJECT (combo_box), "value");
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), -1);
      g_object_notify (G_OBJECT (combo_box), "value");

      return TRUE;
    }
}

/**
 * clay_string_combo_box_get_active:
 * @combo_box: a #ClayStringComboBox
 *
 * Retrieves the value of the selected (active) item in the @combo_box.
 *
 * Returns: newly allocated ID string or %NULL if nothing was selected
 *
 * Since: 2.4
 **/
gchar *
clay_string_combo_box_get_active (ClayStringComboBox *combo_box)
{
  GtkTreeIter                iter;
  ClayStringComboBoxPrivate *priv;

  g_return_val_if_fail (CLAY_IS_STRING_COMBO_BOX (combo_box), NULL);

  priv = clay_string_combo_box_get_instance_private (combo_box);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
      gchar        *value;
      gint          column;

      column = priv->id_column;

      gtk_tree_model_get (model, &iter,
                          column, &value,
                          -1);

      return value;
    }

  return NULL;
}

/**
 * clay_string_combo_box_set_sensitivity:
 * @combo_box: a #ClayStringComboBox
 * @func:      a function that returns a boolean value, or %NULL to unset
 * @data:      data to pass to @func
 * @destroy:   destroy notification for @data
 *
 * Sets a function that is used to decide about the sensitivity of
 * rows in the @combo_box. Use this if you want to set certain rows
 * insensitive.
 *
 * Calling gtk_widget_queue_draw() on the @combo_box will cause the
 * sensitivity to be updated.
 *
 * Since: 3.0
 **/
void
clay_string_combo_box_set_sensitivity (ClayStringComboBox        *combo_box,
                                       ClayStringSensitivityFunc  func,
                                       gpointer                   data,
                                       GDestroyNotify             destroy)
{
  ClayStringComboBoxPrivate *priv;

  g_return_if_fail (CLAY_IS_STRING_COMBO_BOX (combo_box));

  priv = clay_string_combo_box_get_instance_private (combo_box);

  if (priv->sensitivity_destroy)
    {
      GDestroyNotify d = priv->sensitivity_destroy;

      priv->sensitivity_destroy = NULL;
      d (priv->sensitivity_data);
    }

  priv->sensitivity_func    = func;
  priv->sensitivity_data    = data;
  priv->sensitivity_destroy = destroy;

  gtk_widget_queue_draw (GTK_WIDGET (combo_box));
}


/* Private functions. */

static void
clay_string_combo_box_cell_data_func (GtkCellLayout             *cell_layout,
                                      GtkCellRenderer           *cell,
                                      GtkTreeModel              *tree_model,
                                      GtkTreeIter               *iter,
                                      ClayStringComboBoxPrivate *priv)
{
  if (priv->sensitivity_func)
    {
      gchar    *id;
      gboolean  sensitive;

      gtk_tree_model_get (tree_model, iter,
                          priv->id_column, &id,
                          -1);

      sensitive = priv->sensitivity_func (id, priv->sensitivity_data);

      g_object_set (cell,
                    "sensitive", sensitive,
                    NULL);
      g_free (id);
    }
}
