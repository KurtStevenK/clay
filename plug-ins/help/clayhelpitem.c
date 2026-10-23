/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The Clay Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libclay.
 */

#include "config.h"

#include <string.h>

#include <glib.h>

#include "clayhelptypes.h"

#include "clayhelpitem.h"

#ifdef DISABLE_NLS
#define _(String)  (String)
#else
#include "libclay/stdplugins-intl.h"
#endif


/*  public functions  */

ClayHelpItem *
clay_help_item_new (const gchar *ref,
                    const gchar *title,
                    const gchar *sort,
                    const gchar *parent)
{
  ClayHelpItem *item = g_slice_new0 (ClayHelpItem);

  item->ref    = g_strdup (ref);
  item->title  = g_strdup (title);
  item->sort   = g_strdup (sort);
  item->parent = g_strdup (parent);

  return item;
}

void
clay_help_item_free (ClayHelpItem *item)
{
  g_free (item->ref);
  g_free (item->title);
  g_free (item->sort);
  g_free (item->parent);

  g_list_free (item->children);

  g_slice_free (ClayHelpItem, item);
}
