/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginprocframe.h
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


struct _ClayPlugInProcFrame
{
  gint                 ref_count;

  ClayContext         *main_context;
  GList               *context_stack;

  ClayProcedure       *procedure;
  GMainLoop           *main_loop;

  ClayValueArray      *return_vals;

  ClayProgress        *progress;
  gboolean             progress_created;
  gulong               progress_cancel_id;

  ClayPDBErrorHandler  error_handler;

  /*  lists of things to clean up on dispose  */
  GList               *image_cleanups;
  GList               *item_cleanups;
};


ClayPlugInProcFrame * clay_plug_in_proc_frame_new     (ClayContext         *context,
                                                       ClayProgress        *progress,
                                                       ClayPlugInProcedure *procedure);
void                  clay_plug_in_proc_frame_init    (ClayPlugInProcFrame *proc_frame,
                                                       ClayContext         *context,
                                                       ClayProgress        *progress,
                                                       ClayPlugInProcedure *procedure);

void                  clay_plug_in_proc_frame_dispose (ClayPlugInProcFrame *proc_frame,
                                                       ClayPlugIn          *plug_in);

ClayPlugInProcFrame * clay_plug_in_proc_frame_ref     (ClayPlugInProcFrame *proc_frame);
void                  clay_plug_in_proc_frame_unref   (ClayPlugInProcFrame *proc_frame,
                                                       ClayPlugIn          *plug_in);

ClayValueArray      * clay_plug_in_proc_frame_get_return_values
                                                      (ClayPlugInProcFrame *proc_frame);
