/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "claymoduletypes.h"

#include "claymodule.h"
#include "claymoduledb.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claymoduledb
 * @title: ClayModuleDB
 * @short_description: Keeps a list of #ClayModule's found in a given
 *                     searchpath.
 *
 * Keeps a list of #ClayModule's found in a given searchpath.
 **/


struct _ClayModuleDB
{
  GObject    parent_instance;

  GPtrArray *modules;

  gchar     *load_inhibit;
  gboolean   verbose;
};


static void         clay_module_db_finalize            (GObject      *object);

static void         clay_module_db_list_model_iface_init (GListModelInterface *iface);

static void         clay_module_db_load_directory      (ClayModuleDB *db,
                                                        GFile        *directory);
static void         clay_module_db_load_module         (ClayModuleDB *db,
                                                        GFile        *file);

static ClayModule * clay_module_db_module_find_by_file (ClayModuleDB *db,
                                                        GFile        *file);

static void         clay_module_db_module_dump_func    (gpointer      data,
                                                        gpointer      user_data);


G_DEFINE_TYPE_WITH_CODE (ClayModuleDB, clay_module_db, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                clay_module_db_list_model_iface_init))

#define parent_class clay_module_db_parent_class


static void
clay_module_db_class_init (ClayModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_module_db_finalize;
}

static void
clay_module_db_init (ClayModuleDB *db)
{
  db->modules      = g_ptr_array_new ();
  db->load_inhibit = NULL;
  db->verbose      = FALSE;
}

