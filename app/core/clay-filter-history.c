/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * clay-filter-history.c
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

#include "config/claycoreconfig.h"

#include "clay.h"
#include "clay-filter-history.h"

#include "pdb/clayprocedure.h"


/*  local function prototypes  */

static gint   clay_filter_history_compare (ClayProcedure *proc1,
                                           ClayProcedure *proc2);


/*  public functions  */

gint
clay_filter_history_size (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  return MAX (1, clay->config->filter_history_size);
}

gint
clay_filter_history_length (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  return g_list_length (clay->filter_history);
}

ClayProcedure *
clay_filter_history_nth (Clay  *clay,
                         gint   n)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_list_nth_data (clay->filter_history, n);
}

void
clay_filter_history_add (Clay          *clay,
                         ClayProcedure *procedure)
{
  GList *link;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  /* return early if the procedure is already at the top */
  if (clay->filter_history &&
      clay_filter_history_compare (clay->filter_history->data, procedure) == 0)
    return;

  /* ref new first then unref old, they might be the same */
  g_object_ref (procedure);

  link = g_list_find_custom (clay->filter_history, procedure,
                             (GCompareFunc) clay_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      clay->filter_history = g_list_delete_link (clay->filter_history, link);
    }

  clay->filter_history = g_list_prepend (clay->filter_history, procedure);

  link = g_list_nth (clay->filter_history, clay_filter_history_size (clay));

  if (link)
    {
      g_object_unref (link->data);
      clay->filter_history = g_list_delete_link (clay->filter_history, link);
    }

  clay_filter_history_changed (clay);
}

void
clay_filter_history_remove (Clay          *clay,
                            ClayProcedure *procedure)
{
  GList *link;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  link = g_list_find_custom (clay->filter_history, procedure,
                             (GCompareFunc) clay_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      clay->filter_history = g_list_delete_link (clay->filter_history, link);

      clay_filter_history_changed (clay);
    }
}

void
clay_filter_history_clear (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->filter_history)
    {
      g_list_free_full (clay->filter_history, (GDestroyNotify) g_object_unref);
      clay->filter_history = NULL;

      clay_filter_history_changed (clay);
    }
}


/*  private functions  */

static gint
clay_filter_history_compare (ClayProcedure *proc1,
                             ClayProcedure *proc2)
{
  /*  the procedures can have the same name, but could still be two
   *  different filters using the same operation, so also compare
   *  their menu labels
   */
  return (clay_procedure_name_compare (proc1, proc2) ||
          strcmp (clay_procedure_get_menu_label (proc1),
                  clay_procedure_get_menu_label (proc2)));
}
