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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"

#include "clay.h"
#include "claycontext.h"
#include "claydrawable-edit.h"
#include "claydrawable-private.h"
#include "clayerror.h"
#include "clayimage.h"
#include "clayimage-new.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "claylayer-new.h"
#include "claylayermask.h"
#include "claylayer-floating-selection.h"
#include "claypickable.h"
#include "clayselection.h"

#include "clay-intl.h"


static gboolean   clay_selection_is_attached   (ClayItem            *item);
static ClayItemTree * clay_selection_get_tree  (ClayItem            *item);
static void       clay_selection_translate     (ClayItem            *item,
                                                gdouble              offset_x,
                                                gdouble              offset_y,
                                                gboolean             push_undo);
static void       clay_selection_scale         (ClayItem            *item,
                                                gint                 new_width,
                                                gint                 new_height,
                                                gint                 new_offset_x,
                                                gint                 new_offset_y,
                                                ClayInterpolationType interp_type,
                                                ClayProgress        *progress);
static void       clay_selection_resize        (ClayItem            *item,
                                                ClayContext         *context,
                                                ClayFillType         fill_type,
                                                gint                 new_width,
                                                gint                 new_height,
                                                gint                 offset_x,
                                                gint                 offset_y);
static void       clay_selection_flip          (ClayItem            *item,
                                                ClayContext         *context,
                                                ClayOrientationType  flip_type,
                                                gdouble              axis,
                                                gboolean             clip_result);
static void       clay_selection_rotate        (ClayItem            *item,
                                                ClayContext         *context,
                                                ClayRotationType     rotation_type,
                                                gdouble              center_x,
                                                gdouble              center_y,
                                                gboolean             clip_result);
static gboolean   clay_selection_fill          (ClayItem            *item,
                                                ClayDrawable        *drawable,
                                                ClayFillOptions     *fill_options,
                                                gboolean             push_undo,
                                                ClayProgress        *progress,
                                                GError             **error);
static gboolean   clay_selection_stroke        (ClayItem            *item,
                                                ClayDrawable        *drawable,
                                                ClayStrokeOptions   *stroke_options,
                                                gboolean             push_undo,
                                                ClayProgress        *progress,
                                                GError             **error);
static void       clay_selection_convert_type  (ClayDrawable        *drawable,
                                                ClayImage           *dest_image,
                                                const Babl          *new_format,
                                                ClayColorProfile    *src_profile,
                                                ClayColorProfile    *dest_profile,
                                                GeglDitherMethod     layer_dither_type,
                                                GeglDitherMethod     mask_dither_type,
                                                gboolean             push_undo,
                                                ClayProgress        *progress);
static void clay_selection_invalidate_boundary (ClayDrawable        *drawable);

static gboolean   clay_selection_boundary      (ClayChannel         *channel,
                                                const ClayBoundSeg **segs_in,
                                                const ClayBoundSeg **segs_out,
                                                gint                *num_segs_in,
                                                gint                *num_segs_out,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2);
