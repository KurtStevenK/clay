/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysavable.c
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "text/claytext-parasite.c"

#include "clay-utils.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "claydrawable.h"
#include "claypalette.h"
#include "claysavable.h"
#include "claysavable-load.h"


static void    clay_savable_print_incomplete_element (ClaySaveState *state,
                                                      const gchar   *element_name,
                                                      va_list        args);
static gchar * clay_savable_printf                   (const gchar   *format,
                                                      va_list        args);


G_DEFINE_INTERFACE (ClaySavable, clay_savable, G_TYPE_OBJECT)


static void
clay_savable_default_init (ClaySavableInterface *iface)
{
}


/*  public functions  */

void
clay_savable_save (ClaySavable   *savable,
                   ClaySaveState *state)
{
  ClaySavableInterface *savable_iface;

  g_return_if_fail (CLAY_IS_SAVABLE (savable));

  savable_iface = CLAY_SAVABLE_GET_IFACE (savable);

  if (savable_iface->save)
    {
      gint n_elements = g_queue_get_length (state->elements);
      savable_iface->save (savable, state);
      /* Sanity check. */
      g_return_if_fail (n_elements == g_queue_get_length (state->elements));
    }
}

void
clay_savable_print_element_start (ClaySaveState *state,
                                  const gchar   *element_name,
                                  ...)
{
  va_list args;

  va_start (args, element_name);
  clay_savable_print_incomplete_element (state, element_name, args);
  g_output_stream_printf (state->output, NULL, NULL, NULL, ">\n");
  va_end (args);

  /* Assuming we only use static strings and don't need to create dups. */
  g_queue_push_head (state->elements, (gpointer) element_name);
}

void
clay_savable_print_element_end (ClaySaveState *state,
                                const gchar   *element_name)
{
  const gchar *current_element = (const gchar *) g_queue_pop_head (state->elements);
  gint         n_elements;

  g_return_if_fail (g_strcmp0 (current_element, element_name) == 0);

  n_elements = g_queue_get_length (state->elements);

  if (n_elements == 0)
    g_output_stream_printf (state->output, NULL, NULL, NULL, "</%s>\n", element_name);
  else
    g_output_stream_printf (state->output, NULL, NULL, NULL, "%*c</%s>\n",
                            n_elements * 2, ' ', element_name);
}

/**
 * clay_savable_print_element:
 * @state:
 * @element_name:
 * @value_format:
 * @...: a main value (or %NULL) followed by (name, format, value)
 *       triplets for each attribute.
 *
 * This will create the element `<@element_name>`.
 *
 * If @value_format is not %NULL, then the next argument needs to be
 * non-%NULL either and of the type corresponding to @value_format.
 *
 * Then, the arguments will be in triplets, with the first being an
 * attribute name, the second a printf-style format (containing a single
 * conversion specifier), the third an attribute value of corresponding
 * type.
 * End the list of attributes with %NULL.
 *
 * Ex:
 * ```C
 * clay_savable_print_element (state, "plop", "%f", 0.5,
 *                             "hello", "%s", "world",
 *                             "id", "%d", 10,
 *                             NULL);
 * ```
 *
 * will print: `<plop hello='world' id='10'>0.5</plop>`
 *
 * Note that additionally to the common supported specifiers, such as
 * "%d", "%u", "%f", etc. this function adds the following modifiers:
 *
 * - "%b": boolean values (will print "true" or "false");
 * - "%t": will print the %GType name of an object.
 * - "%T": will print the name of a %GType.
 */
void
clay_savable_print_element (ClaySaveState *state,
                            const gchar   *element_name,
                            const gchar   *value_format,
                            ...)
{
  gchar   *strval = NULL;
  va_list  args;

  g_return_if_fail (element_name != NULL);

  va_start (args, value_format);

  if (value_format)
    {
      strval = clay_savable_printf (value_format, args);
      g_return_if_fail (strval != NULL);
    }
    else
    {
      /* Ignore the next arg. */
      (void) va_arg (args, void *);
    }

  clay_savable_print_incomplete_element (state, element_name, args);

  if (strval)
    {
      gchar *encoded;

      encoded = g_markup_escape_text (strval, -1);

      g_output_stream_printf (state->output, NULL, NULL, NULL, ">%s</%s>\n",
                              encoded, element_name);

      g_free (strval);
      g_free (encoded);
    }
  else
    {
      g_output_stream_printf (state->output, NULL, NULL, NULL, "/>\n");
    }

  va_end (args);
}

