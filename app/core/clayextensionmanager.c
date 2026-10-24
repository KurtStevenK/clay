/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * clayextensionmanager.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <json-glib/json-glib.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "clay.h"
#include "clayextension.h"
#include "clayextension-error.h"
#include "clayobject.h"
#include "claymarshal.h"
#include "clay-utils.h"

#include "clayextensionmanager.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_BRUSH_PATHS,
  PROP_DYNAMICS_PATHS,
  PROP_MYPAINT_BRUSH_PATHS,
  PROP_PATTERN_PATHS,
  PROP_GRADIENT_PATHS,
  PROP_PALETTE_PATHS,
  PROP_TOOL_PRESET_PATHS,
  PROP_SPLASH_PATHS,
  PROP_THEME_PATHS,
  PROP_PLUG_IN_EXTENSIONS,
};

enum
{
  EXTENSION_INSTALLED,
  EXTENSION_REMOVED,
  STORE_UPDATED,
  LAST_SIGNAL
};

struct _ClayExtensionManagerPrivate
{
  Clay       *clay;

  /* Installed system (read-only) extensions. */
  GList      *sys_extensions;
  /* Self-installed (read-write) extensions. */
  GList      *extensions;
  /* Uninstalled extensions (cached to allow undo). */
  GList      *uninstalled_extensions;

  /* Running extensions */
  GHashTable *running_extensions;

  /* App Store catalog */
  GList      *store_catalog;
  gchar      *store_url;

  /* Metadata properties */
  GList      *brush_paths;
  GList      *dynamics_paths;
  GList      *mypaint_brush_paths;
  GList      *pattern_paths;
  GList      *gradient_paths;
  GList      *palette_paths;
  GList      *tool_preset_paths;
  GList      *splash_paths;
  GList      *theme_paths;
  GList      *plug_in_extensions;
};

static void     clay_extension_manager_config_iface_init   (ClayConfigInterface  *iface);
static gboolean clay_extension_manager_serialize           (ClayConfig           *config,
                                                            ClayConfigWriter     *writer,
                                                            gpointer              data);
static gboolean clay_extension_manager_deserialize         (ClayConfig           *config,
                                                            GScanner             *scanner,
                                                            gint                  nest_level,
                                                            gpointer              data);

static void     clay_extension_manager_finalize            (GObject              *object);
static void     clay_extension_manager_set_property        (GObject              *object,
                                                            guint                 property_id,
                                                            const GValue         *value,
                                                            GParamSpec           *pspec);
static void     clay_extension_manager_get_property        (GObject              *object,
                                                            guint                 property_id,
                                                            GValue               *value,
                                                            GParamSpec           *pspec);

static void     clay_extension_manager_serialize_extension (ClayExtensionManager *manager,
                                                            ClayExtension        *extension,
                                                            ClayConfigWriter     *writer);

static void     clay_extension_manager_refresh             (ClayExtensionManager *manager);
static void     clay_extension_manager_search_directory    (ClayExtensionManager *manager,
                                                            GFile                *directory,
                                                            gboolean              system_dir);

static void     clay_extension_manager_extension_running   (ClayExtension        *extension,
                                                            GParamSpec           *pspec,
                                                            ClayExtensionManager *manager);

static void     clay_extension_manager_load_store_file     (ClayExtensionManager *manager,
                                                            GFile                *file);
static void     clay_extension_manager_parse_store_json    (ClayExtensionManager *manager,
                                                            const gchar          *json,
                                                            gsize                 length);
static void     clay_extension_manager_fetch_store_ready   (GObject              *source,
                                                            GAsyncResult         *result,
                                                            gpointer              user_data);
static gboolean clay_extension_manager_copy_directory      (GFile                *src,
                                                            GFile                *dest,
                                                            GError              **error);


G_DEFINE_TYPE_WITH_CODE (ClayExtensionManager, clay_extension_manager,
                         CLAY_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayExtensionManager)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_extension_manager_config_iface_init))

#define parent_class clay_extension_manager_parent_class

static guint signals[LAST_SIGNAL] = { 0, };

