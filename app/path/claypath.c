/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypath.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"

#include "path-types.h"

#include "core/clay.h"
#include "core/clay-transform-utils.h"
#include "core/claybezierdesc.h"
#include "core/claychannel-select.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydrawable-fill.h"
#include "core/claydrawable-stroke.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/clayimage-undo-push.h"
#include "core/clayitemstack.h"
#include "core/claypaintinfo.h"
#include "core/claysavable.h"
#include "core/claysavable-load.h"
#include "core/claystrokeoptions.h"

#include "paint/claypaintcore-stroke.h"
#include "paint/claypaintoptions.h"

#include "clayanchor.h"
#include "claybezierstroke.h"
#include "claypath.h"
#include "claypath-preview.h"
#include "claystroke.h"
#include "clayvectorlayer.h"
#include "clayvectorlayeroptions.h"

#include "clay-intl.h"


enum
{
  FREEZE,
  THAW,
  LAST_SIGNAL
};


static void       clay_path_savable_iface_init      (ClaySavableInterface      *iface);

static void       clay_path_finalize                (GObject                   *object);

static gint64     clay_path_get_memsize             (ClayObject                *object,
                                                     gint64                    *gui_size);

static void       clay_path_save                    (ClaySavable               *savable,
                                                     ClaySaveState             *state);
static gboolean   clay_path_load_enter              (ClayLoadState             *state,
                                                     const gchar              **attribute_names,
                                                     const gchar              **attribute_values,
                                                     gpointer                   user_data,
                                                     GError                   **error);
static gboolean   clay_path_load_exit               (ClayLoadState             *state,
                                                     const gchar               *text,
                                                     gsize                      len,
                                                     gpointer                   user_data,
                                                     GError                   **error);

static gboolean   clay_path_is_attached             (ClayItem                  *item);
static ClayItemTree * clay_path_get_tree            (ClayItem                  *item);
static gboolean   clay_path_bounds                  (ClayItem                  *item,
                                                     gdouble                   *x,
                                                     gdouble                   *y,
                                                     gdouble                   *width,
                                                     gdouble                   *height);
static ClayItem * clay_path_duplicate               (ClayItem                  *item,
                                                     GType                      new_type);
static void       clay_path_convert                 (ClayItem                  *item,
                                                     ClayImage                 *dest_image,
                                                     GType                      old_type);
static void       clay_path_translate               (ClayItem                  *item,
                                                     gdouble                    offset_x,
                                                     gdouble                    offset_y,
                                                     gboolean                   push_undo);
static void       clay_path_scale                   (ClayItem                  *item,
                                                     gint                       new_width,
                                                     gint                       new_height,
                                                     gint                       new_offset_x,
                                                     gint                       new_offset_y,
                                                     ClayInterpolationType      interp_type,
                                                     ClayProgress              *progress);
static void       clay_path_resize                  (ClayItem                  *item,
                                                     ClayContext               *context,
                                                     ClayFillType               fill_type,
                                                     gint                       new_width,
                                                     gint                       new_height,
                                                     gint                       offset_x,
                                                     gint                       offset_y);
static void       clay_path_flip                    (ClayItem                  *item,
                                                     ClayContext               *context,
                                                     ClayOrientationType        flip_type,
                                                     gdouble                    axis,
                                                     gboolean                   clip_result);
static void       clay_path_rotate                  (ClayItem                  *item,
                                                     ClayContext               *context,
                                                     ClayRotationType           rotate_type,
                                                     gdouble                    center_x,
                                                     gdouble                    center_y,
                                                     gboolean                   clip_result);
static void       clay_path_transform               (ClayItem                  *item,
                                                     ClayContext               *context,
                                                     const ClayMatrix3         *matrix,
                                                     ClayTransformDirection     direction,
                                                     ClayInterpolationType      interp_type,
                                                     ClayTransformResize        clip_result,
                                                     ClayProgress              *progress,
                                                     gboolean                   push_undo);
static ClayTransformResize
                  clay_path_get_clip                (ClayItem                  *item,
                                                     ClayTransformResize        clip_result);
static gboolean   clay_path_fill                    (ClayItem                  *item,
                                                     ClayDrawable              *drawable,
                                                     ClayFillOptions           *fill_options,
                                                     gboolean                   push_undo,
                                                     ClayProgress              *progress,
                                                     GError                   **error);
static gboolean   clay_path_stroke                  (ClayItem                  *item,
                                                     ClayDrawable              *drawable,
                                                     ClayStrokeOptions         *stroke_options,
                                                     gboolean                   push_undo,
                                                     ClayProgress              *progress,
                                                     GError                   **error);
static void       clay_path_to_selection            (ClayItem                  *item,
                                                     ClayChannelOps             op,
                                                     gboolean                   antialias,
                                                     gboolean                   feather,
                                                     gdouble                    feather_radius_x,
                                                     gdouble                    feather_radius_y);

