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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayconfig-utils.h"

#include "paint/clayinkoptions.h"

#include "widgets/clayblobeditor.h"
#include "widgets/claypropwidgets.h"

#include "clayinkoptions-gui.h"
#include "claypaintoptions-gui.h"

#include "clay-intl.h"


GtkWidget *
clay_ink_options_gui (ClayToolOptions *tool_options)
{
  GObject        *config      = G_OBJECT (tool_options);
  ClayInkOptions *ink_options = CLAY_INK_OPTIONS (tool_options);
  GtkWidget      *vbox        = clay_paint_options_gui (tool_options);
  GtkWidget      *frame;
  GtkWidget      *vbox2;
  GtkWidget      *scale;
  GtkWidget      *combo_box;
  GtkWidget      *blob_box;
  GtkWidget      *hbox;
  GtkWidget      *editor;
  GtkSizeGroup   *size_group;

  /* adjust sliders */
  frame = clay_frame_new (_("Adjustment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  /*  size slider  */
  scale = clay_prop_spin_scale_new (config, "size",
                                    1.0, 2.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* angle adjust slider */
  scale = clay_prop_spin_scale_new (config, "tilt-angle",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* sens sliders */
  frame = clay_frame_new (_("Sensitivity"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  /* size sens slider */
  scale = clay_prop_spin_scale_new (config, "size-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* tilt sens slider */
  scale = clay_prop_spin_scale_new (config, "tilt-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* velocity sens slider */
  scale = clay_prop_spin_scale_new (config, "vel-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* Blob shape widgets */
  frame = clay_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /* Blob type radiobuttons */
  blob_box = clay_prop_enum_icon_box_new (config, "blob-type",
                                          "clay-shape", 0, 0);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (blob_box),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (hbox), blob_box, FALSE, FALSE, 0);

  gtk_size_group_add_widget (size_group, blob_box);
  g_object_unref (size_group);

  /* Blob editor */
  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  gtk_size_group_add_widget (size_group, frame);

  editor = clay_blob_editor_new (ink_options->blob_type,
                                 ink_options->blob_aspect,
                                 ink_options->blob_angle);
  gtk_container_add (GTK_CONTAINER (frame), editor);
  gtk_widget_set_visible (editor, TRUE);

  /* Expand layer options */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  scale = clay_prop_spin_scale_new (config, "expand-amount",
                                    1, 10, 2);
  clay_spin_scale_set_constrain_drag (CLAY_SPIN_SCALE (scale), TRUE);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 1.0, 1000.0);
  clay_spin_scale_set_gamma (CLAY_SPIN_SCALE (scale), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  combo_box = clay_prop_enum_combo_box_new (config, "expand-fill-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), combo_box, FALSE, FALSE, 0);

  frame = clay_prop_enum_radio_frame_new (config, "expand-mask-fill-type",
                                          _("Fill Layer Mask With"), 0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

  frame = clay_prop_expanding_frame_new (config, "expand-use", NULL,
                                         vbox2, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  clay_config_connect (config, G_OBJECT (editor), "blob-type");
  clay_config_connect (config, G_OBJECT (editor), "blob-aspect");
  clay_config_connect (config, G_OBJECT (editor), "blob-angle");

  return vbox;
}
