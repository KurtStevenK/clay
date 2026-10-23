/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-save.h
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


ClayPDBStatusType   file_save (Clay                 *clay,
                               ClayImage            *image,
                               ClayProgress         *progress,
                               GFile                *file,
                               ClayPlugInProcedure  *file_proc,
                               ClayRunMode           run_mode,
                               gboolean              change_saved_state,
                               gboolean              export_backward,
                               gboolean              export_forward,
                               GError              **error);