static gboolean   clay_selection_is_empty      (ClayChannel         *channel);
static void       clay_selection_feather       (ClayChannel         *channel,
                                                gdouble              radius_x,
                                                gdouble              radius_y,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       clay_selection_sharpen       (ClayChannel         *channel,
                                                gboolean             push_undo);
static void       clay_selection_clear         (ClayChannel         *channel,
                                                const gchar         *undo_desc,
                                                gboolean             push_undo);
static void       clay_selection_all           (ClayChannel         *channel,
                                                gboolean             push_undo);
static void       clay_selection_invert        (ClayChannel         *channel,
                                                gboolean             push_undo);
static void       clay_selection_border        (ClayChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                ClayChannelBorderStyle style,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       clay_selection_grow          (ClayChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                gboolean             push_undo);
static void       clay_selection_shrink        (ClayChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       clay_selection_flood         (ClayChannel         *channel,
                                                gboolean             push_undo);


G_DEFINE_TYPE (ClaySelection, clay_selection, CLAY_TYPE_CHANNEL)

#define parent_class clay_selection_parent_class


static void
clay_selection_class_init (ClaySelectionClass *klass)
{
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class     = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class = CLAY_DRAWABLE_CLASS (klass);
  ClayChannelClass  *channel_class  = CLAY_CHANNEL_CLASS (klass);

  viewable_class->default_icon_name   = "clay-selection";
  viewable_class->default_name        = _("Selection Mask");

  item_class->is_attached             = clay_selection_is_attached;
  item_class->get_tree                = clay_selection_get_tree;
  item_class->translate               = clay_selection_translate;
  item_class->scale                   = clay_selection_scale;
  item_class->resize                  = clay_selection_resize;
  item_class->flip                    = clay_selection_flip;
  item_class->rotate                  = clay_selection_rotate;
  item_class->fill                    = clay_selection_fill;
  item_class->stroke                  = clay_selection_stroke;
  item_class->translate_desc          = C_("undo-type", "Move Selection");
  item_class->fill_desc               = C_("undo-type", "Fill Selection");
  item_class->stroke_desc             = C_("undo-type", "Stroke Selection");

  drawable_class->convert_type        = clay_selection_convert_type;
  drawable_class->invalidate_boundary = clay_selection_invalidate_boundary;

  channel_class->boundary             = clay_selection_boundary;
  channel_class->is_empty             = clay_selection_is_empty;
  channel_class->feather              = clay_selection_feather;
  channel_class->sharpen              = clay_selection_sharpen;
  channel_class->clear                = clay_selection_clear;
  channel_class->all                  = clay_selection_all;
  channel_class->invert               = clay_selection_invert;
  channel_class->border               = clay_selection_border;
  channel_class->grow                 = clay_selection_grow;
  channel_class->shrink               = clay_selection_shrink;
  channel_class->flood                = clay_selection_flood;

  channel_class->feather_desc         = C_("undo-type", "Feather Selection");
  channel_class->sharpen_desc         = C_("undo-type", "Sharpen Selection");
  channel_class->clear_desc           = C_("undo-type", "Select None");
  channel_class->all_desc             = C_("undo-type", "Select All");
  channel_class->invert_desc          = C_("undo-type", "Invert Selection");
  channel_class->border_desc          = C_("undo-type", "Border Selection");
  channel_class->grow_desc            = C_("undo-type", "Grow Selection");
  channel_class->shrink_desc          = C_("undo-type", "Shrink Selection");
  channel_class->flood_desc           = C_("undo-type", "Remove Holes");
}

static void
clay_selection_init (ClaySelection *selection)
{
}

static gboolean
clay_selection_is_attached (ClayItem *item)
{
  return (CLAY_IS_IMAGE (clay_item_get_image (item)) &&
          clay_image_get_mask (clay_item_get_image (item)) ==
          CLAY_CHANNEL (item));
}

static ClayItemTree *
clay_selection_get_tree (ClayItem *item)
{
  return NULL;
}

static void
clay_selection_translate (ClayItem *item,
                          gdouble   offset_x,
                          gdouble   offset_y,
                          gboolean  push_undo)
{
  CLAY_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);
}

static void
clay_selection_scale (ClayItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      ClayInterpolationType  interp_type,
                      ClayProgress          *progress)
{
  CLAY_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interp_type, progress);

  clay_item_set_offset (item, 0, 0);
}

static void
clay_selection_resize (ClayItem     *item,
                       ClayContext  *context,
                       ClayFillType  fill_type,
                       gint          new_width,
                       gint          new_height,
                       gint          offset_x,
                       gint          offset_y)
{
  CLAY_ITEM_CLASS (parent_class)->resize (item, context, CLAY_FILL_TRANSPARENT,
                                          new_width, new_height,
                                          offset_x, offset_y);

  clay_item_set_offset (item, 0, 0);
}

static void
clay_selection_flip (ClayItem            *item,
                     ClayContext         *context,
                     ClayOrientationType  flip_type,
                     gdouble              axis,
                     gboolean             clip_result)
{
  CLAY_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis, TRUE);
}

static void
clay_selection_rotate (ClayItem         *item,
                       ClayContext      *context,
                       ClayRotationType  rotation_type,
                       gdouble           center_x,
                       gdouble           center_y,
                       gboolean          clip_result)
{
  CLAY_ITEM_CLASS (parent_class)->rotate (item, context, rotation_type,
                                          center_x, center_y,
                                          clip_result);
}

