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

/*
 * This filter tiles an image to arbitrary width and height
 */
#include "config.h"

#include <string.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-tile"
#define PLUG_IN_BINARY "tile"
#define PLUG_IN_ROLE   "clay-tile"


typedef struct _Tile      Tile;
typedef struct _TileClass TileClass;

struct _Tile
{
  ClayPlugIn parent_instance;
};

struct _TileClass
{
  ClayPlugInClass parent_class;
};


#define TILE_TYPE  (tile_get_type ())
#define TILE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TILE_TYPE, Tile))

GType                   tile_get_type         (void);

static GList          * tile_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * tile_create_procedure (ClayPlugIn           *plug_in,
                                               const gchar          *name);

static ClayValueArray * tile_run              (ClayProcedure        *procedure,
                                               ClayRunMode           run_mode,
                                               ClayImage            *image,
                                               ClayDrawable        **drawables,
                                               ClayProcedureConfig  *config,
                                               gpointer              run_data);

static void             tile                  (ClayImage            *image,
                                               ClayDrawable         *drawable,
                                               gint                  new_width,
                                               gint                  new_height,
                                               gboolean              create_new_image,
                                               ClayImage           **new_image,
                                               ClayLayer           **new_layer);

static void             tile_gegl             (GeglBuffer           *src,
                                               gint                  src_width,
                                               gint                  src_height,
                                               GeglBuffer           *dst,
                                               gint                  dst_width,
                                               gint                  dst_height);

static gboolean         tile_dialog           (ClayProcedure        *procedure,
                                               ClayProcedureConfig  *config,
                                               ClayImage            *image,
                                               ClayDrawable         *drawable);


