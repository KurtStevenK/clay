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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-nodes.h"
#include "gegl/clay-gegl-utils.h"

#include "clay-utils.h"
#include "clayboundary.h"
#include "claychannel-select.h"
#include "claycontext.h"
#include "claycontainer.h"
#include "claydrawable-filters.h"
#include "claydrawable-floating-selection.h"
#include "clayerror.h"
#include "claygrouplayer.h"
#include "clayimage-undo-push.h"
#include "clayimage-undo.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayitemstack.h"
#include "claylayer-floating-selection.h"
#include "claylayer.h"
#include "claylayer-new.h"
#include "claylayermask.h"
#include "clayobjectqueue.h"
#include "claypickable.h"
#include "clayprogress.h"
#include "clayrasterizable.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-intl.h"


enum
{
  OPACITY_CHANGED,
  MODE_CHANGED,
  BLEND_SPACE_CHANGED,
  COMPOSITE_SPACE_CHANGED,
  COMPOSITE_MODE_CHANGED,
  EFFECTIVE_MODE_CHANGED,
  EXCLUDES_BACKDROP_CHANGED,
  LOCK_ALPHA_CHANGED,
  MASK_CHANGED,
  APPLY_MASK_CHANGED,
  EDIT_MASK_CHANGED,
  SHOW_MASK_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_MODE,
  PROP_BLEND_SPACE,
  PROP_COMPOSITE_SPACE,
  PROP_COMPOSITE_MODE,
  PROP_EXCLUDES_BACKDROP,
  PROP_LOCK_ALPHA,
  PROP_MASK,
  PROP_FLOATING_SELECTION
};


static void       clay_color_managed_iface_init (ClayColorManagedInterface *iface);
static void       clay_pickable_iface_init      (ClayPickableInterface     *iface);
static void       clay_savable_iface_init       (ClaySavableInterface      *iface);

