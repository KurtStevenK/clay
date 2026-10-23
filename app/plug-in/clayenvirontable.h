/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayenvirontable.h
 * (C) 2002 Manish Singh <yosh@gimp.org>
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


#define CLAY_TYPE_ENVIRON_TABLE            (clay_environ_table_get_type ())
#define CLAY_ENVIRON_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ENVIRON_TABLE, ClayEnvironTable))
#define CLAY_ENVIRON_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ENVIRON_TABLE, ClayEnvironTableClass))
#define CLAY_IS_ENVIRON_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ENVIRON_TABLE))
#define CLAY_IS_ENVIRON_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ENVIRON_TABLE))
#define CLAY_ENVIRON_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ENVIRON_TABLE, ClayEnvironTableClass))


typedef struct _ClayEnvironTableClass ClayEnvironTableClass;

struct _ClayEnvironTable
{
  GObject      parent_instance;

  gboolean     verbose;

  GHashTable  *vars;
  GHashTable  *internal;

  gchar      **envp;
};

struct _ClayEnvironTableClass
{
  GObjectClass  parent_class;
};


GType               clay_environ_table_get_type  (void);
ClayEnvironTable  * clay_environ_table_new       (gboolean          verbose);

void                clay_environ_table_load      (ClayEnvironTable *environ_table,
                                                  GList            *path);

void                clay_environ_table_add       (ClayEnvironTable *environ_table,
                                                  const gchar      *name,
                                                  const gchar      *value,
                                                  const gchar      *separator);
void                clay_environ_table_remove    (ClayEnvironTable *environ_table,
                                                  const gchar      *name);

void                clay_environ_table_clear     (ClayEnvironTable *environ_table);
void                clay_environ_table_clear_all (ClayEnvironTable *environ_table);

gchar            ** clay_environ_table_get_envp  (ClayEnvironTable *environ_table);
