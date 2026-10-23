/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImage-save
 * Copyright (C) 2026 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <glib/gstdio.h>

#include "libclaybase/claybase.h"
#ifdef G_OS_WIN32
#include <libclaybase/claywin32-io.h>
#endif
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "gegl/clay-babl.h"

#include "path/claypath.h"
#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "clay.h"
#include "claychannel.h"
#include "claygrid.h"
#include "claygrouplayer.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-colormap.h"
#include "clayimage-grid.h"
#include "clayimage-guides.h"
#include "clayimage-metadata.h"
#include "clayimage-private.h"
#include "clayimage-sample-points.h"
#include "clayimage-savable.h"
#include "clayimage-symmetry.h"
#include "clayimage-undo.h"
#include "claylayer.h"
#include "claylinklayer.h"
#include "clayparasitelist.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "claysymmetry.h"
#include "claysymmetry-mandala.h"
#include "claysymmetry-mirror.h"
#include "claysymmetry-tiling.h"
#include "claytemplate.h"

#include "clay-intl.h"


static gboolean   clay_image_enter_spaces      (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError                **error);
static gboolean   clay_image_exit_spaces       (ClayLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_project     (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError                **error);
static gboolean   clay_image_exit_project      (ClayLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_exit_format       (ClayLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_guides      (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_symmetries  (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_exit_symmetry     (ClayLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_parasites   (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_paths       (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_channels    (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_image_enter_layers      (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);


/* Public Functions */

void
clay_image_save_to_cache (ClayImage *image,
                          GFile     *xcf_file)
{
  const gchar   *folder;
  GFile         *file;
  GOutputStream *output;
  GError        *error = NULL;
  ClaySaveState  state = { 0 };

  g_return_if_fail (CLAY_IS_IMAGE (image));

  folder = clay_image_get_cache_folder (image);
  if (g_mkdir_with_parents (folder,
                            S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    {
      g_critical ("%s: failed to create the image cache folder `%s`: %s\n",
                  G_STRFUNC, folder, g_strerror (errno));
      return;
    }
  file   = clay_image_get_cache_xml_file (image);
  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (output == NULL)
    {
      clay_message (image->clay, NULL, CLAY_MESSAGE_ERROR,
                    _("Error creating '%s': %s"),
                    clay_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
      return;
    }

  state.output   = output;
  state.image    = image;
  state.xcf_file = xcf_file;
  state.spaces   = NULL;
  state.elements = g_queue_new ();

  g_output_stream_printf (output, NULL, NULL, NULL, "<?xml version='1.0' encoding='UTF-8'?>\n");
  clay_savable_print_element_start (&state, "xcf", "version", "%d", WLBR_VERSION, NULL);
  clay_savable_save (CLAY_SAVABLE (image), &state);
  clay_savable_print_element_end (&state, "xcf");

  /* Sanity check: we should be back at root. */
  g_return_if_fail (g_queue_get_length (state.elements) == 0);

  if (! g_output_stream_close (output, NULL, &error))
    {
      clay_message (image->clay, NULL, CLAY_MESSAGE_ERROR,
                    _("Error closing '%s': %s"),
                    clay_file_get_utf8_name (file),
                    error->message);
      g_clear_error (&error);
    }

  g_object_unref (output);
  g_queue_free (state.elements);
}

ClayImage *
clay_image_load_from_cache (Clay  *clay,
                            GFile *backup_dir,
                            gint   ID)
{
  GError        *error = NULL;
  ClayLoadState  state = { 0 };

  if (! clay_savable_load_parse (&state, clay, backup_dir, ID, &error))
    {
      g_printerr ("Error loading '%s': %s\n",
                  clay_file_get_utf8_name (backup_dir),
                  error->message);
      g_clear_error (&error);
    }

  if (state.image)
    clay_image_undo_enable (state.image);

  clay_savable_load_free_state (&state);

  return state.image;
}


/* Protected Functions */

void
clay_image_savable_save (ClaySavable   *savable,
                         ClaySaveState *state)
{
  ClayImage        *image   = CLAY_IMAGE (savable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  GList            *iter;

  if (clay_image_get_file (image))
    clay_savable_print_element (state, "file-path", "%s",
                                g_file_peek_path (clay_image_get_file (image)),
                                NULL);

  /* Saving all ICC profiles stored in this XCF. */
  clay_savable_save_all_spaces (image, state);

  /* Saving the project itself */
  clay_savable_print_element_start (state, "project", NULL);
  /* To avoid having dozens of <project> attributes, I break the various
   * properties down into sub-elements. This will also make these easier
   * to update in further versions, e.g. if we add concept of infinite
   * canvas, or add multi dimension concept (e.g. multi-page documents
   * whose pages may be different dimensions), if we reorganize how we
   * store some data, such as the print dimensions/pixel density
   * arguments, etc.
   */
  clay_savable_print_element (state, "dimensions", NULL, NULL,
                              "width",  "%d", private->width,
                              "height", "%d", private->height,
                              NULL);
  clay_savable_format_save (clay_image_get_layer_format (image, TRUE), state);

  /* Image Properties */
  if (clay_image_get_colormap_palette (image))
    {
      ClayPalette *palette = clay_image_get_colormap_palette (image);
      clay_savable_save (CLAY_SAVABLE (palette), state);
    }
  if (clay_image_get_guides (image))
    {
      clay_savable_print_element_start (state, "guides", NULL);
      iter = clay_image_get_guides (image);
      for (; iter; iter = iter->next)
        clay_savable_save (CLAY_SAVABLE (iter->data), state);
      clay_savable_print_element_end (state, "guides");
    }
  if (clay_image_get_sample_points (image))
    {
      clay_savable_print_element_start (state, "sample-points", NULL);
      iter = clay_image_get_sample_points (image);
      for (; iter; iter = iter->next)
        clay_savable_save (CLAY_SAVABLE (iter->data), state);
      clay_savable_print_element_end (state, "sample-points");
    }
  /* TODO: should we make resolution optional? E.g. for images "made for
   * random screen" instead of "made for printing"?
   */
  clay_savable_print_element (state, "print-dimensions", NULL, NULL,
                              "xres", "%f", private->xresolution,
                              "yres", "%f", private->yresolution,
                              NULL);
  clay_savable_print_element (state, "tattoo",
                              "%u", (guint) clay_image_get_tattoo_state (image),
                              NULL);
  /* XXX: maybe we should merge the image unit  with the print
   * dimensions into some element about physical dimensions?
   */
  clay_savable_unit_save (clay_image_get_unit (image), state);

  if (clay_image_get_grid (image))
    clay_savable_save (CLAY_SAVABLE (clay_image_get_grid (image)), state);

  if (clay_image_get_metadata (image))
    clay_savable_metadata_save (clay_image_get_metadata (image), state);

  if (g_list_length (clay_image_symmetry_get (image)))
    {
      clay_savable_print_element_start (state, "symmetries", NULL);
      for (iter = clay_image_symmetry_get (image); iter; iter = iter->next)
        {
          ClaySymmetry *symmetry;

          symmetry = CLAY_SYMMETRY (iter->data);
          if (G_TYPE_FROM_INSTANCE (symmetry) == CLAY_TYPE_SYMMETRY)
            /* Do not save the identity symmetry. */
            continue;

          clay_savable_config_save (CLAY_CONFIG (symmetry), "symmetry", state);
        }
      clay_savable_print_element_end (state, "symmetries");
    }

  if (clay_parasite_list_length (private->parasites) > 0 &&
      clay_parasite_list_persistent_length (private->parasites) > 0)
    clay_savable_save (CLAY_SAVABLE (private->parasites), state);

  if (private->stored_layer_sets)
    {
      clay_savable_print_element_start (state, "layer-sets", NULL);
      for (iter = private->stored_layer_sets; iter; iter = iter->next)
        clay_savable_save (CLAY_SAVABLE (iter->data), state);
      clay_savable_print_element_end (state, "layer-sets");
    }
  if (private->stored_channel_sets)
    {
      clay_savable_print_element_start (state, "channel-sets", NULL);
      for (iter = private->stored_channel_sets; iter; iter = iter->next)
        clay_savable_save (CLAY_SAVABLE (iter->data), state);
      clay_savable_print_element_end (state, "channel-sets");
    }
  if (private->stored_path_sets)
    {
      clay_savable_print_element_start (state, "path-sets", NULL);
      for (iter = private->stored_path_sets; iter; iter = iter->next)
        clay_savable_save (CLAY_SAVABLE (iter->data), state);
      clay_savable_print_element_end (state, "path-sets");
    }

  clay_savable_print_element_start (state, "paths", NULL);
  iter = clay_image_get_path_iter (image);
  for (; iter; iter = iter->next)
    clay_savable_save (CLAY_SAVABLE (iter->data), state);
  clay_savable_print_element_end (state, "paths");

  clay_savable_print_element_start (state, "channels", NULL);
  iter = clay_image_get_channel_iter (image);
  for (; iter; iter = iter->next)
    clay_savable_save (CLAY_SAVABLE (iter->data), state);
  clay_savable_print_element_end (state, "channels");

  clay_savable_print_element_start (state, "layers", NULL);
  iter = clay_image_get_layer_iter (image);
  for (; iter; iter = iter->next)
    clay_savable_save (CLAY_SAVABLE (iter->data), state);
  clay_savable_print_element_end (state, "layers");

  clay_savable_print_element_end (state, "project");

  g_clear_pointer (&state->spaces, g_hash_table_unref);
}

gboolean
clay_image_load_enter (ClayLoadState  *state,
                       const gchar   **attribute_names,
                       const gchar   **attribute_values,
                       gpointer        user_data,
                       GError         **error)
{
  GHashTable *spaces;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "version") == 0)
        {
          gint version = -1;

          clay_savable_load_store_from_string (state, error,
                                               "version", G_TYPE_INT, *attribute_values,
                                               NULL);
          clay_savable_load_get_values (state, "version", &version, NULL);

          if (version != 1)
            {
              g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                           "%s: unsupported WLBR version %d.",
                           G_STRFUNC, version);
              return FALSE;
            }
        }
      else
        {
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  clay_savable_load_add_simple_handler (state, "file-path", G_TYPE_STRING,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);

  spaces = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  clay_savable_load_add_handlers (state, "spaces",
                                  clay_image_enter_spaces,
                                  clay_image_exit_spaces,
                                  spaces, NULL);
  clay_savable_load_add_handlers (state, "project",
                                  clay_image_enter_project,
                                  clay_image_exit_project,
                                  NULL, NULL);

  return TRUE;
}


/* Private Functions */

static gboolean
clay_image_enter_spaces (ClayLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError         **error)
{
  /* <formats> has no attributes. */
  while (*attribute_names)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: unexpected attribute: '%s'",
                   G_STRFUNC, *attribute_names);
      attribute_names++;
      attribute_values++;
    }

  clay_savable_load_add_handlers (state, "space",
                                  clay_savable_enter_space,
                                  clay_savable_exit_space,
                                  user_data, NULL);
  return TRUE;
}

static gboolean
clay_image_exit_spaces (ClayLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  GHashTable *spaces = (GHashTable *) user_data;

  if (! state->spaces)
    {
      /* Spaces are referenced all throughout a project so we don't just
       * bubble this up and store it in the load state.
       */
      state->spaces = spaces;
    }
  else
    {
      /* Though not standard, this would allow concatenating several
       * <spaces/> lists.
       */
      for (GList *iter = g_hash_table_get_keys (spaces); iter; iter = iter->next)
        {
          const Babl *space = g_hash_table_lookup (spaces, iter->data);

          g_hash_table_insert (state->spaces, g_strdup (iter->data), (gpointer) space);
        }
    }

  return TRUE;
}

static gboolean
clay_image_enter_project (ClayLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError         **error)
{
  const gchar *path = NULL;

  if (clay_savable_load_get_parent_values (state, "file-path", &path, NULL))
    clay_savable_load_store_from_string (state, NULL, "xcf-path", G_TYPE_STRING, path, NULL);

  clay_savable_load_add_simple_handler (state, "dimensions", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, TRUE,
                                        "width",  G_TYPE_INT,
                                        "height", G_TYPE_INT,
                                        NULL);
  clay_savable_load_add_handlers (state, "format",
                                  clay_savable_enter_format,
                                  clay_image_exit_format,
                                  NULL, NULL);
  clay_savable_load_add_handlers (state, "guides",
                                  clay_image_enter_guides,
                                  NULL, NULL, NULL);

  clay_savable_load_add_simple_handler (state, "print-dimensions", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "xres", G_TYPE_DOUBLE,
                                        "yres", G_TYPE_DOUBLE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "tattoo", G_TYPE_UINT,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_handlers (state, "unit",
                                  clay_savable_enter_unit,
                                  clay_savable_exit_unit,
                                  NULL, NULL);

  clay_savable_load (CLAY_TYPE_GRID, state);

  clay_savable_load_add_simple_handler (state, "metadata", G_TYPE_STRING,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_handlers (state, "symmetries",
                                  clay_image_enter_symmetries,
                                  NULL, NULL, NULL);
  clay_savable_load_add_handlers (state, "parasites",
                                  clay_image_enter_parasites,
                                  NULL, NULL, NULL);
  clay_savable_load_add_handlers (state, "paths",
                                  clay_image_enter_paths,
                                  NULL, NULL, NULL);
  clay_savable_load_add_handlers (state, "channels",
                                  clay_image_enter_channels,
                                  NULL, NULL, NULL);
  clay_savable_load_add_handlers (state, "layers",
                                  clay_image_enter_layers,
                                  NULL, NULL, NULL);
  return TRUE;
}

static gboolean
clay_image_exit_project (ClayLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  ClayUnit    *unit        = NULL;
  ClayGrid    *grid        = NULL;
  const gchar *meta_string = NULL;
  gdouble      xres        = -1.0;
  gdouble      yres        = -1.0;
  guint        tattoo      = 0;

  clay_savable_load_get_values (state,
                                "unit", &unit,
                                "print-dimensions:xres", &xres,
                                "print-dimensions:yres", &yres,
                                "tattoo",                &tattoo,
                                "grid",                  &grid,
                                "metadata",              &meta_string,
                                NULL);
  if (unit)
    clay_image_set_unit (state->image, unit);

  if (xres > 0.0 && yres > 0.0)
    {
      if (xres < CLAY_MIN_RESOLUTION || xres > CLAY_MAX_RESOLUTION ||
          yres < CLAY_MIN_RESOLUTION || yres > CLAY_MAX_RESOLUTION)
        {
          ClayTemplate *template = state->clay->config->default_image;

          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                       "Warning, resolution out of range in XCF file");
          xres = clay_template_get_resolution_x (template);
          yres = clay_template_get_resolution_y (template);
        }

      clay_image_set_resolution (state->image, xres, yres);
    }

  if (tattoo > 0)
    clay_image_set_tattoo_state (state->image, tattoo);

  if (grid)
    clay_image_set_grid (state->image, grid, FALSE);

  if (meta_string)
    {
      ClayMetadata *metadata;

      metadata = clay_metadata_deserialize (meta_string);
      if (metadata)
        {
          clay_image_set_metadata (state->image, metadata, FALSE);
          g_object_unref (metadata);
        }
    }

  return TRUE;
}

static gboolean
clay_image_exit_format (ClayLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  ClayImage         *image;
  const Babl        *format = NULL;
  const gchar       *path   = NULL;
  gint               width  = 0;
  gint               height = 0;
  ClayImageBaseType  image_type;
  ClayPrecision      precision;

  /* Run the generic exit_format() handler, but additionally (in the
   * case of ClayImage load code), actually create the image!
   *
   * XXX Note that this current logic implies that the <dimensions/> and
   * <format/> elements were both happening first and in this order. I'm
   * not sure if we should just enforce element orders with xs:sequence
   * schema.
   */
  clay_savable_exit_format (state, text, len, user_data, error);
  clay_savable_load_get_parent_values (state,
                                       "format",            &format,
                                       "dimensions:width",  &width,
                                       "dimensions:height", &height,
                                       NULL);

  image_type = clay_babl_format_get_base_type (format);
  precision  = clay_babl_format_get_precision (format);
  image      = g_object_new (CLAY_TYPE_IMAGE,
                             "clay",      state->clay,
                             "width",     width,
                             "height",    height,
                             "base-type", image_type,
                             "precision", precision,
                             "id",        state->ID,
                             NULL);
  clay_image_undo_disable (image);
  state->image = image;

  if (clay_savable_load_get_parent_values (state, "xcf-path", &path, NULL))
    {
      GFile *file = NULL;

      if (! g_path_is_absolute (path))
        CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                 "%s: image path is not absolute: %s",
                                 G_STRFUNC, path);
      else
        file = g_file_new_for_path (path);

      if (file)
        clay_image_set_file (state->image, file);

      g_clear_object (&file);
    }

  return TRUE;
}

static gboolean
clay_image_enter_guides (ClayLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  if (state->image == NULL)
    {
      /* The image was not created. */
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: basic dimensions and/or format information "
                   "to create the image were missing.",
                   G_STRFUNC);
      return FALSE;
    }

  clay_savable_load (CLAY_TYPE_GUIDE, state);

  return TRUE;
}

static gboolean
clay_image_enter_symmetries (ClayLoadState  *state,
                             const gchar   **attribute_names,
                             const gchar   **attribute_values,
                             gpointer        user_data,
                             GError        **error)
{
  /* Making sure the types are registered and known. */
  (void) CLAY_TYPE_MANDALA;
  (void) CLAY_TYPE_MIRROR;
  (void) CLAY_TYPE_TILING;

  clay_savable_config_load (CLAY_TYPE_SYMMETRY, "symmetry", state,
                            clay_image_exit_symmetry,
                            "image", CLAY_TYPE_IMAGE, state->image,
                            NULL);

  return TRUE;
}

static gboolean
clay_image_exit_symmetry (ClayLoadState  *state,
                          const gchar    *text,
                          gsize           len,
                          gpointer        user_data,
                          GError        **error)
{
  ClaySymmetry *symmetry = NULL;

  clay_savable_load_get_values (state, "symmetry", &symmetry, NULL);

  if (symmetry)
    {
      clay_image_symmetry_add (state->image, symmetry);

      g_signal_emit_by_name (symmetry, "active-changed", NULL);
      if (symmetry->active)
        clay_image_set_active_symmetry (state->image,
                                        G_TYPE_FROM_INSTANCE (symmetry));
    }

  return TRUE;
}

static gboolean
clay_image_enter_parasites (ClayLoadState  *state,
                            const gchar   **attribute_names,
                            const gchar   **attribute_values,
                            gpointer        user_data,
                            GError        **error)
{
  clay_savable_parasite_load (state, G_OBJECT (state->image));

  return TRUE;
}

static gboolean
clay_image_enter_paths (ClayLoadState  *state,
                        const gchar   **attribute_names,
                        const gchar   **attribute_values,
                        gpointer        user_data,
                        GError        **error)
{
  g_return_val_if_fail (clay_savable_load_peek_active_object (state) == NULL, FALSE);
  clay_savable_load (CLAY_TYPE_PATH, state);

  return TRUE;
}

static gboolean
clay_image_enter_channels (ClayLoadState  *state,
                           const gchar   **attribute_names,
                           const gchar   **attribute_values,
                           gpointer        user_data,
                           GError        **error)
{
  g_return_val_if_fail (clay_savable_load_peek_active_object (state) == NULL, FALSE);
  clay_savable_load (CLAY_TYPE_CHANNEL, state);

  return TRUE;
}

static gboolean
clay_image_enter_layers (ClayLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  g_return_val_if_fail (clay_savable_load_peek_active_object (state) == NULL, FALSE);

  clay_savable_load (CLAY_TYPE_LAYER, state);
  clay_savable_load (CLAY_TYPE_GROUP_LAYER, state);
  clay_savable_load (CLAY_TYPE_VECTOR_LAYER, state);
  clay_savable_load (CLAY_TYPE_LINK_LAYER, state);
  clay_savable_load (CLAY_TYPE_TEXT_LAYER, state);

  return TRUE;
}