static void       clay_path_real_freeze             (ClayPath                  *path);
static void       clay_path_real_thaw               (ClayPath                  *path);
static void       clay_path_real_stroke_add         (ClayPath                  *path,
                                                     ClayStroke                *stroke);
static void       clay_path_real_stroke_remove      (ClayPath                  *path,
                                                     ClayStroke                *stroke);
static ClayStroke * clay_path_real_stroke_get       (ClayPath                  *path,
                                                     const ClayCoords          *coord);
static ClayStroke *clay_path_real_stroke_get_next   (ClayPath                  *path,
                                                     ClayStroke                *prev);
static gdouble clay_path_real_stroke_get_length     (ClayPath                  *path,
                                                     ClayStroke                *prev);
static ClayAnchor * clay_path_real_anchor_get       (ClayPath                  *path,
                                                     const ClayCoords          *coord,
                                                     ClayStroke               **ret_stroke);
static void       clay_path_real_anchor_delete      (ClayPath                  *path,
                                                     ClayAnchor                *anchor);
static gdouble    clay_path_real_get_length         (ClayPath                  *path,
                                                     const ClayAnchor          *start);
static gdouble    clay_path_real_get_distance       (ClayPath                  *path,
                                                     const ClayCoords          *coord);
static gint       clay_path_real_interpolate        (ClayPath                  *path,
                                                     ClayStroke                *stroke,
                                                     gdouble                    precision,
                                                     gint                       max_points,
                                                     ClayCoords                *ret_coords);

static gboolean   clay_path_attached_to_vector_layer_rec
                                                    (ClayPath                  *path,
                                                     GList                     *layers);

static ClayBezierDesc * clay_path_make_bezier       (ClayPath                  *path);
static ClayBezierDesc * clay_path_real_make_bezier  (ClayPath                  *path);

static gboolean         clay_path_enter_strokes     (ClayLoadState             *state,
                                                     const gchar              **attribute_names,
                                                     const gchar              **attribute_values,
                                                     gpointer                   user_data,
                                                     GError                   **error);


G_DEFINE_TYPE_WITH_CODE (ClayPath, clay_path, CLAY_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_path_savable_iface_init))

#define parent_class clay_path_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;

static guint clay_path_signals[LAST_SIGNAL] = { 0 };


static void
clay_path_class_init (ClayPathClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);

  clay_path_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayPathClass, freeze),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_path_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPathClass, thaw),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = clay_path_finalize;

  clay_object_class->get_memsize    = clay_path_get_memsize;

  viewable_class->default_icon_name = "clay-path";
  viewable_class->default_name      = _("Path");
  viewable_class->get_new_preview   = clay_path_get_new_preview;

  item_class->is_attached           = clay_path_is_attached;
  item_class->get_tree              = clay_path_get_tree;
  item_class->bounds                = clay_path_bounds;
  item_class->duplicate             = clay_path_duplicate;
  item_class->convert               = clay_path_convert;
  item_class->translate             = clay_path_translate;
  item_class->scale                 = clay_path_scale;
  item_class->resize                = clay_path_resize;
  item_class->flip                  = clay_path_flip;
  item_class->rotate                = clay_path_rotate;
  item_class->transform             = clay_path_transform;
  item_class->get_clip              = clay_path_get_clip;
  item_class->fill                  = clay_path_fill;
  item_class->stroke                = clay_path_stroke;
  item_class->to_selection          = clay_path_to_selection;
  item_class->rename_desc           = C_("undo-type", "Rename Path");
  item_class->translate_desc        = C_("undo-type", "Move Path");
  item_class->scale_desc            = C_("undo-type", "Scale Path");
  item_class->resize_desc           = C_("undo-type", "Resize Path");
  item_class->flip_desc             = C_("undo-type", "Flip Path");
  item_class->rotate_desc           = C_("undo-type", "Rotate Path");
  item_class->transform_desc        = C_("undo-type", "Transform Path");
  item_class->fill_desc             = C_("undo-type", "Fill Path");
  item_class->stroke_desc           = C_("undo-type", "Stroke Path");
  item_class->to_selection_desc     = C_("undo-type", "Path to Selection");
  item_class->reorder_desc          = C_("undo-type", "Reorder Path");
  item_class->raise_desc            = C_("undo-type", "Raise Path");
  item_class->raise_to_top_desc     = C_("undo-type", "Raise Path to Top");
  item_class->lower_desc            = C_("undo-type", "Lower Path");
  item_class->lower_to_bottom_desc  = C_("undo-type", "Lower Path to Bottom");
  item_class->raise_failed          = _("Path cannot be raised higher.");
  item_class->lower_failed          = _("Path cannot be lowered more.");

  klass->freeze                     = clay_path_real_freeze;
  klass->thaw                       = clay_path_real_thaw;

  klass->stroke_add                 = clay_path_real_stroke_add;
  klass->stroke_remove              = clay_path_real_stroke_remove;
  klass->stroke_get                 = clay_path_real_stroke_get;
  klass->stroke_get_next            = clay_path_real_stroke_get_next;
  klass->stroke_get_length          = clay_path_real_stroke_get_length;

  klass->anchor_get                 = clay_path_real_anchor_get;
  klass->anchor_delete              = clay_path_real_anchor_delete;

  klass->get_length                 = clay_path_real_get_length;
  klass->get_distance               = clay_path_real_get_distance;
  klass->interpolate                = clay_path_real_interpolate;

  klass->make_bezier                = clay_path_real_make_bezier;
}

