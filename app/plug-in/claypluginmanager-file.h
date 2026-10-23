/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginmanager-file.h
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


void       clay_plug_in_manager_add_load_procedure    (ClayPlugInManager      *manager,
                                                       ClayPlugInProcedure    *proc);
void       clay_plug_in_manager_add_save_procedure    (ClayPlugInManager      *manager,
                                                       ClayPlugInProcedure    *proc);

GSList   * clay_plug_in_manager_get_file_procedures   (ClayPlugInManager      *manager,
                                                       ClayFileProcedureGroup  group);

ClayPlugInProcedure *
clay_plug_in_manager_file_procedure_find              (ClayPlugInManager      *manager,
                                                       ClayFileProcedureGroup  group,
                                                       GFile                  *file,
                                                       GError                **error);
ClayPlugInProcedure *
clay_plug_in_manager_file_procedure_find_by_prefix    (ClayPlugInManager      *manager,
                                                       ClayFileProcedureGroup  group,
                                                       GFile                  *file);
ClayPlugInProcedure *
clay_plug_in_manager_file_procedure_find_by_extension (ClayPlugInManager      *manager,
                                                       ClayFileProcedureGroup  group,
                                                       GFile                  *file);
ClayPlugInProcedure *
clay_plug_in_manager_file_procedure_find_by_mime_type (ClayPlugInManager      *manager,
                                                       ClayFileProcedureGroup  group,
                                                       const gchar            *mime_type);

