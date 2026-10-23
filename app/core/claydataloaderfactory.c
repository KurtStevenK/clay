/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydataloaderfactory.c
 * Copyright (C) 2001-2018 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clay-utils.h"
#include "claycontainer.h"
#include "claydata.h"
#include "claydataloaderfactory.h"

#include "clay-intl.h"


/* Data files that have this string in their path are considered
 * obsolete and are only kept around for backwards compatibility
 */
#define CLAY_OBSOLETE_DATA_DIR_NAME "clay-obsolete-files"


typedef struct _ClayDataLoader ClayDataLoader;

struct _ClayDataLoader
{
  gchar            *name;
  ClayDataLoadFunc  load_func;
  gchar            *extension;
  gboolean          writable;
};


struct _ClayDataLoaderFactoryPrivate
{
  GList          *loaders;
  ClayDataLoader *fallback;
};

#define GET_PRIVATE(obj) (((ClayDataLoaderFactory *) (obj))->priv)


static void   clay_data_loader_factory_finalize       (GObject         *object);

static void   clay_data_loader_factory_data_init      (ClayDataFactory *factory,
                                                       ClayContext     *context);
static void   clay_data_loader_factory_data_refresh   (ClayDataFactory *factory,
                                                       ClayContext     *context);

static ClayDataLoader *
              clay_data_loader_factory_get_loader     (ClayDataFactory *factory,
                                                       GFile           *file);

static void   clay_data_loader_factory_load           (ClayDataFactory *factory,
                                                       ClayContext     *context,
                                                       GHashTable      *cache);
static void   clay_data_loader_factory_load_directory (ClayDataFactory *factory,
                                                       ClayContext     *context,
                                                       GHashTable      *cache,
                                                       gboolean         dir_writable,
                                                       GFile           *directory,
                                                       GFile           *top_directory);
static void   clay_data_loader_factory_load_data      (ClayDataFactory *factory,
                                                       ClayContext     *context,
                                                       GHashTable      *cache,
                                                       gboolean         dir_writable,
                                                       GFile           *file,
                                                       GFileInfo       *info,
                                                       GFile           *top_directory);

static ClayDataLoader * clay_data_loader_new          (const gchar     *name,
                                                       ClayDataLoadFunc load_func,
                                                       const gchar     *extension,
                                                       gboolean         writable);
static void            clay_data_loader_free          (ClayDataLoader  *loader);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDataLoaderFactory, clay_data_loader_factory,
                            CLAY_TYPE_DATA_FACTORY)

#define parent_class clay_data_loader_factory_parent_class


static void
clay_data_loader_factory_class_init (ClayDataLoaderFactoryClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  ClayDataFactoryClass *factory_class = CLAY_DATA_FACTORY_CLASS (klass);

  object_class->finalize      = clay_data_loader_factory_finalize;

  factory_class->data_init    = clay_data_loader_factory_data_init;
  factory_class->data_refresh = clay_data_loader_factory_data_refresh;
}

static void
clay_data_loader_factory_init (ClayDataLoaderFactory *factory)
{
  factory->priv = clay_data_loader_factory_get_instance_private (factory);
}

static void
clay_data_loader_factory_finalize (GObject *object)
{
  ClayDataLoaderFactoryPrivate *priv = GET_PRIVATE (object);

  g_list_free_full (priv->loaders, (GDestroyNotify) clay_data_loader_free);
  priv->loaders = NULL;

  g_clear_pointer (&priv->fallback, clay_data_loader_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_data_loader_factory_data_init (ClayDataFactory *factory,
                                    ClayContext     *context)
{
  clay_data_loader_factory_load (factory, context, NULL);
}

static void
clay_data_loader_factory_refresh_cache_add (ClayDataFactory *factory,
                                            ClayData        *data,
                                            gpointer         user_data)
{
  GFile *file = clay_data_get_file (data);

  if (file)
    {
      ClayContainer *container = clay_data_factory_get_container (factory);
      GHashTable    *cache     = user_data;
      GList         *list;

      g_object_ref (data);

      clay_container_remove (container, CLAY_OBJECT (data));

      list = g_hash_table_lookup (cache, file);
      list = g_list_prepend (list, data);

      g_hash_table_insert (cache, file, list);
    }
}

static gboolean
clay_data_loader_factory_refresh_cache_remove (gpointer key,
                                               gpointer value,
                                               gpointer user_data)
{
  GList *list;

  for (list = value; list; list = list->next)
    g_object_unref (list->data);

  g_list_free (value);

  return TRUE;
}

static void
clay_data_loader_factory_data_refresh (ClayDataFactory *factory,
                                       ClayContext     *context)
{
  ClayContainer *container = clay_data_factory_get_container (factory);
  GHashTable    *cache;

  clay_container_freeze (container);

  /*  First, save all dirty data objects  */
  clay_data_factory_data_save (factory);

  cache = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);

  clay_data_factory_data_foreach (factory, TRUE,
                                  clay_data_loader_factory_refresh_cache_add,
                                  cache);

  /*  Now the cache contains a GFile => list-of-objects mapping of
   *  the old objects. So we should now traverse the directory and for
   *  each file load it only if its mtime is newer.
   *
   *  Once a file was added, it is removed from the cache, so the only
   *  objects remaining there will be those that are not present on
   *  the disk (that have to be destroyed)
   */
  clay_data_loader_factory_load (factory, context, cache);

  /*  Now all the data is loaded. Free what remains in the cache  */
  g_hash_table_foreach_remove (cache,
                               clay_data_loader_factory_refresh_cache_remove,
                               NULL);

  g_hash_table_destroy (cache);

  clay_container_thaw (container);
}


/*  public functions  */

ClayDataFactory *
clay_data_loader_factory_new (Clay                    *clay,
                              GType                    data_type,
                              const gchar             *path_property_name,
                              const gchar             *writable_property_name,
                              const gchar             *ext_property_name,
                              ClayDataNewFunc          new_func,
                              ClayDataGetStandardFunc  get_standard_func)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, CLAY_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (writable_property_name != NULL, NULL);
  g_return_val_if_fail (ext_property_name != NULL, NULL);

  return g_object_new (CLAY_TYPE_DATA_LOADER_FACTORY,
                       "clay",                   clay,
                       "data-type",              data_type,
                       "path-property-name",     path_property_name,
                       "writable-property-name", writable_property_name,
                       "ext-property-name",      ext_property_name,
                       "new-func",               new_func,
                       "get-standard-func",      get_standard_func,
                       NULL);
}

