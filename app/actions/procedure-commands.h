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


ClayValueArray * procedure_commands_get_run_mode_arg (ClayProcedure  *procedure);
ClayValueArray * procedure_commands_get_data_args    (ClayProcedure  *procedure,
                                                      ClayObject     *object);
ClayValueArray * procedure_commands_get_image_args   (ClayProcedure  *procedure,
                                                      ClayImage      *image);
ClayValueArray * procedure_commands_get_items_args   (ClayProcedure  *procedure,
                                                      ClayImage      *image,
                                                      GList          *items);
ClayValueArray * procedure_commands_get_display_args (ClayProcedure  *procedure,
                                                      ClayDisplay    *display,
                                                      ClayObject     *settings);

gboolean      procedure_commands_run_procedure       (ClayProcedure  *procedure,
                                                      Clay           *clay,
                                                      ClayProgress   *progress,
                                                      ClayValueArray *args);
gboolean      procedure_commands_run_procedure_async (ClayProcedure  *procedure,
                                                      Clay           *clay,
                                                      ClayProgress   *progress,
                                                      ClayRunMode     run_mode,
                                                      ClayValueArray *args,
                                                      ClayDisplay    *display);
