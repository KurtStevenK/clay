/* Plug-in to load and export .gih (Clay Brush Pipe) files.
 *
 * Copyright (C) 1999 Tor Lillqvist
 * Copyright (C) 2000 Jens Lautenbacher, Sven Neumann
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

  /* Example of how to call file-gih-export from script-fu:

  (let ((ranks (cons-array 1 'byte)))
    (aset ranks 0 12)
    (file-gih-export 1
                     img
                     "foo.gih"
                     "foo.gih"
                     100
                     "test brush"
                     125
                     125
                     3
                     4
                     1
                     ranks
                     1
                     '("random")))
  */


#include "config.h"

#include <libclay/clay.h>
#include <libclay/clayui.h>
#include <libclaybase/clayparasiteio.h>

#include "libclay/stdplugins-intl.h"


#define EXPORT_PROC    "file-gih-export"
#define PLUG_IN_BINARY "file-gih"
#define PLUG_IN_ROLE   "clay-file-gih"


typedef struct
{
  GtkWidget *rank_entry[CLAY_PIXPIPE_MAXDIM];
  GtkWidget *mode_entry[CLAY_PIXPIPE_MAXDIM];
} SizeAdjustmentData;


typedef struct _Gih      Gih;
typedef struct _GihClass GihClass;

struct _Gih
{
  ClayPlugIn      parent_instance;
};

struct _GihClass
{
  ClayPlugInClass parent_class;
};


#define GIH_TYPE  (gih_get_type ())
#define GIH(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIH_TYPE, Gih))

GType                   gih_get_type          (void);

static GList          * gih_query_procedures  (ClayPlugIn           *plug_in);
static ClayProcedure  * gih_create_procedure  (ClayPlugIn           *plug_in,
                                               const gchar          *name);

static ClayValueArray * gih_export            (ClayProcedure        *procedure,
                                               ClayRunMode           run_mode,
                                               ClayImage            *image,
                                               GFile                *file,
                                               ClayExportOptions    *options,
                                               ClayMetadata         *metadata,
                                               ClayProcedureConfig  *config,
                                               gpointer              run_data);

static void             gih_remove_guides     (ClayProcedureConfig  *config,
                                               ClayImage            *image);
static void             gih_cell_width_notify (ClayProcedureConfig  *config,
                                               const GParamSpec     *pspec,
                                               ClayProcedureDialog  *dialog);

static gboolean         gih_save_dialog       (ClayImage            *image,
                                               ClayProcedure        *procedure,
                                               ClayProcedureConfig  *config);