void
clay_savable_config_save (ClayConfig    *config,
                          const gchar   *element_name,
                          ClaySaveState *state)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs = 0;
  guint          i;

  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (element_name != NULL);

  clay_savable_print_element_start (state, element_name,
                                    "type", "%t", config,
                                    NULL);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];
      GValue      value     = G_VALUE_INIT;

      if (! (prop_spec->flags & CLAY_CONFIG_PARAM_SERIALIZE))
        continue;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (config), prop_spec->name, &value);
      clay_savable_value_save (&value, prop_spec, state);
      g_value_unset (&value);
    }

  g_free (property_specs);

  clay_savable_print_element_end (state, element_name);
}

void
clay_savable_format_save (const Babl    *format,
                          ClaySaveState *state)
{
  const Babl  *space;
  const gchar *encoding;

  encoding = babl_format_get_encoding (format);
  clay_savable_print_element_start (state, "format", "encoding", "%s", encoding, NULL);

  space = babl_format_get_space (format);
  clay_savable_space_save (space, state, 0);

  clay_savable_print_element_end (state, "format");
}

void
clay_savable_space_save (const Babl    *space,
                         ClaySaveState *state,
                         gint           new_space_id)
{
  g_return_if_fail (state->spaces != NULL || new_space_id >= 0);

  if (space == babl_space ("sRGB") || space == NULL)
    {
      /* XXX Maybe I should actually also verify the format. A NULL
       * space on a CMYK or other models won't necessarily be sRGB.
       * Or maybe set name='default'?
       */
      clay_savable_print_element (state, "space", NULL, NULL, "name", "%s", "sRGB", NULL);
    }
  else if (state->spaces == NULL)
    {
      /* A NULL references is a special case where we are creating the
       * hash table of all used space in initialization step. This is
       * the only case where new_space_id is used.
       */
      int         icc_length = 0;
      const char *icc = babl_space_get_icc (space, &icc_length);
      gchar      *icc_b64;

      icc_b64 = g_base64_encode ((const guchar*) icc, icc_length);
      clay_savable_print_element_start (state, "space", "id", "space-%d", new_space_id, NULL);
      clay_savable_print_element (state, "icc", "%s", icc_b64, NULL);
      clay_savable_print_element_end (state, "space");

      g_free (icc_b64);
    }
  else
    {
      gpointer key;
      gpointer space_id;

      if (g_hash_table_lookup_extended (state->spaces, babl_get_name (space), &key, &space_id))
        {
          clay_savable_print_element (state, "space", NULL, NULL,
                                      "idref", "space-%d", GPOINTER_TO_UINT (space_id),
                                      NULL);
        }
      else
        {
          /* We stored the easy target spaces at the start of the XCF
           * file, such as spaces of the image itself, layers and from
           * the colormap of indexed images.
           * We don't go as far as looking for colors used as filter
           * arguments, or inside gradient arguments.
           * XXX Should we update clay_savable_save_all_spaces() to also
           * store these?
           * For now let's create an explicit space, but we could
           * imagine it generates duplicates.
           * g_critical ("%s: the space was not previously stored: %s", G_STRFUNC, babl_get_name (space));
           */
          int         icc_length = 0;
          const char *icc = babl_space_get_icc (space, &icc_length);
          gchar      *icc_b64;

          icc_b64 = g_base64_encode ((const guchar*) icc, icc_length);
          clay_savable_print_element_start (state, "space", NULL);
          clay_savable_print_element (state, "icc", "%s", icc_b64, NULL);
          clay_savable_print_element_end (state, "space");

          g_free (icc_b64);
        }
    }
}

void
clay_savable_color_save  (GeglColor     *color,
                          const gchar   *name,
                          const Babl    *restricted_format,
                          ClaySaveState *state)
{
  const Babl *format;
  gchar      *pixel_b64;
  guint8      pixel[40];
  gint        bpp;

  g_return_if_fail (state->spaces != NULL);

  if (name != NULL)
    clay_savable_print_element_start (state, "color", "name", "%s", name, NULL);
  else
    clay_savable_print_element_start (state, "color", NULL);

  /* XXX Do we want to make it possible to have a restricted format yet
   * store the colors in their source format (which may be higher
   * bit-depth)? If we were able to drop the restriction, we would not
   * lose precision data.
   */
  if (restricted_format)
    {
      format = restricted_format;
    }
  else
    {
      format = gegl_color_get_format (color);
      clay_savable_format_save (format, state);
    }

  gegl_color_get_pixel (color, format, pixel);
  bpp = babl_format_get_bytes_per_pixel (format);
  pixel_b64 = g_base64_encode ((const guchar*) pixel, bpp);
  clay_savable_print_element (state, "pixel", "%s", pixel_b64, NULL);

  clay_savable_print_element_end (state, "color");

  g_free (pixel_b64);
}

