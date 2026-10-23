/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydatafactory.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <stdlib.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clay-utils.h"
#include "clayasyncset.h"
#include "claycancelable.h"
#include "claycontext.h"
#include "claydata.h"
#include "claydatafactory.h"
#include "claylist.h"
#include "clayuncancelablewaitable.h"
#include "claywaitable.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_DATA_TYPE,
  PROP_PATH_PROPERTY_NAME,
  PROP_WRITABLE_PROPERTY_NAME,
  PROP_EXT_PROPERTY_NAME,
  PROP_NEW_FUNC,
  PROP_GET_STANDARD_FUNC,
  PROP_UNIQUE_NAMES
};


typedef struct _ClayDataFactoryPrivate ClayDataFactoryPrivate;

struct _ClayDataFactoryPrivate
{
  Clay                    *clay;

  GType                    data_type;
  ClayContainer           *container;
  ClayContainer           *container_obsolete;
  gboolean                 unique_names;

  gchar                   *path_property_name;
  gchar                   *writable_property_name;
  gchar                   *ext_property_name;

  ClayDataNewFunc          data_new_func;
  ClayDataGetStandardFunc  data_get_standard_func;

  ClayAsyncSet            *async_set;
};

#define GET_PRIVATE(factory) \
  ((ClayDataFactoryPrivate *) clay_data_factory_get_instance_private ((ClayDataFactory *) (factory)))


typedef struct
{
  const gchar *name;
  const gchar *collection;
  gboolean     is_internal;
} SearchData;