G_DEFINE_TYPE (Tile, tile, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (TILE_TYPE)
DEFINE_STD_SET_I18N


static void
tile_class_init (TileClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tile_query_procedures;
  plug_in_class->create_procedure = tile_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
tile_init (Tile *tile)
{
}

static GList *
tile_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static ClayProcedure *
tile_create_procedure (ClayPlugIn  *plug_in,
                       const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            tile_run, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");
      clay_procedure_set_sensitivity_mask (procedure,
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLE);

      clay_procedure_set_menu_label (procedure, _("_Tile..."));
      clay_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      clay_procedure_set_documentation (procedure,
                                        _("Create an array of copies "
                                          "of the image"),
                                        "This function creates a new image "
                                        "with a single layer sized to the "
                                        "specified 'new_width' and "
                                        "'new_height' parameters. The "
                                        "specified drawable is tiled into "
                                        "this layer.  The new layer will have "
                                        "the same type as the specified "
                                        "drawable and the new image will "
                                        "have a corresponding base type.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1996-1997");

      /* XXX: we actually don't care about the default value as we always set
       * the width/height of the active image, though it would be nice to reuse
       * the previously stored value if we run tile on the same image twice.
       * TODO: a flag and/or some special handler function would be interesting
       * for such specific cases where a hardcoded default value doesn't make
       * sense or when we want stored values to only apply when run on a same
       * image.
       */
      clay_procedure_add_int_argument (procedure, "new-width",
                                       _("New _width"),
                                       _("New (tiled) image width"),
                                       1, CLAY_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "new-height",
                                       _("New _height"),
                                       _("New (tiled) image height"),
                                       1, CLAY_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      clay_procedure_add_unit_aux_argument (procedure, "tile-size-unit",
                                            _("Tile size unit of measure"),
                                            _("Tile size unit of measure"),
                                            TRUE, TRUE, clay_unit_pixel (),
                                            CLAY_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "new-image",
                                           _("New _image"),
                                           _("Create a new image"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_image_return_value (procedure, "new-image",
                                             "New image",
                                             "Output image (NULL if new-image == FALSE)",
                                             TRUE,
                                             G_PARAM_READWRITE);

      clay_procedure_add_layer_return_value (procedure, "new-layer",
                                             "New layer",
                                             "Output layer (NULL if new-image == FALSE)",
                                             TRUE,
                                             G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
tile_run (ClayProcedure        *procedure,
          ClayRunMode           run_mode,
          ClayImage            *image,
          ClayDrawable        **drawables,
          ClayProcedureConfig  *config,
          gpointer              run_data)
{
  ClayValueArray *return_vals;
  ClayLayer      *new_layer;
  ClayImage      *new_image;
  ClayDrawable   *drawable;
  gint            new_width;
  gint            new_height;
  gboolean        create_new_image;

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

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      if (! tile_dialog (procedure, config, image, drawable))
        {
          return clay_procedure_new_return_values (procedure,
                                                   CLAY_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case CLAY_RUN_NONINTERACTIVE:
      break;

    case CLAY_RUN_WITH_LAST_VALS:
      break;
    }

  g_object_get (config,
                "new-width",  &new_width,
                "new-height", &new_height,
                "new-image",  &create_new_image,
                NULL);

  clay_progress_init (_("Tiling"));

  tile (image, drawable, new_width, new_height,
        create_new_image, &new_image, &new_layer);

  if (run_mode != CLAY_RUN_NONINTERACTIVE)
    {
      if (create_new_image)
        clay_display_new (new_image);
      else
        clay_displays_flush ();
    }

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, new_image);
  CLAY_VALUES_SET_LAYER (return_vals, 2, new_layer);

  return return_vals;
}

static void
tile_gegl (GeglBuffer  *src,
           gint         src_width,
           gint         src_height,
           GeglBuffer  *dst,
           gint         dst_width,
           gint         dst_height)
{
  GeglNode *node;
  GeglNode *buffer_src_node;
  GeglNode *tile_node;
  GeglNode *crop_src_node;
  GeglNode *crop_dst_node;
  GeglNode *buffer_dst_node;

  GeglProcessor *processor;
  gdouble        progress;

  node = gegl_node_new ();

  buffer_src_node = gegl_node_new_child (node,
                                         "operation", "gegl:buffer-source",
                                         "buffer",    src,
                                         NULL);

  crop_src_node = gegl_node_new_child (node,
                                       "operation", "gegl:crop",
                                       "width",     (gdouble) src_width,
                                       "height",    (gdouble) src_height,
                                       NULL);

  tile_node = gegl_node_new_child (node,
                                   "operation", "gegl:tile",
                                   NULL);

  crop_dst_node = gegl_node_new_child (node,
                                       "operation", "gegl:crop",
                                       "width",     (gdouble) dst_width,
                                       "height",    (gdouble) dst_height,
                                       NULL);

  buffer_dst_node = gegl_node_new_child (node,
                                         "operation", "gegl:write-buffer",
                                         "buffer",    dst,
                                         NULL);

  gegl_node_link_many (buffer_src_node,
                       crop_src_node,
                       tile_node,
                       crop_dst_node,
                       buffer_dst_node,
                       NULL);

  processor = gegl_node_new_processor (buffer_dst_node, NULL);

  while (gegl_processor_work (processor, &progress))
    if (!((gint) (progress * 100.0) % 10))
      clay_progress_update (progress);

  clay_progress_update (1.0);

  g_object_unref (processor);
  g_object_unref (node);
}

static void
tile (ClayImage     *image,
      ClayDrawable  *drawable,
      gint           new_width,
      gint           new_height,
      gboolean       create_new_image,
      ClayImage    **new_image,
      ClayLayer    **new_layer)
{
  ClayDrawable *dst_drawable;
  GeglBuffer   *dst_buffer;
  GeglBuffer   *src_buffer;
  gint          src_width  = clay_drawable_get_width (drawable);
  gint          src_height = clay_drawable_get_height (drawable);

  ClayImageBaseType  image_type = CLAY_RGB;

  if (create_new_image)
    {
      /*  create  a new image  */
      ClayColorProfile *profile   = clay_image_get_color_profile (image);
      gint32            precision = clay_image_get_precision (image);

      switch (clay_drawable_type (drawable))
        {
        case CLAY_RGB_IMAGE:
        case CLAY_RGBA_IMAGE:
          image_type = CLAY_RGB;
          break;

        case CLAY_GRAY_IMAGE:
        case CLAY_GRAYA_IMAGE:
          image_type = CLAY_GRAY;
          break;

        case CLAY_INDEXED_IMAGE:
        case CLAY_INDEXEDA_IMAGE:
          image_type = CLAY_INDEXED;
          break;
        }

      *new_image = clay_image_new_with_precision (new_width,
                                                  new_height,
                                                  image_type,
                                                  precision);
      clay_image_undo_disable (*new_image);

      clay_image_set_color_profile (*new_image, profile);
      /*  copy the colormap, if necessary  */
      if (image_type == CLAY_INDEXED)
        clay_image_set_palette (*new_image, clay_image_get_palette (image));

      *new_layer = clay_layer_new (*new_image, _("Background"),
                                   new_width, new_height,
                                   clay_drawable_type (drawable),
                                   100,
                                   clay_image_get_default_new_layer_mode (*new_image));

      if (*new_layer == NULL)
        return;

      clay_image_insert_layer (*new_image, *new_layer, NULL, 0);
      dst_drawable = CLAY_DRAWABLE (*new_layer);
    }
  else
    {
      *new_image = NULL;
      *new_layer = NULL;

      clay_image_undo_group_start (image);
      clay_image_resize (image, new_width, new_height, 0, 0);

      if (clay_item_is_layer (CLAY_ITEM (drawable)))
        {
          clay_layer_resize (CLAY_LAYER (drawable), new_width, new_height, 0, 0);
        }
      else if (clay_item_is_layer_mask (CLAY_ITEM (drawable)))
        {
          ClayLayer *layer = clay_layer_from_mask (CLAY_LAYER_MASK (drawable));

          clay_layer_resize (layer, new_width, new_height, 0, 0);
        }

      dst_drawable = drawable;
    }

  src_buffer = clay_drawable_get_buffer (drawable);
  dst_buffer = clay_drawable_get_buffer (dst_drawable);

  tile_gegl (src_buffer, src_width, src_height,
             dst_buffer, new_width, new_height);

  gegl_buffer_flush (dst_buffer);
  clay_drawable_update (dst_drawable, 0, 0, new_width, new_height);

  if (create_new_image)
    {
      clay_image_undo_enable (*new_image);
    }
  else
    {
      clay_image_undo_group_end (image);
    }

  g_object_unref (src_buffer);
  g_object_unref (dst_buffer);
}

static gboolean
tile_dialog (ClayProcedure       *procedure,
             ClayProcedureConfig *config,
             ClayImage           *image,
             ClayDrawable        *drawable)
{
  GtkWidget *dlg;
  gint       width;
  gint       height;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  clay_ui_init (PLUG_IN_BINARY);

  width  = clay_drawable_get_width (drawable);
  height = clay_drawable_get_height (drawable);

  clay_image_get_resolution (image, &xres, &yres);

  g_object_set (config,
                "new-width",  width,
                "new-height", height,
                NULL);

  dlg = clay_procedure_dialog_new (procedure,
                                   CLAY_PROCEDURE_CONFIG (config),
                                   _("Tile"));
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dlg),
                                   "new-size-label", _("Tile to New Size"),
                                   FALSE, FALSE);

  clay_procedure_dialog_get_coordinates (CLAY_PROCEDURE_DIALOG (dlg),
                                         "coordinates", "new-width",
                                         "new-height", "tile-size-unit",
                                         "%a", CLAY_SIZE_ENTRY_UPDATE_SIZE,
                                         xres, yres);

  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dlg),
                                    "new-size-frame", "new-size-label", FALSE,
                                    "coordinates");
  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dlg), "new-size-frame", "new-image", NULL);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dlg));

  gtk_widget_destroy (dlg);

  return run;
}
