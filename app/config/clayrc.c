/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayRc, the object for CLAYs user configuration file clayrc.
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include <gio/gio.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "config-types.h"

#include "clayconfig-file.h"
#include "clayrc.h"
#include "clayrc-deserialize.h"
#include "clayrc-serialize.h"
#include "clayrc-unknown.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_CLAYRC,
  PROP_USER_CLAYRC
};


static void         clay_rc_config_iface_init (ClayConfigInterface *iface);

static void         clay_rc_dispose           (GObject      *object);
static void         clay_rc_finalize          (GObject      *object);
static void         clay_rc_set_property      (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void         clay_rc_get_property      (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static ClayConfig * clay_rc_duplicate         (ClayConfig   *object);
static gboolean     clay_rc_idle_save         (ClayRc       *rc);
static void         clay_rc_notify            (ClayRc       *rc,
                                               GParamSpec   *param,
                                               gpointer      data);


G_DEFINE_TYPE_WITH_CODE (ClayRc, clay_rc, CLAY_TYPE_PLUGIN_CONFIG,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_rc_config_iface_init))

#define parent_class clay_rc_parent_class


static void
clay_rc_class_init (ClayRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_rc_dispose;
  object_class->finalize     = clay_rc_finalize;
  object_class->set_property = clay_rc_set_property;
  object_class->get_property = clay_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SYSTEM_CLAYRC,
                                   g_param_spec_object ("system-clayrc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_USER_CLAYRC,
                                   g_param_spec_object ("user-clayrc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_rc_init (ClayRc *rc)
{
  rc->autosave     = FALSE;
  rc->save_idle_id = 0;
}

static void
clay_rc_dispose (GObject *object)
{
  ClayRc *rc = CLAY_RC (object);

  if (rc->save_idle_id)
    clay_rc_idle_save (rc);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_rc_finalize (GObject *object)
{
  ClayRc *rc = CLAY_RC (object);

  g_clear_object (&rc->system_clayrc);
  g_clear_object (&rc->user_clayrc);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_rc_set_property (GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  ClayRc *rc = CLAY_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      rc->verbose = g_value_get_boolean (value);
      break;

    case PROP_SYSTEM_CLAYRC:
      g_set_object (&rc->system_clayrc, g_value_get_object (value));
      if (! rc->system_clayrc)
        rc->system_clayrc = clay_sysconf_directory_file ("clayrc", NULL);
      break;

    case PROP_USER_CLAYRC:
      g_set_object (&rc->user_clayrc, g_value_get_object (value));
      if (! rc->user_clayrc)
        rc->user_clayrc = clay_directory_file ("clayrc", NULL);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_rc_get_property (GObject    *object,
                      guint       property_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  ClayRc *rc = CLAY_RC (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, rc->verbose);
      break;
    case PROP_SYSTEM_CLAYRC:
      g_value_set_object (value, rc->system_clayrc);
      break;
    case PROP_USER_CLAYRC:
      g_value_set_object (value, rc->user_clayrc);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_rc_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_rc_serialize;
  iface->deserialize = clay_rc_deserialize;
  iface->duplicate   = clay_rc_duplicate;
}

static void
clay_rc_duplicate_unknown_token (const gchar *key,
                                 const gchar *value,
                                 gpointer     user_data)
{
  clay_rc_add_unknown_token (CLAY_CONFIG (user_data), key, value);
}

static ClayConfig *
clay_rc_duplicate (ClayConfig *config)
{
  GObject    *clay;
  ClayConfig *dup;

  g_object_get (config, "clay", &clay, NULL);

  dup = g_object_new (CLAY_TYPE_RC,
                      "clay", clay,
                      NULL);

  if (clay)
    g_object_unref (clay);

  clay_config_sync (G_OBJECT (config), G_OBJECT (dup), 0);

  clay_rc_foreach_unknown_token (config,
                                 clay_rc_duplicate_unknown_token, dup);

  return dup;
}

static gboolean
clay_rc_idle_save (ClayRc *rc)
{
  clay_rc_save (rc);

  rc->save_idle_id = 0;

  return FALSE;
}

static void
clay_rc_notify (ClayRc     *rc,
                GParamSpec *param,
                gpointer    data)
{
  if (!rc->autosave)
    return;

  if (!rc->save_idle_id)
    rc->save_idle_id = g_idle_add ((GSourceFunc) clay_rc_idle_save, rc);
}

/**
 * clay_rc_new:
 * @clay:          a #Clay
 * @system_clayrc: the name of the system-wide clayrc file or %NULL to
 *                 use the standard location
 * @user_clayrc:   the name of the user clayrc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading and saving
 *
 * Creates a new ClayRc object and loads the system-wide and the user
 * configuration files.
 *
 * Returns: the new #ClayRc.
 */
ClayRc *
clay_rc_new (GObject  *clay,
             GFile    *system_clayrc,
             GFile    *user_clayrc,
             gboolean  verbose)
{
  ClayRc *rc;

  g_return_val_if_fail (G_IS_OBJECT (clay), NULL);
  g_return_val_if_fail (system_clayrc == NULL || G_IS_FILE (system_clayrc),
                        NULL);
  g_return_val_if_fail (user_clayrc == NULL || G_IS_FILE (user_clayrc),
                        NULL);

  rc = g_object_new (CLAY_TYPE_RC,
                     "clay",          clay,
                     "verbose",       verbose,
                     "system-clayrc", system_clayrc,
                     "user-clayrc",   user_clayrc,
                     NULL);

  clay_rc_load_system (rc);
  clay_rc_load_user (rc);

  return rc;
}

void
clay_rc_load_system (ClayRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             clay_file_get_utf8_name (rc->system_clayrc));

  if (! clay_config_deserialize_file (CLAY_CONFIG (rc),
                                      rc->system_clayrc, NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }
}

void
clay_rc_load_user (ClayRc *rc)
{
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_RC (rc));

  if (rc->verbose)
    g_print ("Parsing '%s'\n",
             clay_file_get_utf8_name (rc->user_clayrc));

  if (! clay_config_deserialize_file (CLAY_CONFIG (rc),
                                      rc->user_clayrc, NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_message ("%s", error->message);

          clay_config_file_backup_on_error (rc->user_clayrc, "clayrc", NULL);
        }

      g_clear_error (&error);
    }
}

void
clay_rc_set_autosave (ClayRc   *rc,
                      gboolean  autosave)
{
  g_return_if_fail (CLAY_IS_RC (rc));

  autosave = autosave ? TRUE : FALSE;

  if (rc->autosave == autosave)
    return;

  if (autosave)
    g_signal_connect (rc, "notify",
                      G_CALLBACK (clay_rc_notify),
                      NULL);
  else
    g_signal_handlers_disconnect_by_func (rc, clay_rc_notify, NULL);

  rc->autosave = autosave;
}


/**
 * clay_rc_query:
 * @rc:  a #ClayRc object.
 * @key: a string used as a key for the lookup.
 *
 * This function looks up @key in the object properties of @rc. If
 * there's a matching property, a string representation of its value
 * is returned. If no property is found, the list of unknown tokens
 * attached to the @rc object is searched.
 *
 * Returns: (nullable): a newly allocated string representing the value or %NULL
 *               if the key couldn't be found.
 **/
gchar *
clay_rc_query (ClayRc      *rc,
               const gchar *key)
{
  GObjectClass  *klass;
  GObject       *rc_object;
  GParamSpec   **property_specs;
  GParamSpec    *prop_spec;
  guint          i, n_property_specs;
  gchar         *retval = NULL;

  g_return_val_if_fail (CLAY_IS_RC (rc), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  rc_object = G_OBJECT (rc);
  klass = G_OBJECT_GET_CLASS (rc);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  if (!property_specs)
    return NULL;

  for (i = 0, prop_spec = NULL; i < n_property_specs && !prop_spec; i++)
    {
      prop_spec = property_specs[i];

      if (! (prop_spec->flags & CLAY_CONFIG_PARAM_SERIALIZE) ||
          strcmp (prop_spec->name, key))
        {
          prop_spec = NULL;
        }
    }

  if (prop_spec)
    {
      GString *str   = g_string_new (NULL);
      GValue   value = G_VALUE_INIT;

      g_value_init (&value, prop_spec->value_type);
      g_object_get_property (rc_object, prop_spec->name, &value);

      if (clay_config_serialize_value (&value, str, FALSE))
        retval = g_string_free (str, FALSE);
      else
        g_string_free (str, TRUE);

      g_value_unset (&value);
    }
  else
    {
      retval = g_strdup (clay_rc_lookup_unknown_token (CLAY_CONFIG (rc), key));
    }

  g_free (property_specs);

  if (!retval)
    {
      const gchar * const path_tokens[] =
      {
        "clay_dir",
        "clay_data_dir",
        "clay_plug_in_dir",
        "clay_plugin_dir",
        "clay_sysconf_dir"
      };

      for (i = 0; !retval && i < G_N_ELEMENTS (path_tokens); i++)
        if (strcmp (key, path_tokens[i]) == 0)
          retval = g_strdup_printf ("${%s}", path_tokens[i]);
    }

  if (retval)
    {
      gchar *tmp = clay_config_path_expand (retval, FALSE, NULL);

      if (tmp)
        {
          g_free (retval);
          retval = tmp;
        }
    }

  return retval;
}

/**
 * clay_rc_set_unknown_token:
 * @clayrc: a #ClayRc object.
 * @token:
 * @value:
 *
 * Calls clay_rc_add_unknown_token() and triggers an idle-save if
 * autosave is enabled on @clayrc.
 **/
void
clay_rc_set_unknown_token (ClayRc      *rc,
                           const gchar *token,
                           const gchar *value)
{
  g_return_if_fail (CLAY_IS_RC (rc));

  clay_rc_add_unknown_token (CLAY_CONFIG (rc), token, value);

  if (rc->autosave)
    clay_rc_notify (rc, NULL, NULL);
}

/**
 * clay_rc_save:
 * @clayrc: a #ClayRc object.
 *
 * Saves any settings that differ from the system-wide defined
 * defaults to the users personal clayrc file.
 **/
void
clay_rc_save (ClayRc *rc)
{
  GObject *clay;
  ClayRc  *global;
  gchar   *header;
  GError  *error = NULL;

  const gchar *top =
    "Clay clayrc\n"
    "\n"
    "This is your personal clayrc file.  Any variable defined in this file "
    "takes precedence over the value defined in the system-wide clayrc: ";
  const gchar *bottom =
    "\n"
    "Most values can be set within Clay by changing some options in "
    "the Preferences dialog.";
  const gchar *footer =
    "end of clayrc";

  g_return_if_fail (CLAY_IS_RC (rc));

  g_object_get (rc, "clay", &clay, NULL);

  global = g_object_new (CLAY_TYPE_RC,
                         "clay", clay,
                         NULL);

  if (clay)
    g_object_unref (clay);

  clay_config_deserialize_file (CLAY_CONFIG (global),
                                rc->system_clayrc, NULL, NULL);

  header = g_strconcat (top, clay_file_get_utf8_name (rc->system_clayrc),
                        bottom, NULL);

  if (rc->verbose)
    g_print ("Writing '%s'\n",
             clay_file_get_utf8_name (rc->user_clayrc));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (rc),
                                       rc->user_clayrc,
                                       header, footer, global,
                                       &error))
    {
      g_message ("%s", error->message);
      g_error_free (error);
    }

  g_free (header);
  g_object_unref (global);
}
