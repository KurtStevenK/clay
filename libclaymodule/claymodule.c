/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymodule.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include "claymodule.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claymodule
 * @title: ClayModule
 * @short_description: A #GTypeModule subclass which implements module
 *                     loading using #GModule.
 * @see_also: #GModule, #GTypeModule
 *
 * #ClayModule is a generic mechanism to dynamically load modules into
 * Clay. It is a #GTypeModule subclass, implementing module loading
 * using #GModule.  #ClayModule does not know which functionality is
 * implemented by the modules, it just provides a framework to get
 * arbitrary #GType implementations loaded from disk.
 **/


enum
{
  PROP_0,
  PROP_AUTO_LOAD,
  PROP_ON_DISK,
  N_PROPS
};
static GParamSpec *obj_props[N_PROPS] = { NULL, };

typedef struct _ClayModulePrivate
{
  GFile           *file;       /* path to the module                       */
  gboolean         auto_load;  /* auto-load the module on creation         */
  gboolean         verbose;    /* verbose error reporting                  */

  ClayModuleInfo  *info;       /* returned values from module_query        */
  ClayModuleState  state;      /* what's happened to the module            */
  gchar           *last_error;

  gboolean         on_disk;    /* TRUE if file still exists                */

  GModule         *module;     /* handle on the module                     */

  ClayModuleQueryFunc     query_module;
  ClayModuleRegisterFunc  register_module;
} ClayModulePrivate;

#define GET_PRIVATE(obj) ((ClayModulePrivate *) clay_module_get_instance_private ((ClayModule *) (obj)))


