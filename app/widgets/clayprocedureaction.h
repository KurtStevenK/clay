/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprocedureaction.h
 * Copyright (C) 2004-2016 Michael Natterer <mitch@gimp.org>
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

#include "clayactionimpl.h"


#define CLAY_TYPE_PROCEDURE_ACTION            (clay_procedure_action_get_type ())
#define CLAY_PROCEDURE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PROCEDURE_ACTION, ClayProcedureAction))
#define CLAY_PROCEDURE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PROCEDURE_ACTION, ClayProcedureActionClass))
#define CLAY_IS_PROCEDURE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PROCEDURE_ACTION))
#define CLAY_IS_PROCEDURE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), CLAY_TYPE_PROCEDURE_ACTION))
#define CLAY_PROCEDURE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CLAY_TYPE_PROCEDURE_ACTION, ClayProcedureActionClass))


typedef struct _ClayProcedureActionClass ClayProcedureActionClass;

struct _ClayProcedureAction
{
  ClayActionImpl  parent_instance;

  ClayProcedure  *procedure;
};

struct _ClayProcedureActionClass
{
  ClayActionImplClass parent_class;
};


GType                 clay_procedure_action_get_type (void);

ClayProcedureAction * clay_procedure_action_new      (const gchar   *name,
                                                      const gchar   *label,
                                                      const gchar   *tooltip,
                                                      const gchar   *icon_name,
                                                      const gchar   *help_id,
                                                      ClayProcedure *procedure,
                                                      ClayContext   *context);
