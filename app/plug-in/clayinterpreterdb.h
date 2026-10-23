/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayinterpreterdb.h
 * (C) 2005 Manish Singh <yosh@gimp.org>
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


#define CLAY_TYPE_INTERPRETER_DB            (clay_interpreter_db_get_type ())
#define CLAY_INTERPRETER_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_INTERPRETER_DB, ClayInterpreterDB))
#define CLAY_INTERPRETER_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_INTERPRETER_DB, ClayInterpreterDBClass))
#define CLAY_IS_INTERPRETER_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_INTERPRETER_DB))
#define CLAY_IS_INTERPRETER_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_INTERPRETER_DB))
#define CLAY_INTERPRETER_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_INTERPRETER_DB, ClayInterpreterDBClass))


typedef struct _ClayInterpreterDBClass ClayInterpreterDBClass;

struct _ClayInterpreterDB
{
  GObject     parent_instance;

  gboolean    verbose;

  GHashTable *programs;

  GSList     *magics;
  GHashTable *magic_names;

  GHashTable *extensions;
  GHashTable *extension_names;
};

struct _ClayInterpreterDBClass
{
  GObjectClass  parent_class;
};


GType               clay_interpreter_db_get_type (void);
ClayInterpreterDB * clay_interpreter_db_new      (gboolean            verbose);

void                clay_interpreter_db_load     (ClayInterpreterDB  *db,
                                                  GList              *path);

void                clay_interpreter_db_clear    (ClayInterpreterDB  *db);

gchar             * clay_interpreter_db_resolve  (ClayInterpreterDB  *db,
                                                  const gchar        *program_path,
                                                  gchar             **interp_arg);
gchar       * clay_interpreter_db_get_extensions (ClayInterpreterDB  *db);