static gboolean
clay_selection_fill (ClayItem         *item,
                     ClayDrawable     *drawable,
                     ClayFillOptions  *fill_options,
                     gboolean          push_undo,
                     ClayProgress     *progress,
                     GError          **error)
{
  ClaySelection      *selection = CLAY_SELECTION (item);
  const ClayBoundSeg *dummy_in;
  const ClayBoundSeg *dummy_out;
  gint                num_dummy_in;
  gint                num_dummy_out;
  gboolean            retval;

  if (! clay_channel_boundary (CLAY_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("There is no selection to fill."));
      return FALSE;
    }

  clay_selection_suspend (selection);

  retval = CLAY_ITEM_CLASS (parent_class)->fill (item, drawable,
                                                 fill_options,
                                                 push_undo, progress, error);

  clay_selection_resume (selection);

  return retval;
}

static gboolean
clay_selection_stroke (ClayItem           *item,
                       ClayDrawable       *drawable,
                       ClayStrokeOptions  *stroke_options,
                       gboolean            push_undo,
                       ClayProgress       *progress,
                       GError            **error)
{
  ClaySelection      *selection = CLAY_SELECTION (item);
  const ClayBoundSeg *dummy_in;
  const ClayBoundSeg *dummy_out;
  gint                num_dummy_in;
  gint                num_dummy_out;
  gboolean            retval;

  if (! clay_channel_boundary (CLAY_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("There is no selection to stroke."));
      return FALSE;
    }

  clay_selection_suspend (selection);

  retval = CLAY_ITEM_CLASS (parent_class)->stroke (item, drawable,
                                                   stroke_options,
                                                   push_undo, progress, error);

  clay_selection_resume (selection);

  return retval;
}

static void
clay_selection_convert_type (ClayDrawable      *drawable,
                             ClayImage         *dest_image,
                             const Babl        *new_format,
                             ClayColorProfile  *src_profile,
                             ClayColorProfile  *dest_profile,
                             GeglDitherMethod   layer_dither_type,
                             GeglDitherMethod   mask_dither_type,
                             gboolean           push_undo,
                             ClayProgress      *progress)
{
  new_format =
    clay_babl_mask_format (clay_babl_format_get_precision (new_format));

  CLAY_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);
}

static void
clay_selection_invalidate_boundary (ClayDrawable *drawable)
{
  ClayImage *image = clay_item_get_image (CLAY_ITEM (drawable));
  GList     *layers;

  /*  Turn the current selection off  */
  clay_image_selection_invalidate (image);

  CLAY_DRAWABLE_CLASS (parent_class)->invalidate_boundary (drawable);

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layers = clay_image_get_selected_layers (image);

  if (g_list_length (layers) == 1 && clay_layer_is_floating_sel (layers->data))
    {
      clay_drawable_update (CLAY_DRAWABLE (layers->data), 0, 0, -1, -1);
    }

#if 0
  /*  invalidate the preview  */
  drawable->private->preview_valid = FALSE;
#endif
}