static void       clay_data_factory_constructed         (GObject             *object);
static void       clay_data_factory_set_property        (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void       clay_data_factory_get_property        (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void       clay_data_factory_finalize            (GObject             *object);

static gint64     clay_data_factory_get_memsize         (ClayObject          *object,
                                                         gint64              *gui_size);

static void       clay_data_factory_real_data_save      (ClayDataFactory     *factory);
static void       clay_data_factory_real_data_cancel    (ClayDataFactory     *factory);
static ClayData * clay_data_factory_real_data_duplicate (ClayDataFactory     *factory,
                                                         ClayData            *data);
static gboolean   clay_data_factory_real_data_delete    (ClayDataFactory     *factory,
                                                         ClayData            *data,
                                                         gboolean             delete_from_disk,
                                                         GError             **error);

static void       clay_data_factory_path_notify         (GObject             *object,
                                                         const GParamSpec    *pspec,
                                                         ClayDataFactory     *factory);
static GFile    * clay_data_factory_get_save_dir        (ClayDataFactory     *factory,
                                                         GError             **error);

static gboolean   clay_data_factory_search_in_container (ClayData            *data,
                                                         SearchData          *search_data);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayDataFactory, clay_data_factory,
                                     CLAY_TYPE_OBJECT)

#define parent_class clay_data_factory_parent_class


static void
clay_data_factory_class_init (ClayDataFactoryClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->constructed      = clay_data_factory_constructed;
  object_class->set_property     = clay_data_factory_set_property;
  object_class->get_property     = clay_data_factory_get_property;
  object_class->finalize         = clay_data_factory_finalize;

  clay_object_class->get_memsize = clay_data_factory_get_memsize;

  klass->data_init               = NULL;
  klass->data_refresh            = NULL;
  klass->data_save               = clay_data_factory_real_data_save;
  klass->data_cancel             = clay_data_factory_real_data_cancel;
  klass->data_duplicate          = clay_data_factory_real_data_duplicate;
  klass->data_delete             = clay_data_factory_real_data_delete;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DATA_TYPE,
                                   g_param_spec_gtype ("data-type", NULL, NULL,
                                                       CLAY_TYPE_DATA,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PATH_PROPERTY_NAME,
                                   g_param_spec_string ("path-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WRITABLE_PROPERTY_NAME,
                                   g_param_spec_string ("writable-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_EXT_PROPERTY_NAME,
                                   g_param_spec_string ("ext-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_NEW_FUNC,
                                   g_param_spec_pointer ("new-func",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GET_STANDARD_FUNC,
                                   g_param_spec_pointer ("get-standard-func",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UNIQUE_NAMES,
                                   g_param_spec_boolean ("unique-names",
                                                         NULL, NULL,
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

}

static void
clay_data_factory_init (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  priv->async_set = clay_async_set_new ();
}

static void
clay_data_factory_constructed (GObject *object)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (priv->clay));
  clay_assert (g_type_is_a (priv->data_type, CLAY_TYPE_DATA));
  clay_assert (CLAY_DATA_FACTORY_GET_CLASS (object)->data_init != NULL);
  clay_assert (CLAY_DATA_FACTORY_GET_CLASS (object)->data_refresh != NULL);

  /* Passing along the "unique names" property to the data container. */
  priv->container = clay_list_new (priv->data_type, priv->unique_names);
  clay_list_set_sort_func (CLAY_LIST (priv->container),
                           (GCompareFunc) clay_data_compare);

  priv->container_obsolete = clay_list_new (priv->data_type, TRUE);
  clay_list_set_sort_func (CLAY_LIST (priv->container_obsolete),
                           (GCompareFunc) clay_data_compare);
}

static void
clay_data_factory_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      priv->clay = g_value_get_object (value); /* don't ref */
      break;

    case PROP_DATA_TYPE:
      priv->data_type = g_value_get_gtype (value);
      break;

    case PROP_PATH_PROPERTY_NAME:
      priv->path_property_name = g_value_dup_string (value);
      break;

    case PROP_WRITABLE_PROPERTY_NAME:
      priv->writable_property_name = g_value_dup_string (value);
      break;

    case PROP_EXT_PROPERTY_NAME:
      priv->ext_property_name = g_value_dup_string (value);
      break;

    case PROP_NEW_FUNC:
      priv->data_new_func = g_value_get_pointer (value);
      break;

    case PROP_GET_STANDARD_FUNC:
      priv->data_get_standard_func = g_value_get_pointer (value);
      break;

    case PROP_UNIQUE_NAMES:
      priv->unique_names = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_data_factory_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, priv->clay);
      break;

    case PROP_DATA_TYPE:
      g_value_set_gtype (value, priv->data_type);
      break;

    case PROP_PATH_PROPERTY_NAME:
      g_value_set_string (value, priv->path_property_name);
      break;

    case PROP_WRITABLE_PROPERTY_NAME:
      g_value_set_string (value, priv->writable_property_name);
      break;

    case PROP_EXT_PROPERTY_NAME:
      g_value_set_string (value, priv->ext_property_name);
      break;

    case PROP_NEW_FUNC:
      g_value_set_pointer (value, priv->data_new_func);
      break;

    case PROP_GET_STANDARD_FUNC:
      g_value_set_pointer (value, priv->data_get_standard_func);
      break;

    case PROP_UNIQUE_NAMES:
      g_value_set_boolean (value, priv->unique_names);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_data_factory_finalize (GObject *object)
{
  ClayDataFactory        *factory = CLAY_DATA_FACTORY (object);
  ClayDataFactoryPrivate *priv    = GET_PRIVATE (object);

  if (priv->async_set)
    {
      clay_data_factory_data_cancel (factory);

      g_clear_object (&priv->async_set);
    }

  g_clear_object (&priv->container);
  g_clear_object (&priv->container_obsolete);

  g_clear_pointer (&priv->path_property_name,     g_free);
  g_clear_pointer (&priv->writable_property_name, g_free);
  g_clear_pointer (&priv->ext_property_name,      g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_data_factory_get_memsize (ClayObject *object,
                               gint64     *gui_size)
{
  ClayDataFactoryPrivate *priv    = GET_PRIVATE (object);
  gint64                  memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (priv->container),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (priv->container_obsolete),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_data_factory_real_data_save (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *dirty = NULL;
  GList                  *list;
  GFile                  *writable_dir;
  GError                 *error = NULL;

  for (list = CLAY_LIST (priv->container)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayData *data = list->data;

      if (clay_data_is_dirty (data) &&
          clay_data_is_writable (data))
        {
          dirty = g_list_prepend (dirty, data);
        }
    }

  if (! dirty)
    return;

  writable_dir = clay_data_factory_get_save_dir (factory, &error);

  if (! writable_dir)
    {
      clay_message (priv->clay, NULL, CLAY_MESSAGE_ERROR,
                    _("Failed to save data:\n\n%s"),
                    error->message);
      g_clear_error (&error);

      g_list_free (dirty);

      return;
    }

  for (list = dirty; list; list = g_list_next (list))
    {
      ClayData *data  = list->data;
      GError   *error = NULL;

      if (clay_data_get_image (data))
        continue;

      if (! clay_data_get_file (data))
        clay_data_create_filename (data, writable_dir);

      if (priv->clay->be_verbose)
        {
          GFile *file = clay_data_get_file (data);

          if (file)
            g_print ("Writing dirty data '%s'\n",
                     clay_file_get_utf8_name (file));
        }

      if (! clay_data_save (data, &error))
        {
          /*  check if there actually was an error (no error
           *  means the data class does not implement save)
           */
          if (error)
            {
              clay_message (priv->clay, NULL, CLAY_MESSAGE_ERROR,
                            _("Failed to save data:\n\n%s"),
                            error->message);
              g_clear_error (&error);
            }
        }
    }

  g_object_unref (writable_dir);

  g_list_free (dirty);
}

static void
clay_data_factory_real_data_cancel (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  clay_cancelable_cancel (CLAY_CANCELABLE (priv->async_set));
  clay_waitable_wait     (CLAY_WAITABLE   (priv->async_set));
}

static ClayData *
clay_data_factory_real_data_duplicate (ClayDataFactory *factory,
                                       ClayData        *data)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  ClayData               *new_data;

  new_data = clay_data_duplicate (data);

  if (new_data)
    {
      const gchar *name = clay_object_get_name (data);
      const gchar *ext;
      gint         copy_len;
      gint         number;
      gchar       *new_name;
      GError      *error = NULL;

      ext      = strrchr (name, '#');
      copy_len = strlen (_("copy"));

      if ((strlen (name) >= copy_len                                 &&
           strcmp (&name[strlen (name) - copy_len], _("copy")) == 0) ||
          (ext && (number = atoi (ext + 1)) > 0                      &&
           ((gint) (log10 (number) + 1)) == strlen (ext + 1)))
        {
          /* don't have redundant "copy"s */
          new_name = g_strdup (name);
        }
      else
        {
          new_name = g_strdup_printf (_("%s copy"), name);
        }

      clay_object_take_name (CLAY_OBJECT (new_data), new_name);

      if (! clay_data_factory_data_save_single (factory, new_data, &error))
        g_critical ("%s: data saving failed: %s", G_STRFUNC, error->message);

      clay_container_add (priv->container, CLAY_OBJECT (new_data));
      g_object_unref (new_data);
      g_clear_error (&error);
    }

  return new_data;
}

static gboolean
clay_data_factory_real_data_delete (ClayDataFactory  *factory,
                                    ClayData         *data,
                                    gboolean          delete_from_disk,
                                    GError          **error)
{
  if (delete_from_disk && clay_data_get_file (data))
    return clay_data_delete_from_disk (data, error);

  return TRUE;
}


/*  public functions  */

void
clay_data_factory_data_init (ClayDataFactory *factory,
                             ClayContext     *context,
                             gboolean         no_data)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *signal_name;

  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  /*  Always freeze() and thaw() the container around initialization,
   *  even if no_data, the thaw() will implicitly make ClayContext
   *  create the standard data that serves as fallback.
   */
  clay_container_freeze (priv->container);

  if (! no_data)
    {
      if (priv->clay->be_verbose)
        {
          const gchar *name = clay_object_get_name (factory);

          g_print ("Loading '%s' data\n", name ? name : "???");
        }

      CLAY_DATA_FACTORY_GET_CLASS (factory)->data_init (factory, context);
    }

  clay_container_thaw (priv->container);

  signal_name = g_strdup_printf ("notify::%s", priv->path_property_name);
  g_signal_connect_object (priv->clay->config, signal_name,
                           G_CALLBACK (clay_data_factory_path_notify),
                           factory, 0);
  g_free (signal_name);

  signal_name = g_strdup_printf ("notify::%s", priv->ext_property_name);
  g_signal_connect_object (priv->clay->extension_manager, signal_name,
                           G_CALLBACK (clay_data_factory_path_notify),
                           factory, 0);
  g_free (signal_name);
}

static void
clay_data_factory_clean_cb (ClayDataFactory *factory,
                            ClayData        *data,
                            gpointer         user_data)
{
  if (clay_data_is_dirty (data))
    clay_data_clean (data);
}

void
clay_data_factory_data_clean (ClayDataFactory *factory)
{
  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));

  clay_data_factory_data_foreach (factory, TRUE,
                                  clay_data_factory_clean_cb, NULL);
}

void
clay_data_factory_data_refresh (ClayDataFactory *factory,
                                ClayContext     *context)
{
  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  CLAY_DATA_FACTORY_GET_CLASS (factory)->data_refresh (factory, context);
}

void
clay_data_factory_data_save (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));

  if (! clay_container_is_empty (priv->container))
    CLAY_DATA_FACTORY_GET_CLASS (factory)->data_save (factory);
}

static void
clay_data_factory_data_free_foreach (ClayDataFactory *factory,
                                     ClayData        *data,
                                     gpointer         user_data)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  clay_container_remove (priv->container, CLAY_OBJECT (data));
}

void
clay_data_factory_data_free (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));

  clay_data_factory_data_cancel (factory);

  if (! clay_container_is_empty (priv->container))
    {
      clay_container_freeze (priv->container);

      clay_data_factory_data_foreach (factory, TRUE,
                                      clay_data_factory_data_free_foreach,
                                      NULL);

      clay_container_thaw (priv->container);
    }
}