void
clay_savable_save_all_spaces (ClayImage     *image,
                              ClaySaveState *state)
{
  GHashTable *spaces;
  GList      *iter;
  const Babl *space;
  gint        icc_id = 0;

  g_return_if_fail (state->spaces == NULL);

  spaces = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

  /* Save the space of the image itself. */
  clay_savable_print_element_start (state, "spaces", NULL);
  space = clay_image_get_layer_space (image);
  if (space != NULL && space != babl_space ("sRGB"))
    {
      clay_savable_space_save (space, state, icc_id);
      g_hash_table_insert (spaces, (gpointer) babl_get_name (space), GINT_TO_POINTER (icc_id++));
    }

  /* Save the space of the layers (preparing a future where layers may
   * have their own model and space.
   */
  iter = clay_image_get_layer_list (image);
  for (; iter; iter = iter->next)
    {
      ClayDrawable *drawable = iter->data;

      space = clay_drawable_get_space (drawable);
      if (space != babl_space ("sRGB") &&
          ! g_hash_table_lookup_extended (spaces, babl_get_name (space), NULL, NULL))
        {
          clay_savable_space_save (space, state, icc_id);
          g_hash_table_insert (spaces, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
        }
    }
  g_list_free (iter);

  /* Save the space of the palette colors (preparing a future where they
   * may have their own model and space.
   */
  if (clay_image_get_colormap_palette (image))
    {
      ClayPalette *palette = clay_image_get_colormap_palette (image);
      const Babl  *format;

      format = clay_palette_get_restriction (palette);

      if (format == NULL)
        {
          iter = clay_palette_get_colors (palette);
          for (; iter; iter = iter->next)
            {
              ClayPaletteEntry *entry = iter->data;

              format = gegl_color_get_format (entry->color);
              space = babl_format_get_space (format);
              if (space != babl_space ("sRGB") &&
                  ! g_hash_table_lookup_extended (spaces, babl_get_name (space), NULL, NULL))
                {
                  clay_savable_space_save (space, state, icc_id);
                  g_hash_table_insert (spaces, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
                }
            }
        }
      else
        {
          space = babl_format_get_space (format);
          if (space != babl_space ("sRGB") &&
              ! g_hash_table_lookup_extended (spaces, babl_get_name (space), NULL, NULL))
            {
              clay_savable_space_save (space, state, icc_id);
              g_hash_table_insert (spaces, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
            }
        }
    }
  clay_savable_print_element_end (state, "spaces");

  state->spaces = spaces;
}

void
clay_savable_unit_save (ClayUnit      *unit,
                        ClaySaveState *state)
{
  if (clay_unit_is_built_in (unit))
    {
      guint32 uid = clay_unit_get_id (unit);

      /* Making sure the GType exists. */
      (void) CLAY_TYPE_UNIT_ID;
      clay_savable_print_element (state, "unit", NULL, NULL,
                                  "built-in", "%[ClayUnitID]", uid, NULL);
    }
  else
    {
      clay_savable_print_element_start (state, "unit", NULL);
      clay_savable_print_element (state, "factor",
                                  "%f", clay_unit_get_factor (unit),
                                  NULL);
      clay_savable_print_element (state, "digits",
                                  "%d", clay_unit_get_digits (unit),
                                  NULL);
      clay_savable_print_element (state, "name",
                                  "%s", clay_unit_get_name (unit),
                                  NULL);
      clay_savable_print_element (state, "symbol",
                                  "%s", clay_unit_get_symbol (unit),
                                  NULL);
      clay_savable_print_element (state, "abbreviation",
                                  "%s", clay_unit_get_abbreviation (unit),
                                  NULL);
      clay_savable_print_element_end (state, "unit");
    }
}

void
clay_savable_metadata_save (ClayMetadata  *metadata,
                            ClaySaveState *state)
{
  gchar *meta_string;

  meta_string = clay_metadata_serialize (metadata);
  if (meta_string)
    clay_savable_print_element (state, "metadata", "%s", meta_string, NULL);

  g_free (meta_string);
}

void
clay_savable_parasite_save (ClayParasite  *parasite,
                            ClaySaveState *state)
{
  const gchar *name;

  name  = clay_parasite_get_name (parasite);

  if (clay_parasite_is_persistent (parasite)                           &&
      g_strcmp0 (name, CLAY_ICC_PROFILE_PARASITE_NAME) != 0            &&
      g_strcmp0 (name, CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME) != 0 &&
      g_strcmp0 (name, clay_text_parasite_name ()) != 0)
    {
      gulong         flags;
      gconstpointer  data;
      guint32        data_length;
      gchar         *data_b64;

      flags = clay_parasite_get_flags (parasite);

      clay_savable_print_element_start (state, "parasite",
                                        "name",  "%s",  name,
                                        "flags", "%lu", flags,
                                        NULL);

      data     = clay_parasite_get_data (parasite, &data_length);
      data_b64 = g_base64_encode ((const guchar*) data, data_length);

      clay_savable_print_element_end (state, "parasite");

      g_free (data_b64);
    }
}

void
clay_savable_matrix2_save (ClayMatrix2   *matrix,
                           ClaySaveState *state)
{
  guint dim = 2;

  clay_savable_print_element_start (state, "matrix",
                                    "dim0", "%u", dim,
                                    "dim1", "%u", dim,
                                    NULL);
  for (guint i = 0; i < dim; i++)
    for (guint j = 0; j < dim; j++)
      clay_savable_print_element (state, "coeff", "%f", matrix->coeff[i][j], NULL);

  clay_savable_print_element_end (state, "matrix");
}

void
clay_savable_matrix3_save (ClayMatrix3   *matrix,
                           ClaySaveState *state)
{
  guint dim = 3;

  clay_savable_print_element_start (state, "matrix",
                                    "dim0", "%u", dim,
                                    "dim1", "%u", dim,
                                    NULL);
  for (guint i = 0; i < dim; i++)
    for (guint j = 0; j < dim; j++)
      clay_savable_print_element (state, "coeff", "%f", matrix->coeff[i][j], NULL);

  clay_savable_print_element_end (state, "matrix");
}

void
clay_savable_value_save (GValue        *value,
                         GParamSpec    *pspec,
                         ClaySaveState *state)
{
  const gchar *tag_name = pspec ? pspec->name : "value";

  if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      clay_savable_print_element (state, tag_name, "%b",
                                  g_value_get_boolean (value),
                                  "type", "%T", G_TYPE_BOOLEAN,
                                  NULL);
    }
  else if (G_VALUE_HOLDS_INT (value))
    {
      clay_savable_print_element (state, tag_name,
                                  "%d", g_value_get_int (value),
                                  "type", "%T", G_TYPE_INT,
                                  NULL);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      clay_savable_print_element (state, tag_name,
                                  "%u", g_value_get_uint (value),
                                  "type", "%T", G_TYPE_UINT,
                                  NULL);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      clay_savable_print_element (state, tag_name, "%s",
                                  clay_get_enum_value_nick (G_VALUE_TYPE (value),
                                                            g_value_get_enum (value)),
                                  "type", "%T", G_VALUE_TYPE (value),
                                  NULL);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      const gchar *str = g_value_get_string (value);
      if (str)
        clay_savable_print_element (state, tag_name,
                                    "%s", str,
                                    "type", "%T", G_TYPE_STRING,
                                    NULL);
      else
        clay_savable_print_element (state, tag_name, NULL, NULL,
                                    "type", "%T", G_TYPE_STRING,
                                    "null", "%b", TRUE,
                                    NULL);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      clay_savable_print_element (state, tag_name,
                                  "%f", g_value_get_double (value),
                                  "type", "%T", G_TYPE_DOUBLE,
                                  NULL);
    }
  else if (G_VALUE_HOLDS_FLOAT (value))
    {
      clay_savable_print_element (state, tag_name,
                                  "%f", g_value_get_float (value),
                                  "type", "%T", G_TYPE_FLOAT,
                                  NULL);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_UNIT))
    {
      ClayUnit *unit = g_value_get_object (value);

      clay_savable_print_element_start (state, tag_name,
                                        "type", "%T", CLAY_TYPE_UNIT,
                                        NULL);
      if (unit)
        clay_savable_unit_save (unit, state);
      clay_savable_print_element_end (state, tag_name);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), GEGL_TYPE_COLOR))
    {
      GeglColor *color = g_value_get_object (value);

      clay_savable_print_element_start (state, tag_name,
                                        "type", "%T", GEGL_TYPE_COLOR,
                                        NULL);
      if (color)
        clay_savable_color_save (color, NULL, NULL, state);
      clay_savable_print_element_end (state, tag_name);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_ITEM))
    {
      ClayItem  *item  = g_value_get_object (value);
      ClayImage *image = clay_item_get_image (item);

      /* When saving an item stored in a property or a ClayValueArray,
       * we only store it as a reference, assuming the item is saved in
       * the file. This assumes this is an item from the same image.
       */
      g_return_if_fail (image == state->image);

      clay_savable_print_element (state, tag_name, NULL, NULL,
                                  "type",       "%T", G_VALUE_TYPE (value),
                                  "tattoo-ref", "%u", (guint) clay_item_get_tattoo (item),
                                  NULL);
    }
  else if (G_VALUE_HOLDS_OBJECT (value) &&
           g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_SAVABLE))
    {
      GObject *object = g_value_get_object (value);

      clay_savable_print_element_start (state, tag_name,
                                        "type", "%T", G_VALUE_TYPE (value),
                                        NULL);
      if (object)
        clay_savable_save (CLAY_SAVABLE (object), state);
      clay_savable_print_element_end (state, tag_name);
    }
  else if (CLAY_VALUE_HOLDS_MATRIX2 (value))
    {
      ClayMatrix2 *matrix = g_value_get_boxed (value);

      clay_savable_print_element_start (state, tag_name,
                                        "type", "%T", CLAY_TYPE_MATRIX2,
                                        NULL);
      if (matrix)
        clay_savable_matrix2_save (matrix, state);
      clay_savable_print_element_end (state, tag_name);
    }
  else if (CLAY_VALUE_HOLDS_VALUE_ARRAY (value))
    {
      ClayValueArray *values = g_value_get_boxed (value);

      clay_savable_print_element_start (state, tag_name,
                                        "type", "%T", CLAY_TYPE_VALUE_ARRAY,
                                        NULL);
      if (values)
        clay_savable_value_array_save (values, state);
      clay_savable_print_element_end (state, tag_name);
    }
  else
    {
      if (pspec)
        g_critical ("%s: value for property '%s' of type '%s' is not supported.",
                    G_STRFUNC, pspec->name, g_type_name (G_VALUE_TYPE (value)));
      else
        g_critical ("%s: value of type '%s' is not supported.",
                    G_STRFUNC, g_type_name (G_VALUE_TYPE (value)));
    }
}