static void
clay_path_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "path";
  iface->save       = clay_path_save;
  iface->load_enter = clay_path_load_enter;
  iface->load_exit  = clay_path_load_exit;
}

static void
clay_path_init (ClayPath *path)
{
  clay_item_set_visible (CLAY_ITEM (path), FALSE, FALSE);

  path->strokes        = g_queue_new ();
  path->stroke_to_list = g_hash_table_new (g_direct_hash, g_direct_equal);
  path->last_stroke_id = 0;
  path->freeze_count   = 0;
  path->precision      = 0.2;

  path->bezier_desc    = NULL;
  path->bounds_valid   = FALSE;
}

static void
clay_path_finalize (GObject *object)
{
  ClayPath *path = CLAY_PATH (object);

  if (path->bezier_desc)
    {
      clay_bezier_desc_free (path->bezier_desc);
      path->bezier_desc = NULL;
    }

  if (path->strokes)
    {
      g_queue_free_full (path->strokes, (GDestroyNotify) g_object_unref);
      path->strokes = NULL;
    }

  if (path->stroke_to_list)
    {
      g_hash_table_destroy (path->stroke_to_list);
      path->stroke_to_list = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_path_get_memsize (ClayObject *object,
                       gint64     *gui_size)
{
  ClayPath *path;
  GList    *list;
  gint64    memsize = 0;

  path = CLAY_PATH (object);

  for (list = path->strokes->head; list; list = g_list_next (list))
    memsize += (clay_object_get_memsize (CLAY_OBJECT (list->data), gui_size) +
                sizeof (GList));

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_path_save (ClaySavable   *savable,
                ClaySaveState *state)
{
  ClayPath *path = CLAY_PATH (savable);
  GList    *iter;

  clay_savable_print_element_start (state, "path", NULL);

  parent_savable_iface->save (savable, state);

  clay_savable_print_element_start (state, "strokes", NULL);
  for (iter = g_list_first (path->strokes->head); iter; iter = iter->next)
    {
      ClayStroke *stroke = iter->data;

      clay_savable_save (CLAY_SAVABLE (stroke), state);
    }
  clay_savable_print_element_end (state, "strokes");

  clay_savable_print_element_end (state, "path");
}

static gboolean
clay_path_load_enter (ClayLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError        **error)
{
  clay_savable_load_add_handlers (state, "strokes",
                                  clay_path_enter_strokes,
                                  NULL, NULL, NULL);

  return parent_savable_iface->load_enter (state,
                                           attribute_names, attribute_values,
                                           user_data, error);
}

static gboolean
clay_path_load_exit (ClayLoadState  *state,
                     const gchar    *text,
                     gsize           len,
                     gpointer        user_data,
                     GError        **error)
{
  ClayPath     *path            = NULL;
  gboolean      selected        = FALSE;
  gboolean      visible         = FALSE;
  guint         tattoo          = 0;
  ClayColorTag  tag             = CLAY_COLOR_TAG_NONE;
  gboolean      lock_content    = FALSE;
  gboolean      lock_position   = FALSE;
  gboolean      lock_visibility = FALSE;

  if (CLAY_IS_PATH (clay_savable_load_peek_active_object (state)))
    {
      path = CLAY_PATH (clay_savable_load_pop_active_object (state));
      clay_image_add_path (state->image, path, NULL,
                           clay_container_get_n_children (clay_image_get_paths (state->image)),
                           FALSE);

      clay_savable_load_get_values (state,
                                    "selected",        &selected,
                                    "visible",         &visible,
                                    "tattoo",          &tattoo,
                                    "tag",             &tag,
                                    "lock-content",    &lock_content,
                                    "lock-position",   &lock_position,
                                    "lock-visibility", &lock_visibility,
                                    NULL);

      /* TODO: implement selected paths. */

      clay_item_set_visible (CLAY_ITEM (path), visible, FALSE);
      if (tattoo > 0)
        clay_item_set_tattoo (CLAY_ITEM (path), tattoo);
      clay_item_set_color_tag (CLAY_ITEM (path), tag, FALSE);
      clay_item_set_lock_content (CLAY_ITEM (path), lock_content, FALSE);
      clay_item_set_lock_position (CLAY_ITEM (path), lock_position, FALSE);
      clay_item_set_lock_visibility (CLAY_ITEM (path), lock_visibility, FALSE);
    }

  return TRUE;
}

static gboolean
clay_path_is_attached (ClayItem *item)
{
  ClayImage *image = clay_item_get_image (item);

  return (CLAY_IS_IMAGE (image) &&
          clay_container_have (clay_image_get_paths (image),
                               CLAY_OBJECT (item)));
}

static ClayItemTree *
clay_path_get_tree (ClayItem *item)
{
  if (clay_item_is_attached (item))
    {
      ClayImage *image = clay_item_get_image (item);

      return clay_image_get_path_tree (image);
    }

  return NULL;
}

static gboolean
clay_path_bounds (ClayItem *item,
                  gdouble  *x,
                  gdouble  *y,
                  gdouble  *width,
                  gdouble  *height)
{
  ClayPath *path = CLAY_PATH (item);

  if (! path->bounds_valid)
    {
      ClayStroke *stroke;

      path->bounds_empty = TRUE;
      path->bounds_x1 = path->bounds_x2 = 0.0;
      path->bounds_y1 = path->bounds_y2 = 0.0;

      for (stroke = clay_path_stroke_get_next (path, NULL);
           stroke;
           stroke = clay_path_stroke_get_next (path, stroke))
        {
          GArray   *stroke_coords;
          gboolean  closed;

          stroke_coords = clay_stroke_interpolate (stroke, 1.0, &closed);

          if (stroke_coords)
            {
              ClayCoords point;
              gint       i;

              if (path->bounds_empty && stroke_coords->len > 0)
                {
                  point = g_array_index (stroke_coords, ClayCoords, 0);

                  path->bounds_x1 = path->bounds_x2 = point.x;
                  path->bounds_y1 = path->bounds_y2 = point.y;

                  path->bounds_empty = FALSE;
                }

              for (i = 0; i < stroke_coords->len; i++)
                {
                  point = g_array_index (stroke_coords, ClayCoords, i);

                  path->bounds_x1 = MIN (path->bounds_x1, point.x);
                  path->bounds_y1 = MIN (path->bounds_y1, point.y);
                  path->bounds_x2 = MAX (path->bounds_x2, point.x);
                  path->bounds_y2 = MAX (path->bounds_y2, point.y);
                }

              g_array_free (stroke_coords, TRUE);
            }
        }

      path->bounds_valid = TRUE;
    }

  *x      = path->bounds_x1;
  *y      = path->bounds_y1;
  *width  = path->bounds_x2 - path->bounds_x1;
  *height = path->bounds_y2 - path->bounds_y1;

  return ! path->bounds_empty;
}

static ClayItem *
clay_path_duplicate (ClayItem *item,
                     GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_PATH), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_PATH (new_item))
    {
      ClayPath *path     = CLAY_PATH (item);
      ClayPath *new_path = CLAY_PATH (new_item);

      clay_path_copy_strokes (path, new_path);
    }

  return new_item;
}

static void
clay_path_convert (ClayItem  *item,
                   ClayImage *dest_image,
                   GType      old_type)
{
  clay_item_set_size (item,
                      clay_image_get_width  (dest_image),
                      clay_image_get_height (dest_image));

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
clay_path_translate (ClayItem *item,
                     gdouble   offset_x,
                     gdouble   offset_y,
                     gboolean  push_undo)
{
  ClayPath *path = CLAY_PATH (item);
  GList    *list;

  clay_path_freeze (path);

  if (push_undo)
    clay_image_undo_push_path_mod (clay_item_get_image (item),
                                   _("Move Path"),
                                   path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_translate (stroke, offset_x, offset_y);
    }

  clay_path_thaw (path);
}

static void
clay_path_scale (ClayItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 ClayInterpolationType  interpolation_type,
                 ClayProgress          *progress)
{
  ClayPath  *path  = CLAY_PATH (item);
  ClayImage *image = clay_item_get_image (item);
  GList     *list;

  clay_path_freeze (path);

  if (clay_item_is_attached (item))
    clay_image_undo_push_path_mod (image, NULL, path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_scale (stroke,
                         (gdouble) new_width  / (gdouble) clay_item_get_width  (item),
                         (gdouble) new_height / (gdouble) clay_item_get_height (item));
      clay_stroke_translate (stroke, new_offset_x, new_offset_y);
    }

  CLAY_ITEM_CLASS (parent_class)->scale (item,
                                         clay_image_get_width  (image),
                                         clay_image_get_height (image),
                                         0, 0,
                                         interpolation_type, progress);

  clay_path_thaw (path);
}

static void
clay_path_resize (ClayItem     *item,
                  ClayContext  *context,
                  ClayFillType  fill_type,
                  gint          new_width,
                  gint          new_height,
                  gint          offset_x,
                  gint          offset_y)
{
  ClayPath  *path  = CLAY_PATH (item);
  ClayImage *image = clay_item_get_image (item);
  GList     *list;

  clay_path_freeze (path);

  if (clay_item_is_attached (item))
    clay_image_undo_push_path_mod (image, NULL, path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_translate (stroke, offset_x, offset_y);
    }

  CLAY_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                          clay_image_get_width  (image),
                                          clay_image_get_height (image),
                                          0, 0);

  clay_path_thaw (path);
}

static void
clay_path_flip (ClayItem            *item,
                ClayContext         *context,
                ClayOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  ClayPath    *path = CLAY_PATH (item);
  GList       *list;
  ClayMatrix3  matrix;

  clay_matrix3_identity (&matrix);
  clay_transform_matrix_flip (&matrix, flip_type, axis);

  clay_path_freeze (path);

  clay_image_undo_push_path_mod (clay_item_get_image (item),
                                 _("Flip Path"),
                                 path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_transform (stroke, &matrix, NULL);
    }

  clay_path_thaw (path);
}

static void
clay_path_rotate (ClayItem         *item,
                  ClayContext      *context,
                  ClayRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  ClayPath    *path = CLAY_PATH (item);
  GList       *list;
  ClayMatrix3  matrix;

  clay_matrix3_identity (&matrix);
  clay_transform_matrix_rotate (&matrix, rotate_type, center_x, center_y);

  clay_path_freeze (path);

  clay_image_undo_push_path_mod (clay_item_get_image (item),
                                 _("Rotate Path"),
                                 path);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_transform (stroke, &matrix, NULL);
    }

  clay_path_thaw (path);
}

