/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-context.c
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "plug-in-types.h"

#include "core/clay.h"

#include "pdb/claypdbcontext.h"

#include "clayplugin.h"
#include "clayplugin-context.h"
#include "claypluginmanager.h"


gboolean
clay_plug_in_context_push (ClayPlugIn *plug_in)
{
  ClayPlugInProcFrame *proc_frame;
  ClayContext         *parent;
  ClayContext         *context;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    parent = proc_frame->context_stack->data;
  else
    parent = proc_frame->main_context;

  context = clay_pdb_context_new (plug_in->manager->clay, parent, FALSE);

  proc_frame->context_stack = g_list_prepend (proc_frame->context_stack,
                                              context);

  return TRUE;
}

gboolean
clay_plug_in_context_pop (ClayPlugIn *plug_in)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    {
      ClayContext *context = proc_frame->context_stack->data;

      proc_frame->context_stack = g_list_remove (proc_frame->context_stack,
                                                 context);
      g_object_unref (context);

      return TRUE;
    }

  return FALSE;
}
