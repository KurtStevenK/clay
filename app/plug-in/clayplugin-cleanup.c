/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-cleanup.c
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
#include "core/claycontainer.h"
#include "core/claydrawable.h"
#include "core/claydrawable-shadow.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayundostack.h"

#include "clayplugin.h"
#include "clayplugin-cleanup.h"
#include "claypluginmanager.h"
#include "claypluginprocedure.h"

#include "clay-log.h"


typedef struct _ClayPlugInCleanupImage ClayPlugInCleanupImage;

struct _ClayPlugInCleanupImage
{
  ClayImage *image;
  gint       image_id;

  gint       undo_group_count;
  gint       layers_freeze_count;
  gint       channels_freeze_count;
  gint       paths_freeze_count;
};


typedef struct _ClayPlugInCleanupItem ClayPlugInCleanupItem;

struct _ClayPlugInCleanupItem
{
  ClayItem *item;
  gint      item_id;

  gboolean  shadow_buffer;
};


/*  local function prototypes  */

static ClayPlugInCleanupImage *
              clay_plug_in_cleanup_image_new      (ClayPlugInProcFrame    *proc_frame,
                                                   ClayImage              *image);
static void   clay_plug_in_cleanup_image_free     (ClayPlugInProcFrame    *proc_frame,
                                                   ClayPlugInCleanupImage *cleanup);
static gboolean
              clay_plug_in_cleanup_image_is_clean (ClayPlugInCleanupImage *cleanup);
static ClayPlugInCleanupImage *
              clay_plug_in_cleanup_image_get      (ClayPlugInProcFrame    *proc_frame,
                                                   ClayImage              *image);
static void   clay_plug_in_cleanup_image          (ClayPlugInProcFrame    *proc_frame,
                                                   ClayPlugInCleanupImage *cleanup);

static ClayPlugInCleanupItem *
              clay_plug_in_cleanup_item_new       (ClayPlugInProcFrame    *proc_frame,
                                                   ClayItem               *item);
static void   clay_plug_in_cleanup_item_free      (ClayPlugInProcFrame    *proc_frame,
                                                   ClayPlugInCleanupItem  *cleanup);
static gboolean
              clay_plug_in_cleanup_item_is_clean  (ClayPlugInCleanupItem  *cleanup);
static ClayPlugInCleanupItem *
              clay_plug_in_cleanup_item_get       (ClayPlugInProcFrame    *proc_frame,
                                                   ClayItem               *item);
static void   clay_plug_in_cleanup_item           (ClayPlugInProcFrame    *proc_frame,
                                                   ClayPlugInCleanupItem  *cleanup);


/*  public functions  */

gboolean
clay_plug_in_cleanup_undo_group_start (ClayPlugIn *plug_in,
                                       ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = clay_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->undo_group_count++;

  return TRUE;
}

