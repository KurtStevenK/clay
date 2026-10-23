/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayprocedureconfig-private.h
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

#ifndef __CLAY_PROCEDURE_CONFIG_PRIVATE_H__
#define __CLAY_PROCEDURE_CONFIG_PRIVATE_H__

G_BEGIN_DECLS


G_GNUC_INTERNAL void           _clay_procedure_config_get_values   (ClayProcedureConfig  *config,
                                                                    ClayValueArray       *values);

G_GNUC_INTERNAL void           _clay_procedure_config_begin_run    (ClayProcedureConfig  *config,
                                                                    ClayImage            *image,
                                                                    ClayRunMode           run_mode,
                                                                    const ClayValueArray *args,
                                                                    gboolean             *editable_properties);
G_GNUC_INTERNAL void           _clay_procedure_config_end_run      (ClayProcedureConfig  *config,
                                                                    ClayPDBStatusType     status);

G_GNUC_INTERNAL ClayMetadata * _clay_procedure_config_begin_export (ClayProcedureConfig  *config,
                                                                    ClayImage            *original_image,
                                                                    ClayRunMode           run_mode,
                                                                    const ClayValueArray *args,
                                                                    const gchar          *mime_type);
G_GNUC_INTERNAL void           _clay_procedure_config_end_export   (ClayProcedureConfig  *config,
                                                                    ClayImage            *exported_image,
                                                                    GFile                *file,
                                                                    ClayPDBStatusType     status);


/* These 3 functions are not marked internal because they are used in libclayui.
 * The headers is not installed so these functions should be considered not
 * existing. Yet they are still exported in the library.
 */
gboolean   clay_procedure_config_has_default   (ClayProcedureConfig  *config);

gboolean   clay_procedure_config_load_default  (ClayProcedureConfig  *config,
                                                GError              **error);
gboolean   clay_procedure_config_save_default  (ClayProcedureConfig  *config,
                                                GError              **error);


G_END_DECLS

#endif /* __CLAY_PROCEDURE_CONFIG_PRIVATE_H__ */