static void       clay_layer_set_property       (GObject            *object,
                                                 guint               property_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void       clay_layer_get_property       (GObject            *object,
                                                 guint               property_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static void       clay_layer_dispose            (GObject            *object);
static void       clay_layer_finalize           (GObject            *object);
static void       clay_layer_notify             (GObject            *object,
                                                 GParamSpec         *pspec);

static void       clay_layer_name_changed       (ClayObject         *object);
static gint64     clay_layer_get_memsize        (ClayObject         *object,
                                                 gint64             *gui_size);

static void       clay_layer_invalidate_preview (ClayViewable       *viewable);
static gchar    * clay_layer_get_description    (ClayViewable       *viewable,
                                                 gchar             **tooltip);

static GeglNode * clay_layer_get_node           (ClayFilter         *filter);

static void       clay_layer_removed            (ClayItem           *item);
static void       clay_layer_unset_removed      (ClayItem           *item);
static gboolean   clay_layer_is_attached        (ClayItem           *item);
static ClayItemTree * clay_layer_get_tree       (ClayItem           *item);
static ClayItem * clay_layer_duplicate          (ClayItem           *item,
                                                 GType               new_type);
static void       clay_layer_convert            (ClayItem           *item,
                                                 ClayImage          *dest_image,
                                                 GType               old_type);
static gboolean   clay_layer_rename             (ClayItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc,
                                                 GError            **error);
static void       clay_layer_start_move         (ClayItem           *item,
                                                 gboolean            push_undo);
static void       clay_layer_end_move           (ClayItem           *item,
                                                 gboolean            push_undo);
static void       clay_layer_translate          (ClayItem           *item,
                                                 gdouble             offset_x,
                                                 gdouble             offset_y,
                                                 gboolean            push_undo);
static void       clay_layer_scale              (ClayItem           *item,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 ClayInterpolationType  interp_type,
                                                 ClayProgress       *progress);
static void       clay_layer_resize             (ClayItem           *item,
                                                 ClayContext        *context,
                                                 ClayFillType        fill_type,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       clay_layer_flip               (ClayItem           *item,
                                                 ClayContext        *context,
                                                 ClayOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       clay_layer_rotate             (ClayItem           *item,
                                                 ClayContext        *context,
                                                 ClayRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       clay_layer_transform          (ClayItem           *item,
                                                 ClayContext        *context,
                                                 const ClayMatrix3  *matrix,
                                                 ClayTransformDirection direction,
                                                 ClayInterpolationType  interpolation_type,
                                                 ClayTransformResize clip_result,
                                                 ClayProgress       *progress,
                                                 gboolean            push_undo);
static void       clay_layer_to_selection       (ClayItem           *item,
                                                 ClayChannelOps      op,
                                                 gboolean            antialias,
                                                 gboolean            feather,
                                                 gdouble             feather_radius_x,
                                                 gdouble             feather_radius_y);

static void       clay_layer_alpha_changed      (ClayDrawable       *drawable);
static gint64     clay_layer_estimate_memsize   (ClayDrawable       *drawable,
                                                 ClayComponentType   component_type,
                                                 gint                width,
                                                 gint                height);
static gboolean   clay_layer_supports_alpha     (ClayDrawable       *drawable);
static void       clay_layer_convert_type       (ClayDrawable       *drawable,
                                                 ClayImage          *dest_image,
                                                 const Babl         *new_format,
                                                 ClayColorProfile   *src_profile,
                                                 ClayColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 ClayProgress       *progress);
static void    clay_layer_invalidate_boundary   (ClayDrawable       *drawable);
static ClayComponentMask
               clay_layer_get_active_mask       (ClayDrawable       *drawable);
static void    clay_layer_set_buffer            (ClayDrawable       *drawable,
                                                 gboolean            push_undo,
                                                 const gchar        *undo_desc,
                                                 GeglBuffer         *buffer,
                                                 const GeglRectangle *bounds);
static GeglRectangle
               clay_layer_get_bounding_box      (ClayDrawable       *drawable);

static ClayColorProfile *
               clay_layer_get_color_profile     (ClayColorManaged   *managed);

static gdouble clay_layer_get_opacity_at        (ClayPickable       *pickable,
                                                 gint                x,
                                                 gint                y);


static void       clay_layer_save               (ClaySavable        *savable,
                                                 ClaySaveState      *state);
static gboolean   clay_layer_load_enter         (ClayLoadState      *state,
                                                 const gchar       **attribute_names,
                                                 const gchar       **attribute_values,
                                                 gpointer            user_data,
                                                 GError            **error);
static gboolean   clay_layer_load_exit          (ClayLoadState      *state,
                                                 const gchar        *text,
                                                 gsize               len,
                                                 gpointer            user_data,
                                                 GError            **error);

static gboolean clay_layer_real_is_alpha_locked (ClayLayer          *layer,
                                                 ClayLayer         **locked_layer);
static void       clay_layer_real_translate     (ClayLayer          *layer,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       clay_layer_real_scale         (ClayLayer          *layer,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 ClayInterpolationType  interp_type,
                                                 ClayProgress       *progress);
static void       clay_layer_real_resize        (ClayLayer          *layer,
                                                 ClayContext        *context,
                                                 ClayFillType        fill_type,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       clay_layer_real_flip          (ClayLayer          *layer,
                                                 ClayContext        *context,
                                                 ClayOrientationType flip_type,
                                                 gdouble             axis,
                                                 gboolean            clip_result);
static void       clay_layer_real_rotate        (ClayLayer          *layer,
                                                 ClayContext        *context,
                                                 ClayRotationType    rotate_type,
                                                 gdouble             center_x,
                                                 gdouble             center_y,
                                                 gboolean            clip_result);
static void       clay_layer_real_transform     (ClayLayer          *layer,
                                                 ClayContext        *context,
                                                 const ClayMatrix3  *matrix,
                                                 ClayTransformDirection direction,
                                                 ClayInterpolationType  interpolation_type,
                                                 ClayTransformResize clip_result,
                                                 ClayProgress       *progress,
                                                 gboolean            push_undo);
static void       clay_layer_real_convert_type  (ClayLayer          *layer,
                                                 ClayImage          *dest_image,
                                                 const Babl         *new_format,
                                                 ClayColorProfile   *src_profile,
                                                 ClayColorProfile   *dest_profile,
                                                 GeglDitherMethod    layer_dither_type,
                                                 GeglDitherMethod    mask_dither_type,
                                                 gboolean            push_undo,
                                                 ClayProgress       *progress);
static GeglRectangle
               clay_layer_real_get_bounding_box (ClayLayer          *layer);
static void  clay_layer_real_get_effective_mode (ClayLayer          *layer,
                                                 ClayLayerMode          *mode,
                                                 ClayLayerColorSpace    *blend_space,
                                                 ClayLayerColorSpace    *composite_space,
                                                 ClayLayerCompositeMode *composite_mode);
static gboolean
          clay_layer_real_get_excludes_backdrop (ClayLayer          *layer);

static void       clay_layer_layer_mask_update  (ClayDrawable       *layer_mask,
                                                 gint                x,
                                                 gint                y,
                                                 gint                width,
                                                 gint                height,
                                                 ClayLayer          *layer);

static gboolean   clay_layer_enter_mask         (ClayLoadState      *state,
                                                 const gchar       **attribute_names,
                                                 const gchar       **attribute_values,
                                                 gpointer            user_data,
                                                 GError            **error);
static gboolean   clay_layer_exit_mask          (ClayLoadState      *state,
                                                 const gchar        *text,
                                                 gsize               len,
                                                 gpointer            user_data,
                                                 GError            **error);


G_DEFINE_TYPE_WITH_CODE (ClayLayer, clay_layer, CLAY_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                clay_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_savable_iface_init))

#define parent_class clay_layer_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;

static guint layer_signals[LAST_SIGNAL] = { 0 };


static void
clay_layer_class_init (ClayLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayFilterClass   *filter_class      = CLAY_FILTER_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);

  layer_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[BLEND_SPACE_CHANGED] =
    g_signal_new ("blend-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, blend_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_SPACE_CHANGED] =
    g_signal_new ("composite-space-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, composite_space_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[COMPOSITE_MODE_CHANGED] =
    g_signal_new ("composite-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, composite_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EFFECTIVE_MODE_CHANGED] =
    g_signal_new ("effective-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, effective_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EXCLUDES_BACKDROP_CHANGED] =
    g_signal_new ("excludes-backdrop-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, excludes_backdrop_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[LOCK_ALPHA_CHANGED] =
    g_signal_new ("lock-alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, lock_alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[APPLY_MASK_CHANGED] =
    g_signal_new ("apply-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, apply_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[EDIT_MASK_CHANGED] =
    g_signal_new ("edit-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, edit_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  layer_signals[SHOW_MASK_CHANGED] =
    g_signal_new ("show-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLayerClass, show_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property          = clay_layer_set_property;
  object_class->get_property          = clay_layer_get_property;
  object_class->dispose               = clay_layer_dispose;
  object_class->finalize              = clay_layer_finalize;
  object_class->notify                = clay_layer_notify;

  clay_object_class->name_changed     = clay_layer_name_changed;
  clay_object_class->get_memsize      = clay_layer_get_memsize;

  viewable_class->default_icon_name   = "clay-layer";
  viewable_class->default_name        = _("Layer");
  viewable_class->invalidate_preview  = clay_layer_invalidate_preview;
  viewable_class->get_description     = clay_layer_get_description;

  filter_class->get_node              = clay_layer_get_node;

  item_class->removed                 = clay_layer_removed;
  item_class->unset_removed           = clay_layer_unset_removed;
  item_class->is_attached             = clay_layer_is_attached;
  item_class->get_tree                = clay_layer_get_tree;
  item_class->duplicate               = clay_layer_duplicate;
  item_class->convert                 = clay_layer_convert;
  item_class->rename                  = clay_layer_rename;
  item_class->start_move              = clay_layer_start_move;
  item_class->end_move                = clay_layer_end_move;
  item_class->translate               = clay_layer_translate;
  item_class->scale                   = clay_layer_scale;
  item_class->resize                  = clay_layer_resize;
  item_class->flip                    = clay_layer_flip;
  item_class->rotate                  = clay_layer_rotate;
  item_class->transform               = clay_layer_transform;
  item_class->to_selection            = clay_layer_to_selection;
  item_class->rename_desc             = C_("undo-type", "Rename Layer");
  item_class->translate_desc          = C_("undo-type", "Move Layer");
  item_class->scale_desc              = C_("undo-type", "Scale Layer");
  item_class->resize_desc             = C_("undo-type", "Resize Layer");
  item_class->flip_desc               = C_("undo-type", "Flip Layer");
  item_class->rotate_desc             = C_("undo-type", "Rotate Layer");
  item_class->transform_desc          = C_("undo-type", "Transform Layer");
  item_class->to_selection_desc       = C_("undo-type", "Alpha to Selection");
  item_class->reorder_desc            = C_("undo-type", "Reorder Layer");
  item_class->raise_desc              = C_("undo-type", "Raise Layer");
  item_class->raise_to_top_desc       = C_("undo-type", "Raise Layer to Top");
  item_class->lower_desc              = C_("undo-type", "Lower Layer");
  item_class->lower_to_bottom_desc    = C_("undo-type", "Lower Layer to Bottom");
  item_class->raise_failed            = _("Layer cannot be raised higher.");
  item_class->lower_failed            = _("Layer cannot be lowered more.");

  drawable_class->alpha_changed         = clay_layer_alpha_changed;
  drawable_class->estimate_memsize      = clay_layer_estimate_memsize;
  drawable_class->supports_alpha        = clay_layer_supports_alpha;
  drawable_class->convert_type          = clay_layer_convert_type;
  drawable_class->invalidate_boundary   = clay_layer_invalidate_boundary;
  drawable_class->get_active_mask       = clay_layer_get_active_mask;
  drawable_class->set_buffer            = clay_layer_set_buffer;
  drawable_class->get_bounding_box      = clay_layer_get_bounding_box;

  klass->opacity_changed              = NULL;
  klass->mode_changed                 = NULL;
  klass->blend_space_changed          = NULL;
  klass->composite_space_changed      = NULL;
  klass->composite_mode_changed       = NULL;
  klass->excludes_backdrop_changed    = NULL;
  klass->lock_alpha_changed           = NULL;
  klass->mask_changed                 = NULL;
  klass->apply_mask_changed           = NULL;
  klass->edit_mask_changed            = NULL;
  klass->show_mask_changed            = NULL;
  klass->is_alpha_locked              = clay_layer_real_is_alpha_locked;
  klass->translate                    = clay_layer_real_translate;
  klass->scale                        = clay_layer_real_scale;
  klass->resize                       = clay_layer_real_resize;
  klass->flip                         = clay_layer_real_flip;
  klass->rotate                       = clay_layer_real_rotate;
  klass->transform                    = clay_layer_real_transform;
  klass->convert_type                 = clay_layer_real_convert_type;
  klass->get_bounding_box             = clay_layer_real_get_bounding_box;
  klass->get_effective_mode           = clay_layer_real_get_effective_mode;
  klass->get_excludes_backdrop        = clay_layer_real_get_excludes_backdrop;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        CLAY_OPACITY_TRANSPARENT,
                                                        CLAY_OPACITY_OPAQUE,
                                                        CLAY_OPACITY_OPAQUE,
                                                        CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode", NULL, NULL,
                                                      CLAY_TYPE_LAYER_MODE,
                                                      CLAY_LAYER_MODE_NORMAL,
                                                      CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_BLEND_SPACE,
                                   g_param_spec_enum ("blend-space",
                                                      NULL, NULL,
                                                      CLAY_TYPE_LAYER_COLOR_SPACE,
                                                      CLAY_LAYER_COLOR_SPACE_AUTO,
                                                      CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_SPACE,
                                   g_param_spec_enum ("composite-space",
                                                      NULL, NULL,
                                                      CLAY_TYPE_LAYER_COLOR_SPACE,
                                                      CLAY_LAYER_COLOR_SPACE_AUTO,
                                                      CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_enum ("composite-mode",
                                                      NULL, NULL,
                                                      CLAY_TYPE_LAYER_COMPOSITE_MODE,
                                                      CLAY_LAYER_COMPOSITE_AUTO,
                                                      CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_EXCLUDES_BACKDROP,
                                   g_param_spec_boolean ("excludes-backdrop",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_LOCK_ALPHA,
                                   g_param_spec_boolean ("lock-alpha",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_MASK,
                                   g_param_spec_object ("mask",
                                                        NULL, NULL,
                                                        CLAY_TYPE_LAYER_MASK,
                                                        CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_FLOATING_SELECTION,
                                   g_param_spec_boolean ("floating-selection",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READABLE));
}

static void
clay_layer_init (ClayLayer *layer)
{
  layer->opacity                   = CLAY_OPACITY_OPAQUE;
  layer->mode                      = CLAY_LAYER_MODE_NORMAL;
  layer->blend_space               = CLAY_LAYER_COLOR_SPACE_AUTO;
  layer->composite_space           = CLAY_LAYER_COLOR_SPACE_AUTO;
  layer->composite_mode            = CLAY_LAYER_COMPOSITE_AUTO;
  layer->effective_mode            = layer->mode;
  layer->effective_blend_space     = clay_layer_get_real_blend_space (layer);
  layer->effective_composite_space = clay_layer_get_real_composite_space (layer);
  layer->effective_composite_mode  = clay_layer_get_real_composite_mode (layer);
  layer->excludes_backdrop         = FALSE;
  layer->lock_alpha                = FALSE;

  layer->mask       = NULL;
  layer->apply_mask = TRUE;
  layer->edit_mask  = TRUE;
  layer->show_mask  = FALSE;

  /*  floating selection  */
  layer->fs.drawable       = NULL;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs           = NULL;
  layer->fs.num_segs       = 0;
}

static void
clay_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_color_profile = clay_layer_get_color_profile;
}

static void
clay_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->get_opacity_at = clay_layer_get_opacity_at;
}

static void
clay_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "raster-layer";
  iface->save       = clay_layer_save;
  iface->load_enter = clay_layer_load_enter;
  iface->load_exit  = clay_layer_load_exit;
}

static void
clay_layer_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_layer_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayLayer *layer = CLAY_LAYER (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      g_value_set_double (value, clay_layer_get_opacity (layer));
      break;
    case PROP_MODE:
      g_value_set_enum (value, clay_layer_get_mode (layer));
      break;
    case PROP_BLEND_SPACE:
      g_value_set_enum (value, clay_layer_get_blend_space (layer));
      break;
    case PROP_COMPOSITE_SPACE:
      g_value_set_enum (value, clay_layer_get_composite_space (layer));
      break;
    case PROP_COMPOSITE_MODE:
      g_value_set_enum (value, clay_layer_get_composite_mode (layer));
      break;
    case PROP_EXCLUDES_BACKDROP:
      g_value_set_boolean (value, clay_layer_get_excludes_backdrop (layer));
      break;
    case PROP_LOCK_ALPHA:
      g_value_set_boolean (value, clay_layer_get_lock_alpha (layer));
      break;
    case PROP_MASK:
      g_value_set_object (value, clay_layer_get_mask (layer));
      break;
    case PROP_FLOATING_SELECTION:
      g_value_set_boolean (value, clay_layer_is_floating_sel (layer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_layer_dispose (GObject *object)
{
  ClayLayer *layer = CLAY_LAYER (object);

  if (layer->mask)
    g_signal_handlers_disconnect_by_func (layer->mask,
                                          clay_layer_layer_mask_update,
                                          layer);

  if (clay_layer_is_floating_sel (layer))
    {
      ClayDrawable *fs_drawable = clay_layer_get_floating_sel_drawable (layer);

      /* only detach if this is actually the drawable's fs because the
       * layer might be on the undo stack and not attached to anything
       */
      if (clay_drawable_get_floating_sel (fs_drawable) == layer)
        clay_drawable_detach_floating_sel (fs_drawable);

      clay_layer_set_floating_sel_drawable (layer, NULL);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_layer_finalize (GObject *object)
{
  ClayLayer *layer = CLAY_LAYER (object);

  g_clear_object (&layer->mask);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_layer_update_mode_node (ClayLayer *layer)
{
  GeglNode               *mode_node;
  ClayLayerMode           visible_mode;
  ClayLayerColorSpace     visible_blend_space;
  ClayLayerColorSpace     visible_composite_space;
  ClayLayerCompositeMode  visible_composite_mode;

  mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));

  if (layer->mask && layer->show_mask)
    {
      visible_mode            = CLAY_LAYER_MODE_NORMAL;
      visible_blend_space     = CLAY_LAYER_COLOR_SPACE_AUTO;
      visible_composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
      visible_composite_mode  = CLAY_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      visible_composite_space = clay_layer_get_real_composite_space (layer);
    }
  else
    {
      visible_mode            = layer->effective_mode;
      visible_blend_space     = layer->effective_blend_space;
      visible_composite_space = layer->effective_composite_space;
      visible_composite_mode  = layer->effective_composite_mode;
    }

  clay_gegl_mode_node_set_mode (mode_node,
                                visible_mode,
                                visible_blend_space,
                                visible_composite_space,
                                visible_composite_mode);
  clay_gegl_mode_node_set_opacity (mode_node, layer->opacity);
}

static void
clay_layer_notify (GObject    *object,
                   GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "is-last-node") &&
      clay_filter_peek_node (CLAY_FILTER (object)))
    {
      clay_layer_update_mode_node (CLAY_LAYER (object));

      clay_drawable_update (CLAY_DRAWABLE (object), 0, 0, -1, -1);
    }
}

static void
clay_layer_name_changed (ClayObject *object)
{
  ClayLayer *layer = CLAY_LAYER (object);

  if (CLAY_OBJECT_CLASS (parent_class)->name_changed)
    CLAY_OBJECT_CLASS (parent_class)->name_changed (object);

  if (layer->mask)
    {
      gchar *mask_name = g_strdup_printf (_("%s mask"),
                                          clay_object_get_name (object));

      clay_object_take_name (CLAY_OBJECT (layer->mask), mask_name);
    }
}

static gint64
clay_layer_get_memsize (ClayObject *object,
                        gint64     *gui_size)
{
  ClayLayer *layer   = CLAY_LAYER (object);
  gint64     memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (layer->mask), gui_size);

  *gui_size += layer->fs.num_segs * sizeof (ClayBoundSeg);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_layer_invalidate_preview (ClayViewable *viewable)
{
  ClayLayer *layer = CLAY_LAYER (viewable);

  CLAY_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  if (clay_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static gchar *
clay_layer_get_description (ClayViewable  *viewable,
                            gchar        **tooltip)
{
  if (clay_layer_is_floating_sel (CLAY_LAYER (viewable)))
    {
      ClayDrawable *drawable;
      const gchar  *header = _("Floating Selection");

      drawable = clay_layer_get_floating_sel_drawable (CLAY_LAYER (viewable));
      if (CLAY_IS_LAYER_MASK (drawable))
        header = _("Floating Mask");
      if (CLAY_IS_CHANNEL (drawable))
        header = _("Floating Channel");
      else if (CLAY_IS_LAYER (drawable))
        header = _("Floating Layer");
      /* TRANSLATORS: the first %s will be the type of floating item, i.e.
       * either a "Floating Layer" or "Floating Mask" usually. The second will
       * be a layer name.
       */
      return g_strdup_printf (_("%s\n(%s)"),
                              header, clay_object_get_name (viewable));
    }

  return CLAY_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
clay_layer_get_node (ClayFilter *filter)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (filter);
  ClayLayer    *layer    = CLAY_LAYER (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *source;
  GeglNode     *mode_node;
  gboolean      source_node_hijacked = FALSE;

  node = CLAY_FILTER_CLASS (parent_class)->get_node (filter);

  input = gegl_node_get_input_proxy (node, "input");

  source = clay_drawable_get_source_node (drawable);

  /* if the source node already has a parent, we are a floating
   * selection and the source node has been hijacked by the fs'
   * drawable
   */
  if (gegl_node_get_parent (source))
    source_node_hijacked = TRUE;

  if (! source_node_hijacked)
    gegl_node_add_child (node, source);

  gegl_node_connect (input, "output", source, "input");

  g_warn_if_fail (layer->layer_offset_node == NULL);
  g_warn_if_fail (layer->mask_offset_node == NULL);

  /* the mode node connects it all, and has aux and aux2 inputs for
   * the layer and its mask
   */
  mode_node = clay_drawable_get_mode_node (drawable);
  clay_layer_update_mode_node (layer);

  /* the layer's offset node */
  layer->layer_offset_node = gegl_node_new_child (node,
                                                  "operation", "gegl:translate",
                                                  NULL);
  clay_item_add_offset_node (CLAY_ITEM (layer), layer->layer_offset_node);

  /* the layer mask's offset node */
  layer->mask_offset_node = gegl_node_new_child (node,
                                                 "operation", "gegl:translate",
                                                  NULL);
  clay_item_add_offset_node (CLAY_ITEM (layer), layer->mask_offset_node);

  if (! source_node_hijacked)
    {
      gegl_node_connect (source,                   "output",
                         layer->layer_offset_node, "input");
    }

  if (! (layer->mask && clay_layer_get_show_mask (layer)))
    {
      gegl_node_connect (layer->layer_offset_node, "output",
                         mode_node,                "aux");
    }

  if (layer->mask)
    {
      GeglNode *mask;

      mask = clay_drawable_get_source_node (CLAY_DRAWABLE (layer->mask));

      gegl_node_connect (mask,                    "output",
                         layer->mask_offset_node, "input");

      if (clay_layer_get_show_mask (layer))
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux");
        }
      else if (clay_layer_get_apply_mask (layer))
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux2");
        }
    }

  return node;
}

static void
clay_layer_removed (ClayItem *item)
{
  ClayLayer *layer = CLAY_LAYER (item);

  if (layer->mask)
    clay_item_removed (CLAY_ITEM (layer->mask));

  if (CLAY_ITEM_CLASS (parent_class)->removed)
    CLAY_ITEM_CLASS (parent_class)->removed (item);
}

static void
clay_layer_unset_removed (ClayItem *item)
{
  ClayLayer *layer = CLAY_LAYER (item);

  if (layer->mask)
    clay_item_unset_removed (CLAY_ITEM (layer->mask));

  if (CLAY_ITEM_CLASS (parent_class)->unset_removed)
    CLAY_ITEM_CLASS (parent_class)->unset_removed (item);
}

static gboolean
clay_layer_is_attached (ClayItem *item)
{
  ClayImage *image = clay_item_get_image (item);

  return (CLAY_IS_IMAGE (image) &&
          clay_container_have (clay_image_get_layers (image),
                               CLAY_OBJECT (item)));
}

static ClayItemTree *
clay_layer_get_tree (ClayItem *item)
{
  if (clay_item_is_attached (item))
    {
      ClayImage *image = clay_item_get_image (item);

      return clay_image_get_layer_tree (image);
    }

  return NULL;
}

static ClayItem *
clay_layer_duplicate (ClayItem *item,
                      GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_LAYER (new_item))
    {
      ClayLayer *layer     = CLAY_LAYER (item);
      ClayLayer *new_layer = CLAY_LAYER (new_item);

      /* PASS_THROUGH mode is invalid for regular layers.
       * We used to change the mode to NORMAL *before* duplicating (see
       * #793714 on bugzilla) but it would change the image's render.
       * Instead we first duplicate so that the group's render is used
       * as-is for the non-group duplicate layer. Then we set NORMAL
       * mode.
       */
      if (clay_layer_get_mode (layer) == CLAY_LAYER_MODE_PASS_THROUGH &&
          ! CLAY_IS_GROUP_LAYER (new_item))
        {
          ClayLayerColorSpace    blend_space;
          ClayLayerColorSpace    composite_space;
          ClayLayerCompositeMode composite_mode;

          /* keep the group's current blend space, composite space, and composite
           * mode.
           */
          blend_space     = clay_layer_get_blend_space     (layer);
          composite_space = clay_layer_get_composite_space (layer);
          composite_mode  = clay_layer_get_composite_mode  (layer);

          clay_layer_set_mode            (new_layer, CLAY_LAYER_MODE_NORMAL, FALSE);
          clay_layer_set_blend_space     (new_layer, blend_space,            FALSE);
          clay_layer_set_composite_space (new_layer, composite_space,        FALSE);
          clay_layer_set_composite_mode  (new_layer, composite_mode,         FALSE);
          clay_layer_set_opacity         (new_layer, 1.0,                    FALSE);
        }
      else
        {
          clay_layer_set_mode            (new_layer,
                                          clay_layer_get_mode (layer), FALSE);
          clay_layer_set_blend_space     (new_layer,
                                          clay_layer_get_blend_space (layer), FALSE);
          clay_layer_set_composite_space (new_layer,
                                          clay_layer_get_composite_space (layer), FALSE);
          clay_layer_set_composite_mode  (new_layer,
                                          clay_layer_get_composite_mode (layer), FALSE);
          clay_layer_set_opacity         (new_layer,
                                          clay_layer_get_opacity (layer), FALSE);
        }

      if (clay_layer_can_lock_alpha (new_layer))
        clay_layer_set_lock_alpha (new_layer,
                                   clay_layer_get_lock_alpha (layer), FALSE);

      /*  duplicate the layer mask if necessary  */
      if (layer->mask)
        {
          ClayItem *mask;

          mask = clay_item_duplicate (CLAY_ITEM (layer->mask),
                                      G_TYPE_FROM_INSTANCE (layer->mask));
          clay_layer_add_mask (new_layer, CLAY_LAYER_MASK (mask),
                               layer->edit_mask, FALSE, NULL);

          new_layer->apply_mask = layer->apply_mask;
          new_layer->show_mask  = layer->show_mask;
        }
    }

  return new_item;
}

static void
clay_layer_convert (ClayItem  *item,
                    ClayImage *dest_image,
                    GType      old_type)
{
  ClayLayer         *layer    = CLAY_LAYER (item);
  ClayDrawable      *drawable = CLAY_DRAWABLE (item);
  ClayImageBaseType  old_base_type;
  ClayImageBaseType  new_base_type;
  ClayPrecision      old_precision;
  ClayPrecision      new_precision;
  ClayColorProfile  *src_profile  = NULL;
  ClayColorProfile  *dest_profile = NULL;

  old_base_type = clay_drawable_get_base_type (drawable);
  new_base_type = clay_image_get_base_type (dest_image);

  old_precision = clay_drawable_get_precision (drawable);
  new_precision = clay_image_get_precision (dest_image);

  if (g_type_is_a (old_type, CLAY_TYPE_LAYER))
    {
      src_profile =
        clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (item));

      dest_profile =
        clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (dest_image));

      if (clay_color_profile_is_equal (dest_profile, src_profile))
        {
          src_profile  = NULL;
          dest_profile = NULL;
        }
    }

  if (old_base_type != new_base_type ||
      old_precision != new_precision ||
      dest_profile)
    {
      clay_drawable_convert_type (drawable, dest_image,
                                  new_base_type,
                                  new_precision,
                                  clay_drawable_has_alpha (drawable),
                                  src_profile,
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  FALSE, NULL);
    }

  if (layer->mask)
    clay_item_set_image (CLAY_ITEM (layer->mask), dest_image);

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static gboolean
clay_layer_rename (ClayItem     *item,
                   const gchar  *new_name,
                   const gchar  *undo_desc,
                   GError      **error)
{
  ClayLayer *layer = CLAY_LAYER (item);
  ClayImage *image = clay_item_get_image (item);
  gboolean   attached;
  gboolean   floating_sel;

  attached     = clay_item_is_attached (item);
  floating_sel = clay_layer_is_floating_sel (layer);

  if (floating_sel)
    {
      if (CLAY_IS_CHANNEL (clay_layer_get_floating_sel_drawable (layer)))
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("Cannot create a new layer from the floating "
                                 "selection because it belongs to a layer mask "
                                 "or channel."));
          return FALSE;
        }

      if (attached)
        {
          clay_image_undo_group_start (image,
                                       CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                                       undo_desc);

          floating_sel_to_layer (layer, NULL);
        }
    }

  CLAY_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error);

  if (attached && floating_sel)
    clay_image_undo_group_end (image);

  return TRUE;
}

static void
clay_layer_start_move (ClayItem *item,
                       gboolean  push_undo)
{
  ClayLayer *layer     = CLAY_LAYER (item);
  ClayLayer *ancestor  = layer;
  GSList    *ancestors = NULL;

  /* suspend mask cropping for all of the layer's ancestors */
  while ((ancestor = clay_layer_get_parent (ancestor)))
    {
      clay_group_layer_suspend_mask (CLAY_GROUP_LAYER (ancestor), push_undo);

      ancestors = g_slist_prepend (ancestors, g_object_ref (ancestor));
    }

  /* we keep the ancestor list around, so that we can resume mask cropping for
   * the same set of groups in clay_layer_end_move().  note that
   * clay_image_remove_layer() calls start_move() before removing the layer,
   * while it's still part of the layer tree, and end_move() afterwards, when
   * it's no longer part of the layer tree, and hence we can't use get_parent()
   * in end_move() to get the same set of ancestors.
   */
  layer->move_stack = g_slist_prepend (layer->move_stack, ancestors);

  if (CLAY_ITEM_CLASS (parent_class)->start_move)
    CLAY_ITEM_CLASS (parent_class)->start_move (item, push_undo);
}

static void
clay_layer_end_move (ClayItem *item,
                     gboolean  push_undo)
{
  ClayLayer *layer = CLAY_LAYER (item);
  GSList    *ancestors;
  GSList    *iter;

  g_return_if_fail (layer->move_stack != NULL);

  if (CLAY_ITEM_CLASS (parent_class)->end_move)
    CLAY_ITEM_CLASS (parent_class)->end_move (item, push_undo);

  ancestors = layer->move_stack->data;

  layer->move_stack = g_slist_remove (layer->move_stack, ancestors);

  /* resume mask cropping for all of the layer's ancestors */
  for (iter = ancestors; iter; iter = g_slist_next (iter))
    {
      ClayGroupLayer *ancestor = iter->data;

      clay_group_layer_resume_mask (ancestor, push_undo);

      g_object_unref (ancestor);
    }

  g_slist_free (ancestors);
}

static void
clay_layer_translate (ClayItem *item,
                      gdouble   offset_x,
                      gdouble   offset_y,
                      gboolean  push_undo)
{
  ClayLayer *layer = CLAY_LAYER (item);

  if (push_undo)
    clay_image_undo_push_item_displace (clay_item_get_image (item), NULL, item);

  CLAY_LAYER_GET_CLASS (layer)->translate (layer,
                                           SIGNED_ROUND (offset_x),
                                           SIGNED_ROUND (offset_y));

  if (layer->mask)
    {
      gint off_x, off_y;

      clay_item_get_offset (item, &off_x, &off_y);
      clay_item_set_offset (CLAY_ITEM (layer->mask), off_x, off_y);

      clay_viewable_invalidate_preview (CLAY_VIEWABLE (layer->mask));
    }
}

static void
clay_layer_scale (ClayItem              *item,
                  gint                   new_width,
                  gint                   new_height,
                  gint                   new_offset_x,
                  gint                   new_offset_y,
                  ClayInterpolationType  interpolation_type,
                  ClayProgress          *progress)
{
  ClayLayer       *layer = CLAY_LAYER (item);
  ClayObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      ClayLayerMask *mask;

      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      clay_object_queue_push (queue, layer);
      clay_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    clay_object_queue_pop (queue);

  CLAY_LAYER_GET_CLASS (layer)->scale (layer, new_width, new_height,
                                       new_offset_x, new_offset_y,
                                       interpolation_type, progress);

  if (layer->mask)
    {
      if (queue)
        clay_object_queue_pop (queue);

      clay_item_scale (CLAY_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type, progress);
    }

  g_clear_object (&queue);
}

static void
clay_layer_resize (ClayItem     *item,
                   ClayContext  *context,
                   ClayFillType  fill_type,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y)
{
  ClayLayer *layer  = CLAY_LAYER (item);

  CLAY_LAYER_GET_CLASS (layer)->resize (layer, context, fill_type,
                                        new_width, new_height,
                                        offset_x, offset_y);

  if (layer->mask)
    clay_item_resize (CLAY_ITEM (layer->mask), context, CLAY_FILL_TRANSPARENT,
                      new_width, new_height, offset_x, offset_y);
}

static void
clay_layer_flip (ClayItem            *item,
                 ClayContext         *context,
                 ClayOrientationType  flip_type,
                 gdouble              axis,
                 gboolean             clip_result)
{
  ClayLayer *layer = CLAY_LAYER (item);

  CLAY_LAYER_GET_CLASS (layer)->flip (layer, context, flip_type, axis,
                                      clip_result);

  if (layer->mask)
    clay_item_flip (CLAY_ITEM (layer->mask), context,
                    flip_type, axis, clip_result);
}

static void
clay_layer_rotate (ClayItem         *item,
                   ClayContext      *context,
                   ClayRotationType  rotate_type,
                   gdouble           center_x,
                   gdouble           center_y,
                   gboolean          clip_result)
{
  ClayLayer *layer = CLAY_LAYER (item);

  CLAY_LAYER_GET_CLASS (layer)->rotate (layer, context,
                                        rotate_type, center_x, center_y,
                                        clip_result);

  if (layer->mask)
    clay_item_rotate (CLAY_ITEM (layer->mask), context,
                      rotate_type, center_x, center_y, clip_result);
}

static void
clay_layer_transform (ClayItem               *item,
                      ClayContext            *context,
                      const ClayMatrix3      *matrix,
                      ClayTransformDirection  direction,
                      ClayInterpolationType   interpolation_type,
                      ClayTransformResize     clip_result,
                      ClayProgress           *progress,
                      gboolean                push_undo)
{
  ClayLayer       *layer = CLAY_LAYER (item);
  ClayObjectQueue *queue = NULL;

  if (progress && layer->mask)
    {
      ClayLayerMask *mask;

      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      clay_object_queue_push (queue, layer);
      clay_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    clay_object_queue_pop (queue);

  CLAY_LAYER_GET_CLASS (layer)->transform (layer, context, matrix, direction,
                                           interpolation_type,
                                           clip_result,
                                           progress, push_undo);

  if (layer->mask)
    {
      if (queue)
        clay_object_queue_pop (queue);

      clay_item_transform (CLAY_ITEM (layer->mask), context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  g_clear_object (&queue);
}

static void
clay_layer_to_selection (ClayItem       *item,
                         ClayChannelOps  op,
                         gboolean        antialias,
                         gboolean        feather,
                         gdouble         feather_radius_x,
                         gdouble         feather_radius_y)
{
  ClayLayer *layer = CLAY_LAYER (item);
  ClayImage *image = clay_item_get_image (item);

  clay_channel_select_alpha (clay_image_get_mask (image),
                             CLAY_DRAWABLE (layer),
                             op,
                             feather, feather_radius_x, feather_radius_y);
}

static void
clay_layer_alpha_changed (ClayDrawable *drawable)
{
  if (CLAY_DRAWABLE_CLASS (parent_class)->alpha_changed)
    CLAY_DRAWABLE_CLASS (parent_class)->alpha_changed (drawable);

  /* When we add/remove alpha, whatever cached color transforms in
   * view renderers need to be recreated because they cache the wrong
   * lcms formats. See bug 478528.
   */
  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (drawable));
}

static gint64
clay_layer_estimate_memsize (ClayDrawable      *drawable,
                             ClayComponentType  component_type,
                             gint               width,
                             gint               height)
{
  ClayLayer *layer   = CLAY_LAYER (drawable);
  gint64     memsize = 0;

  if (layer->mask)
    memsize += clay_drawable_estimate_memsize (CLAY_DRAWABLE (layer->mask),
                                               component_type,
                                               width, height);

  return memsize +
         CLAY_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static gboolean
clay_layer_supports_alpha (ClayDrawable *drawable)
{
  return TRUE;
}

static void
clay_layer_convert_type (ClayDrawable     *drawable,
                         ClayImage        *dest_image,
                         const Babl       *new_format,
                         ClayColorProfile *src_profile,
                         ClayColorProfile *dest_profile,
                         GeglDitherMethod  layer_dither_type,
                         GeglDitherMethod  mask_dither_type,
                         gboolean          push_undo,
                         ClayProgress     *progress)
{
  ClayLayer       *layer      = CLAY_LAYER (drawable);
  ClayObjectQueue *queue      = NULL;
  const Babl      *dest_space = NULL;
  const Babl      *space_format;
  gboolean         convert_mask;

  convert_mask = layer->mask &&
                 clay_babl_format_get_precision (new_format) !=
                 clay_drawable_get_precision (CLAY_DRAWABLE (layer->mask));

  if (progress && convert_mask)
    {
      ClayLayerMask *mask;

      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      clay_object_queue_push (queue, layer);
      clay_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    clay_object_queue_pop (queue);

  /*  when called with a dest_profile, always use the space from that
   *  profile, we can't use clay_image_get_layer_space() during an
   *  image type or precision conversion
   */
  if (dest_profile)
    {
      dest_space =
        clay_color_profile_get_space (dest_profile,
                                      CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                      NULL);
    }

  space_format = babl_format_with_space ((const gchar *) new_format, dest_space);

  CLAY_LAYER_GET_CLASS (layer)->convert_type (layer, dest_image, space_format,
                                              src_profile, dest_profile,
                                              layer_dither_type,
                                              mask_dither_type, push_undo,
                                              progress);

  if (convert_mask)
    {
      if (queue)
        clay_object_queue_pop (queue);

      clay_drawable_convert_type (CLAY_DRAWABLE (layer->mask), dest_image,
                                  CLAY_GRAY,
                                  clay_babl_format_get_precision (new_format),
                                  clay_drawable_has_alpha (CLAY_DRAWABLE (layer->mask)),
                                  NULL, NULL,
                                  layer_dither_type, mask_dither_type,
                                  push_undo, progress);
    }

  g_clear_object (&queue);
}

static void
clay_layer_invalidate_boundary (ClayDrawable *drawable)
{
  ClayLayer *layer = CLAY_LAYER (drawable);

  if (clay_item_is_attached (CLAY_ITEM (drawable)) &&
      clay_item_is_visible (CLAY_ITEM (drawable)))
    {
      ClayImage   *image = clay_item_get_image (CLAY_ITEM (drawable));
      ClayChannel *mask  = clay_image_get_mask (image);

      /*  Turn the current selection off  */
      clay_image_selection_invalidate (image);

      /*  Only bother with the bounds if there is a selection  */
      if (! clay_channel_is_empty (mask))
        {
          mask->bounds_known   = FALSE;
          mask->boundary_known = FALSE;
        }
    }

  if (clay_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static ClayComponentMask
clay_layer_get_active_mask (ClayDrawable *drawable)
{
  ClayLayer         *layer = CLAY_LAYER (drawable);
  ClayImage         *image = clay_item_get_image (CLAY_ITEM (drawable));
  ClayComponentMask  mask  = clay_image_get_active_mask (image);

  if (clay_drawable_has_alpha (drawable) &&
      clay_layer_is_alpha_locked (layer, NULL))
    mask &= ~CLAY_COMPONENT_MASK_ALPHA;

  return mask;
}

static void
clay_layer_set_buffer (ClayDrawable        *drawable,
                       gboolean             push_undo,
                       const gchar         *undo_desc,
                       GeglBuffer          *buffer,
                       const GeglRectangle *bounds)
{
  GeglBuffer *old_buffer = clay_drawable_get_buffer (drawable);
  gint        old_trc    = -1;

  if (old_buffer)
    old_trc = clay_drawable_get_trc (drawable);

  CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (clay_filter_peek_node (CLAY_FILTER (drawable)))
    {
      if (clay_drawable_get_trc (drawable) != old_trc)
        clay_layer_update_mode_node (CLAY_LAYER (drawable));
    }
}

static GeglRectangle
clay_layer_get_bounding_box (ClayDrawable *drawable)
{
  ClayLayer     *layer = CLAY_LAYER (drawable);
  ClayLayerMask *mask  = clay_layer_get_mask (layer);
  GeglRectangle  bounding_box;

  if (mask && clay_layer_get_show_mask (layer))
    {
      bounding_box = clay_drawable_get_bounding_box (CLAY_DRAWABLE (mask));
    }
  else
    {
      bounding_box = CLAY_LAYER_GET_CLASS (layer)->get_bounding_box (layer);

      if (mask && clay_layer_get_apply_mask (layer))
        {
          GeglRectangle mask_bounding_box;

          mask_bounding_box = clay_drawable_get_bounding_box (
            CLAY_DRAWABLE (mask));

          gegl_rectangle_intersect (&bounding_box,
                                    &bounding_box, &mask_bounding_box);
        }
    }

  return bounding_box;
}

static ClayColorProfile *
clay_layer_get_color_profile (ClayColorManaged *managed)
{
  ClayImage *image = clay_item_get_image (CLAY_ITEM (managed));

  return clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));
}

static gdouble
clay_layer_get_opacity_at (ClayPickable *pickable,
                           gint          x,
                           gint          y)
{
  ClayLayer *layer = CLAY_LAYER (pickable);
  gdouble    value = CLAY_OPACITY_TRANSPARENT;

  if (x >= 0 && x < clay_item_get_width  (CLAY_ITEM (layer)) &&
      y >= 0 && y < clay_item_get_height (CLAY_ITEM (layer)) &&
      clay_item_is_visible (CLAY_ITEM (layer)))
    {
      if (! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
        {
          value = CLAY_OPACITY_OPAQUE;
        }
      else
        {
          gegl_buffer_sample (clay_drawable_get_buffer (CLAY_DRAWABLE (layer)),
                              x, y, NULL, &value, babl_format ("A double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }

      if (clay_layer_get_mask (layer) &&
          clay_layer_get_apply_mask (layer))
        {
          gdouble mask_value;

          mask_value = clay_pickable_get_opacity_at (CLAY_PICKABLE (layer->mask),
                                                     x, y);

          value *= mask_value;
        }
    }

  return value;
}

static void
clay_layer_save (ClaySavable   *savable,
                 ClaySaveState *state)
{
  ClayLayer              *layer = CLAY_LAYER (savable);
  ClayImage              *image = clay_item_get_image (CLAY_ITEM (layer));
  ClayLayerMode           mode;
  ClayLayerColorSpace     space;
  ClayLayerCompositeMode  composite_mode;
  gint                    offset_x;
  gint                    offset_y;
  gboolean                drop_root;

  drop_root = (G_TYPE_FROM_INSTANCE (layer) != CLAY_TYPE_LAYER);

  if (! drop_root)
    clay_savable_print_element_start (state, "raster-layer", NULL);

  if (clay_image_get_floating_selection (image) == layer)
    {
      ClayItem *item = CLAY_ITEM (clay_layer_get_floating_sel_drawable (layer));
      clay_savable_print_element (state, "floating", NULL, NULL,
                                  "attached-to", "%u", (guint) clay_item_get_tattoo (item),
                                  NULL);
    }

  parent_savable_iface->save (savable, state);

  clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);
  if (! CLAY_IS_GROUP_LAYER (layer))
    clay_savable_print_element (state, "offsets", NULL, NULL,
                                "x", "%d", offset_x,
                                "y", "%d", offset_y,
                                NULL);

  clay_savable_print_element (state, "opacity", "%f",
                              clay_layer_get_opacity (layer), NULL);

  mode = clay_layer_get_mode (layer);
  clay_savable_print_element (state, "mode", "%[ClayLayerMode]", mode, NULL);

  space = clay_layer_get_blend_space (layer);
  clay_savable_blend_space_save (space, mode, state);

  space = clay_layer_get_composite_space (layer);
  clay_savable_composite_space_save (space, mode, state);

  composite_mode = clay_layer_get_composite_mode (layer);
  clay_savable_composite_mode_save (composite_mode, mode, state);

  if (clay_layer_get_lock_alpha (layer))
    clay_savable_print_element (state, "lock-alpha", NULL, NULL, NULL);

  if (clay_layer_get_mask (layer))
    {
      clay_savable_print_element_start (state, "mask",
                                        "apply", "%b", clay_layer_get_apply_mask (layer),
                                        "edit",  "%b", clay_layer_get_edit_mask (layer),
                                        "show",  "%b", clay_layer_get_show_mask (layer),
                                        NULL);
      clay_savable_save (CLAY_SAVABLE (clay_layer_get_mask (layer)), state);
      clay_savable_print_element_end (state, "mask");
    }

  if (! drop_root)
    clay_savable_print_element_end (state, "raster-layer");
}

static gboolean
clay_layer_load_enter (ClayLoadState  *state,
                       const gchar   **attribute_names,
                       const gchar   **attribute_values,
                       gpointer        user_data,
                       GError        **error)
{
  ClayLayer *layer;
  GType      real_type;

  real_type = user_data ? CLAYPOINTER_TO_TYPE (user_data) : CLAY_TYPE_LAYER;
  if (real_type == CLAY_TYPE_LAYER)
    {
      while (*attribute_names)
        {
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);

          attribute_names++;
          attribute_values++;
        }
    }

  parent_savable_iface->load_enter (state, attribute_names, attribute_values, user_data, error);

  clay_savable_load_add_simple_handler (state, "floating", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "attached-to", G_TYPE_UINT,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "offsets", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "x", G_TYPE_INT,
                                        "y", G_TYPE_INT,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "opacity", G_TYPE_DOUBLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "mode", CLAY_TYPE_LAYER_MODE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "blend-space", CLAY_TYPE_LAYER_COLOR_SPACE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "composite-space", CLAY_TYPE_LAYER_COLOR_SPACE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "composite-mode", CLAY_TYPE_LAYER_COMPOSITE_MODE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "lock-alpha", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);

  clay_savable_load_add_handlers (state, "mask",
                                  clay_layer_enter_mask,
                                  clay_layer_exit_mask,
                                  NULL, NULL);

  if (real_type == CLAY_TYPE_LAYER)
    {
      layer = clay_layer_new (state->image, 1, 1,
                              clay_image_get_layer_format (state->image, FALSE),
                              NULL,
                              CLAY_OPACITY_OPAQUE, CLAY_LAYER_MODE_NORMAL);
      clay_savable_load_push_active_object (state, G_OBJECT (layer));
    }

  return TRUE;
}

static gboolean
clay_layer_load_exit (ClayLoadState  *state,
                      const gchar    *text,
                      gsize           len,
                      gpointer        user_data,
                      GError        **error)
{
  ClayLayer              *layer;
  ClayLayerMask          *mask             = NULL;
  const gchar            *name             = NULL;
  gdouble                 opacity          = -1.0;
  ClayLayerMode           mode             = CLAY_LAYER_MODE_NORMAL;
  ClayLayerColorSpace     blend_space      = CLAY_LAYER_COLOR_SPACE_AUTO;
  ClayLayerColorSpace     composite_space  = CLAY_LAYER_COLOR_SPACE_AUTO;
  ClayLayerCompositeMode  composite_mode   = CLAY_LAYER_COMPOSITE_AUTO;
  gboolean                blend_space_auto = FALSE;
  gboolean                compo_space_auto = FALSE;
  gboolean                compo_mode_auto  = FALSE;
  gboolean                lock_alpha       = FALSE;
  gint                    offset_x         = -1;
  gint                    offset_y         = -1;

  g_return_val_if_fail (CLAY_IS_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  parent_savable_iface->load_exit (state, text, len, user_data, error);

  layer = CLAY_LAYER (clay_savable_load_pop_active_object (state));

  if (! clay_savable_load_get_values (state,
                                      "name",            &name,
                                      "opacity",         &opacity,
                                      "mode",            &mode,
                                      "blend-space",     &blend_space,
                                      "composite-space", &composite_space,
                                      "composite-mode",  &composite_mode,
                                      NULL))
    {
      if (name)
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: failed to load layer '%s' because of missing elements.",
                     G_STRFUNC, name);
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: failed to load a layer because of missing elements.",
                     G_STRFUNC);

      g_object_unref (layer);
      return TRUE;
    }

  if (opacity >= CLAY_OPACITY_TRANSPARENT && opacity <= CLAY_OPACITY_OPAQUE)
    clay_layer_set_opacity (layer, opacity, FALSE);
  clay_layer_set_mode (layer, mode, FALSE);

  clay_savable_load_get_values (state,
                                "blend-space:auto",     &blend_space_auto,
                                "composite-space:auto", &compo_space_auto,
                                "composite-mode:auto",  &compo_mode_auto,
                                NULL);
  /* If "Auto" space/mode was set, we re-apply it, unless the actual
   * space/mode changes. This allows us to change the default
   * composite/blend spaces or mode for any layer mode in the future
   * without breaking renders of existing files.
   */
  if (blend_space_auto &&
      blend_space == clay_layer_mode_get_blend_space (mode))
    blend_space = CLAY_LAYER_COLOR_SPACE_AUTO;
  if (compo_space_auto &&
      composite_space == clay_layer_mode_get_composite_space (mode))
    composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
  if (compo_mode_auto &&
      composite_mode == clay_layer_mode_get_composite_mode (mode))
    composite_mode = CLAY_LAYER_COMPOSITE_AUTO;

  clay_layer_set_blend_space (layer, blend_space, FALSE);
  clay_layer_set_composite_space (layer, composite_space, FALSE);
  clay_layer_set_composite_mode (layer, composite_mode, FALSE);

  clay_savable_load_get_values (state,
                                "lock-alpha", &lock_alpha,
                                NULL);

  clay_layer_set_lock_alpha (layer, lock_alpha, FALSE);
  if (! CLAY_IS_GROUP_LAYER (layer) &&
      clay_savable_load_get_values (state,
                                    "offsets:x", &offset_x,
                                    "offsets:y", &offset_y,
                                    NULL))
    clay_item_set_offset (CLAY_ITEM (layer), offset_x, offset_y);

  /* TODO: handle floating layers too. */
  if (! clay_item_is_attached (CLAY_ITEM (layer)))
    {
      ClayLayer     *parent = NULL;
      ClayContainer *container;

      if (CLAY_IS_GROUP_LAYER (clay_savable_load_peek_active_object (state)))
        {
          parent    = CLAY_LAYER (clay_savable_load_peek_active_object (state));
          container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
        }
      else
        {
          container = clay_image_get_layers (state->image);
        }

      clay_image_add_layer (state->image, layer, parent,
                            clay_container_get_n_children (container),
                            FALSE);
    }

  if (clay_savable_load_get_values (state, "layer-mask", &mask, NULL))
    {
      gboolean apply = TRUE;
      gboolean edit  = FALSE;
      gboolean show  = FALSE;

      clay_savable_load_get_values (state,
                                    "layer-mask:edit",  &edit,
                                    "layer-mask:apply", &apply,
                                    "layer-mask:show",  &show,
                                    NULL);
      clay_layer_add_mask (layer, g_object_ref (mask), edit, FALSE, NULL);
      clay_layer_set_apply_mask (layer, apply, FALSE);
      clay_layer_set_show_mask  (layer, show, FALSE);
    }

  return TRUE;
}

static gboolean
clay_layer_real_is_alpha_locked (ClayLayer  *layer,
                                 ClayLayer **locked_layer)
{
  ClayItem *parent = clay_item_get_parent (CLAY_ITEM (layer));

  if (layer->lock_alpha)
    {
      if (locked_layer)
        *locked_layer = layer;
    }
  else if (parent &&
           clay_layer_is_alpha_locked (CLAY_LAYER (parent), locked_layer))
    {
      return TRUE;
    }

  return layer->lock_alpha;
}

static void
clay_layer_real_translate (ClayLayer *layer,
                           gint       offset_x,
                           gint       offset_y)
{
  /*  update the old region  */
  clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);

  /*  invalidate the selection boundary because of a layer modification  */
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (layer));

  CLAY_ITEM_CLASS (parent_class)->translate (CLAY_ITEM (layer),
                                             offset_x, offset_y,
                                             FALSE);

  /*  update the new region  */
  clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);
}

static void
clay_layer_real_scale (ClayLayer             *layer,
                       gint                   new_width,
                       gint                   new_height,
                       gint                   new_offset_x,
                       gint                   new_offset_y,
                       ClayInterpolationType  interpolation_type,
                       ClayProgress          *progress)
{
  CLAY_ITEM_CLASS (parent_class)->scale (CLAY_ITEM (layer),
                                         new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type, progress);
}

static void
clay_layer_real_resize (ClayLayer    *layer,
                        ClayContext  *context,
                        ClayFillType  fill_type,
                        gint          new_width,
                        gint          new_height,
                        gint          offset_x,
                        gint          offset_y)
{
  if (fill_type == CLAY_FILL_TRANSPARENT &&
      ! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    {
      fill_type = CLAY_FILL_BACKGROUND;
    }

  CLAY_ITEM_CLASS (parent_class)->resize (CLAY_ITEM (layer),
                                          context, fill_type,
                                          new_width, new_height,
                                          offset_x, offset_y);
}

static void
clay_layer_real_flip (ClayLayer           *layer,
                      ClayContext         *context,
                      ClayOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  CLAY_ITEM_CLASS (parent_class)->flip (CLAY_ITEM (layer),
                                        context, flip_type, axis, clip_result);
}

static void
clay_layer_real_rotate (ClayLayer        *layer,
                        ClayContext      *context,
                        ClayRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  CLAY_ITEM_CLASS (parent_class)->rotate (CLAY_ITEM (layer),
                                          context, rotate_type,
                                          center_x, center_y,
                                          clip_result);
}

static void
clay_layer_real_transform (ClayLayer              *layer,
                           ClayContext            *context,
                           const ClayMatrix3      *matrix,
                           ClayTransformDirection  direction,
                           ClayInterpolationType   interpolation_type,
                           ClayTransformResize     clip_result,
                           ClayProgress           *progress,
                           gboolean                push_undo)
{
  if (! clay_matrix3_is_simple (matrix) &&
      ! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    clay_layer_add_alpha (CLAY_LAYER (layer));

  CLAY_ITEM_CLASS (parent_class)->transform (CLAY_ITEM (layer),
                                             context, matrix, direction,
                                             interpolation_type,
                                             clip_result,
                                             progress, push_undo);
}

static void
clay_layer_real_convert_type (ClayLayer        *layer,
                              ClayImage        *dest_image,
                              const Babl       *new_format,
                              ClayColorProfile *src_profile,
                              ClayColorProfile *dest_profile,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              gboolean          push_undo,
                              ClayProgress     *progress)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (layer);
  GeglBuffer   *src_buffer;
  GeglBuffer   *dest_buffer;

  if (layer_dither_type == GEGL_DITHER_NONE)
    {
      src_buffer = g_object_ref (clay_drawable_get_buffer (drawable));
    }
  else
    {
      gint bits;

      src_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                         clay_item_get_width  (CLAY_ITEM (layer)),
                                         clay_item_get_height (CLAY_ITEM (layer))),
                         clay_drawable_get_format (drawable));

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      clay_gegl_apply_dither (clay_drawable_get_buffer (drawable),
                              NULL, NULL,
                              src_buffer, 1 << bits, layer_dither_type);
    }

  dest_buffer =
    clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                          clay_item_get_width  (CLAY_ITEM (layer)),
                                          clay_item_get_height (CLAY_ITEM (layer))),
                          new_format, drawable);

  if (dest_profile)
    {
      if (! src_profile)
        src_profile =
          clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (layer));

      clay_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                       dest_buffer, NULL, dest_profile,
                                       CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                       TRUE, progress);
    }
  else
    {
      clay_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
    }

  clay_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);
}

static GeglRectangle
clay_layer_real_get_bounding_box (ClayLayer *layer)
{
  return CLAY_DRAWABLE_CLASS (parent_class)->get_bounding_box (
    CLAY_DRAWABLE (layer));
}

static void
clay_layer_real_get_effective_mode (ClayLayer              *layer,
                                    ClayLayerMode          *mode,
                                    ClayLayerColorSpace    *blend_space,
                                    ClayLayerColorSpace    *composite_space,
                                    ClayLayerCompositeMode *composite_mode)
{
  *mode            = clay_layer_get_mode (layer);
  *blend_space     = clay_layer_get_real_blend_space (layer);
  *composite_space = clay_layer_get_real_composite_space (layer);
  *composite_mode  = clay_layer_get_real_composite_mode (layer);
}

static gboolean
clay_layer_real_get_excludes_backdrop (ClayLayer *layer)
{
  ClayLayerCompositeRegion included_region;

  included_region = clay_layer_mode_get_included_region (layer->mode,
                                                         layer->effective_composite_mode);

  return ! (included_region & CLAY_LAYER_COMPOSITE_REGION_DESTINATION);
}

static void
clay_layer_layer_mask_update (ClayDrawable *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height,
                              ClayLayer    *layer)
{
  if (clay_layer_get_apply_mask (layer) ||
      clay_layer_get_show_mask (layer))
    {
      clay_drawable_update (CLAY_DRAWABLE (layer),
                            x, y, width, height);
    }
}

static gboolean
clay_layer_enter_mask (ClayLoadState  *state,
                       const gchar   **attribute_names,
                       const gchar   **attribute_values,
                       gpointer        user_data,
                       GError        **error)
{
  clay_savable_load (CLAY_TYPE_CHANNEL, state);

  return TRUE;
}

static gboolean
clay_layer_exit_mask (ClayLoadState  *state,
                      const gchar    *text,
                      gsize           len,
                      gpointer        user_data,
                      GError        **error)
{
  clay_savable_load_bubble_up (state, "layer-mask");
  clay_savable_load_bubble_up (state, "layer-mask:apply");
  clay_savable_load_bubble_up (state, "layer-mask:edit");
  clay_savable_load_bubble_up (state, "layer-mask:show");

  return TRUE;
}


/*  public functions  */

void
clay_layer_fix_format_space (ClayLayer *layer,
                             gboolean   copy_buffer,
                             gboolean   push_undo)
{
  ClayDrawable *drawable;
  const Babl   *format;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  drawable = CLAY_DRAWABLE (layer);

  format = clay_image_get_layer_format (clay_item_get_image (CLAY_ITEM (layer)),
                                        clay_drawable_has_alpha (drawable));

  if (format != clay_drawable_get_format (drawable))
    {
      clay_drawable_set_format (drawable, format, copy_buffer, push_undo);
    }
}

ClayLayer *
clay_layer_get_parent (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);

  return CLAY_LAYER (clay_viewable_get_parent (CLAY_VIEWABLE (layer)));
}

ClayLayerMask *
clay_layer_get_mask (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);

  return layer->mask;
}

ClayLayerMask *
clay_layer_add_mask (ClayLayer      *layer,
                     ClayLayerMask  *mask,
                     gboolean        edit_mask,
                     gboolean        push_undo,
                     GError        **error)
{
  ClayImage *image;

  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (clay_item_get_image (CLAY_ITEM (layer)) ==
                        clay_item_get_image (CLAY_ITEM (mask)), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! clay_item_is_attached (CLAY_ITEM (layer)))
    push_undo = FALSE;

  image = clay_item_get_image (CLAY_ITEM (layer));

  if (layer->mask)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Unable to add a layer mask since "
                             "the layer already has one."));
      return NULL;
    }

  if (! CLAY_IS_GROUP_LAYER (layer) &&
      ((clay_item_get_width (CLAY_ITEM (layer)) !=
       clay_item_get_width (CLAY_ITEM (mask))) ||
      (clay_item_get_height (CLAY_ITEM (layer)) !=
       clay_item_get_height (CLAY_ITEM (mask)))))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot add layer mask of different "
                             "dimensions than specified layer."));
      return NULL;
    }

  if (push_undo)
    clay_image_undo_push_layer_mask_add (image, C_("undo-type", "Add Layer Masks"),
                                         layer, mask);

  layer->mask = g_object_ref_sink (mask);
  layer->apply_mask = TRUE;
  layer->edit_mask  = edit_mask;
  layer->show_mask  = FALSE;

  clay_layer_mask_set_layer (mask, layer);

  if (clay_filter_peek_node (CLAY_FILTER (layer)))
    {
      GeglNode *mode_node;
      GeglNode *mask;

      mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));

      mask = clay_drawable_get_source_node (CLAY_DRAWABLE (layer->mask));

      gegl_node_connect (mask,                    "output",
                         layer->mask_offset_node, "input");

      if (layer->show_mask)
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux");
        }
      else
        {
          gegl_node_connect (layer->mask_offset_node, "output",
                             mode_node,               "aux2");
        }

      clay_layer_update_mode_node (layer);
    }

  clay_drawable_update_bounding_box (CLAY_DRAWABLE (layer));

  clay_layer_update_effective_mode (layer);
  clay_layer_update_excludes_backdrop (layer);

  if (clay_layer_get_apply_mask (layer) ||
      clay_layer_get_show_mask (layer))
    {
      clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);
    }

  g_signal_connect (mask, "update",
                    G_CALLBACK (clay_layer_layer_mask_update),
                    layer);

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");

  /*  if the mask came from the undo stack, reset its "removed" state  */
  if (clay_item_is_removed (CLAY_ITEM (mask)))
    clay_item_unset_removed (CLAY_ITEM (mask));

  return layer->mask;
}

