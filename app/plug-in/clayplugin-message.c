/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-message.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaybase/clayprotocol-private.h"
#include "libclaybase/claywire-private.h"

#include "libclay/claygpparams.h"

#include "plug-in-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-tile-compat.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/claydrawable-shadow.h"

#include "pdb/clay-pdb-compat.h"
#include "pdb/claypdb.h"
#include "pdb/claypdberror.h"

#include "clayplugin.h"
#include "clayplugin-cleanup.h"
#include "clayplugin-message.h"
#include "claypluginmanager.h"
#include "clayplugindef.h"
#include "claypluginshm.h"
#include "claytemporaryprocedure.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void clay_plug_in_handle_quit             (ClayPlugIn      *plug_in);
static void clay_plug_in_handle_tile_request     (ClayPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void clay_plug_in_handle_tile_put         (ClayPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void clay_plug_in_handle_tile_get         (ClayPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void clay_plug_in_handle_proc_run         (ClayPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void clay_plug_in_handle_proc_return      (ClayPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void clay_plug_in_handle_temp_proc_return (ClayPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void clay_plug_in_handle_proc_install     (ClayPlugIn      *plug_in,
                                                  GPProcInstall   *proc_install);
static void clay_plug_in_handle_proc_uninstall   (ClayPlugIn      *plug_in,
                                                  GPProcUninstall *proc_uninstall);
static void clay_plug_in_handle_persistent_ack   (ClayPlugIn      *plug_in);
static void clay_plug_in_handle_has_init         (ClayPlugIn      *plug_in);


/*  public functions  */

void
clay_plug_in_handle_message (ClayPlugIn      *plug_in,
                             ClayWireMessage *msg)
{
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->open == TRUE);
  g_return_if_fail (msg != NULL);

  switch (msg->type)
    {
    case GP_QUIT:
      clay_plug_in_handle_quit (plug_in);
      break;

    case GP_CONFIG:
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a CONFIG message.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_REQ:
      clay_plug_in_handle_tile_request (plug_in, msg->data);
      break;

    case GP_TILE_ACK:
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TILE_ACK message.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_DATA:
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TILE_DATA message.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
      break;

    case GP_PROC_RUN:
      clay_plug_in_handle_proc_run (plug_in, msg->data);
      break;

    case GP_PROC_RETURN:
      clay_plug_in_handle_proc_return (plug_in, msg->data);
      break;

    case GP_TEMP_PROC_RUN:
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RUN message.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
      break;

    case GP_TEMP_PROC_RETURN:
      clay_plug_in_handle_temp_proc_return (plug_in, msg->data);
      break;

    case GP_PROC_INSTALL:
      clay_plug_in_handle_proc_install (plug_in, msg->data);
      break;

    case GP_PROC_UNINSTALL:
      clay_plug_in_handle_proc_uninstall (plug_in, msg->data);
      break;

    case GP_PERSISTENT_ACK:
      clay_plug_in_handle_persistent_ack (plug_in);
      break;

    case GP_HAS_INIT:
      clay_plug_in_handle_has_init (plug_in);
      break;
    }
}


/*  private functions  */

static void
clay_plug_in_handle_quit (ClayPlugIn *plug_in)
{
  clay_plug_in_close (plug_in, FALSE);
}

static void
clay_plug_in_handle_tile_request (ClayPlugIn *plug_in,
                                  GPTileReq  *request)
{
  g_return_if_fail (request != NULL);

  if (request->drawable_id == -1)
    clay_plug_in_handle_tile_put (plug_in, request);
  else
    clay_plug_in_handle_tile_get (plug_in, request);
}

static void
clay_plug_in_handle_tile_put (ClayPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  GPTileData      *tile_info;
  ClayWireMessage  msg;
  ClayDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;

  tile_data.drawable_id = -1;
  tile_data.tile_num    = 0;
  tile_data.shadow      = 0;
  tile_data.bpp         = 0;
  tile_data.width       = 0;
  tile_data.height      = 0;
  tile_data.use_shm     = (plug_in->manager->shm != NULL);
  tile_data.data        = NULL;

  if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! clay_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (msg.type != GP_TILE_DATA)
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "expected tile data and received: %d", msg.type);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  tile_info = msg.data;

  drawable = (ClayDrawable *) clay_item_get_by_id (plug_in->manager->clay,
                                                   tile_info->drawable_id);

  if (! CLAY_IS_DRAWABLE (drawable))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried writing to invalid drawable %d (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_id);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (clay_item_is_removed (CLAY_ITEM (drawable)))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried writing to drawable %d which was removed "
                    "from the image (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_id);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (tile_info->shadow)
    {

      /*  don't check whether the drawable is a group or locked here,
       *  the plugin will get a proper error message when it tries to
       *  merge the shadow tiles, which is much better than just
       *  killing it.
       */
      buffer = clay_drawable_get_shadow_buffer (drawable);

      clay_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      if (clay_item_is_content_locked (CLAY_ITEM (drawable), NULL))
        {
          clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                        "Plug-in \"%s\"\n(%s)\n\n"
                        "tried writing to a locked drawable %d (killing)",
                        clay_object_get_name (plug_in),
                        clay_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_id);
          clay_plug_in_close (plug_in, TRUE);
          return;
        }
      else if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
        {
          clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                        "Plug-in \"%s\"\n(%s)\n\n"
                        "tried writing to a group layer %d (killing)",
                        clay_object_get_name (plug_in),
                        clay_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_id);
          clay_plug_in_close (plug_in, TRUE);
          return;
        }

      buffer = clay_drawable_get_buffer (drawable);
    }

  if (! clay_gegl_buffer_get_tile_rect (buffer,
                                        CLAY_PLUG_IN_TILE_WIDTH,
                                        CLAY_PLUG_IN_TILE_HEIGHT,
                                        tile_info->tile_num,
                                        &tile_rect))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "requested invalid tile #%d for writing (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    tile_info->tile_num);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  if (tile_data.use_shm)
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       clay_plug_in_shm_get_addr (plug_in->manager->shm),
                       GEGL_AUTO_ROWSTRIDE);
    }
  else
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       tile_info->data,
                       GEGL_AUTO_ROWSTRIDE);
    }

  clay_wire_destroy (&msg);

  if (! gp_tile_ack_write (plug_in->my_write, plug_in))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }
}