G_DEFINE_TYPE (Gih, gih, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (GIH_TYPE)
DEFINE_STD_SET_I18N


static const gchar * const selection_modes[] = { "incremental",
                                                 "angular",
                                                 "random",
                                                 "velocity",
                                                 "pressure",
                                                 "xtilt",
                                                 "ytilt" };

static const gchar * const mode_labels[] = { N_("Incremental"),
                                             N_("Angular"),
                                             N_("Random"),
                                             N_("Velocity"),
                                             N_("Pressure"),
                                             N_("X tilt"),
                                             N_("Y tilt") };

static void
gih_class_init (GihClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gih_query_procedures;
  plug_in_class->create_procedure = gih_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gih_init (Gih *gih)
{
}

static GList *
gih_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static ClayProcedure *
gih_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, gih_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "RGB*, GRAY*");

      clay_procedure_set_menu_label (procedure, _("Clay brush (animated)"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        _("Exports images in Clay Brush Pipe "
                                          "format"),
                                        _("This plug-in exports an image in "
                                          "the Clay brush pipe format. For a "
                                          "colored brush pipe, RGBA layers are "
                                          "used, otherwise the layers should be "
                                          "grayscale masks. The image can be "
                                          "multi-layered, and additionally the "
                                          "layers can be divided into a "
                                          "rectangular array of brushes."),
                                        EXPORT_PROC);
      clay_procedure_set_attribution (procedure,
                                      "Tor Lillqvist",
                                      "Tor Lillqvist",
                                      "1999");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-clay-gih");
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("Brush Pipe"));
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "gih");
      clay_file_procedure_set_handles_remote (CLAY_FILE_PROCEDURE (procedure),
                                              TRUE);

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB   |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY  |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      clay_procedure_add_int_argument (procedure, "spacing",
                                       _("Spacing (_percent)"),
                                       _("Spacing of the brush"),
                                       1, 1000, 20,
                                       CLAY_PARAM_READWRITE);

      clay_procedure_add_string_argument (procedure, "description",
                                          _("_Description"),
                                          _("Short description of the GIH brush pipe"),
                                          "Clay Brush Pipe",
                                          CLAY_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "cell-width",
                                       _("Cell _width"),
                                       _("Width of the brush cells in pixels"),
                                       1, CLAY_MAX_IMAGE_SIZE, 1,
                                       CLAY_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "cell-height",
                                       _("Cell _height"),
                                       _("Height of the brush cells in pixels"),
                                       1, CLAY_MAX_IMAGE_SIZE, 1,
                                       CLAY_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "num-cells",
                                       _("_Number of cells"),
                                       _("Number of cells to cut up"),
                                       1, 1000, 1,
                                       CLAY_PARAM_READWRITE);

      clay_procedure_add_bytes_argument (procedure, "ranks",
                                         _("_Rank"),
                                         _("Ranks of the dimensions"),
                                         CLAY_PARAM_READWRITE);

      clay_procedure_add_string_array_argument (procedure, "selection-modes",
                                               _("Selection modes"),
                                               _("Selection modes"),
                                               CLAY_PARAM_READWRITE);

      /* Auxiliary arguments. Only useful for the GUI, to pass info around. */

      clay_procedure_add_string_aux_argument (procedure, "info-text",
                                              _("Display as"),
                                              _("Describe how the layers will be split"),
                                              "", CLAY_PARAM_READWRITE);

      clay_procedure_add_int_aux_argument (procedure, "dimension",
                                           _("D_imension"),
                                           _("How many dimensions the animated brush has"),
                                           1, CLAY_PIXPIPE_MAXDIM, 1,
                                           CLAY_PARAM_READWRITE);

      clay_procedure_add_int32_array_aux_argument (procedure, "guides",
                                                   "Guides",
                                                   "Guides to show how the layers will be split in cells",
                                                   CLAY_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
gih_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status    = CLAY_PDB_SUCCESS;
  ClayExportReturn   export    = CLAY_EXPORT_IGNORE;
  ClayLayer        **layers    = NULL;
  ClayParasite      *parasite;
  ClayImage         *orig_image;
  GError            *error     = NULL;

  ClayPixPipeParams  gihparams   = { 0, };
  gchar             *description = NULL;
  gint               spacing;
  GBytes            *rank_bytes;
  const guint8      *rank;
  gchar            **selection;

  orig_image = image;

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      GParamSpec *spec;
      gint        image_width;
      gint        image_height;
      gint        cell_width;
      gint        cell_height;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      clay_pixpipe_params_init (&gihparams);
      G_GNUC_END_IGNORE_DEPRECATIONS

      /*  Possibly retrieve data  */
      parasite = clay_image_get_parasite (orig_image,
                                          "clay-brush-pipe-name");
      if (parasite)
        {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) clay_parasite_get_data (parasite, &parasite_size);

          g_object_set (config, "description", parasite_data, NULL);

          clay_parasite_free (parasite);
        }
      else
        {
          gchar *name = g_path_get_basename (clay_file_get_utf8_name (file));

          if (g_str_has_suffix (name, ".gih"))
            name[strlen (name) - 4] = '\0';

          if (strlen (name))
            g_object_set (config, "description", name, NULL);

          g_free (name);
        }

      parasite = clay_image_get_parasite (orig_image,
                                          "clay-brush-pipe-parameters");
      if (parasite)
       {
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) clay_parasite_get_data (parasite,
                                                            &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);

          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          clay_pixpipe_params_parse (parasite_data, &gihparams);
          G_GNUC_END_IGNORE_DEPRECATIONS

          g_object_set (config,
                        "num-cells", gihparams.ncells,
                        "dimension", gihparams.dim,
                        NULL);

          if (gihparams.dim > 0)
            {
              GBytes  *ranks = NULL;
              guint8   rank_int[gihparams.dim];
              gchar  **selection_modes;

              selection_modes = g_new0 (gchar *, gihparams.dim + 1);

              for (gint i = 0; i < gihparams.dim; i++)
                {
                  selection_modes[i] = gihparams.selection[i];
                  rank_int[i]        = (guint8) gihparams.rank[i];
                }

              ranks =
                g_bytes_new (rank_int, sizeof (guint8) * gihparams.dim);
              g_object_set (config,
                            "ranks",           ranks,
                            "selection-modes", selection_modes,
                            NULL);

              g_bytes_unref (ranks);
              g_free (selection_modes);
            }

          clay_parasite_free (parasite);
          g_free (parasite_data);
       }

      g_object_get (config,
                    "cell-width",  &cell_width,
                    "cell-height", &cell_height,
                    NULL);

      image_width  = clay_image_get_width (image);
      image_height = clay_image_get_height (image);

      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "cell-width");
      G_PARAM_SPEC_INT (spec)->default_value = image_width;
      G_PARAM_SPEC_INT (spec)->maximum       = image_width;

      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "cell-height");
      G_PARAM_SPEC_INT (spec)->default_value = image_height;
      G_PARAM_SPEC_INT (spec)->maximum       = image_height;

      if (cell_width == 1 && cell_height == 1)
        {
          g_object_set (config,
                        "cell-width",  image_width,
                        "cell-height", image_height,
                        NULL);

          spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "num-cells");
          G_PARAM_SPEC_INT (spec)->maximum = 1;
        }

      clay_ui_init (PLUG_IN_BINARY);

      if (! gih_save_dialog (image, procedure, config))
        {
          status = CLAY_PDB_CANCEL;
          goto out;
        }
    }

  export = clay_export_options_get_image (options, &image);
  layers = clay_image_get_layers (image);

  g_object_get (config,
                "spacing",         &spacing,
                "description",     &description,
                "cell-width",      &gihparams.cellwidth,
                "cell-height",     &gihparams.cellheight,
                "num-cells",       &gihparams.ncells,
                "ranks",           &rank_bytes,
                "selection-modes", &selection,
                NULL);

  gihparams.cols = clay_image_get_width (image) / gihparams.cellwidth;
  gihparams.rows = clay_image_get_height (image) / gihparams.cellheight;

  rank           = g_bytes_get_data (rank_bytes, NULL);
  gihparams.dim  = g_bytes_get_size (rank_bytes);

  for (gint i = 0; i < gihparams.dim; i++)
    {
      gihparams.rank[i]      = rank[i];
      gihparams.selection[i] = g_strdup (selection[i]);
    }

  g_strfreev (selection);
  g_bytes_unref (rank_bytes);

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayProcedure  *procedure;
      ClayValueArray *save_retvals;
      gchar          *paramstring;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      paramstring = clay_pixpipe_params_build (&gihparams);
      G_GNUC_END_IGNORE_DEPRECATIONS

      procedure = clay_pdb_lookup_procedure (clay_get_pdb (),
                                             "file-gih-export-internal");
      save_retvals = clay_procedure_run (procedure,
                                         "image",     image,
                                         "drawables", (ClayDrawable **) layers,
                                         "file",      file,
                                         "spacing",   spacing,
                                         "name",      description,
                                         "params",    paramstring,
                                         NULL);

      if (CLAY_VALUES_GET_ENUM (save_retvals, 0) == CLAY_PDB_SUCCESS)
        {
          parasite = clay_parasite_new ("clay-brush-pipe-name",
                                        CLAY_PARASITE_PERSISTENT,
                                        strlen (description) + 1,
                                        description);
          clay_image_attach_parasite (orig_image, parasite);
          clay_parasite_free (parasite);

          clay_image_detach_parasite (image, "clay-brush-pipe-parameters");
        }
      else
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gih-export-internal' "
                       "failed: %s",
                       clay_pdb_get_last_error (clay_get_pdb ()));

          status = CLAY_PDB_EXECUTION_ERROR;
        }

      g_free (paramstring);
    }

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  clay_pixpipe_params_free (&gihparams);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_free (description);

 out:
  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_free (layers);
  return clay_procedure_new_return_values (procedure, status, error);
}