ClayAsyncSet *
clay_data_factory_get_async_set (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  return priv->async_set;
}

gboolean
clay_data_factory_data_wait (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  ClayWaitable           *waitable;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), FALSE);

  /* don't allow cancellation for now */
  waitable = clay_uncancelable_waitable_new (CLAY_WAITABLE (priv->async_set));

  clay_wait (priv->clay, waitable,
             _("Loading fonts (this may take a while...)"));

  g_object_unref (waitable);

  return TRUE;
}

void
clay_data_factory_data_cancel (ClayDataFactory *factory)
{
  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));

  CLAY_DATA_FACTORY_GET_CLASS (factory)->data_cancel (factory);
}

ClayData *
clay_data_factory_get_data (ClayDataFactory *factory,
                            const gchar     *name,
                            const gchar     *collection,
                            gboolean         is_internal)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  ClayContainer          *container;
  ClayObject             *data;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  container = clay_data_factory_get_container (factory);

  if (collection == NULL)
    {
      data = clay_container_get_child_by_name (container, name);
    }
  else
    {
      SearchData *search_data = g_new (SearchData, 1);

      search_data->name        = name;
      search_data->collection  = collection;
      search_data->is_internal = is_internal;
      data = clay_container_search (container,
                                    (ClayContainerSearchFunc) clay_data_factory_search_in_container,
                                    search_data);
      g_free (search_data);
    }

  if (! data)
    data = clay_container_get_child_by_name (clay_data_factory_get_container_obsolete (factory),
                                             name);

  if (! data && ! strcmp (name, "Standard"))
    data = (ClayObject *) clay_data_factory_data_get_standard (factory,
                                                               clay_get_user_context (priv->clay));

  return (ClayData *) data;
}

