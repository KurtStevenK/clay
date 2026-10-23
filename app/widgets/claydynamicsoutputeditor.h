/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydynamicsoutputeditor.h
 * Copyright (C) 2010 Alexia Death
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


#define CLAY_TYPE_DYNAMICS_OUTPUT_EDITOR (clay_dynamics_output_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDynamicsOutputEditor,
                          clay_dynamics_output_editor,
                          CLAY, DYNAMICS_OUTPUT_EDITOR,
                          GtkBox)


struct _ClayDynamicsOutputEditorClass
{
  GtkBoxClass  parent_class;
};


GtkWidget * clay_dynamics_output_editor_new (ClayDynamicsOutput *output);
