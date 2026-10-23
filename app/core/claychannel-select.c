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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-mask-combine.h"

#include "clay.h"
#include "claychannel.h"
#include "claychannel-select.h"
#include "claychannel-combine.h"
#include "claycontainer.h"
#include "clayimage.h"
#include "clayimage-new.h"
#include "claypickable.h"
#include "claypickable-contiguous-region.h"
#include "clayscanconvert.h"

#include "path/claypath.h"
#include "path/claystroke.h"

#include "clay-intl.h"


/*  basic selection functions  */

void
clay_channel_select_rectangle (ClayChannel    *channel,
                               gint            x,
                               gint            y,
                               gint            w,
                               gint            h,
                               ClayChannelOps  op,
                               gboolean        feather,
                               gdouble         feather_radius_x,
                               gdouble         feather_radius_y,
                               gboolean        push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));

  if (push_undo)
    clay_channel_push_undo (channel, C_("undo-type", "Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      ClayItem   *item = CLAY_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                clay_item_get_width  (item),
                                                clay_item_get_height (item)),
                                babl_format ("Y float"));

      clay_gegl_mask_combine_rect (add_on, CLAY_CHANNEL_OP_REPLACE, x, y, w, h);

      clay_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      clay_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      clay_channel_combine_rect (channel, op, x, y, w, h);
    }
}

void
clay_channel_select_ellipse (ClayChannel    *channel,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             ClayChannelOps  op,
                             gboolean        antialias,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y,
                             gboolean        push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));

  if (push_undo)
    clay_channel_push_undo (channel, C_("undo-type", "Ellipse Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      ClayItem   *item = CLAY_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                clay_item_get_width  (item),
                                                clay_item_get_height (item)),
                                babl_format ("Y float"));

      clay_gegl_mask_combine_ellipse (add_on, CLAY_CHANNEL_OP_REPLACE,
                                      x, y, w, h, antialias);

      clay_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      clay_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      clay_channel_combine_ellipse (channel, op, x, y, w, h, antialias);
    }
}

void
clay_channel_select_round_rect (ClayChannel         *channel,
                                gint                 x,
                                gint                 y,
                                gint                 w,
                                gint                 h,
                                gdouble              corner_radius_x,
                                gdouble              corner_radius_y,
                                ClayChannelOps       op,
                                gboolean             antialias,
                                gboolean             feather,
                                gdouble              feather_radius_x,
                                gdouble              feather_radius_y,
                                gboolean             push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));

  if (push_undo)
    clay_channel_push_undo (channel, C_("undo-type", "Rounded Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      ClayItem   *item = CLAY_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                clay_item_get_width  (item),
                                                clay_item_get_height (item)),
                                babl_format ("Y float"));

      clay_gegl_mask_combine_ellipse_rect (add_on, CLAY_CHANNEL_OP_REPLACE,
                                           x, y, w, h,
                                           corner_radius_x, corner_radius_y,
                                           antialias);

      clay_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      clay_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      clay_channel_combine_ellipse_rect (channel, op, x, y, w, h,
                                         corner_radius_x, corner_radius_y,
                                         antialias);
    }
}

/*  select by ClayScanConvert functions  */

void
clay_channel_select_scan_convert (ClayChannel     *channel,
                                  const gchar     *undo_desc,
                                  ClayScanConvert *scan_convert,
                                  gint             offset_x,
                                  gint             offset_y,
                                  ClayChannelOps   op,
                                  gboolean         antialias,
                                  gboolean         feather,
                                  gdouble          feather_radius_x,
                                  gdouble          feather_radius_y,
                                  gboolean         push_undo)
{
  ClayItem   *item;
  GeglBuffer *add_on;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (scan_convert != NULL);

  if (push_undo)
    clay_channel_push_undo (channel, undo_desc);

  item = CLAY_ITEM (channel);

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            clay_item_get_width  (item),
                                            clay_item_get_height (item)),
                            babl_format ("Y float"));

  clay_scan_convert_render (scan_convert, add_on,
                            offset_x, offset_y, antialias);

  if (feather)
    clay_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                             feather_radius_x,
                             feather_radius_y,
                             TRUE);

  clay_channel_combine_buffer (channel, add_on, op, 0, 0);
  g_object_unref (add_on);
}

void
clay_channel_select_polygon (ClayChannel       *channel,
                             const gchar       *undo_desc,
                             gint               n_points,
                             const ClayVector2 *points,
                             ClayChannelOps     op,
                             gboolean           antialias,
                             gboolean           feather,
                             gdouble            feather_radius_x,
                             gdouble            feather_radius_y,
                             gboolean           push_undo)
{
  ClayScanConvert *scan_convert;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);

  scan_convert = clay_scan_convert_new ();

  clay_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

  clay_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                    op, antialias, feather,
                                    feather_radius_x, feather_radius_y,
                                    push_undo);

  clay_scan_convert_free (scan_convert);
}