static void
clay_plug_in_handle_tile_get (ClayPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  ClayWireMessage  msg;
  ClayDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;
  gint             tile_size;

  drawable = (ClayDrawable *) clay_item_get_by_id (plug_in->manager->clay,
                                                   request->drawable_id);

  if (! CLAY_IS_DRAWABLE (drawable))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried reading from invalid drawable %d (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    request->drawable_id);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (clay_item_is_removed (CLAY_ITEM (drawable)))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried reading from drawable %d which was removed "
                    "from the image (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    request->drawable_id);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (request->shadow)
    {
      buffer = clay_drawable_get_shadow_buffer (drawable);

      clay_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      buffer = clay_drawable_get_buffer (drawable);
    }

  if (! clay_gegl_buffer_get_tile_rect (buffer,
                                        CLAY_PLUG_IN_TILE_WIDTH,
                                        CLAY_PLUG_IN_TILE_HEIGHT,
                                        request->tile_num,
                                        &tile_rect))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "requested invalid tile #%d for reading (killing)",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    request->tile_num);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  tile_size = (babl_format_get_bytes_per_pixel (format) *
               tile_rect.width * tile_rect.height);

  tile_data.drawable_id = request->drawable_id;
  tile_data.tile_num    = request->tile_num;
  tile_data.shadow      = request->shadow;
  tile_data.bpp         = babl_format_get_bytes_per_pixel (format);
  tile_data.width       = tile_rect.width;
  tile_data.height      = tile_rect.height;
  tile_data.use_shm     = (plug_in->manager->shm != NULL);

  if (tile_data.use_shm)
    {
      gegl_buffer_get (buffer, &tile_rect, 1.0, format,
                       clay_plug_in_shm_get_addr (plug_in->manager->shm),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  else
    {
      tile_data.data = g_malloc (tile_size);

      gegl_buffer_get (buffer, &tile_rect, 1.0, format,
                       tile_data.data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }

  if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! clay_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  if (msg.type != GP_TILE_ACK)
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "expected tile ack and received: %d", msg.type);
      clay_plug_in_close (plug_in, TRUE);
      return;
    }

  clay_wire_destroy (&msg);
}