gboolean
clay_data_factory_has_data_new_func (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), FALSE);

  return priv->data_new_func != NULL;
}

ClayData *
clay_data_factory_data_new (ClayDataFactory *factory,
                            ClayContext     *context,
                            const gchar     *name)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  if (priv->data_new_func)
    {
      ClayData *data = priv->data_new_func (context, name);

      if (data)
        {
          GError *error = NULL;

          if (! clay_data_factory_data_save_single (factory, data, &error))
            g_critical ("%s: data saving failed: %s", G_STRFUNC, error->message);

          clay_container_add (priv->container, CLAY_OBJECT (data));
          g_object_unref (data);
          g_clear_error (&error);

          return data;
        }

      g_warning ("%s: ClayDataFactory::data_new_func() returned NULL",
                 G_STRFUNC);
    }

  return NULL;
}

ClayData *
clay_data_factory_data_get_standard (ClayDataFactory *factory,
                                     ClayContext     *context)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  if (priv->data_get_standard_func)
    return priv->data_get_standard_func (context);

  return NULL;
}

ClayData *
clay_data_factory_data_duplicate (ClayDataFactory *factory,
                                  ClayData        *data)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_DATA (data), NULL);

  return CLAY_DATA_FACTORY_GET_CLASS (factory)->data_duplicate (factory, data);
}

