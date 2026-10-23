/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claygeglprocedure.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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

#include "pdb/clayprocedure.h"


#define CLAY_TYPE_GEGL_PROCEDURE            (clay_gegl_procedure_get_type ())
#define CLAY_GEGL_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GEGL_PROCEDURE, ClayGeglProcedure))
#define CLAY_GEGL_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GEGL_PROCEDURE, ClayGeglProcedureClass))
#define CLAY_IS_GEGL_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GEGL_PROCEDURE))
#define CLAY_IS_GEGL_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GEGL_PROCEDURE))
#define CLAY_GEGL_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GEGL_PROCEDURE, ClayGeglProcedureClass))


typedef struct _ClayGeglProcedure      ClayGeglProcedure;
typedef struct _ClayGeglProcedureClass ClayGeglProcedureClass;

struct _ClayGeglProcedure
{
  ClayProcedure  parent_instance;

  ClayDrawableFilter *filter;
  gchar              *operation;

  ClayRunMode         default_run_mode;
  ClayObject         *default_settings;

  gchar              *menu_label;

  /* TODO: Once we can serialize ClayDrawable, remove so that
   * filters with aux nodes can be non-destructive */
  gboolean            has_aux;
};

struct _ClayGeglProcedureClass
{
  ClayProcedureClass parent_class;
};


GType           clay_gegl_procedure_get_type (void);

ClayProcedure * clay_gegl_procedure_new               (Clay               *clay,
                                                       ClayDrawableFilter *filter_to_edit,
                                                       ClayRunMode         default_run_mode,
                                                       ClayObject         *default_settings,
                                                       const gchar        *operation,
                                                       const gchar        *name,
                                                       const gchar        *menu_label,
                                                       const gchar        *tooltip,
                                                       const gchar        *icon_name,
                                                       const gchar        *help_id);
gboolean        clay_gegl_procedure_is_editing_filter (ClayGeglProcedure  *procedure);
