/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <glib/gstdio.h>

#include "libclaycolor/claycolor.h"

#include "clay-gegl-types.h"

#include "core/clayerror.h"
#include "core/claydrawable.h"
#include "core/claypattern.h"
#include "core/clayprogress.h"

#include "clay-babl.h"
#include "clay-gegl-loops.h"
#include "clay-gegl-utils.h"


/*  local function prototypes  */

static gboolean   clay_gegl_op_blacklisted         (const gchar         *name,
                                                    const gchar         *categories,
                                                    gboolean             block_clay_ops,
                                                    GError             **error);
static GList    * clay_gegl_get_op_subclasses      (GType                type,
                                                    GList               *classes,
                                                    gboolean             block_clay_ops);
static gint       clay_gegl_compare_op_names       (GeglOperationClass  *a,
                                                    GeglOperationClass  *b);
static void       clay_gegl_buffer_drawable_unlink (gchar               *path);


/* Comes from gegl/operation/gegl-operations.h which is not public. */
GType gegl_operation_gtype_from_name (const gchar *name);


/*  public functions  */

GList *
clay_gegl_get_op_classes (gboolean block_clay_ops)
{
  GList *operations;

  operations = clay_gegl_get_op_subclasses (GEGL_TYPE_OPERATION, NULL, block_clay_ops);

  operations = g_list_sort (operations,
                            (GCompareFunc)
                            clay_gegl_compare_op_names);

  return operations;
}

gboolean
clay_gegl_op_nde_allowed (const gchar  *name,
                          GError      **error)
{
  GType               op_type;
  GeglOperationClass *klass;
  const gchar        *categories;

  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);

  if (name == NULL || strlen (name) == 0)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED, "the filter has no name.");
      return FALSE;
    }
  else if (! gegl_has_operation (name))
    {
      g_set_error (error, CLAY_ERROR, CLAY_FAILED, "the filter \"%s\" is not installed.", name);
      return FALSE;
    }

  op_type    = gegl_operation_gtype_from_name (name);
  klass      = GEGL_OPERATION_CLASS (g_type_class_ref (op_type));
  categories = gegl_operation_class_get_key (klass, "categories");

  return ! clay_gegl_op_blacklisted (name, categories, FALSE, error);
}

GType
clay_gegl_get_op_enum_type (const gchar *operation,
                            const gchar *property)
{
  GeglNode   *node;
  GObject    *op;
  GParamSpec *pspec;

  g_return_val_if_fail (operation != NULL, G_TYPE_NONE);
  g_return_val_if_fail (property != NULL, G_TYPE_NONE);

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation,
                       NULL);
  g_object_get (node, "gegl-operation", &op, NULL);
  g_object_unref (node);

  g_return_val_if_fail (op != NULL, G_TYPE_NONE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (op), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (pspec), G_TYPE_NONE);

  g_object_unref (op);

  return G_TYPE_FROM_CLASS (G_PARAM_SPEC_ENUM (pspec)->enum_class);
}

static void
clay_gegl_progress_callback (GObject      *object,
                             gdouble       value,
                             ClayProgress *progress)
{
  if (value == 0.0)
    {
      const gchar *text = g_object_get_data (object, "clay-progress-text");

      if (clay_progress_is_active (progress))
        clay_progress_set_text (progress, "%s", text);
      else
        clay_progress_start (progress, FALSE, "%s", text);
    }
  else
    {
      clay_progress_set_value (progress, value);

      if (value == 1.0)
        clay_progress_end (progress);
    }
}

void
clay_gegl_progress_connect (GeglNode     *node,
                            ClayProgress *progress,
                            const gchar  *text)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (CLAY_IS_PROGRESS (progress));
  g_return_if_fail (text != NULL);

  g_signal_connect_object (node, "progress",
                           G_CALLBACK (clay_gegl_progress_callback),
                           progress, 0);

  g_object_set_data_full (G_OBJECT (node),
                          "clay-progress-text", g_strdup (text),
                          (GDestroyNotify) g_free);
}

void
clay_gegl_progress_disconnect (GeglNode     *node,
                               ClayProgress *progress)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (CLAY_IS_PROGRESS (progress));

  g_signal_handlers_disconnect_by_func (node,
                                        clay_gegl_progress_callback,
                                        progress);
}

gboolean
clay_gegl_node_is_source_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_SOURCE (operation);
}

gboolean
clay_gegl_node_is_point_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_POINT_RENDER    (operation) ||
         GEGL_IS_OPERATION_POINT_FILTER    (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER  (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER3 (operation);
}

gboolean
clay_gegl_node_is_area_filter_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_AREA_FILTER (operation) ||
         /* be conservative and return TRUE for meta ops, since they may
          * involve an area op
          */
         GEGL_IS_OPERATION_META (operation);
}