ClayLayerMask *
clay_layer_create_mask (ClayLayer       *layer,
                        ClayAddMaskType  add_mask_type,
                        ClayChannel     *channel)
{
  ClayDrawable  *drawable;
  ClayItem      *item;
  ClayLayerMask *mask;
  ClayImage     *image;
  guint          width;
  guint          height;
  gchar         *mask_name;
  GeglColor     *black = gegl_color_new ("black");

  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (add_mask_type != CLAY_ADD_MASK_CHANNEL ||
                        CLAY_IS_CHANNEL (channel), NULL);

  drawable = CLAY_DRAWABLE (layer);
  item     = CLAY_ITEM (layer);
  image    = clay_item_get_image (item);

  /* Since empty pass through layer groups are larger than their item
   * width/height, we'll get those from the bounding box instead */
  if (CLAY_IS_GROUP_LAYER (layer))
    {
      GeglRectangle rect = clay_layer_get_bounding_box (drawable);

      width  = rect.width;
      height = rect.height;
    }
  else
    {
      width  = clay_item_get_width (item);
      height = clay_item_get_height (item);
    }

  mask_name = g_strdup_printf (_("%s mask"),
                               clay_object_get_name (layer));

  mask = clay_layer_mask_new (image, width, height, mask_name, black);

  g_free (mask_name);
  g_object_unref (black);

  switch (add_mask_type)
    {
    case CLAY_ADD_MASK_WHITE:
      clay_channel_all (CLAY_CHANNEL (mask), FALSE);
      break;

    case CLAY_ADD_MASK_BLACK:
      clay_channel_clear (CLAY_CHANNEL (mask), NULL, FALSE);
      break;

    case CLAY_ADD_MASK_ALPHA:
    case CLAY_ADD_MASK_ALPHA_TRANSFER:
      if (clay_drawable_has_alpha (drawable))
        {
          GeglBuffer *dest_buffer;
          const Babl *component_format;

          dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

          component_format =
            clay_image_get_component_format (image, CLAY_CHANNEL_ALPHA);

          gegl_buffer_set_format (dest_buffer, component_format);
          clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 dest_buffer, NULL);
          gegl_buffer_set_format (dest_buffer, NULL);

          if (add_mask_type == CLAY_ADD_MASK_ALPHA_TRANSFER)
            {
              clay_drawable_push_undo (drawable,
                                       C_("undo-type", "Transfer Alpha to Mask"),
                                       NULL,
                                       0, 0,
                                       width, height);

              clay_gegl_apply_set_alpha (clay_drawable_get_buffer (drawable),
                                         NULL, NULL,
                                         clay_drawable_get_buffer (drawable),
                                         1.0);
            }
        }
      else
        {
          clay_channel_all (CLAY_CHANNEL (mask), FALSE);
        }
      break;

    case CLAY_ADD_MASK_SELECTION:
    case CLAY_ADD_MASK_CHANNEL:
      {
        gboolean channel_empty;
        gint     offset_x, offset_y;
        gint     copy_x, copy_y;
        gint     copy_width, copy_height;

        if (add_mask_type == CLAY_ADD_MASK_SELECTION)
          channel = CLAY_CHANNEL (clay_image_get_mask (image));

        channel_empty = clay_channel_is_empty (channel);

        clay_item_get_offset (item, &offset_x, &offset_y);

        clay_rectangle_intersect (0, 0,
                                  clay_image_get_width  (image),
                                  clay_image_get_height (image),
                                  offset_x, offset_y,
                                  width, height,
                                  &copy_x, &copy_y,
                                  &copy_width, &copy_height);

        if (copy_width  < width  ||
            copy_height < height ||
            channel_empty)
          clay_channel_clear (CLAY_CHANNEL (mask), NULL, FALSE);

        if ((copy_width || copy_height) && ! channel_empty)
          {
            GeglBuffer *src;
            GeglBuffer *dest;
            const Babl *format;

            src  = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));
            dest = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

            format = gegl_buffer_get_format (src);

            /* make sure no gamma conversion happens */
            gegl_buffer_set_format (dest, format);
            clay_gegl_buffer_copy (src,
                                   GEGL_RECTANGLE (copy_x, copy_y,
                                                   copy_width, copy_height),
                                   GEGL_ABYSS_NONE,
                                   dest,
                                   GEGL_RECTANGLE (copy_x - offset_x,
                                                   copy_y - offset_y,
                                                   0, 0));
            gegl_buffer_set_format (dest, NULL);

            CLAY_CHANNEL (mask)->bounds_known = FALSE;
          }
      }
      break;

    case CLAY_ADD_MASK_COPY:
      {
        GeglBuffer *src_buffer;
        GeglBuffer *dest_buffer;

        if (! clay_drawable_is_gray (drawable))
          {
            const Babl *copy_format =
              clay_image_get_format (image, CLAY_GRAY,
                                     clay_drawable_get_precision (drawable),
                                     clay_drawable_has_alpha (drawable),
                                     NULL);

            src_buffer =
              gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                               width, height),
                               copy_format);

            clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                                   GEGL_ABYSS_NONE,
                                   src_buffer, NULL);
          }
        else
          {
            src_buffer = clay_drawable_get_buffer (drawable);
            g_object_ref (src_buffer);
          }

        dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

        if (clay_drawable_has_alpha (drawable))
          {
            GeglColor *background = gegl_color_new ("transparent");

            clay_gegl_apply_flatten (src_buffer, NULL, NULL,
                                     dest_buffer, background,
                                     CLAY_LAYER_COLOR_SPACE_RGB_LINEAR);

            g_object_unref (background);
          }
        else
          {
            clay_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                                   dest_buffer, NULL);
          }

        g_object_unref (src_buffer);
      }

      CLAY_CHANNEL (mask)->bounds_known = FALSE;
      break;
    }

  return mask;
}