static void
clay_path_transform (ClayItem               *item,
                     ClayContext            *context,
                     const ClayMatrix3      *matrix,
                     ClayTransformDirection  direction,
                     ClayInterpolationType   interpolation_type,
                     ClayTransformResize     clip_result,
                     ClayProgress           *progress,
                     gboolean                push_undo)
{
  ClayPath    *path = CLAY_PATH (item);
  ClayMatrix3  local_matrix;
  GQueue       strokes;
  GList       *list;

  clay_path_freeze (path);

  if (push_undo)
    clay_image_undo_push_path_mod (clay_item_get_image (item),
                                   _("Transform Path"),
                                   path);

  local_matrix = *matrix;

  if (direction == CLAY_TRANSFORM_BACKWARD)
    clay_matrix3_invert (&local_matrix);

  g_queue_init (&strokes);

  while (! g_queue_is_empty (path->strokes))
    {
      ClayStroke *stroke = g_queue_peek_head (path->strokes);

      g_object_ref (stroke);

      clay_path_stroke_remove (path, stroke);

      clay_stroke_transform (stroke, &local_matrix, &strokes);

      g_object_unref (stroke);
    }

  path->last_stroke_id = 0;

  for (list = strokes.head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_path_stroke_add (path, stroke);

      g_object_unref (stroke);
    }

  g_queue_clear (&strokes);

  clay_path_thaw (path);
}

