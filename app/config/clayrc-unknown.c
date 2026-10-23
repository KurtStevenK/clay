/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayRc serialization and deserialization helpers
 * Copyright (C) 2001-2005  Sven Neumann <sven@gimp.org>
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "config-types.h"

#include "clayrc-unknown.h"


/*
 * Code to store and lookup unknown tokens (string key/value pairs).
 */

#define CLAY_RC_UNKNOWN_TOKENS "clay-rc-unknown-tokens"

typedef struct
{
  gchar *key;
  gchar *value;
} ClayConfigToken;

static void  clay_rc_destroy_unknown_tokens (GSList *unknown_tokens);


/**
 * clay_rc_add_unknown_token:
 * @config: a #GObject.
 * @key: a nul-terminated string to identify the value.
 * @value: a nul-terminated string representing the value.
 *
 * This function adds arbitrary key/value pairs to a GObject.  It's
 * purpose is to attach additional data to a #ClayConfig object that
 * can be stored along with the object properties when serializing the
 * object to a configuration file. Please note however that the
 * default clay_config_serialize() implementation does not serialize
 * unknown tokens.
 *
 * If you want to remove a key/value pair from the object, call this
 * function with a %NULL @value.
 **/
void
clay_rc_add_unknown_token (ClayConfig  *config,
                           const gchar *key,
                           const gchar *value)
{
  ClayConfigToken *token;
  GSList          *unknown_tokens;
  GSList          *last;
  GSList          *list;

  g_return_if_fail (CLAY_IS_CONFIG (config));
  g_return_if_fail (key != NULL);

  unknown_tokens = (GSList *) g_object_get_data (G_OBJECT (config),
                                                 CLAY_RC_UNKNOWN_TOKENS);

  for (last = NULL, list = unknown_tokens;
       list;
       last = list, list = g_slist_next (list))
    {
      token = (ClayConfigToken *) list->data;

      if (strcmp (token->key, key) == 0)
        {
          g_free (token->value);

          if (value)
            {
              token->value = g_strdup (value);
            }
          else
            {
              g_free (token->key);

              unknown_tokens = g_slist_remove (unknown_tokens, token);
              g_object_set_data_full (G_OBJECT (config),
                                      CLAY_RC_UNKNOWN_TOKENS,
                                      unknown_tokens,
                     (GDestroyNotify) clay_rc_destroy_unknown_tokens);
            }

          return;
        }
    }

  if (!value)
    return;

  token = g_slice_new (ClayConfigToken);
  token->key   = g_strdup (key);
  token->value = g_strdup (value);

  if (last)
    {
      last = g_slist_last (g_slist_append (last, token));
    }
  else
    {
      unknown_tokens = g_slist_append (NULL, token);

      g_object_set_data_full (G_OBJECT (config),
                              CLAY_RC_UNKNOWN_TOKENS,
                              unknown_tokens,
                              (GDestroyNotify) clay_rc_destroy_unknown_tokens);
    }
}

/**
 * clay_rc_lookup_unknown_token:
 * @config: a #GObject.
 * @key: a nul-terminated string to identify the value.
 *
 * This function retrieves data that was previously attached using
 * clay_rc_add_unknown_token(). You should not free or modify
 * the returned string.
 *
 * Returns: a pointer to a constant string.
 **/
const gchar *
clay_rc_lookup_unknown_token (ClayConfig  *config,
                              const gchar *key)
{
  GSList          *unknown_tokens;
  GSList          *list;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  unknown_tokens = g_object_get_data (G_OBJECT (config),
                                      CLAY_RC_UNKNOWN_TOKENS);

  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      ClayConfigToken *token = list->data;

      if (strcmp (token->key, key) == 0)
        return token->value;
    }

  return NULL;
}

/**
 * clay_rc_foreach_unknown_token:
 * @config: a #GObject.
 * @func: a function to call for each key/value pair.
 * @user_data: data to pass to @func.
 *
 * Calls @func for each key/value stored with the @config using
 * clay_rc_add_unknown_token().
 **/
void
clay_rc_foreach_unknown_token (ClayConfig            *config,
                               ClayConfigForeachFunc  func,
                               gpointer               user_data)
{
  GSList *unknown_tokens;
  GSList *list;

  g_return_if_fail (CLAY_IS_CONFIG (config));
  g_return_if_fail (func != NULL);

  unknown_tokens = g_object_get_data (G_OBJECT (config),
                                      CLAY_RC_UNKNOWN_TOKENS);

  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      ClayConfigToken *token = list->data;

      func (token->key, token->value, user_data);
    }
}

static void
clay_rc_destroy_unknown_tokens (GSList *unknown_tokens)
{
  GSList *list;

  for (list = unknown_tokens; list; list = g_slist_next (list))
    {
      ClayConfigToken *token = list->data;

      g_free (token->key);
      g_free (token->value);
      g_slice_free (ClayConfigToken, token);
    }

  g_slist_free (unknown_tokens);
}