static gboolean
clay_selection_boundary (ClayChannel         *channel,
                         const ClayBoundSeg **segs_in,
                         const ClayBoundSeg **segs_out,
                         gint                *num_segs_in,
                         gint                *num_segs_out,
                         gint                 unused1,
                         gint                 unused2,
                         gint                 unused3,
                         gint                 unused4)
{
  ClayImage    *image = clay_item_get_image (CLAY_ITEM (channel));
  ClayLayer    *floating_selection;
  GList        *drawables;
  GList        *layers;
  gboolean      channel_selected;

  drawables = clay_image_get_selected_drawables (image);
  channel_selected = (drawables && CLAY_IS_CHANNEL (drawables->data));
  g_list_free (drawables);

  if ((floating_selection = clay_image_get_floating_selection (image)))
    {
      /*  If there is a floating selection, then
       *  we need to do some slightly different boundaries.
       *  Instead of inside and outside boundaries being defined
       *  by the extents of the layer, the inside boundary (the one
       *  that actually marches and is black/white) is the boundary of
       *  the floating selection.  The outside boundary (doesn't move,
       *  is black/gray) is defined as the normal selection mask
       */

      /*  Find the selection mask boundary  */
      CLAY_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                   segs_in, segs_out,
                                                   num_segs_in, num_segs_out,
                                                   0, 0, 0, 0);

      /*  Find the floating selection boundary  */
      *segs_in = floating_sel_boundary (floating_selection, num_segs_in);

      return TRUE;
    }
  else if (channel_selected)
    {
      /*  Otherwise, return the boundary...if a channels are selected  */

      return CLAY_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          0, 0,
                                                          clay_image_get_width  (image),
                                                          clay_image_get_height (image));
    }
  else if ((layers = clay_image_get_selected_layers (image)))
    {
      /*  If layers are selected, we return multiple boundaries based
       *  on the extents
       */
      GList *iter;
      gint    x1, y1;
      gint    x2       = G_MININT;
      gint    y2       = G_MININT;
      gint    offset_x = G_MAXINT;
      gint    offset_y = G_MAXINT;

      for (iter = layers; iter; iter = iter->next)
        {
          gint item_off_x, item_off_y;
          gint item_x2, item_y2;

          clay_item_get_offset (iter->data, &item_off_x, &item_off_y);
          offset_x = MIN (offset_x, item_off_x);
          offset_y = MIN (offset_y, item_off_y);

          item_x2 = item_off_x + clay_item_get_width (CLAY_ITEM (iter->data));
          item_y2 = item_off_y + clay_item_get_height (CLAY_ITEM (iter->data));
          x2 = MAX (x2, item_x2);
          y2 = MAX (y2, item_y2);
        }

      x1 = CLAMP (offset_x, 0, clay_image_get_width  (image));
      y1 = CLAMP (offset_y, 0, clay_image_get_height (image));
      x2 = CLAMP (x2, 0, clay_image_get_width (image));
      y2 = CLAMP (y2, 0, clay_image_get_height (image));

      return CLAY_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          x1, y1, x2, y2);
    }

  *segs_in      = NULL;
  *segs_out     = NULL;
  *num_segs_in  = 0;
  *num_segs_out = 0;

  return FALSE;
}

static gboolean
clay_selection_is_empty (ClayChannel *channel)
{
  ClaySelection *selection = CLAY_SELECTION (channel);

  /*  in order to allow stroking of selections, we need to pretend here
   *  that the selection mask is empty so that it doesn't mask the paint
   *  during the stroke operation.
   */
  if (selection->suspend_count > 0)
    return TRUE;

  return CLAY_CHANNEL_CLASS (parent_class)->is_empty (channel);
}

static void
clay_selection_feather (ClayChannel *channel,
                        gdouble      radius_x,
                        gdouble      radius_y,
                        gboolean     edge_lock,
                        gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->feather (channel, radius_x, radius_y,
                                              edge_lock, push_undo);
}

static void
clay_selection_sharpen (ClayChannel *channel,
                        gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->sharpen (channel, push_undo);
}

static void
clay_selection_clear (ClayChannel *channel,
                      const gchar *undo_desc,
                      gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->clear (channel, undo_desc, push_undo);
}

static void
clay_selection_all (ClayChannel *channel,
                    gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->all (channel, push_undo);
}

static void
clay_selection_invert (ClayChannel *channel,
                       gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->invert (channel, push_undo);
}

static void
clay_selection_border (ClayChannel            *channel,
                       gint                    radius_x,
                       gint                    radius_y,
                       ClayChannelBorderStyle  style,
                       gboolean                edge_lock,
                       gboolean                push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->border (channel,
                                             radius_x, radius_y,
                                             style, edge_lock,
                                             push_undo);
}

static void
clay_selection_grow (ClayChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->grow (channel,
                                           radius_x, radius_y,
                                           push_undo);
}

static void
clay_selection_shrink (ClayChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->shrink (channel,
                                             radius_x, radius_y, edge_lock,
                                             push_undo);
}

static void
clay_selection_flood (ClayChannel *channel,
                      gboolean     push_undo)
{
  CLAY_CHANNEL_CLASS (parent_class)->flood (channel, push_undo);
}


/*  public functions  */

ClayChannel *
clay_selection_new (ClayImage *image,
                    gint       width,
                    gint       height)
{
  GeglColor   *black = gegl_color_new ("black");
  ClayChannel *channel;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  clay_color_set_alpha (black, 0.5);
  channel = CLAY_CHANNEL (clay_drawable_new (CLAY_TYPE_SELECTION,
                                             image, NULL,
                                             0, 0, width, height,
                                             clay_image_get_mask_format (image)));

  clay_channel_set_color (channel, black, FALSE);
  clay_channel_set_show_masked (channel, TRUE);

  channel->x2 = width;
  channel->y2 = height;

  g_object_unref (black);

  return channel;
}