static ClayTransformResize
clay_path_get_clip (ClayItem            *item,
                    ClayTransformResize  clip_result)
{
  return CLAY_TRANSFORM_RESIZE_ADJUST;
}

static gboolean
clay_path_fill (ClayItem         *item,
                ClayDrawable     *drawable,
                ClayFillOptions  *fill_options,
                gboolean          push_undo,
                ClayProgress     *progress,
                GError          **error)
{
  ClayPath *path = CLAY_PATH (item);

  if (g_queue_is_empty (path->strokes))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Not enough points to fill"));
      return FALSE;
    }

  return clay_drawable_fill_path (drawable, fill_options,
                                     path, push_undo, error);
}

static gboolean
clay_path_stroke (ClayItem           *item,
                  ClayDrawable       *drawable,
                  ClayStrokeOptions  *stroke_options,
                  gboolean            push_undo,
                  ClayProgress       *progress,
                  GError            **error)
{
  ClayPath *path   = CLAY_PATH (item);
  gboolean  retval = FALSE;

  if (g_queue_is_empty (path->strokes))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Not enough points to stroke"));
      return FALSE;
    }

  switch (clay_stroke_options_get_method (stroke_options))
    {
    case CLAY_STROKE_LINE:
      retval = clay_drawable_stroke_path (drawable, stroke_options,
                                          path, push_undo, error);
      break;

    case CLAY_STROKE_PAINT_METHOD:
      {
        ClayPaintInfo    *paint_info;
        ClayPaintCore    *core;
        ClayPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = clay_context_get_paint_info (CLAY_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = clay_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = clay_stroke_options_get_emulate_dynamics (stroke_options);

        retval = clay_paint_core_stroke_path (core, drawable,
                                              paint_options,
                                              emulate_dynamics,
                                              path, push_undo, error);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
clay_path_to_selection (ClayItem       *item,
                        ClayChannelOps  op,
                        gboolean        antialias,
                        gboolean        feather,
                        gdouble         feather_radius_x,
                        gdouble         feather_radius_y)
{
  ClayPath    *path  = CLAY_PATH (item);
  ClayImage   *image = clay_item_get_image (item);

  clay_channel_select_path (clay_image_get_mask (image),
                            CLAY_ITEM_GET_CLASS (item)->to_selection_desc,
                            path,
                            op, antialias,
                            feather, feather_radius_x, feather_radius_x,
                            TRUE);
}

static void
clay_path_real_freeze (ClayPath *path)
{
  /*  release cached bezier representation  */
  if (path->bezier_desc)
    {
      clay_bezier_desc_free (path->bezier_desc);
      path->bezier_desc = NULL;
    }

  /*  invalidate bounds  */
  path->bounds_valid = FALSE;
}

static void
clay_path_real_thaw (ClayPath *path)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (path));
}


/*  public functions  */

ClayPath *
clay_path_new (ClayImage   *image,
               const gchar *name)
{
  ClayPath *path;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  path = CLAY_PATH (clay_item_new (CLAY_TYPE_PATH,
                                   image, name,
                                   0, 0,
                                   clay_image_get_width  (image),
                                   clay_image_get_height (image)));

  return path;
}

ClayPath *
clay_path_get_parent (ClayPath *path)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  return CLAY_PATH (clay_viewable_get_parent (CLAY_VIEWABLE (path)));
}

