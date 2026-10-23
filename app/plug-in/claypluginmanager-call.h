/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginmanager-call.h
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

#ifndef __YES_I_NEED_CLAY_PLUG_IN_MANAGER_CALL__
#error Do not use clay_plug_in_manager_call_run*(), use clay_procedure_execute*() instead.
#endif


/*  Call the plug-in's query() function
 */
void             clay_plug_in_manager_call_query    (ClayPlugInManager      *manager,
                                                     ClayContext            *context,
                                                     ClayPlugInDef          *plug_in_def);

/*  Call the plug-in's init() function
 */
void             clay_plug_in_manager_call_init     (ClayPlugInManager      *manager,
                                                     ClayContext            *context,
                                                     ClayPlugInDef          *plug_in_def);

/*  Run a plug-in as if it were a procedure database procedure
 */
ClayValueArray * clay_plug_in_manager_call_run      (ClayPlugInManager      *manager,
                                                     ClayContext            *context,
                                                     ClayProgress           *progress,
                                                     ClayPlugInProcedure    *procedure,
                                                     ClayValueArray         *args,
                                                     gboolean                synchronous,
                                                     ClayDisplay            *display);

/*  Run a temp plug-in proc as if it were a procedure database procedure
 */
ClayValueArray * clay_plug_in_manager_call_run_temp (ClayPlugInManager      *manager,
                                                     ClayContext            *context,
                                                     ClayProgress           *progress,
                                                     ClayTemporaryProcedure *procedure,
                                                     ClayValueArray         *args);
