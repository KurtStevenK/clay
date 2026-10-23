/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayidtable.h
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

#pragma once

#include "clayobject.h"

#define CLAY_ID_TABLE_START_ID 1
#define CLAY_ID_TABLE_END_ID   G_MAXINT


#define CLAY_TYPE_ID_TABLE            (clay_id_table_get_type ())
#define CLAY_ID_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ID_TABLE, ClayIdTable))
#define CLAY_ID_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ID_TABLE, ClayIdTableClass))
#define CLAY_IS_ID_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ID_TABLE))
#define CLAY_IS_ID_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ID_TABLE))
#define CLAY_ID_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ID_TABLE, ClayIdTableClass))


typedef struct _ClayIdTableClass   ClayIdTableClass;
typedef struct _ClayIdTablePrivate ClayIdTablePrivate;

struct _ClayIdTable
{
  ClayObject          parent_instance;

  ClayIdTablePrivate *priv;
};

struct _ClayIdTableClass
{
  ClayObjectClass  parent_class;
};


GType          clay_id_table_get_type        (void);
ClayIdTable *  clay_id_table_new             (void);
gboolean       clay_id_table_is_valid_id     (gint64       id);
gint           clay_id_table_insert          (ClayIdTable *id_table,
                                              gpointer     data);
gboolean       clay_id_table_reserve         (ClayIdTable *id_table,
                                              gint         id);
void           clay_id_table_unreserve       (ClayIdTable *id_table,
                                              gint         id);
void           clay_id_table_insert_reserved (ClayIdTable *id_table,
                                              gint         id,
                                              gpointer     data);
gint           clay_id_table_insert_with_id  (ClayIdTable *id_table,
                                              gint         id,
                                              gpointer     data);
void           clay_id_table_replace         (ClayIdTable *id_table,
                                              gint         id,
                                              gpointer     data);
gpointer       clay_id_table_lookup          (ClayIdTable *id_table,
                                              gint         id);
gboolean       clay_id_table_remove          (ClayIdTable *id_table,
                                              gint         id);
