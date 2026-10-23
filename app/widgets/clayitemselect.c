/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemselect.c
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "gegl/clay-babl-compat.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayitem.h"
#include "core/clayparamspecs.h"
#include "core/claytempbuf.h"

#include "pdb/claypdb.h"

#include "clayitemchooser.h"
#include "clayitemselect.h"
#include "claycontainerbox.h"

#include "clay-intl.h"


static void             clay_item_select_constructed     (GObject             *object);

static ClayValueArray * clay_item_select_run_callback    (ClayPdbDialog       *dialog,
                                                          ClayObject          *object,
                                                          gboolean             closing,
                                                          GError             **error);
static ClayObject     * clay_item_select_get_object      (ClayPdbDialog       *dialog);
static void             clay_item_select_set_object      (ClayPdbDialog       *dialog,
                                                          ClayObject          *object);

static void             clay_item_select_activate        (ClayItemSelect  *select);
static void             clay_item_select_notify_item     (ClayItemSelect  *select);


G_DEFINE_TYPE (ClayItemSelect, clay_item_select, CLAY_TYPE_PDB_DIALOG)

#define parent_class clay_item_select_parent_class


static void
clay_item_select_class_init (ClayItemSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  ClayPdbDialogClass *pdb_class    = CLAY_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = clay_item_select_constructed;

  pdb_class->run_callback    = clay_item_select_run_callback;
  pdb_class->get_object      = clay_item_select_get_object;
  pdb_class->set_object      = clay_item_select_set_object;
}

static void
clay_item_select_init (ClayItemSelect *select)
{
}

static void
clay_item_select_constructed (GObject *object)
{
  ClayPdbDialog  *dialog = CLAY_PDB_DIALOG (object);
  ClayItemSelect *select = CLAY_ITEM_SELECT (object);
  GtkWidget      *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  select->chooser = clay_item_chooser_new (dialog->context, dialog->select_type,
                                           CLAY_VIEW_SIZE_LARGE, 1);
  clay_item_chooser_set_item (CLAY_ITEM_CHOOSER (select->chooser),
                              CLAY_ITEM (dialog->initial_object));
  g_signal_connect_swapped (select->chooser, "notify::item",
                            G_CALLBACK (clay_item_select_notify_item),
                            select);
  g_signal_connect_swapped (select->chooser, "activate",
                            G_CALLBACK (clay_item_select_activate),
                            select);
  gtk_box_pack_start (GTK_BOX (content_area), select->chooser, TRUE, TRUE, 0);
  gtk_widget_set_visible (select->chooser, TRUE);
}

static ClayValueArray *
clay_item_select_run_callback (ClayPdbDialog  *dialog,
                               ClayObject     *object,
                               gboolean        closing,
                               GError        **error)
{
  ClayItem       *item = CLAY_ITEM (object);
  ClayValueArray *return_vals;

  return_vals =
    clay_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        CLAY_TYPE_ITEM, item,
                                        G_TYPE_BOOLEAN, closing,
                                        G_TYPE_NONE);

  return return_vals;
}

static ClayObject *
clay_item_select_get_object (ClayPdbDialog *dialog)
{
  ClayItemSelect *select = CLAY_ITEM_SELECT (dialog);

  return (ClayObject *) clay_item_chooser_get_item (CLAY_ITEM_CHOOSER (select->chooser));
}

static void
clay_item_select_set_object (ClayPdbDialog *dialog,
                                 ClayObject    *object)
{
  ClayItemSelect *select = CLAY_ITEM_SELECT (dialog);

  clay_item_chooser_set_item (CLAY_ITEM_CHOOSER (select->chooser), CLAY_ITEM (object));
}

static void
clay_item_select_activate (ClayItemSelect *select)
{
  clay_pdb_dialog_run_callback ((ClayPdbDialog **) &select, TRUE);
  gtk_widget_destroy (GTK_WIDGET (select));
}

static void
clay_item_select_notify_item (ClayItemSelect *select)
{
  GtkWidget       *button;
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (select->chooser);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (select),
                                               GTK_RESPONSE_OK);

  /* Clicking Okay when an image but not an item has been
   * selected causes the widget to "lock up" and not reappear when you
   * click on it again. For now, we'll disable the Okay button until
   * an item is selected */
  if (CLAY_IS_IMAGE (clay_item_chooser_get_item (chooser)))
    gtk_widget_set_sensitive (button, FALSE);
  else
    gtk_widget_set_sensitive (button, TRUE);

  clay_pdb_dialog_run_callback ((ClayPdbDialog **) &select, FALSE);
}