/*  export routines */

static void
gih_remove_guides (ClayProcedureConfig *config,
                   ClayImage           *image)
{
  ClayArray *array;

  g_object_get (config, "guides", &array, NULL);

  if (array != NULL)
    {
      gint32 *guides   = (gint32 *) array->data;
      gint    n_guides = array->length / 4;

      for (gint i = 0; i < n_guides; i++)
        clay_image_delete_guide (image, guides[i]);
    }
  g_clear_pointer (&array, clay_array_free);

  g_object_set (config, "guides", NULL, NULL);
}

static void
gih_cell_width_notify (ClayProcedureConfig *config,
                       const GParamSpec    *pspec,
                       ClayProcedureDialog *dialog)
{
  ClayImage      *image;
  GParamSpec     *spec;
  gchar          *info_text;
  gchar          *grid_text      = NULL;
  const gchar    *width_warning  = _("Width Mismatch!");
  gchar          *height_warning = _("Height Mismatch!");
  GtkWidget      *widget;
  GtkAdjustment  *rank0;
  gint            dimension;

  gint            cell_width;
  gint            image_width;
  gint            n_horiz_cells;
  gint            n_vert_cells;

  gint            cell_height;
  gint            image_height;
  gint            n_vert_guides;
  gint            n_horiz_guides;

  gint            old_max_cells;
  gint            max_cells;
  gint            num_cells;

  image = g_object_get_data (G_OBJECT (dialog), "image");
  rank0 = g_object_get_data (G_OBJECT (dialog), "rank0");

  g_object_get (config,
                "cell-width",  &cell_width,
                "cell-height", &cell_height,
                "num-cells",   &num_cells,
                "dimension",   &dimension,
                NULL);

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "cell-width");
  image_width    = G_PARAM_SPEC_INT (spec)->maximum;
  n_horiz_cells  = image_width / cell_width;
  n_vert_guides  = n_horiz_cells - 1;

  if (cell_width * n_horiz_cells == image_width)
    width_warning = NULL;

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "cell-height");
  image_height   = G_PARAM_SPEC_INT (spec)->maximum;
  n_vert_cells   = image_height / cell_height;
  n_horiz_guides = n_vert_cells - 1;

  if (cell_height * n_vert_cells == image_height)
    height_warning = NULL;

  /* TRANSLATORS: \xc3\x97 is the UTF-8 encoding for the multiplication sign. */
  if (n_horiz_guides + n_vert_guides > 0)
    grid_text = g_strdup_printf (_("Displays as a %d \xc3\x97 %d grid on each layer"),
                                 n_horiz_cells, n_vert_cells);
  info_text = g_strdup_printf ("%s%s%s%s%s",
                               grid_text != NULL ? grid_text : "",
                               grid_text != NULL && width_warning != NULL ? " / " : "",
                               width_warning != NULL ? width_warning : "",
                               (grid_text != NULL || width_warning != NULL) && height_warning != NULL ? " / " : "",
                               height_warning != NULL ? height_warning : "");

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "num-cells");
  old_max_cells = G_PARAM_SPEC_INT (spec)->maximum;
  max_cells     = n_horiz_cells * n_vert_cells;

  if (num_cells == old_max_cells)
    num_cells = max_cells;
  else
    num_cells = MIN (max_cells, num_cells);

  G_PARAM_SPEC_INT (spec)->maximum = max_cells;
  widget = clay_procedure_dialog_get_widget (dialog, "num-cells", G_TYPE_NONE);
  g_object_set (widget, "upper", (gdouble) max_cells, NULL);
  clay_label_spin_set_increments (CLAY_LABEL_SPIN (widget), 1.0, 1.0);

  gih_remove_guides (config, image);

  widget = clay_procedure_dialog_get_widget (dialog, "info-text", G_TYPE_NONE);
  gtk_widget_set_visible (widget, (strlen (info_text) > 0));
  if (n_horiz_guides + n_vert_guides > 0)
    {
      gint32 *guides = NULL;
      GValue  value  = G_VALUE_INIT;

      guides = g_new0 (gint32, n_horiz_guides + n_vert_guides);

      for (gint i = 0; i < n_vert_guides; i++)
        guides[i] = clay_image_add_vguide (image, cell_width * (i + 1));

      for (gint i = 0; i < n_horiz_guides; i++)
        guides[n_vert_guides + i] = clay_image_add_hguide (image, cell_height * (i + 1));

      g_value_init (&value, CLAY_TYPE_INT32_ARRAY);
      clay_value_set_int32_array (&value, guides, n_horiz_guides + n_vert_guides);
      g_object_set_property (G_OBJECT (config), "guides", &value);
      g_free (guides);
    }

  g_object_set (config,
                "info-text", info_text,
                "num-cells", num_cells,
                NULL);
  if (rank0 != NULL && dimension == 1)
    gtk_adjustment_set_value (rank0, num_cells);

  g_free (info_text);
  g_free (grid_text);
}

