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


#define FILE_SAVE_RESPONSE_OTHER_DIALOG -23


GtkWidget * file_save_dialog_new        (Clay                *clay,
                                         gboolean             export);

gboolean    file_save_dialog_save_image (ClayProgress        *progress_and_handler,
                                         Clay                *clay,
                                         ClayImage           *image,
                                         GFile               *file,
                                         ClayPlugInProcedure *write_proc,
                                         ClayRunMode          run_mode,
                                         gboolean             save_a_copy,
                                         gboolean             export_backward,
                                         gboolean             export_forward,
                                         gboolean             xcf_compression,
                                         gboolean             verbose_cancel);