gint
clay_selection_suspend (ClaySelection *selection)
{
  g_return_val_if_fail (CLAY_IS_SELECTION (selection), 0);

  selection->suspend_count++;

  return selection->suspend_count;
}

gint
clay_selection_resume (ClaySelection *selection)
{
  g_return_val_if_fail (CLAY_IS_SELECTION (selection), 0);
  g_return_val_if_fail (selection->suspend_count > 0, 0);

  selection->suspend_count--;

  return selection->suspend_count;
}

GeglBuffer *
clay_selection_extract (ClaySelection *selection,
                        GList         *pickables,
                        ClayContext   *context,
                        gboolean       cut_image,
                        gboolean       keep_indexed,
                        gboolean       add_alpha,
                        gint          *offset_x,
                        gint          *offset_y,
                        GError       **error)
{
  ClayImage    *image      = NULL;
  ClayImage    *temp_image = NULL;
  ClayPickable *pickable   = NULL;
  GeglBuffer   *src_buffer;
  GeglBuffer   *dest_buffer;
  GList        *iter;
  const Babl   *src_format;
  const Babl   *dest_format;
  gint          x1, y1, x2, y2;
  gboolean      non_empty;
  gint          off_x, off_y;

  g_return_val_if_fail (CLAY_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (pickables != NULL, NULL);

  for (iter = pickables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_PICKABLE (iter->data), NULL);

      if (CLAY_IS_ITEM (iter->data))
        g_return_val_if_fail (clay_item_is_attached (iter->data), NULL);

      if (! image)
        image = clay_pickable_get_image (iter->data);
      else
        g_return_val_if_fail (image == clay_pickable_get_image (iter->data), NULL);
    }

  if (g_list_length (pickables) == 1)
    {
      pickable = pickables->data;
    }
  else
    {
      for (iter = pickables; iter; iter = iter->next)
        g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), NULL);

      temp_image = clay_image_new_from_drawables (image->clay, pickables, TRUE, FALSE);
      selection  = CLAY_SELECTION (clay_image_get_mask (temp_image));

      pickable   = CLAY_PICKABLE (temp_image);

      /* Don't cut from the temporary image. */
      cut_image = FALSE;
    }

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  if (CLAY_IS_DRAWABLE (pickable))
    {
      non_empty = clay_item_mask_bounds (CLAY_ITEM (pickable),
                                         &x1, &y1, &x2, &y2);

      clay_item_get_offset (CLAY_ITEM (pickable), &off_x, &off_y);
    }
  else
    {
      non_empty = clay_item_bounds (CLAY_ITEM (selection),
                                    &x1, &y1, &x2, &y2);
      x2 += x1;
      y2 += y1;

      off_x = 0;
      off_y = 0;

      /* can't cut from non-drawables, fall back to copy */
      cut_image = FALSE;
    }

  if (non_empty && ((x1 == x2) || (y1 == y2)))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Unable to cut or copy because the "
                             "selected region is empty."));

      if (temp_image)
        g_object_unref (temp_image);

      return NULL;
    }

  /*  If there is a selection, we must add alpha because the selection
   *  could have any shape.
   */
  if (non_empty)
    add_alpha = TRUE;

  src_format = clay_pickable_get_format (pickable);

  /*  How many bytes in the temp buffer?  */
  if (babl_format_is_palette (src_format) && ! keep_indexed)
    {
      dest_format = clay_image_get_format (image, CLAY_RGB,
                                           clay_image_get_precision (image),
                                           add_alpha ||
                                           babl_format_has_alpha (src_format),
                                           babl_format_get_space (src_format));
    }
  else
    {
      if (add_alpha)
        dest_format = clay_pickable_get_format_with_alpha (pickable);
      else
        dest_format = src_format;
    }

  clay_pickable_flush (pickable);

  src_buffer = clay_pickable_get_buffer (pickable);

  /*  Allocate the temp buffer  */
  dest_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                                 dest_format);

  /*  First, copy the pixels, possibly doing INDEXED->RGB and adding alpha  */
  clay_gegl_buffer_copy (src_buffer,  GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                         GEGL_ABYSS_NONE,
                         dest_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

  if (non_empty)
    {
      /*  If there is a selection, mask the dest_buffer with it  */

      GeglBuffer *mask_buffer;

      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (selection));

      clay_gegl_apply_opacity (dest_buffer, NULL, NULL, dest_buffer,
                               mask_buffer,
                               - (off_x + x1),
                               - (off_y + y1),
                               1.0);

      if (cut_image)
        {
          clay_drawable_edit_clear (CLAY_DRAWABLE (pickable), context);
        }
    }
  else if (cut_image)
    {
      /*  If we're cutting without selection, remove either the layer
       *  (or floating selection), the layer mask, or the channel
       */
      if (CLAY_IS_LAYER (pickable))
        {
          clay_image_remove_layer (image, CLAY_LAYER (pickable),
                                   TRUE, NULL);
        }
      else if (CLAY_IS_LAYER_MASK (pickable))
        {
          clay_layer_apply_mask (clay_layer_mask_get_layer (CLAY_LAYER_MASK (pickable)),
                                 CLAY_MASK_DISCARD, TRUE);
        }
      else if (CLAY_IS_CHANNEL (pickable))
        {
          clay_image_remove_channel (image, CLAY_CHANNEL (pickable),
                                     TRUE, NULL);
        }
    }

  *offset_x = x1 + off_x;
  *offset_y = y1 + off_y;

  if (temp_image)
    g_object_unref (temp_image);

  return dest_buffer;
}

