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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay-transform-utils.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolgui.h"

#include "claygenerictransformtool.h"
#include "claytoolcontrol.h"
#include "claytransformgridoptions.h"

#include "clay-intl.h"


/*  local function prototypes  */

static gboolean   clay_generic_transform_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                                              ClayMatrix3           *transform);
static void       clay_generic_transform_tool_dialog         (ClayTransformGridTool *tg_tool);
static void       clay_generic_transform_tool_dialog_update  (ClayTransformGridTool *tg_tool);
static void       clay_generic_transform_tool_prepare        (ClayTransformGridTool *tg_tool);


G_DEFINE_TYPE (ClayGenericTransformTool, clay_generic_transform_tool,
               CLAY_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class clay_generic_transform_tool_parent_class


static void
clay_generic_transform_tool_class_init (ClayGenericTransformToolClass *klass)
{
  ClayTransformGridToolClass *tg_class = CLAY_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix = clay_generic_transform_tool_info_to_matrix;
  tg_class->dialog         = clay_generic_transform_tool_dialog;
  tg_class->dialog_update  = clay_generic_transform_tool_dialog_update;
  tg_class->prepare        = clay_generic_transform_tool_prepare;
}

static void
clay_generic_transform_tool_init (ClayGenericTransformTool *unified_tool)
{
}

static gboolean
clay_generic_transform_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                            ClayMatrix3           *transform)
{
  ClayGenericTransformTool *generic = CLAY_GENERIC_TRANSFORM_TOOL (tg_tool);

  if (CLAY_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points)
    CLAY_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points (generic);

  clay_matrix3_identity (transform);

  return clay_transform_matrix_generic (transform,
                                        generic->input_points,
                                        generic->output_points);
}

static void
clay_generic_transform_tool_dialog (ClayTransformGridTool *tg_tool)
{
  ClayGenericTransformTool *generic = CLAY_GENERIC_TRANSFORM_TOOL (tg_tool);
  GtkWidget                *frame;
  GtkWidget                *vbox;
  GtkWidget                *label;
  GtkSizeGroup             *size_group;

  frame = clay_frame_new (_("Transform Matrix"));
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (tg_tool->gui)), frame,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  label = generic->matrix_label = gtk_label_new (" ");
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  clay_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_widget_set_visible (label, TRUE);

  label = generic->invalid_label = gtk_label_new (_("Invalid transform"));
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  g_object_unref (size_group);
}

static void
clay_generic_transform_tool_dialog_update (ClayTransformGridTool *tg_tool)
{
  ClayGenericTransformTool *generic = CLAY_GENERIC_TRANSFORM_TOOL (tg_tool);
  ClayMatrix3               transform;
  gboolean                  transform_valid;

  transform_valid = clay_transform_grid_tool_info_to_matrix (tg_tool,
                                                             &transform);

  if (transform_valid)
    {
      gchar buf[256];
      gtk_widget_set_visible (generic->matrix_label, TRUE);
      gtk_widget_set_visible (generic->invalid_label, FALSE);

      g_snprintf (buf, sizeof (buf), "<tt>% 11.4f\t% 11.4f\t% 11.4f\n% 11.4f\t% 11.4f"
                  "\t% 11.4f\n% 11.4f\t% 11.4f\t% 11.4f</tt>",
                  transform.coeff[0][0], transform.coeff[0][1], transform.coeff[0][2],
                  transform.coeff[1][0], transform.coeff[1][1], transform.coeff[1][2],
                  transform.coeff[2][0], transform.coeff[2][1], transform.coeff[2][2]);
      gtk_label_set_markup (GTK_LABEL (generic->matrix_label), buf);
    }
  else
    {
      gtk_widget_set_visible (generic->invalid_label, TRUE);
      gtk_widget_set_visible (generic->matrix_label, FALSE);
    }
}

static void
clay_generic_transform_tool_prepare (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool        *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayGenericTransformTool *generic = CLAY_GENERIC_TRANSFORM_TOOL (tg_tool);

  generic->input_points[0] = (ClayVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1] = (ClayVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2] = (ClayVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3] = (ClayVector2) {tr_tool->x2, tr_tool->y2};

  memcpy (generic->output_points, generic->input_points,
          sizeof (generic->input_points));
}