void
clay_savable_value_array_save (ClayValueArray *values,
                               ClaySaveState  *state)
{
  gint length = clay_value_array_length (values);

  clay_savable_print_element_start (state, "value-array", NULL);

  for (gint i = 0; i < length; i++)
    {
      GValue *value = clay_value_array_index (values, i);

      clay_savable_value_save (value, NULL, state);
    }

  clay_savable_print_element_end (state, "value-array");
}
void
clay_savable_composite_mode_save (ClayLayerCompositeMode  composite_mode,
                                  ClayLayerMode           mode,
                                  ClaySaveState          *state)
{
  gboolean auto_set = FALSE;

  if (composite_mode == CLAY_LAYER_COMPOSITE_AUTO)
    {
      /* if composite_mode is AUTO, save the actual value AUTO maps to
       * for the given mode, so that we can correctly load it even if
       * the mapping changes in the future.
       */
      composite_mode = clay_layer_mode_get_composite_mode (mode);
      auto_set       = TRUE;
    }

  clay_savable_print_element (state, "composite-mode",
                              "%[ClayLayerCompositeMode]", composite_mode,
                              auto_set ? "auto" : NULL, "%b", TRUE,
                              NULL);
}

void
clay_savable_composite_space_save (ClayLayerColorSpace  composite_space,
                                   ClayLayerMode        mode,
                                   ClaySaveState       *state)
{
  gboolean auto_set = FALSE;

  if (composite_space == CLAY_LAYER_COLOR_SPACE_AUTO)
    {
      /* if composite_space is AUTO, save the actual value AUTO maps to
       * for the given mode, so that we can correctly load it even if
       * the mapping changes in the future.
       */
      composite_space = clay_layer_mode_get_composite_space (mode);
      auto_set        = TRUE;
    }

  clay_savable_print_element (state, "composite-space",
                              "%[ClayLayerColorSpace]", composite_space,
                              auto_set ? "auto" : NULL, "%b", TRUE,
                              NULL);
}

