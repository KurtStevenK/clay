/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayidtable.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "core-types.h"

#include "clay-memsize.h"
#include "clayidtable.h"



struct _ClayIdTablePrivate
{
  GHashTable *id_table;
  gint        next_id;
};


static void    clay_id_table_finalize    (GObject    *object);
static gint64  clay_id_table_get_memsize (ClayObject *object,
                                          gint64     *gui_size);

static gboolean clay_id_table_exists     (ClayIdTable *id_table,
                                          gint         id);

G_DEFINE_TYPE_WITH_PRIVATE (ClayIdTable, clay_id_table, CLAY_TYPE_OBJECT)

#define parent_class clay_id_table_parent_class


static void
clay_id_table_class_init (ClayIdTableClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_id_table_finalize;

  clay_object_class->get_memsize = clay_id_table_get_memsize;
}

static void
clay_id_table_init (ClayIdTable *id_table)
{
  id_table->priv = clay_id_table_get_instance_private (id_table);

  id_table->priv->id_table = g_hash_table_new (g_direct_hash, NULL);
  id_table->priv->next_id  = CLAY_ID_TABLE_START_ID;
}

static void
clay_id_table_finalize (GObject *object)
{
  ClayIdTable *id_table = CLAY_ID_TABLE (object);

  g_clear_pointer (&id_table->priv->id_table, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_id_table_get_memsize (ClayObject *object,
                           gint64     *gui_size)
{
  ClayIdTable *id_table = CLAY_ID_TABLE (object);
  gint64       memsize  = 0;

  memsize += clay_g_hash_table_get_memsize (id_table->priv->id_table, 0);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/* Public Functions */

/**
 * clay_id_table_new:
 *
 * Returns: A new #ClayIdTable.
 **/
ClayIdTable *
clay_id_table_new (void)
{
  return g_object_new (CLAY_TYPE_ID_TABLE, NULL);
}

/**
 * clay_id_table_insert:
 * @id_table: A #ClayIdTable
 * @data: Data to insert and assign an id to
 *
 * Insert data in the id table. The data will get an unused ID assigned
 * to it that can be used to later lookup the data.
 *
 * Returns: The assigned ID.
 **/
gint
clay_id_table_insert (ClayIdTable *id_table,
                      gpointer     data)
{
  gint new_id;
  gint start_id;

  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), 0);

  start_id = id_table->priv->next_id;

  do
    {
      new_id = id_table->priv->next_id;

      if (id_table->priv->next_id == CLAY_ID_TABLE_END_ID)
        id_table->priv->next_id = CLAY_ID_TABLE_START_ID;
      else
        id_table->priv->next_id++;

      if (start_id == id_table->priv->next_id)
        {
          /* We looped once over all used ids. Very unlikely to happen.
             And if it does, there is probably not much to be done.
             It is just good design not to allow a theoretical infinite loop. */
          g_error ("%s: out of ids!", G_STRFUNC);
          break;
        }
    }
  while (clay_id_table_exists (id_table, new_id));

  return clay_id_table_insert_with_id (id_table, new_id, data);
}

/**
 * clay_id_table_insert_with_id:
 * @id_table: A #ClayIdTable.
 * @id:       The ID to use. Must be greater than 0.
 * @data:     The data to associate with @id.
 *
 * Insert data in the id table with a specific ID. If data already
 * exists with the given ID, this function fails.
 *
 * Returns: The used ID if successful, -1 if it was already in use.
 **/
gint
clay_id_table_insert_with_id (ClayIdTable *id_table, gint id, gpointer data)
{
  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), 0);
  g_return_val_if_fail (id > 0, 0);

  if (clay_id_table_lookup (id_table, id))
    return -1;

  g_hash_table_insert (id_table->priv->id_table, GINT_TO_POINTER (id), data);

  return id;
}

/**
 * clay_id_table_replace:
 * @id_table: A #ClayIdTable.
 * @id:       The ID to use. Must be greater than 0.
 * @data:     The data to insert/replace.
 *
 * Replaces (if an item with the given ID exists) or inserts a new
 * entry in the id table.
 **/