void
clay_data_loader_factory_add_loader (ClayDataFactory  *factory,
                                     const gchar      *name,
                                     ClayDataLoadFunc  load_func,
                                     const gchar      *extension,
                                     gboolean          writable)
{
  ClayDataLoaderFactoryPrivate *priv;
  ClayDataLoader               *loader;

  g_return_if_fail (CLAY_IS_DATA_LOADER_FACTORY (factory));
  g_return_if_fail (name != NULL);
  g_return_if_fail (load_func != NULL);
  g_return_if_fail (extension != NULL);

  priv = GET_PRIVATE (factory);

  loader = clay_data_loader_new (name, load_func, extension, writable);

  priv->loaders = g_list_append (priv->loaders, loader);
}

void
clay_data_loader_factory_add_fallback (ClayDataFactory  *factory,
                                       const gchar      *name,
                                       ClayDataLoadFunc  load_func)
{
  ClayDataLoaderFactoryPrivate *priv;

  g_return_if_fail (CLAY_IS_DATA_LOADER_FACTORY (factory));
  g_return_if_fail (name != NULL);
  g_return_if_fail (load_func != NULL);

  priv = GET_PRIVATE (factory);

  g_clear_pointer (&priv->fallback, clay_data_loader_free);

  priv->fallback = clay_data_loader_new (name, load_func, NULL, FALSE);
}


/*  private functions  */

static ClayDataLoader *
clay_data_loader_factory_get_loader (ClayDataFactory *factory,
                                     GFile           *file)
{
  ClayDataLoaderFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                        *list;

  for (list = priv->loaders; list; list = g_list_next (list))
    {
      ClayDataLoader *loader = list->data;

      if (clay_file_has_extension (file, loader->extension))
        return loader;
    }

  return priv->fallback;
}

static void
clay_data_loader_factory_load (ClayDataFactory *factory,
                               ClayContext     *context,
                               GHashTable      *cache)
{
  const GList *ext_path;
  GList       *path;
  GList       *writable_path;
  GList       *list;

  path          = clay_data_factory_get_data_path          (factory);
  writable_path = clay_data_factory_get_data_path_writable (factory);
  ext_path      = clay_data_factory_get_data_path_ext      (factory);

  for (list = (GList *) ext_path; list; list = g_list_next (list))
    {
      /* Adding data from extensions.
       * Consider these always non-writable (even when the directory is
       * writable, since writability of extension is only taken into
       * account for extension update).
       */
      clay_data_loader_factory_load_directory (factory, context, cache,
                                               FALSE,
                                               list->data,
                                               list->data);
    }

  for (list = path; list; list = g_list_next (list))
    {
      gboolean dir_writable = FALSE;

      if (g_list_find_custom (writable_path, list->data,
                              (GCompareFunc) clay_file_compare))
        dir_writable = TRUE;

      clay_data_loader_factory_load_directory (factory, context, cache,
                                               dir_writable,
                                               list->data,
                                               list->data);
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);
}