void
clay_path_freeze (ClayPath *path)
{
  g_return_if_fail (CLAY_IS_PATH (path));

  path->freeze_count++;

  if (path->freeze_count == 1)
    g_signal_emit (path, clay_path_signals[FREEZE], 0);
}

void
clay_path_thaw (ClayPath *path)
{
  g_return_if_fail (CLAY_IS_PATH (path));
  g_return_if_fail (path->freeze_count > 0);

  path->freeze_count--;

  if (path->freeze_count == 0)
    g_signal_emit (path, clay_path_signals[THAW], 0);
}

void
clay_path_copy_strokes (ClayPath *src_path,
                        ClayPath *dest_path)
{
  g_return_if_fail (CLAY_IS_PATH (src_path));
  g_return_if_fail (CLAY_IS_PATH (dest_path));

  clay_path_freeze (dest_path);

  g_queue_free_full (dest_path->strokes, (GDestroyNotify) g_object_unref);
  dest_path->strokes = g_queue_new ();
  g_hash_table_remove_all (dest_path->stroke_to_list);

  dest_path->last_stroke_id = 0;

  clay_path_add_strokes (src_path, dest_path);

  clay_path_thaw (dest_path);
}


void
clay_path_add_strokes (ClayPath *src_path,
                       ClayPath *dest_path)
{
  GList *stroke;

  g_return_if_fail (CLAY_IS_PATH (src_path));
  g_return_if_fail (CLAY_IS_PATH (dest_path));

  clay_path_freeze (dest_path);

  for (stroke = src_path->strokes->head;
       stroke != NULL;
       stroke = g_list_next (stroke))
    {
      ClayStroke *newstroke = clay_stroke_duplicate (stroke->data);

      g_queue_push_tail (dest_path->strokes, newstroke);

      /* Also add to {stroke: GList node} map */
      g_hash_table_insert (dest_path->stroke_to_list,
                           newstroke,
                           g_queue_peek_tail_link (dest_path->strokes));

      dest_path->last_stroke_id++;
      clay_stroke_set_id (newstroke,
                          dest_path->last_stroke_id);
    }

  clay_path_thaw (dest_path);
}


void
clay_path_stroke_add (ClayPath    *path,
                      ClayStroke  *stroke)
{
  g_return_if_fail (CLAY_IS_PATH (path));
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  clay_path_freeze (path);

  CLAY_PATH_GET_CLASS (path)->stroke_add (path, stroke);

  clay_path_thaw (path);
}

static void
clay_path_real_stroke_add (ClayPath    *path,
                           ClayStroke  *stroke)
{
  /*
   * Don't prepend into path->strokes.  See ChangeLog 2003-05-21
   * --Mitch
   */
  g_queue_push_tail (path->strokes, g_object_ref (stroke));

  /* Also add to {stroke: GList node} map */
  g_hash_table_insert (path->stroke_to_list,
                       stroke,
                       g_queue_peek_tail_link (path->strokes));

  path->last_stroke_id++;
  clay_stroke_set_id (stroke, path->last_stroke_id);
}

void
clay_path_stroke_remove (ClayPath    *path,
                         ClayStroke  *stroke)
{
  g_return_if_fail (CLAY_IS_PATH (path));
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  clay_path_freeze (path);

  CLAY_PATH_GET_CLASS (path)->stroke_remove (path, stroke);

  clay_path_thaw (path);
}

