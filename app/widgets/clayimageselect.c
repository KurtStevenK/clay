/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimageselect.c
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
#include "core/clayparamspecs.h"
#include "core/claytempbuf.h"

#include "pdb/claypdb.h"

#include "claycontainerbox.h"
#include "claycontainerview.h"
#include "clayimagechooser.h"
#include "clayimageselect.h"
#include "claylayermodebox.h"

#include "clay-intl.h"


static void             clay_image_select_constructed     (GObject          *object);

static ClayValueArray * clay_image_select_run_callback    (ClayPdbDialog    *dialog,
                                                           ClayObject       *object,
                                                           gboolean          closing,
                                                           GError          **error);
static ClayObject     * clay_image_select_get_object      (ClayPdbDialog     *dialog);
static void             clay_image_select_set_object      (ClayPdbDialog     *dialog,
                                                           ClayObject        *object);


static void             clay_image_select_activate        (ClayImageSelect   *select);
static void             clay_image_select_notify_image    (ClayImageSelect   *select);

G_DEFINE_TYPE (ClayImageSelect, clay_image_select, CLAY_TYPE_PDB_DIALOG)

#define parent_class clay_image_select_parent_class


static void
clay_image_select_class_init (ClayImageSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  ClayPdbDialogClass *pdb_class    = CLAY_PDB_DIALOG_CLASS (klass);

  object_class->constructed = clay_image_select_constructed;

  pdb_class->run_callback   = clay_image_select_run_callback;
  pdb_class->set_object     = clay_image_select_set_object;
  pdb_class->get_object     = clay_image_select_get_object;
}

static void
clay_image_select_init (ClayImageSelect *select)
{
}

static void
clay_image_select_constructed (GObject *object)
{
  ClayPdbDialog   *dialog = CLAY_PDB_DIALOG (object);
  ClayImageSelect *select = CLAY_IMAGE_SELECT (object);
  GtkWidget       *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  select->chooser = clay_image_chooser_new (dialog->context,
                                            CLAY_VIEW_SIZE_LARGE, 1);
  clay_image_chooser_set_image (CLAY_IMAGE_CHOOSER (select->chooser),
                                CLAY_IMAGE (dialog->initial_object));
  g_signal_connect_swapped (select->chooser, "notify::image",
                            G_CALLBACK (clay_image_select_notify_image),
                            select);
  g_signal_connect_swapped (select->chooser, "activate",
                            G_CALLBACK (clay_image_select_activate),
                            select);

  gtk_box_pack_start (GTK_BOX (content_area), select->chooser, TRUE, TRUE, 0);
  gtk_widget_set_visible (select->chooser, TRUE);
}

static ClayValueArray *
clay_image_select_run_callback (ClayPdbDialog  *dialog,
                                ClayObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  ClayImage      *image = CLAY_IMAGE (object);
  ClayValueArray *return_vals;

  return_vals =
    clay_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        CLAY_TYPE_IMAGE, image,
                                        G_TYPE_BOOLEAN,  closing,
                                        G_TYPE_NONE);

  return return_vals;
}

static ClayObject *
clay_image_select_get_object (ClayPdbDialog *dialog)
{
  ClayImageSelect *select = CLAY_IMAGE_SELECT (dialog);

  return (ClayObject *) clay_image_chooser_get_image (CLAY_IMAGE_CHOOSER (select->chooser));

}

static void
clay_image_select_set_object (ClayPdbDialog *dialog,
                              ClayObject    *object)
{
  ClayImageSelect *select = CLAY_IMAGE_SELECT (dialog);

  clay_image_chooser_set_image (CLAY_IMAGE_CHOOSER (select->chooser), CLAY_IMAGE (object));
}


static void
clay_image_select_activate (ClayImageSelect *select)
{
  clay_pdb_dialog_run_callback ((ClayPdbDialog **) &select, TRUE);
  gtk_widget_destroy (GTK_WIDGET (select));
}

static void
clay_image_select_notify_image (ClayImageSelect *select)
{
  clay_pdb_dialog_run_callback ((ClayPdbDialog **) &select, FALSE);
}
