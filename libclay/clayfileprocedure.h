/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfileprocedure.h
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

#ifndef __CLAY_FILE_PROCEDURE_H__
#define __CLAY_FILE_PROCEDURE_H__

#include <libclay/clayprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_FILE_PROCEDURE (clay_file_procedure_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayFileProcedure, clay_file_procedure, CLAY, FILE_PROCEDURE, ClayProcedure)

struct _ClayFileProcedureClass
{
  ClayProcedureClass parent_class;

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


void            clay_file_procedure_set_format_name (ClayFileProcedure *procedure,
                                                     const gchar       *format_name);
const gchar   * clay_file_procedure_get_format_name (ClayFileProcedure *procedure);

void            clay_file_procedure_set_mime_types  (ClayFileProcedure *procedure,
                                                     const gchar       *mime_types);
const gchar   * clay_file_procedure_get_mime_types  (ClayFileProcedure *procedure);

void            clay_file_procedure_set_extensions  (ClayFileProcedure *procedure,
                                                     const gchar       *extensions);
const gchar   * clay_file_procedure_get_extensions  (ClayFileProcedure *procedure);
void            clay_file_procedure_set_meta        (ClayFileProcedure *procedure,
                                                     gboolean          is_meta,
                                                     const gchar       *meta_extensions);
gboolean        clay_file_procedure_get_meta        (ClayFileProcedure *procedure,
                                                     const gchar       **meta_extensions);

void            clay_file_procedure_set_prefixes    (ClayFileProcedure *procedure,
                                                     const gchar       *prefixes);
const gchar   * clay_file_procedure_get_prefixes    (ClayFileProcedure *procedure);

void            clay_file_procedure_set_magics      (ClayFileProcedure *procedure,
                                                     const gchar       *magics);
const gchar   * clay_file_procedure_get_magics      (ClayFileProcedure *procedure);

void            clay_file_procedure_set_priority    (ClayFileProcedure *procedure,
                                                     gint               priority);
gint            clay_file_procedure_get_priority    (ClayFileProcedure *procedure);

void            clay_file_procedure_set_handles_remote
                                                    (ClayFileProcedure *procedure,
                                                     gboolean           handles_remote);
gboolean        clay_file_procedure_get_handles_remote
                                                    (ClayFileProcedure *procedure);


G_END_DECLS

#endif  /*  __CLAY_FILE_PROCEDURE_H__  */