const gchar *
clay_gegl_node_get_key (GeglNode    *node,
                        const gchar *key)
{
  const gchar *operation_name;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  operation_name = gegl_node_get_operation (node);

  if (operation_name)
    return gegl_operation_get_key (operation_name, key);
  else
    return NULL;
}

gboolean
clay_gegl_node_has_key (GeglNode    *node,
                        const gchar *key)
{
  return clay_gegl_node_get_key (node, key) != NULL;
}

const Babl *
clay_gegl_node_get_format (GeglNode    *node,
                           const gchar *pad_name)
{
  GeglOperation *op;
  const Babl    *format = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);
  g_return_val_if_fail (pad_name != NULL, NULL);

  g_object_get (node, "gegl-operation", &op, NULL);

  if (op)
    {
      format = gegl_operation_get_format (op, pad_name);

      g_object_unref (op);
    }

  if (! format)
    format = babl_format ("RGBA float");

  return format;
}

void
clay_gegl_node_set_underlying_operation (GeglNode *node,
                                         GeglNode *operation)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (operation == NULL || GEGL_IS_NODE (operation));

  g_object_set_data (G_OBJECT (node),
                     "clay-gegl-node-underlying-operation", operation);
}

GeglNode *
clay_gegl_node_get_underlying_operation (GeglNode *node)
{
  GeglNode *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  operation = g_object_get_data (G_OBJECT (node),
                                 "clay-gegl-node-underlying-operation");

  if (operation)
    return clay_gegl_node_get_underlying_operation (operation);
  else
    return node;
}

gboolean
clay_gegl_param_spec_has_key (GParamSpec  *pspec,
                              const gchar *key,
                              const gchar *value)
{
  const gchar *v = gegl_param_spec_get_property_key (pspec, key);

  if (v && ! strcmp (v, value))
    return TRUE;

  return FALSE;
}

GeglBuffer *
clay_gegl_buffer_new (const GeglRectangle *extent,
                      const Babl          *format,
                      ClayDrawable        *drawable)
{
  GeglBuffer *buffer;
  gchar      *path = NULL;

  g_return_val_if_fail (drawable == NULL || CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (extent != NULL, NULL);

  if (drawable)
    path = clay_drawable_get_new_cache_path (drawable);

  buffer = g_object_new (GEGL_TYPE_BUFFER,
                         "format", format,
                         "path",   path,
                         "x",      extent->x,
                         "y",      extent->y,
                         "width",  extent->width,
                         "height", extent->height,
                         NULL);

  if (path)
    /* Trick to delete the on-disk file when destroying the buffer. */
    g_object_set_data_full (G_OBJECT (buffer),
                            "clay-gegl-buffer-drawable-destroy", path,
                            (GDestroyNotify) clay_gegl_buffer_drawable_unlink);

  return buffer;
}

GeglBuffer *
clay_gegl_buffer_dup (GeglBuffer   *buffer,
                      ClayDrawable *drawable)
{
  GeglBuffer          *new_buffer;
  const GeglRectangle *extent;
  const GeglRectangle *abyss;
  gchar               *path = NULL;
  GeglRectangle        rect;
  gint                 shift_x;
  gint                 shift_y;
  gint                 tile_width;
  gint                 tile_height;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (drawable == NULL || CLAY_IS_DRAWABLE (drawable), NULL);

  extent = gegl_buffer_get_extent (buffer);
  abyss  = gegl_buffer_get_abyss  (buffer);
  if (drawable)
    path = clay_drawable_get_new_cache_path (drawable);

  g_object_get (buffer,
                "shift-x",     &shift_x,
                "shift-y",     &shift_y,
                "tile-width",  &tile_width,
                "tile-height", &tile_height,
                NULL);

  new_buffer = g_object_new (GEGL_TYPE_BUFFER,
                             "format",       gegl_buffer_get_format (buffer),
                             "x",            extent->x,
                             "y",            extent->y,
                             "width",        extent->width,
                             "height",       extent->height,
                             "abyss-x",      abyss->x,
                             "abyss-y",      abyss->y,
                             "abyss-width",  abyss->width,
                             "abyss-height", abyss->height,
                             "shift-x",      shift_x,
                             "shift-y",      shift_y,
                             "tile-width",   tile_width,
                             "tile-height",  tile_height,
                             "path",         path,
                             NULL);

  gegl_rectangle_align_to_buffer (&rect, extent, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  clay_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                         new_buffer, &rect);

  if (path)
    /* Trick to delete the on-disk file when destroying the buffer. */
    g_object_set_data_full (G_OBJECT (new_buffer),
                            "clay-gegl-buffer-drawable-destroy", path,
                            (GDestroyNotify) clay_gegl_buffer_drawable_unlink);

  return new_buffer;
}

GeglBuffer *
clay_gegl_buffer_resize (GeglBuffer   *buffer,
                         gint          new_width,
                         gint          new_height,
                         gint          offset_x,
                         gint          offset_y,
                         GeglColor    *color,
                         ClayPattern  *pattern,
                         gint          pattern_offset_x,
                         gint          pattern_offset_y)
{
  GeglBuffer          *new_buffer;
  gboolean             intersect;
  GeglRectangle        copy_rect;
  const GeglRectangle *extent;
  const Babl          *format;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  extent     = gegl_buffer_get_extent (buffer);
  format     = gegl_buffer_get_format (buffer);
  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, new_width, new_height),
                                format);

  intersect = gegl_rectangle_intersect (&copy_rect,
                                        GEGL_RECTANGLE (0, 0, extent->width,
                                                        extent->height),
                                        GEGL_RECTANGLE (offset_x, offset_y,
                                                        new_width, new_height));

  if (! intersect                   ||
      copy_rect.width  != new_width ||
      copy_rect.height != new_height)
    {
      /*  Clear the new buffer if needed and color/pattern is given */
      if (pattern)
        {
          GeglBuffer       *src_buffer;
          GeglBuffer       *dest_buffer;
          ClayColorProfile *src_profile;
          ClayColorProfile *dest_profile;

          src_buffer = clay_pattern_create_buffer (pattern);

          src_profile  = clay_babl_format_get_color_profile (
                           gegl_buffer_get_format (src_buffer));
          dest_profile = clay_babl_format_get_color_profile (
                           gegl_buffer_get_format (new_buffer));

          if (clay_color_transform_can_gegl_copy (src_profile, dest_profile))
            {
              dest_buffer = g_object_ref (src_buffer);
            }
          else
            {
              dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                             gegl_buffer_get_format (new_buffer));

              clay_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                               dest_buffer, NULL, dest_profile,
                                               CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                               TRUE, NULL);
            }

          g_object_unref (src_profile);
          g_object_unref (dest_profile);

          gegl_buffer_set_pattern (new_buffer, NULL, dest_buffer,
                                   pattern_offset_x, pattern_offset_y);

          g_object_unref (src_buffer);
          g_object_unref (dest_buffer);
        }
      else if (color)
        {
          gegl_buffer_set_color (new_buffer, NULL, color);
        }
    }

  if (intersect && copy_rect.width && copy_rect.height)
    {
      /*  Copy the pixels in the intersection  */
      clay_gegl_buffer_copy (buffer, &copy_rect, GEGL_ABYSS_NONE, new_buffer,
                             GEGL_RECTANGLE (copy_rect.x - offset_x,
                                             copy_rect.y - offset_y, 0, 0));
    }

  return new_buffer;
}