void
clay_savable_blend_space_save (ClayLayerColorSpace  blend_space,
                               ClayLayerMode        mode,
                               ClaySaveState       *state)
{
  gboolean auto_set = FALSE;

  if (blend_space == CLAY_LAYER_COLOR_SPACE_AUTO)
    {
      /* if blend_space is AUTO, save the actual value AUTO maps to for
       * the given mode, so that we can correctly load it even if the
       * mapping changes in the future.
       */
      blend_space = clay_layer_mode_get_blend_space (mode);
      auto_set    = TRUE;
    }

  clay_savable_print_element (state, "blend-space",
                              "%[ClayLayerColorSpace]", blend_space,
                              auto_set ? "auto" : NULL, "%b", TRUE,
                              NULL);
}


/* Private Functions */

static void
clay_savable_print_incomplete_element (ClaySaveState *state,
                                       const gchar   *element_name,
                                       va_list        args)
{
  const gchar *attr;
  const gchar *format;
  gint         n_elements;

  n_elements = g_queue_get_length (state->elements);
  if (n_elements == 0)
    g_output_stream_printf (state->output, NULL, NULL, NULL, "<%s", element_name);
  else
    g_output_stream_printf (state->output, NULL, NULL, NULL, "%*c<%s",
                            n_elements * 2, ' ', element_name);

  attr = va_arg (args, char *);
  while (attr)
    {
      gchar *strval;
      gchar *encoded;

      format = va_arg (args, char *);
      g_return_if_fail (format != NULL);

      strval = clay_savable_printf (format, args);
      g_return_if_fail (strval != NULL);

      encoded = g_markup_escape_text (strval, -1);
      g_output_stream_printf (state->output, NULL, NULL, NULL, " %s='%s'", attr, encoded);

      attr = va_arg (args, char *);

      g_free (strval);
      g_free (encoded);
    }
}

