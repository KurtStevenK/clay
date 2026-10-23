/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportprocedure.h
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

#ifndef __CLAY_EXPORT_PROCEDURE_H__
#define __CLAY_EXPORT_PROCEDURE_H__

#include <libclay/clayfileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayRunExportFunc:
 * @procedure:   the #ClayProcedure that runs.
 * @run_mode:    the #ClayRunMode.
 * @image:       the image to export.
 * @file:        the #GFile to export to.
 * @options:     the #ClayExportOptions settings.
 * @metadata:    metadata object prepared for the mimetype passed in
 *               clay_file_procedure_set_mime_types() if export_metadata
 *               argument was set in clay_export_procedure_new().
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in clay_export_procedure_new().
 *
 * The export function is run during the lifetime of the Clay session,
 * each time a plug-in export procedure is called.
 *
 * If a MimeType was passed in clay_export_procedure_new(), then @metadata will be
 * non-%NULL and can be tweaked by the run() function if needed. Otherwise you
 * can let it as-is and it will be stored back into the exported @file according
 * to rules on metadata export shared across formats.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef ClayValueArray * (* ClayRunExportFunc) (ClayProcedure        *procedure,
                                                ClayRunMode           run_mode,
                                                ClayImage            *image,
                                                GFile                *file,
                                                ClayExportOptions    *options,
                                                ClayMetadata         *metadata,
                                                ClayProcedureConfig  *config,
                                                gpointer              run_data);

/**
 * ClayExportGetCapabilitiesFunc:
 * @procedure:   the #ClayProcedure that runs.
 * @config:      the #ClayProcedureConfig.
 * @options:     the @ClayExportOptions object to update.
 * @get_capabilities_data: (closure): the @get_capabilities_data argument
 *                         given to [method@Clay.ExportProcedure.set_capabilities].
 *
 * This function returns the capabilities requested by your export
 * procedure, depending on @config or @options.
 *
 * Returns: a bitfield of the capabilities you want when preparing the
 *          image for exporting.
 *
 * Since: 3.0
 **/
typedef ClayExportCapabilities   (* ClayExportGetCapabilitiesFunc) (ClayProcedure        *procedure,
                                                                    ClayProcedureConfig  *config,
                                                                    ClayExportOptions    *options,
                                                                    gpointer              get_capabilities_data);


#define CLAY_TYPE_EXPORT_PROCEDURE (clay_export_procedure_get_type ())
G_DECLARE_FINAL_TYPE (ClayExportProcedure, clay_export_procedure, CLAY, EXPORT_PROCEDURE, ClayFileProcedure)


ClayProcedure * clay_export_procedure_new                   (ClayPlugIn                    *plug_in,
                                                             const gchar                   *name,
                                                             ClayPDBProcType                proc_type,
                                                             gboolean                       export_metadata,
                                                             ClayRunExportFunc              run_func,
                                                             gpointer                       run_data,
                                                             GDestroyNotify                 run_data_destroy);

void            clay_export_procedure_set_capabilities      (ClayExportProcedure           *procedure,
                                                             ClayExportCapabilities         capabilities,
                                                             ClayExportGetCapabilitiesFunc  get_capabilities_func,
                                                             gpointer                       get_capabilities_data,
                                                             GDestroyNotify                 get_capabilities_data_destroy);


void            clay_export_procedure_set_export_metadata   (ClayExportProcedure           *procedure,
                                                             gboolean                       export_metadata);
void            clay_export_procedure_set_support_exif      (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);
void            clay_export_procedure_set_support_iptc      (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);
void            clay_export_procedure_set_support_xmp       (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);
void            clay_export_procedure_set_support_profile   (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);
void            clay_export_procedure_set_support_thumbnail (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);
void            clay_export_procedure_set_support_comment   (ClayExportProcedure           *procedure,
                                                             gboolean                       supports);

gboolean        clay_export_procedure_get_export_metadata   (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_exif      (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_iptc      (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_xmp       (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_profile   (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_thumbnail (ClayExportProcedure           *procedure);
gboolean        clay_export_procedure_get_support_comment   (ClayExportProcedure           *procedure);


G_END_DECLS

#endif  /*  __CLAY_EXPORT_PROCEDURE_H__  */
