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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clay.h"
#include "clayimage.h"
#include "clayguide.h"
#include "clayimage-guides.h"
#include "clayimage-private.h"
#include "clayimage-undo-push.h"

#include "clay-intl.h"


/*  public functions  */

ClayGuide *
clay_image_add_hguide (ClayImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  ClayGuide *guide;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  guide = clay_guide_new (CLAY_ORIENTATION_HORIZONTAL,
                          image->clay->next_guide_id++);

  if (push_undo)
    clay_image_undo_push_guide (image,
                                C_("undo-type", "Add Horizontal Guide"), guide);

  clay_image_add_guide (image, guide, position);
  g_object_unref (G_OBJECT (guide));

  return guide;
}

ClayGuide *
clay_image_add_vguide (ClayImage *image,
                       gint       position,
                       gboolean   push_undo)
{
  ClayGuide *guide;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  guide = clay_guide_new (CLAY_ORIENTATION_VERTICAL,
                          image->clay->next_guide_id++);

  if (push_undo)
    clay_image_undo_push_guide (image,
                                C_("undo-type", "Add Vertical Guide"), guide);

  clay_image_add_guide (image, guide, position);
  g_object_unref (guide);

  return guide;
}

void
clay_image_add_guide (ClayImage *image,
                      ClayGuide *guide,
                      gint       position)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->guides = g_list_prepend (private->guides, guide);

  clay_guide_set_position (guide, position);
  g_object_ref (guide);

  clay_image_guide_added (image, guide);
}

void
clay_image_remove_guide (ClayImage *image,
                         ClayGuide *guide,
                         gboolean   push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (clay_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_guide (image, C_("undo-type", "Remove Guide"), guide);

  private->guides = g_list_remove (private->guides, guide);
  clay_aux_item_removed (CLAY_AUX_ITEM (guide));

  clay_image_guide_removed (image, guide);

  clay_guide_set_position (guide, CLAY_GUIDE_POSITION_UNDEFINED);
  g_object_unref (guide);
}

void
clay_image_move_guide (ClayImage *image,
                       ClayGuide *guide,
                       gint       position,
                       gboolean   push_undo)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  if (clay_guide_is_custom (guide))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_guide (image, C_("undo-type", "Move Guide"), guide);

  clay_guide_set_position (guide, position);

  clay_image_guide_moved (image, guide);
}

GList *
clay_image_get_guides (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->guides;
}

ClayGuide *
clay_image_get_guide (ClayImage *image,
                      guint32    id)
{
  GList *guides;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  for (guides = CLAY_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      ClayGuide *guide = guides->data;

      if (clay_aux_item_get_id (CLAY_AUX_ITEM (guide)) == id)
        return guide;
    }

  return NULL;
}

ClayGuide *
clay_image_get_next_guide (ClayImage *image,
                           guint32    id,
                           gboolean  *guide_found)
{
  GList *guides;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (guide_found != NULL, NULL);

  if (id == 0)
    *guide_found = TRUE;
  else
    *guide_found = FALSE;

  for (guides = CLAY_IMAGE_GET_PRIVATE (image)->guides;
       guides;
       guides = g_list_next (guides))
    {
      ClayGuide *guide = guides->data;

      if (*guide_found) /* this is the first guide after the found one */
        return guide;

      if (clay_aux_item_get_id (CLAY_AUX_ITEM (guide)) == id) /* found it, next one will be returned */
        *guide_found = TRUE;
    }

  return NULL;
}
