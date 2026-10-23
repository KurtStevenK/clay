/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdb.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_PDB_H__
#define __CLAY_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PDB (clay_pdb_get_type ())
G_DECLARE_FINAL_TYPE (ClayPDB, clay_pdb, CLAY, PDB, GObject)


gboolean             clay_pdb_procedure_exists     (ClayPDB              *pdb,
                                                    const gchar          *procedure_name);

ClayProcedure      * clay_pdb_lookup_procedure     (ClayPDB              *pdb,
                                                    const gchar          *procedure_name);

gchar              * clay_pdb_temp_procedure_name  (ClayPDB              *pdb);

gboolean             clay_pdb_dump_to_file         (ClayPDB              *pdb,
                                                    GFile                *file);
gchar             ** clay_pdb_query_procedures     (ClayPDB              *pdb,
                                                    const gchar          *name,
                                                    const gchar          *blurb,
                                                    const gchar          *help,
                                                    const gchar          *help_id,
                                                    const gchar          *authors,
                                                    const gchar          *copyright,
                                                    const gchar          *date,
                                                    const gchar          *proc_type);

const gchar        * clay_pdb_get_last_error       (ClayPDB              *pdb);
ClayPDBStatusType    clay_pdb_get_last_status      (ClayPDB              *pdb);


/* Internal use */

G_GNUC_INTERNAL ClayValueArray * _clay_pdb_run_procedure_array  (ClayPDB              *pdb,
                                                                 const gchar          *procedure_name,
                                                                 const ClayValueArray *arguments);


G_END_DECLS

#endif  /*  __CLAY_PDB_H__  */