gboolean
clay_data_factory_data_delete (ClayDataFactory  *factory,
                               ClayData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  ClayDataFactoryPrivate *priv   = GET_PRIVATE (factory);
  gboolean                retval = TRUE;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (CLAY_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_container_have (priv->container, CLAY_OBJECT (data)))
    {
      g_object_ref (data);

      clay_container_remove (priv->container, CLAY_OBJECT (data));

      retval = CLAY_DATA_FACTORY_GET_CLASS (factory)->data_delete (factory, data,
                                                                   delete_from_disk,
                                                                   error);

      g_object_unref (data);
    }

  return retval;
}

gboolean
clay_data_factory_data_save_single (ClayDataFactory  *factory,
                                    ClayData         *data,
                                    GError          **error)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (CLAY_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_data_is_dirty (data) || clay_data_get_image (data))
    return TRUE;

  if (! clay_data_get_file (data))
    {
      GFile  *writable_dir;
      GError *my_error = NULL;

      writable_dir = clay_data_factory_get_save_dir (factory, &my_error);

      if (! writable_dir)
        {
          g_set_error (error, CLAY_DATA_ERROR, 0,
                       _("Failed to save data:\n\n%s"),
                       my_error->message);
          g_clear_error (&my_error);

          return FALSE;
        }

      clay_data_create_filename (data, writable_dir);

      g_object_unref (writable_dir);
    }

  if (! clay_data_is_writable (data))
    return FALSE;

  if (priv->clay->be_verbose)
    {
      GFile *file = clay_data_get_file (data);

      if (file)
        g_print ("Writing dirty data '%s'\n",
                 clay_file_get_utf8_name (file));
    }

  if (! clay_data_save (data, error))
    {
      /*  check if there actually was an error (no error
       *  means the data class does not implement save)
       */
      if (! error)
        g_set_error (error, CLAY_DATA_ERROR, 0,
                     _("Failed to save data:\n\n%s"),
                     "Data class does not implement saving");

      return FALSE;
    }

  return TRUE;
}

void
clay_data_factory_data_foreach (ClayDataFactory     *factory,
                                gboolean             skip_internal,
                                ClayDataForeachFunc  callback,
                                gpointer             user_data)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *list;

  g_return_if_fail (CLAY_IS_DATA_FACTORY (factory));
  g_return_if_fail (callback != NULL);

  list = CLAY_LIST (priv->container)->queue->head;

  while (list)
    {
      GList *next = g_list_next (list);

      if (! (skip_internal && clay_data_is_internal (list->data)))
        callback (factory, list->data, user_data);

      list = next;
    }
}

Clay *
clay_data_factory_get_clay (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  return priv->clay;
}

GType
clay_data_factory_get_data_type (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), G_TYPE_NONE);

  return clay_container_get_child_type (priv->container);
}

ClayContainer *
clay_data_factory_get_container (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  return priv->container;
}

ClayContainer *
clay_data_factory_get_container_obsolete (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  return priv->container_obsolete;
}

