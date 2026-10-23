/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

/* This file contains the definition of the size unit objects. The
 * factor of the units is relative to inches (which have a factor of 1).
 */

#include "config.h"

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clayunit.h"

#include "clay-intl.h"


/* public functions */

ClayUnit *
_clay_unit_new (Clay        *clay,
                const gchar *name,
                gdouble      factor,
                gint         digits,
                const gchar *symbol,
                const gchar *abbreviation)
{
  ClayUnit *unit;
  gint      unit_id;

  unit_id = CLAY_UNIT_END + g_list_length (clay->user_units);
  unit = g_object_new (CLAY_TYPE_UNIT,
                       "id",           unit_id,
                       "name",         name,
                       "factor",       factor,
                       "digits",       digits,
                       "symbol",       symbol,
                       "abbreviation", abbreviation,
                       NULL);

  clay->user_units = g_list_append (clay->user_units, unit);
  clay_unit_set_deletion_flag (unit, TRUE);

  return unit;
}

/* Unlike _clay_unit_new(), this function will first try to find a
 * matching user unit, and will return it if found. Otherwise it creates
 * a new unit set with deletion flag.
 *
 * Note that @plural_form is only used if @before_xcf_21 is %TRUE.
 *
 * The returned unit (whether new or not) belongs to @clay and should
 * not be freed.
 */
ClayUnit *
_clay_unit_get (Clay        *clay,
                const gchar *name,
                gdouble      factor,
                gint         digits,
                const gchar *symbol,
                const gchar *abbreviation,
                gboolean     before_xcf_21,
                const gchar *plural_form)
{
  ClayUnit *unit;
  GList    *iter;

  for (iter = clay->user_units; iter; iter = iter->next)
    {
      unit = iter->data;
      /* if the factor and the name match some unit in unitrc,
       * use the unitrc unit
       */
      if (ABS (clay_unit_get_factor (unit) - factor) < 1e-5 &&
          (strcmp (name, clay_unit_get_name (unit)) == 0 ||
           (before_xcf_21 &&
            g_strcmp0 (plural_form, clay_unit_get_name (unit)) == 0)))
        {
          break;
        }
    }

  if (iter == NULL)
    unit = _clay_unit_new (clay,
                           plural_form && strlen (plural_form) > 0 ? plural_form : name,
                           factor, digits, symbol, abbreviation);

  return unit;
}
