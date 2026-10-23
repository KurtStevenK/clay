/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayEarlyRc: pre-parsing of clayrc suitable for use during early
 * initialization, when the clay singleton is not constructed yet
 *
 * Copyright (C) 2017  Jehan <jehan@gimp.org>
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

#include <gio/gio.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "config-types.h"

#include "core/core-types.h"
#include "core/clay-utils.h"

#include "clayearlyrc.h"

enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_SYSTEM_CLAYRC,
  PROP_USER_CLAYRC,
  PROP_LANGUAGE,
#ifdef G_OS_WIN32
  PROP_WIN32_POINTER_INPUT_API,
#endif
};


static void   clay_early_rc_constructed  (GObject      *object);
static void   clay_early_rc_finalize     (GObject      *object);
static void   clay_early_rc_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   clay_early_rc_get_property (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


/* Just use ClayConfig interface's default implementation which will
 * fill the properties. */
G_DEFINE_TYPE_WITH_CODE (ClayEarlyRc, clay_early_rc, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL))

#define parent_class clay_early_rc_parent_class


static void
clay_early_rc_class_init (ClayEarlyRcClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_early_rc_constructed;
  object_class->finalize     = clay_early_rc_finalize;
  object_class->set_property = clay_early_rc_set_property;
  object_class->get_property = clay_early_rc_get_property;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SYSTEM_CLAYRC,
                                   g_param_spec_object ("system-clayrc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USER_CLAYRC,
                                   g_param_spec_object ("user-clayrc",
                                                        NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  CLAY_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language", NULL, NULL, NULL,
                           CLAY_PARAM_STATIC_STRINGS);

#ifdef G_OS_WIN32
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_WIN32_POINTER_INPUT_API,
                         "win32-pointer-input-api", NULL, NULL,
                         CLAY_TYPE_WIN32_POINTER_INPUT_API,
                         CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK,
                         CLAY_PARAM_STATIC_STRINGS |
                         CLAY_CONFIG_PARAM_RESTART);
#endif
}

static void
clay_early_rc_init (ClayEarlyRc *rc)
{
}

static void
clay_early_rc_constructed (GObject *object)
{
  ClayEarlyRc *rc    = CLAY_EARLY_RC (object);
  GError      *error = NULL;

  if (rc->verbose)
    g_print ("Parsing '%s' for configuration data required during early initialization.\n",
             clay_file_get_utf8_name (rc->system_clayrc));

  if (! clay_config_deserialize_file (CLAY_CONFIG (rc),
                                      rc->system_clayrc, NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }

  if (rc->verbose)
    g_print ("Parsing '%s' for configuration data required during early initialization.\n",
             clay_file_get_utf8_name (rc->user_clayrc));

  if (! clay_config_deserialize_file (CLAY_CONFIG (rc),
                                      rc->user_clayrc, NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        g_message ("%s", error->message);

      g_clear_error (&error);
    }

  if (rc->verbose)
    {
      if (rc->language)
        g_print ("Language property found: %s.\n", rc->language);
      else
        g_print ("No language property found.\n");
    }
}

static void
clay_early_rc_finalize (GObject *object)
{
  ClayEarlyRc *rc = CLAY_EARLY_RC (object);

  g_clear_object (&rc->system_clayrc);
  g_clear_object (&rc->user_clayrc);

  g_clear_pointer (&rc->language, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_early_rc_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClayEarlyRc *rc = CLAY_EARLY_RC (object);

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

    case PROP_LANGUAGE:
      g_set_str (&rc->language, g_value_get_string (value));
      break;

#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      {
        ClayWin32PointerInputAPI api = g_value_get_enum (value);
        gboolean have_wintab         = clay_win32_have_wintab ();
        gboolean have_windows_ink    = clay_win32_have_windows_ink ();
        gboolean api_is_wintab       = (api == CLAY_WIN32_POINTER_INPUT_API_WINTAB);
        gboolean api_is_windows_ink  = (api == CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK);

        if (api_is_wintab && !have_wintab && have_windows_ink)
          {
            rc->win32_pointer_input_api = CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK;
          }
        else if (api_is_windows_ink && !have_windows_ink && have_wintab)
          {
            rc->win32_pointer_input_api = CLAY_WIN32_POINTER_INPUT_API_WINTAB;
          }
        else
          {
            rc->win32_pointer_input_api = api;
          }
      }
      break;
#endif

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_early_rc_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClayEarlyRc *rc = CLAY_EARLY_RC (object);

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

    case PROP_LANGUAGE:
      g_value_set_string (value, rc->language);
      break;

#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      g_value_set_enum (value, rc->win32_pointer_input_api);
      break;
#endif

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_early_rc_new:
 * @system_clayrc: the name of the system-wide clayrc file or %NULL to
 *                 use the standard location
 * @user_clayrc:   the name of the user clayrc file or %NULL to use the
 *                 standard location
 * @verbose:       enable console messages about loading the preferences
 *
 * Creates a new ClayEarlyRc object which looks for some configuration
 * data required during early initialization steps
 *
 * Returns: the new #ClayEarlyRc.
 */
ClayEarlyRc *
clay_early_rc_new (GFile    *system_clayrc,
                   GFile    *user_clayrc,
                   gboolean  verbose)
{
  ClayEarlyRc *rc;

  g_return_val_if_fail (system_clayrc == NULL || G_IS_FILE (system_clayrc),
                        NULL);
  g_return_val_if_fail (user_clayrc == NULL || G_IS_FILE (user_clayrc),
                        NULL);

  rc = g_object_new (CLAY_TYPE_EARLY_RC,
                     "verbose",       verbose,
                     "system-clayrc", system_clayrc,
                     "user-clayrc",   user_clayrc,
                     NULL);

  return rc;
}

/**
 * clay_early_rc_get_language:
 * @rc: a #ClayEarlyRc object.
 *
 * This function looks up the language set in `clayrc`.
 *
 * Returns: a newly allocated string representing the language or
 *               %NULL if the key couldn't be found.
 **/
gchar *
clay_early_rc_get_language (ClayEarlyRc *rc)
{
  return rc->language ? g_strdup (rc->language) : NULL;
}

#ifdef G_OS_WIN32

/**
 * clay_early_rc_get_win32_pointer_input_api:
 * @rc: a #ClayEarlyRc object.
 *
 * This function looks up the win32-specific pointer input API
 * set in `clayrc`.
 *
 * Returns: the selected win32-specific pointer input API
 **/
ClayWin32PointerInputAPI
clay_early_rc_get_win32_pointer_input_api (ClayEarlyRc *rc)
{
  return rc->win32_pointer_input_api;
}

#endif
