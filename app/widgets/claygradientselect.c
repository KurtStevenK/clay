/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claygradientselect.c
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

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claygradient.h"
#include "core/clayparamspecs.h"

#include "pdb/claypdb.h"

#include "claycontainerbox.h"
#include "claydatafactoryview.h"
#include "claygradientselect.h"


enum
{
  PROP_0,
  PROP_SAMPLE_SIZE
};


static void             clay_gradient_select_constructed  (GObject        *object);
static void             clay_gradient_select_set_property (GObject        *object,
                                                           guint           property_id,
                                                           const GValue   *value,
                                                           GParamSpec     *pspec);

static ClayValueArray * clay_gradient_select_run_callback (ClayPdbDialog  *dialog,
                                                           ClayObject     *object,
                                                           gboolean        closing,
                                                           GError        **error);


G_DEFINE_TYPE (ClayGradientSelect, clay_gradient_select,
               CLAY_TYPE_PDB_DIALOG)

#define parent_class clay_gradient_select_parent_class


static void
clay_gradient_select_class_init (ClayGradientSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  ClayPdbDialogClass *pdb_class    = CLAY_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = clay_gradient_select_constructed;
  object_class->set_property = clay_gradient_select_set_property;

  pdb_class->run_callback    = clay_gradient_select_run_callback;

  g_object_class_install_property (object_class, PROP_SAMPLE_SIZE,
                                   g_param_spec_int ("sample-size", NULL, NULL,
                                                     0, 10000, 84,
                                                     CLAY_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_gradient_select_init (ClayGradientSelect *select)
{
}

static void
clay_gradient_select_constructed (GObject *object)
{
  ClayPdbDialog *dialog = CLAY_PDB_DIALOG (object);
  GtkWidget     *content_area;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    clay_data_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                dialog->context->clay->gradient_factory,
                                dialog->context,
                                CLAY_VIEW_SIZE_MEDIUM, 1,
                                dialog->menu_factory, "<Gradients>",
                                "/gradients-popup",
                                "gradients");

  clay_container_box_set_size_request (CLAY_CONTAINER_BOX (CLAY_CONTAINER_EDITOR (dialog->view)->view),
                                       6 * (CLAY_VIEW_SIZE_MEDIUM + 2),
                                       6 * (CLAY_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_set_visible (dialog->view, TRUE);
}

static void
clay_gradient_select_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayGradientSelect *select = CLAY_GRADIENT_SELECT (object);

  switch (property_id)
    {
    case PROP_SAMPLE_SIZE:
      select->sample_size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static ClayValueArray *
clay_gradient_select_run_callback (ClayPdbDialog  *dialog,
                                   ClayObject     *object,
                                   gboolean        closing,
                                   GError        **error)
{
  ClayGradient        *gradient = CLAY_GRADIENT (object);
  ClayGradientSegment *seg      = NULL;
  gdouble             *values, *pv;
  gdouble              pos, delta;
  gint                 i;
  ClayArray           *array;
  ClayValueArray      *return_vals;

  i      = CLAY_GRADIENT_SELECT (dialog)->sample_size;
  pos    = 0.0;
  delta  = 1.0 / (i - 1);

  values = g_new (gdouble, 4 * i);
  pv     = values;

  while (i--)
    {
      GeglColor *color = NULL;

      seg = clay_gradient_get_color_at (gradient, dialog->caller_context,
                                        seg, pos, FALSE,
                                        CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                        &color);

      gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), pv);

      pv  += 4;
      pos += delta;

      g_object_unref (color);
    }

  array = clay_array_new ((guint8 *) values,
                          CLAY_GRADIENT_SELECT (dialog)->sample_size * 4 *
                          sizeof (gdouble),
                          TRUE);
  array->static_data = FALSE;

  return_vals =
    clay_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        CLAY_TYPE_RESOURCE,     object,
                                        CLAY_TYPE_DOUBLE_ARRAY, array,
                                        G_TYPE_BOOLEAN,         closing,
                                        G_TYPE_NONE);

  clay_array_free (array);

  return return_vals;
}
