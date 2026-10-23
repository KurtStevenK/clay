/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextension.c
 * Copyright (C) 2018 Jehan <jehan@girinstud.io>
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

#include <appstream.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay-utils.h"
#include "clayerror.h"
#include "clayextension.h"
#include "clayextension-error.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_PATH,
  PROP_WRITABLE,
  PROP_RUNNING
};

struct _ClayExtensionPrivate
{
  gchar       *path;

  AsMetadata  *metadata;
  AsComponent *component;
  gboolean     writable;
  gboolean     running;

  /* Extension metadata: directories. */
  GList    *brush_paths;
  GList    *dynamics_paths;
  GList    *mypaint_brush_paths;
  GList    *pattern_paths;
  GList    *gradient_paths;
  GList    *palette_paths;
  GList    *tool_preset_paths;
  GList    *splash_paths;
  GList    *theme_paths;

  /* Extension metadata: plug-in entry points. */
  GList    *plug_in_paths;
};

typedef struct
{
  GString  *text;
  gint      level;

  gboolean  numbered_list;
  gint      list_num;
  gboolean  unnumbered_list;

  const gchar *lang;
  GString     *original;
  gint         foreign_level;
} ParseState;


static void         clay_extension_finalize        (GObject        *object);
static void         clay_extension_set_property    (GObject        *object,
                                                    guint           property_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void         clay_extension_get_property    (GObject        *object,
                                                    guint           property_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);

static void         clay_extension_clean           (ClayExtension  *extension);
static gint         clay_extension_file_cmp        (GFile          *a,
                                                    GFile          *b);
static GList      * clay_extension_validate_paths  (ClayExtension  *extension,
                                                    const gchar    *paths,
                                                    gboolean        as_directories,
                                                    GError        **error);


G_DEFINE_TYPE_WITH_PRIVATE (ClayExtension, clay_extension, CLAY_TYPE_OBJECT)

#define parent_class clay_extension_parent_class


static void
clay_extension_class_init (ClayExtensionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_extension_finalize;
  object_class->set_property = clay_extension_set_property;
  object_class->get_property = clay_extension_get_property;

  g_object_class_install_property (object_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        NULL, NULL, NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_WRITABLE,
                                   g_param_spec_boolean ("writable",
                                                         NULL, NULL, FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_RUNNING,
                                   g_param_spec_boolean ("running",
                                                         NULL, NULL, FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_extension_init (ClayExtension *extension)
{
  extension->p = clay_extension_get_instance_private (extension);
}

static void
clay_extension_finalize (GObject *object)
{
  ClayExtension *extension = CLAY_EXTENSION (object);

  clay_extension_clean (extension);

  g_free (extension->p->path);
  g_clear_object (&extension->p->metadata);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_extension_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayExtension *extension = CLAY_EXTENSION (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_set_str (&extension->p->path, g_value_get_string (value));
      clay_object_take_name (CLAY_OBJECT (object),
                             g_path_get_basename (extension->p->path));
      break;
    case PROP_WRITABLE:
      extension->p->writable = g_value_get_boolean (value);
      break;
    case PROP_RUNNING:
      extension->p->running = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_extension_get_property (GObject      *object,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  ClayExtension *extension = CLAY_EXTENSION (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_string (value, extension->p->path);
      break;
    case PROP_WRITABLE:
      g_value_set_boolean (value, extension->p->writable);
      break;
    case PROP_RUNNING:
      g_value_set_boolean (value, extension->p->running);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

ClayExtension *
clay_extension_new (const gchar *dir,
                    gboolean     writable)
{
  g_return_val_if_fail (dir && g_file_test (dir, G_FILE_TEST_IS_DIR), NULL);

  return g_object_new (CLAY_TYPE_EXTENSION,
                       "path",     dir,
                       "writable", writable,
                       NULL);
}

const gchar *
clay_extension_get_name (ClayExtension *extension)
{
  g_return_val_if_fail (extension->p->component != NULL, NULL);

  return as_component_get_name (extension->p->component);
}

const gchar *
clay_extension_get_comment (ClayExtension *extension)
{

  g_return_val_if_fail (extension->p->component != NULL, NULL);

  return as_component_get_summary (extension->p->component);
}

const gchar *
clay_extension_get_description (ClayExtension *extension)
{
  g_return_val_if_fail (extension->p->component != NULL, NULL);

  return as_component_get_description (extension->p->component);
}

GdkPixbuf *
clay_extension_get_screenshot (ClayExtension  *extension,
                               gint            width,
                               gint            height,
                               const gchar   **caption)
{
  GdkPixbuf       *pixbuf       = NULL;
  AsScreenshot    *screenshot   = NULL;
  const GPtrArray *screenshots;

  g_return_val_if_fail (extension->p->component != NULL, NULL);

#if AS_CHECK_VERSION(1, 0, 0)
  screenshots = as_component_get_screenshots_all (extension->p->component);
#else
  screenshots = as_component_get_screenshots (extension->p->component);
#endif

  if (screenshots == NULL || screenshots->len == 0)
    {
      return pixbuf;
    }

  /* Look for the screenshot marked as default */
  for (guint i = 0; i < screenshots->len; i++)
    {
      AsScreenshot *ss = g_ptr_array_index (screenshots, i);
      if (as_screenshot_get_kind (ss) == AS_SCREENSHOT_KIND_DEFAULT)
        {
          screenshot = ss;
          break;
        }
    }

  if (screenshot == NULL)
    {
      g_printerr (_("Invalid AppStream metadata: failed to find default screenshot for extension \"%s\""),
                  as_component_get_id (extension->p->component));
    }

  if (screenshot)
    {
      AsImage          *image   = NULL;
      GFile            *file    = NULL;
      GFileInputStream *istream = NULL;
      GError           *error   = NULL;

#if AS_CHECK_VERSION(1, 0, 0)
      image = as_screenshot_get_image (screenshot, width, height, 1);
#else
      image = as_screenshot_get_image (screenshot, width, height);
#endif

      file = g_file_new_for_uri (as_image_get_url (image));
      istream = g_file_read (file, NULL, &error);
      if (istream != NULL)
        {
          pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (istream), NULL, &error);
        }

      if (error != NULL)
        {
          g_printerr (_("Invalid AppStream metadata: Error loading image: \"%s\""), error->message);
        }

      if (pixbuf)
        {
          g_object_ref (pixbuf);
        }
      else
        {
          GFile            *file    = NULL;
          GFileInputStream *istream = NULL;
          GError           *error   = NULL;

          file = g_file_new_for_uri (as_image_get_url (image));
          istream = g_file_read (file, NULL, &error);
          if (istream)
            {
              pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (istream), NULL, &error);
              g_object_unref (istream);
            }

          if (error)
            {
              g_printerr ("%s: %s\n", G_STRFUNC, error->message);
              g_error_free (error);
            }
          g_object_unref (file);
        }

      if (caption)
        {
          *caption = as_screenshot_get_caption (screenshot);
        }
    }

  return pixbuf;
}

const gchar *
clay_extension_get_path (ClayExtension *extension)
{
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension), NULL);

  return extension->p->path;
}

gchar *
clay_extension_get_markup_description (ClayExtension *extension)
{
  const gchar *description;

  g_return_val_if_fail (CLAY_IS_EXTENSION (extension), NULL);

  description = clay_extension_get_description (extension);

  return clay_appstream_to_pango_markup (description);
}

gboolean
clay_extension_load (ClayExtension  *extension,
                     GError        **error)
{
  AsMetadata     *metadata;
  AsComponent    *component;
#if AS_CHECK_VERSION(1, 0, 0)
  AsComponentBox *components   = NULL;
#else
  GPtrArray      *components   = NULL;
#endif
  GPtrArray      *extends;
  GPtrArray      *relations;
  AsRelease      *release      = NULL;
#if AS_CHECK_VERSION(1, 0, 0)
  AsReleaseList  *rlist        = NULL;
#else
  GPtrArray      *rlist        = NULL;
#endif
  gchar          *appdata_name;
  gchar          *path;
  GFile          *file;
  gboolean        success      = FALSE;
  gboolean        has_require  = FALSE;

  g_clear_object (&extension->p->metadata);
  extension->p->component = NULL;

  /* Search in subdirectory if a file with the same name as
   * directory and ending with ".metainfo.xml" exists.
   */
  appdata_name = g_strdup_printf ("%s.metainfo.xml",
                                  clay_object_get_name (CLAY_OBJECT (extension)));
  path = g_build_filename (extension->p->path, appdata_name, NULL);
  g_free (appdata_name);

  file = g_file_new_for_path (path);

  metadata = as_metadata_new ();
  success = as_metadata_parse_file (metadata, file, AS_FORMAT_KIND_XML, error);

  if (success)
    {
#if AS_CHECK_VERSION(1, 0, 0)
      components = as_metadata_get_components (metadata);
      component  = as_component_box_index (components, 0);
#else
      components = as_metadata_get_components (metadata);
      component  = g_ptr_array_index (components, 0);
#endif
    }

  g_object_unref (file);
  g_free (path);

  if (!success)
    {
      return success;
    }


  if (success && as_component_get_kind (component) != AS_COMPONENT_KIND_ADDON)
    {
      /* Properly setting the type will allow extensions to be
       * distributed appropriately through other means.
       */
      if (error && *error == NULL)
        *error = g_error_new (CLAY_EXTENSION_ERROR,
                              CLAY_EXTENSION_BAD_APPDATA,
                              _("Extension AppData must be of type \"addon\", found \"%s\" instead."),
                              as_component_kind_to_string (as_component_get_kind (component)));
      success = FALSE;
    }

  extends = as_component_get_extends (component);
  if (success &&
      ! g_ptr_array_find_with_equal_func (extends, CLAY_DESKTOP_NAME,
                                          g_str_equal, NULL))
    {
      /* Properly setting the <extends> will allow extensions to be
       * distributed appropriately through other means.
       */
      if (error && *error == NULL)
        *error = g_error_new (CLAY_EXTENSION_ERROR,
                              CLAY_EXTENSION_BAD_APPDATA,
                              _("Extension AppData must extend \"%s\"."),
                              CLAY_DESKTOP_NAME);
      success = FALSE;
    }

  if (success &&
      g_strcmp0 (as_component_get_id (component),
                 clay_object_get_name (extension)) != 0)
    {
      /* Extension IDs will be unique and we want therefore the
       * installation folder to sync in order to avoid path clashes.
       */
      if (error && *error == NULL)
        *error = g_error_new (CLAY_EXTENSION_ERROR,
                              CLAY_EXTENSION_FAILED,
                              _("Extension AppData id (\"%s\") and directory (\"%s\") must be the same."),
                              as_component_get_id (component), clay_object_get_name (extension));
      success = FALSE;
    }

#if AS_CHECK_VERSION(1, 0, 0)
  rlist = as_component_get_releases_plain (component);
  if (rlist != NULL && !as_release_list_is_empty (rlist))
    {
      release = as_release_list_index (rlist, 0);
    }
#else
  rlist = as_component_get_releases (component);
  if (rlist != NULL && rlist->len > 0)
    {
      release = g_ptr_array_index (rlist, 0);
    }
#endif

  if (success && (! release || ! as_release_get_version (release)))
    {
      /* We don't need the detail, just to know that the extension has a
       * release tag with a version. This is very important since it is
       * the only way we can manage updates.
       */
      if (error && *error == NULL)
        *error = g_error_new (CLAY_EXTENSION_ERROR,
                              CLAY_EXTENSION_NO_VERSION,
                              _("Extension AppData must advertise a version in a <release> tag."));
      success = FALSE;
    }

  relations = as_component_get_requires (component);
  if (success && relations != NULL)
    {
      for (guint i = 0; i < relations->len; i++)
        {
          AsRelation *relation = g_ptr_array_index(relations, i);

          if (as_relation_get_item_kind(relation) == AS_RELATION_ITEM_KIND_ID &&
              g_strcmp0(as_relation_get_value_str(relation), CLAY_DESKTOP_NAME) == 0)
            {
              has_require = TRUE;

              if (! as_relation_version_compare (relation, CLAY_VERSION, error))
                {
                  success = FALSE;
                  break;
                }
            }
          else if (error && *error == NULL)
            {
              *error = g_error_new (CLAY_EXTENSION_ERROR,
                                    CLAY_EXTENSION_FAILED,
                                    _("Unsupported <relation> \"%s\" (type %s)."),
                                    as_relation_get_value_str(relation),
                                    as_relation_item_kind_to_string(as_relation_get_item_kind(relation)));
              success = FALSE;
              break;
            }
        }
    }
  if (! has_require)
    {
      success = FALSE;
      if (error && *error == NULL)
        {
          *error = g_error_new (CLAY_EXTENSION_ERROR,
                                CLAY_EXTENSION_FAILED,
                                _("<requires><id>%s</id></requires> for version comparison is mandatory."),
                                CLAY_DESKTOP_NAME);
        }
    }

  if (success)
    {
      extension->p->metadata  = metadata;
      extension->p->component = component;
    }
  else
    {
      g_clear_object (&metadata);
    }

  return success;
}

gboolean
clay_extension_run (ClayExtension  *extension,
                    GError        **error)
{
  GHashTable *metadata;
  gchar      *value;

  g_return_val_if_fail (extension->p->component != NULL, FALSE);
  g_return_val_if_fail (error && *error == NULL, FALSE);

  clay_extension_clean (extension);
  metadata = as_component_get_custom (extension->p->component);

  value = g_hash_table_lookup (metadata, "Clay::brush-path");
  extension->p->brush_paths = clay_extension_validate_paths (extension,
                                                             value, TRUE,
                                                             error);

  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::dynamics-path");
      extension->p->dynamics_paths = clay_extension_validate_paths (extension,
                                                                    value, TRUE,
                                                                    error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::mypaint-brush-path");
      extension->p->mypaint_brush_paths = clay_extension_validate_paths (extension,
                                                                         value, TRUE,
                                                                         error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::pattern-path");
      extension->p->pattern_paths = clay_extension_validate_paths (extension,
                                                                   value, TRUE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::gradient-path");
      extension->p->gradient_paths = clay_extension_validate_paths (extension,
                                                                    value, TRUE,
                                                                    error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::palette-path");
      extension->p->palette_paths = clay_extension_validate_paths (extension,
                                                                   value, TRUE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::tool-preset-path");
      extension->p->tool_preset_paths = clay_extension_validate_paths (extension,
                                                                       value, TRUE,
                                                                       error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::plug-in-path");
      extension->p->plug_in_paths = clay_extension_validate_paths (extension,
                                                                   value, FALSE,
                                                                   error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::splash-path");
      extension->p->splash_paths = clay_extension_validate_paths (extension,
                                                                  value, TRUE,
                                                                  error);
    }
  if (! (*error))
    {
      value = g_hash_table_lookup (metadata, "Clay::theme-path");
      extension->p->theme_paths = clay_extension_validate_paths (extension,
                                                                 value, TRUE,
                                                                 error);
    }

  if (*error)
    clay_extension_clean (extension);

  g_object_set (extension,
                "running", TRUE,
                NULL);

  return (*error == NULL);
}

void
clay_extension_stop (ClayExtension  *extension)
{
  clay_extension_clean (extension);
  g_object_set (extension,
                "running", FALSE,
                NULL);
}

GList *
clay_extension_get_brush_paths (ClayExtension  *extension)
{
  return extension->p->brush_paths;
}

GList *
clay_extension_get_dynamics_paths (ClayExtension *extension)
{
  return extension->p->dynamics_paths;
}

GList *
clay_extension_get_mypaint_brush_paths (ClayExtension *extension)
{
  return extension->p->mypaint_brush_paths;
}

GList *
clay_extension_get_pattern_paths (ClayExtension *extension)
{
  return extension->p->pattern_paths;
}

GList *
clay_extension_get_gradient_paths (ClayExtension *extension)
{
  return extension->p->gradient_paths;
}

GList *
clay_extension_get_palette_paths (ClayExtension *extension)
{
  return extension->p->palette_paths;
}

GList *
clay_extension_get_tool_preset_paths (ClayExtension *extension)
{
  return extension->p->tool_preset_paths;
}

GList *
clay_extension_get_splash_paths (ClayExtension *extension)
{
  return extension->p->splash_paths;
}

GList *
clay_extension_get_theme_paths (ClayExtension *extension)
{
  return extension->p->theme_paths;
}

GList *
clay_extension_get_plug_in_paths (ClayExtension *extension)
{
  return extension->p->plug_in_paths;
}

/**
 * @extension1: a #ClayExtension.
 * @extension2: another #ClayExtension.
 *
 * Compare 2 extensions by their ID.
 *
 * Returns: 0 if the 2 extensions have the same ID (even though they may
 * represent different versions of the same extension).
 */
gint
clay_extension_cmp (ClayExtension *extension1,
                    ClayExtension *extension2)
{
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension1), -1);
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension2), -1);

  return g_strcmp0 (clay_object_get_name (extension1),
                    clay_object_get_name (extension2));
}

/**
 * @extension: a #ClayExtension.
 * @id:        an extension ID (reverse-DNS scheme)
 *
 * Compare the extension ID with @id.
 *
 * Returns: 0 if @extension have @id as appstream ID.
 */
gint
clay_extension_id_cmp (ClayExtension *extension,
                       const gchar   *id)
{
  return g_strcmp0 (clay_object_get_name (extension), id);
}

static void
clay_extension_clean (ClayExtension  *extension)
{
  g_list_free_full (extension->p->brush_paths, g_object_unref);
  extension->p->brush_paths = NULL;
  g_list_free_full (extension->p->dynamics_paths, g_object_unref);
  extension->p->dynamics_paths = NULL;
  g_list_free_full (extension->p->mypaint_brush_paths, g_object_unref);
  extension->p->brush_paths = NULL;
  g_list_free_full (extension->p->pattern_paths, g_object_unref);
  extension->p->pattern_paths = NULL;
  g_list_free_full (extension->p->gradient_paths, g_object_unref);
  extension->p->gradient_paths = NULL;
  g_list_free_full (extension->p->palette_paths, g_object_unref);
  extension->p->palette_paths = NULL;
  g_list_free_full (extension->p->tool_preset_paths, g_object_unref);
  extension->p->tool_preset_paths = NULL;
  g_list_free_full (extension->p->plug_in_paths, g_object_unref);
  extension->p->plug_in_paths = NULL;
  g_list_free_full (extension->p->splash_paths, g_object_unref);
  extension->p->splash_paths = NULL;
  g_list_free_full (extension->p->theme_paths, g_object_unref);
  extension->p->theme_paths = NULL;
}

/**
 * clay_extension_file_cmp:
 * @a:
 * @b:
 *
 * A small g_file_equal() wrapper using GCompareFunc signature.
 */
static gint
clay_extension_file_cmp (GFile *a,
                         GFile *b)
{
  return g_file_equal (a, b) ? 0 : 1;
}

/**
 * clay_extension_validate_paths:
 * @extension: the #ClayExtension
 * @path:      A list of directories separated by ':'.
 * @error:
 *
 * Very similar to clay_parse_search_path() except that we don't use
 * G_SEARCHPATH_SEPARATOR as path separator, because it must not be
 * os-dependent.
 * Also we only allow relative path which are children of the main
 * extension directory (we do not allow extensions to list external
 * folders).
 *
 * Returns: A #GList of #GFile as listed in @path.
 **/
static GList *
clay_extension_validate_paths (ClayExtension  *extension,
                               const gchar    *paths,
                               gboolean        as_directories,
                               GError        **error)
{
  gchar **patharray;
  GList *list      = NULL;
  gint   i;

  g_return_val_if_fail (error && *error == NULL, FALSE);

  if (!paths || ! (*paths))
    return NULL;

  patharray = g_strsplit (paths, ":", 0);

  for (i = 0; patharray[i]; i++)
    {
      /* Note: appstream is supposed to return everything as UTF-8,
       * so we should not have to bother about this. */
      gchar *path;
      GFile *file;
      GFile *ext_dir;

      if (g_path_is_absolute (patharray[i]))
        {
          *error = g_error_new (CLAY_EXTENSION_ERROR,
                                CLAY_EXTENSION_BAD_PATH,
                                _("'%s' is not a relative path."),
                                patharray[i]);
          break;
        }
      path = g_build_filename (extension->p->path, patharray[i], NULL);
      file = g_file_new_for_path (path);
      g_free (path);

      ext_dir = g_file_new_for_path (extension->p->path);
      /* Even with relative paths, it is easy to trick the system
       * and leak out of the extension. So check actual kinship.
       */
      if (! clay_file_is_ancestor (ext_dir, file))
        {
          *error = g_error_new (CLAY_EXTENSION_ERROR,
                                CLAY_EXTENSION_BAD_PATH,
                                _("'%s' is not a child of the extension."),
                                patharray[i]);
          g_object_unref (ext_dir);
          g_object_unref (file);
          break;
        }
      g_object_unref (ext_dir);

      if (as_directories)
        {
          if (g_file_query_file_type (file,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      NULL) != G_FILE_TYPE_DIRECTORY)
            {
              *error = g_error_new (CLAY_EXTENSION_ERROR,
                                    CLAY_EXTENSION_BAD_PATH,
                                    _("'%s' is not a directory."),
                                    patharray[i]);
              g_object_unref (file);
              break;
            }
        }
      else
        {
          if (g_file_query_file_type (file,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      NULL) != G_FILE_TYPE_REGULAR)
            {
              *error = g_error_new (CLAY_EXTENSION_ERROR,
                                    CLAY_EXTENSION_BAD_PATH,
                                    _("'%s' is not a valid file."),
                                    patharray[i]);
              g_object_unref (file);
              break;
            }
        }

      g_return_val_if_fail (path != NULL, NULL);
      if (g_list_find_custom (list, file, (GCompareFunc) clay_extension_file_cmp))
        {
          /* Silently ignore duplicate paths. */
          g_object_unref (file);
          continue;
        }

      list = g_list_prepend (list, file);
    }

  g_strfreev (patharray);
  list = g_list_reverse (list);

  return list;
}
