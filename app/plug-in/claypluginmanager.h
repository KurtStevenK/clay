/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claypluginmanager.h
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

#include "core/clayobject.h"


#define CLAY_TYPE_PLUG_IN_MANAGER            (clay_plug_in_manager_get_type ())
#define CLAY_PLUG_IN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUG_IN_MANAGER, ClayPlugInManager))
#define CLAY_PLUG_IN_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUG_IN_MANAGER, ClayPlugInManagerClass))
#define CLAY_IS_PLUG_IN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUG_IN_MANAGER))
#define CLAY_IS_PLUG_IN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUG_IN_MANAGER))


typedef struct _ClayPlugInManagerClass ClayPlugInManagerClass;

struct _ClayPlugInManager
{
  ClayObject         parent_instance;

  Clay              *clay;

  GSList            *plug_in_defs;
  gboolean           write_pluginrc;

  GSList            *plug_in_procedures;

  GSList            *load_procs;
  GSList            *save_procs;
  GSList            *export_procs;
  GSList            *raw_load_procs;
  GSList            *batch_procs;

  GSList            *display_load_procs;
  GSList            *display_save_procs;
  GSList            *display_export_procs;
  GSList            *display_raw_load_procs;

  GSList            *help_domains;

  ClayPlugIn        *current_plug_in;
  GSList            *open_plug_ins;
  GSList            *plug_in_stack;

  GHashTable        *zombie_plug_ins;

  ClayPlugInShm     *shm;
  ClayInterpreterDB *interpreter_db;
  ClayEnvironTable  *environ_table;
  ClayPlugInDebug   *debug;
  GList             *data_list;
};

struct _ClayPlugInManagerClass
{
  ClayObjectClass  parent_class;

  void (* plug_in_opened)    (ClayPlugInManager *manager,
                              ClayPlugIn        *plug_in);
  void (* plug_in_closed)    (ClayPlugInManager *manager,
                              ClayPlugIn        *plug_in);
};


GType               clay_plug_in_manager_get_type (void);

ClayPlugInManager * clay_plug_in_manager_new      (Clay                *clay);

void    clay_plug_in_manager_initialize           (ClayPlugInManager   *manager,
                                                   ClayInitStatusFunc   status_callback);
void    clay_plug_in_manager_exit                 (ClayPlugInManager   *manager);

/* Register a plug-in. This function is public for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void    clay_plug_in_manager_add_procedure        (ClayPlugInManager   *manager,
                                                   ClayPlugInProcedure *procedure);

void    clay_plug_in_manager_add_batch_procedure  (ClayPlugInManager      *manager,
                                                   ClayPlugInProcedure    *proc);
GSList * clay_plug_in_manager_get_batch_procedures (ClayPlugInManager      *manager);

void    clay_plug_in_manager_add_temp_proc        (ClayPlugInManager      *manager,
                                                   ClayTemporaryProcedure *procedure);
void    clay_plug_in_manager_remove_temp_proc     (ClayPlugInManager      *manager,
                                                   ClayTemporaryProcedure *procedure);

void    clay_plug_in_manager_add_open_plug_in     (ClayPlugInManager   *manager,
                                                   ClayPlugIn          *plug_in);
void    clay_plug_in_manager_remove_open_plug_in  (ClayPlugInManager   *manager,
                                                   ClayPlugIn          *plug_in);

void    clay_plug_in_manager_watch_zombie_plug_in (ClayPlugInManager   *manager,
                                                   ClayPlugIn          *plug_in);

void    clay_plug_in_manager_plug_in_push         (ClayPlugInManager   *manager,
                                                   ClayPlugIn          *plug_in);
void    clay_plug_in_manager_plug_in_pop          (ClayPlugInManager   *manager);