gboolean
clay_plug_in_cleanup_undo_group_end (ClayPlugIn *plug_in,
                                     ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->undo_group_count > 0)
    {
      cleanup->undo_group_count--;

      if (clay_plug_in_cleanup_image_is_clean (cleanup))
        clay_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_plug_in_cleanup_layers_freeze (ClayPlugIn *plug_in,
                                    ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = clay_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->layers_freeze_count++;

  return TRUE;
}

gboolean
clay_plug_in_cleanup_layers_thaw (ClayPlugIn *plug_in,
                                  ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    {
      cleanup->layers_freeze_count--;

      if (clay_plug_in_cleanup_image_is_clean (cleanup))
        clay_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_plug_in_cleanup_channels_freeze (ClayPlugIn *plug_in,
                                      ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = clay_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->channels_freeze_count++;

  return TRUE;
}

gboolean
clay_plug_in_cleanup_channels_thaw (ClayPlugIn *plug_in,
                                    ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    {
      cleanup->channels_freeze_count--;

      if (clay_plug_in_cleanup_image_is_clean (cleanup))
        clay_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_plug_in_cleanup_paths_freeze (ClayPlugIn *plug_in,
                                   ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = clay_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->paths_freeze_count++;

  return TRUE;
}

gboolean
clay_plug_in_cleanup_paths_thaw (ClayPlugIn *plug_in,
                                 ClayImage  *image)
{
  ClayPlugInProcFrame    *proc_frame;
  ClayPlugInCleanupImage *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->paths_freeze_count > 0)
    {
      cleanup->paths_freeze_count--;

      if (clay_plug_in_cleanup_image_is_clean (cleanup))
        clay_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_plug_in_cleanup_add_shadow (ClayPlugIn   *plug_in,
                                 ClayDrawable *drawable)
{
  ClayPlugInProcFrame   *proc_frame;
  ClayPlugInCleanupItem *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_item_get (proc_frame, CLAY_ITEM (drawable));

  if (! cleanup)
    cleanup = clay_plug_in_cleanup_item_new (proc_frame, CLAY_ITEM (drawable));

  cleanup->shadow_buffer = TRUE;

  return TRUE;
}

gboolean
clay_plug_in_cleanup_remove_shadow (ClayPlugIn   *plug_in,
                                    ClayDrawable *drawable)
{
  ClayPlugInProcFrame   *proc_frame;
  ClayPlugInCleanupItem *cleanup;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);
  cleanup    = clay_plug_in_cleanup_item_get (proc_frame, CLAY_ITEM (drawable));

  if (! cleanup)
    return FALSE;

  if (cleanup->shadow_buffer)
    {
      cleanup->shadow_buffer = FALSE;

      if (clay_plug_in_cleanup_item_is_clean (cleanup))
        clay_plug_in_cleanup_item_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

void
clay_plug_in_cleanup (ClayPlugIn          *plug_in,
                      ClayPlugInProcFrame *proc_frame)
{
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  while (proc_frame->image_cleanups)
    {
      ClayPlugInCleanupImage *cleanup = proc_frame->image_cleanups->data;

      if (clay_image_get_by_id (plug_in->manager->clay,
                                cleanup->image_id) == cleanup->image)
        {
          clay_plug_in_cleanup_image (proc_frame, cleanup);
        }

      clay_plug_in_cleanup_image_free (proc_frame, cleanup);
    }

  while (proc_frame->item_cleanups)
    {
      ClayPlugInCleanupItem *cleanup = proc_frame->item_cleanups->data;

      if (clay_item_get_by_id (plug_in->manager->clay,
                               cleanup->item_id) == cleanup->item)
        {
          clay_plug_in_cleanup_item (proc_frame, cleanup);
        }

      clay_plug_in_cleanup_item_free (proc_frame, cleanup);
    }
}


/*  private functions  */

static ClayPlugInCleanupImage *
clay_plug_in_cleanup_image_new (ClayPlugInProcFrame *proc_frame,
                                ClayImage           *image)
{
  ClayPlugInCleanupImage *cleanup = g_slice_new0 (ClayPlugInCleanupImage);

  cleanup->image    = image;
  cleanup->image_id = clay_image_get_id (image);

  proc_frame->image_cleanups = g_list_prepend (proc_frame->image_cleanups,
                                               cleanup);

  return cleanup;
}

static void
clay_plug_in_cleanup_image_free (ClayPlugInProcFrame    *proc_frame,
                                 ClayPlugInCleanupImage *cleanup)
{
  proc_frame->image_cleanups = g_list_remove (proc_frame->image_cleanups,
                                              cleanup);

  g_slice_free (ClayPlugInCleanupImage, cleanup);
}

static gboolean
clay_plug_in_cleanup_image_is_clean (ClayPlugInCleanupImage *cleanup)
{
  if (cleanup->undo_group_count > 0)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    return FALSE;

  if (cleanup->paths_freeze_count > 0)
    return FALSE;

  return TRUE;
}

static ClayPlugInCleanupImage *
clay_plug_in_cleanup_image_get (ClayPlugInProcFrame *proc_frame,
                                ClayImage           *image)
{
  GList *list;

  for (list = proc_frame->image_cleanups; list; list = g_list_next (list))
    {
      ClayPlugInCleanupImage *cleanup = list->data;

      if (cleanup->image == image)
        return cleanup;
    }

  return NULL;
}

static void
clay_plug_in_cleanup_image (ClayPlugInProcFrame    *proc_frame,
                            ClayPlugInCleanupImage *cleanup)
{
  ClayImage     *image = cleanup->image;
  ClayContainer *container;

  if (cleanup->undo_group_count > 0)
    {
      g_message ("Plug-in '%s' left image undo in inconsistent state, "
                 "closing open undo groups.",
                 clay_procedure_get_label (proc_frame->procedure));

      while (cleanup->undo_group_count--)
        if (! clay_image_undo_group_end (image))
          break;
    }

  container = clay_image_get_layers (image);

  if (cleanup->layers_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's layers frozen, "
                 "thawing layers.",
                 clay_procedure_get_label (proc_frame->procedure));

      while (cleanup->layers_freeze_count-- > 0 &&
             clay_container_frozen (container))
        {
          clay_container_thaw (container);
        }
    }

  container = clay_image_get_channels (image);

  if (cleanup->channels_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's channels frozen, "
                 "thawing channels.",
                 clay_procedure_get_label (proc_frame->procedure));

      while (cleanup->channels_freeze_count-- > 0 &&
             clay_container_frozen (container))
        {
          clay_container_thaw (container);
        }
    }

  container = clay_image_get_paths (image);

  if (cleanup->paths_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's paths frozen, "
                 "thawing paths.",
                 clay_procedure_get_label (proc_frame->procedure));

      while (cleanup->paths_freeze_count > 0 &&
             clay_container_frozen (container))
        {
          clay_container_thaw (container);
        }
    }
}

static ClayPlugInCleanupItem *
clay_plug_in_cleanup_item_new (ClayPlugInProcFrame *proc_frame,
                               ClayItem            *item)
{
  ClayPlugInCleanupItem *cleanup = g_slice_new0 (ClayPlugInCleanupItem);

  cleanup->item    = item;
  cleanup->item_id = clay_item_get_id (item);

  proc_frame->item_cleanups = g_list_prepend (proc_frame->item_cleanups,
                                              cleanup);

  return cleanup;
}

static void
clay_plug_in_cleanup_item_free (ClayPlugInProcFrame   *proc_frame,
                                ClayPlugInCleanupItem *cleanup)
{
  proc_frame->item_cleanups = g_list_remove (proc_frame->item_cleanups,
                                             cleanup);

  g_slice_free (ClayPlugInCleanupItem, cleanup);
}

static gboolean
clay_plug_in_cleanup_item_is_clean (ClayPlugInCleanupItem *cleanup)
{
  if (cleanup->shadow_buffer)
    return FALSE;

  return TRUE;
}

static ClayPlugInCleanupItem *
clay_plug_in_cleanup_item_get (ClayPlugInProcFrame *proc_frame,
                               ClayItem            *item)
{
  GList *list;

  for (list = proc_frame->item_cleanups; list; list = g_list_next (list))
    {
      ClayPlugInCleanupItem *cleanup = list->data;

      if (cleanup->item == item)
        return cleanup;
    }

  return NULL;
}

static void
clay_plug_in_cleanup_item (ClayPlugInProcFrame   *proc_frame,
                           ClayPlugInCleanupItem *cleanup)
{
  ClayItem *item = cleanup->item;

  if (cleanup->shadow_buffer)
    {
      CLAY_LOG (SHADOW_TILES,
                "Freeing shadow buffer of drawable '%s' on behalf of '%s'.",
                clay_object_get_name (item),
                clay_procedure_get_label (proc_frame->procedure));

      clay_drawable_free_shadow_buffer (CLAY_DRAWABLE (item));

      cleanup->shadow_buffer = FALSE;
    }
}
