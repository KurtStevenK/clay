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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clayapplicator.h"

#include "claychannel.h"
#include "claycontainer.h"
#include "claydrawable-floating-selection.h"
#include "claydrawable-filters.h"
#include "claydrawable-private.h"
#include "clayimage.h"
#include "claylayer.h"

#include "clay-log.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void    clay_drawable_remove_fs_filter             (ClayDrawable      *drawable);
static void    clay_drawable_sync_fs_filter               (ClayDrawable      *drawable);

static void    clay_drawable_fs_notify                    (GObject           *object,
                                                           const GParamSpec  *pspec,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_lock_position_changed     (ClayDrawable      *signal_drawable,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_format_changed            (ClayDrawable      *signal_drawable,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_affect_changed            (ClayImage         *image,
                                                           ClayChannelType    channel,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_mask_changed              (ClayImage         *image,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_visibility_changed        (ClayLayer         *fs,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_excludes_backdrop_changed (ClayLayer         *fs,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_bounding_box_changed      (ClayLayer         *fs,
                                                           ClayDrawable      *drawable);
static void    clay_drawable_fs_update                    (ClayLayer         *fs,
                                                           gint               x,
                                                           gint               y,
                                                           gint               width,
                                                           gint               height,
                                                           ClayDrawable      *drawable);


/*  public functions  */

ClayLayer *
clay_drawable_get_floating_sel (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return drawable->private->floating_selection;
}

void
clay_drawable_attach_floating_sel (ClayDrawable *drawable,
                                   ClayLayer    *fs)
{
  ClayImage *image;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (clay_drawable_get_floating_sel (drawable) == NULL);
  g_return_if_fail (CLAY_IS_LAYER (fs));

  CLAY_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = clay_item_get_image (CLAY_ITEM (drawable));

  drawable->private->floating_selection = fs;
  clay_image_set_floating_selection (image, fs);

  /*  clear the selection  */
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (fs));

  clay_item_bind_visible_to_active (CLAY_ITEM (fs), FALSE);
  clay_filter_set_active (CLAY_FILTER (fs), FALSE);

  _clay_drawable_add_floating_sel_filter (drawable);

  g_signal_connect (fs, "visibility-changed",
                    G_CALLBACK (clay_drawable_fs_visibility_changed),
                    drawable);
  g_signal_connect (fs, "excludes-backdrop-changed",
                    G_CALLBACK (clay_drawable_fs_excludes_backdrop_changed),
                    drawable);
  g_signal_connect (fs, "bounding-box-changed",
                    G_CALLBACK (clay_drawable_fs_bounding_box_changed),
                    drawable);
  g_signal_connect (fs, "update",
                    G_CALLBACK (clay_drawable_fs_update),
                    drawable);

  clay_drawable_fs_update (fs,
                           0, 0,
                           clay_item_get_width  (CLAY_ITEM (fs)),
                           clay_item_get_height (CLAY_ITEM (fs)),
                           drawable);
}

void
clay_drawable_detach_floating_sel (ClayDrawable *drawable)
{
  ClayImage *image;
  ClayLayer *fs;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_drawable_get_floating_sel (drawable) != NULL);

  CLAY_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = clay_item_get_image (CLAY_ITEM (drawable));
  fs    = drawable->private->floating_selection;

  clay_drawable_remove_fs_filter (drawable);

  g_signal_handlers_disconnect_by_func (fs,
                                        clay_drawable_fs_visibility_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        clay_drawable_fs_excludes_backdrop_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        clay_drawable_fs_bounding_box_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        clay_drawable_fs_update,
                                        drawable);

  clay_drawable_fs_update (fs,
                           0, 0,
                           clay_item_get_width  (CLAY_ITEM (fs)),
                           clay_item_get_height (CLAY_ITEM (fs)),
                           drawable);

  clay_item_bind_visible_to_active (CLAY_ITEM (fs), TRUE);

  /*  clear the selection  */
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (fs));

  clay_image_set_floating_selection (image, NULL);
  drawable->private->floating_selection = NULL;
}

ClayFilter *
clay_drawable_get_floating_sel_filter (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_drawable_get_floating_sel (drawable) != NULL, NULL);

  /* Ensure that the graph is constructed before the filter is used.
   * Otherwise, we rely on the projection to cause the graph to be
   * constructed, which fails for images that aren't displayed.
   */
  clay_filter_get_node (CLAY_FILTER (drawable));

  return drawable->private->fs_filter;
}


/*  private functions  */

void
_clay_drawable_add_floating_sel_filter (ClayDrawable *drawable)
{
  ClayDrawablePrivate *private = drawable->private;
  ClayContainer       *filters = clay_drawable_get_filters (drawable);
  ClayImage           *image   = clay_item_get_image (CLAY_ITEM (drawable));
  ClayLayer           *fs      = clay_drawable_get_floating_sel (drawable);
  GeglNode            *node;
  GeglNode            *fs_source;
  gint                 end_index;
  gint                 index;

  if (! private->source_node)
    return;

  private->fs_filter = clay_filter_new (_("Floating Selection"));
  clay_viewable_set_icon_name (CLAY_VIEWABLE (private->fs_filter),
                               "clay-floating-selection");

  node = clay_filter_get_node (private->fs_filter);

  fs_source = clay_drawable_get_source_node (CLAY_DRAWABLE (fs));

  /* rip the fs' source node out of its graph */
  if (fs->layer_offset_node)
    {
      gegl_node_disconnect (fs->layer_offset_node, "input");
      gegl_node_remove_child (clay_filter_get_node (CLAY_FILTER (fs)),
                              fs_source);
    }

  gegl_node_add_child (node, fs_source);

  private->fs_applicator = clay_applicator_new (node);

  clay_filter_set_applicator (private->fs_filter, private->fs_applicator);

  clay_applicator_set_cache (private->fs_applicator, TRUE);

  private->fs_crop_node = gegl_node_new_child (node,
                                               "operation", "gegl:nop",
                                               NULL);

  gegl_node_link (fs_source, private->fs_crop_node);
  gegl_node_connect (private->fs_crop_node, "output",
                     node,                  "aux");

  clay_drawable_add_filter (drawable, private->fs_filter);

  /* Move the floating selection below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  end_index = clay_container_get_n_children (filters) - 1;
  index     =
    clay_container_get_child_index (filters, CLAY_OBJECT (private->fs_filter));
  if (end_index > 0 && index != end_index)
    clay_container_reorder (filters, CLAY_OBJECT (private->fs_filter),
                            end_index);

  g_signal_connect (fs, "notify",
                    G_CALLBACK (clay_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-x",
                    G_CALLBACK (clay_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-y",
                    G_CALLBACK (clay_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "lock-position-changed",
                    G_CALLBACK (clay_drawable_fs_lock_position_changed),
                    drawable);
  g_signal_connect (drawable, "format-changed",
                    G_CALLBACK (clay_drawable_fs_format_changed),
                    drawable);
  g_signal_connect (image, "component-active-changed",
                    G_CALLBACK (clay_drawable_fs_affect_changed),
                    drawable);
  g_signal_connect (image, "mask-changed",
                    G_CALLBACK (clay_drawable_fs_mask_changed),
                    drawable);

  clay_drawable_sync_fs_filter (drawable);
}


/*  private functions  */

static void
clay_drawable_remove_fs_filter (ClayDrawable *drawable)
{
  ClayDrawablePrivate *private = drawable->private;
  ClayImage           *image   = clay_item_get_image (CLAY_ITEM (drawable));
  ClayLayer           *fs      = clay_drawable_get_floating_sel (drawable);

  if (private->fs_filter)
    {
      GeglNode *node;
      GeglNode *fs_source;

      g_signal_handlers_disconnect_by_func (fs,
                                            clay_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_fs_lock_position_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_fs_format_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            clay_drawable_fs_affect_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            clay_drawable_fs_mask_changed,
                                            drawable);

      clay_drawable_remove_filter (drawable, private->fs_filter);

      node = clay_filter_get_node (private->fs_filter);

      fs_source = clay_drawable_get_source_node (CLAY_DRAWABLE (fs));

      gegl_node_remove_child (node, fs_source);

      /* plug the fs' source node back into its graph */
      if (fs->layer_offset_node)
        {
          gegl_node_add_child (clay_filter_get_node (CLAY_FILTER (fs)),
                               fs_source);
          gegl_node_link (fs_source, fs->layer_offset_node);
        }

      g_clear_object (&private->fs_filter);
      g_clear_object (&private->fs_applicator);

      private->fs_crop_node = NULL;

      clay_drawable_update_bounding_box (drawable);
    }
}

static void
clay_drawable_sync_fs_filter (ClayDrawable *drawable)
{
  ClayDrawablePrivate *private = drawable->private;
  ClayImage           *image   = clay_item_get_image (CLAY_ITEM (drawable));
  ClayChannel         *mask    = clay_image_get_mask (image);
  ClayLayer           *fs      = clay_drawable_get_floating_sel (drawable);
  gint                 off_x, off_y;
  gint                 fs_off_x, fs_off_y;

  clay_filter_set_active (private->fs_filter,
                          clay_item_get_visible (CLAY_ITEM (fs)));

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);
  clay_item_get_offset (CLAY_ITEM (fs), &fs_off_x, &fs_off_y);

  if (clay_item_get_clip (CLAY_ITEM (drawable), CLAY_TRANSFORM_RESIZE_ADJUST) ==
      CLAY_TRANSFORM_RESIZE_CLIP ||
      ! clay_drawable_has_alpha (drawable))
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:crop",
        "x",         (gdouble) (off_x - fs_off_x),
        "y",         (gdouble) (off_y - fs_off_y),
        "width",     (gdouble) clay_item_get_width  (CLAY_ITEM (drawable)),
        "height",    (gdouble) clay_item_get_height (CLAY_ITEM (drawable)),
        NULL);
    }
  else
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:nop",
        NULL);
    }

  clay_applicator_set_apply_offset (private->fs_applicator,
                                    fs_off_x - off_x,
                                    fs_off_y - off_y);

  if (clay_channel_is_empty (mask))
    {
      clay_applicator_set_mask_buffer (private->fs_applicator, NULL);
    }
  else
    {
      GeglBuffer *buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

      clay_applicator_set_mask_buffer (private->fs_applicator, buffer);
      clay_applicator_set_mask_offset (private->fs_applicator,
                                       -off_x, -off_y);
    }

  clay_applicator_set_opacity (private->fs_applicator,
                               clay_layer_get_opacity (fs));
  clay_applicator_set_mode (private->fs_applicator,
                            clay_layer_get_mode (fs),
                            clay_layer_get_blend_space (fs),
                            clay_layer_get_composite_space (fs),
                            clay_layer_get_composite_mode (fs));
  clay_applicator_set_affect (private->fs_applicator,
                              clay_drawable_get_active_mask (drawable));
  clay_applicator_set_output_format (private->fs_applicator,
                                     clay_drawable_get_format (drawable));

  clay_drawable_update_bounding_box (drawable);
}