gboolean
clay_gegl_buffer_set_extent (GeglBuffer          *buffer,
                             const GeglRectangle *extent)
{
  GeglRectangle aligned_old_extent;
  GeglRectangle aligned_extent;
  GeglRectangle old_extent_rem;
  GeglRectangle diff_rects[4];
  gint          n_diff_rects;
  gint          i;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);
  g_return_val_if_fail (extent != NULL, FALSE);

  gegl_rectangle_align_to_buffer (&aligned_old_extent,
                                  gegl_buffer_get_extent (buffer), buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);
  gegl_rectangle_align_to_buffer (&aligned_extent,
                                  extent, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                          &aligned_old_extent,
                                          &aligned_extent);

  for (i = 0; i < n_diff_rects; i++)
    gegl_buffer_clear (buffer, &diff_rects[i]);

  if (gegl_rectangle_intersect (&old_extent_rem,
                                gegl_buffer_get_extent (buffer),
                                &aligned_extent))
    {
      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &old_extent_rem,
                                              extent);

      for (i = 0; i < n_diff_rects; i++)
        gegl_buffer_clear (buffer, &diff_rects[i]);
    }

  return gegl_buffer_set_extent (buffer, extent);
}


/*  private functions  */

static gboolean
clay_gegl_op_blacklisted (const gchar  *name,
                          const gchar  *categories_str,
                          gboolean      block_clay_ops,
                          GError      **error)
{
  static const gchar * const category_blacklist[] =
  {
    "compositors",
    "core",
    "debug",
    "display",
    "hidden",
    "input",
    "output",
    "programming",
    "transform",
    "video"
  };
  static const gchar * const name_blacklist[] =
  {
    /* these ops are blacklisted for other reasons */
    "gegl:color", /* pointless */
    "gegl:contrast-curve",
    "gegl:convert-format", /* pointless */
    "gegl:ditto", /* pointless */
    "gegl:fill-path",
    "gegl:gray", /* we use clay's op */
    "gegl:hstack", /* deleted from GEGL and replaced by gegl:pack */
    "gegl:introspect", /* pointless */
    "gegl:json:dropshadow2", /* has shortcomings, and duplicates gegl:dropshadow */
    "gegl:json:grey2", /* has shortcomings, and duplicates gegl:gray */
    "gegl:layer", /* we use clay's ops */
    "gegl:lcms-from-profile", /* not usable here */
    "gegl:linear-gradient", /* we use the blend tool */
    "gegl:map-absolute", /* pointless */
    "gegl:map-relative", /* pointless */
    "gegl:matting-global", /* used in the foreground select tool */
    "gegl:matting-levin", /* used in the foreground select tool */
    "gegl:nop", /* pointless */
    "gegl:opacity", /* poinless */
    "gegl:pack", /* pointless */
    "gegl:path",
    "gegl:posterize", /* we use clay's op */
    "gegl:radial-gradient", /* we use the blend tool */
    "gegl:rectangle", /* pointless */
    "gegl:seamless-clone", /* used in the seamless clone tool */
    "gegl:text", /* we use clay's text rendering */
    "gegl:threshold", /* we use clay's op */
    "gegl:tile", /* pointless */
    "gegl:unpremul", /* pointless */
    "gegl:vector-stroke",
    "gegl:wavelet-blur", /* we use clay's op wavelet-decompose */
  };

  GType   op_type;
  gint    i;

  /* Operations with no name are abstract base classes */
  if (! name)
    return TRUE;

  g_return_val_if_fail (error == NULL || *error == NULL, TRUE);

  /* use this flag to include all ops for testing */
  if (g_getenv ("CLAY_TESTING_NO_GEGL_BLACKLIST"))
    return FALSE;

  if (block_clay_ops && g_str_has_prefix (name, "clay"))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           "Filters in namespace \"clay:\" are hidden.");
      return TRUE;
    }

  for (i = 0; i < G_N_ELEMENTS (name_blacklist); i++)
    {
      if (! strcmp (name, name_blacklist[i]))
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               "The filter is hidden.");
          return TRUE;
        }
    }

  if (g_strcmp0 (name, "gegl:gegl") == 0)
    {
      if (g_getenv ("CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT") == NULL)
        {
          g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                       "%s\n%s", "The filter is unsafe.",
                       "For development purpose, set environment variable CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT.");
          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }

  if (categories_str)
    {
      gchar **categories;

      categories = g_strsplit (categories_str, ":", 0);

      for (i = 0; i < G_N_ELEMENTS (category_blacklist); i++)
        {
          gint j;

          for (j = 0; categories[j]; j++)
            if (! strcmp (categories[j], category_blacklist[i]))
              {
                g_strfreev (categories);
                g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                             "Filters from category \"%s\" are hidden.",
                             category_blacklist[i]);
                return TRUE;
              }
        }

      g_strfreev (categories);
    }

  op_type = gegl_operation_gtype_from_name (name);
  /* Forbid filters which directly write into files. These should
   * not be creatable through Clay UI, but just in case someone
   * builds one such XCF file (maliciously or by mistake/through a
   * bug), let's prevent this filter to overwrite random files on
   * load.
   * Most of these sink ops are in group "output", but double-checking
   * is not a bad thing here.
   */
  if (g_type_is_a (op_type, GEGL_TYPE_OPERATION_SINK))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           "The filter is unsafe.");
      return TRUE;
    }

  return FALSE;
}