static void
clay_extension_manager_class_init (ClayExtensionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_extension_manager_finalize;
  object_class->set_property = clay_extension_manager_set_property;
  object_class->get_property = clay_extension_manager_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BRUSH_PATHS,
                                   g_param_spec_pointer ("brush-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DYNAMICS_PATHS,
                                   g_param_spec_pointer ("dynamics-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MYPAINT_BRUSH_PATHS,
                                   g_param_spec_pointer ("mypaint-brush-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PATTERN_PATHS,
                                   g_param_spec_pointer ("pattern-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_GRADIENT_PATHS,
                                   g_param_spec_pointer ("gradient-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PALETTE_PATHS,
                                   g_param_spec_pointer ("palette-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_TOOL_PRESET_PATHS,
                                   g_param_spec_pointer ("tool-preset-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SPLASH_PATHS,
                                   g_param_spec_pointer ("splash-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_THEME_PATHS,
                                   g_param_spec_pointer ("theme-paths",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLUG_IN_EXTENSIONS,
                                   g_param_spec_pointer ("plug-in-extensions",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));

  signals[EXTENSION_INSTALLED] =
    g_signal_new ("extension-installed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionManagerClass, extension_installed),
                  NULL, NULL,
                  clay_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_EXTENSION,
                  G_TYPE_BOOLEAN);
  signals[EXTENSION_REMOVED] =
    g_signal_new ("extension-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionManagerClass, extension_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
  signals[STORE_UPDATED] =
    g_signal_new ("store-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionManagerClass, store_updated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
clay_extension_manager_init (ClayExtensionManager *manager)
{
  manager->p = clay_extension_manager_get_instance_private (manager);
  manager->p->extensions     = NULL;
  manager->p->sys_extensions = NULL;
}

static void
clay_extension_manager_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_extension_manager_serialize;
  iface->deserialize = clay_extension_manager_deserialize;
}

static gboolean
clay_extension_manager_serialize (ClayConfig       *config,
                                  ClayConfigWriter *writer,
                                  gpointer          data G_GNUC_UNUSED)
{
  ClayExtensionManager *manager = CLAY_EXTENSION_MANAGER (config);
  GList                *iter;

  /* TODO: another information we will want to add will be the last
   * extension list update to allow ourselves to regularly check for
   * updates while not doing it too often.
   */
  for (iter = manager->p->extensions; iter; iter = iter->next)
    clay_extension_manager_serialize_extension (manager, iter->data, writer);
  for (iter = manager->p->sys_extensions; iter; iter = iter->next)
    {
      if (g_list_find_custom (manager->p->extensions, iter->data,
                              (GCompareFunc) clay_extension_cmp))
        continue;
      clay_extension_manager_serialize_extension (manager, iter->data, writer);
    }

  return TRUE;
}

static gboolean
clay_extension_manager_deserialize (ClayConfig *config,
                                    GScanner   *scanner,
                                    gint        nest_level,
                                    gpointer    data)
{
  ClayExtensionManager  *manager  = CLAY_EXTENSION_MANAGER (config);
  GList                **processed = (GList**) data;
  GTokenType             token;

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_IDENTIFIER;
          break;

        case G_TOKEN_IDENTIFIER:
          {
            gchar    *name      = NULL;
            gboolean  is_active = FALSE;
            GType     type;

            type = g_type_from_name (scanner->value.v_identifier);

            if (! type)
              {
                g_scanner_error (scanner,
                                 "unable to determine type of '%s'",
                                 scanner->value.v_identifier);
                return FALSE;
              }

            if (! g_type_is_a (type, CLAY_TYPE_EXTENSION))
              {
                g_scanner_error (scanner,
                                 "'%s' is not a subclass of '%s'",
                                 scanner->value.v_identifier,
                                 g_type_name (CLAY_TYPE_EXTENSION));
                return FALSE;
              }

            if (! clay_scanner_parse_string (scanner, &name))
              {
                g_scanner_error (scanner,
                                 "Expected extension id not found after ClayExtension.");
                return FALSE;
              }

            if (! name || strlen (name) == 0)
              {
                g_scanner_error (scanner,
                                 "NULL or empty strings are not valid extension IDs.");
                if (name)
                  g_free (name);
                return FALSE;
              }

            if (! clay_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
              {
                g_scanner_error (scanner,
                                 "Left paren expected after extension ID.");
                g_free (name);
                return FALSE;
              }
            if (! clay_scanner_parse_identifier (scanner, "active"))
              {
                g_scanner_error (scanner,
                                 "Expected identifier \"active\" after extension ID.");
                g_free (name);
                return FALSE;
              }
            if (clay_scanner_parse_boolean (scanner, &is_active))
              {
                if (is_active)
                  {
                    GList *list;

                    list = g_list_find_custom (manager->p->extensions, name,
                                               (GCompareFunc) clay_extension_id_cmp);
                    if (! list)
                      list = g_list_find_custom (manager->p->sys_extensions, name,
                                                 (GCompareFunc) clay_extension_id_cmp);
                    if (list)
                      {
                        GError *error = NULL;

                        if (clay_extension_run (list->data, &error))
                          {
                            g_hash_table_insert (manager->p->running_extensions,
                                                 (gpointer) clay_object_get_name (list->data),
                                                 list->data);
                          }
                        else
                          {
                            g_printerr ("Extension '%s' failed to run: %s\n",
                                        clay_object_get_name (list->data),
                                        error->message);
                            g_error_free (error);
                          }
                      }
                  }

                /* Save the list of processed extension IDs, whether
                 * active or not.
                 */
                *processed = g_list_prepend (*processed, name);
              }
            else
              {
                g_scanner_error (scanner,
                                 "Expected boolean after \"active\" identifier.");
                g_free (name);
                return FALSE;
              }

            if (! clay_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
              {
                g_scanner_error (scanner,
                                 "Right paren expected after \"active\" identifier.");
                return FALSE;
              }
          }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  return clay_config_deserialize_return (scanner, token, nest_level);
}

static void
clay_extension_manager_finalize (GObject *object)
{
  GList *iter;

  ClayExtensionManager *manager = CLAY_EXTENSION_MANAGER (object);

  g_list_free_full (manager->p->sys_extensions, g_object_unref);
  g_list_free_full (manager->p->extensions, g_object_unref);
  g_list_free_full (manager->p->store_catalog, (GDestroyNotify) clay_store_entry_free);
  g_free (manager->p->store_url);
  g_hash_table_unref (manager->p->running_extensions);

  for (iter = manager->p->uninstalled_extensions; iter; iter = iter->next)
    {
      /* Recursively delete folders of uninstalled extensions. */
      GError *error = NULL;
      GFile  *file;

      file = g_file_new_for_path (clay_extension_get_path (iter->data));
      if (! clay_file_delete_recursive (file, &error))
        g_warning ("%s: %s\n", G_STRFUNC, error->message);
      g_object_unref (file);
    }
  g_list_free_full (manager->p->uninstalled_extensions, g_object_unref);

  g_list_free_full (manager->p->brush_paths, g_object_unref);
  manager->p->brush_paths = NULL;
  g_list_free_full (manager->p->dynamics_paths, g_object_unref);
  manager->p->dynamics_paths = NULL;
  g_list_free_full (manager->p->mypaint_brush_paths, g_object_unref);
  manager->p->brush_paths = NULL;
  g_list_free_full (manager->p->pattern_paths, g_object_unref);
  manager->p->pattern_paths = NULL;
  g_list_free_full (manager->p->gradient_paths, g_object_unref);
  manager->p->gradient_paths = NULL;
  g_list_free_full (manager->p->palette_paths, g_object_unref);
  manager->p->palette_paths = NULL;
  g_list_free_full (manager->p->tool_preset_paths, g_object_unref);
  manager->p->tool_preset_paths = NULL;
  g_list_free_full (manager->p->plug_in_extensions, g_object_unref);
  manager->p->plug_in_extensions = NULL;
  g_list_free_full (manager->p->splash_paths, g_object_unref);
  manager->p->splash_paths = NULL;
  g_list_free_full (manager->p->theme_paths, g_object_unref);
  manager->p->theme_paths = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_extension_manager_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayExtensionManager *manager = CLAY_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_CLAY:
      manager->p->clay = g_value_get_object (value);
      break;
    case PROP_BRUSH_PATHS:
      g_list_free_full (manager->p->brush_paths, g_object_unref);
      manager->p->brush_paths = g_value_get_pointer (value);
      break;
    case PROP_DYNAMICS_PATHS:
      g_list_free_full (manager->p->dynamics_paths, g_object_unref);
      manager->p->dynamics_paths = g_value_get_pointer (value);
      break;
    case PROP_MYPAINT_BRUSH_PATHS:
      g_list_free_full (manager->p->mypaint_brush_paths, g_object_unref);
      manager->p->mypaint_brush_paths = g_value_get_pointer (value);
      break;
    case PROP_PATTERN_PATHS:
      g_list_free_full (manager->p->pattern_paths, g_object_unref);
      manager->p->pattern_paths = g_value_get_pointer (value);
      break;
    case PROP_GRADIENT_PATHS:
      g_list_free_full (manager->p->gradient_paths, g_object_unref);
      manager->p->gradient_paths = g_value_get_pointer (value);
      break;
    case PROP_PALETTE_PATHS:
      g_list_free_full (manager->p->palette_paths, g_object_unref);
      manager->p->palette_paths = g_value_get_pointer (value);
      break;
    case PROP_TOOL_PRESET_PATHS:
      g_list_free_full (manager->p->tool_preset_paths, g_object_unref);
      manager->p->tool_preset_paths = g_value_get_pointer (value);
      break;
    case PROP_SPLASH_PATHS:
      g_list_free_full (manager->p->splash_paths, g_object_unref);
      manager->p->splash_paths = g_value_get_pointer (value);
      break;
    case PROP_THEME_PATHS:
      g_list_free_full (manager->p->theme_paths, g_object_unref);
      manager->p->theme_paths = g_value_get_pointer (value);
      break;
    case PROP_PLUG_IN_EXTENSIONS:
      g_list_free_full (manager->p->plug_in_extensions, g_object_unref);
      manager->p->plug_in_extensions = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_extension_manager_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  ClayExtensionManager *manager = CLAY_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, manager->p->clay);
      break;
    case PROP_BRUSH_PATHS:
      g_value_set_pointer (value, manager->p->brush_paths);
      break;
    case PROP_DYNAMICS_PATHS:
      g_value_set_pointer (value, manager->p->dynamics_paths);
      break;
    case PROP_MYPAINT_BRUSH_PATHS:
      g_value_set_pointer (value, manager->p->mypaint_brush_paths);
      break;
    case PROP_PATTERN_PATHS:
      g_value_set_pointer (value, manager->p->pattern_paths);
      break;
    case PROP_GRADIENT_PATHS:
      g_value_set_pointer (value, manager->p->gradient_paths);
      break;
    case PROP_PALETTE_PATHS:
      g_value_set_pointer (value, manager->p->palette_paths);
      break;
    case PROP_TOOL_PRESET_PATHS:
      g_value_set_pointer (value, manager->p->tool_preset_paths);
      break;
    case PROP_SPLASH_PATHS:
      g_value_set_pointer (value, manager->p->splash_paths);
      break;
    case PROP_THEME_PATHS:
      g_value_set_pointer (value, manager->p->theme_paths);
      break;
    case PROP_PLUG_IN_EXTENSIONS:
      g_value_set_pointer (value, manager->p->plug_in_extensions);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public functions. */

ClayExtensionManager *
clay_extension_manager_new (Clay *clay)
{
  ClayExtensionManager *manager;

  manager = g_object_new (CLAY_TYPE_EXTENSION_MANAGER,
                          "clay", clay,
                          NULL);

  return manager;
}

void
clay_extension_manager_initialize (ClayExtensionManager *manager)
{
  GFile  *file;
  GError *error = NULL;
  gchar  *path_str;
  GList  *path;
  GList  *list;
  GList  *processed_ids;

  g_return_if_fail (CLAY_IS_EXTENSION_MANAGER (manager));

  /* List user-installed extensions. */
  path_str = clay_config_build_writable_path ("extensions");
  path = clay_config_path_expand_to_files (path_str, NULL);
  g_free (path_str);
  for (list = path; list; list = g_list_next (list))
    clay_extension_manager_search_directory (manager, list->data, FALSE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* List system extensions. */
  path_str = clay_config_build_system_path ("extensions");
  path = clay_config_path_expand_to_files (path_str, NULL);
  g_free (path_str);
  for (list = path; list; list = g_list_next (list))
    clay_extension_manager_search_directory (manager, list->data, TRUE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* Actually load the extensions. */
  if (manager->p->running_extensions)
    g_hash_table_unref (manager->p->running_extensions);
  manager->p->running_extensions = g_hash_table_new (g_str_hash, g_str_equal);

  file = clay_directory_file ("extensionrc", NULL);

  processed_ids = NULL;
  if (g_file_query_exists (file, NULL))
    {
      if (manager->p->clay->be_verbose)
        g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

      clay_config_deserialize_file (CLAY_CONFIG (manager),
                                    file, &processed_ids, &error);
      if (error)
        {
          g_printerr ("Failed to parse '%s': %s\n",
                      clay_file_get_utf8_name (file),
                      error->message);
          g_error_free (error);
        }
    }
  g_object_unref (file);

  /* System extensions are on by default and user ones are off by
   * default */
  for (list = manager->p->extensions; list; list = list->next)
    {
      /* Directly flag user-installed extensions as processed. We do not
       * load them unless they were explicitly loaded (i.e. they must be
       * set in extensionrc.
       */
      if (! g_list_find_custom (processed_ids,
                                clay_object_get_name (list->data),
                                (GCompareFunc) g_strcmp0))
        processed_ids = g_list_prepend (processed_ids,
                                        g_strdup (clay_object_get_name (list->data)));
      g_signal_connect (list->data, "notify::running",
                        G_CALLBACK (clay_extension_manager_extension_running),
                        manager);
    }
  for (list = manager->p->sys_extensions; list; list = g_list_next (list))
    {
      /* Unlike user-installed extensions, system extensions are loaded
       * by default if they were not set in the extensionrc (so that new
       * extensions installed with Clay updates get loaded) and if they
       * were not overridden by a user-installed extension (same ID).
       */
      if (! g_list_find_custom (processed_ids,
                                clay_object_get_name (list->data),
                                (GCompareFunc) g_strcmp0))

        {
          error = NULL;
          if (clay_extension_run (list->data, &error))
            {
              g_hash_table_insert (manager->p->running_extensions,
                                   (gpointer) clay_object_get_name (list->data),
                                   list->data);
            }
          else
            {
              g_printerr ("Extension '%s' failed to run: %s\n",
                          clay_object_get_name (list->data),
                          error->message);
              g_error_free (error);
            }
        }
      g_signal_connect (list->data, "notify::running",
                        G_CALLBACK (clay_extension_manager_extension_running),
                        manager);
    }

  clay_extension_manager_refresh (manager);
  g_list_free_full (processed_ids, g_free);

  clay_extension_manager_refresh_store (manager);
}

void
clay_extension_manager_exit (ClayExtensionManager *manager)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_EXTENSION_MANAGER (manager));

  file = clay_directory_file ("extensionrc", NULL);

  if (manager->p->clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (manager),
                                       file,
                                       "Clay extensionrc",
                                       "end of extensionrc",
                                       NULL,
                                       &error))
    {
      clay_message_literal (manager->p->clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

const GList *
clay_extension_manager_get_system_extensions (ClayExtensionManager *manager)
{
  return manager->p->sys_extensions;
}

const GList *
clay_extension_manager_get_user_extensions (ClayExtensionManager *manager)
{
  return manager->p->extensions;
}

/**
 * clay_extension_manager_is_running:
 * @extension:
 *
 * Returns: %TRUE if @extension is ON.
 */
gboolean
clay_extension_manager_is_running (ClayExtensionManager *manager,
                                   ClayExtension        *extension)
{
  ClayExtension *ext;

  ext = g_hash_table_lookup (manager->p->running_extensions,
                             clay_object_get_name (extension));

  return (ext && ext == extension);
}

/**
 * clay_extension_manager_can_run:
 * @extension:
 *
 * Returns: %TRUE is @extension can be run.
 */
gboolean
clay_extension_manager_can_run (ClayExtensionManager *manager,
                                ClayExtension        *extension)
{
  /* System extension overridden by another extension. */
  if (g_list_find (manager->p->sys_extensions, extension) &&
      g_list_find_custom (manager->p->extensions, extension,
                          (GCompareFunc) clay_extension_cmp))
    return FALSE;

  /* TODO: should return FALSE if required Clay version or other
   * requirements are not filled as well.
   */

  return TRUE;
}

/**
 * clay_extension_manager_is_removed:
 * @manager:
 * @extension:
 *
 * Returns: %TRUE is @extension was installed and has been removed
 * (hence clay_extension_manager_undo_remove() can be used on it).
 */
gboolean
clay_extension_manager_is_removed (ClayExtensionManager *manager,
                                   ClayExtension        *extension)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension), FALSE);

  iter = manager->p->uninstalled_extensions;
  for (; iter; iter = iter->next)
    if (clay_extension_cmp (iter->data, extension) == 0)
      break;

  return (iter != NULL);
}

/**
 * clay_extension_manager_install:
 * @manager:
 * @extension:
 * @error:
 *
 * Install @extension. This merely adds the extension in the known
 * extension list to make the manager aware of it at runtime, and to
 * emit a signal for GUI update.
 */
gboolean
clay_extension_manager_install (ClayExtensionManager *manager,
                                ClayExtension        *extension,
                                GError              **error)
{
  gboolean success = FALSE;

  if ((success = clay_extension_load (extension, error)))
    {
      manager->p->extensions = g_list_prepend (manager->p->extensions,
                                               extension);
      g_signal_connect (extension, "notify::running",
                        G_CALLBACK (clay_extension_manager_extension_running),
                        manager);
      g_signal_emit (manager, signals[EXTENSION_INSTALLED], 0, extension, FALSE);
    }

  return success;
}

/**
 * clay_extension_manager_remove:
 * @manager:
 * @extension:
 * @error:
 *
 * Uninstall @extension. Technically this only move the object to a
 * temporary list. The extension folder will be really deleted when Clay
 * will stop.
 * This allows to undo a deletion for as long as the session runs.
 */
gboolean
clay_extension_manager_remove (ClayExtensionManager  *manager,
                               ClayExtension         *extension,
                               GError               **error)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension), FALSE);

  iter = (GList *) clay_extension_manager_get_system_extensions (manager);
  for (; iter; iter = iter->next)
    if (iter->data == extension)
      {
        /* System extensions cannot be uninstalled. */
        if (error)
          *error = g_error_new (CLAY_EXTENSION_ERROR,
                                CLAY_EXTENSION_FAILED,
                                _("System extensions cannot be uninstalled."));
        return FALSE;
      }

  iter = (GList *) clay_extension_manager_get_user_extensions (manager);
  for (; iter; iter = iter->next)
    if (clay_extension_cmp (iter->data, extension) == 0)
      break;

  /* The extension has to be in the extension list. */
  g_return_val_if_fail (iter != NULL, FALSE);

  clay_extension_stop (extension);

  manager->p->extensions = g_list_remove_link (manager->p->extensions,
                                               iter);
  manager->p->uninstalled_extensions = g_list_concat (manager->p->uninstalled_extensions,
                                                      iter);
  g_signal_emit (manager, signals[EXTENSION_REMOVED], 0,
                 clay_object_get_name (extension));

  return TRUE;
}

ClayExtension *
clay_extension_manager_lookup (ClayExtensionManager *manager,
                               const gchar          *id)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  for (iter = manager->p->extensions; iter; iter = iter->next)
    {
      if (clay_extension_id_cmp (iter->data, id) == 0)
        return iter->data;
    }

  for (iter = manager->p->sys_extensions; iter; iter = iter->next)
    {
      if (clay_extension_id_cmp (iter->data, id) == 0)
        return iter->data;
    }

  return NULL;
}

void
clay_store_entry_free (ClayStoreEntry *entry)
{
  if (! entry)
    return;

  g_free (entry->id);
  g_free (entry->name);
  g_free (entry->name_de);
  g_free (entry->summary);
  g_free (entry->summary_de);
  g_free (entry->description);
  g_free (entry->category);
  g_free (entry->tags);
  g_free (entry->version);
  g_free (entry->author);
  g_free (entry->icon_name);
  g_free (entry->download_url);
  g_free (entry->local_gex_path);
  g_free (entry->menu_hint);
  g_free (entry);
}

const GList *
clay_extension_manager_get_store_catalog (ClayExtensionManager *manager)
{
  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), NULL);

  return manager->p->store_catalog;
}

ClayStoreEntry *
clay_extension_manager_find_store_entry (ClayExtensionManager *manager,
                                         const gchar          *id)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  for (iter = manager->p->store_catalog; iter; iter = iter->next)
    {
      ClayStoreEntry *entry = iter->data;

      if (g_strcmp0 (entry->id, id) == 0)
        return entry;
    }

  return NULL;
}

static gboolean
clay_store_haystack_matches (ClayStoreEntry *entry,
                             gchar         **tokens)
{
  gchar    *haystack;
  gchar    *folded;
  gboolean  match = TRUE;
  gint      i;

  haystack = g_strdup_printf ("%s %s %s %s %s %s %s %s",
                              entry->id ? entry->id : "",
                              entry->name ? entry->name : "",
                              entry->name_de ? entry->name_de : "",
                              entry->summary ? entry->summary : "",
                              entry->summary_de ? entry->summary_de : "",
                              entry->description ? entry->description : "",
                              entry->category ? entry->category : "",
                              entry->tags ? entry->tags : "");
  folded = g_utf8_casefold (haystack, -1);
  g_free (haystack);

  for (i = 0; tokens[i]; i++)
    {
      gchar *needle = g_utf8_casefold (tokens[i], -1);

      if (needle[0] && strstr (folded, needle) == NULL)
        match = FALSE;

      g_free (needle);
      if (! match)
        break;
    }

  g_free (folded);
  return match;
}

GList *
clay_extension_manager_search_store (ClayExtensionManager *manager,
                                     const gchar          *query,
                                     const gchar          *category)
{
  GList *matches = NULL;
  GList *iter;
  gchar **tokens = NULL;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), NULL);

  if (query && *query)
    tokens = g_strsplit (query, " ", -1);

  for (iter = manager->p->store_catalog; iter; iter = iter->next)
    {
      ClayStoreEntry *entry = iter->data;

      if (category && *category && g_strcmp0 (category, "all") != 0)
        {
          if (g_strcmp0 (category, "featured") == 0)
            {
              if (! entry->featured)
                continue;
            }
          else if (g_strcmp0 (entry->category, category) != 0)
            {
              continue;
            }
        }

      if (tokens && ! clay_store_haystack_matches (entry, tokens))
        continue;

      matches = g_list_prepend (matches, entry);
    }

  g_strfreev (tokens);
  return g_list_reverse (matches);
}

gboolean
clay_extension_manager_install_from_directory (ClayExtensionManager *manager,
                                               const gchar          *path,
                                               GError              **error)
{
  ClayExtension *extension;
  gchar         *basename;
  gchar         *writable_spec;
  gchar         *writable;
  gchar         *dest_path;
  GFile         *src;
  GFile         *dest;
  gboolean       success;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  basename = g_path_get_basename (path);
  extension = clay_extension_manager_lookup (manager, basename);
  if (extension)
    {
      GError *local_error = NULL;

      g_free (basename);
      if (clay_extension_manager_can_run (manager, extension) &&
          ! clay_extension_manager_is_running (manager, extension))
        {
          gboolean ok = clay_extension_run (extension, error ? error : &local_error);
          g_clear_error (&local_error);
          return ok;
        }

      return TRUE;
    }

  writable_spec = clay_config_build_writable_path ("extensions");
  writable = clay_config_path_expand (writable_spec, TRUE, NULL);
  g_free (writable_spec);
  dest_path = g_build_filename (writable, basename, NULL);
  g_free (basename);
  src = g_file_new_for_path (path);
  dest = g_file_new_for_path (dest_path);

  if (g_file_query_exists (dest, NULL))
    {
      extension = clay_extension_new (dest_path, TRUE);
      success = clay_extension_manager_install (manager, extension, error);
      if (success)
        {
          GError *local_error = NULL;
          clay_extension_run (extension, error ? error : &local_error);
          g_clear_error (&local_error);
        }
    }
  else
    {
      success = clay_extension_manager_copy_directory (src, dest, error);
      if (success)
        {
          GError *local_error = NULL;

          extension = clay_extension_new (dest_path, TRUE);
          success = clay_extension_manager_install (manager, extension, error);
          if (success)
            {
              clay_extension_run (extension, error ? error : &local_error);
              g_clear_error (&local_error);
            }
          else
            g_object_unref (extension);
        }
    }

  g_object_unref (src);
  g_object_unref (dest);
  g_free (writable);
  g_free (dest_path);
  return success;
}

gboolean
clay_extension_manager_install_from_store (ClayExtensionManager *manager,
                                           const gchar          *entry_id,
                                           GError              **error)
{
  ClayStoreEntry *entry;
  ClayExtension  *extension;
  GFile          *local_dir;
  gchar          *path;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (entry_id != NULL, FALSE);

  extension = clay_extension_manager_lookup (manager, entry_id);
  if (extension)
    {
      GError *local_error = NULL;

      if (clay_extension_manager_can_run (manager, extension) &&
          ! clay_extension_manager_is_running (manager, extension))
        {
          gboolean ok = clay_extension_run (extension, error ? error : &local_error);
          g_clear_error (&local_error);
          return ok;
        }

      if (clay_extension_manager_is_running (manager, extension))
        return TRUE;

      /* Listed in the store but switched off: turn it on instead of
       * pretending the install already succeeded.
       */
      {
        gboolean ok = clay_extension_run (extension, error ? error : &local_error);
        g_clear_error (&local_error);
        return ok;
      }
    }

  entry = clay_extension_manager_find_store_entry (manager, entry_id);
  if (! entry)
    {
      g_set_error (error, CLAY_EXTENSION_ERROR, CLAY_EXTENSION_FAILED,
                   _("Unknown store item '%s'."), entry_id);
      return FALSE;
    }

  local_dir = clay_data_directory_file ("extensions", entry_id, NULL);
  path = g_file_get_path (local_dir);
  if (path && g_file_test (path, G_FILE_TEST_IS_DIR))
    {
      gboolean ok = clay_extension_manager_install_from_directory (manager, path, error);
      g_free (path);
      g_object_unref (local_dir);
      return ok;
    }
  g_free (path);
  g_object_unref (local_dir);

  if (entry->local_gex_path && g_file_test (entry->local_gex_path, G_FILE_TEST_IS_DIR))
    return clay_extension_manager_install_from_directory (manager, entry->local_gex_path, error);

  g_set_error (error, CLAY_EXTENSION_ERROR, CLAY_EXTENSION_FAILED,
               _("Store item '%s' is not available locally. Download it from the Clay App Store."),
               entry_id);
  return FALSE;
}

void
clay_extension_manager_refresh_store (ClayExtensionManager *manager)
{
  GFile *file;

  g_return_if_fail (CLAY_IS_EXTENSION_MANAGER (manager));

  file = clay_data_directory_file ("extensions", "store-catalog.json", NULL);
  if (g_file_query_exists (file, NULL))
    clay_extension_manager_load_store_file (manager, file);
  g_object_unref (file);

  if (manager->p->store_url && *manager->p->store_url)
    {
      GFile *remote = g_file_new_for_uri (manager->p->store_url);

      g_file_load_contents_async (remote, NULL,
                                  clay_extension_manager_fetch_store_ready,
                                  g_object_ref (manager));
      g_object_unref (remote);
    }

  g_signal_emit (manager, signals[STORE_UPDATED], 0);
}

gboolean
clay_extension_manager_undo_remove (ClayExtensionManager *manager,
                                    ClayExtension        *extension,
                                    GError              **error)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (CLAY_IS_EXTENSION (extension), FALSE);

  iter = manager->p->uninstalled_extensions;
  for (; iter; iter = iter->next)
    if (clay_extension_cmp (iter->data, extension) == 0)
      break;

  /* The extension has to be in the uninstalled extension list. */
  g_return_val_if_fail (iter != NULL, FALSE);

  manager->p->uninstalled_extensions = g_list_remove (manager->p->uninstalled_extensions,
                                                      extension);
  clay_extension_manager_install (manager, extension, error);

  return TRUE;
}

/* Private functions. */

static void
clay_extension_manager_serialize_extension (ClayExtensionManager *manager,
                                            ClayExtension        *extension,
                                            ClayConfigWriter     *writer)
{
  const gchar *name = clay_object_get_name (extension);

  g_return_if_fail (name != NULL);

  clay_config_writer_open (writer, g_type_name (G_TYPE_FROM_INSTANCE (extension)));
  clay_config_writer_string (writer, name);

  /* The extensionrc does not need to save any information about an
   * extension. This is all saved in the metadata. The only thing we
   * care to store is the state of extensions, i.e. which one is loaded
   * or not (since you can install an extension but leave it unloaded),
   * named by its unique ID.
   * As a consequence, ClayExtension does not need to implement
   * ClayConfigInterface.
   */
  clay_config_writer_open (writer, "active");
  if (g_hash_table_contains (manager->p->running_extensions, name))
    clay_config_writer_identifier (writer, "yes");
  else
    clay_config_writer_identifier (writer, "no");
  clay_config_writer_close (writer);

  clay_config_writer_close (writer);
}

static void
clay_extension_manager_refresh (ClayExtensionManager *manager)
{
  GHashTableIter iter;
  gpointer       key;
  gpointer       value;
  GList         *brush_paths         = NULL;
  GList         *dynamics_paths      = NULL;
  GList         *mypaint_brush_paths = NULL;
  GList         *pattern_paths       = NULL;
  GList         *gradient_paths      = NULL;
  GList         *palette_paths       = NULL;
  GList         *tool_preset_paths   = NULL;
  GList         *splash_paths        = NULL;
  GList         *theme_paths         = NULL;
  GList         *plug_in_extensions  = NULL;

  g_hash_table_iter_init (&iter, manager->p->running_extensions);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ClayExtension *extension = value;
      GList         *new_paths;

      new_paths = g_list_copy_deep (clay_extension_get_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      brush_paths = g_list_concat (brush_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_dynamics_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      dynamics_paths = g_list_concat (dynamics_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_mypaint_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      mypaint_brush_paths = g_list_concat (mypaint_brush_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_pattern_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      pattern_paths = g_list_concat (pattern_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_gradient_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      gradient_paths = g_list_concat (gradient_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_palette_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      palette_paths = g_list_concat (palette_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_tool_preset_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      tool_preset_paths = g_list_concat (tool_preset_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_splash_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      splash_paths = g_list_concat (splash_paths, new_paths);

      new_paths = g_list_copy_deep (clay_extension_get_theme_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      theme_paths = g_list_concat (theme_paths, new_paths);

      if (clay_extension_get_plug_in_paths (extension))
        plug_in_extensions = g_list_prepend (plug_in_extensions, g_object_ref (extension));
    }

  g_object_set (manager,
                "brush-paths",         brush_paths,
                "dynamics-paths",      dynamics_paths,
                "mypaint-brush-paths", brush_paths,
                "pattern-paths",       pattern_paths,
                "gradient-paths",      gradient_paths,
                "palette-paths",       palette_paths,
                "tool-preset-paths",   tool_preset_paths,
                "plug-in-extensions",  plug_in_extensions,
                "splash-paths",        splash_paths,
                "theme-paths",         theme_paths,
                NULL);
}

static void
clay_extension_manager_search_directory (ClayExtensionManager *manager,
                                         GFile                *directory,
                                         gboolean              system_dir)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFile *subdir;

          if (g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            {
              g_object_unref (info);
              continue;
            }

          subdir = g_file_enumerator_get_child (enumerator, info);

          if (g_file_query_file_type (subdir,
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL) == G_FILE_TYPE_DIRECTORY)
            {
              ClayExtension *extension;
              GError        *error = NULL;

              extension = clay_extension_new (g_file_peek_path (subdir),
                                              ! system_dir);

              if (clay_extension_load (extension, &error))
                {
                  if (system_dir)
                    manager->p->sys_extensions = g_list_prepend (manager->p->sys_extensions,
                                                                 extension);
                  else
                    manager->p->extensions = g_list_prepend (manager->p->extensions,
                                                             extension);
                }
              else
                {
                  g_object_unref (extension);
                  if (error)
                    {
                      g_printerr (_("Skipping extension '%s': %s\n"),
                                  g_file_peek_path (subdir), error->message);
                      g_error_free (error);
                    }
                }
            }
          else
            {
              g_printerr (_("Skipping unknown file '%s' in extension directory.\n"),
                          g_file_peek_path (subdir));
            }

          g_object_unref (subdir);
          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
clay_extension_manager_extension_running (ClayExtension        *extension,
                                          GParamSpec           *pspec,
                                          ClayExtensionManager *manager)
{
  gboolean running;

  g_object_get (extension,
                "running", &running,
                NULL);
  if (running)
    g_hash_table_insert (manager->p->running_extensions,
                         (gpointer) clay_object_get_name (extension),
                         extension);
  else
    g_hash_table_remove (manager->p->running_extensions,
                         (gpointer) clay_object_get_name (extension));

  clay_extension_manager_refresh (manager);
}

static gchar *
clay_store_json_get_string (JsonObject  *object,
                            const gchar *key)
{
  if (! json_object_has_member (object, key))
    return NULL;

  return g_strdup (json_object_get_string_member (object, key));
}

static void
clay_extension_manager_parse_store_json (ClayExtensionManager *manager,
                                         const gchar          *json,
                                         gsize                 length)
{
  JsonParser *parser;
  JsonNode   *root;
  JsonObject *object;
  JsonArray  *array;
  GError     *error = NULL;
  guint       i;

  parser = json_parser_new ();
  if (! json_parser_load_from_data (parser, json, length, &error))
    {
      g_printerr ("Failed to parse Clay store catalog: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return;
    }

  root = json_parser_get_root (parser);
  if (! JSON_NODE_HOLDS_OBJECT (root))
    {
      g_object_unref (parser);
      return;
    }

  object = json_node_get_object (root);

  if (json_object_has_member (object, "registry_url"))
    {
      g_free (manager->p->store_url);
      manager->p->store_url = g_strdup (json_object_get_string_member (object, "registry_url"));
    }

  if (! json_object_has_member (object, "extensions"))
    {
      g_object_unref (parser);
      return;
    }

  array = json_object_get_array_member (object, "extensions");
  if (! array)
    {
      g_object_unref (parser);
      return;
    }

  g_list_free_full (manager->p->store_catalog, (GDestroyNotify) clay_store_entry_free);
  manager->p->store_catalog = NULL;

  for (i = 0; i < json_array_get_length (array); i++)
    {
      JsonObject     *item = json_array_get_object_element (array, i);
      ClayStoreEntry *entry;

      if (! item || ! json_object_has_member (item, "id"))
        continue;

      entry = g_new0 (ClayStoreEntry, 1);
      entry->id             = clay_store_json_get_string (item, "id");
      entry->name           = clay_store_json_get_string (item, "name");
      entry->name_de        = clay_store_json_get_string (item, "name_de");
      entry->summary        = clay_store_json_get_string (item, "summary");
      entry->summary_de     = clay_store_json_get_string (item, "summary_de");
      entry->description    = clay_store_json_get_string (item, "description");
      entry->category       = clay_store_json_get_string (item, "category");
      entry->tags           = clay_store_json_get_string (item, "tags");
      entry->version        = clay_store_json_get_string (item, "version");
      entry->author         = clay_store_json_get_string (item, "author");
      entry->icon_name      = clay_store_json_get_string (item, "icon_name");
      entry->download_url   = clay_store_json_get_string (item, "download_url");
      entry->local_gex_path = clay_store_json_get_string (item, "local_gex_path");
      entry->menu_hint      = clay_store_json_get_string (item, "menu_hint");
      entry->rating         = json_object_has_member (item, "rating") ?
                              json_object_get_double_member (item, "rating") : 0.0;
      entry->downloads      = json_object_has_member (item, "downloads") ?
                              (gint) json_object_get_int_member (item, "downloads") : 0;
      entry->featured       = json_object_has_member (item, "featured") &&
                              json_object_get_boolean_member (item, "featured");

      manager->p->store_catalog = g_list_prepend (manager->p->store_catalog, entry);
    }

  manager->p->store_catalog = g_list_reverse (manager->p->store_catalog);
  g_object_unref (parser);
}

static void
clay_extension_manager_load_store_file (ClayExtensionManager *manager,
                                        GFile                *file)
{
  gchar  *contents = NULL;
  gsize   length   = 0;
  GError *error    = NULL;

  if (g_file_load_contents (file, NULL, &contents, &length, NULL, &error))
    {
      clay_extension_manager_parse_store_json (manager, contents, length);
      g_free (contents);
    }
  else
    {
      g_printerr ("Could not load Clay store catalog: %s\n", error->message);
      g_error_free (error);
    }
}

static void
clay_extension_manager_fetch_store_ready (GObject      *source,
                                          GAsyncResult *result,
                                          gpointer      user_data)
{
  ClayExtensionManager *manager  = user_data;
  gchar                *contents = NULL;
  gsize                 length   = 0;
  GError               *error    = NULL;

  if (g_file_load_contents_finish (G_FILE (source), result,
                                   &contents, &length, NULL, &error))
    {
      clay_extension_manager_parse_store_json (manager, contents, length);
      g_free (contents);
      g_signal_emit (manager, signals[STORE_UPDATED], 0);
    }
  else if (error)
    {
      g_printerr ("Remote Clay store catalog unavailable: %s\n", error->message);
      g_error_free (error);
    }

  g_object_unref (manager);
}

static gboolean
clay_extension_manager_copy_directory (GFile   *src,
                                       GFile   *dest,
                                       GError **error)
{
  GFileEnumerator *enumerator;
  GFileInfo       *info;

  if (! g_file_make_directory_with_parents (dest, NULL, error))
    {
      if (error && *error &&
          ! g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_EXISTS))
        return FALSE;
      g_clear_error (error);
    }

  enumerator = g_file_enumerate_children (src,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          NULL, error);
  if (! enumerator)
    return FALSE;

  while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
    {
      GFile    *child_src  = g_file_enumerator_get_child (enumerator, info);
      GFile    *child_dest = g_file_get_child (dest, g_file_info_get_name (info));
      gboolean  ok         = TRUE;

      if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
        {
          ok = clay_extension_manager_copy_directory (child_src, child_dest, error);
        }
      else
        {
          ok = g_file_copy (child_src, child_dest,
                            G_FILE_COPY_OVERWRITE | G_FILE_COPY_NOFOLLOW_SYMLINKS,
                            NULL, NULL, NULL, error);
        }

      g_object_unref (child_src);
      g_object_unref (child_dest);
      g_object_unref (info);

      if (! ok)
        {
          g_object_unref (enumerator);
          return FALSE;
        }
    }

  g_object_unref (enumerator);
  return TRUE;
}