static void
clay_path_real_stroke_remove (ClayPath    *path,
                              ClayStroke  *stroke)
{
  GList *list = g_hash_table_lookup (path->stroke_to_list, stroke);

  if (list)
    {
      g_queue_delete_link (path->strokes, list);
      g_hash_table_remove (path->stroke_to_list, stroke);
      g_object_unref (stroke);
    }
}

gint
clay_path_get_n_strokes (ClayPath *path)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0);

  return g_queue_get_length (path->strokes);
}


ClayStroke *
clay_path_stroke_get (ClayPath         *path,
                      const ClayCoords *coord)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  return CLAY_PATH_GET_CLASS (path)->stroke_get (path, coord);
}

static ClayStroke *
clay_path_real_stroke_get (ClayPath         *path,
                           const ClayCoords *coord)
{
  ClayStroke *minstroke = NULL;
  gdouble     mindist   = G_MAXDOUBLE;
  GList      *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;
      ClayAnchor *anchor = clay_stroke_anchor_get (stroke, coord);

      if (anchor)
        {
          gdouble dx = coord->x - anchor->position.x;
          gdouble dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy)
            {
              mindist   = dx * dx + dy * dy;
              minstroke = stroke;
            }
        }
    }

  return minstroke;
}

ClayStroke *
clay_path_stroke_get_by_id (ClayPath *path,
                            gint      id)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      if (clay_stroke_get_id (list->data) == id)
        return list->data;
    }

  return NULL;
}


ClayStroke *
clay_path_stroke_get_next (ClayPath    *path,
                           ClayStroke  *prev)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  return CLAY_PATH_GET_CLASS (path)->stroke_get_next (path, prev);
}

static ClayStroke *
clay_path_real_stroke_get_next (ClayPath    *path,
                                ClayStroke  *prev)
{
  if (! prev)
    {
      return g_queue_peek_head (path->strokes);
    }
  else
    {
      GList *stroke = g_hash_table_lookup (path->stroke_to_list, prev);

      g_return_val_if_fail (stroke != NULL, NULL);

      return stroke->next ? stroke->next->data : NULL;
    }
}


gdouble
clay_path_stroke_get_length (ClayPath    *path,
                             ClayStroke  *stroke)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0.0);
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), 0.0);

  return CLAY_PATH_GET_CLASS (path)->stroke_get_length (path, stroke);
}

static gdouble
clay_path_real_stroke_get_length (ClayPath    *path,
                                  ClayStroke  *stroke)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0.0);
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), 0.0);

  return clay_stroke_get_length (stroke, path->precision);
}


ClayAnchor *
clay_path_anchor_get (ClayPath          *path,
                      const ClayCoords  *coord,
                      ClayStroke       **ret_stroke)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  return CLAY_PATH_GET_CLASS (path)->anchor_get (path, coord,
                                                    ret_stroke);
}

static ClayAnchor *
clay_path_real_anchor_get (ClayPath          *path,
                           const ClayCoords  *coord,
                           ClayStroke       **ret_stroke)
{
  ClayAnchor *minanchor = NULL;
  gdouble     mindist   = -1;
  GList      *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;
      ClayAnchor *anchor = clay_stroke_anchor_get (stroke, coord);

      if (anchor)
        {
          gdouble dx = coord->x - anchor->position.x;
          gdouble dy = coord->y - anchor->position.y;

          if (mindist > dx * dx + dy * dy || mindist < 0)
            {
              mindist   = dx * dx + dy * dy;
              minanchor = anchor;

              if (ret_stroke)
                *ret_stroke = stroke;
            }
        }
    }

  return minanchor;
}


void
clay_path_anchor_delete (ClayPath    *path,
                         ClayAnchor  *anchor)
{
  g_return_if_fail (CLAY_IS_PATH (path));
  g_return_if_fail (anchor != NULL);

  CLAY_PATH_GET_CLASS (path)->anchor_delete (path, anchor);
}

static void
clay_path_real_anchor_delete (ClayPath    *path,
                              ClayAnchor  *anchor)
{
}


void
clay_path_anchor_select (ClayPath    *path,
                         ClayStroke  *target_stroke,
                         ClayAnchor  *anchor,
                         gboolean     selected,
                         gboolean     exclusive)
{
  GList *list;

  for (list = path->strokes->head; list; list = g_list_next (list))
    {
      ClayStroke *stroke = list->data;

      clay_stroke_anchor_select (stroke,
                                 stroke == target_stroke ? anchor : NULL,
                                 selected, exclusive);
    }
}


gdouble
clay_path_get_length (ClayPath         *path,
                      const ClayAnchor *start)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0.0);

  return CLAY_PATH_GET_CLASS (path)->get_length (path, start);
}

static gdouble
clay_path_real_get_length (ClayPath         *path,
                           const ClayAnchor *start)
{
  g_printerr ("clay_path_get_length: default implementation\n");

  return 0;
}