static void
clay_data_loader_factory_load_directory (ClayDataFactory *factory,
                                         ClayContext     *context,
                                         GHashTable      *cache,
                                         gboolean         dir_writable,
                                         GFile           *directory,
                                         GFile           *top_directory)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType  file_type;
          GFile     *child;

          if (g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            {
              g_object_unref (info);
              continue;
            }

          file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);
          child     = g_file_enumerator_get_child (enumerator, info);

          if (file_type == G_FILE_TYPE_DIRECTORY)
            {
              clay_data_loader_factory_load_directory (factory, context, cache,
                                                       dir_writable,
                                                       child,
                                                       top_directory);
            }
          else if (file_type == G_FILE_TYPE_REGULAR)
            {
              clay_data_loader_factory_load_data (factory, context, cache,
                                                  dir_writable,
                                                  child, info,
                                                  top_directory);
            }

          g_object_unref (child);
          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
clay_data_loader_factory_load_data (ClayDataFactory *factory,
                                    ClayContext     *context,
                                    GHashTable      *cache,
                                    gboolean         dir_writable,
                                    GFile           *file,
                                    GFileInfo       *info,
                                    GFile           *top_directory)
{
  ClayDataLoader *loader;
  ClayContainer  *container;
  ClayContainer  *container_obsolete;
  GList          *data_list = NULL;
  GInputStream   *input;
  guint64         mtime;
  GError         *error = NULL;

  loader = clay_data_loader_factory_get_loader (factory, file);

  if (! loader)
    return;

  container          = clay_data_factory_get_container          (factory);
  container_obsolete = clay_data_factory_get_container_obsolete (factory);

  if (clay_data_factory_get_clay (factory)->be_verbose)
    g_print ("  Loading %s\n", clay_file_get_utf8_name (file));

  mtime = g_file_info_get_attribute_uint64 (info,
                                            G_FILE_ATTRIBUTE_TIME_MODIFIED);

  if (cache)
    {
      GList *cached_data = g_hash_table_lookup (cache, file);

      if (cached_data &&
          clay_data_get_mtime (cached_data->data) != 0 &&
          clay_data_get_mtime (cached_data->data) == mtime)
        {
          GList *list;

          for (list = cached_data; list; list = g_list_next (list))
            clay_container_add (container, list->data);

          return;
        }
    }

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));

  if (input)
    {
      GInputStream *buffered = g_buffered_input_stream_new (input);

      data_list = loader->load_func (context, file, buffered, &error);

      if (error)
        {
          g_prefix_error (&error,
                          _("Error loading '%s': "),
                          clay_file_get_utf8_name (file));
        }
      else if (! data_list)
        {
          g_set_error (&error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                       _("Error loading '%s'"),
                       clay_file_get_utf8_name (file));
        }

      g_object_unref (buffered);
      g_object_unref (input);
    }
  else
    {
      g_prefix_error (&error,
                      _("Could not open '%s' for reading: "),
                      clay_file_get_utf8_name (file));
    }

  if (G_LIKELY (data_list))
    {
      GList    *list;
      gchar    *uri;
      gboolean  obsolete;
      gboolean  writable  = FALSE;
      gboolean  deletable = FALSE;

      uri = g_file_get_uri (file);

      obsolete = (strstr (uri, CLAY_OBSOLETE_DATA_DIR_NAME) != 0);

      g_free (uri);

      /* obsolete files are immutable, don't check their writability */
      if (! obsolete)
        {
          deletable = (g_list_length (data_list) == 1 && dir_writable);
          writable  = (deletable && loader->writable);
        }

      for (list = data_list; list; list = g_list_next (list))
        {
          ClayData *data = list->data;

          clay_data_set_file (data, file, writable, deletable);
          clay_data_set_mtime (data, mtime);
          clay_data_clean (data);

          if (obsolete)
            {
              clay_container_add (container_obsolete,
                                  CLAY_OBJECT (data));
            }
          else
            {
              clay_data_set_folder_tags (data, top_directory);

              clay_container_add (container,
                                  CLAY_OBJECT (data));
            }

          g_object_unref (data);
        }

      g_list_free (data_list);
    }

  /*  not else { ... } because loader->load_func() can return a list
   *  of data objects *and* an error message if loading failed after
   *  something was already loaded
   */
  if (G_UNLIKELY (error))
    {
      clay_message (clay_data_factory_get_clay (factory), NULL,
                    CLAY_MESSAGE_ERROR,
                    _("Failed to load data:\n\n%s"), error->message);
      g_clear_error (&error);
    }
}

static ClayDataLoader *
clay_data_loader_new (const gchar      *name,
                      ClayDataLoadFunc  load_func,
                      const gchar      *extension,
                      gboolean          writable)
{
  ClayDataLoader *loader = g_slice_new (ClayDataLoader);

  loader->name      = g_strdup (name);
  loader->load_func = load_func;
  loader->extension = g_strdup (extension);
  loader->writable  = writable ? TRUE : FALSE;

  return loader;
}

static void
clay_data_loader_free (ClayDataLoader *loader)
{
  g_free (loader->name);
  g_free (loader->extension);

  g_slice_free (ClayDataLoader, loader);
}