void
clay_layer_apply_mask (ClayLayer         *layer,
                       ClayMaskApplyMode  mode,
                       gboolean           push_undo)
{
  ClayItem      *item;
  ClayImage     *image;
  ClayLayerMask *mask;
  gboolean       view_changed = FALSE;

  g_return_if_fail (CLAY_IS_LAYER (layer));

  mask = clay_layer_get_mask (layer);

  if (! mask)
    return;

  /*  APPLY can not be done to group layers  */
  g_return_if_fail (! clay_viewable_get_children (CLAY_VIEWABLE (layer)) ||
                    mode == CLAY_MASK_DISCARD);

  /*  APPLY can only be done to layers with an alpha channel  */
  g_return_if_fail (clay_drawable_has_alpha (CLAY_DRAWABLE (layer)) ||
                    mode == CLAY_MASK_DISCARD || push_undo == TRUE);

  item  = CLAY_ITEM (layer);
  image = clay_item_get_image (item);

  if (! image)
    return;

  if (push_undo)
    {
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_APPLY_MASK,
                                   (mode == CLAY_MASK_APPLY) ?
                                   C_("undo-type", "Apply Layer Masks") :
                                   C_("undo-type", "Delete Layer Masks"));

      clay_image_undo_push_layer_mask_show (image, NULL, layer);
      clay_image_undo_push_layer_mask_apply (image, NULL, layer);
      clay_image_undo_push_layer_mask_remove (image, NULL, layer, mask);

      if (mode == CLAY_MASK_APPLY &&
          ! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
        {
          clay_layer_add_alpha (layer);
        }
    }

  /*  check if applying the mask changes the projection  */
  if (clay_layer_get_show_mask (layer)                                   ||
      (mode == CLAY_MASK_APPLY   && ! clay_layer_get_apply_mask (layer)) ||
      (mode == CLAY_MASK_DISCARD &&   clay_layer_get_apply_mask (layer)))
    {
      view_changed = TRUE;
    }

  if (mode == CLAY_MASK_APPLY)
    {
      GeglBuffer *mask_buffer;
      GeglBuffer *dest_buffer;

      if (push_undo)
        clay_drawable_push_undo (CLAY_DRAWABLE (layer), NULL,
                                 NULL,
                                 0, 0,
                                 clay_item_get_width  (item),
                                 clay_item_get_height (item));

      /* Merge any active NDE filters applied to the mask first */
      clay_drawable_merge_filters (CLAY_DRAWABLE (mask));

      /*  Combine the current layer's alpha channel and the mask  */
      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));
      dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

      clay_gegl_apply_opacity (clay_drawable_get_buffer (CLAY_DRAWABLE (layer)),
                               NULL, NULL, dest_buffer,
                               mask_buffer, 0, 0, 1.0);
    }

  g_signal_handlers_disconnect_by_func (mask,
                                        clay_layer_layer_mask_update,
                                        layer);

  clay_item_removed (CLAY_ITEM (mask));
  g_object_unref (mask);
  layer->mask = NULL;

  if (push_undo)
    clay_image_undo_group_end (image);

  if (clay_filter_peek_node (CLAY_FILTER (layer)))
    {
      GeglNode *mode_node;

      mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));

      if (layer->show_mask)
        {
          gegl_node_connect (layer->layer_offset_node, "output",
                             mode_node,                "aux");
        }
      else
        {
          gegl_node_disconnect (mode_node, "aux2");
        }

      clay_layer_update_mode_node (layer);
    }

  clay_drawable_update_bounding_box (CLAY_DRAWABLE (layer));

  clay_layer_update_effective_mode (layer);
  clay_layer_update_excludes_backdrop (layer);

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);
    }
  else
    {
      clay_viewable_invalidate_preview (CLAY_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  g_object_notify (G_OBJECT (layer), "mask");
}

void
clay_layer_set_apply_mask (ClayLayer *layer,
                           gboolean   apply,
                           gboolean   push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->apply_mask != apply)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        clay_image_undo_push_layer_mask_apply (image,
                                               apply ?
                                               C_("undo-type", "Enable Layer Masks") :
                                               C_("undo-type", "Disable Layer Masks"),
                                               layer);

      layer->apply_mask = apply ? TRUE : FALSE;

      if (clay_filter_peek_node (CLAY_FILTER (layer)) &&
          ! clay_layer_get_show_mask (layer))
        {
          GeglNode *mode_node;

          mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));

          if (layer->apply_mask)
            {
              gegl_node_connect (layer->mask_offset_node, "output",
                                 mode_node,               "aux2");
            }
          else
            {
              gegl_node_disconnect (mode_node, "aux2");
            }
        }

      clay_drawable_update_bounding_box (CLAY_DRAWABLE (layer));

      clay_layer_update_effective_mode (layer);
      clay_layer_update_excludes_backdrop (layer);

      clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[APPLY_MASK_CHANGED], 0);
    }
}