void
clay_channel_select_path (ClayChannel    *channel,
                          const gchar    *undo_desc,
                          ClayPath       *path,
                          ClayChannelOps  op,
                          gboolean        antialias,
                          gboolean        feather,
                          gdouble         feather_radius_x,
                          gdouble         feather_radius_y,
                          gboolean        push_undo)
{
  const ClayBezierDesc *bezier;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (CLAY_IS_PATH (path));

  bezier = clay_path_get_bezier (path);

  if (bezier && bezier->num_data > 4)
    {
      ClayScanConvert *scan_convert;

      scan_convert = clay_scan_convert_new ();
      clay_scan_convert_add_bezier (scan_convert, bezier);

      clay_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                        op, antialias, feather,
                                        feather_radius_x, feather_radius_y,
                                        push_undo);

      clay_scan_convert_free (scan_convert);
    }
}


/*  select by ClayChannel functions  */

void
clay_channel_select_buffer (ClayChannel    *channel,
                            const gchar    *undo_desc,
                            GeglBuffer     *add_on,
                            gint            offset_x,
                            gint            offset_y,
                            ClayChannelOps  op,
                            gboolean        feather,
                            gdouble         feather_radius_x,
                            gdouble         feather_radius_y)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (add_on));

  clay_channel_push_undo (channel, undo_desc);

  if (feather)
    {
      ClayItem   *item = CLAY_ITEM (channel);
      GeglBuffer *add_on2;

      add_on2 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 clay_item_get_width  (item),
                                                 clay_item_get_height (item)),
                                 babl_format ("Y float"));

      clay_gegl_mask_combine_buffer (add_on2, add_on,
                                     CLAY_CHANNEL_OP_REPLACE,
                                     offset_x, offset_y);

      clay_gegl_apply_feather (add_on2, NULL, NULL, add_on2, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      clay_channel_combine_buffer (channel, add_on2, op, 0, 0);
      g_object_unref (add_on2);
    }
  else
    {
      clay_channel_combine_buffer (channel, add_on, op, offset_x, offset_y);
    }
}

void
clay_channel_select_channel (ClayChannel    *channel,
                             const gchar    *undo_desc,
                             ClayChannel    *add_on,
                             gint            offset_x,
                             gint            offset_y,
                             ClayChannelOps  op,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (CLAY_IS_CHANNEL (add_on));

  clay_channel_select_buffer (channel, undo_desc,
                              clay_drawable_get_buffer (CLAY_DRAWABLE (add_on)),
                              offset_x, offset_y, op,
                              feather,
                              feather_radius_x, feather_radius_y);
}

void
clay_channel_select_alpha (ClayChannel    *channel,
                           ClayDrawable   *drawable,
                           ClayChannelOps  op,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  ClayItem    *item;
  ClayChannel *add_on;
  gint         off_x, off_y;
  const gchar *undo_desc = NULL;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  item = CLAY_ITEM (channel);

  if (clay_drawable_has_alpha (drawable))
    {
      add_on = clay_channel_new_from_alpha (clay_item_get_image (item),
                                            drawable, NULL, NULL);
    }
  else
    {
      /*  no alpha is equivalent to completely opaque alpha,
       *  so simply select the whole layer's extents.  --mitch
       */
      add_on = clay_channel_new_mask (clay_item_get_image (item),
                                      clay_item_get_width  (CLAY_ITEM (drawable)),
                                      clay_item_get_height (CLAY_ITEM (drawable)));
      clay_channel_all (add_on, FALSE);
    }

  switch (op)
    {
    case CLAY_CHANNEL_OP_ADD:
      undo_desc = C_("undo-type", "Add Alpha to Selection");
      break;
    case CLAY_CHANNEL_OP_SUBTRACT:
      undo_desc = C_("undo-type", "Subtract Alpha from Selection");
      break;
    case CLAY_CHANNEL_OP_REPLACE:
      undo_desc = C_("undo-type", "Alpha to Selection");
      break;
    case CLAY_CHANNEL_OP_INTERSECT:
      undo_desc = C_("undo-type", "Intersect Alpha with Selection");
      break;
    }

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  clay_channel_select_channel (channel, undo_desc, add_on,
                               off_x, off_y,
                               op, feather,
                               feather_radius_x,
                               feather_radius_y);
  g_object_unref (add_on);
}

void
clay_channel_select_component (ClayChannel     *channel,
                               ClayChannelType  component,
                               ClayChannelOps   op,
                               gboolean         feather,
                               gdouble          feather_radius_x,
                               gdouble          feather_radius_y)
{
  ClayItem    *item;
  ClayChannel *add_on;
  const gchar *desc;
  gchar       *undo_desc;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));

  item = CLAY_ITEM (channel);

  add_on = clay_channel_new_from_component (clay_item_get_image (item),
                                            component, NULL, NULL);

  if (feather)
    clay_channel_feather (add_on,
                          feather_radius_x,
                          feather_radius_y,
                          TRUE,
                          FALSE /* no undo */);

  clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "%s Channel to Selection"), desc);

  clay_channel_select_channel (channel, undo_desc, add_on,
                               0, 0, op,
                               FALSE, 0.0, 0.0);

  g_free (undo_desc);
  g_object_unref (add_on);
}