static void
gih_selection_mode_changed (GtkWidget *widget,
                            gpointer   data)
{
  gint index;

  index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

  g_free (*((gchar **) data));
  *((gchar **) data) = g_strdup (selection_modes [index]);
}

static void
gih_dimension_notify (ClayProcedureConfig *config,
                      const GParamSpec    *pspec,
                      SizeAdjustmentData  *data)
{
  gint dimension;
  gint i;

  g_object_get (config, "dimension", &dimension, NULL);

  for (i = 0; i < CLAY_PIXPIPE_MAXDIM; i++)
    {
      gtk_widget_set_sensitive (data->rank_entry[i], i < dimension);
      gtk_widget_set_sensitive (data->mode_entry[i], i < dimension);
    }
}

static gboolean
gih_save_dialog (ClayImage           *image,
                 ClayProcedure       *procedure,
                 ClayProcedureConfig *config)
{
  GtkWidget           *dialog;
  GtkWidget           *dimgrid;
  GtkWidget           *label;
  GtkAdjustment       *adjustment;
  GtkWidget           *spinbutton;
  GtkWidget           *cb;
  gint                 i;
  SizeAdjustmentData   cellw_adjust;
  SizeAdjustmentData   cellh_adjust;
  gchar              **selection;
  GBytes              *rank_bytes = NULL;
  guint8               rank[CLAY_PIXPIPE_MAXDIM];
  gint                 num_cells;
  gint                 dimension;
  gboolean             run;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  gtk_widget_set_halign (clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                                          "info-text", NULL, TRUE, FALSE),
                         GTK_ALIGN_START);
  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "description", "spacing",
                              "cell-width", "cell-height", "num-cells",
                              "info-text", "dimension", NULL);

  g_object_set_data (G_OBJECT (dialog), "image", image);
  g_signal_connect (config, "notify::cell-width",
                    G_CALLBACK (gih_cell_width_notify),
                    dialog);
  g_signal_connect (config, "notify::cell-height",
                    G_CALLBACK (gih_cell_width_notify),
                    dialog);
  gih_cell_width_notify (config, NULL, CLAY_PROCEDURE_DIALOG (dialog));

  g_signal_connect (config, "notify::dimension",
                    G_CALLBACK (gih_dimension_notify),
                    &cellw_adjust);

  /*
   * Ranks / Selection: ______ ______ ______ ______ ______
   */

  g_object_get (config,
                "selection-modes", &selection,
                "ranks",           &rank_bytes,
                "num-cells",       &num_cells,
                NULL);
  if (selection == NULL || g_strv_length (selection) != CLAY_PIXPIPE_MAXDIM)
    {
      GStrvBuilder *builder = g_strv_builder_new ();
      gint          old_len;

      old_len = (selection == NULL ? 0 : g_strv_length (selection));

      for (i = 0; i < MIN (old_len, CLAY_PIXPIPE_MAXDIM); i++)
        g_strv_builder_add (builder, selection[i]);

      for (i = old_len; i < CLAY_PIXPIPE_MAXDIM; i++)
        g_strv_builder_add (builder, "random");

      g_strfreev (selection);
      selection = g_strv_builder_end (builder);
      g_strv_builder_unref (builder);
    }

  if (rank_bytes == NULL)
    {
      dimension = 1;

      rank[0] = 1;
      for (i = 1; i < CLAY_PIXPIPE_MAXDIM; i++)
        rank[i] = 0;
    }
  else
    {
      const guint8 *stored;

      stored = g_bytes_get_data (rank_bytes, NULL);
      dimension = g_bytes_get_size (rank_bytes);

      for (i = 0; i < dimension; i++)
        rank[i] = stored[i];

      for (i = dimension; i < CLAY_PIXPIPE_MAXDIM; i++)
        rank[i] = 1;
    }
  g_bytes_unref (rank_bytes);
  dimension = MAX (dimension, 1);

  dimgrid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (dimgrid), 4);

  label = gtk_label_new (_("Ranks:"));
  gtk_grid_attach (GTK_GRID (dimgrid), label, 0, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  for (i = 0; i < CLAY_PIXPIPE_MAXDIM; i++)
    {
      gsize j;

      adjustment = gtk_adjustment_new (rank[i], 1, 100, 1, 1, 0);
      if (i == 0)
        g_object_set_data (G_OBJECT (dialog), "rank0", adjustment);

      spinbutton = clay_spin_button_new (adjustment, 1.0, 0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_grid_attach (GTK_GRID (dimgrid), spinbutton, 1, i, 1, 1);

      gtk_widget_set_visible (spinbutton, TRUE);

      if (i >= dimension)
        gtk_widget_set_sensitive (spinbutton, FALSE);

      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (clay_int_adjustment_update),
                        &rank[i]);

      cellw_adjust.rank_entry[i] = cellh_adjust.rank_entry[i] = spinbutton;

      cb = gtk_combo_box_text_new ();

      for (j = 0; j < G_N_ELEMENTS (selection_modes); j++)
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cb),
                                        _(mode_labels[j]));
      gtk_combo_box_set_active (GTK_COMBO_BOX (cb), 2);  /* random */

      for (j = 0; j < G_N_ELEMENTS (selection_modes); j++)
        if (!strcmp (selection[i], selection_modes[j]))
          {
            gtk_combo_box_set_active (GTK_COMBO_BOX (cb), j);
            break;
          }

      gtk_grid_attach (GTK_GRID (dimgrid), cb, 2, i, 1, 1);

      gtk_widget_set_visible (cb, TRUE);

      if (i >= dimension)
        gtk_widget_set_sensitive (cb, FALSE);

      g_signal_connect (GTK_COMBO_BOX (cb), "changed",
                        G_CALLBACK (gih_selection_mode_changed),
                        &selection[i]);

      cellw_adjust.mode_entry[i] = cellh_adjust.mode_entry[i] = cb;
    }

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dimgrid, TRUE, TRUE, 0);
  gtk_widget_set_visible (dimgrid, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  g_object_get (config, "dimension", &dimension, NULL);
  rank_bytes = g_bytes_new (rank, sizeof (guint8) * dimension);

  g_object_set (config,
                "selection-modes", selection,
                "ranks",           rank_bytes,
                NULL);

  gih_remove_guides (config, image);
  gtk_widget_destroy (dialog);

  g_strfreev (selection);
  g_bytes_unref (rank_bytes);

  return run;
}
