/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayRc serialization routines
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

#include <gio/gio.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "config-types.h"

#include "clayrc.h"
#include "clayrc-serialize.h"
#include "clayrc-unknown.h"


static gboolean clay_rc_serialize_properties_diff (ClayConfig       *config,
                                                   ClayConfig       *compare,
                                                   ClayConfigWriter *writer);
static gboolean clay_rc_serialize_unknown_tokens  (ClayConfig       *config,
                                                   ClayConfigWriter *writer);


gboolean
clay_rc_serialize (ClayConfig       *config,
                   ClayConfigWriter *writer,
                   gpointer          data)
{
  if (data && CLAY_IS_RC (data))
    {
      if (! clay_rc_serialize_properties_diff (config, data, writer))
        return FALSE;
    }
  else
    {
      if (! clay_config_serialize_properties (config, writer))
        return FALSE;
    }

  return clay_rc_serialize_unknown_tokens (config, writer);
}

static gboolean
clay_rc_serialize_properties_diff (ClayConfig       *config,
                                   ClayConfig       *compare,
                                   ClayConfigWriter *writer)
{
  GList    *diff;
  GList    *list;
  gboolean  retval = TRUE;

  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (compare), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (config) ==
                        G_TYPE_FROM_INSTANCE (compare), FALSE);

  diff = clay_config_diff (G_OBJECT (config),
                           G_OBJECT (compare), CLAY_CONFIG_PARAM_SERIALIZE);

  for (list = diff; list; list = g_list_next (list))
    {
      GParamSpec *prop_spec = list->data;

      if (! (prop_spec->flags & CLAY_CONFIG_PARAM_SERIALIZE))
        continue;

      if (! clay_config_serialize_property (config, prop_spec, writer))
        {
          retval = FALSE;
          break;
        }
    }

  g_list_free (diff);

  return retval;
}

static void
serialize_unknown_token (const gchar *key,
                         const gchar *value,
                         gpointer     data)
{
  ClayConfigWriter *writer = data;

  clay_config_writer_open (writer, key);
  clay_config_writer_string (writer, value);
  clay_config_writer_close (writer);
}

static gboolean
clay_rc_serialize_unknown_tokens (ClayConfig       *config,
                                  ClayConfigWriter *writer)
{
  g_return_val_if_fail (G_IS_OBJECT (config), FALSE);

  clay_config_writer_linefeed (writer);
  clay_rc_foreach_unknown_token (config, serialize_unknown_token, writer);

  return TRUE;
}
