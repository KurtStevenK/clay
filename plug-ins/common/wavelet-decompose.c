/*
 *  Wavelet decompose plug-in by Miroslav Talasek, miroslav.talasek@seznam.cz
 */

/*
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

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-wavelet-decompose"
#define PLUG_IN_ROLE   "clay-wavelet-decompose"
#define PLUG_IN_BINARY "wavelet-decompose"




typedef struct _Wavelet      Wavelet;
typedef struct _WaveletClass WaveletClass;

struct _Wavelet
{
  ClayPlugIn parent_instance;
};

struct _WaveletClass
{
  ClayPlugInClass parent_class;
};


#define WAVELET_TYPE  (wavelet_get_type ())
#define WAVELET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAVELET_TYPE, Wavelet))

GType                   wavelet_get_type         (void);

static GList          * wavelet_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * wavelet_create_procedure (ClayPlugIn           *plug_in,
                                                  const gchar          *name);

static ClayValueArray * wavelet_run              (ClayProcedure        *procedure,
                                                  ClayRunMode           run_mode,
                                                  ClayImage            *image,
                                                  ClayDrawable        **drawables,
                                                  ClayProcedureConfig  *config,
                                                  gpointer              run_data);

static void             wavelet_blur             (ClayDrawable         *drawable,
                                                  gint                  radius);

static gboolean         wavelet_decompose_dialog (ClayProcedure        *procedure,
                                                  GObject              *config);


G_DEFINE_TYPE (Wavelet, wavelet, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (WAVELET_TYPE)
DEFINE_STD_SET_I18N




static void
wavelet_class_init (WaveletClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wavelet_query_procedures;
  plug_in_class->create_procedure = wavelet_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wavelet_init (Wavelet *wavelet)
{
}

static GList *
wavelet_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static ClayProcedure *
wavelet_create_procedure (ClayPlugIn  *plug_in,
                           const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            wavelet_run, NULL, NULL);

      clay_procedure_set_image_types (procedure, "RGB*, GRAY*");
      clay_procedure_set_sensitivity_mask (procedure,
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLE);

      clay_procedure_set_menu_label (procedure, _("_Wavelet Decompose..."));
      clay_procedure_add_menu_path (procedure, "<Image>/Filters/Enhance");

      clay_procedure_set_documentation (procedure,
                                        _("Wavelet decompose"),
                                        "Compute and render wavelet scales",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "Miroslav Talasek <miroslav.talasek@seznam.cz>",
                                      "19 January 2017");

      clay_procedure_add_int_argument (procedure, "scales",
                                       _("Scal_es"),
                                       _("Number of scales"),
                                       1, 7, 5,
                                       G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "create-group",
                                           _("Create a layer group to store the "
                                             "_decomposition"),
                                           _("Create a layer group to store the "
                                           "decomposition"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "create-masks",
                                           _("_Add a layer mask to each scales layer"),
                                           _("Add a layer mask to each scales layer"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
wavelet_run (ClayProcedure        *procedure,
              ClayRunMode           run_mode,
              ClayImage            *image,
              ClayDrawable        **drawables,
              ClayProcedureConfig  *config,
              gpointer              run_data)
{
  ClayLayer     **scale_layers;
  ClayLayer      *new_scale;
  ClayLayer      *parent             = NULL;
  ClayDrawable   *drawable;
  ClayLayerMode   grain_extract_mode = CLAY_LAYER_MODE_GRAIN_EXTRACT;
  ClayLayerMode   grain_merge_mode   = CLAY_LAYER_MODE_GRAIN_MERGE;
  gint            id;
  gint            scales;
  gboolean        create_group;
  gboolean        create_masks;

  gegl_init (NULL, NULL);

  if (clay_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   clay_procedure_get_name (procedure));

      return clay_procedure_new_return_values (procedure,
                                               CLAY_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == CLAY_RUN_INTERACTIVE && ! wavelet_decompose_dialog (procedure, G_OBJECT (config)))
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_CANCEL,
                                             NULL);

  g_object_get (config,
                "scales",       &scales,
                "create-group", &create_group,
                "create-masks", &create_masks,
                NULL);

  clay_progress_init (_("Wavelet Decompose"));

  clay_image_undo_group_start (image);

  clay_image_freeze_layers (image);

  if (create_group)
    {
      parent = CLAY_LAYER (clay_group_layer_new (image, _("Decomposition")));

      clay_item_set_visible (CLAY_ITEM (parent), FALSE);
      clay_image_insert_layer (image, parent,
                               CLAY_LAYER (clay_item_get_parent (CLAY_ITEM (drawable))),
                               clay_image_get_item_position (image,
                                                             CLAY_ITEM (drawable)));
    }

  scale_layers = g_new (ClayLayer *, scales);
  new_scale = clay_layer_copy (CLAY_LAYER (drawable));
  clay_image_insert_layer (image, new_scale, parent,
                           clay_image_get_item_position (image,
                                                         CLAY_ITEM (drawable)));

  /* the exact result of the grain-extract and grain-merge modes
   * depends on the choice of (gamma-corrected) midpoint intensity
   * value.  for the non-legacy modes, the midpoint value is 0.5,
   * which isn't representable exactly using integer precision.  for
   * the legacy modes, the midpoint value is 128/255 (i.e., 0x80),
   * which is representable exactly using (gamma-corrected) integer
   * precision.  we therefore use the legacy modes when the input
   * image precision is integer, and only use the (preferable)
   * non-legacy modes when the input image precision is floating
   * point.
   *
   * this avoids imperfect reconstruction of the image when using
   * integer precision.  see bug #786844.
   */
  switch (clay_image_get_precision (image))
    {
    case CLAY_PRECISION_U8_LINEAR:
    case CLAY_PRECISION_U8_NON_LINEAR:
    case CLAY_PRECISION_U8_PERCEPTUAL:
    case CLAY_PRECISION_U16_LINEAR:
    case CLAY_PRECISION_U16_NON_LINEAR:
    case CLAY_PRECISION_U16_PERCEPTUAL:
    case CLAY_PRECISION_U32_LINEAR:
    case CLAY_PRECISION_U32_NON_LINEAR:
    case CLAY_PRECISION_U32_PERCEPTUAL:
      grain_extract_mode = CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY;
      grain_merge_mode   = CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY;
      break;

    case CLAY_PRECISION_HALF_LINEAR:
    case CLAY_PRECISION_HALF_NON_LINEAR:
    case CLAY_PRECISION_HALF_PERCEPTUAL:
    case CLAY_PRECISION_FLOAT_LINEAR:
    case CLAY_PRECISION_FLOAT_NON_LINEAR:
    case CLAY_PRECISION_FLOAT_PERCEPTUAL:
    case CLAY_PRECISION_DOUBLE_LINEAR:
    case CLAY_PRECISION_DOUBLE_NON_LINEAR:
    case CLAY_PRECISION_DOUBLE_PERCEPTUAL:
      grain_extract_mode = CLAY_LAYER_MODE_GRAIN_EXTRACT;
      grain_merge_mode   = CLAY_LAYER_MODE_GRAIN_MERGE;
      break;
    }

  for (id = 0 ; id < scales; id++)
    {
      ClayLayer *blur;
      ClayLayer *tmp;
      gchar      scale_name[20];

      clay_progress_update ((gdouble) id / (gdouble) scales);

      scale_layers[id] = new_scale;

      g_snprintf (scale_name, sizeof (scale_name), _("Scale %d"), id + 1);
      clay_item_set_name (CLAY_ITEM (new_scale), scale_name);

      tmp = clay_layer_copy (new_scale);
      clay_image_insert_layer (image, tmp, parent,
                               clay_image_get_item_position (image,
                                                             CLAY_ITEM (new_scale)));
      wavelet_blur (CLAY_DRAWABLE (tmp), pow (2.0, id));

      blur = clay_layer_copy (tmp);
      clay_image_insert_layer (image, blur, parent,
                               clay_image_get_item_position (image,
                                                             CLAY_ITEM (tmp)));

      clay_layer_set_mode (tmp, grain_extract_mode);
      new_scale = clay_image_merge_down (image, tmp,
                                         CLAY_EXPAND_AS_NECESSARY);
      scale_layers[id] = new_scale;

      clay_item_set_visible (CLAY_ITEM (new_scale), FALSE);

      new_scale = blur;
    }

  clay_item_set_name (CLAY_ITEM (new_scale), _("Residual"));

  for (id = 0; id < scales; id++)
    {
      clay_image_reorder_item (image, CLAY_ITEM (scale_layers[id]),
                               CLAY_ITEM (parent),
                               clay_image_get_item_position (image,
                                                             CLAY_ITEM (new_scale)));
      clay_layer_set_mode (scale_layers[id], grain_merge_mode);

      if (create_masks)
        {
          ClayLayerMask *mask = clay_layer_create_mask (scale_layers[id],
                                                        CLAY_ADD_MASK_WHITE);
          clay_layer_add_mask (scale_layers[id], mask);
        }

      clay_item_set_visible (CLAY_ITEM (scale_layers[id]), TRUE);
    }

  if (create_group)
    clay_item_set_visible (CLAY_ITEM (parent), TRUE);

  g_free (scale_layers);

  clay_image_thaw_layers (image);

  clay_image_undo_group_end (image);

  clay_progress_update (1.0);

  clay_displays_flush ();

  gegl_exit ();

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}

static void
wavelet_blur (ClayDrawable *drawable,
              gint          radius)
{
  gint x, y, width, height;

  if (clay_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      GeglBuffer *buffer = clay_drawable_get_buffer (drawable);
      GeglBuffer *shadow = clay_drawable_get_shadow_buffer (drawable);

      gegl_render_op (buffer, shadow,
                      "gegl:wavelet-blur",
                      "radius", (gdouble) radius,
                      NULL);

      gegl_buffer_flush (shadow);
      clay_drawable_merge_shadow (drawable, FALSE);
      clay_drawable_update (drawable, x, y, width, height);
      g_object_unref (buffer);
      g_object_unref (shadow);
    }
}

static gboolean
wavelet_decompose_dialog (ClayProcedure *procedure,
                          GObject       *config)
{
  GtkWidget *dialog;
  gboolean   run;

  clay_ui_init (PLUG_IN_BINARY);

  dialog = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Wavelet Decompose"));

  clay_window_set_transient (GTK_WINDOW (dialog));

  /* scales */
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                        "scales", 1.0);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