GList *
clay_data_factory_get_data_path (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->clay->config,
                priv->path_property_name, &path,
                NULL);

  if (path)
    {
      list = clay_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}

GList *
clay_data_factory_get_data_path_writable (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->clay->config,
                priv->writable_property_name, &path,
                NULL);

  if (path)
    {
      list = clay_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}

const GList *
clay_data_factory_get_data_path_ext (ClayDataFactory *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *list = NULL;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->clay->extension_manager,
                priv->ext_property_name, &list,
                NULL);

  return list;
}


/*  private functions  */

static void
clay_data_factory_path_notify (GObject          *object,
                               const GParamSpec *pspec,
                               ClayDataFactory  *factory)
{
  ClayDataFactoryPrivate *priv = GET_PRIVATE (factory);

  clay_set_busy (priv->clay);

  clay_data_factory_data_refresh (factory, clay_get_user_context (priv->clay));

  clay_unset_busy (priv->clay);
}

static GFile *
clay_data_factory_get_save_dir (ClayDataFactory  *factory,
                                GError          **error)
{
  GList *path;
  GList *writable_path;
  GFile *writable_dir = NULL;

  path          = clay_data_factory_get_data_path          (factory);
  writable_path = clay_data_factory_get_data_path_writable (factory);

  if (writable_path)
    {
      GList    *list;
      gboolean  found_any = FALSE;

      for (list = writable_path; list; list = g_list_next (list))
        {
          GList *found = g_list_find_custom (path, list->data,
                                             (GCompareFunc) clay_file_compare);
          if (found)
            {
              GFile  *dir         = found->data;
              GError *mkdir_error = NULL;

              found_any = TRUE;

              /* We always try and create but do not check the error code
               * because some errors might be non-fatale. E.g.
               * G_IO_ERROR_EXISTS may mean either that the folder exists
               * (good) or that a file of another type exists (bad).
               * I am also unsure if G_IO_ERROR_NOT_SUPPORTED might happen in
               * cases where creating directories is forbidden but one might
               * already exist.
               * So in the end, we create then check the file type.
               */
              if (! g_file_make_directory_with_parents (dir, NULL, &mkdir_error) &&
                  g_file_query_file_type (dir, G_FILE_QUERY_INFO_NONE,
                                          NULL) != G_FILE_TYPE_DIRECTORY)
                {
                  /*  error out only if this is the last chance  */
                  if (! list->next)
                    {
                      if (mkdir_error)
                        g_set_error (error, CLAY_DATA_ERROR, 0,
                                     _("You have a writable data folder "
                                       "configured (%s), but this folder could "
                                       "not be created: \"%s\"\n\n"
                                       "Please check your configuration in the "
                                       "Preferences dialog's 'Folders' section."),
                                     clay_file_get_utf8_name (dir),
                                     mkdir_error->message);
                      else
                        g_set_error (error, CLAY_DATA_ERROR, 0,
                                     _("You have a writable data folder "
                                       "configured (%s), but this folder does "
                                       "not exist. Please create the folder or "
                                       "fix your configuration in the "
                                       "Preferences dialog's 'Folders' section."),
                                     clay_file_get_utf8_name (dir));
                    }

                  g_clear_error (&mkdir_error);
                }
              else
                {
                  writable_dir = g_object_ref (dir);
                  break;
                }
            }
        }

      if (! writable_dir && ! found_any)
        {
          g_set_error (error, CLAY_DATA_ERROR, 0,
                       _("You have a writable data folder configured, but this "
                         "folder is not part of your data search path. You "
                         "probably edited the clayrc file manually, "
                         "please fix it in the Preferences dialog's 'Folders' "
                         "section."));
        }
    }
  else
    {
      g_set_error (error, CLAY_DATA_ERROR, 0,
                   _("You don't have any writable data folder configured."));
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);

  return writable_dir;
}

static gboolean
clay_data_factory_search_in_container (ClayData   *data,
                                       SearchData *search_data)
{
  return clay_data_identify (data, search_data->name, search_data->collection, search_data->is_internal);
}