gdouble
clay_path_get_distance (ClayPath         *path,
                        const ClayCoords *coord)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0.0);

  return CLAY_PATH_GET_CLASS (path)->get_distance (path, coord);
}

static gdouble
clay_path_real_get_distance (ClayPath         *path,
                             const ClayCoords *coord)
{
  g_printerr ("clay_path_get_distance: default implementation\n");

  return 0;
}

gint
clay_path_interpolate (ClayPath    *path,
                       ClayStroke  *stroke,
                       gdouble      precision,
                       gint         max_points,
                       ClayCoords  *ret_coords)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), 0);

  return CLAY_PATH_GET_CLASS (path)->interpolate (path, stroke,
                                                  precision, max_points,
                                                  ret_coords);
}

static gint
clay_path_real_interpolate (ClayPath    *path,
                            ClayStroke  *stroke,
                            gdouble      precision,
                            gint         max_points,
                            ClayCoords  *ret_coords)
{
  g_printerr ("clay_path_interpolate: default implementation\n");

  return 0;
}

gboolean
clay_path_attached_to_vector_layer (ClayPath  *path,
                                    ClayImage *image)
{
  GList *layers;
  GList *list;

  g_return_val_if_fail (CLAY_IS_PATH (path), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  layers = clay_image_get_layer_iter (image);

  for (list = layers; list; list = g_list_next (list))
    {
      if (CLAY_IS_VECTOR_LAYER (list->data))
        {
          ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (list->data);

          if (vector_layer->options       &&
              vector_layer->options->path &&
              (path == vector_layer->options->path))
            return TRUE;
        }

      if (clay_viewable_get_children (CLAY_VIEWABLE (list->data)))
        {
          ClayContainer *container;
          GList         *iter2;

          container = clay_viewable_get_children (CLAY_VIEWABLE (list->data));

          iter2 =
            clay_item_stack_get_item_iter (CLAY_ITEM_STACK (container));

          if (clay_path_attached_to_vector_layer_rec (path, iter2))
            return TRUE;
        }
    }

  return FALSE;
}

static gboolean
clay_path_attached_to_vector_layer_rec (ClayPath  *path,
                                        GList     *layers)
{
  GList *list;

  for (list = layers; list; list = g_list_next (list))
    {
      if (CLAY_IS_VECTOR_LAYER (list->data))
        {
          ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (list->data);

          if (vector_layer->options       &&
              vector_layer->options->path &&
              (path == vector_layer->options->path))
            return TRUE;
        }

      if (clay_viewable_get_children (CLAY_VIEWABLE (list->data)))
        {
          ClayContainer *container;
          GList         *iter2;

          container = clay_viewable_get_children (CLAY_VIEWABLE (list->data));

          iter2 =
            clay_item_stack_get_item_iter (CLAY_ITEM_STACK (container));

          if (clay_path_attached_to_vector_layer_rec (path, iter2))
            return TRUE;
        }
    }

  return FALSE;
}

const ClayBezierDesc *
clay_path_get_bezier (ClayPath *path)
{
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);

  if (! path->bezier_desc)
    {
      path->bezier_desc = clay_path_make_bezier (path);
    }

  return  path->bezier_desc;
}

static ClayBezierDesc *
clay_path_make_bezier (ClayPath *path)
{
  return CLAY_PATH_GET_CLASS (path)->make_bezier (path);
}

static ClayBezierDesc *
clay_path_real_make_bezier (ClayPath *path)
{
  ClayStroke     *stroke;
  GArray         *cmd_array;
  ClayBezierDesc *ret_bezdesc = NULL;

  cmd_array = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

  for (stroke = clay_path_stroke_get_next (path, NULL);
       stroke;
       stroke = clay_path_stroke_get_next (path, stroke))
    {
      ClayBezierDesc *bezdesc = clay_stroke_make_bezier (stroke);

      if (bezdesc)
        {
          cmd_array = g_array_append_vals (cmd_array, bezdesc->data,
                                           bezdesc->num_data);
          clay_bezier_desc_free (bezdesc);
        }
    }

  if (cmd_array->len > 0)
    ret_bezdesc = clay_bezier_desc_new ((cairo_path_data_t *) cmd_array->data,
                                        cmd_array->len);

  g_array_free (cmd_array, FALSE);

  return ret_bezdesc;
}

static gboolean
clay_path_enter_strokes (ClayLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  const gchar *name = NULL;

  if (clay_savable_load_get_parent_values (state, "name", &name, NULL))
    {
      ClayPath *path = NULL;

      path = clay_path_new (state->image, name);
      clay_savable_load_push_active_object (state, G_OBJECT (path));
      clay_savable_load (CLAY_TYPE_BEZIER_STROKE, state);
    }
  else
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                   "%s: element <name> must happen before <strokes> in <path>.",
                   G_STRFUNC);
    }

  return TRUE;
}