static void
clay_module_db_finalize (GObject *object)
{
  ClayModuleDB *db = CLAY_MODULE_DB (object);

  g_clear_pointer (&db->modules,      g_ptr_array_unref);
  g_clear_pointer (&db->load_inhibit, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GType
clay_module_db_get_item_type (GListModel *list)
{
  return CLAY_TYPE_MODULE;
}

static guint
clay_module_db_get_n_items (GListModel *list)
{
  ClayModuleDB *self = CLAY_MODULE_DB (list);

  return self->modules->len;
}

static void *
clay_module_db_get_item (GListModel *list,
                         guint       index)
{
  ClayModuleDB *self = CLAY_MODULE_DB (list);

  if (index >= self->modules->len)
    return NULL;

  return g_object_ref (g_ptr_array_index (self->modules, index));
}

static void
clay_module_db_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = clay_module_db_get_item_type;
  iface->get_n_items   = clay_module_db_get_n_items;
  iface->get_item      = clay_module_db_get_item;
}

/**
 * clay_module_db_new:
 * @verbose: Pass %TRUE to enable debugging output.
 *
 * Creates a new #ClayModuleDB instance. The @verbose parameter will be
 * passed to the created #ClayModule instances using clay_module_new().
 *
 * Returns: The new #ClayModuleDB instance.
 **/
ClayModuleDB *
clay_module_db_new (gboolean verbose)
{
  ClayModuleDB *db;

  db = g_object_new (CLAY_TYPE_MODULE_DB, NULL);

  db->verbose = verbose ? TRUE : FALSE;

  return db;
}

/**
 * clay_module_db_set_verbose:
 * @db:      A #ClayModuleDB.
 * @verbose: the new 'verbose' setting
 *
 * Sets the 'verbose' setting of @db.
 *
 * Since: 3.0
 **/
void
clay_module_db_set_verbose (ClayModuleDB *db,
                            gboolean      verbose)
{
  g_return_if_fail (CLAY_IS_MODULE_DB (db));

  db->verbose = verbose ? TRUE : FALSE;
}

/**
 * clay_module_db_get_verbose:
 * @db: A #ClayModuleDB.
 *
 * Returns the 'verbose' setting of @db.
 *
 * Returns: the 'verbose' setting.
 *
 * Since: 3.0
 **/
gboolean
clay_module_db_get_verbose (ClayModuleDB *db)
{
  g_return_val_if_fail (CLAY_IS_MODULE_DB (db), FALSE);

  return db->verbose;
}

static gboolean
is_in_inhibit_list (GFile       *file,
                    const gchar *inhibit_list)
{
  gchar       *filename;
  const gchar *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  if (! inhibit_list || ! strlen (inhibit_list))
    return FALSE;

  filename = g_file_get_path (file);

  p = strstr (inhibit_list, filename);
  if (! p)
    {
      g_free (filename);
      return FALSE;
    }

  /* we have a substring, but check for colons either side */
  start = p;
  while (start != inhibit_list && *start != G_SEARCHPATH_SEPARATOR)
    start--;

  if (*start == G_SEARCHPATH_SEPARATOR)
    start++;

  end = strchr (p, G_SEARCHPATH_SEPARATOR);
  if (! end)
    end = inhibit_list + strlen (inhibit_list);

  pathlen = strlen (filename);

  g_free (filename);

  if ((end - start) == pathlen)
    return TRUE;

  return FALSE;
}

/**
 * clay_module_db_set_load_inhibit:
 * @db:           A #ClayModuleDB.
 * @load_inhibit: A #G_SEARCHPATH_SEPARATOR delimited list of module
 *                filenames to exclude from auto-loading.
 *
 * Sets the @load_inhibit flag for all #ClayModule's which are kept
 * by @db (using clay_module_set_load_inhibit()).
 **/
void
clay_module_db_set_load_inhibit (ClayModuleDB *db,
                                 const gchar  *load_inhibit)
{
  guint i;

  g_return_if_fail (CLAY_IS_MODULE_DB (db));

  g_free (db->load_inhibit);

  db->load_inhibit = g_strdup (load_inhibit);

  for (i = 0; i < db->modules->len; i++)
    {
      ClayModule *module = g_ptr_array_index (db->modules, i);
      gboolean    inhibit;

      inhibit = is_in_inhibit_list (clay_module_get_file (module),
                                    load_inhibit);

      clay_module_set_auto_load (module, ! inhibit);
    }
}

/**
 * clay_module_db_get_load_inhibit:
 * @db: A #ClayModuleDB.
 *
 * Return the #G_SEARCHPATH_SEPARATOR delimited list of module filenames
 * which are excluded from auto-loading.
 *
 * Returns: the @db's @load_inhibit string.
 **/
const gchar *
clay_module_db_get_load_inhibit (ClayModuleDB *db)
{
  g_return_val_if_fail (CLAY_IS_MODULE_DB (db), NULL);

  return db->load_inhibit;
}

/**
 * clay_module_db_load:
 * @db:          A #ClayModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Scans the directories contained in @module_path and creates a
 * #ClayModule instance for every loadable module contained in the
 * directories.
 **/
void
clay_module_db_load (ClayModuleDB *db,
                     const gchar  *module_path)
{
  g_return_if_fail (CLAY_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  if (g_module_supported ())
    {
      GList *path;
      GList *list;

      path = clay_config_path_expand_to_files (module_path, NULL);

      for (list = path; list; list = g_list_next (list))
        {
          clay_module_db_load_directory (db, list->data);
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  if (FALSE)
    g_ptr_array_foreach (db->modules, clay_module_db_module_dump_func, NULL);
}

/**
 * clay_module_db_refresh:
 * @db:          A #ClayModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Does the same as clay_module_db_load(), plus removes all #ClayModule
 * instances whose modules have been deleted from disk.
 *
 * Note that the #ClayModule's will just be removed from the internal
 * list and not freed as this is not possible with #GTypeModule
 * instances which actually implement types.
 **/
void
clay_module_db_refresh (ClayModuleDB *db,
                        const gchar  *module_path)
{
  guint i;

  g_return_if_fail (CLAY_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  for (i = 0; i < db->modules->len; i++)
    {
      ClayModule *module = g_ptr_array_index (db->modules, i);

      if (! clay_module_is_on_disk (module) &&
          ! clay_module_is_loaded (module))
        {
          g_ptr_array_remove_index (db->modules, i);
          g_list_model_items_changed (G_LIST_MODEL (db), i, 1, 0);
          i--;
        }
    }

  /* walk filesystem and add new things we find */
  clay_module_db_load (db, module_path);
}

static void
clay_module_db_load_directory (ClayModuleDB *db,
                               GFile        *directory)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);

         if (file_type == G_FILE_TYPE_REGULAR &&
             ! g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            {
              GFile *child = g_file_enumerator_get_child (enumerator, info);

              clay_module_db_load_module (db, child);

              g_object_unref (child);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
clay_module_db_load_module (ClayModuleDB *db,
                            GFile        *file)
{
  gchar      *filename;
  ClayModule *module;
  GModule    *gmodule;
  gboolean   load_inhibit;

  filename = g_file_get_path (file);
  if (! (gmodule = g_module_open (filename, G_MODULE_BIND_LAZY)))
    {
      g_free (filename);
      return;
    }
  g_free (filename);
  g_module_close (gmodule);

  /* don't load if we already know about it */
  if (clay_module_db_module_find_by_file (db, file))
    return;

  load_inhibit = is_in_inhibit_list (file, db->load_inhibit);

  module = clay_module_new (file,
                            ! load_inhibit,
                            db->verbose);

  g_ptr_array_add (db->modules, module);
  g_list_model_items_changed (G_LIST_MODEL (db), db->modules->len - 1, 0, 1);
}

static ClayModule *
clay_module_db_module_find_by_file (ClayModuleDB *db,
                                    GFile        *file)
{
  guint i;

  for (i = 0; i < db->modules->len; i++)
    {
      ClayModule *module = g_ptr_array_index (db->modules, i);

      if (g_file_equal (file, clay_module_get_file (module)))
        return module;
    }

  return NULL;
}

static void
clay_module_db_module_dump_func (gpointer data,
                                 gpointer user_data)
{
  static const gchar * const statenames[] =
  {
    N_("Module error"),
    N_("Loaded"),
    N_("Load failed"),
    N_("Not loaded")
  };

  ClayModule           *module = data;
  const ClayModuleInfo *info   = clay_module_get_info (module);

  g_print ("\n%s: %s\n",
           clay_file_get_utf8_name (clay_module_get_file (module)),
           gettext (statenames[clay_module_get_state (module)]));

#if 0
  g_print ("  module: %p  lasterr: %s  query: %p register: %p\n",
           module->module,
           module->last_module_error ? module->last_module_error : "NONE",
           module->query_module,
           module->register_module);
#endif

  if (info)
    {
      g_print ("  purpose:   %s\n"
               "  author:    %s\n"
               "  version:   %s\n"
               "  copyright: %s\n"
               "  date:      %s\n",
               info->purpose   ? info->purpose   : "NONE",
               info->author    ? info->author    : "NONE",
               info->version   ? info->version   : "NONE",
               info->copyright ? info->copyright : "NONE",
               info->date      ? info->date      : "NONE");
    }
}