void
clay_id_table_replace (ClayIdTable *id_table, gint id, gpointer data)
{
  g_return_if_fail (CLAY_IS_ID_TABLE (id_table));
  g_return_if_fail (id > 0);

  g_hash_table_replace (id_table->priv->id_table, GINT_TO_POINTER (id), data);
}

/**
 * clay_id_table_reserve:
 * @id_table: A #ClayIdTable
 * @id:       An unused ID to reserve.
 *
 * Reserve an ID for data which doesn't exist yet. The reservation may
 * fail so you should check the return value, unless you know for a fact
 * that @id cannot be taken yet.
 *
 * Returns: %TRUE if the reservation succeeded; FALSE is the @id is
 * already taken or reserved.
 **/
gboolean
clay_id_table_reserve (ClayIdTable *id_table,
                       gint         id)
{
  gboolean success;

  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), FALSE);
  g_return_val_if_fail (id >= CLAY_ID_TABLE_START_ID && id <= CLAY_ID_TABLE_END_ID, FALSE);

  success = (! clay_id_table_exists (id_table, id));
  if (success)
    g_hash_table_insert (id_table->priv->id_table, GINT_TO_POINTER (id), NULL);

  return success;
}

void
clay_id_table_unreserve (ClayIdTable *id_table,
                         gint         id)
{
  g_return_if_fail (CLAY_IS_ID_TABLE (id_table));
  g_return_if_fail (id >= CLAY_ID_TABLE_START_ID && id <= CLAY_ID_TABLE_END_ID);
  g_return_if_fail (clay_id_table_exists (id_table, id) &&
                    clay_id_table_lookup (id_table, id) == NULL);

  g_hash_table_remove (id_table->priv->id_table, GINT_TO_POINTER (id));
}

/**
 * clay_id_table_insert_reserved:
 * @id_table: A #ClayIdTable
 * @id:       An unused ID to reserve.
 * @data:     Data to insert and assign @id to.
 *
 * Insert data in the id table with an ID previously reserved with
 * clay_id_table_reserve().
 **/
void
clay_id_table_insert_reserved (ClayIdTable *id_table,
                               gint         id,
                               gpointer     data)
{
  g_return_if_fail (CLAY_IS_ID_TABLE (id_table));
  g_return_if_fail (id >= CLAY_ID_TABLE_START_ID && id <= CLAY_ID_TABLE_END_ID);
  g_return_if_fail (clay_id_table_exists (id_table, id) &&
                    clay_id_table_lookup (id_table, id) == NULL);

  clay_id_table_replace (id_table, id, data);
}

/**
 * clay_id_table_lookup:
 * @id_table: A #ClayIdTable
 * @id: The ID of the data to lookup
 *
 * Lookup data based on ID. Note that it cannot distinguish a reserved
 * ID from an absent ID.
 *
 * Returns: (nullable) (transfer none): The data,
 *          or %NULL if no data with the given ID was found.
 **/
gpointer
clay_id_table_lookup (ClayIdTable *id_table, gint id)
{
  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), NULL);

  return g_hash_table_lookup (id_table->priv->id_table, GINT_TO_POINTER (id));
}

/**
 * clay_id_table_remove:
 * @id_table: A #ClayIdTable
 * @id: The ID of the data to remove.
 *
 * Remove the data from the table with the given ID.
 *
 * Returns: %TRUE if data with the ID existed and was successfully
 *          removed, %FALSE otherwise.
 **/
gboolean
clay_id_table_remove (ClayIdTable *id_table, gint id)
{
  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), FALSE);

  g_return_val_if_fail (id_table != NULL, FALSE);

  return g_hash_table_remove (id_table->priv->id_table, GINT_TO_POINTER (id));
}


/* Private Functions */

/**
 * clay_id_table_exists:
 * @id_table: A #ClayIdTable
 * @id:       The ID of the data to lookup
 *
 * Verify if an ID has data or is reserved.
 *
 * Returns: whether @id exists in @id_table.
 **/
static gboolean
clay_id_table_exists (ClayIdTable *id_table,
                      gint         id)
{
  g_return_val_if_fail (CLAY_IS_ID_TABLE (id_table), FALSE);

  return g_hash_table_lookup_extended (id_table->priv->id_table, GINT_TO_POINTER (id), NULL, NULL);
}