gboolean
clay_layer_get_apply_mask (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->apply_mask;
}

void
clay_layer_set_edit_mask (ClayLayer *layer,
                          gboolean   edit)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->edit_mask != edit)
    {
      layer->edit_mask = edit ? TRUE : FALSE;

      g_signal_emit (layer, layer_signals[EDIT_MASK_CHANGED], 0);
    }
}

gboolean
clay_layer_get_edit_mask (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->edit_mask;
}

void
clay_layer_set_show_mask (ClayLayer *layer,
                          gboolean   show,
                          gboolean   push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (layer->mask != NULL);

  if (layer->show_mask != show)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

      if (push_undo)
        clay_image_undo_push_layer_mask_show (image,
                                              C_("undo-type", "Show Layer Masks"),
                                              layer);

      layer->show_mask = show ? TRUE : FALSE;

      if (clay_filter_peek_node (CLAY_FILTER (layer)))
        {
          GeglNode *mode_node;

          mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));

          if (layer->show_mask)
            {
              gegl_node_disconnect (mode_node, "aux2");

              gegl_node_connect (layer->mask_offset_node, "output",
                                 mode_node,               "aux");
            }
          else
            {
              gegl_node_connect (layer->layer_offset_node, "output",
                                 mode_node,                "aux");

              if (clay_layer_get_apply_mask (layer))
                {
                  gegl_node_connect (layer->mask_offset_node, "output",
                                     mode_node,               "aux2");
                }
            }

          clay_layer_update_mode_node (layer);
        }

      clay_drawable_update_bounding_box (CLAY_DRAWABLE (layer));

      clay_layer_update_effective_mode (layer);
      clay_layer_update_excludes_backdrop (layer);

      clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);

      g_signal_emit (layer, layer_signals[SHOW_MASK_CHANGED], 0);
    }
}

