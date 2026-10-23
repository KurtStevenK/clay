/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * script-fu-dialog.h
 * Copyright (C) 2022 Lloyd Konneker
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

 #ifndef __SCRIPT_FU_DIALOG_H__
 #define __SCRIPT_FU_DIALOG_H__

ClayValueArray *script_fu_dialog_run_image_proc (
                                      ClayProcedure        *procedure,
                                      SFScript             *script,
                                      ClayImage            *image,
                                      ClayDrawable        **drawables,
                                      ClayProcedureConfig  *config);
ClayValueArray *script_fu_dialog_run_regular_proc (
                                      ClayProcedure        *procedure,
                                      SFScript             *script,
                                      ClayProcedureConfig  *config);
#endif /* __SCRIPT_FU_DIALOG_H__ */