static void
clay_plug_in_handle_proc_error (ClayPlugIn          *plug_in,
                                ClayPlugInProcFrame *proc_frame,
                                const gchar         *name,
                                const GError        *error)
{
  switch (proc_frame->error_handler)
    {
    case CLAY_PDB_ERROR_HANDLER_INTERNAL:
      if (error->domain == CLAY_PDB_ERROR)
        {
          clay_message (plug_in->manager->clay,
                        G_OBJECT (proc_frame->progress),
                        CLAY_MESSAGE_ERROR,
                        _("Calling error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      else
        {
          clay_message (plug_in->manager->clay,
                        G_OBJECT (proc_frame->progress),
                        CLAY_MESSAGE_ERROR,
                        _("Execution error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      break;

    case CLAY_PDB_ERROR_HANDLER_PLUGIN:
      /*  the plug-in is responsible for handling this error  */
      break;
    }
}

static void
clay_plug_in_handle_proc_run (ClayPlugIn *plug_in,
                              GPProcRun  *proc_run)
{
  ClayPlugInProcFrame *proc_frame;
  gchar               *canonical;
  const gchar         *proc_name   = NULL;
  ClayProcedure       *procedure;
  ClayValueArray      *args        = NULL;
  ClayValueArray      *return_vals = NULL;
  ClayRunMode          run_mode    = CLAY_RUN_INTERACTIVE;
  GError              *error       = NULL;

  g_return_if_fail (proc_run != NULL);
  g_return_if_fail (proc_run->name != NULL);

  canonical = clay_canonicalize_identifier (proc_run->name);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  procedure = clay_pdb_lookup_procedure (plug_in->manager->clay->pdb,
                                         canonical);

  if (! procedure)
    {
      proc_name = clay_pdb_lookup_compat_proc_name (plug_in->manager->clay->pdb,
                                                    canonical);

      if (proc_name)
        {
          procedure = clay_pdb_lookup_procedure (plug_in->manager->clay->pdb,
                                                 proc_name);

          if (plug_in->manager->clay->pdb_compat_mode == CLAY_PDB_COMPAT_WARN)
            {
              clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_WARNING,
                            "Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            clay_object_get_name (plug_in),
                            clay_file_get_utf8_name (plug_in->file),
                            canonical, proc_name);
            }
        }
    }
  else if (procedure->deprecated)
    {
      if (plug_in->manager->clay->pdb_compat_mode == CLAY_PDB_COMPAT_WARN)
        {
          if (! strcmp (procedure->deprecated, "NONE"))
            {
              clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_WARNING,
                            "Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.",
                            clay_object_get_name (plug_in),
                            clay_file_get_utf8_name (plug_in->file),
                            canonical);
            }
          else
            {
              clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_WARNING,
                            "WARNING: Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            clay_object_get_name (plug_in),
                            clay_file_get_utf8_name (plug_in->file),
                            canonical, procedure->deprecated);
            }
        }
    }

  if (! proc_name)
    proc_name = canonical;

  args = _clay_gp_params_to_value_array (plug_in->manager->clay,
                                         procedure ? procedure->args     : NULL,
                                         procedure ? procedure->num_args : 0,
                                         proc_run->params,
                                         proc_run->n_params,
                                         FALSE);

  for (gint i = 0; i < proc_run->n_params; i++)
    {
      if (G_VALUE_TYPE (clay_value_array_index (args, i)) == CLAY_TYPE_RUN_MODE)
        {
          run_mode = g_value_get_enum (clay_value_array_index (args, i));
          break;
        }
    }

  /*  Execute the procedure even if clay_pdb_lookup_procedure()
   *  returned NULL, clay_pdb_execute_procedure_by_name_args() will
   *  return appropriate error return_vals.
   */
  clay_plug_in_manager_plug_in_push (plug_in->manager, plug_in);
  return_vals = clay_pdb_execute_procedure_by_name_args (plug_in->manager->clay->pdb,
                                                         proc_frame->context_stack ?
                                                         proc_frame->context_stack->data :
                                                         proc_frame->main_context,
                                                         proc_frame->progress,
                                                         &error,
                                                         proc_name,
                                                         args);
  clay_plug_in_manager_plug_in_pop (plug_in->manager);

  clay_value_array_unref (args);

  if (error)
    {
      if (run_mode == CLAY_RUN_INTERACTIVE)
        clay_plug_in_handle_proc_error (plug_in, proc_frame, canonical, error);

      g_error_free (error);
    }

  g_free (canonical);

  /*  Don't bother to send the return value if executing the procedure
   *  closed the plug-in (e.g. if the procedure is clay-quit)
   */
  if (plug_in->open)
    {
      GPProcReturn proc_return;

      /*  Return the name we got called with, *not* proc_name or canonical,
       *  since proc_name may have been remapped by clay->procedural_compat_ht
       *  and canonical may be different too.
       */
      proc_return.name     = proc_run->name;
      proc_return.n_params = clay_value_array_length (return_vals);
      proc_return.params   = _clay_value_array_to_gp_params (return_vals, FALSE);

      if (! gp_proc_return_write (plug_in->my_write, &proc_return, plug_in))
        {
          clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                        "%s: ERROR", G_STRFUNC);
          clay_plug_in_close (plug_in, TRUE);
        }

      _clay_gp_params_free (proc_return.params, proc_return.n_params, FALSE);
    }

  clay_value_array_unref (return_vals);
}

static void
clay_plug_in_handle_proc_return (ClayPlugIn   *plug_in,
                                 GPProcReturn *proc_return)
{
  ClayPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

  g_return_if_fail (proc_return != NULL);

  proc_frame->return_vals =
    _clay_gp_params_to_value_array (plug_in->manager->clay,
                                    proc_frame->procedure->values,
                                    proc_frame->procedure->num_values,
                                    proc_return->params,
                                    proc_return->n_params,
                                    TRUE);

  if (proc_frame->main_loop)
    {
      g_main_loop_quit (proc_frame->main_loop);
    }
  else
    {
      /*  the plug-in is run asynchronously, so display its error
       *  messages here because nobody else will do it
       */
      clay_plug_in_procedure_handle_return_values (CLAY_PLUG_IN_PROCEDURE (proc_frame->procedure),
                                                   plug_in->manager->clay,
                                                   proc_frame->progress,
                                                   proc_frame->return_vals);
    }

  clay_plug_in_close (plug_in, FALSE);
}

static void
clay_plug_in_handle_temp_proc_return (ClayPlugIn   *plug_in,
                                      GPProcReturn *proc_return)
{
  g_return_if_fail (proc_return != NULL);

  if (plug_in->temp_proc_frames)
    {
      ClayPlugInProcFrame *proc_frame = plug_in->temp_proc_frames->data;

      proc_frame->return_vals =
        _clay_gp_params_to_value_array (plug_in->manager->clay,
                                        proc_frame->procedure->values,
                                        proc_frame->procedure->num_values,
                                        proc_return->params,
                                        proc_return->n_params,
                                        TRUE);

      clay_plug_in_main_loop_quit (plug_in);
      clay_plug_in_proc_frame_pop (plug_in);
    }
  else
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RETURN message while not running "
                    "a temporary procedure.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
    }
}

static void
clay_plug_in_handle_proc_install (ClayPlugIn    *plug_in,
                                  GPProcInstall *proc_install)
{
  ClayPlugInProcedure *proc       = NULL;
  ClayProcedure       *procedure  = NULL;
  gboolean             null_name  = FALSE;
  gboolean             valid_utf8 = TRUE;
  gint                 i;

  g_return_if_fail (proc_install != NULL);
  g_return_if_fail (proc_install->name != NULL);

  if (! clay_is_canonical_identifier (proc_install->name))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with a "
                    "non-canonical name.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  /*  Sanity check strings for UTF-8 validity  */

#define VALIDATE(str) ((str) == NULL || g_utf8_validate ((str), -1, NULL))

  for (i = 0; i < proc_install->n_params && valid_utf8 && ! null_name; i++)
    {
      if (! proc_install->params[i].name)
        {
          null_name = TRUE;
        }
      else if (! (VALIDATE (proc_install->params[i].name) &&
                  VALIDATE (proc_install->params[i].nick) &&
                  VALIDATE (proc_install->params[i].blurb)))
        {
          valid_utf8 = FALSE;
        }
    }

  for (i = 0; i < proc_install->n_return_vals && valid_utf8 && !null_name; i++)
    {
      if (! proc_install->return_vals[i].name)
        {
          null_name = TRUE;
        }
      else if (! (VALIDATE (proc_install->return_vals[i].name) &&
                  VALIDATE (proc_install->return_vals[i].nick) &&
                  VALIDATE (proc_install->return_vals[i].blurb)))
        {
          valid_utf8 = FALSE;
        }
    }

#undef VALIDATE

  if (null_name)
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with a "
                    "NULL parameter name.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  if (! valid_utf8)
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with "
                    "invalid UTF-8 strings.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  /*  Create the procedure object  */

  switch (proc_install->type)
    {
    case CLAY_PDB_PROC_TYPE_PLUGIN:
    case CLAY_PDB_PROC_TYPE_PERSISTENT:
      procedure = clay_plug_in_procedure_new (proc_install->type,
                                              plug_in->file,
                                              plug_in->root_folder);
      break;

    case CLAY_PDB_PROC_TYPE_TEMPORARY:
      procedure = clay_temporary_procedure_new (plug_in);
      break;
    }

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);

  proc->mtime                 = time (NULL);
  proc->installed_during_init = (plug_in->call_mode == CLAY_PLUG_IN_CALL_INIT);

  clay_object_set_name (CLAY_OBJECT (procedure), proc_install->name);

  for (i = 0; i < proc_install->n_params; i++)
    {
      GParamSpec *pspec =
        _clay_gp_param_def_to_param_spec (&proc_install->params[i]);

      if (pspec)
        clay_procedure_add_argument (procedure, pspec);
    }

  for (i = 0; i < proc_install->n_return_vals; i++)
    {
      GParamSpec *pspec =
        _clay_gp_param_def_to_param_spec (&proc_install->return_vals[i]);

      if (pspec)
        clay_procedure_add_return_value (procedure, pspec);
    }

  /*  Install the procedure  */

  switch (proc_install->type)
    {
    case CLAY_PDB_PROC_TYPE_PLUGIN:
    case CLAY_PDB_PROC_TYPE_PERSISTENT:
      clay_plug_in_def_add_procedure (plug_in->plug_in_def, proc);
      break;

    case CLAY_PDB_PROC_TYPE_TEMPORARY:
      clay_plug_in_add_temp_proc (plug_in, CLAY_TEMPORARY_PROCEDURE (proc));
      break;
    }

  g_object_unref (proc);
}