static void
clay_drawable_fs_notify (GObject          *object,
                         const GParamSpec *pspec,
                         ClayDrawable     *drawable)
{
  if (! strcmp (pspec->name, "offset-x")        ||
      ! strcmp (pspec->name, "offset-y")        ||
      ! strcmp (pspec->name, "visible")         ||
      ! strcmp (pspec->name, "mode")            ||
      ! strcmp (pspec->name, "blend-space")     ||
      ! strcmp (pspec->name, "composite-space") ||
      ! strcmp (pspec->name, "composite-mode")  ||
      ! strcmp (pspec->name, "opacity"))
    {
      clay_drawable_sync_fs_filter (drawable);
    }
}

static void
clay_drawable_fs_lock_position_changed (ClayDrawable *signal_drawable,
                                        ClayDrawable *drawable)
{
  ClayLayer *fs = clay_drawable_get_floating_sel (drawable);

  clay_drawable_sync_fs_filter (drawable);

  clay_drawable_update (CLAY_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
clay_drawable_fs_format_changed (ClayDrawable *signal_drawable,
                                 ClayDrawable *drawable)
{
  ClayLayer *fs = clay_drawable_get_floating_sel (drawable);

  clay_drawable_sync_fs_filter (drawable);

  clay_drawable_update (CLAY_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
clay_drawable_fs_affect_changed (ClayImage       *image,
                                 ClayChannelType  channel,
                                 ClayDrawable    *drawable)
{
  ClayLayer *fs = clay_drawable_get_floating_sel (drawable);

  clay_drawable_sync_fs_filter (drawable);

  clay_drawable_update (CLAY_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
clay_drawable_fs_mask_changed (ClayImage    *image,
                               ClayDrawable *drawable)
{
  ClayLayer *fs = clay_drawable_get_floating_sel (drawable);

  clay_drawable_sync_fs_filter (drawable);

  clay_drawable_update (CLAY_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
clay_drawable_fs_visibility_changed (ClayLayer    *fs,
                                     ClayDrawable *drawable)
{
  if (clay_layer_get_excludes_backdrop (fs))
    clay_drawable_update (drawable, 0, 0, -1, -1);
  else
    clay_drawable_update (CLAY_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
clay_drawable_fs_excludes_backdrop_changed (ClayLayer    *fs,
                                            ClayDrawable *drawable)
{
  if (clay_item_get_visible (CLAY_ITEM (fs)))
    clay_drawable_update (drawable, 0, 0, -1, -1);
}

static void
clay_drawable_fs_bounding_box_changed (ClayLayer    *fs,
                                       ClayDrawable *drawable)
{
  clay_drawable_update_bounding_box (drawable);
}

static void
clay_drawable_fs_update (ClayLayer    *fs,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         ClayDrawable *drawable)
{
  GeglRectangle bounding_box;
  GeglRectangle rect;
  gint          fs_off_x, fs_off_y;
  gint          off_x, off_y;

  clay_item_get_offset (CLAY_ITEM (fs), &fs_off_x, &fs_off_y);
  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  bounding_box = clay_drawable_get_bounding_box (drawable);

  bounding_box.x += off_x;
  bounding_box.y += off_y;

  rect.x      = x + fs_off_x;
  rect.y      = y + fs_off_y;
  rect.width  = width;
  rect.height = height;

  if (gegl_rectangle_intersect (&rect, &rect, &bounding_box))
    {
      clay_drawable_update (drawable,
                            rect.x - off_x, rect.y - off_y,
                            rect.width, rect.height);
    }
}
