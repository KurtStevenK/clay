/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbprocedure.h
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

#ifndef __CLAY_PDB_PROCEDURE_H__
#define __CLAY_PDB_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PDB_PROCEDURE (_clay_pdb_procedure_get_type ())
G_DECLARE_FINAL_TYPE (ClayPDBProcedure, _clay_pdb_procedure, CLAY, PDB_PROCEDURE, ClayProcedure)


G_GNUC_INTERNAL ClayProcedure * _clay_pdb_procedure_new (ClayPDB     *pdb,
                                                         const gchar *name);


G_END_DECLS

#endif  /*  __CLAY_PDB_PROCEDURE_H__  */