static void
clay_plug_in_handle_proc_uninstall (ClayPlugIn      *plug_in,
                                    GPProcUninstall *proc_uninstall)
{
  ClayPlugInProcedure *proc;

  g_return_if_fail (proc_uninstall != NULL);
  g_return_if_fail (proc_uninstall->name != NULL);

  if (! clay_is_canonical_identifier (proc_uninstall->name))
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to uninstall procedure \"%s\" with a "
                    "non-canonical name.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file),
                    proc_uninstall->name);
      return;
    }

  proc = clay_plug_in_procedure_find (plug_in->temp_procedures,
                                      proc_uninstall->name);

  if (proc)
    clay_plug_in_remove_temp_proc (plug_in, CLAY_TEMPORARY_PROCEDURE (proc));
}

static void
clay_plug_in_handle_persistent_ack (ClayPlugIn *plug_in)
{
  if (plug_in->ext_main_loop)
    {
      g_main_loop_quit (plug_in->ext_main_loop);
    }
  else
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a PERSISTENT_ACK message while not being started "
                    "as a persistent plug-in.  This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
    }
}

static void
clay_plug_in_handle_has_init (ClayPlugIn *plug_in)
{
  if (plug_in->call_mode == CLAY_PLUG_IN_CALL_QUERY)
    {
      clay_plug_in_def_set_has_init (plug_in->plug_in_def, TRUE);
    }
  else
    {
      clay_message (plug_in->manager->clay, NULL, CLAY_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent an HAS_INIT message while not in query().  "
                    "This should not happen.",
                    clay_object_get_name (plug_in),
                    clay_file_get_utf8_name (plug_in->file));
      clay_plug_in_close (plug_in, TRUE);
    }
}
