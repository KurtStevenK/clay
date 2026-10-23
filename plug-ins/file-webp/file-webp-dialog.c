/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the Clay
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include <webp/encode.h>

#include "file-webp.h"
#include "file-webp-dialog.h"

#include "libclay/stdplugins-intl.h"


static void
show_maxkeyframe_hints (GObject          *config,
                        const GParamSpec *pspec,
                        GtkLabel         *label)
{
  gint kmax;

  g_object_get (config,
                "keyframe-distance", &kmax,
                NULL);

  if (kmax == 0)
    {
      gtk_label_set_text (label, _("(no keyframes)"));
    }
  else if (kmax == 1)
    {
      gtk_label_set_text (label, _("(all frames are keyframes)"));
    }
  else
    {
      gtk_label_set_text (label, "");
    }
}

gboolean
save_dialog (ClayImage     *image,
             ClayProcedure *procedure,
             GObject       *config)
{
  GtkWidget  *dialog;
  ClayLayer **layers;
  gint32      nlayers;
  gboolean    animation_supported = FALSE;
  gboolean    run;

  layers  = clay_image_get_layers (image);
  nlayers = clay_core_object_array_get_length ((GObject **) layers);
  g_free (layers);

  animation_supported = nlayers > 1;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  /* Create scale for image and alpha quality */
  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "quality", CLAY_TYPE_SPIN_SCALE);
  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "alpha-quality", CLAY_TYPE_SPIN_SCALE);

  /* Create frame for quality options */
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                  "quality-options",
                                  "quality", "alpha-quality",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "quality-frame", "lossless", TRUE,
                                    "quality-options");

  /* Create frame for additional features like Sharp YUV */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                   "advanced-title", _("Advanced Options"),
                                   FALSE, FALSE);

  clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                       "use-sharp-yuv",
                                       TRUE, config, "lossless", TRUE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                  "advanced-options",
                                  "use-sharp-yuv",
                                  NULL);

  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "advanced-frame", "advanced-title", FALSE,
                                    "advanced-options");

  if (animation_supported)
    {
      GtkWidget      *label_kf;

      /* Hint for some special values of keyframe-distance. */
      label_kf = clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                                  "keyframe-hint", NULL,
                                                  FALSE, FALSE);
      gtk_label_set_xalign (GTK_LABEL (label_kf), 1.0);
      gtk_label_set_ellipsize (GTK_LABEL (label_kf), PANGO_ELLIPSIZE_END);
      clay_label_set_attributes (GTK_LABEL (label_kf),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      g_signal_connect (config, "notify::keyframe-distance",
                        G_CALLBACK (show_maxkeyframe_hints),
                        label_kf);
      show_maxkeyframe_hints (config, NULL, GTK_LABEL (label_kf));

      /* when minimize-size is true, keyframe-distance and hint are insensitive. */
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "keyframe-distance",
                                           TRUE, config, "minimize-size", TRUE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "keyframe-hint",
                                           TRUE, config, "minimize-size", TRUE);

      /* Create frame for animation options */
      clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                      "animation-options",
                                      "animation-loop",
                                      "minimize-size",
                                      "keyframe-distance",
                                      "keyframe-hint",
                                      "default-delay",
                                      "force-delay",
                                      NULL);
      clay_procedure_dialog_fill_expander (CLAY_PROCEDURE_DIALOG (dialog),
                                           "animation-frame", "animation", FALSE,
                                           "animation-options");

      /* Fill dialog with containers*/
      clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                                  "preset", "quality-frame",
                                  "advanced-frame", "animation-frame",
                                  NULL);
    }
  else
    {
      /* Fill dialog with containers*/
      clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                                  "preset", "quality-frame", "advanced-frame",
                                  NULL);
    }

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