static gchar *
clay_savable_printf (const gchar *format,
                     va_list      args)
{
  gchar *strval;

  g_return_val_if_fail (format != NULL, NULL);

  if (strstr (format, "%s"))
    {
      const gchar *value = va_arg (args, gchar *);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%d"))
    {
      gint value = va_arg (args, gint);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%lu"))
    {
      gulong value = va_arg (args, gulong);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%ld"))
    {
      glong value = va_arg (args, glong);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%u"))
    {
      guint value = va_arg (args, guint);
      strval = g_strdup_printf (format, value);
    }
  else if (strstr (format, "%f"))
    {
      gdouble value = va_arg (args, gdouble);
      strval = g_strdup_printf (format, value);
    }
  else if (g_strcmp0 ("%b", format) == 0)
    {
      /* Custom format: boolean type. */
      gboolean value = va_arg (args, gboolean);
      strval = g_strdup (value ? "true" : "false");
    }
  else if (g_strcmp0 ("%t", format) == 0)
    {
      /* Custom format: type name of passed object. */
      GObject *gobject = va_arg (args, GObject *);
      strval = g_strdup (g_type_name (G_TYPE_FROM_INSTANCE (gobject)));
    }
  else if (g_strcmp0 ("%T", format) == 0)
    {
      /* Custom format: type name. */
      GType gtype = va_arg (args, GType);
      strval = g_strdup (g_type_name (gtype));
    }
  else
    {
      gsize flen = strlen (format);

      if (flen > 3 && format[0] == '%' && format[1] == '[' &&
          format[flen - 1] == ']')
        {
          /* Custom format: enum types. */
          gchar *type_name;
          GType  enum_type;
          gint   value = va_arg (args, gint);

          type_name = g_strdup (format + 2);
          type_name[strlen (type_name) - 1] = '\0';

          enum_type = g_type_from_name (type_name);
          g_free (type_name);
          g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
          strval = g_strdup (clay_get_enum_value_nick (enum_type, value));
        }
      else
        {
          g_return_val_if_reached (NULL);
        }
    }

  return strval;
}