ClayLayer *
clay_selection_float (ClaySelection *selection,
                      GList         *drawables,
                      ClayContext   *context,
                      gboolean       cut_image,
                      gint           off_x,
                      gint           off_y,
                      GError       **error)
{
  ClayImage        *image;
  ClayLayer        *layer;
  GeglBuffer       *buffer;
  ClayColorProfile *profile;
  ClayImage        *temp_image = NULL;
  const Babl       *format     = NULL;
  GList            *iter;
  gint              x1, y1;
  gint              x2, y2;

  g_return_val_if_fail (CLAY_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (clay_item_is_attached (iter->data), NULL);

      if (! format)
        format = clay_drawable_get_format_with_alpha (iter->data);
      else
        g_return_val_if_fail (format == clay_drawable_get_format_with_alpha (iter->data),
                              NULL);
    }

  image = clay_item_get_image (CLAY_ITEM (selection));

  /*  Make sure there is a region to float...  */
  for (iter = drawables; iter; iter = iter->next)
    {
      if (clay_item_mask_bounds (iter->data, &x1, &y1, &x2, &y2) &&
          x1 != x2 && y1 != y2)
        break;
    }
  if (iter == NULL)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot float selection because the selected "
                             "region is empty."));
      return NULL;
    }

  /*  Start an undo group  */
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_FS_FLOAT,
                               C_("undo-type", "Float Selection"));

  /*  Cut or copy the selected region  */
  buffer = clay_selection_extract (selection, drawables, context,
                                   cut_image, FALSE, TRUE,
                                   &x1, &y1, NULL);

  profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (drawables->data));

  /*  Clear the selection  */
  clay_channel_clear (CLAY_CHANNEL (selection), NULL, TRUE);

  /* Create a new layer from the buffer, using the drawables' type
   *  because it may be different from the image's type if we cut from
   *  a channel or layer mask
   */
  layer = clay_layer_new_from_gegl_buffer (buffer, image, format,
                                           _("Floated Layer"),
                                           CLAY_OPACITY_OPAQUE,
                                           clay_image_get_default_new_layer_mode (image),
                                           profile);

  /*  Set the offsets  */
  clay_item_set_offset (CLAY_ITEM (layer), x1 + off_x, y1 + off_y);

  /*  Free the temp buffer  */
  g_object_unref (buffer);

  /*  Add the floating layer to the image  */
  floating_sel_attach (layer, drawables->data);

  /*  End an undo group  */
  clay_image_undo_group_end (image);

  /*  invalidate the image's boundary variables  */
  CLAY_CHANNEL (selection)->boundary_known = FALSE;

  if (temp_image)
    g_object_unref (temp_image);

  return layer;
}
