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

#include "claypaintoptions.h"


#define CLAY_TYPE_SMUDGE_OPTIONS            (clay_smudge_options_get_type ())
#define CLAY_SMUDGE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SMUDGE_OPTIONS, ClaySmudgeOptions))
#define CLAY_SMUDGE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SMUDGE_OPTIONS, ClaySmudgeOptionsClass))
#define CLAY_IS_SMUDGE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SMUDGE_OPTIONS))
#define CLAY_IS_SMUDGE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SMUDGE_OPTIONS))
#define CLAY_SMUDGE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SMUDGE_OPTIONS, ClaySmudgeOptionsClass))


typedef struct _ClaySmudgeOptionsClass ClaySmudgeOptionsClass;

struct _ClaySmudgeOptions
{
  ClayPaintOptions  parent_instance;

  gdouble           rate;
  gdouble           flow;
  gboolean          no_erasing;
  gboolean          sample_merged;
};

struct _ClaySmudgeOptionsClass
{
  ClayPaintOptionsClass  parent_class;
};


GType   clay_smudge_options_get_type (void);
