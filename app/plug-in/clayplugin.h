/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin.h
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
#include "claypluginprocframe.h"


#define WRITE_BUFFER_SIZE  512


#define CLAY_TYPE_PLUG_IN            (clay_plug_in_get_type ())
#define CLAY_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUG_IN, ClayPlugIn))
#define CLAY_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUG_IN, ClayPlugInClass))
#define CLAY_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUG_IN))
#define CLAY_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUG_IN))


typedef struct _ClayPlugInClass ClayPlugInClass;

struct _ClayPlugIn
{
  ClayObject           parent_instance;

  ClayPlugInManager   *manager;
  GFile               *file;            /*  Plug-in's full path name          */
  GFile               *root_folder;     /*  Plug-in's root full path          */

  ClayDisplay         *display;         /*  The display this plug-in was called from. */
  ClayPlugInCallMode   call_mode;       /*  QUERY, INIT or RUN                */
  guint                open : 1;        /*  Is the plug-in open?              */
  guint                hup : 1;         /*  Did we receive a G_IO_HUP         */
  GPid                 pid;             /*  Plug-in's process id              */

  GIOChannel          *my_read;         /*  App's read and write channels     */
  GIOChannel          *my_write;
  GIOChannel          *his_read;        /*  Plug-in's read and write channels */
  GIOChannel          *his_write;

  guint                input_id;        /*  Id of input proc                  */

  gchar                write_buffer[WRITE_BUFFER_SIZE]; /* Buffer for writing */
  gint                 write_buffer_index;              /* Buffer index       */

  GSList              *temp_procedures; /*  Temporary procedures              */

  GMainLoop           *ext_main_loop;   /*  for waiting for extension_ack     */

  ClayPlugInProcFrame  main_proc_frame;

  GList               *temp_proc_frames;

  ClayPlugInDef       *plug_in_def;     /*  Valid during query() and init()   */
};

struct _ClayPlugInClass
{
  ClayObjectClass  parent_class;
};


GType         clay_plug_in_get_type          (void);

ClayPlugIn  * clay_plug_in_new               (ClayPlugInManager      *manager,
                                              ClayContext            *context,
                                              ClayProgress           *progress,
                                              ClayPlugInProcedure    *procedure,
                                              GFile                  *file,
                                              GFile                  *root_folder,
                                              ClayDisplay            *display);

gboolean      clay_plug_in_open              (ClayPlugIn             *plug_in,
                                              ClayPlugInCallMode      call_mode,
                                              gboolean                synchronous);
void          clay_plug_in_close             (ClayPlugIn             *plug_in,
                                              gboolean                kill_it);

ClayPlugInProcFrame *
              clay_plug_in_get_proc_frame    (ClayPlugIn             *plug_in);

ClayPlugInProcFrame *
              clay_plug_in_proc_frame_push   (ClayPlugIn             *plug_in,
                                              ClayContext            *context,
                                              ClayProgress           *progress,
                                              ClayTemporaryProcedure *procedure);
void          clay_plug_in_proc_frame_pop    (ClayPlugIn             *plug_in);

void          clay_plug_in_main_loop         (ClayPlugIn             *plug_in);
void          clay_plug_in_main_loop_quit    (ClayPlugIn             *plug_in);

const gchar * clay_plug_in_get_undo_desc     (ClayPlugIn             *plug_in);

void          clay_plug_in_add_temp_proc     (ClayPlugIn             *plug_in,
                                              ClayTemporaryProcedure *procedure);
void          clay_plug_in_remove_temp_proc  (ClayPlugIn             *plug_in,
                                              ClayTemporaryProcedure *procedure);

void          clay_plug_in_set_error_handler (ClayPlugIn             *plug_in,
                                              ClayPDBErrorHandler     handler);
ClayPDBErrorHandler
              clay_plug_in_get_error_handler (ClayPlugIn             *plug_in);
