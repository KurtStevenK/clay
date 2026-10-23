/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include "core/claybrush.h"
#include "core/clayparamspecs.h"
#include "core/claytempbuf.h"

#include "pdb/claypdb.h"

#include "claybrushfactoryview.h"
#include "claybrushselect.h"
#include "claycontainerbox.h"
#include "claylayermodebox.h"

#include "clay-intl.h"


static void             clay_brush_select_constructed  (GObject         *object);

static ClayValueArray * clay_brush_select_run_callback (ClayPdbDialog   *dialog,
                                                        ClayObject      *object,
                                                        gboolean         closing,
                                                        GError         **error);


G_DEFINE_TYPE (ClayBrushSelect, clay_brush_select, CLAY_TYPE_PDB_DIALOG)

#define parent_class clay_brush_select_parent_class


static void
clay_brush_select_class_init (ClayBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  ClayPdbDialogClass *pdb_class    = CLAY_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = clay_brush_select_constructed;

  pdb_class->run_callback    = clay_brush_select_run_callback;
}

static void
clay_brush_select_init (ClayBrushSelect *select)
{
}

static void
clay_brush_select_constructed (GObject *object)
{
  ClayPdbDialog   *dialog = CLAY_PDB_DIALOG (object);
  GtkWidget       *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    clay_brush_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                 dialog->context->clay->brush_factory,
                                 dialog->context,
                                 FALSE,
                                 CLAY_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  clay_container_box_set_size_request (CLAY_CONTAINER_BOX (CLAY_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (CLAY_VIEW_SIZE_MEDIUM + 2),
                                       5 * (CLAY_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_set_visible (dialog->view, TRUE);
}

static ClayValueArray *
clay_brush_select_run_callback (ClayPdbDialog  *dialog,
                                ClayObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  ClayBrush      *brush = CLAY_BRUSH (object);
  ClayTempBuf    *mask  = clay_brush_get_mask (brush);
  const Babl     *format;
  gpointer        data;
  GBytes         *bytes;
  ClayValueArray *return_vals;

  format = clay_babl_compat_u8_mask_format (clay_temp_buf_get_format (mask));
  data   = clay_temp_buf_lock (mask, format, GEGL_ACCESS_READ);

  bytes = g_bytes_new_static (data,
                              clay_temp_buf_get_width         (mask) *
                              clay_temp_buf_get_height        (mask) *
                              babl_format_get_bytes_per_pixel (format));

  return_vals =
    clay_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        CLAY_TYPE_RESOURCE,    object,
                                        G_TYPE_INT,            clay_brush_get_width  (brush),
                                        G_TYPE_INT,            clay_brush_get_height (brush),
                                        G_TYPE_BYTES,          bytes,
                                        G_TYPE_BOOLEAN,        closing,
                                        G_TYPE_NONE);

  g_bytes_unref (bytes);

  clay_temp_buf_unlock (mask, data);

  return return_vals;
}
