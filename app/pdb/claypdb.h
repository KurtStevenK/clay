/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core/clayobject.h"


#define CLAY_TYPE_PDB            (clay_pdb_get_type ())
#define CLAY_PDB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PDB, ClayPDB))
#define CLAY_PDB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PDB, ClayPDBClass))
#define CLAY_IS_PDB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PDB))
#define CLAY_IS_PDB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PDB))
#define CLAY_PDB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PDB, ClayPDBClass))


typedef struct _ClayPDBClass ClayPDBClass;

struct _ClayPDB
{
  ClayObject  parent_instance;

  Clay       *clay;

  GHashTable *procedures;
  GHashTable *compat_proc_names;
};

struct _ClayPDBClass
{
  ClayObjectClass parent_class;

  void (* register_procedure)   (ClayPDB       *pdb,
                                 ClayProcedure *procedure);
  void (* unregister_procedure) (ClayPDB       *pdb,
                                 ClayProcedure *procedure);
};


GType            clay_pdb_get_type                       (void);

ClayPDB        * clay_pdb_new                            (Clay           *clay);

void             clay_pdb_register_procedure             (ClayPDB        *pdb,
                                                          ClayProcedure  *procedure);
void             clay_pdb_unregister_procedure           (ClayPDB        *pdb,
                                                          ClayProcedure  *procedure);

ClayProcedure  * clay_pdb_lookup_procedure               (ClayPDB        *pdb,
                                                          const gchar    *name);

void             clay_pdb_register_compat_proc_name      (ClayPDB        *pdb,
                                                          const gchar    *old_name,
                                                          const gchar    *new_name);
const gchar    * clay_pdb_lookup_compat_proc_name        (ClayPDB        *pdb,
                                                          const gchar    *old_name);

ClayValueArray * clay_pdb_execute_procedure_by_name_args (ClayPDB        *pdb,
                                                          ClayContext    *context,
                                                          ClayProgress   *progress,
                                                          GError        **error,
                                                          const gchar    *name,
                                                          ClayValueArray *args);
ClayValueArray * clay_pdb_execute_procedure_by_name      (ClayPDB        *pdb,
                                                          ClayContext    *context,
                                                          ClayProgress   *progress,
                                                          GError        **error,
                                                          const gchar    *name,
                                                          ...);

GList          * clay_pdb_get_deprecated_procedures      (ClayPDB        *pdb);
