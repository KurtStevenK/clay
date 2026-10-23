/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claycontainer-filter.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "claycontainer.h"
#include "claycontainer-filter.h"
#include "claylist.h"


typedef struct
{
  ClayObjectFilterFunc   filter;
  ClayContainer         *container;
  gpointer               user_data;
} ClayContainerFilterContext;


static void
clay_container_filter_foreach_func (ClayObject                 *object,
                                    ClayContainerFilterContext *context)
{
  if (context->filter (object, context->user_data))
    clay_container_add (context->container, object);
}

/**
 * clay_container_filter:
 * @container: a #ClayContainer to filter
 * @filter: a #ClayObjectFilterFunc
 * @user_data: a pointer passed to @filter
 *
 * Calls the supplied @filter function on each object in @container.
 * A return value of %TRUE is interpreted as a match.
 *
 * Returns: a weak #ClayContainer filled with matching objects.
 **/
ClayContainer *
clay_container_filter (ClayContainer        *container,
                       ClayObjectFilterFunc  filter,
                       gpointer              user_data)
{
  ClayContainer              *result;
  ClayContainerFilterContext  context;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (filter != NULL, NULL);

  result =
    g_object_new (G_TYPE_FROM_INSTANCE (container),
                  "child-type", clay_container_get_child_type (container),
                  "policy",     CLAY_CONTAINER_POLICY_WEAK,
                  NULL);

  context.filter    = filter;
  context.container = result;
  context.user_data = user_data;

  clay_container_foreach (container,
                          (GFunc) clay_container_filter_foreach_func,
                          &context);

  /*  This is somewhat ugly, but it keeps lists in the same order.  */
  if (CLAY_IS_LIST (result))
    clay_list_reverse (CLAY_LIST (result));

  return result;
}


static gboolean
clay_object_filter_by_name (ClayObject   *object,
                            const GRegex *regex)
{
  return g_regex_match (regex, clay_object_get_name (object), 0, NULL);
}

/**
 * clay_container_filter_by_name:
 * @container: a #ClayContainer to filter
 * @regexp: a regular expression (as a %NULL-terminated string)
 * @error: error location to report errors or %NULL
 *
 * This function performs a case-insensitive regular expression search
 * on the names of the ClayObjects in @container.
 *
 * Returns: a weak #ClayContainer filled with matching objects.
 **/
ClayContainer *
clay_container_filter_by_name (ClayContainer  *container,
                               const gchar    *regexp,
                               GError        **error)
{
  ClayContainer *result;
  GRegex        *regex;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (regexp != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  regex = g_regex_new (regexp, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, 0,
                       error);

  if (! regex)
    return NULL;

  result =
    clay_container_filter (container,
                           (ClayObjectFilterFunc) clay_object_filter_by_name,
                           regex);

  g_regex_unref (regex);

  return result;
}

gchar **
clay_container_get_filtered_name_array (ClayContainer *container,
                                        const gchar   *regexp)
{
  ClayContainer *weak;
  GError        *error = NULL;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);

  if (regexp == NULL || strlen (regexp) == 0)
    return (clay_container_get_name_array (container));

  weak = clay_container_filter_by_name (container, regexp, &error);

  if (weak)
    {
      gchar **retval = clay_container_get_name_array (weak);

      g_object_unref (weak);

      return retval;
    }
  else
    {
      g_warning ("%s", error->message);
      g_error_free (error);

      return NULL;
    }
}

ClayObject **
clay_container_get_filtered_array (ClayContainer *container,
                                   const gchar   *regexp)
{
  ClayObject    **retval = NULL;
  ClayContainer  *weak   = NULL;
  GError         *error  = NULL;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);

  if (regexp != NULL && strlen (regexp) > 0)
    weak = clay_container_filter_by_name (container, regexp, &error);

  if (error == NULL)
    {
      GList *list;
      GList *iter;
      gint   i;

      list   = CLAY_LIST (weak ? weak : container)->queue->head;
      retval = g_new0 (ClayObject *, g_list_length (list) + 1);
      for (iter = list, i = 0; iter; iter = iter->next, i++)
        retval[i] = iter->data;
    }
  else
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_clear_object (&weak);

  return retval;
}
