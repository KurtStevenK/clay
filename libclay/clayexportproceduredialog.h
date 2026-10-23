/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportproceduredialog.h
 * Copyright (C) 2020 Jehan
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_EXPORT_PROCEDURE_DIALOG_H__
#define __CLAY_EXPORT_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_EXPORT_PROCEDURE_DIALOG (clay_export_procedure_dialog_get_type ())
G_DECLARE_FINAL_TYPE (ClayExportProcedureDialog, clay_export_procedure_dialog, CLAY, EXPORT_PROCEDURE_DIALOG, ClayProcedureDialog)


GtkWidget * clay_export_procedure_dialog_new          (ClayExportProcedure       *procedure,
                                                       ClayProcedureConfig       *config,
                                                       ClayImage                 *image);

void        clay_export_procedure_dialog_add_metadata (ClayExportProcedureDialog *dialog,
                                                       const gchar               *property);


G_END_DECLS

#endif /* __CLAY_EXPORT_PROCEDURE_DIALOG_H__ */
