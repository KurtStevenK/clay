/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * clayunitcombobox.c
 * Copyright (C) 2004, 2008  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "claywidgetstypes.h"

#include "clayunitcombobox.h"
#include "clayunitstore.h"


/**
 * SECTION: clayunitcombobox
 * @title: ClayUnitComboBox
 * @short_description: A #GtkComboBox to select a #ClayUnit.
 * @see_also: #ClayUnit, #ClayUnitStore
 *
 * #ClayUnitComboBox selects units stored in a #ClayUnitStore.
 **/


struct _ClayUnitComboBox
{
  GtkComboBox  parent_instance;
};


static void clay_unit_combo_box_popup_shown (ClayUnitComboBox *widget);
static void clay_unit_combo_box_constructed (GObject          *object);

G_DEFINE_TYPE (ClayUnitComboBox, clay_unit_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class clay_unit_combo_box_parent_class


static void
clay_unit_combo_box_class_init (ClayUnitComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = clay_unit_combo_box_constructed;
}

static void
clay_unit_combo_box_init (ClayUnitComboBox *combo)
{
  g_signal_connect (combo, "notify::popup-shown",
                    G_CALLBACK (clay_unit_combo_box_popup_shown),
                    NULL);
}

static void
clay_unit_combo_box_constructed (GObject *object)
{
  ClayUnitComboBox *combo = CLAY_UNIT_COMBO_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_unit_combo_box_popup_shown (combo);

  /* Hack to fix issue #2642 and #12491 */
  gtk_combo_box_set_wrap_width (GTK_COMBO_BOX (combo), 1);
}

static void
clay_unit_combo_box_popup_shown (ClayUnitComboBox *widget)
{
  ClayUnitStore   *store;
  gboolean         shown;
  GtkCellRenderer *cell;

  g_object_get (widget,
                "model",       &store,
                "popup-shown", &shown,
                NULL);

  if (store)
    {
      if (shown)
        _clay_unit_store_sync_units (store);

      g_object_unref (store);
    }

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));
  cell = gtk_cell_renderer_text_new ();

  if (shown)
    {
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell,
                                      "text", CLAY_UNIT_STORE_UNIT_LONG_FORMAT,
                                      NULL);

      /* XXX This is ugly but it seems that GtkComboBox won't resize its popup
       * menu when the contents changes (it only resizes the main "chosen item"
       * area). We force a redraw by popping down then up after a contents
       * change.
       */
      g_signal_handlers_disconnect_by_func (widget,
                                            G_CALLBACK (clay_unit_combo_box_popup_shown),
                                            NULL);
      gtk_combo_box_popdown (GTK_COMBO_BOX (widget));
      gtk_combo_box_popup (GTK_COMBO_BOX (widget));
      g_signal_connect (widget, "notify::popup-shown",
                        G_CALLBACK (clay_unit_combo_box_popup_shown),
                        NULL);
    }
  else
    {
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell,
                                      "text", CLAY_UNIT_STORE_UNIT_SHORT_FORMAT,
                                      NULL);
    }
}


/**
 * clay_unit_combo_box_new:
 *
 * Returns: a new #ClayUnitComboBox.
 **/
GtkWidget *
clay_unit_combo_box_new (void)
{
  GtkWidget     *combo_box;
  ClayUnitStore *store;

  store = clay_unit_store_new (0);

  combo_box = g_object_new (CLAY_TYPE_UNIT_COMBO_BOX,
                            "model",     store,
                            "id-column", CLAY_UNIT_STORE_UNIT_LONG_FORMAT,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * clay_unit_combo_box_new_with_model:
 * @model: a #ClayUnitStore
 *
 * Returns: a new #ClayUnitComboBox.
 **/
GtkWidget *
clay_unit_combo_box_new_with_model (ClayUnitStore *model)
{
  return g_object_new (CLAY_TYPE_UNIT_COMBO_BOX,
                       "model",     model,
                       "id-column", CLAY_UNIT_STORE_UNIT_LONG_FORMAT,
                       NULL);
}

/**
 * clay_unit_combo_box_get_active:
 * @combo: a #ClayUnitComboBox
 *
 * Returns the #ClayUnit currently selected in the combo box.
 *
 * Returns: (transfer none): The selected #ClayUnit.
 **/
ClayUnit *
clay_unit_combo_box_get_active (ClayUnitComboBox *combo)
{
  GtkTreeIter  iter;
  ClayUnit    *unit;

  g_return_val_if_fail (CLAY_IS_UNIT_COMBO_BOX (combo), NULL);

  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)), &iter,
                      CLAY_UNIT_STORE_UNIT, &unit,
                      -1);

  return unit;
}

/**
 * clay_unit_combo_box_set_active:
 * @combo: a #ClayUnitComboBox
 * @unit:  a #ClayUnit
 *
 * Sets @unit as the currently selected #ClayUnit on @combo.
 **/
void
clay_unit_combo_box_set_active (ClayUnitComboBox *combo,
                                ClayUnit         *unit)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  g_return_if_fail (CLAY_IS_UNIT_COMBO_BOX (combo));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  _clay_unit_store_sync_units (CLAY_UNIT_STORE (model));

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      ClayUnit *iter_unit;

      gtk_tree_model_get (model, &iter,
                          CLAY_UNIT_STORE_UNIT, &iter_unit,
                          -1);

      if (unit == iter_unit)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
          break;
        }
    }
}