gboolean
clay_layer_get_show_mask (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (layer->mask, FALSE);

  return layer->show_mask;
}

void
clay_layer_add_alpha (ClayLayer *layer)
{
  ClayItem     *item;
  ClayDrawable *drawable;
  GeglBuffer   *new_buffer;

  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    return;

  item     = CLAY_ITEM (layer);
  drawable = CLAY_DRAWABLE (layer);

  new_buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     clay_item_get_width  (item),
                                                     clay_item_get_height (item)),
                                     clay_drawable_get_format_with_alpha (drawable),
                                     CLAY_DRAWABLE (layer));

  clay_gegl_buffer_copy (
    clay_drawable_get_buffer (drawable), NULL, GEGL_ABYSS_NONE,
    new_buffer, NULL);

  clay_drawable_set_buffer (CLAY_DRAWABLE (layer),
                            clay_item_is_attached (CLAY_ITEM (layer)),
                            C_("undo-type", "Add Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
clay_layer_remove_alpha (ClayLayer   *layer,
                         ClayContext *context)
{
  GeglBuffer *new_buffer;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  if (! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    return;

  new_buffer =
    clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                          clay_item_get_width  (CLAY_ITEM (layer)),
                                          clay_item_get_height (CLAY_ITEM (layer))),
                          clay_drawable_get_format_without_alpha (CLAY_DRAWABLE (layer)),
                          CLAY_DRAWABLE (layer));

  clay_gegl_apply_flatten (clay_drawable_get_buffer (CLAY_DRAWABLE (layer)),
                           NULL, NULL, new_buffer,
                           clay_context_get_background (context),
                           clay_layer_get_real_composite_space (layer));

  clay_drawable_set_buffer (CLAY_DRAWABLE (layer),
                            clay_item_is_attached (CLAY_ITEM (layer)),
                            C_("undo-type", "Remove Alpha Channel"),
                            new_buffer);
  g_object_unref (new_buffer);
}

void
clay_layer_resize_to_image (ClayLayer    *layer,
                            ClayContext  *context,
                            ClayFillType  fill_type)
{
  ClayImage *image;
  gint       offset_x;
  gint       offset_y;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (layer)));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  image = clay_item_get_image (CLAY_ITEM (layer));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_RESIZE,
                               C_("undo-type", "Layer to Image Size"));

  clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);
  clay_item_resize (CLAY_ITEM (layer), context, fill_type,
                    clay_image_get_width  (image),
                    clay_image_get_height (image),
                    offset_x, offset_y);

  clay_image_undo_group_end (image);
}

