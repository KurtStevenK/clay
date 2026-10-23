/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdb-private.h
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

#ifndef __CLAY_PDB_PRIVATE_H__
#define __CLAY_PDB_PRIVATE_H__

G_BEGIN_DECLS


typedef enum
{
  CLAY_PDB_ERROR_FAILED,  /* generic error condition */
  CLAY_PDB_ERROR_CANCELLED,
  CLAY_PDB_ERROR_PDB_NOT_FOUND,
  CLAY_PDB_ERROR_INVALID_ARGUMENT,
  CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
  CLAY_PDB_ERROR_INTERNAL_ERROR
} ClayPdbErrorCode;


#define CLAY_PDB_ERROR (_clay_pdb_error_quark ())

GQuark _clay_pdb_error_quark (void) G_GNUC_CONST;


G_GNUC_INTERNAL ClayPDB        * _clay_pdb_new                 (ClayPlugIn            *plug_in);

G_GNUC_INTERNAL ClayPlugIn     * _clay_pdb_get_plug_in         (ClayPDB               *pdb);

gboolean                         clay_pdb_get_data             (const gchar           *identifier,
                                                                GBytes               **data);
gboolean                         clay_pdb_set_data             (const gchar           *identifier,
                                                                GBytes                *data);


G_END_DECLS

#endif  /*  __CLAY_PDB_PRIVATE_H__  */