void
clay_channel_select_fuzzy (ClayChannel         *channel,
                           ClayDrawable        *drawable,
                           gboolean             sample_merged,
                           gint                 x,
                           gint                 y,
                           gfloat               threshold,
                           gboolean             select_transparent,
                           ClaySelectCriterion  select_criterion,
                           gboolean             diagonal_neighbors,
                           ClayChannelOps       op,
                           gboolean             antialias,
                           gboolean             feather,
                           gdouble              feather_radius_x,
                           gdouble              feather_radius_y)
{
  ClayPickable *pickable;
  GeglBuffer   *add_on;
  gint          add_on_x = 0;
  gint          add_on_y = 0;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  if (sample_merged)
    pickable = CLAY_PICKABLE (clay_item_get_image (CLAY_ITEM (drawable)));
  else
    pickable = CLAY_PICKABLE (drawable);

  add_on = clay_pickable_contiguous_region_by_seed (pickable,
                                                    antialias,
                                                    threshold,
                                                    select_transparent,
                                                    select_criterion,
                                                    diagonal_neighbors,
                                                    x, y);

  if (! sample_merged)
    clay_item_get_offset (CLAY_ITEM (drawable), &add_on_x, &add_on_y);

  clay_channel_select_buffer (channel, C_("undo-type", "Fuzzy Select"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}

void
clay_channel_select_by_color (ClayChannel         *channel,
                              GList               *drawables,
                              gboolean             sample_merged,
                              GeglColor           *color,
                              gfloat               threshold,
                              gboolean             select_transparent,
                              ClaySelectCriterion  select_criterion,
                              ClayChannelOps       op,
                              gboolean             antialias,
                              gboolean             feather,
                              gdouble              feather_radius_x,
                              gdouble              feather_radius_y)
{
  ClayPickable  *pickable;
  GeglBuffer    *add_on;
  ClayImage     *image;
  ClayImage     *sel_image = NULL;
  gint           add_on_x  = 0;
  gint           add_on_y  = 0;
  GeglRectangle  roi       = { 0, 0, 0, 0 };

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (color != NULL);

  image = clay_item_get_image (drawables->data);
  if (sample_merged)
    {
      pickable = CLAY_PICKABLE (image);
    }
  else
    {
      if (g_list_length (drawables) == 1)
        {
          pickable = CLAY_PICKABLE (drawables->data);
        }
      else
        {
          sel_image = clay_image_new_from_drawables (image->clay, drawables, FALSE, FALSE);
          clay_container_remove (image->clay->images, CLAY_OBJECT (sel_image));

          pickable = CLAY_PICKABLE (sel_image);
          clay_pickable_flush (pickable);
        }
    }
  
    /* if region of interest provided, only scan the intersection */
    if (op == CLAY_CHANNEL_OP_INTERSECT && ! clay_channel_is_empty (channel))
      clay_item_bounds (CLAY_ITEM (channel), &roi.x , &roi.y, &roi.width,
                        &roi.height);

  add_on = clay_pickable_contiguous_region_by_color (pickable,
                                                     antialias,
                                                     threshold,
                                                     select_transparent,
                                                     select_criterion,
                                                     color,
                                                     &roi);

  if (! sample_merged && ! sel_image)
    clay_item_get_offset (CLAY_ITEM (drawables->data), &add_on_x, &add_on_y);

  clay_channel_select_buffer (channel, C_("undo-type", "Select by Color"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);

  g_object_unref (add_on);
  if (sel_image)
    g_object_unref (sel_image);

}

void
clay_channel_select_by_index (ClayChannel    *channel,
                              ClayDrawable   *drawable,
                              gint            index,
                              ClayChannelOps  op,
                              gboolean        feather,
                              gdouble         feather_radius_x,
                              gdouble         feather_radius_y)
{
  GeglBuffer *add_on;
  gint        add_on_x = 0;
  gint        add_on_y = 0;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_drawable_is_indexed (drawable));

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            clay_item_get_width  (CLAY_ITEM (drawable)),
                                            clay_item_get_height (CLAY_ITEM (drawable))),
                            babl_format ("Y float"));

  clay_gegl_index_to_mask (clay_drawable_get_buffer (drawable), NULL,
                           clay_drawable_get_format_without_alpha (drawable),
                           add_on, NULL,
                           index);

  clay_item_get_offset (CLAY_ITEM (drawable), &add_on_x, &add_on_y);

  clay_channel_select_buffer (channel, C_("undo-type", "Select by Indexed Color"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}