/**********************/
/*  access functions  */
/**********************/

ClayDrawable *
clay_layer_get_floating_sel_drawable (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);

  return layer->fs.drawable;
}

void
clay_layer_set_floating_sel_drawable (ClayLayer    *layer,
                                      ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (drawable == NULL || CLAY_IS_DRAWABLE (drawable));

  if (g_set_object (&layer->fs.drawable, drawable))
    {
      if (layer->fs.segs)
        {
          g_clear_pointer (&layer->fs.segs, g_free);
          layer->fs.num_segs = 0;
        }

      g_object_notify (G_OBJECT (layer), "floating-selection");
    }
}

gboolean
clay_layer_is_floating_sel (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);

  return (clay_layer_get_floating_sel_drawable (layer) != NULL);
}

void
clay_layer_set_opacity (ClayLayer *layer,
                        gdouble    opacity,
                        gboolean   push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  opacity = CLAMP (opacity, CLAY_OPACITY_TRANSPARENT, CLAY_OPACITY_OPAQUE);

  if (layer->opacity != opacity)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_undo_push_layer_opacity (image, NULL, layer);
        }

      layer->opacity = opacity;

      g_signal_emit (layer, layer_signals[OPACITY_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "opacity");

      if (clay_filter_peek_node (CLAY_FILTER (layer)))
        clay_layer_update_mode_node (layer);

      clay_drawable_update (CLAY_DRAWABLE (layer), 0, 0, -1, -1);
    }
}