static void       clay_module_get_property   (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void       clay_module_set_property   (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void       clay_module_finalize       (GObject      *object);

static gboolean   clay_module_load           (GTypeModule  *module);
static void       clay_module_unload         (GTypeModule  *module);

static gboolean   clay_module_open           (ClayModule   *module);
static gboolean   clay_module_close          (ClayModule   *module);
static void       clay_module_set_last_error (ClayModule   *module,
                                              const gchar  *error_str);

static ClayModuleInfo * clay_module_info_new  (guint32               abi_version,
                                               const gchar          *purpose,
                                               const gchar          *author,
                                               const gchar          *version,
                                               const gchar          *copyright,
                                               const gchar          *date);
static ClayModuleInfo * clay_module_info_copy (const ClayModuleInfo *info);
static void             clay_module_info_free (ClayModuleInfo       *info);


G_DEFINE_TYPE_WITH_PRIVATE (ClayModule, clay_module, G_TYPE_TYPE_MODULE)

#define parent_class clay_module_parent_class


static void
clay_module_class_init (ClayModuleClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  object_class->set_property = clay_module_set_property;
  object_class->get_property = clay_module_get_property;
  object_class->finalize = clay_module_finalize;

  obj_props[PROP_AUTO_LOAD] = g_param_spec_boolean ("auto-load", "auto-load", "auto-load",
                                                    FALSE, CLAY_PARAM_READWRITE);
  obj_props[PROP_ON_DISK] = g_param_spec_boolean ("on-disk", "on-disk", "on-disk",
                                                   FALSE, CLAY_PARAM_READABLE);
  g_object_class_install_properties (object_class, N_PROPS, obj_props);

  module_class->load     = clay_module_load;
  module_class->unload   = clay_module_unload;
}

static void
clay_module_init (ClayModule *module)
{
  GET_PRIVATE (module)->state = CLAY_MODULE_STATE_ERROR;
}

static void
clay_module_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ClayModule *module = CLAY_MODULE (object);

  switch (property_id)
    {
    case PROP_AUTO_LOAD:
      clay_module_set_auto_load (module, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_module_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ClayModule *module = CLAY_MODULE (object);

  switch (property_id)
    {
    case PROP_AUTO_LOAD:
      g_value_set_boolean (value, clay_module_get_auto_load (module));
      break;

    case PROP_ON_DISK:
      g_value_set_boolean (value, clay_module_is_on_disk (module));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_module_finalize (GObject *object)
{
  ClayModule        *module = CLAY_MODULE (object);
  ClayModulePrivate *priv   = GET_PRIVATE (module);

  g_clear_object  (&priv->file);
  g_clear_pointer (&priv->info, clay_module_info_free);
  g_clear_pointer (&priv->last_error, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_module_load (GTypeModule *module)
{
  ClayModule        *clay_module = CLAY_MODULE (module);
  ClayModulePrivate *priv        = GET_PRIVATE (clay_module);
  gpointer    func;

  g_return_val_if_fail (priv->file != NULL, FALSE);
  g_return_val_if_fail (priv->module == NULL, FALSE);

  if (priv->verbose)
    g_print ("Loading module '%s'\n",
             clay_file_get_utf8_name (priv->file));

  if (! clay_module_open (clay_module))
    return FALSE;

  if (! clay_module_query_module (clay_module))
    return FALSE;

  /* find the clay_module_register symbol */
  if (! g_module_symbol (priv->module, "clay_module_register",
                         &func))
    {
      clay_module_set_last_error (clay_module,
                                  "Missing clay_module_register() symbol");

      g_message (_("Module '%s' load error: %s"),
                 clay_file_get_utf8_name (priv->file),
                 priv->last_error);

      clay_module_close (clay_module);

      priv->state = CLAY_MODULE_STATE_ERROR;

      return FALSE;
    }

  priv->register_module = func;

  if (! priv->register_module (module))
    {
      clay_module_set_last_error (clay_module,
                                  "clay_module_register() returned FALSE");

      g_message (_("Module '%s' load error: %s"),
                 clay_file_get_utf8_name (priv->file),
                 priv->last_error);

      clay_module_close (clay_module);

      priv->state = CLAY_MODULE_STATE_LOAD_FAILED;

      return FALSE;
    }

  priv->state = CLAY_MODULE_STATE_LOADED;

  return TRUE;
}

static void
clay_module_unload (GTypeModule *module)
{
  ClayModule        *clay_module = CLAY_MODULE (module);
  ClayModulePrivate *priv        = GET_PRIVATE (clay_module);

  g_return_if_fail (priv->module != NULL);

  if (priv->verbose)
    g_print ("Unloading module '%s'\n",
             clay_file_get_utf8_name (priv->file));

  clay_module_close (clay_module);
}


/*  public functions  */

/**
 * clay_module_new:
 * @file:      A #GFile pointing to a loadable module.
 * @auto_load: Pass %TRUE to exclude this module from auto-loading.
 * @verbose:   Pass %TRUE to enable debugging output.
 *
 * Creates a new #ClayModule instance.
 *
 * Returns: The new #ClayModule object.
 **/
ClayModule *
clay_module_new (GFile    *file,
                 gboolean  auto_load,
                 gboolean  verbose)
{
  ClayModule        *module;
  ClayModulePrivate *priv;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (g_file_is_native (file), NULL);

  module = g_object_new (CLAY_TYPE_MODULE, NULL);
  priv   = GET_PRIVATE (module);

  priv->file      = g_object_ref (file);
  priv->auto_load = auto_load ? TRUE : FALSE;
  priv->verbose   = verbose ? TRUE : FALSE;

  if (priv->auto_load)
    {
      if (clay_module_load (G_TYPE_MODULE (module)))
        clay_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
        g_print ("Skipping module '%s'\n",
                 clay_file_get_utf8_name (file));

      priv->state = CLAY_MODULE_STATE_NOT_LOADED;
    }

  return module;
}

/**
 * clay_module_get_file:
 * @module: A #ClayModule
 *
 * Returns #GFile of the @module,
 *
 * Returns: (transfer none): The @module's #GFile.
 *
 * Since: 3.0
 **/
GFile *
clay_module_get_file (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->file;
}

/**
 * clay_module_set_auto_load:
 * @module:    A #ClayModule
 * @auto_load: Pass %FALSE to exclude this module from auto-loading
 *
 * Sets the @auto_load property of the module
 *
 * Since: 3.0
 **/
void
clay_module_set_auto_load (ClayModule *module,
                           gboolean    auto_load)
{
  ClayModulePrivate *priv;

  g_return_if_fail (CLAY_IS_MODULE (module));

  priv = GET_PRIVATE (module);

  if (auto_load != priv->auto_load)
    {
      priv->auto_load = auto_load;

      g_object_notify_by_pspec (G_OBJECT (module), obj_props[PROP_AUTO_LOAD]);
    }
}

/**
 * clay_module_get_auto_load:
 * @module: A #ClayModule
 *
 * Returns whether this @module in automatically loaded at startup.
 *
 * Returns: The @module's 'auto_load' property.
 *
 * Since: 3.0
 **/
gboolean
clay_module_get_auto_load (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), FALSE);

  return GET_PRIVATE (module)->auto_load;
}

/**
 * clay_module_is_on_disk:
 * @module: A #ClayModule
 *
 * Returns: Whether the @module is present on diak.
 *
 * Since: 3.0
 **/
gboolean
clay_module_is_on_disk (ClayModule *module)
{
  ClayModulePrivate *priv;
  gboolean           old_on_disk;

  g_return_val_if_fail (CLAY_IS_MODULE (module), FALSE);

  priv = GET_PRIVATE (module);

  old_on_disk = priv->on_disk;

  priv->on_disk =
    (g_file_query_file_type (priv->file,
                             G_FILE_QUERY_INFO_NONE, NULL) ==
     G_FILE_TYPE_REGULAR);

  if (priv->on_disk != old_on_disk)
    g_object_notify_by_pspec (G_OBJECT (module), obj_props[PROP_ON_DISK]);

  return priv->on_disk;
}

/**
 * clay_module_is_loaded:
 * @module: A #ClayModule
 *
 * Returns: Whether the @module is currently loaded.
 *
 * Since: 3.0
 **/
gboolean
clay_module_is_loaded (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), FALSE);

  return GET_PRIVATE (module)->module != NULL;
}

/**
 * clay_module_get_info:
 * @module: A #ClayModule
 *
 * Returns: (transfer none): The @module's #ClayModuleInfo as provided
 *          by the actual module, or %NULL.
 *
 * Since: 3.0
 **/
const ClayModuleInfo *
clay_module_get_info (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->info;
}

/**
 * clay_module_get_state:
 * @module: A #ClayModule
 *
 * Returns: The @module's state.
 *
 * Since: 3.0
 **/
ClayModuleState
clay_module_get_state (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), CLAY_MODULE_STATE_ERROR);

  return GET_PRIVATE (module)->state;
}

/**
 * clay_module_get_last_error:
 * @module: A #ClayModule
 *
 * Returns: The @module's last error message.
 *
 * Since: 3.0
 **/
const gchar *
clay_module_get_last_error (ClayModule *module)
{
  g_return_val_if_fail (CLAY_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->last_error;
}

/**
 * clay_module_query_module:
 * @module: A #ClayModule.
 *
 * Queries the module without actually registering any of the types it
 * may implement. After successful query, clay_module_get_info() can be
 * used to get further about the module.
 *
 * Returns: %TRUE on success.
 **/
gboolean
clay_module_query_module (ClayModule *module)
{
  const ClayModuleInfo *info;
  ClayModulePrivate    *priv;
  gboolean              close_module = FALSE;
  gpointer              func;

  g_return_val_if_fail (CLAY_IS_MODULE (module), FALSE);

  priv = GET_PRIVATE (module);

  if (! priv->module)
    {
      if (! clay_module_open (module))
        return FALSE;

      close_module = TRUE;
    }

  /* find the clay_module_query symbol */
  if (! g_module_symbol (priv->module, "clay_module_query", &func))
    {
      clay_module_set_last_error (module,
                                  "Missing clay_module_query() symbol");

      g_message (_("Module '%s' load error: %s"),
                 clay_file_get_utf8_name (priv->file),
                 priv->last_error);

      clay_module_close (module);

      priv->state = CLAY_MODULE_STATE_ERROR;
      return FALSE;
    }

  priv->query_module = func;

  g_clear_pointer (&priv->info, clay_module_info_free);

  info = priv->query_module (G_TYPE_MODULE (module));

  if (! info || info->abi_version != CLAY_MODULE_ABI_VERSION)
    {
      clay_module_set_last_error (module,
                                  info ?
                                  "module ABI version does not match" :
                                  "clay_module_query() returned NULL");

      g_message (_("Module '%s' load error: %s"),
                 clay_file_get_utf8_name (priv->file),
                 priv->last_error);

      clay_module_close (module);

      priv->state = CLAY_MODULE_STATE_ERROR;
      return FALSE;
    }

  priv->info = clay_module_info_copy (info);

  if (close_module)
    return clay_module_close (module);

  return TRUE;
}

/**
 * clay_module_error_quark:
 *
 * This function is never called directly. Use CLAY_MODULE_ERROR() instead.
 *
 * Returns: the #GQuark that defines the Clay module error domain.
 *
 * Since: 2.8
 **/
GQuark
clay_module_error_quark (void)
{
  return g_quark_from_static_string ("clay-module-error-quark");
}


/*  private functions  */

static gboolean
clay_module_open (ClayModule *module)
{
  ClayModulePrivate *priv = GET_PRIVATE (module);
  gchar             *path = g_file_get_path (priv->file);

  priv->module = g_module_open (path, 0);

  g_free (path);

  if (! priv->module)
    {
      priv->state = CLAY_MODULE_STATE_ERROR;
      clay_module_set_last_error (module, g_module_error ());

      g_message (_("Module '%s' load error: %s"),
                 clay_file_get_utf8_name (priv->file),
                 priv->last_error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
clay_module_close (ClayModule *module)
{
  ClayModulePrivate *priv = GET_PRIVATE (module);

  g_module_close (priv->module); /* FIXME: error handling */
  priv->module          = NULL;
  priv->query_module    = NULL;
  priv->register_module = NULL;

  priv->state = CLAY_MODULE_STATE_NOT_LOADED;

  return TRUE;
}

static void
clay_module_set_last_error (ClayModule  *module,
                            const gchar *error_str)
{
  ClayModulePrivate *priv = GET_PRIVATE (module);

  if (priv->last_error)
    g_free (priv->last_error);

  priv->last_error = g_strdup (error_str);
}


/*  ClayModuleInfo functions  */

/**
 * clay_module_info_new:
 * @abi_version: The #CLAY_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * Creates a newly allocated #ClayModuleInfo struct.
 *
 * Returns: The new #ClayModuleInfo struct.
 **/
static ClayModuleInfo *
clay_module_info_new (guint32      abi_version,
                      const gchar *purpose,
                      const gchar *author,
                      const gchar *version,
                      const gchar *copyright,
                      const gchar *date)
{
  ClayModuleInfo *info = g_slice_new0 (ClayModuleInfo);

  info->abi_version = abi_version;
  info->purpose     = g_strdup (purpose);
  info->author      = g_strdup (author);
  info->version     = g_strdup (version);
  info->copyright   = g_strdup (copyright);
  info->date        = g_strdup (date);

  return info;
}

/**
 * clay_module_info_copy:
 * @info: The #ClayModuleInfo struct to copy.
 *
 * Copies a #ClayModuleInfo struct.
 *
 * Returns: The new copy.
 **/
static ClayModuleInfo *
clay_module_info_copy (const ClayModuleInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return clay_module_info_new (info->abi_version,
                               info->purpose,
                               info->author,
                               info->version,
                               info->copyright,
                               info->date);
}

/**
 * clay_module_info_free:
 * @info: The #ClayModuleInfo struct to free
 *
 * Frees the passed #ClayModuleInfo.
 **/
static void
clay_module_info_free (ClayModuleInfo *info)
{
  g_return_if_fail (info != NULL);

  g_free (info->purpose);
  g_free (info->author);
  g_free (info->version);
  g_free (info->copyright);
  g_free (info->date);

  g_slice_free (ClayModuleInfo, info);
}
