/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Clay Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#ifndef __ICO_EXPORT_H__
#define __ICO_EXPORT_H__


ClayPDBStatusType ico_export_image        (GFile                *file,
                                           ClayImage            *image,
                                           ClayProcedure        *procedure,
                                           ClayProcedureConfig  *config,
                                           gint32                run_mode,
                                           GError              **error);

ClayPDBStatusType cur_export_image        (GFile                *file,
                                           ClayImage            *image,
                                           ClayProcedure        *procedure,
                                           ClayProcedureConfig  *config,
                                           gint32                run_mode,
                                           gsize                *n_hot_spot_x,
                                           const gint32         *hot_spot_x,
                                           gint32              **new_hot_spot_x,
                                           gsize                *n_hot_spot_y,
                                           const gint32         *hot_spot_y,
                                           gint32              **new_hot_spot_y,
                                           GError              **error);

gboolean          ico_cmap_contains_black (ClayPalette         *cmap);

gboolean          ico_save_init           (ClayImage           *image,
                                           gint32               run_mode,
                                           IcoSaveInfo         *info,
                                           gint                 n_hot_spot_x,
                                           const gint32        *hot_spot_x,
                                           gint                 n_hot_spot_y,
                                           const gint32        *hot_spot_y,
                                           GError             **error);

gboolean          ico_save_dialog         (ClayImage           *image,
                                           ClayProcedure       *procedure,
                                           ClayProcedureConfig *config,
                                           IcoSaveInfo         *info,
                                           AniFileHeader       *ani_header,
                                           AniSaveInfo         *ani_info);

void              ico_save_info_free      (IcoSaveInfo         *info);

ClayPDBStatusType shared_save_image       (GFile               *file,
                                           FILE                *fp_ani,
                                           ClayImage           *image,
                                           ClayProcedure       *procedure,
                                           ClayProcedureConfig *config,
                                           gint32               run_mode,
                                           gsize               *n_hot_spot_x,
                                           const gint32        *hot_spot_x,
                                           gint32             **new_hot_spot_x,
                                           gsize               *n_hot_spot_y,
                                           const gint32        *hot_spot_y,
                                           gint32             **new_hot_spot_y,
                                           gint32               file_offset,
                                           gint                 icon_index,
                                           GError             **error,
                                           IcoSaveInfo         *info);

#endif /* __ICO_EXPORT_H__ */