/* Builds a GList of the class structures of all subtypes of type.
 */
static GList *
clay_gegl_get_op_subclasses (GType     type,
                             GList    *classes,
                             gboolean  block_clay_ops)
{
  GeglOperationClass *klass;
  GType              *ops;
  const gchar        *categories;
  guint               n_ops;
  gint                i;

  if (! type)
    return classes;

  klass = GEGL_OPERATION_CLASS (g_type_class_ref (type));
  ops = g_type_children (type, &n_ops);

  categories = gegl_operation_class_get_key (klass, "categories");

  if (! clay_gegl_op_blacklisted (klass->name, categories, block_clay_ops, NULL))
    classes = g_list_prepend (classes, klass);

  for (i = 0; i < n_ops; i++)
    classes = clay_gegl_get_op_subclasses (ops[i], classes, block_clay_ops);

  if (ops)
    g_free (ops);

  return classes;
}

static gint
clay_gegl_compare_op_names (GeglOperationClass *a,
                            GeglOperationClass *b)
{
  const gchar *name_a = gegl_operation_class_get_key (a, "title");
  const gchar *name_b = gegl_operation_class_get_key (b, "title");

  if (! name_a) name_a = a->name;
  if (! name_b) name_b = b->name;

  return strcmp (name_a, name_b);
}

static void
clay_gegl_buffer_drawable_unlink (gchar *path)
{
  g_return_if_fail (g_str_has_prefix (path, clay_cache_directory ()));
  /* We ignore failure: the buffer may have never been flushed to disk. */
  g_unlink (path);
  g_free (path);
}