gdouble
clay_layer_get_opacity (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_OPACITY_OPAQUE);

  return layer->opacity;
}

void
clay_layer_set_mode (ClayLayer     *layer,
                     ClayLayerMode  mode,
                     gboolean       push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (clay_viewable_get_children (CLAY_VIEWABLE (layer)) == NULL)
    {
      g_return_if_fail (clay_layer_mode_get_context (mode) &
                        CLAY_LAYER_MODE_CONTEXT_LAYER);
    }
  else
    {
      g_return_if_fail (clay_layer_mode_get_context (mode) &
                        CLAY_LAYER_MODE_CONTEXT_GROUP);
    }

  if (layer->mode != mode)
    {
      if (clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_unset_default_new_layer_mode (image);

          if (push_undo)
            clay_image_undo_push_layer_mode (image, NULL, layer);
        }

      g_object_freeze_notify (G_OBJECT (layer));

      layer->mode = mode;

      g_signal_emit (layer, layer_signals[MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "mode");

      /*  when changing modes, we always switch to AUTO blend and
       *  composite in order to avoid confusion
       */
      if (layer->blend_space != CLAY_LAYER_COLOR_SPACE_AUTO)
        {
          layer->blend_space = CLAY_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "blend-space");
        }

      if (layer->composite_space != CLAY_LAYER_COLOR_SPACE_AUTO)
        {
          layer->composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-space");
        }

      if (layer->composite_mode != CLAY_LAYER_COMPOSITE_AUTO)
        {
          layer->composite_mode = CLAY_LAYER_COMPOSITE_AUTO;

          g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
          g_object_notify (G_OBJECT (layer), "composite-mode");
        }

      g_object_thaw_notify (G_OBJECT (layer));

      clay_layer_update_effective_mode (layer);
      clay_layer_update_excludes_backdrop (layer);
    }
}

ClayLayerMode
clay_layer_get_mode (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_MODE_NORMAL);

  return layer->mode;
}

void
clay_layer_set_blend_space (ClayLayer           *layer,
                            ClayLayerColorSpace  blend_space,
                            gboolean             push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (! clay_layer_mode_is_blend_space_mutable (layer->mode))
    return;

  if (layer->blend_space != blend_space)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_undo_push_layer_mode (image, _("Set layer's blend space"), layer);
        }

      layer->blend_space = blend_space;

      g_signal_emit (layer, layer_signals[BLEND_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "blend-space");

      clay_layer_update_effective_mode (layer);
    }
}

ClayLayerColorSpace
clay_layer_get_blend_space (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COLOR_SPACE_AUTO);

  return layer->blend_space;
}

ClayLayerColorSpace
clay_layer_get_real_blend_space (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->blend_space == CLAY_LAYER_COLOR_SPACE_AUTO)
    return clay_layer_mode_get_blend_space (layer->mode);
  else
    return layer->blend_space;
}

void
clay_layer_set_composite_space (ClayLayer           *layer,
                                ClayLayerColorSpace  composite_space,
                                gboolean             push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (! clay_layer_mode_is_composite_space_mutable (layer->mode))
    return;

  if (layer->composite_space != composite_space)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_undo_push_layer_mode (image, _("Set layer's composite space"), layer);
        }

      layer->composite_space = composite_space;

      g_signal_emit (layer, layer_signals[COMPOSITE_SPACE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-space");

      clay_layer_update_effective_mode (layer);
    }
}

ClayLayerColorSpace
clay_layer_get_composite_space (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COLOR_SPACE_AUTO);

  return layer->composite_space;
}

ClayLayerColorSpace
clay_layer_get_real_composite_space (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COLOR_SPACE_RGB_LINEAR);

  if (layer->composite_space == CLAY_LAYER_COLOR_SPACE_AUTO)
    return clay_layer_mode_get_composite_space (layer->mode);
  else
    return layer->composite_space;
}

void
clay_layer_set_composite_mode (ClayLayer              *layer,
                               ClayLayerCompositeMode  composite_mode,
                               gboolean                push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (! clay_layer_mode_is_composite_mode_mutable (layer->mode))
    return;

  if (layer->composite_mode != composite_mode)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_undo_push_layer_mode (image, _("Set layer's composite mode"), layer);
        }

      layer->composite_mode = composite_mode;

      g_signal_emit (layer, layer_signals[COMPOSITE_MODE_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "composite-mode");

      clay_layer_update_effective_mode (layer);
      clay_layer_update_excludes_backdrop (layer);
    }
}

ClayLayerCompositeMode
clay_layer_get_composite_mode (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COMPOSITE_AUTO);

  return layer->composite_mode;
}

ClayLayerCompositeMode
clay_layer_get_real_composite_mode (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), CLAY_LAYER_COMPOSITE_UNION);

  if (layer->composite_mode == CLAY_LAYER_COMPOSITE_AUTO)
    return clay_layer_mode_get_composite_mode (layer->mode);
  else
    return layer->composite_mode;
}

void
clay_layer_get_effective_mode (ClayLayer              *layer,
                               ClayLayerMode          *mode,
                               ClayLayerColorSpace    *blend_space,
                               ClayLayerColorSpace    *composite_space,
                               ClayLayerCompositeMode *composite_mode)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (mode)            *mode            = layer->effective_mode;
  if (blend_space)     *blend_space     = layer->effective_blend_space;
  if (composite_space) *composite_space = layer->effective_composite_space;
  if (composite_mode)  *composite_mode  = layer->effective_composite_mode;
}

gboolean
clay_layer_get_excludes_backdrop (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);

  return layer->excludes_backdrop;
}

void
clay_layer_set_lock_alpha (ClayLayer *layer,
                           gboolean   lock_alpha,
                           gboolean   push_undo)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_layer_can_lock_alpha (layer));

  lock_alpha = lock_alpha ? TRUE : FALSE;

  if (layer->lock_alpha != lock_alpha)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (layer)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

          clay_image_undo_push_layer_lock_alpha (image, NULL, layer);
        }

      layer->lock_alpha = lock_alpha;

      g_signal_emit (layer, layer_signals[LOCK_ALPHA_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "lock-alpha");
    }
}

gboolean
clay_layer_get_lock_alpha (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);

  return layer->lock_alpha;
}

gboolean
clay_layer_can_lock_alpha (ClayLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);

  return TRUE;
}

gboolean
clay_layer_is_alpha_locked (ClayLayer  *layer,
                            ClayLayer **locked_layer)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);

  return CLAY_LAYER_GET_CLASS (layer)->is_alpha_locked (layer, locked_layer);
}

/*  protected functions  */

void
clay_layer_update_effective_mode (ClayLayer *layer)
{
  ClayLayerMode          mode;
  ClayLayerColorSpace    blend_space;
  ClayLayerColorSpace    composite_space;
  ClayLayerCompositeMode composite_mode;

  g_return_if_fail (CLAY_IS_LAYER (layer));

  if (layer->mask && layer->show_mask)
    {
      mode            = CLAY_LAYER_MODE_NORMAL;
      blend_space     = CLAY_LAYER_COLOR_SPACE_AUTO;
      composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
      composite_mode  = CLAY_LAYER_COMPOSITE_AUTO;

      /* This makes sure that masks of LEGACY-mode layers are
       * composited in PERCEPTUAL space, and non-LEGACY layers in
       * LINEAR space, or whatever composite space was chosen in the
       * layer attributes dialog
       */
      composite_space = clay_layer_get_real_composite_space (layer);
    }
  else
    {
      CLAY_LAYER_GET_CLASS (layer)->get_effective_mode (layer,
                                                        &mode,
                                                        &blend_space,
                                                        &composite_space,
                                                        &composite_mode);
    }

  if (mode            != layer->effective_mode            ||
      blend_space     != layer->effective_blend_space     ||
      composite_space != layer->effective_composite_space ||
      composite_mode  != layer->effective_composite_mode)
    {
      GeglRectangle rect = CLAY_LAYER_GET_CLASS (layer)->get_bounding_box (layer);
      GeglRectangle rect2;

      layer->effective_mode            = mode;
      layer->effective_blend_space     = blend_space;
      layer->effective_composite_space = composite_space;
      layer->effective_composite_mode  = composite_mode;

      g_signal_emit (layer, layer_signals[EFFECTIVE_MODE_CHANGED], 0);

      if (clay_filter_peek_node (CLAY_FILTER (layer)))
        clay_layer_update_mode_node (layer);

      /* The effective mode change might be enough to shrink the
       * effective bounding box. This may happen in particular for
       * pass-through group layers changed to other modes. So make sure
       * we update a box containing both the old and new boundaries.
       */
      rect2 = CLAY_LAYER_GET_CLASS (layer)->get_bounding_box (layer);
      gegl_rectangle_bounding_box (&rect, &rect, &rect2);

      clay_drawable_update (CLAY_DRAWABLE (layer), rect.x, rect.y, rect.width, rect.height);
    }
}

void
clay_layer_update_excludes_backdrop (ClayLayer *layer)
{
  gboolean excludes_backdrop;

  g_return_if_fail (CLAY_IS_LAYER (layer));

  excludes_backdrop =
    CLAY_LAYER_GET_CLASS (layer)->get_excludes_backdrop (layer);

  if (excludes_backdrop != layer->excludes_backdrop)
    {
      layer->excludes_backdrop = excludes_backdrop;

      g_signal_emit (layer, layer_signals[EXCLUDES_BACKDROP_CHANGED], 0);
      g_object_notify (G_OBJECT (layer), "excludes-backdrop");
    }
}
