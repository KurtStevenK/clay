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
#include <time.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <glib/gstdio.h>

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#ifdef G_OS_WIN32
#include <libclaybase/claywin32-io.h>
#endif
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "clay.h"
#include "clay-memsize.h"
#include "clay-parasites.h"
#include "clay-utils.h"
#include "claycontext.h"
#include "claydrawable-filters.h"
#include "claydrawable-floating-selection.h"
#include "claydrawablefilter.h"
#include "claydrawablestack.h"
#include "claygrid.h"
#include "clayerror.h"
#include "clayguide.h"
#include "clayidtable.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "clayimage-guides.h"
#include "clayimage-grid.h"
#include "clayimage-item-list.h"
#include "clayimage-metadata.h"
#include "clayimage-sample-points.h"
#include "clayimage-preview.h"
#include "clayimage-private.h"
#include "clayimage-quick-mask.h"
#include "clayimage-savable.h"
#include "clayimage-symmetry.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitemlist.h"
#include "clayitemtree.h"
#include "claylayer.h"
#include "claylayer-floating-selection.h"
#include "claylayermask.h"
#include "claylayerstack.h"
#include "claylinklayer.h"
#include "claymarshal.h"
#include "claypalette.h"
#include "clayparasitelist.h"
#include "claypickable.h"
#include "clayprojectable.h"
#include "clayprojection.h"
#include "claysamplepoint.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "clayselection.h"
#include "claysymmetry.h"
#include "claytempbuf.h"
#include "claytemplate.h"
#include "clayundostack.h"

#include "text/claytext.h"
#include "text/claytextlayer.h"

#include "path/claypath.h"
#include "path/clayvectorlayer.h"

#include "clay-log.h"
#include "clay-intl.h"


#ifdef DEBUG
#define TRC(x) g_printerr x
#else
#define TRC(x)
#endif


enum
{
  MODE_CHANGED,
  PRECISION_CHANGED,
  ALPHA_CHANGED,
  FLOATING_SELECTION_CHANGED,
  SELECTED_CHANNELS_CHANGED,
  SELECTED_PATHS_CHANGED,
  SELECTED_LAYERS_CHANGED,
  SELECTED_DRAWABLES_CHANGED,
  COMPONENT_VISIBILITY_CHANGED,
  COMPONENT_ACTIVE_CHANGED,
  MASK_CHANGED,
  RESOLUTION_CHANGED,
  SIZE_CHANGED_DETAILED,
  UNIT_CHANGED,
  QUICK_MASK_CHANGED,
  SELECTION_INVALIDATE,
  CLEAN,
  DIRTY,
  SAVING,
  SAVED,
  EXPORTED,
  GUIDE_ADDED,
  GUIDE_REMOVED,
  GUIDE_MOVED,
  SAMPLE_POINT_ADDED,
  SAMPLE_POINT_REMOVED,
  SAMPLE_POINT_MOVED,
  PARASITE_ATTACHED,
  PARASITE_DETACHED,
  COLORMAP_CHANGED,
  UNDO_EVENT,
  ITEM_SETS_CHANGED,
  DISPLAY_COUNT_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CLAY,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_BASE_TYPE,
  PROP_PRECISION,
  PROP_METADATA,
  PROP_BUFFER,
  PROP_SYMMETRY,
  PROP_CONVERTING,
};


/*  local function prototypes  */

static void     clay_color_managed_iface_init    (ClayColorManagedInterface *iface);
static void     clay_projectable_iface_init      (ClayProjectableInterface  *iface);
static void     clay_pickable_iface_init         (ClayPickableInterface     *iface);
static void     clay_savable_iface_init          (ClaySavableInterface      *iface);

static void     clay_image_constructed           (GObject           *object);
static void     clay_image_set_property          (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void     clay_image_get_property          (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void     clay_image_dispose               (GObject           *object);
static void     clay_image_finalize              (GObject           *object);

static void     clay_image_name_changed          (ClayObject        *object);
static gint64   clay_image_get_memsize           (ClayObject        *object,
                                                  gint64            *gui_size);

static gboolean clay_image_get_size              (ClayViewable      *viewable,
                                                  gint              *width,
                                                  gint              *height);
static void     clay_image_size_changed          (ClayViewable      *viewable);
static gchar  * clay_image_get_description       (ClayViewable      *viewable,
                                                  gchar            **tooltip);

static void     clay_image_real_mode_changed     (ClayImage         *image);
static void     clay_image_real_precision_changed(ClayImage         *image);
static void     clay_image_real_resolution_changed(ClayImage        *image);
static void     clay_image_real_size_changed_detailed
                                                 (ClayImage         *image,
                                                  gint               previous_origin_x,
                                                  gint               previous_origin_y,
                                                  gint               previous_width,
                                                  gint               previous_height);
static void     clay_image_real_unit_changed     (ClayImage         *image);
static void     clay_image_real_colormap_changed (ClayImage         *image,
                                                  gint               color_index);

static const guint8 *
        clay_image_color_managed_get_icc_profile (ClayColorManaged  *managed,
                                                  gsize             *len);
static ClayColorProfile *
      clay_image_color_managed_get_color_profile (ClayColorManaged  *managed);
static void
        clay_image_color_managed_profile_changed (ClayColorManaged  *managed);

static ClayColorProfile *
      clay_image_color_managed_get_simulation_profile     (ClayColorManaged  *managed);
static void
      clay_image_color_managed_simulation_profile_changed (ClayColorManaged  *managed);

static ClayColorRenderingIntent
      clay_image_color_managed_get_simulation_intent      (ClayColorManaged  *managed);
static void
      clay_image_color_managed_simulation_intent_changed  (ClayColorManaged  *managed);

static gboolean
      clay_image_color_managed_get_simulation_bpc         (ClayColorManaged  *managed);
static void
      clay_image_color_managed_simulation_bpc_changed     (ClayColorManaged  *managed);

static void        clay_image_projectable_flush  (ClayProjectable   *projectable,
                                                  gboolean           invalidate_preview);
static GeglRectangle clay_image_get_bounding_box (ClayProjectable   *projectable);
static GeglNode   * clay_image_get_graph         (ClayProjectable   *projectable);
static ClayImage  * clay_image_get_image         (ClayProjectable   *projectable);
static const Babl * clay_image_get_proj_format   (ClayProjectable   *projectable);

static void         clay_image_pickable_flush    (ClayPickable      *pickable);
static GeglBuffer * clay_image_get_buffer        (ClayPickable      *pickable);
static gboolean     clay_image_get_pixel_at      (ClayPickable      *pickable,
                                                  gint               x,
                                                  gint               y,
                                                  const Babl        *format,
                                                  gpointer           pixel);
static gdouble      clay_image_get_opacity_at    (ClayPickable      *pickable,
                                                  gint               x,
                                                  gint               y);
static void         clay_image_get_pixel_average (ClayPickable      *pickable,
                                                  const GeglRectangle *rect,
                                                  const Babl        *format,
                                                  gpointer           pixel);

static void     clay_image_projection_buffer_notify
                                                 (ClayProjection    *projection,
                                                  const GParamSpec  *pspec,
                                                  ClayImage         *image);
static void     clay_image_mask_update           (ClayDrawable      *drawable,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height,
                                                  ClayImage         *image);
static void     clay_image_layers_changed        (ClayContainer     *container,
                                                  ClayChannel       *channel,
                                                  ClayImage         *image);
static void     clay_image_layer_offset_changed  (ClayDrawable      *drawable,
                                                  const GParamSpec  *pspec,
                                                  ClayImage         *image);
static void     clay_image_layer_bounding_box_changed
                                                 (ClayDrawable      *drawable,
                                                  ClayImage         *image);
static void     clay_image_layer_alpha_changed   (ClayDrawable      *drawable,
                                                  ClayImage         *image);
static void     clay_image_channel_add           (ClayContainer     *container,
                                                  ClayChannel       *channel,
                                                  ClayImage         *image);
static void     clay_image_channel_remove        (ClayContainer     *container,
                                                  ClayChannel       *channel,
                                                  ClayImage         *image);
static void     clay_image_channel_name_changed  (ClayChannel       *channel,
                                                  ClayImage         *image);
static void     clay_image_channel_color_changed (ClayChannel       *channel,
                                                  ClayImage         *image);

static void     clay_image_selected_layers_notify   (ClayItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     ClayImage         *image);
static void     clay_image_selected_channels_notify (ClayItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     ClayImage         *image);
static void     clay_image_selected_paths_notify    (ClayItemTree      *tree,
                                                     const GParamSpec  *pspec,
                                                     ClayImage         *image);

static void     clay_image_freeze_bounding_box   (ClayImage         *image);
static void     clay_image_thaw_bounding_box     (ClayImage         *image);
static void     clay_image_update_bounding_box   (ClayImage         *image);

static gint     clay_image_layer_stack_cmp         (GList           *layers1,
                                                    GList           *layers2);
static void clay_image_rec_remove_layer_stack_dups (ClayImage       *image,
                                                    GSList          *start);
static void     clay_image_clean_layer_stack       (ClayImage       *image);
static void     clay_image_rec_filter_remove_undo  (ClayImage       *image,
                                                    ClayDrawable    *drawable);
static void     clay_image_remove_from_layer_stack (ClayImage       *image,
                                                    ClayLayer       *layer);
static gint     clay_image_selected_is_descendant  (ClayViewable    *selected,
                                                    ClayViewable    *viewable);


G_DEFINE_TYPE_WITH_CODE (ClayImage, clay_image, CLAY_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (ClayImage)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                clay_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROJECTABLE,
                                                clay_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_savable_iface_init))

#define parent_class clay_image_parent_class

static guint clay_image_signals[LAST_SIGNAL] = { 0 };


static void
clay_image_class_init (ClayImageClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);

  clay_image_signals[MODE_CHANGED] =
    g_signal_new ("mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[PRECISION_CHANGED] =
    g_signal_new ("precision-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, precision_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[FLOATING_SELECTION_CHANGED] =
    g_signal_new ("floating-selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, floating_selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SELECTED_PATHS_CHANGED] =
    g_signal_new ("selected-paths-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, selected_paths_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SELECTED_CHANNELS_CHANGED] =
    g_signal_new ("selected-channels-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, selected_channels_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SELECTED_LAYERS_CHANGED] =
    g_signal_new ("selected-layers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, selected_layers_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SELECTED_DRAWABLES_CHANGED] =
    g_signal_new ("selected-drawables-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, selected_drawables_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[COMPONENT_VISIBILITY_CHANGED] =
    g_signal_new ("component-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, component_visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_CHANNEL_TYPE);

  clay_image_signals[COMPONENT_ACTIVE_CHANGED] =
    g_signal_new ("component-active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, component_active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_CHANNEL_TYPE);

  clay_image_signals[MASK_CHANGED] =
    g_signal_new ("mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[RESOLUTION_CHANGED] =
    g_signal_new ("resolution-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, resolution_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SIZE_CHANGED_DETAILED] =
    g_signal_new ("size-changed-detailed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, size_changed_detailed),
                  NULL, NULL,
                  clay_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  clay_image_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, unit_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[QUICK_MASK_CHANGED] =
    g_signal_new ("quick-mask-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, quick_mask_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SELECTION_INVALIDATE] =
    g_signal_new ("selection-invalidate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, selection_invalidate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[CLEAN] =
    g_signal_new ("clean",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, clean),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DIRTY_MASK);

  clay_image_signals[DIRTY] =
    g_signal_new ("dirty",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, dirty),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DIRTY_MASK);

  clay_image_signals[SAVING] =
    g_signal_new ("saving",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, saving),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_image_signals[SAVED] =
    g_signal_new ("saved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, saved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  clay_image_signals[EXPORTED] =
    g_signal_new ("exported",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, exported),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_FILE);

  clay_image_signals[GUIDE_ADDED] =
    g_signal_new ("guide-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, guide_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_GUIDE);

  clay_image_signals[GUIDE_REMOVED] =
    g_signal_new ("guide-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, guide_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_GUIDE);

  clay_image_signals[GUIDE_MOVED] =
    g_signal_new ("guide-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, guide_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_GUIDE);

  clay_image_signals[SAMPLE_POINT_ADDED] =
    g_signal_new ("sample-point-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, sample_point_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_SAMPLE_POINT);

  clay_image_signals[SAMPLE_POINT_REMOVED] =
    g_signal_new ("sample-point-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, sample_point_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_SAMPLE_POINT);

  clay_image_signals[SAMPLE_POINT_MOVED] =
    g_signal_new ("sample-point-moved",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, sample_point_moved),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_SAMPLE_POINT);

  clay_image_signals[PARASITE_ATTACHED] =
    g_signal_new ("parasite-attached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, parasite_attached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  clay_image_signals[PARASITE_DETACHED] =
    g_signal_new ("parasite-detached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, parasite_detached),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  clay_image_signals[COLORMAP_CHANGED] =
    g_signal_new ("colormap-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, colormap_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  clay_image_signals[UNDO_EVENT] =
    g_signal_new ("undo-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, undo_event),
                  NULL, NULL,
                  clay_marshal_VOID__ENUM_OBJECT,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_UNDO_EVENT,
                  CLAY_TYPE_UNDO);

  clay_image_signals[ITEM_SETS_CHANGED] =
    g_signal_new ("item-sets-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, item_sets_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_GTYPE);

  clay_image_signals[DISPLAY_COUNT_CHANGED] =
    g_signal_new ("display-count-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageClass, display_count_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  object_class->constructed           = clay_image_constructed;
  object_class->set_property          = clay_image_set_property;
  object_class->get_property          = clay_image_get_property;
  object_class->dispose               = clay_image_dispose;
  object_class->finalize              = clay_image_finalize;

  clay_object_class->name_changed     = clay_image_name_changed;
  clay_object_class->get_memsize      = clay_image_get_memsize;

  viewable_class->default_icon_name   = "clay-image";
  viewable_class->default_name        = _("Image");
  viewable_class->get_size            = clay_image_get_size;
  viewable_class->size_changed        = clay_image_size_changed;
  viewable_class->get_preview_size    = clay_image_get_preview_size;
  viewable_class->get_popup_size      = clay_image_get_popup_size;
  viewable_class->get_new_preview     = clay_image_get_new_preview;
  viewable_class->get_new_pixbuf      = clay_image_get_new_pixbuf;
  viewable_class->get_description     = clay_image_get_description;

  klass->mode_changed                 = clay_image_real_mode_changed;
  klass->precision_changed            = clay_image_real_precision_changed;
  klass->alpha_changed                = NULL;
  klass->floating_selection_changed   = NULL;
  klass->selected_layers_changed      = NULL;
  klass->selected_channels_changed    = NULL;
  klass->selected_paths_changed       = NULL;
  klass->component_visibility_changed = NULL;
  klass->component_active_changed     = NULL;
  klass->mask_changed                 = NULL;
  klass->resolution_changed           = clay_image_real_resolution_changed;
  klass->size_changed_detailed        = clay_image_real_size_changed_detailed;
  klass->unit_changed                 = clay_image_real_unit_changed;
  klass->quick_mask_changed           = NULL;
  klass->selection_invalidate         = NULL;

  klass->clean                        = NULL;
  klass->dirty                        = NULL;
  klass->saving                       = NULL;
  klass->saved                        = NULL;
  klass->exported                     = NULL;
  klass->guide_added                  = NULL;
  klass->guide_removed                = NULL;
  klass->guide_moved                  = NULL;
  klass->sample_point_added           = NULL;
  klass->sample_point_removed         = NULL;
  klass->sample_point_moved           = NULL;
  klass->parasite_attached            = NULL;
  klass->parasite_detached            = NULL;
  klass->colormap_changed             = clay_image_real_colormap_changed;
  klass->undo_event                   = NULL;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, CLAY_MAX_IMAGE_SIZE, 1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, CLAY_MAX_IMAGE_SIZE, 1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BASE_TYPE,
                                   g_param_spec_enum ("base-type", NULL, NULL,
                                                      CLAY_TYPE_IMAGE_BASE_TYPE,
                                                      CLAY_RGB,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PRECISION,
                                   g_param_spec_enum ("precision", NULL, NULL,
                                                      CLAY_TYPE_PRECISION,
                                                      CLAY_PRECISION_U8_NON_LINEAR,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_METADATA,
                                   g_param_spec_object ("metadata", NULL, NULL,
                                                        GEXIV2_TYPE_METADATA,
                                                        CLAY_PARAM_READABLE));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");

  g_object_class_install_property (object_class, PROP_SYMMETRY,
                                   g_param_spec_gtype ("symmetry",
                                                       NULL, _("Symmetry"),
                                                       CLAY_TYPE_SYMMETRY,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONVERTING,
                                   g_param_spec_boolean ("converting",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_icc_profile            = clay_image_color_managed_get_icc_profile;
  iface->get_color_profile          = clay_image_color_managed_get_color_profile;
  iface->profile_changed            = clay_image_color_managed_profile_changed;
  iface->get_simulation_profile     = clay_image_color_managed_get_simulation_profile;
  iface->simulation_profile_changed = clay_image_color_managed_simulation_profile_changed;
  iface->get_simulation_intent      = clay_image_color_managed_get_simulation_intent;
  iface->simulation_intent_changed  = clay_image_color_managed_simulation_intent_changed;
  iface->get_simulation_bpc         = clay_image_color_managed_get_simulation_bpc;
  iface->simulation_bpc_changed     = clay_image_color_managed_simulation_bpc_changed;
}

static void
clay_projectable_iface_init (ClayProjectableInterface *iface)
{
  iface->flush              = clay_image_projectable_flush;
  iface->get_image          = clay_image_get_image;
  iface->get_format         = clay_image_get_proj_format;
  iface->get_bounding_box   = clay_image_get_bounding_box;
  iface->get_graph          = clay_image_get_graph;
  iface->invalidate_preview = (void (*) (ClayProjectable*)) clay_viewable_invalidate_preview;
}

static void
clay_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->flush                 = clay_image_pickable_flush;
  iface->get_image             = (ClayImage  * (*) (ClayPickable *pickable)) clay_image_get_image;
  iface->get_format            = (const Babl * (*) (ClayPickable *pickable)) clay_image_get_proj_format;
  iface->get_format_with_alpha = (const Babl * (*) (ClayPickable *pickable)) clay_image_get_proj_format;
  iface->get_buffer            = clay_image_get_buffer;
  iface->get_pixel_at          = clay_image_get_pixel_at;
  iface->get_opacity_at        = clay_image_get_opacity_at;
  iface->get_pixel_average     = clay_image_get_pixel_average;
}

static void
clay_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "xcf";
  iface->save       = clay_image_savable_save;
  iface->load_enter = clay_image_load_enter;
  iface->load_exit  = NULL;
}

static void
clay_image_init (ClayImage *image)
{
  ClayImagePrivate *private = clay_image_get_instance_private (image);
  gint              i;

  image->priv = private;

  private->ID                  = -1;

  private->load_proc           = NULL;
  private->save_proc           = NULL;

  private->width               = 0;
  private->height              = 0;
  private->xresolution         = 1.0;
  private->yresolution         = 1.0;
  private->resolution_set      = FALSE;
  private->resolution_unit     = clay_unit_inch ();
  private->base_type           = CLAY_RGB;
  private->precision           = CLAY_PRECISION_U8_NON_LINEAR;
  private->new_layer_mode      = -1;

  private->show_all            = 0;
  private->bounding_box.x      = 0;
  private->bounding_box.y      = 0;
  private->bounding_box.width  = 0;
  private->bounding_box.height = 0;
  private->pickable_buffer     = NULL;

  private->palette             = NULL;

  private->metadata            = NULL;

  private->simulation_profile  = NULL;
  private->simulation_intent   = CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  private->simulation_bpc      = FALSE;

  private->cache_xml           = NULL;
  private->cache_folder        = NULL;
  private->buffers_folder      = NULL;

  private->dirty               = 1;
  private->dirty_time          = 0;
  private->undo_freeze_count   = 0;

  private->export_dirty        = 1;

  private->instance_count      = 0;
  private->disp_count          = 0;

  private->tattoo_state        = 0;

  private->projection          = clay_projection_new (CLAY_PROJECTABLE (image));

  private->symmetries          = NULL;
  private->active_symmetry     = NULL;

  private->guides              = NULL;
  private->grid                = NULL;
  private->sample_points       = NULL;

  private->layers              = clay_item_tree_new (image,
                                                     CLAY_TYPE_LAYER_STACK,
                                                     CLAY_TYPE_LAYER);
  private->channels            = clay_item_tree_new (image,
                                                     CLAY_TYPE_DRAWABLE_STACK,
                                                     CLAY_TYPE_CHANNEL);
  private->paths               = clay_item_tree_new (image,
                                                     CLAY_TYPE_ITEM_STACK,
                                                     CLAY_TYPE_PATH);
  private->layer_stack         = NULL;

  private->stored_layer_sets   = NULL;
  private->stored_channel_sets = NULL;
  private->stored_path_sets    = NULL;

  g_signal_connect (private->projection, "notify::buffer",
                    G_CALLBACK (clay_image_projection_buffer_notify),
                    image);

  g_signal_connect (private->layers, "notify::selected-items",
                    G_CALLBACK (clay_image_selected_layers_notify),
                    image);
  g_signal_connect (private->channels, "notify::selected-items",
                    G_CALLBACK (clay_image_selected_channels_notify),
                    image);
  g_signal_connect (private->paths, "notify::selected-items",
                    G_CALLBACK (clay_image_selected_paths_notify),
                    image);

  g_signal_connect_swapped (private->layers->container, "update",
                            G_CALLBACK (clay_image_invalidate),
                            image);

  private->layer_offset_x_handler =
    clay_container_add_handler (private->layers->container, "notify::offset-x",
                                G_CALLBACK (clay_image_layer_offset_changed),
                                image);
  private->layer_offset_y_handler =
    clay_container_add_handler (private->layers->container, "notify::offset-y",
                                G_CALLBACK (clay_image_layer_offset_changed),
                                image);
  private->layer_bounding_box_handler =
    clay_container_add_handler (private->layers->container, "bounding-box-changed",
                                G_CALLBACK (clay_image_layer_bounding_box_changed),
                                image);
  private->layer_alpha_handler =
    clay_container_add_handler (private->layers->container, "alpha-changed",
                                G_CALLBACK (clay_image_layer_alpha_changed),
                                image);

  g_signal_connect (private->layers->container, "add",
                    G_CALLBACK (clay_image_layers_changed),
                    image);
  g_signal_connect (private->layers->container, "remove",
                    G_CALLBACK (clay_image_layers_changed),
                    image);

  g_signal_connect_swapped (private->channels->container, "update",
                            G_CALLBACK (clay_image_invalidate),
                            image);

  private->channel_name_changed_handler =
    clay_container_add_handler (private->channels->container, "name-changed",
                                G_CALLBACK (clay_image_channel_name_changed),
                                image);
  private->channel_color_changed_handler =
    clay_container_add_handler (private->channels->container, "color-changed",
                                G_CALLBACK (clay_image_channel_color_changed),
                                image);

  g_signal_connect (private->channels->container, "add",
                    G_CALLBACK (clay_image_channel_add),
                    image);
  g_signal_connect (private->channels->container, "remove",
                    G_CALLBACK (clay_image_channel_remove),
                    image);

  private->floating_sel        = NULL;
  private->selection_mask      = NULL;

  private->parasites           = clay_parasite_list_new ();

  for (i = 0; i < MAX_CHANNELS; i++)
    {
      private->visible[i] = TRUE;
      private->active[i]  = TRUE;
    }

  private->quick_mask_state    = FALSE;
  private->quick_mask_inverted = FALSE;
  /* Quick mask color set to sRGB red with half opacity by default. */
  private->quick_mask_color    = gegl_color_new ("red");
  clay_color_set_alpha (private->quick_mask_color, 0.5);

  private->undo_stack          = clay_undo_stack_new (image);
  private->redo_stack          = clay_undo_stack_new (image);
  private->group_count         = 0;
  private->pushing_undo_group  = CLAY_UNDO_GROUP_NONE;

  private->flush_accum.alpha_changed              = FALSE;
  private->flush_accum.mask_changed               = FALSE;
  private->flush_accum.floating_selection_changed = FALSE;
  private->flush_accum.preview_invalidated        = FALSE;

  private->from_command_line                      = FALSE;
}

static void
clay_image_constructed (GObject *object)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  ClayChannel      *selection;
  ClayCoreConfig   *config;
  ClayTemplate     *template;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (image->clay));

  config = image->clay->config;

  if (private->ID < 0)
    private->ID = clay_id_table_insert (image->clay->image_table, image);
  else
    clay_id_table_insert_reserved (image->clay->image_table,
                                   private->ID, image);

  template = config->default_image;

  private->xresolution     = clay_template_get_resolution_x (template);
  private->yresolution     = clay_template_get_resolution_y (template);
  private->resolution_unit = clay_template_get_resolution_unit (template);

  private->grid = clay_config_duplicate (CLAY_CONFIG (config->default_grid));

  g_clear_object (&private->quick_mask_color);
  private->quick_mask_color = gegl_color_duplicate (config->quick_mask_color);

  clay_image_update_bounding_box (image);

  if (private->base_type == CLAY_INDEXED)
    clay_image_colormap_init (image);

  selection = clay_selection_new (image,
                                  clay_image_get_width  (image),
                                  clay_image_get_height (image));
  clay_image_take_mask (image, selection);

  g_signal_connect_object (config, "notify::transparency-type",
                           G_CALLBACK (clay_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-size",
                           G_CALLBACK (clay_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color1",
                           G_CALLBACK (clay_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::transparency-custom-color2",
                           G_CALLBACK (clay_item_stack_invalidate_previews),
                           private->layers->container, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::layer-previews",
                           G_CALLBACK (clay_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);
  g_signal_connect_object (config, "notify::group-layer-previews",
                           G_CALLBACK (clay_viewable_size_changed),
                           image, G_CONNECT_SWAPPED);

  clay_container_add (image->clay->images, CLAY_OBJECT (image));

  (void) clay_image_get_buffers_folder (image);
  (void) clay_image_get_cache_xml_file (image);
}

static void
clay_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_CLAY:
      image->clay = g_value_get_object (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;

    case PROP_BASE_TYPE:
      private->base_type = g_value_get_enum (value);
      _clay_image_free_color_transforms (image);
      break;

    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      _clay_image_free_color_transforms (image);
      break;

    case PROP_SYMMETRY:
      {
        GList *iter;
        GType  type = g_value_get_gtype (value);

        if (private->active_symmetry)
          g_object_set (private->active_symmetry,
                        "active", FALSE,
                        NULL);
        private->active_symmetry = NULL;

        for (iter = private->symmetries; iter; iter = g_list_next (iter))
          {
            ClaySymmetry *sym = iter->data;

            if (type == G_TYPE_FROM_INSTANCE (sym))
              private->active_symmetry = iter->data;
          }

        if (! private->active_symmetry &&
            g_type_is_a (type, CLAY_TYPE_SYMMETRY))
          {
            ClaySymmetry *sym = clay_image_symmetry_new (image, type);

            clay_image_symmetry_add (image, sym);
            g_object_unref (sym);

            private->active_symmetry = sym;
          }

        if (private->active_symmetry)
          g_object_set (private->active_symmetry,
                        "active", TRUE,
                        NULL);
      }
      break;

    case PROP_CONVERTING:
      private->converting = g_value_get_boolean (value);
      break;

    case PROP_ID:
      private->ID = g_value_get_int (value);
      break;

    case PROP_METADATA:
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, image->clay);
      break;
    case PROP_ID:
      g_value_set_int (value, private->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;
    case PROP_BASE_TYPE:
      g_value_set_enum (value, private->base_type);
      break;
    case PROP_PRECISION:
      g_value_set_enum (value, private->precision);
      break;
    case PROP_METADATA:
      g_value_set_object (value, clay_image_get_metadata (image));
      break;
    case PROP_BUFFER:
      g_value_set_object (value, clay_image_get_buffer (CLAY_PICKABLE (image)));
      break;
    case PROP_SYMMETRY:
      g_value_set_gtype (value,
                         private->active_symmetry ?
                         G_TYPE_FROM_INSTANCE (private->active_symmetry) :
                         G_TYPE_NONE);
      break;
    case PROP_CONVERTING:
      g_value_set_boolean (value, private->converting);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_dispose (GObject *object)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->palette)
    clay_image_colormap_dispose (image);

  clay_image_undo_free (image);

  g_list_free_full (private->stored_layer_sets,   g_object_unref);
  g_list_free_full (private->stored_channel_sets, g_object_unref);
  g_list_free_full (private->stored_path_sets,    g_object_unref);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        clay_image_invalidate,
                                        image);

  clay_container_remove_handler (private->layers->container,
                                 private->layer_offset_x_handler);
  clay_container_remove_handler (private->layers->container,
                                 private->layer_offset_y_handler);
  clay_container_remove_handler (private->layers->container,
                                 private->layer_bounding_box_handler);
  clay_container_remove_handler (private->layers->container,
                                 private->layer_alpha_handler);

  g_signal_handlers_disconnect_by_func (private->layers->container,
                                        clay_image_layers_changed,
                                        image);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        clay_image_invalidate,
                                        image);

  clay_container_remove_handler (private->channels->container,
                                 private->channel_name_changed_handler);
  clay_container_remove_handler (private->channels->container,
                                 private->channel_color_changed_handler);

  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        clay_image_channel_add,
                                        image);
  g_signal_handlers_disconnect_by_func (private->channels->container,
                                        clay_image_channel_remove,
                                        image);

  g_object_run_dispose (G_OBJECT (private->layers));
  g_object_run_dispose (G_OBJECT (private->channels));
  g_object_run_dispose (G_OBJECT (private->paths));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_image_finalize (GObject *object)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->projection);
  g_clear_object (&private->graph);
  private->visible_mask = NULL;

  if (private->palette)
    clay_image_colormap_free (image);

  _clay_image_free_color_profile (image);

  g_clear_object (&private->pickable_buffer);
  g_clear_object (&private->metadata);
  g_clear_object (&private->file);
  g_clear_object (&private->imported_file);
  g_clear_object (&private->exported_file);
  g_clear_object (&private->save_a_copy_file);
  g_clear_object (&private->untitled_file);
  g_clear_object (&private->layers);
  g_clear_object (&private->channels);
  g_clear_object (&private->paths);
  g_clear_object (&private->quick_mask_color);
  g_clear_pointer (&private->quick_mask_selected, g_list_free);

  if (private->layer_stack)
    {
      g_slist_free_full (private->layer_stack,
                         (GDestroyNotify) g_list_free);
      private->layer_stack = NULL;
    }

  g_clear_object (&private->selection_mask);
  g_clear_object (&private->parasites);

  if (private->guides)
    {
      g_list_free_full (private->guides, (GDestroyNotify) g_object_unref);
      private->guides = NULL;
    }

  if (private->symmetries)
    {
      g_list_free_full (private->symmetries, g_object_unref);
      private->symmetries = NULL;
    }

  g_clear_object (&private->grid);

  if (private->sample_points)
    {
      g_list_free_full (private->sample_points,
                        (GDestroyNotify) g_object_unref);
      private->sample_points = NULL;
    }

  g_clear_object (&private->undo_stack);
  g_clear_object (&private->redo_stack);

  if (image->clay && image->clay->image_table)
    {
      clay_id_table_remove (image->clay->image_table, private->ID);
      image->clay = NULL;
    }

  g_clear_pointer (&private->display_name, g_free);
  g_clear_pointer (&private->display_path, g_free);

  /* First delete the buffers folder, then the root cache folder.
   * We expect each folder to be empty when we delete them, but we don't
   * raise CRITICALs here, because if the user stored some file manually
   * in one of these folders, there is nothing Clay can do (and we
   * certainly are not going to delete files we haven't created
   * ourselves). So we just print to stderr for letting people know.
   *
   * Each child item is responsible for cleaning up after itself.
   */
  if (private->cache_xml)
    {
      GError *error = NULL;

      if (! g_file_delete (private->cache_xml, NULL, &error))
        g_printerr ("%s: failed to delete the cached XML file `%s`: %s\n",
                    G_STRFUNC, g_file_peek_path (private->cache_xml),
                    error->message);

      g_object_unref (private->cache_xml);
      g_clear_error (&error);
    }
  if (private->buffers_folder)
    {
      if (g_rmdir (private->buffers_folder) == -1)
        g_printerr ("%s: failed to delete the cached buffers folder `%s`: %s\n",
                    G_STRFUNC, private->buffers_folder, g_strerror (errno));

      g_free (private->buffers_folder);
    }
  if (private->cache_folder)
    {
      if (g_rmdir (private->cache_folder) == -1)
        g_printerr ("%s: failed to delete the image cache folder `%s`: %s\n",
                    G_STRFUNC, private->cache_folder, g_strerror (errno));

      g_free (private->cache_folder);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);

  /* Don't just free the list silently. It is supposed to be empty.
   * Instead just assert this fact to warn on improper management of
   * hidden items.
   */
  if (private->hidden_items != NULL)
    {
      g_warning ("%s: the hidden items list should be empty (%d items remaining).",
                 G_STRFUNC, g_list_length (private->hidden_items));
      g_list_free (private->hidden_items);
    }
}

static void
clay_image_name_changed (ClayObject *object)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  const gchar      *name;

  if (CLAY_OBJECT_CLASS (parent_class)->name_changed)
    CLAY_OBJECT_CLASS (parent_class)->name_changed (object);

  g_clear_pointer (&private->display_name, g_free);
  g_clear_pointer (&private->display_path, g_free);

  /* We never want the empty string as a name, so change empty strings
   * to NULL strings (without emitting the "name-changed" signal
   * again)
   */
  name = clay_object_get_name (object);
  if (name && strlen (name) == 0)
    {
      clay_object_name_free (object);
      name = NULL;
    }

  g_clear_object (&private->file);

  if (name)
    private->file = g_file_new_for_uri (name);
}

static gint64
clay_image_get_memsize (ClayObject *object,
                        gint64     *gui_size)
{
  ClayImage        *image   = CLAY_IMAGE (object);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  gint64            memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->palette),
                                      gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->projection),
                                      gui_size);

  memsize += clay_g_list_get_memsize (clay_image_get_guides (image),
                                      sizeof (ClayGuide));

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->grid), gui_size);

  memsize += clay_g_list_get_memsize (clay_image_get_sample_points (image),
                                      sizeof (ClaySamplePoint));

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->layers),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (private->channels),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (private->paths),
                                      gui_size);

  memsize += clay_g_slist_get_memsize (private->layer_stack, 0);

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->selection_mask),
                                      gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->parasites),
                                      gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->undo_stack),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (private->redo_stack),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_image_get_size (ClayViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  ClayImage *image = CLAY_IMAGE (viewable);

  *width  = clay_image_get_width  (image);
  *height = clay_image_get_height (image);

  return TRUE;
}

static void
clay_image_size_changed (ClayViewable *viewable)
{
  ClayImage *image = CLAY_IMAGE (viewable);
  GList     *all_items;
  GList     *list;

  if (CLAY_VIEWABLE_CLASS (parent_class)->size_changed)
    CLAY_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  all_items = clay_image_get_layer_list (image);
  for (list = all_items; list; list = g_list_next (list))
    {
      ClayLayerMask *mask = clay_layer_get_mask (CLAY_LAYER (list->data));

      clay_viewable_size_changed (CLAY_VIEWABLE (list->data));

      if (mask)
        clay_viewable_size_changed (CLAY_VIEWABLE (mask));
    }
  g_list_free (all_items);

  all_items = clay_image_get_channel_list (image);
  g_list_free_full (all_items, (GDestroyNotify) clay_viewable_size_changed);

  all_items = clay_image_get_path_list (image);
  g_list_free_full (all_items, (GDestroyNotify) clay_viewable_size_changed);

  clay_viewable_size_changed (CLAY_VIEWABLE (clay_image_get_mask (image)));

  clay_image_metadata_update_pixel_size (image);

  g_clear_object (&CLAY_IMAGE_GET_PRIVATE (image)->pickable_buffer);

  clay_image_update_bounding_box (image);
}

static gchar *
clay_image_get_description (ClayViewable  *viewable,
                            gchar        **tooltip)
{
  ClayImage *image = CLAY_IMAGE (viewable);

  if (tooltip)
    *tooltip = g_strdup (clay_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          clay_image_get_display_name (image),
                          clay_image_get_id (image));
}

static void
clay_image_real_mode_changed (ClayImage *image)
{
  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));

  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (image));
}

static void
clay_image_real_precision_changed (ClayImage *image)
{
  clay_image_metadata_update_bits_per_sample (image);

  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));
}

static void
clay_image_real_resolution_changed (ClayImage *image)
{
  clay_image_metadata_update_resolution (image);
}

static void
clay_image_real_size_changed_detailed (ClayImage *image,
                                       gint       previous_origin_x,
                                       gint       previous_origin_y,
                                       gint       previous_width,
                                       gint       previous_height)
{
  /* Whenever ClayImage::size-changed-detailed is emitted, so is
   * ClayViewable::size-changed. Clients choose what signal to listen
   * to depending on how much info they need.
   */
  clay_viewable_size_changed (CLAY_VIEWABLE (image));
}

static void
clay_image_real_unit_changed (ClayImage *image)
{
  clay_image_metadata_update_resolution (image);
}

static void
clay_image_real_colormap_changed (ClayImage *image,
                                  gint       color_index)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  clay_image_colormap_update_formats (image);

  if (clay_image_get_base_type (image) == CLAY_INDEXED)
    {
      /* A colormap alteration affects the whole image */
      clay_image_invalidate_all (image);

      clay_item_stack_invalidate_previews (CLAY_ITEM_STACK (private->layers->container));
    }
}

static const guint8 *
clay_image_color_managed_get_icc_profile (ClayColorManaged *managed,
                                          gsize            *len)
{
  return clay_image_get_icc_profile (CLAY_IMAGE (managed), len);
}

static ClayColorProfile *
clay_image_color_managed_get_color_profile (ClayColorManaged *managed)
{
  ClayImage        *image = CLAY_IMAGE (managed);
  ClayColorProfile *profile;

  profile = clay_image_get_color_profile (image);

  if (! profile)
    profile = clay_image_get_builtin_color_profile (image);

  return profile;
}

static void
clay_image_color_managed_profile_changed (ClayColorManaged *managed)
{
  ClayImage     *image  = CLAY_IMAGE (managed);
  ClayItemStack *layers = CLAY_ITEM_STACK (clay_image_get_layers (image));

  clay_image_metadata_update_colorspace (image);

  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (image));
  clay_item_stack_profile_changed (layers);
}

static ClayColorProfile *
clay_image_color_managed_get_simulation_profile (ClayColorManaged *managed)
{
  ClayImage        *image = CLAY_IMAGE (managed);
  ClayColorProfile *profile;

  profile = clay_image_get_simulation_profile (image);

  return profile;
}

static void
clay_image_color_managed_simulation_profile_changed (ClayColorManaged *managed)
{
  ClayImage *image = CLAY_IMAGE (managed);

  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (image));
}

static ClayColorRenderingIntent
clay_image_color_managed_get_simulation_intent (ClayColorManaged *managed)
{
  ClayImage *image = CLAY_IMAGE (managed);

  return clay_image_get_simulation_intent (image);
}

static void
clay_image_color_managed_simulation_intent_changed (ClayColorManaged *managed)
{
  ClayImage *image = CLAY_IMAGE (managed);

  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (image));
}

static gboolean
clay_image_color_managed_get_simulation_bpc (ClayColorManaged *managed)
{
  ClayImage *image = CLAY_IMAGE (managed);

  return clay_image_get_simulation_bpc (image);
}

static void
clay_image_color_managed_simulation_bpc_changed (ClayColorManaged *managed)
{
  ClayImage *image = CLAY_IMAGE (managed);

  clay_projectable_structure_changed (CLAY_PROJECTABLE (image));
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (image));
}

static void
clay_image_projectable_flush (ClayProjectable *projectable,
                              gboolean         invalidate_preview)
{
  ClayImage        *image   = CLAY_IMAGE (projectable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->flush_accum.alpha_changed)
    {
      clay_image_alpha_changed (image);
      private->flush_accum.alpha_changed = FALSE;
    }

  if (private->flush_accum.mask_changed)
    {
      clay_image_mask_changed (image);
      private->flush_accum.mask_changed = FALSE;
    }

  if (private->flush_accum.floating_selection_changed)
    {
      clay_image_floating_selection_changed (image);
      private->flush_accum.floating_selection_changed = FALSE;
    }

  if (private->flush_accum.preview_invalidated)
    {
      /*  don't invalidate the preview here, the projection does this when
       *  it is completely constructed.
       */
      private->flush_accum.preview_invalidated = FALSE;
    }
}

static ClayImage *
clay_image_get_image (ClayProjectable *projectable)
{
  return CLAY_IMAGE (projectable);
}

static const Babl *
clay_image_get_proj_format (ClayProjectable *projectable)
{
  ClayImage        *image   = CLAY_IMAGE (projectable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  switch (private->base_type)
    {
    case CLAY_RGB:
    case CLAY_INDEXED:
      return clay_image_get_format (image, CLAY_RGB,
                                    clay_image_get_precision (image), TRUE,
                                    clay_image_get_layer_space (image));

    case CLAY_GRAY:
      return clay_image_get_format (image, CLAY_GRAY,
                                    clay_image_get_precision (image), TRUE,
                                    clay_image_get_layer_space (image));
    }

  g_return_val_if_reached (NULL);
}

static void
clay_image_pickable_flush (ClayPickable *pickable)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (pickable);

  return clay_pickable_flush (CLAY_PICKABLE (private->projection));
}

static GeglBuffer *
clay_image_get_buffer (ClayPickable *pickable)
{
  ClayImage        *image   = CLAY_IMAGE (pickable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! private->pickable_buffer)
    {
      GeglBuffer *buffer;

      buffer = clay_pickable_get_buffer (CLAY_PICKABLE (private->projection));

      if (! private->show_all)
        {
          private->pickable_buffer = g_object_ref (buffer);
        }
      else
        {
          private->pickable_buffer = gegl_buffer_create_sub_buffer (
            buffer,
            GEGL_RECTANGLE (0, 0,
                            clay_image_get_width  (image),
                            clay_image_get_height (image)));
        }
    }

  return private->pickable_buffer;
}

static gboolean
clay_image_get_pixel_at (ClayPickable *pickable,
                         gint          x,
                         gint          y,
                         const Babl   *format,
                         gpointer      pixel)
{
  ClayImage        *image   = CLAY_IMAGE (pickable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < clay_image_get_width  (image) &&
      y < clay_image_get_height (image))
    {
      return clay_pickable_get_pixel_at (CLAY_PICKABLE (private->projection),
                                         x, y, format, pixel);
    }

  return FALSE;
}

static gdouble
clay_image_get_opacity_at (ClayPickable *pickable,
                           gint          x,
                           gint          y)
{
  ClayImage        *image   = CLAY_IMAGE (pickable);
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (x >= 0                            &&
      y >= 0                            &&
      x < clay_image_get_width  (image) &&
      y < clay_image_get_height (image))
    {
      return clay_pickable_get_opacity_at (CLAY_PICKABLE (private->projection),
                                           x, y);
    }

  return FALSE;
}

static void
clay_image_get_pixel_average (ClayPickable        *pickable,
                              const GeglRectangle *rect,
                              const Babl          *format,
                              gpointer             pixel)
{
  GeglBuffer *buffer = clay_pickable_get_buffer (pickable);

  return clay_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
}

static GeglRectangle
clay_image_get_bounding_box (ClayProjectable *projectable)
{
  ClayImage *image = CLAY_IMAGE (projectable);

  return CLAY_IMAGE_GET_PRIVATE (image)->bounding_box;
}

static GeglNode *
clay_image_get_graph (ClayProjectable *projectable)
{
  ClayImage         *image   = CLAY_IMAGE (projectable);
  ClayImagePrivate  *private = CLAY_IMAGE_GET_PRIVATE (image);
  GeglNode          *layers_node;
  GeglNode          *channels_node;
  GeglNode          *output;
  ClayComponentMask  mask;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  layers_node =
    clay_filter_stack_get_graph (CLAY_FILTER_STACK (private->layers->container));

  gegl_node_add_child (private->graph, layers_node);

  mask = ~clay_image_get_visible_mask (image) & CLAY_COMPONENT_MASK_ALL;

  private->visible_mask =
    gegl_node_new_child (private->graph,
                         "operation", "clay:mask-components",
                         "mask",      mask,
                         "alpha",     1.0,
                         NULL);

  gegl_node_link (layers_node, private->visible_mask);

  channels_node =
    clay_filter_stack_get_graph (CLAY_FILTER_STACK (private->channels->container));

  gegl_node_add_child (private->graph, channels_node);

  gegl_node_link (private->visible_mask, channels_node);

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_link (channels_node, output);

  return private->graph;
}

static void
clay_image_projection_buffer_notify (ClayProjection   *projection,
                                     const GParamSpec *pspec,
                                     ClayImage        *image)
{
  g_clear_object (&CLAY_IMAGE_GET_PRIVATE (image)->pickable_buffer);
}

static void
clay_image_mask_update (ClayDrawable *drawable,
                        gint          x,
                        gint          y,
                        gint          width,
                        gint          height,
                        ClayImage    *image)
{
  CLAY_IMAGE_GET_PRIVATE (image)->flush_accum.mask_changed = TRUE;
}

static void
clay_image_layers_changed (ClayContainer *container,
                           ClayChannel   *channel,
                           ClayImage     *image)
{
  clay_image_update_bounding_box (image);
}

static void
clay_image_layer_offset_changed (ClayDrawable     *drawable,
                                 const GParamSpec *pspec,
                                 ClayImage        *image)
{
  clay_image_update_bounding_box (image);
}

static void
clay_image_layer_bounding_box_changed (ClayDrawable *drawable,
                                       ClayImage    *image)
{
  clay_image_update_bounding_box (image);
}

static void
clay_image_layer_alpha_changed (ClayDrawable *drawable,
                                ClayImage    *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  if (clay_container_get_n_children (private->layers->container) == 1)
    private->flush_accum.alpha_changed = TRUE;
}

static void
clay_image_channel_add (ClayContainer *container,
                        ClayChannel   *channel,
                        ClayImage     *image)
{
  if (! strcmp (CLAY_IMAGE_QUICK_MASK_NAME,
                clay_object_get_name (channel)))
    {
      clay_image_set_quick_mask_state (image, TRUE);
    }
}

static void
clay_image_channel_remove (ClayContainer *container,
                           ClayChannel   *channel,
                           ClayImage     *image)
{
  if (! strcmp (CLAY_IMAGE_QUICK_MASK_NAME,
                clay_object_get_name (channel)))
    {
      clay_image_set_quick_mask_state (image, FALSE);
    }
}

static void
clay_image_channel_name_changed (ClayChannel *channel,
                                 ClayImage   *image)
{
  if (! strcmp (CLAY_IMAGE_QUICK_MASK_NAME,
                clay_object_get_name (channel)))
    {
      clay_image_set_quick_mask_state (image, TRUE);
    }
  else if (clay_image_get_quick_mask_state (image) &&
           ! clay_image_get_quick_mask (image))
    {
      clay_image_set_quick_mask_state (image, FALSE);
    }
}

static void
clay_image_channel_color_changed (ClayChannel *channel,
                                  ClayImage   *image)
{
  if (! strcmp (CLAY_IMAGE_QUICK_MASK_NAME,
                clay_object_get_name (channel)))
    {
      g_clear_object (&(CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_color));
      CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_color = gegl_color_duplicate (channel->color);
    }
}

static void
clay_image_selected_layers_notify (ClayItemTree     *tree,
                                   const GParamSpec *pspec,
                                   ClayImage        *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  GList            *layers  = clay_image_get_selected_layers (image);

  if (private->layer_stack)
    for (GList *iter = private->layer_stack->data; iter; iter = iter->next)
      g_signal_handlers_disconnect_by_func (iter->data,
                                            G_CALLBACK (clay_image_selected_drawables_changed),
                                            image);

  if (layers)
    {
      /*  Configure the layer stack to reflect this change  */
      GSList *prev_layers;

      while ((prev_layers = g_slist_find_custom (private->layer_stack, layers,
                                                 (GCompareFunc) clay_image_layer_stack_cmp)))
        {
          g_list_free (prev_layers->data);
          private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                      prev_layers);
        }
      private->layer_stack = g_slist_prepend (private->layer_stack, g_list_copy (layers));
    }

  for (GList *iter = layers; iter; iter = iter->next)
    g_signal_connect_swapped (iter->data, "edit-mask-changed",
                              G_CALLBACK (clay_image_selected_drawables_changed),
                              image);

  if (layers && clay_image_get_selected_channels (image))
    clay_image_set_selected_channels (image, NULL);
  else
    g_signal_emit (image, clay_image_signals[SELECTED_DRAWABLES_CHANGED], 0);

  /* When changing the layer selection, we try and save all layers which
   * are not selected.
   */
#if 0
  /* XXX Disabling the whole buffer saving code for now. */
  for (GList *iter = clay_image_get_layer_list (image); iter; iter = iter->next)
    {
      ClayLayer *layer = CLAY_LAYER (iter->data);

      if (! g_list_find (layers, layer))
        {
          clay_drawable_save_buffer (CLAY_DRAWABLE (layer));
          if (clay_layer_get_mask (layer))
            clay_drawable_save_buffer (CLAY_DRAWABLE (clay_layer_get_mask (layer)));
        }
    }
#endif

  g_signal_emit (image, clay_image_signals[SELECTED_LAYERS_CHANGED], 0);
}

static void
clay_image_selected_channels_notify (ClayItemTree     *tree,
                                     const GParamSpec *pspec,
                                     ClayImage        *image)
{
  GList *channels = clay_image_get_selected_channels (image);

  g_signal_emit (image, clay_image_signals[SELECTED_CHANNELS_CHANGED], 0);

  if (channels && clay_image_get_selected_layers (image))
    clay_image_set_selected_layers (image, NULL);
  else
    g_signal_emit (image, clay_image_signals[SELECTED_DRAWABLES_CHANGED], 0);
}

static void
clay_image_selected_paths_notify (ClayItemTree     *tree,
                                  const GParamSpec *pspec,
                                  ClayImage        *image)
{
  g_signal_emit (image, clay_image_signals[SELECTED_PATHS_CHANGED], 0);
}

static void
clay_image_freeze_bounding_box (ClayImage *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count++;
}

static void
clay_image_thaw_bounding_box (ClayImage *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);

  private->bounding_box_freeze_count--;

  if (private->bounding_box_freeze_count == 0 &&
      private->bounding_box_update_pending)
    {
      private->bounding_box_update_pending = FALSE;

      clay_image_update_bounding_box (image);
    }
}

static void
clay_image_update_bounding_box (ClayImage *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  GeglRectangle     bounding_box;

  if (private->bounding_box_freeze_count > 0)
    {
      private->bounding_box_update_pending = TRUE;

      return;
    }

  bounding_box.x      = 0;
  bounding_box.y      = 0;
  bounding_box.width  = clay_image_get_width  (image);
  bounding_box.height = clay_image_get_height (image);

  if (private->show_all)
    {
      GList *iter;

      for (iter = clay_image_get_layer_iter (image);
           iter;
           iter = g_list_next (iter))
        {
          ClayLayer     *layer = iter->data;
          GeglRectangle  layer_bounding_box;
          gint           offset_x;
          gint           offset_y;

          clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);

          layer_bounding_box = clay_drawable_get_bounding_box (
            CLAY_DRAWABLE (layer));

          layer_bounding_box.x += offset_x;
          layer_bounding_box.y += offset_y;

          gegl_rectangle_bounding_box (&bounding_box,
                                       &bounding_box, &layer_bounding_box);
        }
    }

  if (! gegl_rectangle_equal (&bounding_box, &private->bounding_box))
    {
      private->bounding_box = bounding_box;

      clay_projectable_bounds_changed (CLAY_PROJECTABLE (image), 0, 0);
    }
}

static gint
clay_image_layer_stack_cmp (GList *layers1,
                            GList *layers2)
{
  if (g_list_length (layers1) != g_list_length (layers2))
    {
      /* We don't really need to order lists of layers, and only care
       * about identity.
       */
      return 1;
    }
  else
    {
      GList *iter;

      for (iter = layers1; iter; iter = iter->next)
        {
          if (! g_list_find (layers2, iter->data))
            return 1;
        }
      return 0;
    }
}

/**
 * clay_image_rec_remove_layer_stack_dups:
 * @image:
 *
 * Recursively remove duplicates from the layer stack. You should not
 * call this directly, call clay_image_clean_layer_stack() instead.
 */
static void
clay_image_rec_remove_layer_stack_dups (ClayImage *image,
                                        GSList    *start)
{
  ClayImagePrivate *private;
  GSList           *dup_layers;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (start == NULL || start->next == NULL)
    return;

  while ((dup_layers = g_slist_find_custom (start->next, start->data,
                                            (GCompareFunc) clay_image_layer_stack_cmp)))
    {
      g_list_free (dup_layers->data);
      /* We can safely remove the duplicate then search again because we
       * know that @start is never removed as we search after it.
       */
      private->layer_stack = g_slist_delete_link (private->layer_stack,
                                                  dup_layers);
    }

  clay_image_rec_remove_layer_stack_dups (image, start->next);
}

/**
 * clay_image_clean_layer_stack:
 * @image:
 *
 * Remove any duplicate and empty selections in the layer stack.
 */
static void
clay_image_clean_layer_stack (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /* First remove all empty layer lists. */
  private->layer_stack = g_slist_remove_all (private->layer_stack, NULL);
  /* Then remove all duplicates. */
  clay_image_rec_remove_layer_stack_dups (image, private->layer_stack);
}

static void
clay_image_rec_filter_remove_undo (ClayImage    *image,
                                   ClayDrawable *drawable)
{
  ClayContainer *filters;

  if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
    {
      ClayContainer *stack = clay_viewable_get_children (CLAY_VIEWABLE (drawable));
      GList         *children;
      GList         *iter;

      children = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (stack));

      for (iter = children; iter; iter = iter->next)
        {
          ClayDrawable *child = iter->data;

          clay_image_rec_filter_remove_undo (image, child);
        }
    }

  filters = clay_drawable_get_filters (drawable);

  if (clay_container_get_n_children (filters) > 0)
    {
      GList *filter_list;

      for (filter_list = CLAY_LIST (filters)->queue->tail; filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (CLAY_IS_DRAWABLE_FILTER (filter_list->data) &&
              ! clay_drawable_filter_get_temporary (filter_list->data))
            {
              ClayDrawableFilter *filter = filter_list->data;

              clay_image_undo_push_filter_remove (image,
                                                  _("Remove filter"),
                                                  drawable, filter);
            }
        }
    }
}

static void
clay_image_remove_from_layer_stack (ClayImage *image,
                                    ClayLayer *layer)
{
  ClayImagePrivate *private;
  GSList           *slist;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_LAYER (layer));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /* Remove layer itself from the MRU layer stack. */
  for (slist = private->layer_stack; slist; slist = slist->next)
    slist->data = g_list_remove (slist->data, layer);

  /*  Also remove all children of a group layer from the layer_stack  */
  if (clay_viewable_get_children (CLAY_VIEWABLE (layer)))
    {
      ClayContainer *stack = clay_viewable_get_children (CLAY_VIEWABLE (layer));
      GList         *children;
      GList         *list;

      children = clay_item_stack_get_item_list (CLAY_ITEM_STACK (stack));

      for (list = children; list; list = g_list_next (list))
        {
          ClayLayer *child = list->data;

          for (slist = private->layer_stack; slist; slist = slist->next)
            slist->data = g_list_remove (slist->data, child);
        }

      g_list_free (children);
    }

  clay_image_clean_layer_stack (image);
}

static gint
clay_image_selected_is_descendant (ClayViewable *selected,
                                   ClayViewable *viewable)
{
  /* Used as a GCompareFunc to g_list_find_custom() in order to know if
   * one of the selected items is a descendant to @viewable.
   */
  if (clay_viewable_is_ancestor (viewable, selected))
    return 0;
  else
    return 1;
}


/*  public functions  */

ClayImage *
clay_image_new (Clay              *clay,
                gint               width,
                gint               height,
                ClayImageBaseType  base_type,
                ClayPrecision      precision)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (clay_babl_is_valid (base_type, precision), NULL);

  return g_object_new (CLAY_TYPE_IMAGE,
                       "clay",      clay,
                       "width",     width,
                       "height",    height,
                       "base-type", base_type,
                       "precision", precision,
                       NULL);
}

gint64
clay_image_estimate_memsize (ClayImage         *image,
                             ClayComponentType  component_type,
                             gint               width,
                             gint               height)
{
  GList  *drawables;
  GList  *list;
  gint    current_width;
  gint    current_height;
  gint64  current_size;
  gint64  scalable_size = 0;
  gint64  scaled_size   = 0;
  gint64  new_size;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  current_width  = clay_image_get_width (image);
  current_height = clay_image_get_height (image);
  current_size   = clay_object_get_memsize (CLAY_OBJECT (image), NULL);

  /*  the part of the image's memsize that scales linearly with the image  */
  drawables = clay_image_item_list_get_list (image,
                                             CLAY_ITEM_TYPE_LAYERS |
                                             CLAY_ITEM_TYPE_CHANNELS,
                                             CLAY_ITEM_SET_ALL);

  clay_image_item_list_filter (drawables);

  drawables = g_list_prepend (drawables, clay_image_get_mask (image));

  for (list = drawables; list; list = g_list_next (list))
    {
      ClayDrawable *drawable = list->data;
      gdouble       drawable_width;
      gdouble       drawable_height;

      drawable_width  = clay_item_get_width  (CLAY_ITEM (drawable));
      drawable_height = clay_item_get_height (CLAY_ITEM (drawable));

      scalable_size += clay_drawable_estimate_memsize (drawable,
                                                       clay_drawable_get_component_type (drawable),
                                                       drawable_width,
                                                       drawable_height);

      scaled_size += clay_drawable_estimate_memsize (drawable,
                                                     component_type,
                                                     drawable_width * width /
                                                     current_width,
                                                     drawable_height * height /
                                                     current_height);
    }

  g_list_free (drawables);

  scalable_size +=
    clay_projection_estimate_memsize (clay_image_get_base_type (image),
                                      clay_image_get_component_type (image),
                                      clay_image_get_width (image),
                                      clay_image_get_height (image));

  scaled_size +=
    clay_projection_estimate_memsize (clay_image_get_base_type (image),
                                      component_type,
                                      width, height);

  CLAY_LOG (IMAGE_SCALE,
            "scalable_size = %"G_GINT64_FORMAT"  scaled_size = %"G_GINT64_FORMAT,
            scalable_size, scaled_size);

  new_size = current_size - scalable_size + scaled_size;

  return new_size;
}

ClayImageBaseType
clay_image_get_base_type (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), -1);

  return CLAY_IMAGE_GET_PRIVATE (image)->base_type;
}

ClayComponentType
clay_image_get_component_type (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), -1);

  return clay_babl_component_type (CLAY_IMAGE_GET_PRIVATE (image)->precision);
}

ClayPrecision
clay_image_get_precision (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), -1);

  return CLAY_IMAGE_GET_PRIVATE (image)->precision;
}

const Babl *
clay_image_get_format (ClayImage         *image,
                       ClayImageBaseType  base_type,
                       ClayPrecision      precision,
                       gboolean           with_alpha,
                       const Babl        *space)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  switch (base_type)
    {
    case CLAY_RGB:
    case CLAY_GRAY:
      return clay_babl_format (base_type, precision, with_alpha, space);

    case CLAY_INDEXED:
      if (precision == CLAY_PRECISION_U8_NON_LINEAR)
        {
          if (with_alpha)
            return clay_image_colormap_get_rgba_format (image);
          else
            return clay_image_colormap_get_rgb_format (image);
        }
    }

  g_return_val_if_reached (NULL);
}

const Babl *
clay_image_get_layer_space (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->layer_space;
}

const Babl *
clay_image_get_layer_format (ClayImage *image,
                             gboolean   with_alpha)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_get_format (image,
                                clay_image_get_base_type (image),
                                clay_image_get_precision (image),
                                with_alpha,
                                clay_image_get_layer_space (image));
}

const Babl *
clay_image_get_channel_format (ClayImage *image)
{
  ClayPrecision precision;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  precision = clay_image_get_precision (image);

  if (precision == CLAY_PRECISION_U8_NON_LINEAR)
    return clay_image_get_format (image, CLAY_GRAY,
                                  clay_image_get_precision (image),
                                  FALSE, NULL);

  return clay_babl_mask_format (precision);
}

const Babl *
clay_image_get_mask_format (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_babl_mask_format (clay_image_get_precision (image));
}

ClayLayerMode
clay_image_get_default_new_layer_mode (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), CLAY_LAYER_MODE_NORMAL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->new_layer_mode == -1)
    {
      GList *layers = clay_image_get_layer_list (image);

      if (layers)
        {
          GList *list;

          for (list = layers; list; list = g_list_next (list))
            {
              ClayLayer     *layer = list->data;
              ClayLayerMode  mode  = clay_layer_get_mode (layer);

              if (! clay_layer_mode_is_legacy (mode))
                {
                  /*  any non-legacy layer switches the mode to non-legacy
                   */
                  private->new_layer_mode = CLAY_LAYER_MODE_NORMAL;
                  break;
                }
            }

          /*  only if all layers are legacy, the mode is also legacy
           */
          if (! list)
            private->new_layer_mode = CLAY_LAYER_MODE_NORMAL_LEGACY;

          g_list_free (layers);
        }
      else
        {
          /*  empty images are never considered legacy
           */
          private->new_layer_mode = CLAY_LAYER_MODE_NORMAL;
        }
    }

  return private->new_layer_mode;
}

void
clay_image_unset_default_new_layer_mode (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->new_layer_mode = -1;
}

gint
clay_image_get_id (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), -1);

  return CLAY_IMAGE_GET_PRIVATE (image)->ID;
}

ClayImage *
clay_image_get_by_id (Clay *clay,
                      gint  image_id)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->image_table == NULL)
    return NULL;

  return (ClayImage *) clay_id_table_lookup (clay->image_table, image_id);
}

void
clay_image_set_file (ClayImage *image,
                     GFile     *file)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->file != file)
    {
      clay_object_take_name (CLAY_OBJECT (image),
                             file ? g_file_get_uri (file) : NULL);
    }
}

/**
 * clay_image_get_untitled_file:
 *
 * Returns: A #GFile saying "Untitled" for newly created images.
 **/
GFile *
clay_image_get_untitled_file (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! private->untitled_file)
    private->untitled_file = g_file_new_for_uri (_("Untitled"));

  return private->untitled_file;
}

/**
 * clay_image_get_file_or_untitled:
 * @image: A #ClayImage.
 *
 * Get the file of the XCF image, or the "Untitled" file if there is no file.
 *
 * Returns: A #GFile.
 **/
GFile *
clay_image_get_file_or_untitled (ClayImage *image)
{
  GFile *file;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  file = clay_image_get_file (image);

  if (! file)
    file = clay_image_get_untitled_file (image);

  return file;
}

/**
 * clay_image_get_file:
 * @image: A #ClayImage.
 *
 * Get the file of the XCF image, or %NULL if there is no file.
 *
 * Returns: (nullable): The file, or %NULL.
 **/
GFile *
clay_image_get_file (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->file;
}

/**
 * clay_image_get_imported_file:
 * @image: A #ClayImage.
 *
 * Returns: (nullable): The file of the imported image, or %NULL if the image
 * has been saved as XCF after it was imported.
 **/
GFile *
clay_image_get_imported_file (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->imported_file;
}

/**
 * clay_image_get_exported_file:
 * @image: A #ClayImage.
 *
 * Returns: (nullable): The file of the image last exported from this XCF file,
 * or %NULL if the image has never been exported.
 **/
GFile *
clay_image_get_exported_file (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->exported_file;
}

/**
 * clay_image_get_save_a_copy_file:
 * @image: A #ClayImage.
 *
 * Returns: The URI of the last copy that was saved of this XCF file.
 **/
GFile *
clay_image_get_save_a_copy_file (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->save_a_copy_file;
}

/**
 * clay_image_get_any_file:
 * @image: A #ClayImage.
 *
 * Returns: The XCF file, the imported file, or the exported file, in
 * that order of precedence.
 **/
GFile *
clay_image_get_any_file (ClayImage *image)
{
  GFile *file;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  file = clay_image_get_file (image);
  if (! file)
    {
      file = clay_image_get_imported_file (image);
      if (! file)
        {
          file = clay_image_get_exported_file (image);
        }
    }

  return file;
}

/**
 * clay_image_set_imported_uri:
 * @image: A #ClayImage.
 * @file:
 *
 * Sets the URI this file was imported from.
 **/
void
clay_image_set_imported_file (ClayImage *image,
                              GFile     *file)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->imported_file, file))
    {
      clay_object_name_changed (CLAY_OBJECT (image));
    }

  if (! private->resolution_set && file != NULL)
    {
      /* Unlike new files (which follow technological progress and will
       * use higher default resolution, or explicitly chosen templates),
       * imported files have a more backward-compatible value.
       *
       * 72 PPI is traditionally the default value when none other had
       * been explicitly set (for instance it is the default when no
       * resolution metadata was set in Exif version 2.32, and below,
       * standard). This historical value will only ever apply to loaded
       * images. New images will continue having more modern or
       * templated defaults.
       */
      private->xresolution     = 72.0;
      private->yresolution     = 72.0;
      private->resolution_unit = clay_unit_inch ();
    }
}

/**
 * clay_image_set_exported_file:
 * @image: A #ClayImage.
 * @file:
 *
 * Sets the file this image was last exported to. Note that saving as
 * XCF is not "exporting".
 **/
void
clay_image_set_exported_file (ClayImage *image,
                              GFile     *file)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (g_set_object (&private->exported_file, file))
    {
      clay_object_name_changed (CLAY_OBJECT (image));
    }
}

/**
 * clay_image_set_save_a_copy_file:
 * @image: A #ClayImage.
 * @uri:
 *
 * Set the URI to the last copy this XCF file was saved to through the
 * "save a copy" action.
 **/
void
clay_image_set_save_a_copy_file (ClayImage *image,
                                 GFile     *file)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_set_object (&private->save_a_copy_file, file);
}

static gchar *
clay_image_format_display_uri (ClayImage *image,
                               gboolean   basename)
{
  const gchar *uri_format    = NULL;
  const gchar *export_status = NULL;
  GFile       *file          = NULL;
  GFile       *source        = NULL;
  GFile       *dest          = NULL;
  GFile       *display_file  = NULL;
  gboolean     is_imported;
  gboolean     is_exported;
  gchar       *display_uri   = NULL;
  gchar       *format_string;
  gchar       *tmp;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  file   = clay_image_get_file (image);
  source = clay_image_get_imported_file (image);
  dest   = clay_image_get_exported_file (image);

  is_imported = (source != NULL);
  is_exported = (dest   != NULL);

  if (file)
    {
      display_file = g_object_ref (file);
      uri_format   = "%s";
    }
  else
    {
      if (is_imported)
        display_file = source;

      /* Calculate filename suffix */
      if (! clay_image_is_export_dirty (image))
        {
          if (is_exported)
            {
              display_file  = dest;
              export_status = _(" (exported)");
            }
          else if (is_imported)
            {
              export_status = _(" (overwritten)");
            }
          else
            {
              g_warning ("Unexpected code path, Save+export implementation is buggy!");
            }
        }
      else if (is_imported)
        {
          export_status = _(" (imported)");
        }

      if (display_file)
        display_file = clay_file_with_new_extension (display_file, NULL);

      uri_format = "[%s]";
    }

  if (! display_file)
    display_file = g_object_ref (clay_image_get_untitled_file (image));

  if (basename)
    display_uri = g_path_get_basename (clay_file_get_utf8_name (display_file));
  else
    display_uri = g_strdup (clay_file_get_utf8_name (display_file));

  g_object_unref (display_file);

  format_string = g_strconcat (uri_format, export_status, NULL);

  tmp = g_strdup_printf (format_string, display_uri);
  g_free (display_uri);
  display_uri = tmp;

  g_free (format_string);

  return display_uri;
}

const gchar *
clay_image_get_display_name (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! private->display_name)
    private->display_name = clay_image_format_display_uri (image, TRUE);

  return private->display_name;
}

const gchar *
clay_image_get_display_path (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! private->display_path)
    private->display_path = clay_image_format_display_uri (image, FALSE);

  return private->display_path;
}

void
clay_image_set_load_proc (ClayImage           *image,
                          ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->load_proc = proc;
}

ClayPlugInProcedure *
clay_image_get_load_proc (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->load_proc;
}

void
clay_image_set_save_proc (ClayImage           *image,
                          ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->save_proc = proc;
}

ClayPlugInProcedure *
clay_image_get_save_proc (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->save_proc;
}

void
clay_image_set_export_proc (ClayImage           *image,
                            ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->export_proc = proc;
}

ClayPlugInProcedure *
clay_image_get_export_proc (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->export_proc;
}

const gchar *
clay_image_get_cache_folder (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->cache_folder == NULL)
    {
      gchar *folder_name = g_strdup_printf ("image-%d", clay_image_get_id (image));
      gchar *path        = g_build_filename (clay_cache_directory (), "images", folder_name, NULL);

      private->cache_folder = path;

      g_free (folder_name);
    }

  return private->cache_folder;
}

GFile *
clay_image_get_cache_xml_file (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (clay_image_get_cache_folder (image) != NULL, NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->cache_xml == NULL)
    {
      gchar *path;

      path = g_build_filename (clay_image_get_cache_folder (image), "wlbr-project.xml", NULL);

      private->cache_xml = g_file_new_for_path (path);
      g_free (path);
    }

  return private->cache_xml;
}

const gchar *
clay_image_get_buffers_folder (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (clay_image_get_cache_folder (image) != NULL, NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->buffers_folder == NULL)
    {
      gchar *folder;

      folder = g_build_filename (clay_image_get_cache_folder (image), "buffers", NULL);
      if (g_mkdir_with_parents (folder, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
        {
          g_critical ("%s: failed to create the buffer folder `%s`: %s\n",
                      G_STRFUNC, folder, g_strerror (errno));
          g_free (folder);
          return NULL;
        }

      private->buffers_folder = folder;
    }

  return private->buffers_folder;
}

gint
clay_image_get_xcf_version (ClayImage    *image,
                            gboolean      zlib_compression,
                            gint         *clay_version,
                            const gchar **version_string,
                            gchar       **version_reason)
{
  GList       *items;
  GList       *list;
  GList       *reasons = NULL;
  gint         version = 0;  /* default to oldest */
  const gchar *enum_desc;

#define ADD_REASON(_reason)                                       \
  if (version_reason) {                                           \
    gchar *tmp = _reason;                                         \
    if (g_list_find_custom (reasons, tmp, (GCompareFunc) strcmp)) \
      g_free (tmp);                                               \
    else                                                          \
      reasons = g_list_prepend (reasons, tmp); }

  /* need version 1 for colormaps */
  if (clay_image_get_colormap_palette (image))
    version = 1;

  items = clay_image_get_layer_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      ClayLayer     *layer = CLAY_LAYER (list->data);
      ClayContainer *filters;

      switch (clay_layer_get_mode (layer))
        {
          /*  Modes that exist since ancient times  */
        case CLAY_LAYER_MODE_NORMAL_LEGACY:
        case CLAY_LAYER_MODE_DISSOLVE:
        case CLAY_LAYER_MODE_BEHIND_LEGACY:
        case CLAY_LAYER_MODE_MULTIPLY_LEGACY:
        case CLAY_LAYER_MODE_SCREEN_LEGACY:
        case CLAY_LAYER_MODE_OVERLAY_LEGACY:
        case CLAY_LAYER_MODE_DIFFERENCE_LEGACY:
        case CLAY_LAYER_MODE_ADDITION_LEGACY:
        case CLAY_LAYER_MODE_SUBTRACT_LEGACY:
        case CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY:
        case CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
        case CLAY_LAYER_MODE_HSV_HUE_LEGACY:
        case CLAY_LAYER_MODE_HSV_SATURATION_LEGACY:
        case CLAY_LAYER_MODE_HSL_COLOR_LEGACY:
        case CLAY_LAYER_MODE_HSV_VALUE_LEGACY:
        case CLAY_LAYER_MODE_DIVIDE_LEGACY:
        case CLAY_LAYER_MODE_DODGE_LEGACY:
        case CLAY_LAYER_MODE_BURN_LEGACY:
        case CLAY_LAYER_MODE_HARDLIGHT_LEGACY:
          break;

          /*  Since 2.6  */
        case CLAY_LAYER_MODE_SOFTLIGHT_LEGACY:
        case CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
        case CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY:
        case CLAY_LAYER_MODE_COLOR_ERASE_LEGACY:
          clay_enum_get_value (CLAY_TYPE_LAYER_MODE,
                               clay_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "Clay 2.6"));
          version = MAX (2, version);
          break;

          /*  Since 2.10  */
        case CLAY_LAYER_MODE_OVERLAY:
        case CLAY_LAYER_MODE_LCH_HUE:
        case CLAY_LAYER_MODE_LCH_CHROMA:
        case CLAY_LAYER_MODE_LCH_COLOR:
        case CLAY_LAYER_MODE_LCH_LIGHTNESS:
          clay_enum_get_value (CLAY_TYPE_LAYER_MODE,
                               clay_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "Clay 2.10"));
          version = MAX (9, version);
          break;

          /*  Since 2.10  */
        case CLAY_LAYER_MODE_NORMAL:
        case CLAY_LAYER_MODE_BEHIND:
        case CLAY_LAYER_MODE_MULTIPLY:
        case CLAY_LAYER_MODE_SCREEN:
        case CLAY_LAYER_MODE_DIFFERENCE:
        case CLAY_LAYER_MODE_ADDITION:
        case CLAY_LAYER_MODE_SUBTRACT:
        case CLAY_LAYER_MODE_DARKEN_ONLY:
        case CLAY_LAYER_MODE_LIGHTEN_ONLY:
        case CLAY_LAYER_MODE_HSV_HUE:
        case CLAY_LAYER_MODE_HSV_SATURATION:
        case CLAY_LAYER_MODE_HSL_COLOR:
        case CLAY_LAYER_MODE_HSV_VALUE:
        case CLAY_LAYER_MODE_DIVIDE:
        case CLAY_LAYER_MODE_DODGE:
        case CLAY_LAYER_MODE_BURN:
        case CLAY_LAYER_MODE_HARDLIGHT:
        case CLAY_LAYER_MODE_SOFTLIGHT:
        case CLAY_LAYER_MODE_GRAIN_EXTRACT:
        case CLAY_LAYER_MODE_GRAIN_MERGE:
        case CLAY_LAYER_MODE_VIVID_LIGHT:
        case CLAY_LAYER_MODE_PIN_LIGHT:
        case CLAY_LAYER_MODE_LINEAR_LIGHT:
        case CLAY_LAYER_MODE_HARD_MIX:
        case CLAY_LAYER_MODE_EXCLUSION:
        case CLAY_LAYER_MODE_LINEAR_BURN:
        case CLAY_LAYER_MODE_LUMA_DARKEN_ONLY:
        case CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY:
        case CLAY_LAYER_MODE_LUMINANCE:
        case CLAY_LAYER_MODE_COLOR_ERASE:
        case CLAY_LAYER_MODE_ERASE:
        case CLAY_LAYER_MODE_MERGE:
        case CLAY_LAYER_MODE_SPLIT:
        case CLAY_LAYER_MODE_PASS_THROUGH:
          clay_enum_get_value (CLAY_TYPE_LAYER_MODE,
                               clay_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          ADD_REASON (g_strdup_printf (_("Layer mode '%s' was added in %s"),
                                       enum_desc, "Clay 2.10"));
          version = MAX (10, version);
          break;

          /*  Just here instead of default so we get compiler warnings  */
        case CLAY_LAYER_MODE_REPLACE:
        case CLAY_LAYER_MODE_OVERWRITE:
        case CLAY_LAYER_MODE_ANTI_ERASE:
        case CLAY_LAYER_MODE_SEPARATOR:
          clay_enum_get_value (CLAY_TYPE_LAYER_MODE,
                               clay_layer_get_mode (layer),
                               NULL, NULL, &enum_desc, NULL);
          g_warning ("%s: layer mode '%s' should not be set on a layer.",
                     G_STRFUNC, enum_desc);
          break;
        }

      if (clay_layer_get_mask (layer))
        {
          ClayLayerMask *mask = clay_layer_get_mask (layer);

          filters = clay_drawable_get_filters (CLAY_DRAWABLE (mask));
          if (clay_container_get_n_children (filters) > 0)
            {
              ADD_REASON (g_strdup_printf (_("Channel effects were saved "
                                             "in %s"), "Clay 3.2.6"));
              version = MAX (26, version);
            }
        }

      /* need version 3 for layer trees */
      if (clay_viewable_get_children (CLAY_VIEWABLE (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Layer groups were added in %s"),
                                       "Clay 2.8"));
          version = MAX (3, version);

          /* need version 13 for group layers with masks */
          if (clay_layer_get_mask (layer))
            {
              ADD_REASON (g_strdup_printf (_("Masks on layer groups were "
                                             "added in %s"), "Clay 2.10"));
              version = MAX (13, version);
            }

          if (clay_item_get_lock_position (CLAY_ITEM (layer)))
            {
              ADD_REASON (g_strdup_printf (_("Position locks on layer groups were added in %s"),
                                           "Clay 3.0"));
              version = MAX (17, version);
            }

          if (clay_layer_get_lock_alpha (layer))
            {
              ADD_REASON (g_strdup_printf (_("Alpha channel locks on layer groups were added in %s"),
                                           "Clay 3.0"));
              version = MAX (17, version);
            }
        }

      if (clay_item_get_lock_visibility (CLAY_ITEM (layer)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "Clay 3.0"));
          version = MAX (17, version);
        }

      if (CLAY_IS_TEXT_LAYER (layer) &&
          /* If we loaded an old XCF and didn't touch the text layers, then we
           * just resave the text layer as-is. No need to bump the XCF version.
           */
          ! CLAY_TEXT_LAYER (layer)->text_parasite_is_old)
        {
          ADD_REASON (g_strdup_printf (_("Format of font information in text layer was changed in %s"),
                                       "Clay 3.0"));
          version = MAX (19, version);
        }

      filters = clay_drawable_get_filters (CLAY_DRAWABLE (layer));
      if (clay_container_get_n_children (filters) > 0)
        {
          ADD_REASON (g_strdup_printf (_("Layer effects were added in %s"),
                                       "Clay 3.0"));
          /* Version 20 added layer effects; version 22 stored the effect
           * version as an additional field. Only ever store as XCF 22
           * when layer effects are used.
           */
          version = MAX (22, version);
        }

      if ((clay_layer_get_real_blend_space (layer) == CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL)
        ||(clay_layer_get_real_composite_space (layer) == CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL))
        {
          ADD_REASON (g_strdup_printf (_("A new perceptual blending space was added in %s"),
                                       "Clay 3.0"));
          /* The blending space variant corresponding to SPACE_RGB_PERCEPTUAL in <3.0
           * corresponds to R'G'B'A which is NON_LINEAR in babl. Perceptual in babl is
           * R~G~B~A, >= 3.0 the code, comments and usage matches the existing enum value
           * as being NON_LINEAR and new layers created use the new integer value for
           * PERCEPTUAL.
           */
          version = MAX (23, version);
        }

      /* Need version 24 for vector layers. */
      if (CLAY_IS_VECTOR_LAYER (layer))
        {
          ADD_REASON (g_strdup_printf (_("Vector layers were added in %s"),
                                       "Clay 3.2"));
          version = MAX (24, version);
        }

      /* Need version 25 for link layers. */
      if (CLAY_IS_LINK_LAYER (layer))
        {
          ADD_REASON (g_strdup_printf (_("Link layers were added in %s"),
                                       "Clay 3.2"));
          version = MAX (25, version);
        }

      /* Need version 27 for text layers carrying a transformation.  The
       * properties themselves are as old as ClayText, so an older Clay
       * deserializes the parasite happily and then renders the layer
       * untransformed -- the file would open looking wrong rather than
       * failing.  Only bump when a matrix is actually stored, so plain text
       * layers keep saving as before.
       */
      if (CLAY_IS_TEXT_LAYER (layer))
        {
          ClayText *text = clay_text_layer_get_text (CLAY_TEXT_LAYER (layer));

          if (text)
            {
              ClayMatrix3 matrix;

              clay_text_get_transformation (text, &matrix);

              if (! clay_matrix3_is_identity (&matrix))
                {
                  ADD_REASON (g_strdup_printf (_("Transformed text layers were "
                                                 "added in %s"), "Clay 3.4"));
                  version = MAX (27, version);
                }
            }
        }
    }
  g_list_free (items);

  items = clay_image_get_channel_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      ClayChannel   *channel = CLAY_CHANNEL (list->data);
      ClayContainer *filters;

      if (clay_item_get_lock_visibility (CLAY_ITEM (channel)))
        {
          ADD_REASON (g_strdup_printf (_("Visibility locks were added in %s"),
                                       "Clay 3.0"));
          version = MAX (17, version);
        }

      filters = clay_drawable_get_filters (CLAY_DRAWABLE (channel));
      if (clay_container_get_n_children (filters) > 0)
        {
          ADD_REASON (g_strdup_printf (_("Channel effects were saved in %s"),
                                         "Clay 3.2.6"));
          version = MAX (26, version);
        }
    }
  g_list_free (items);

  if (g_list_length (clay_image_get_selected_paths (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple path selection was "
                                     "added in %s"), "Clay 3.0.0"));
      version = MAX (18, version);
    }

  items = clay_image_get_path_list (image);
  for (list = items; list; list = g_list_next (list))
    {
      ClayPath *path = CLAY_PATH (list->data);

      if (clay_item_get_color_tag (CLAY_ITEM (path)) != CLAY_COLOR_TAG_NONE)
        {
          ADD_REASON (g_strdup_printf (_("Storing color tags in path was "
                                         "added in %s"), "Clay 3.0.0"));
          version = MAX (18, version);
        }
      if (clay_item_get_lock_content (CLAY_ITEM (list->data)) ||
          clay_item_get_lock_position (CLAY_ITEM (list->data)))
        {
          ADD_REASON (g_strdup_printf (_("Storing locks in path was "
                                         "added in %s"), "Clay 3.0.0"));
          version = MAX (18, version);
        }
    }
  g_list_free (items);

  /* version 6 for new metadata has been dropped since they are
   * saved through parasites, which is compatible with older versions.
   */

  /* need version 7 for != 8-bit gamma images */
  if (clay_image_get_precision (image) != CLAY_PRECISION_U8_NON_LINEAR)
    {
      ADD_REASON (g_strdup_printf (_("High bit-depth images were added "
                                     "in %s"), "Clay 2.10"));
      version = MAX (7, version);
    }

  /* need version 12 for > 8-bit images for proper endian swapping */
  if (clay_image_get_component_type (image) > CLAY_COMPONENT_TYPE_U8)
    {
      ADD_REASON (g_strdup_printf (_("Encoding of high bit-depth images was "
                                     "fixed in %s"), "Clay 2.10"));
      version = MAX (12, version);
    }

  /* need version 8 for zlib compression */
  if (zlib_compression)
    {
      ADD_REASON (g_strdup_printf (_("Internal zlib compression was "
                                     "added in %s"), "Clay 2.10"));
      version = MAX (8, version);
    }

  /* if version is 10 (lots of new layer modes), go to version 11 with
   * 64 bit offsets right away
   */
  if (version == 10)
    version = 11;

  /* use the image's in-memory size as an upper bound to estimate the
   * need for 64 bit file offsets inside the XCF, this is a *very*
   * conservative estimate and should never fail
   */
  if (clay_object_get_memsize (CLAY_OBJECT (image), NULL) >= ((gint64) 1 << 32))
    {
      ADD_REASON (g_strdup_printf (_("Support for image files larger than "
                                     "4GB was added in %s"), "Clay 2.10"));
      version = MAX (11, version);
    }

  if (g_list_length (clay_image_get_selected_layers (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple layer selection was "
                                     "added in %s"), "Clay 3.0.0"));
      version = MAX (14, version);
    }

  if ((list = clay_image_get_guides (image)))
    {
      for (; list; list = g_list_next (list))
        {
          gint32 position = clay_guide_get_position (list->data);

          if (position < 0 ||
              (clay_guide_get_orientation (list->data) == CLAY_ORIENTATION_HORIZONTAL &&
               position > clay_image_get_height (image)) ||
              (clay_guide_get_orientation (list->data) == CLAY_ORIENTATION_VERTICAL &&
               position > clay_image_get_width (image)))
            {
              ADD_REASON (g_strdup_printf (_("Off-canvas guides "
                                             "added in %s"), "Clay 3.0.0"));
              version = MAX (15, version);
              break;
            }
        }
    }

  if (clay_image_get_stored_item_sets (image, CLAY_TYPE_LAYER) ||
      clay_image_get_stored_item_sets (image, CLAY_TYPE_CHANNEL))
    {
      ADD_REASON (g_strdup_printf (_("Item set and pattern search in item's name were "
                                     "added in %s"), "Clay 3.0.0"));
      version = MAX (16, version);
    }
  if (g_list_length (clay_image_get_selected_channels (image)) > 1)
    {
      ADD_REASON (g_strdup_printf (_("Multiple channel selection was "
                                     "added in %s"), "Clay 3.0.0"));
      version = MAX (16, version);
    }

  /* Note: user unit storage was changed in XCF 21, but we can still
   * easily save older XCF (we use the unit name for both singular and
   * plural forms). Therefore we don't bump the XCF version unnecessarily
   * and don't add any test.
   */

#undef ADD_REASON

  switch (version)
    {
    case 0:
    case 1:
    case 2:
      if (clay_version)   *clay_version   = 206;
      if (version_string) *version_string = "Clay 2.6";
      break;

    case 3:
      if (clay_version)   *clay_version   = 208;
      if (version_string) *version_string = "Clay 2.8";
      break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
      if (clay_version)   *clay_version   = 210;
      if (version_string) *version_string = "Clay 2.10";
      break;
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      if (clay_version)   *clay_version   = 300;
      if (version_string) *version_string = "Clay 3.0";
      break;
    case 24:
    case 25:
      if (clay_version)   *clay_version   = 320;
      if (version_string) *version_string = "Clay 3.2";
      break;
    case 26:
      if (clay_version)   *clay_version   = 326;
      if (version_string) *version_string = "Clay 3.2.6";
      break;
    case 27:
      if (clay_version)   *clay_version   = 340;
      if (version_string) *version_string = "Clay 3.4";
      break;
    default:
      /* We should have as many cases as there are function elements in
       * array xcf_loaders in app/xcf/xcf.c
       * Don't forget to add new cases. This is a reminder CRITICAL in
       * case we forgot.
       */
      g_return_val_if_reached (version);
    }

  if (version_reason && reasons)
    {
      GString *reason = g_string_new (NULL);

      reasons = g_list_sort (reasons, (GCompareFunc) strcmp);

      for (list = reasons; list; list = g_list_next (list))
        {
          g_string_append (reason, list->data);
          if (g_list_next (list))
            g_string_append_c (reason, '\n');
        }

      *version_reason = g_string_free (reason, FALSE);
    }
  if (reasons)
    g_list_free_full (reasons, g_free);

  return version;
}

void
clay_image_set_xcf_compression (ClayImage *image,
                                gboolean   compression)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->xcf_compression = compression;
}

gboolean
clay_image_get_xcf_compression (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return CLAY_IMAGE_GET_PRIVATE (image)->xcf_compression;
}

void
clay_image_set_resolution (ClayImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /* don't allow to set the resolution out of bounds */
  if (xresolution < CLAY_MIN_RESOLUTION || xresolution > CLAY_MAX_RESOLUTION ||
      yresolution < CLAY_MIN_RESOLUTION || yresolution > CLAY_MAX_RESOLUTION)
    return;

  private->resolution_set = TRUE;

  if ((ABS (private->xresolution - xresolution) >= 1e-5) ||
      (ABS (private->yresolution - yresolution) >= 1e-5))
    {
      clay_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Resolution"));

      private->xresolution = xresolution;
      private->yresolution = yresolution;

      clay_image_resolution_changed (image);
      clay_image_size_changed_detailed (image,
                                        0,
                                        0,
                                        clay_image_get_width (image),
                                        clay_image_get_height (image));
    }
}

void
clay_image_get_resolution (ClayImage *image,
                           gdouble   *xresolution,
                           gdouble   *yresolution)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (xresolution != NULL && yresolution != NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  *xresolution = private->xresolution;
  *yresolution = private->yresolution;
}

void
clay_image_resolution_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[RESOLUTION_CHANGED], 0);
}

void
clay_image_set_unit (ClayImage *image,
                     ClayUnit  *unit)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_UNIT (unit));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->resolution_unit != unit)
    {
      clay_image_undo_push_image_resolution (image,
                                             C_("undo-type", "Change Image Unit"));

      private->resolution_unit = unit;
      clay_image_unit_changed (image);
    }
}

ClayUnit *
clay_image_get_unit (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->resolution_unit;
}

void
clay_image_unit_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[UNIT_CHANGED], 0);
}

gint
clay_image_get_width (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->width;
}

gint
clay_image_get_height (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->height;
}

gboolean
clay_image_has_alpha (ClayImage *image)
{
  ClayImagePrivate *private;
  ClayLayer        *layer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), TRUE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  layer = CLAY_LAYER (clay_container_get_first_child (private->layers->container));

  return ((clay_image_get_n_layers (image) > 1) ||
          (layer && clay_drawable_has_alpha (CLAY_DRAWABLE (layer))));
}

/* While clay_image_has_alpha() is a simple heuristic just checking
 * number or layers and the alpha channel of the one layer, this will
 * actually loop through every pixel of the projection and check if any
 * has transparency. It will necessarily be slower yet should be used
 * when we want a more accurate answer.
 */
gboolean
clay_image_has_transparency (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), TRUE);

  return clay_projection_has_alpha (clay_image_get_projection (image));
}

gboolean
clay_image_is_empty (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), TRUE);

  return clay_container_is_empty (CLAY_IMAGE_GET_PRIVATE (image)->layers->container);
}

void
clay_image_set_floating_selection (ClayImage *image,
                                   ClayLayer *floating_sel)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (floating_sel == NULL || CLAY_IS_LAYER (floating_sel));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->floating_sel != floating_sel)
    {
      private->floating_sel = floating_sel;

      private->flush_accum.floating_selection_changed = TRUE;
    }
}

ClayLayer *
clay_image_get_floating_selection (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->floating_sel;
}

void
clay_image_floating_selection_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[FLOATING_SELECTION_CHANGED], 0);
}

ClayChannel *
clay_image_get_mask (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->selection_mask;
}

void
clay_image_mask_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[MASK_CHANGED], 0);
}

/**
 * clay_image_mask_intersect:
 * @image:   the #ClayImage
 * @items:   a list of #ClayItem
 * @x: (out) (optional): return location for x
 * @y: (out) (optional): return location for y
 * @width: (out) (optional): return location for the width
 * @height: (out) (optional): return location for the height
 *
 * Intersect the area of the @items and its image's selection mask.
 * The computed area is the bounding box of the selection intersection
 * within the image. These values are only returned if the function
 * returns %TRUE.
 *
 * Note that even though the items bounding box may not be empty, it is
 * possible to get a return value of %FALSE. Imagine disjoint items, one
 * on the left, one on the right, and a selection in the middle not
 * intersecting with any of the items.
 *
 * Returns: %TRUE if the selection intersects with any of the @items.
 */
gboolean
clay_image_mask_intersect (ClayImage *image,
                           GList     *items,
                           gint      *x,
                           gint      *y,
                           gint      *width,
                           gint      *height)
{
  ClayChannel *selection;
  GList       *iter;
  gint         sel_x, sel_y, sel_width, sel_height;
  gint         x1 = G_MAXINT;
  gint         y1 = G_MAXINT;
  gint         x2 = G_MININT;
  gint         y2 = G_MININT;
  gboolean     intersect = FALSE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  for (iter = items; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_ITEM (iter->data), FALSE);
      g_return_val_if_fail (clay_item_is_attached (iter->data), FALSE);
      g_return_val_if_fail (clay_item_get_image (iter->data) == image, FALSE);
    }

  selection = clay_image_get_mask (image);
  if (selection)
    clay_item_bounds (CLAY_ITEM (selection),
                      &sel_x, &sel_y, &sel_width, &sel_height);

  for (iter = items; iter; iter = iter->next)
    {
      ClayItem *item = iter->data;
      gboolean  item_intersect;
      gint      tmp_x, tmp_y;
      gint      tmp_width, tmp_height;

      clay_item_get_offset (item, &tmp_x, &tmp_y);

      /* check for is_empty() before intersecting so we ignore the
       * selection if it is suspended (like when stroking)
       */
      if (CLAY_ITEM (selection) != item       &&
          ! clay_channel_is_empty (selection))
        {
          item_intersect = clay_rectangle_intersect (sel_x, sel_y, sel_width, sel_height,
                                                     tmp_x, tmp_y,
                                                     clay_item_get_width  (item),
                                                     clay_item_get_height (item),
                                                     &tmp_x, &tmp_y,
                                                     &tmp_width, &tmp_height);
        }
      else
        {
          tmp_width  = clay_item_get_width  (item);
          tmp_height = clay_item_get_height (item);

          item_intersect = TRUE;
        }

      if (item_intersect)
        {
          x1 = MIN (x1, tmp_x);
          y1 = MIN (y1, tmp_y);
          x2 = MAX (x2, tmp_x + tmp_width);
          y2 = MAX (y2, tmp_y + tmp_height);

          intersect = TRUE;
        }
    }

  if (intersect)
    {
      if (x)      *x      = x1;
      if (y)      *y      = y1;
      if (width)  *width  = x2 - x1;
      if (height) *height = y2 - y1;
    }

  return intersect;
}

void
clay_image_take_mask (ClayImage   *image,
                      ClayChannel *mask)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SELECTION (mask));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->selection_mask)
    g_object_unref (private->selection_mask);

  private->selection_mask = g_object_ref_sink (mask);

  g_signal_connect (private->selection_mask, "update",
                    G_CALLBACK (clay_image_mask_update),
                    image);
}


/*  image components  */

const Babl *
clay_image_get_component_format (ClayImage       *image,
                                 ClayChannelType  channel)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  switch (channel)
    {
    case CLAY_CHANNEL_RED:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_image_get_precision (image),
                                         RED);

    case CLAY_CHANNEL_GREEN:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_image_get_precision (image),
                                         GREEN);

    case CLAY_CHANNEL_BLUE:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_image_get_precision (image),
                                         BLUE);

    case CLAY_CHANNEL_ALPHA:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_image_get_precision (image),
                                         ALPHA);

    case CLAY_CHANNEL_GRAY:
      return clay_babl_component_format (CLAY_GRAY,
                                         clay_image_get_precision (image),
                                         GRAY);

    case CLAY_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
clay_image_get_component_index (ClayImage       *image,
                                ClayChannelType  channel)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), -1);

  switch (channel)
    {
    case CLAY_CHANNEL_RED:     return RED;
    case CLAY_CHANNEL_GREEN:   return GREEN;
    case CLAY_CHANNEL_BLUE:    return BLUE;
    case CLAY_CHANNEL_GRAY:    return GRAY;
    case CLAY_CHANNEL_INDEXED: return INDEXED;
    case CLAY_CHANNEL_ALPHA:
      switch (clay_image_get_base_type (image))
        {
        case CLAY_RGB:     return ALPHA;
        case CLAY_GRAY:    return ALPHA_G;
        case CLAY_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

void
clay_image_set_component_active (ClayImage       *image,
                                 ClayChannelType  channel,
                                 gboolean         active)
{
  ClayImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  index = clay_image_get_component_index (image, channel);

  if (index != -1 && active != private->active[index])
    {
      private->active[index] = active ? TRUE : FALSE;

      /*  If there is an active channel and we mess with the components,
       *  the active channel gets unset...
       */
      clay_image_unset_selected_channels (image);

      g_signal_emit (image,
                     clay_image_signals[COMPONENT_ACTIVE_CHANGED], 0,
                     channel);
    }
}

gboolean
clay_image_get_component_active (ClayImage       *image,
                                 ClayChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  index = clay_image_get_component_index (image, channel);

  if (index != -1)
    return CLAY_IMAGE_GET_PRIVATE (image)->active[index];

  return FALSE;
}

ClayComponentMask
clay_image_get_active_mask (ClayImage *image)
{
  ClayImagePrivate  *private;
  ClayComponentMask  mask = 0;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  switch (clay_image_get_base_type (image))
    {
    case CLAY_RGB:
      mask |= (private->active[RED])   ? CLAY_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GREEN]) ? CLAY_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[BLUE])  ? CLAY_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA]) ? CLAY_COMPONENT_MASK_ALPHA : 0;
      break;

    case CLAY_GRAY:
    case CLAY_INDEXED:
      mask |= (private->active[GRAY])    ? CLAY_COMPONENT_MASK_RED   : 0;
      mask |= (private->active[GRAY])    ? CLAY_COMPONENT_MASK_GREEN : 0;
      mask |= (private->active[GRAY])    ? CLAY_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->active[ALPHA_G]) ? CLAY_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}

void
clay_image_set_component_visible (ClayImage       *image,
                                  ClayChannelType  channel,
                                  gboolean         visible)
{
  ClayImagePrivate *private;
  gint              index = -1;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  index = clay_image_get_component_index (image, channel);

  if (index != -1 && visible != private->visible[index])
    {
      private->visible[index] = visible ? TRUE : FALSE;

      if (private->visible_mask)
        {
          ClayComponentMask mask;

          mask = ~clay_image_get_visible_mask (image) & CLAY_COMPONENT_MASK_ALL;

          gegl_node_set (private->visible_mask,
                         "mask", mask,
                         NULL);
        }

      g_signal_emit (image,
                     clay_image_signals[COMPONENT_VISIBILITY_CHANGED], 0,
                     channel);

      clay_image_invalidate_all (image);
    }
}

gboolean
clay_image_get_component_visible (ClayImage       *image,
                                  ClayChannelType  channel)
{
  gint index = -1;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  index = clay_image_get_component_index (image, channel);

  if (index != -1)
    return CLAY_IMAGE_GET_PRIVATE (image)->visible[index];

  return FALSE;
}

ClayComponentMask
clay_image_get_visible_mask (ClayImage *image)
{
  ClayImagePrivate  *private;
  ClayComponentMask  mask = 0;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  switch (clay_image_get_base_type (image))
    {
    case CLAY_RGB:
      mask |= (private->visible[RED])   ? CLAY_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GREEN]) ? CLAY_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[BLUE])  ? CLAY_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? CLAY_COMPONENT_MASK_ALPHA : 0;
      break;

    case CLAY_GRAY:
    case CLAY_INDEXED:
      mask |= (private->visible[GRAY])  ? CLAY_COMPONENT_MASK_RED   : 0;
      mask |= (private->visible[GRAY])  ? CLAY_COMPONENT_MASK_GREEN : 0;
      mask |= (private->visible[GRAY])  ? CLAY_COMPONENT_MASK_BLUE  : 0;
      mask |= (private->visible[ALPHA]) ? CLAY_COMPONENT_MASK_ALPHA : 0;
      break;
    }

  return mask;
}


/*  emitting image signals  */

void
clay_image_mode_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[MODE_CHANGED], 0);
}

void
clay_image_precision_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[PRECISION_CHANGED], 0);
}

void
clay_image_alpha_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[ALPHA_CHANGED], 0);
}

void
clay_image_selected_drawables_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[SELECTED_DRAWABLES_CHANGED], 0);
}

void
clay_image_invalidate (ClayImage *image,
                       gint       x,
                       gint       y,
                       gint       width,
                       gint       height)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  clay_projectable_invalidate (CLAY_PROJECTABLE (image),
                               x, y, width, height);

  CLAY_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated = TRUE;
}

void
clay_image_invalidate_all (ClayImage *image)
{
  const GeglRectangle *bounding_box;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  bounding_box = &CLAY_IMAGE_GET_PRIVATE (image)->bounding_box;

  clay_image_invalidate (image,
                         bounding_box->x,     bounding_box->y,
                         bounding_box->width, bounding_box->height);
}

void
clay_image_guide_added (ClayImage *image,
                        ClayGuide *guide)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  g_signal_emit (image, clay_image_signals[GUIDE_ADDED], 0,
                 guide);
}

void
clay_image_guide_removed (ClayImage *image,
                          ClayGuide *guide)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  g_signal_emit (image, clay_image_signals[GUIDE_REMOVED], 0,
                 guide);
}

void
clay_image_guide_moved (ClayImage *image,
                        ClayGuide *guide)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  g_signal_emit (image, clay_image_signals[GUIDE_MOVED], 0,
                 guide);
}

void
clay_image_sample_point_added (ClayImage       *image,
                               ClaySamplePoint *sample_point)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, clay_image_signals[SAMPLE_POINT_ADDED], 0,
                 sample_point);
}

void
clay_image_sample_point_removed (ClayImage       *image,
                                 ClaySamplePoint *sample_point)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, clay_image_signals[SAMPLE_POINT_REMOVED], 0,
                 sample_point);
}

void
clay_image_sample_point_moved (ClayImage       *image,
                               ClaySamplePoint *sample_point)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  g_signal_emit (image, clay_image_signals[SAMPLE_POINT_MOVED], 0,
                 sample_point);
}

/**
 * clay_image_size_changed_detailed:
 * @image:
 * @previous_origin_x:
 * @previous_origin_y:
 *
 * Emits the size-changed-detailed signal that is typically used to adjust the
 * position of the image in the display shell on various operations,
 * e.g. crop.
 *
 * This function makes sure that ClayViewable::size-changed is also emitted.
 **/
void
clay_image_size_changed_detailed (ClayImage *image,
                                  gint       previous_origin_x,
                                  gint       previous_origin_y,
                                  gint       previous_width,
                                  gint       previous_height)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[SIZE_CHANGED_DETAILED], 0,
                 previous_origin_x,
                 previous_origin_y,
                 previous_width,
                 previous_height);
}

void
clay_image_colormap_changed (ClayImage *image,
                             gint       color_index)
{
  ClayPalette *palette;
  gint         n_colors;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  palette = CLAY_IMAGE_GET_PRIVATE (image)->palette;
  n_colors = palette ? clay_palette_get_n_colors (palette) : 0;
  g_return_if_fail (color_index >= -1 && color_index < n_colors);

  g_signal_emit (image, clay_image_signals[COLORMAP_CHANGED], 0,
                 color_index);
}

void
clay_image_selection_invalidate (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[SELECTION_INVALIDATE], 0);
}

void
clay_image_quick_mask_changed (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[QUICK_MASK_CHANGED], 0);
}

void
clay_image_undo_event (ClayImage     *image,
                       ClayUndoEvent  event,
                       ClayUndo      *undo)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (((event == CLAY_UNDO_EVENT_UNDO_FREE   ||
                      event == CLAY_UNDO_EVENT_UNDO_FREEZE ||
                      event == CLAY_UNDO_EVENT_UNDO_THAW) && undo == NULL) ||
                    CLAY_IS_UNDO (undo));

  g_signal_emit (image, clay_image_signals[UNDO_EVENT], 0, event, undo);
}


/*  dirty counters  */

/* NOTE about the image->dirty counter:
 *   If 0, then the image is clean (ie, copy on disk is the same as the one
 *      in memory).
 *   If positive, then that's the number of dirtying operations done
 *       on the image since the last save.
 *   If negative, then user has hit undo and gone back in time prior
 *       to the saved copy.  Hitting redo will eventually come back to
 *       the saved copy.
 *
 *   The image is dirty (ie, needs saving) if counter is non-zero.
 *
 *   If the counter is around 100000, this is due to undo-ing back
 *   before a saved version, then changing the image (thus destroying
 *   the redo stack).  Once this has happened, it's impossible to get
 *   the image back to the state on disk, since the redo info has been
 *   freed.  See clayimage-undo.c for the gory details.
 */

/*
 * NEVER CALL clay_image_dirty() directly!
 *
 * If your code has just dirtied the image, push an undo instead.
 * Failing that, push the trivial undo which tells the user the
 * command is not undoable: undo_push_cantundo() (But really, it would
 * be best to push a proper undo).  If you just dirty the image
 * without pushing an undo then the dirty count is increased, but
 * popping that many undo actions won't lead to a clean image.
 */

gint
clay_image_dirty (ClayImage     *image,
                  ClayDirtyMask  dirty_mask)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->dirty++;
  private->export_dirty++;

  if (! private->dirty_time)
    private->dirty_time = time (NULL);

  g_signal_emit (image, clay_image_signals[DIRTY], 0, dirty_mask);

  TRC (("dirty %d -> %d\n", private->dirty - 1, private->dirty));

  return private->dirty;
}

gint
clay_image_clean (ClayImage     *image,
                  ClayDirtyMask  dirty_mask)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->dirty--;
  private->export_dirty--;

  g_signal_emit (image, clay_image_signals[CLEAN], 0, dirty_mask);

  TRC (("clean %d -> %d\n", private->dirty + 1, private->dirty));

  return private->dirty;
}

void
clay_image_clean_all (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->dirty      = 0;
  private->dirty_time = 0;

  g_signal_emit (image, clay_image_signals[CLEAN], 0, CLAY_DIRTY_ALL);

  clay_object_name_changed (CLAY_OBJECT (image));
}

void
clay_image_export_clean_all (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->export_dirty = 0;

  g_signal_emit (image, clay_image_signals[CLEAN], 0, CLAY_DIRTY_ALL);

  clay_object_name_changed (CLAY_OBJECT (image));
}

/**
 * clay_image_is_dirty:
 * @image:
 *
 * Returns: True if the image is dirty, false otherwise.
 **/
gint
clay_image_is_dirty (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return CLAY_IMAGE_GET_PRIVATE (image)->dirty != 0;
}

/**
 * clay_image_is_export_dirty:
 * @image:
 *
 * Returns: True if the image export is dirty, false otherwise.
 **/
gboolean
clay_image_is_export_dirty (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return CLAY_IMAGE_GET_PRIVATE (image)->export_dirty != 0;
}

gint64
clay_image_get_dirty_time (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->dirty_time;
}

/**
 * clay_image_saving:
 * @image:
 *
 * Emits the "saving" signal, indicating that @image is about to be saved,
 * or exported.
 */
void
clay_image_saving (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_signal_emit (image, clay_image_signals[SAVING], 0);
}

/**
 * clay_image_saved:
 * @image:
 * @file:
 *
 * Emits the "saved" signal, indicating that @image was saved to the
 * location specified by @file.
 */
void
clay_image_saved (ClayImage *image,
                  GFile     *file)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, clay_image_signals[SAVED], 0, file);
}

/**
 * clay_image_exported:
 * @image:
 * @file:
 *
 * Emits the "exported" signal, indicating that @image was exported to the
 * location specified by @file.
 */
void
clay_image_exported (ClayImage *image,
                     GFile     *file)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (image, clay_image_signals[EXPORTED], 0, file);
}


/*  flush this image's displays  */

void
clay_image_flush (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  clay_projectable_flush (CLAY_PROJECTABLE (image),
                          CLAY_IMAGE_GET_PRIVATE (image)->flush_accum.preview_invalidated);
}


/*  display / instance counters  */

gint
clay_image_get_display_count (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->disp_count;
}

void
clay_image_inc_display_count (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->disp_count++;
  g_signal_emit (image, clay_image_signals[DISPLAY_COUNT_CHANGED], 0,
                 CLAY_IMAGE_GET_PRIVATE (image)->disp_count);
}

void
clay_image_dec_display_count (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->disp_count--;
  g_signal_emit (image, clay_image_signals[DISPLAY_COUNT_CHANGED], 0,
                 CLAY_IMAGE_GET_PRIVATE (image)->disp_count);
}

gint
clay_image_get_instance_count (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->instance_count;
}

void
clay_image_inc_instance_count (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->instance_count++;
}

void
clay_image_inc_show_all_count (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->show_all++;

  if (CLAY_IMAGE_GET_PRIVATE (image)->show_all == 1)
    {
      g_clear_object (&CLAY_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      clay_image_update_bounding_box (image);
    }
}

void
clay_image_dec_show_all_count (ClayImage *image)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  CLAY_IMAGE_GET_PRIVATE (image)->show_all--;

  if (CLAY_IMAGE_GET_PRIVATE (image)->show_all == 0)
    {
      g_clear_object (&CLAY_IMAGE_GET_PRIVATE (image)->pickable_buffer);

      clay_image_update_bounding_box (image);
    }
}


/*  parasites  */

const ClayParasite *
clay_image_parasite_find (ClayImage   *image,
                          const gchar *name)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_parasite_list_find (CLAY_IMAGE_GET_PRIVATE (image)->parasites,
                                  name);
}

static void
list_func (gchar          *key,
           ClayParasite   *p,
           gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
clay_image_parasite_list (ClayImage *image)
{
  ClayImagePrivate  *private;
  gint               count;
  gchar            **list;
  gchar            **cur;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  count = clay_parasite_list_length (private->parasites);
  cur = list = g_new0 (gchar *, count + 1);

  clay_parasite_list_foreach (private->parasites, (GHFunc) list_func, &cur);

  return list;
}

gboolean
clay_image_parasite_validate (ClayImage           *image,
                              const ClayParasite  *parasite,
                              GError             **error)
{
  const gchar *name;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = clay_parasite_get_name (parasite);

  if (strcmp (name, CLAY_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      return clay_image_validate_icc_parasite (image, parasite,
                                               name, NULL, error);
    }
  else if (strcmp (name, "clay-comment") == 0)
    {
      const gchar *data;
      guint32      length;
      gboolean     valid  = FALSE;

      data = clay_parasite_get_data (parasite, &length);
      if (length > 0)
        {
          if (data[length - 1] == '\0')
            valid = g_utf8_validate (data, -1, NULL);
          else
            valid = g_utf8_validate (data, length, NULL);
        }

      if (! valid)
        {
          g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                       _("'clay-comment' parasite validation failed: "
                         "comment contains invalid UTF-8"));
          return FALSE;
        }
    }

  return TRUE;
}

void
clay_image_parasite_attach (ClayImage          *image,
                            const ClayParasite *parasite,
                            gboolean            push_undo)
{
  ClayImagePrivate *private;
  ClayParasite      copy;
  const gchar      *name;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (parasite != NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  name = clay_parasite_get_name (parasite);

  /*  this is so ugly and is only for the PDB  */
  if (strcmp (name, CLAY_ICC_PROFILE_PARASITE_NAME) == 0 ||
      strcmp (name, CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      ClayColorProfile *profile;
      ClayColorProfile *builtin;
      const guint8     *parasite_data;
      guint32           parasite_length;

      parasite_data = clay_parasite_get_data (parasite, &parasite_length);
      profile =
        clay_color_profile_new_from_icc_profile (parasite_data, parasite_length,
                                                 NULL);
      builtin = clay_image_get_builtin_color_profile (image);

      if (clay_color_profile_is_equal (profile, builtin))
        {
          /* setting the builtin profile is equal to removing the profile */
          clay_image_parasite_detach (image, CLAY_ICC_PROFILE_PARASITE_NAME,
                                      push_undo);
          g_object_unref (profile);
          return;
        }

      g_object_unref (profile);
    }

  /*  make a temporary copy of the ClayParasite struct because
   *  clay_parasite_shift_parent() changes it
   */
  copy = *parasite;

  /*  only set the dirty bit manually if we can be saved and the new
   *  parasite differs from the current one and we aren't undoable
   */
  if (push_undo && clay_parasite_is_undoable (&copy))
    clay_image_undo_push_image_parasite (image,
                                         C_("undo-type", "Attach Parasite to Image"),
                                         &copy);

  /*  We used to push a cantundo on the stack here. This made the undo stack
   *  unusable (NULL on the stack) and prevented people from undoing after a
   *  save (since most save plug-ins attach an undoable comment parasite).
   *  Now we simply attach the parasite without pushing an undo. That way
   *  it's undoable but does not block the undo system.   --Sven
   */
  clay_parasite_list_add (private->parasites, &copy);

  if (push_undo && clay_parasite_has_flag (&copy, CLAY_PARASITE_ATTACH_PARENT))
    {
      clay_parasite_shift_parent (&copy);
      clay_parasite_attach (image->clay, &copy);
    }

  if (strcmp (name, CLAY_ICC_PROFILE_PARASITE_NAME) == 0)
    _clay_image_update_color_profile (image, parasite);

  if (strcmp (name, CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _clay_image_update_simulation_profile (image, parasite);

  g_signal_emit (image, clay_image_signals[PARASITE_ATTACHED], 0,
                 name);
}

void
clay_image_parasite_detach (ClayImage   *image,
                            const gchar *name,
                            gboolean     push_undo)
{
  ClayImagePrivate   *private;
  const ClayParasite *parasite;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (name != NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! (parasite = clay_parasite_list_find (private->parasites, name)))
    return;

  if (push_undo && clay_parasite_is_undoable (parasite))
    clay_image_undo_push_image_parasite_remove (image,
                                                C_("undo-type", "Remove Parasite from Image"),
                                                name);

  clay_parasite_list_remove (private->parasites, name);

  if (strcmp (name, CLAY_ICC_PROFILE_PARASITE_NAME) == 0)
    _clay_image_update_color_profile (image, NULL);

  if (strcmp (name, CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
    _clay_image_update_simulation_profile (image, NULL);

  g_signal_emit (image, clay_image_signals[PARASITE_DETACHED], 0,
                 name);
}


/*  tattoos  */

ClayTattoo
clay_image_get_new_tattoo (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->tattoo_state++;

  if (G_UNLIKELY (private->tattoo_state == 0))
    g_warning ("%s: Tattoo state corrupted (integer overflow).", G_STRFUNC);

  return private->tattoo_state;
}

ClayTattoo
clay_image_get_tattoo_state (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->tattoo_state;
}

gboolean
clay_image_set_tattoo_state (ClayImage  *image,
                             ClayTattoo  val)
{
  GList      *all_items;
  GList      *list;
  gboolean    retval = TRUE;
  ClayTattoo  maxval = 0;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  /* Check that the layer tattoos don't overlap with channel or vector ones */
  all_items = clay_image_get_layer_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      ClayTattoo ltattoo;

      ltattoo = clay_item_get_tattoo (CLAY_ITEM (list->data));
      if (ltattoo > maxval)
        maxval = ltattoo;

      if (clay_image_get_channel_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in channel */

      if (clay_image_get_path_by_tattoo (image, ltattoo))
        retval = FALSE; /* Oopps duplicated tattoo in path */
    }

  g_list_free (all_items);

  /* Now check that the channel and path tattoos don't overlap */
  all_items = clay_image_get_channel_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      ClayTattoo ctattoo;

      ctattoo = clay_item_get_tattoo (CLAY_ITEM (list->data));
      if (ctattoo > maxval)
        maxval = ctattoo;

      if (clay_image_get_path_by_tattoo (image, ctattoo))
        retval = FALSE; /* Oopps duplicated tattoo in path */
    }

  g_list_free (all_items);

  /* Find the max tattoo value in the path */
  all_items = clay_image_get_path_list (image);

  for (list = all_items; list; list = g_list_next (list))
    {
      ClayTattoo vtattoo;

      vtattoo = clay_item_get_tattoo (CLAY_ITEM (list->data));
      if (vtattoo > maxval)
        maxval = vtattoo;
    }

  g_list_free (all_items);

  if (val < maxval)
    retval = FALSE;

  /* Must check if the state is valid */
  if (retval == TRUE)
    CLAY_IMAGE_GET_PRIVATE (image)->tattoo_state = val;

  return retval;
}


/*  projection  */

ClayProjection *
clay_image_get_projection (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->projection;
}


/*  layers / channels / path  */

ClayItemTree *
clay_image_get_layer_tree (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->layers;
}

ClayItemTree *
clay_image_get_channel_tree (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->channels;
}

ClayItemTree *
clay_image_get_path_tree (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->paths;
}

ClayContainer *
clay_image_get_items (ClayImage *image,
                      GType      item_type)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  if (item_type == CLAY_TYPE_LAYER)
    {
      return clay_image_get_layers (image);
    }
  else if (item_type == CLAY_TYPE_CHANNEL)
    {
      return clay_image_get_channels (image);
    }
  else if (item_type == CLAY_TYPE_PATH)
    {
      return clay_image_get_paths (image);
    }

  g_return_val_if_reached (NULL);
}

ClayContainer *
clay_image_get_layers (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->layers->container;
}

ClayContainer *
clay_image_get_channels (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->channels->container;
}

ClayContainer *
clay_image_get_paths (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->paths->container;
}

gint
clay_image_get_n_layers (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  stack = CLAY_ITEM_STACK (clay_image_get_layers (image));

  return clay_item_stack_get_n_items (stack);
}

gint
clay_image_get_n_channels (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  stack = CLAY_ITEM_STACK (clay_image_get_channels (image));

  return clay_item_stack_get_n_items (stack);
}

gint
clay_image_get_n_paths (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  stack = CLAY_ITEM_STACK (clay_image_get_paths (image));

  return clay_item_stack_get_n_items (stack);
}

GList *
clay_image_get_layer_iter (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_layers (image));

  return clay_item_stack_get_item_iter (stack);
}

GList *
clay_image_get_channel_iter (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_channels (image));

  return clay_item_stack_get_item_iter (stack);
}

GList *
clay_image_get_path_iter (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_paths (image));

  return clay_item_stack_get_item_iter (stack);
}

GList *
clay_image_get_layer_list (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_layers (image));

  return clay_item_stack_get_item_list (stack);
}

GList *
clay_image_get_channel_list (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_channels (image));

  return clay_item_stack_get_item_list (stack);
}

GList *
clay_image_get_path_list (ClayImage *image)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_paths (image));

  return clay_item_stack_get_item_list (stack);
}


/*  active drawable, layer, channel, path  */

void
clay_image_unset_selected_channels (ClayImage *image)
{
  ClayImagePrivate *private;
  GList            *channels;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  channels = clay_image_get_selected_channels (image);

  if (channels)
    {
      clay_image_set_selected_channels (image, NULL);

      if (private->layer_stack)
        clay_image_set_selected_layers (image, private->layer_stack->data);
    }
}

/**
 * clay_image_is_selected_drawable:
 * @image:
 * @drawable:
 *
 * Checks if @drawable belong to the list of currently selected
 * drawables. It doesn't mean this is the only selected drawable (if
 * this is what you want to check, use
 * clay_image_equal_selected_drawables() with a list containing only
 * this drawable).
 *
 * Returns: %TRUE is @drawable is one of the selected drawables.
 */
gboolean
clay_image_is_selected_drawable (ClayImage    *image,
                                 ClayDrawable *drawable)
{
  GList    *selected_drawables;
  gboolean  found;

  selected_drawables = clay_image_get_selected_drawables (image);
  found = (g_list_find (selected_drawables, drawable) != NULL);
  g_list_free (selected_drawables);

  return found;
}

/**
 * clay_image_equal_selected_drawables:
 * @image:
 * @drawables:
 *
 * Compare the list of @drawables with the selected drawables in @image
 * (i.e. the result of clay_image_equal_selected_drawables()).
 * The order of the @drawables does not matter, only if the size and
 * contents of the list is the same.
 */
gboolean
clay_image_equal_selected_drawables (ClayImage *image,
                                     GList     *drawables)
{
  GList    *selected_drawables;
  GList    *iter;
  gboolean  equal = FALSE;

  selected_drawables = clay_image_get_selected_drawables (image);

  if (g_list_length (drawables) == g_list_length (selected_drawables))
    {
      equal = TRUE;

      for (iter = drawables; iter; iter = iter->next)
        if (! g_list_find (selected_drawables, iter->data))
          {
            equal = FALSE;
            break;
          }
    }
  g_list_free (selected_drawables);

  return equal;
}

GList *
clay_image_get_selected_drawables (ClayImage *image)
{
  ClayImagePrivate *private;
  GList            *selected_channels;
  GList            *selected_layers;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  selected_channels = clay_item_tree_get_selected_items (private->channels);
  selected_layers   = clay_item_tree_get_selected_items (private->layers);

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (selected_channels)
    {
      return g_list_copy (selected_channels);
    }
  else if (selected_layers)
    {
      selected_layers = g_list_copy (selected_layers);
      if (g_list_length (selected_layers) == 1)
        {
          /* As a special case, if only one layer is selected and mask
           * edit is in progress, we return the mask as selected
           * drawable instead of the layer.
           */
          ClayLayer     *layer = CLAY_LAYER (selected_layers->data);
          ClayLayerMask *mask  = clay_layer_get_mask (layer);

          if (mask && clay_layer_get_edit_mask (layer))
            selected_layers->data = mask;
        }
      return selected_layers;
    }

  return NULL;
}

GList *
clay_image_get_selected_items (ClayImage *image,
                               GType      item_type)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  if (g_type_is_a (item_type, CLAY_TYPE_LAYER))
    {
      return clay_image_get_selected_layers (image);
    }
  else if (item_type == CLAY_TYPE_CHANNEL)
    {
      return clay_image_get_selected_channels (image);
    }
  else if (item_type == CLAY_TYPE_PATH)
    {
      return clay_image_get_selected_paths (image);
    }

  g_return_val_if_reached (NULL);
}

GList *
clay_image_get_selected_layers (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return clay_item_tree_get_selected_items (private->layers);
}

GList *
clay_image_get_selected_channels (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return clay_item_tree_get_selected_items (private->channels);
}

GList *
clay_image_get_selected_paths (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return clay_item_tree_get_selected_items (private->paths);
}

void
clay_image_set_selected_items (ClayImage *image,
                               GType      item_type,
                               GList     *items)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  if (g_type_is_a (item_type, CLAY_TYPE_LAYER))
    {
      clay_image_set_selected_layers (image, items);
    }
  else if (g_type_is_a (item_type, CLAY_TYPE_CHANNEL))
    {
      clay_image_set_selected_channels (image, items);
    }
  else if (g_type_is_a (item_type, CLAY_TYPE_PATH))
    {
      clay_image_set_selected_paths (image, items);
    }
  else
    {
      g_return_if_reached ();
    }
}

void
clay_image_set_selected_layers (ClayImage *image,
                                GList     *layers)
{
  ClayImagePrivate *private;
  ClayLayer        *floating_sel;
  GList            *selected_layers;
  GList            *layers2;
  GList            *iter;
  gboolean          selection_changed = TRUE;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  layers2 = g_list_copy (layers);
  for (iter = layers; iter; iter = iter->next)
    {
      g_return_if_fail (CLAY_IS_LAYER (iter->data));
      g_return_if_fail (clay_item_get_image (CLAY_ITEM (iter->data)) == image);

      /* Silently remove non-attached layers from selection. Do not
       * error out on it as it may happen for instance when selection
       * changes while in the process of removing a layer group.
       */
      if (! clay_item_is_attached (CLAY_ITEM (iter->data)))
        layers2 = g_list_remove (layers2, iter->data);
    }

  private = CLAY_IMAGE_GET_PRIVATE (image);

  floating_sel = clay_image_get_floating_selection (image);

  /*  Make sure the floating_sel always is the active layer  */
  if (floating_sel && (g_list_length (layers2) != 1 || layers2->data != floating_sel))
    {
      g_list_free (layers2);
      return;
    }

  selected_layers = clay_image_get_selected_layers (image);

  if (g_list_length (layers2) == g_list_length (selected_layers))
    {
      selection_changed = FALSE;
      for (iter = layers2; iter; iter = iter->next)
        {
          if (g_list_find (selected_layers, iter->data) == NULL)
            {
              selection_changed = TRUE;
              break;
            }
        }
    }

  if (selection_changed)
    {
      GList *layers3;

      /*  Don't cache selection info for the previous active layer  */
      if (selected_layers)
        clay_drawable_invalidate_boundary (CLAY_DRAWABLE (selected_layers->data));

      layers3 = g_list_copy (layers2);
      clay_item_tree_set_selected_items (private->layers, layers2);

      /* We cannot edit masks with multiple selected layers. */
      if (g_list_length (layers3) > 1)
        {
          for (iter = layers3; iter; iter = iter->next)
            {
              if (clay_layer_get_mask (iter->data))
                clay_layer_set_edit_mask (iter->data, FALSE);
            }
        }

      g_list_free (layers3);
    }
  else
    {
      g_list_free (layers2);
    }
}

void
clay_image_set_selected_channels (ClayImage *image,
                                  GList     *channels)
{
  ClayImagePrivate *private;
  GList            *iter;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  for (iter = channels; iter; iter = iter->next)
    {
      g_return_if_fail (CLAY_IS_CHANNEL (iter->data));
      g_return_if_fail (clay_item_is_attached (CLAY_ITEM (iter->data)) &&
                        clay_item_get_image (CLAY_ITEM (iter->data)) == image);
    }
  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  Not if there is a floating selection  */
  if (g_list_length (channels) > 0 && clay_image_get_floating_selection (image))
    return;

  clay_item_tree_set_selected_items (private->channels, g_list_copy (channels));
}

void
clay_image_set_selected_paths (ClayImage *image,
                               GList     *paths)
{
  ClayImagePrivate *private;
  GList            *iter;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  for (iter = paths; iter; iter = iter->next)
    {
      g_return_if_fail (CLAY_IS_PATH (iter->data));
      g_return_if_fail (clay_item_is_attached (CLAY_ITEM (iter->data)) &&
                        clay_item_get_image (CLAY_ITEM (iter->data)) == image);
    }

  private = CLAY_IMAGE_GET_PRIVATE (image);

  clay_item_tree_set_selected_items (private->paths, g_list_copy (paths));
}


/*  layer, channel, path by tattoo  */

ClayLayer *
clay_image_get_layer_by_tattoo (ClayImage  *image,
                                ClayTattoo  tattoo)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_layers (image));

  return CLAY_LAYER (clay_item_stack_get_item_by_tattoo (stack, tattoo));
}

ClayChannel *
clay_image_get_channel_by_tattoo (ClayImage  *image,
                                  ClayTattoo  tattoo)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_channels (image));

  return CLAY_CHANNEL (clay_item_stack_get_item_by_tattoo (stack, tattoo));
}

ClayPath *
clay_image_get_path_by_tattoo (ClayImage  *image,
                               ClayTattoo  tattoo)
{
  ClayItemStack *stack;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  stack = CLAY_ITEM_STACK (clay_image_get_paths (image));

  return CLAY_PATH (clay_item_stack_get_item_by_tattoo (stack, tattoo));
}


/*  layer, channel, path by name  */

ClayLayer *
clay_image_get_layer_by_name (ClayImage   *image,
                              const gchar *name)
{
  ClayItemTree *tree;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = clay_image_get_layer_tree (image);

  return CLAY_LAYER (clay_item_tree_get_item_by_name (tree, name));
}

ClayChannel *
clay_image_get_channel_by_name (ClayImage   *image,
                                const gchar *name)
{
  ClayItemTree *tree;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = clay_image_get_channel_tree (image);

  return CLAY_CHANNEL (clay_item_tree_get_item_by_name (tree, name));
}

ClayPath *
clay_image_get_path_by_name (ClayImage   *image,
                             const gchar *name)
{
  ClayItemTree *tree;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  tree = clay_image_get_path_tree (image);

  return CLAY_PATH (clay_item_tree_get_item_by_name (tree, name));
}


/*  items  */

gboolean
clay_image_reorder_item (ClayImage   *image,
                         ClayItem    *item,
                         ClayItem    *new_parent,
                         gint         new_index,
                         gboolean     push_undo,
                         const gchar *undo_desc)
{
  ClayItemTree *tree;
  gboolean      result;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (clay_item_get_image (item) == image, FALSE);

  tree = clay_item_get_tree (item);

  g_return_val_if_fail (tree != NULL, FALSE);

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = CLAY_ITEM_GET_CLASS (item)->reorder_desc;

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   undo_desc);
    }

  clay_image_freeze_bounding_box (image);

  clay_item_start_move (item, push_undo);

  /*  item and new_parent are type-checked in ClayItemTree
   */
  result = clay_item_tree_reorder_item (tree, item,
                                        new_parent, new_index,
                                        push_undo, undo_desc);

  clay_item_end_move (item, push_undo);

  clay_image_thaw_bounding_box (image);

  if (push_undo)
    clay_image_undo_group_end (image);

  return result;
}

gboolean
clay_image_raise_item (ClayImage *image,
                       ClayItem  *item,
                       GError    **error)
{
  gint index;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  index = clay_item_get_index (item);

  g_return_val_if_fail (index != -1, FALSE);

  if (index == 0)
    {
      g_set_error_literal (error,  CLAY_ERROR, CLAY_FAILED,
                           CLAY_ITEM_GET_CLASS (item)->raise_failed);
      return FALSE;
    }

  return clay_image_reorder_item (image, item,
                                  clay_item_get_parent (item), index - 1,
                                  TRUE, CLAY_ITEM_GET_CLASS (item)->raise_desc);
}

gboolean
clay_image_raise_item_to_top (ClayImage *image,
                              ClayItem  *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return clay_image_reorder_item (image, item,
                                  clay_item_get_parent (item), 0,
                                  TRUE, CLAY_ITEM_GET_CLASS (item)->raise_to_top_desc);
}

gboolean
clay_image_lower_item (ClayImage *image,
                       ClayItem  *item,
                       GError    **error)
{
  ClayContainer *container;
  gint           index;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  container = clay_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  index = clay_item_get_index (item);

  if (index == clay_container_get_n_children (container) - 1)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           CLAY_ITEM_GET_CLASS (item)->lower_failed);
      return FALSE;
    }

  return clay_image_reorder_item (image, item,
                                  clay_item_get_parent (item), index + 1,
                                  TRUE, CLAY_ITEM_GET_CLASS (item)->lower_desc);
}

gboolean
clay_image_lower_item_to_bottom (ClayImage *image,
                                  ClayItem *item)
{
  ClayContainer *container;
  gint           length;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  container = clay_item_get_container (item);

  g_return_val_if_fail (container != NULL, FALSE);

  length = clay_container_get_n_children (container);

  return clay_image_reorder_item (image, item,
                                  clay_item_get_parent (item), length - 1,
                                  TRUE, CLAY_ITEM_GET_CLASS (item)->lower_to_bottom_desc);
}


gboolean
clay_image_add_item (ClayImage *image,
                     ClayItem  *item,
                     ClayItem  *parent,
                     gint       position,
                     gboolean   push_undo)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  if (CLAY_IS_LAYER (item))
    {
      return clay_image_add_layer (image, CLAY_LAYER (item),
                                   CLAY_LAYER (parent),
                                   position, push_undo);
    }
  else if (CLAY_IS_CHANNEL (item))
    {
      return clay_image_add_channel (image, CLAY_CHANNEL (item),
                                     CLAY_CHANNEL (parent),
                                     position, push_undo);
    }
  else if (CLAY_IS_PATH (item))
    {
      return clay_image_add_path (image, CLAY_PATH (item),
                                  CLAY_PATH (parent),
                                  position, push_undo);
    }

  g_return_val_if_reached (FALSE);
}

void
clay_image_remove_item (ClayImage *image,
                        ClayItem  *item,
                        gboolean   push_undo,
                        GList     *new_selected)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  if (CLAY_IS_LAYER (item))
    {
      clay_image_remove_layer (image, CLAY_LAYER (item),
                               push_undo, new_selected);
    }
  else if (CLAY_IS_CHANNEL (item))
    {
      clay_image_remove_channel (image, CLAY_CHANNEL (item),
                                 push_undo, new_selected);
    }
  else if (CLAY_IS_PATH (item))
    {
      clay_image_remove_path (image, CLAY_PATH (item),
                              push_undo, new_selected);
    }
  else
    {
      g_return_if_reached ();
    }
}


/*  layers  */

gboolean
clay_image_add_layer (ClayImage *image,
                      ClayLayer *layer,
                      ClayLayer *parent,
                      gint       position,
                      gboolean   push_undo)
{
  ClayImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in ClayItemTree
   */
  if (! clay_item_tree_get_insert_pos (private->layers,
                                       (ClayItem *) layer,
                                       (ClayItem **) &parent,
                                       &position))
    return FALSE;

  clay_image_unset_default_new_layer_mode (image);

  /*  If there is a floating selection (and this isn't it!),
   *  make sure the insert position is greater than 0
   */
  if (parent == NULL && position == 0 &&
      clay_image_get_floating_selection (image))
    position = 1;

  old_has_alpha = clay_image_has_alpha (image);

  if (push_undo)
    clay_image_undo_push_layer_add (image, C_("undo-type", "Add Layer"),
                                    layer,
                                    clay_image_get_selected_layers (image));

  clay_item_tree_add_item (private->layers, CLAY_ITEM (layer),
                           CLAY_ITEM (parent), position);

  selected_layers = g_list_prepend (NULL, layer);
  clay_image_set_selected_layers (image, selected_layers);
  g_list_free (selected_layers);

  /*  If the layer is a floating selection, attach it to the drawable  */
  if (clay_layer_is_floating_sel (layer))
    clay_drawable_attach_floating_sel (clay_layer_get_floating_sel_drawable (layer),
                                       layer);

  /* If the layer is a vector layer, also add its path to the image */
  if (clay_item_is_vector_layer (CLAY_ITEM (layer)))
    {
      ClayPath *path = clay_vector_layer_get_path (CLAY_VECTOR_LAYER (layer));

      if (path                                         &&
          (! clay_item_is_attached (CLAY_ITEM (path))) &&
          clay_item_get_image (CLAY_ITEM (path)) == image)
        {
          clay_image_add_path (image, path, NULL, -1, FALSE);
        }

      if (! clay_item_is_rasterized (CLAY_ITEM (layer)))
        clay_vector_layer_refresh (CLAY_VECTOR_LAYER (layer));
    }

  if (old_has_alpha != clay_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  return TRUE;
}

void
clay_image_remove_layer (ClayImage *image,
                         ClayLayer *layer,
                         gboolean   push_undo,
                         GList     *new_selected)
{
  ClayImagePrivate *private;
  GList            *selected_layers;
  gboolean          old_has_alpha;
  const gchar      *undo_desc;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (layer)));
  g_return_if_fail (clay_item_get_image (CLAY_ITEM (layer)) == image);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  clay_image_unset_default_new_layer_mode (image);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Layer"));

  clay_item_start_move (CLAY_ITEM (layer), push_undo);

  if (clay_drawable_get_floating_sel (CLAY_DRAWABLE (layer)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the layer "
                     "had a floating selection. Please report this at "
                     "https://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      clay_image_remove_layer (image,
                               clay_drawable_get_floating_sel (CLAY_DRAWABLE (layer)),
                               TRUE, NULL);
    }

  selected_layers = g_list_copy (clay_image_get_selected_layers (image));

  old_has_alpha = clay_image_has_alpha (image);

  if (clay_layer_is_floating_sel (layer))
    {
      undo_desc = C_("undo-type", "Remove Floating Selection");

      clay_drawable_detach_floating_sel (clay_layer_get_floating_sel_drawable (layer));
    }
  else
    {
      undo_desc = C_("undo-type", "Remove Layer");
    }

  if (push_undo)
    {
      clay_image_rec_filter_remove_undo (image, CLAY_DRAWABLE (layer));
      clay_image_undo_push_layer_remove (image, undo_desc, layer,
                                         clay_layer_get_parent (layer),
                                         clay_item_get_index (CLAY_ITEM (layer)),
                                         selected_layers);

      if (clay_layer_get_mask (layer) != NULL)
        {
          ClayLayerMask *mask = clay_layer_get_mask (layer);

          clay_image_rec_filter_remove_undo (image, CLAY_DRAWABLE (mask));
        }
    }

  g_object_ref (layer);

  /*  Make sure we're not caching any old selection info  */
  if (g_list_find (selected_layers, layer))
    clay_drawable_invalidate_boundary (CLAY_DRAWABLE (layer));

  /* Remove layer and its children from the MRU layer stack. */
  clay_image_remove_from_layer_stack (image, layer);

  new_selected = clay_item_tree_remove_item (private->layers,
                                             CLAY_ITEM (layer),
                                             new_selected);

  if (clay_layer_is_floating_sel (layer))
    {
      /*  If this was the floating selection, activate the underlying drawable
       */
      floating_sel_activate_drawable (layer);
    }
  else if (selected_layers &&
           (g_list_find (selected_layers, layer) ||
            g_list_find_custom (selected_layers, layer,
                                (GCompareFunc) clay_image_selected_is_descendant)))
    {
      clay_image_set_selected_layers (image, new_selected);
    }

  clay_item_end_move (CLAY_ITEM (layer), push_undo);

  g_object_unref (layer);
  g_list_free (selected_layers);
  if (new_selected)
    g_list_free (new_selected);

  if (old_has_alpha != clay_image_has_alpha (image))
    private->flush_accum.alpha_changed = TRUE;

  if (push_undo)
    clay_image_undo_group_end (image);
}

void
clay_image_add_layers (ClayImage   *image,
                       GList       *layers,
                       ClayLayer   *parent,
                       gint         position,
                       gint         x,
                       gint         y,
                       gint         width,
                       gint         height,
                       const gchar *undo_desc)
{
  ClayImagePrivate *private;
  GList            *list;
  gint              layers_x      = G_MAXINT;
  gint              layers_y      = G_MAXINT;
  gint              layers_width  = 0;
  gint              layers_height = 0;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (layers != NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in ClayItemTree
   */
  if (! clay_item_tree_get_insert_pos (private->layers,
                                       (ClayItem *) layers->data,
                                       (ClayItem **) &parent,
                                       &position))
    return;

  for (list = layers; list; list = g_list_next (list))
    {
      ClayItem *item = CLAY_ITEM (list->data);
      gint      off_x, off_y;

      clay_item_get_offset (item, &off_x, &off_y);

      layers_x = MIN (layers_x, off_x);
      layers_y = MIN (layers_y, off_y);

      layers_width  = MAX (layers_width,
                           off_x + clay_item_get_width (item)  - layers_x);
      layers_height = MAX (layers_height,
                           off_y + clay_item_get_height (item) - layers_y);
    }

  offset_x = x + (width  - layers_width)  / 2 - layers_x;
  offset_y = y + (height - layers_height) / 2 - layers_y;

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_ADD, undo_desc);

  for (list = layers; list; list = g_list_next (list))
    {
      ClayItem *new_item = CLAY_ITEM (list->data);

      clay_item_translate (new_item, offset_x, offset_y, FALSE);

      clay_image_add_layer (image, CLAY_LAYER (new_item),
                            parent, position, TRUE);
      position++;
    }

  if (layers)
    clay_image_set_selected_layers (image, layers);

  clay_image_undo_group_end (image);
}


/*  channels  */

gboolean
clay_image_add_channel (ClayImage   *image,
                        ClayChannel *channel,
                        ClayChannel *parent,
                        gint         position,
                        gboolean     push_undo)
{
  ClayImagePrivate *private;
  GList            *channels;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in ClayItemTree
   */
  if (! clay_item_tree_get_insert_pos (private->channels,
                                       (ClayItem *) channel,
                                       (ClayItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    clay_image_undo_push_channel_add (image, C_("undo-type", "Add Channel"),
                                      channel,
                                      clay_image_get_selected_channels (image));

  clay_item_tree_add_item (private->channels, CLAY_ITEM (channel),
                           CLAY_ITEM (parent), position);

  channels = g_list_prepend (NULL, channel);
  clay_image_set_selected_channels (image, channels);
  g_list_free (channels);

  return TRUE;
}

void
clay_image_remove_channel (ClayImage   *image,
                           ClayChannel *channel,
                           gboolean     push_undo,
                           GList       *new_selected)
{
  ClayImagePrivate *private;
  GList            *selected_channels;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));
  g_return_if_fail (clay_item_get_image (CLAY_ITEM (channel)) == image);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Channel"));

  clay_item_start_move (CLAY_ITEM (channel), push_undo);

  if (clay_drawable_get_floating_sel (CLAY_DRAWABLE (channel)))
    {
      if (! push_undo)
        {
          g_warning ("%s() was called from an undo function while the channel "
                     "had a floating selection. Please report this at "
                     "https://www.gimp.org/bugs/", G_STRFUNC);
          return;
        }

      clay_image_remove_layer (image,
                               clay_drawable_get_floating_sel (CLAY_DRAWABLE (channel)),
                               TRUE, NULL);
    }

  private = CLAY_IMAGE_GET_PRIVATE (image);

  selected_channels = clay_image_get_selected_channels (image);
  selected_channels = g_list_copy (selected_channels);

  if (push_undo)
    {
      clay_image_rec_filter_remove_undo (image, CLAY_DRAWABLE (channel));
      clay_image_undo_push_channel_remove (image,
                                           C_("undo-type", "Remove Channel"),
                                           channel,
                                           clay_channel_get_parent (channel),
                                           clay_item_get_index (CLAY_ITEM (channel)),
                                           selected_channels);
    }

  g_object_ref (channel);

  new_selected = clay_item_tree_remove_item (private->channels,
                                             CLAY_ITEM (channel),
                                             new_selected);

  if (selected_channels &&
      (g_list_find (selected_channels, channel) ||
       g_list_find_custom (selected_channels, channel,
                           (GCompareFunc) clay_image_selected_is_descendant)))
    {
      if (new_selected)
        clay_image_set_selected_channels (image, new_selected);
      else
        clay_image_unset_selected_channels (image);
    }

  g_list_free (selected_channels);

  clay_item_end_move (CLAY_ITEM (channel), push_undo);

  g_object_unref (channel);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    clay_image_undo_group_end (image);
}


/*  paths  */

gboolean
clay_image_add_path (ClayImage   *image,
                     ClayPath    *path,
                     ClayPath    *parent,
                     gint         position,
                     gboolean     push_undo)
{
  ClayImagePrivate *private;
  GList            *list = NULL;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  item and parent are type-checked in ClayItemTree
   */
  if (! clay_item_tree_get_insert_pos (private->paths,
                                       (ClayItem *) path,
                                       (ClayItem **) &parent,
                                       &position))
    return FALSE;

  if (push_undo)
    clay_image_undo_push_path_add (image, C_("undo-type", "Add Path"),
                                   path,
                                   clay_image_get_selected_paths (image));

  clay_item_tree_add_item (private->paths, CLAY_ITEM (path),
                           CLAY_ITEM (parent), position);

  if (path != NULL)
    list = g_list_prepend (NULL, path);

  clay_image_set_selected_paths (image, list);

  g_list_free (list);

  return TRUE;
}

void
clay_image_remove_path (ClayImage   *image,
                        ClayPath    *path,
                        gboolean     push_undo,
                        GList       *new_selected)
{
  ClayImagePrivate *private;
  GList            *selected_path;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_PATH (path));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (path)));
  g_return_if_fail (clay_item_get_image (CLAY_ITEM (path)) == image);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                 C_("undo-type", "Remove Path"));

  clay_item_start_move (CLAY_ITEM (path), push_undo);

  selected_path = clay_image_get_selected_paths (image);
  selected_path = g_list_copy (selected_path);

  if (push_undo)
    clay_image_undo_push_path_remove (image, C_("undo-type", "Remove Path"), path,
                                      clay_path_get_parent (path),
                                      clay_item_get_index (CLAY_ITEM (path)),
                                      selected_path);

  g_object_ref (path);

  new_selected = clay_item_tree_remove_item (private->paths,
                                             CLAY_ITEM (path),
                                             new_selected);

  if (selected_path &&
      (g_list_find (selected_path, path) ||
       g_list_find_custom (selected_path, path,
                           (GCompareFunc) clay_image_selected_is_descendant)))
    {
      clay_image_set_selected_paths (image, new_selected);
    }

  g_list_free (selected_path);

  clay_item_end_move (CLAY_ITEM (path), push_undo);

  g_object_unref (path);
  if (new_selected)
    g_list_free (new_selected);

  if (push_undo)
    clay_image_undo_group_end (image);
}


/*  item sets  */

/*
 * clay_image_store_item_set:
 * @image:
 * @set: (transfer full): a set of items which @images takes ownership
 *                        of.
 *
 * Store a new set of @layers.
 * If a set with the same name and type existed, this call will silently
 * replace it with the new set of layers.
 */
void
clay_image_store_item_set (ClayImage    *image,
                           ClayItemList *set)
{
  ClayImagePrivate  *private;
  GList            **stored_sets;
  GList             *iter;
  guint              signal;
  GType              item_type;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM_LIST (set));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  signal    = clay_image_signals[ITEM_SETS_CHANGED];
  item_type = clay_item_list_get_item_type (set);
  if (item_type == CLAY_TYPE_LAYER)
    stored_sets = &private->stored_layer_sets;
  else if (item_type == CLAY_TYPE_CHANNEL)
    stored_sets = &private->stored_channel_sets;
  else if (item_type == CLAY_TYPE_PATH)
    stored_sets = &private->stored_path_sets;
  else
    g_return_if_reached ();

  for (iter = *stored_sets; iter; iter = iter->next)
    {
      gboolean         is_pattern;
      gboolean         is_pattern2;
      ClaySelectMethod pattern_syntax;
      ClaySelectMethod pattern_syntax2;

      is_pattern  = clay_item_list_is_pattern (iter->data, &pattern_syntax);
      is_pattern2 = clay_item_list_is_pattern (set, &pattern_syntax2);

      /* Remove a previous item set of same type and name. */
      if (is_pattern == is_pattern2 && (! is_pattern || pattern_syntax == pattern_syntax2) &&
          g_strcmp0 (clay_object_get_name (iter->data), clay_object_get_name (set)) == 0)
        break;
    }
  if (iter)
    {
      g_object_unref (iter->data);
      *stored_sets = g_list_delete_link (*stored_sets, iter);
    }

  *stored_sets = g_list_prepend (*stored_sets, set);
  g_signal_emit (image, signal, 0, item_type);
}

/*
 * @clay_image_unlink_item_set:
 * @image:
 * @link_name:
 *
 * Remove the set of layers named @link_name.
 *
 * Returns: %TRUE if the set was removed, %FALSE if no sets with this
 *          name existed.
 */
gboolean
clay_image_unlink_item_set (ClayImage    *image,
                            ClayItemList *set)
{
  ClayImagePrivate  *private;
  GList             *found;
  GList            **stored_sets;
  guint              signal;
  GType              item_type;
  gboolean           success;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  signal    = clay_image_signals[ITEM_SETS_CHANGED];
  item_type = clay_item_list_get_item_type (set);
  if (item_type == CLAY_TYPE_LAYER)
    stored_sets = &private->stored_layer_sets;
  else if (item_type == CLAY_TYPE_CHANNEL)
    stored_sets = &private->stored_channel_sets;
  else if (item_type == CLAY_TYPE_PATH)
    stored_sets = &private->stored_path_sets;
  else
    g_return_val_if_reached (FALSE);

  found = g_list_find (*stored_sets, set);
  success = (found != NULL);
  if (success)
    {
      *stored_sets = g_list_delete_link (*stored_sets, found);
      g_object_unref (set);
      g_signal_emit (image, signal, 0, item_type);
    }

  return success;
}

/*
 * @clay_image_get_stored_item_sets:
 * @image:
 * @item_type:
 *
 * Returns: (transfer none): the list of all the layer sets (which you
 *          should not modify). Order of items is relevant.
 */
GList *
clay_image_get_stored_item_sets (ClayImage *image,
                                 GType      item_type)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (item_type == CLAY_TYPE_LAYER)
    return private->stored_layer_sets;
  else if (item_type == CLAY_TYPE_CHANNEL)
    return private->stored_channel_sets;
  else if (item_type == CLAY_TYPE_PATH)
    return private->stored_path_sets;

  g_return_val_if_reached (FALSE);
}

/*
 * @clay_image_select_item_set:
 * @image:
 * @set:
 *
 * Replace currently selected layers in @image with the layers belonging
 * to @set.
 */
void
clay_image_select_item_set (ClayImage    *image,
                            ClayItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM_LIST (set));

  items = clay_item_list_get_items (set, &error);

  if (! error)
    {
      GType item_type = clay_item_list_get_item_type (set);

      if (item_type == CLAY_TYPE_LAYER)
        clay_image_set_selected_layers (image, items);
      else if (item_type == CLAY_TYPE_CHANNEL)
        clay_image_set_selected_channels (image, items);
      else if (item_type == CLAY_TYPE_PATH)
        clay_image_set_selected_paths (image, items);
      else
        g_return_if_reached ();
    }

  g_list_free (items);
  g_clear_error (&error);
}

/*
 * @clay_image_add_item_set:
 * @image:
 * @set:
 *
 * Add the layers belonging to @set to the items currently selected in
 * @image.
 */
void
clay_image_add_item_set (ClayImage    *image,
                         ClayItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM_LIST (set));

  items = clay_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = clay_item_list_get_item_type (set);

      if (item_type == CLAY_TYPE_LAYER)
        selected = clay_image_get_selected_layers (image);
      else if (item_type == CLAY_TYPE_CHANNEL)
        selected = clay_image_get_selected_channels (image);
      else if (item_type == CLAY_TYPE_PATH)
        selected = clay_image_get_selected_paths (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          if (! g_list_find (selected, iter->data))
            selected = g_list_prepend (selected, iter->data);
        }

      if (item_type == CLAY_TYPE_LAYER)
        clay_image_set_selected_layers (image, selected);
      else if (item_type == CLAY_TYPE_CHANNEL)
        clay_image_set_selected_channels (image, selected);
      else if (item_type == CLAY_TYPE_PATH)
        clay_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @clay_image_remove_item_set:
 * @image:
 * @set:
 *
 * Remove the layers belonging to the set named @link_name (which must
 * exist) from the layers currently selected in @image.
 *
 * Returns: %TRUE if the selection change is done (even if it turned out
 *          selected layers stay the same), %FALSE if no sets with this
 *          name existed.
 */
void
clay_image_remove_item_set (ClayImage    *image,
                            ClayItemList *set)
{
  GList  *items;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM_LIST (set));

  items = clay_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *iter;
      GType  item_type = clay_item_list_get_item_type (set);

      if (item_type == CLAY_TYPE_LAYER)
        selected = clay_image_get_selected_layers (image);
      else if (item_type == CLAY_TYPE_CHANNEL)
        selected = clay_image_get_selected_channels (image);
      else if (item_type == CLAY_TYPE_PATH)
        selected = clay_image_get_selected_paths (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);
      for (iter = items; iter; iter = iter->next)
        {
          GList *remove;

          if ((remove = g_list_find (selected, iter->data)))
            selected = g_list_delete_link (selected, remove);
        }

      if (item_type == CLAY_TYPE_LAYER)
        clay_image_set_selected_layers (image, selected);
      else if (item_type == CLAY_TYPE_CHANNEL)
        clay_image_set_selected_channels (image, selected);
      else if (item_type == CLAY_TYPE_PATH)
        clay_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}

/*
 * @clay_image_intersect_item_set:
 * @image:
 * @set:
 *
 * Remove any layers from the layers currently selected in @image if
 * they don't also belong to the set named @link_name (which must
 * exist).
 *
 * Returns: %TRUE if the selection change is done (even if it turned out
 *          selected layers stay the same), %FALSE if no sets with this
 *          name existed.
 */
void
clay_image_intersect_item_set (ClayImage    *image,
                               ClayItemList *set)
{
  GList *items;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM_LIST (set));

  items = clay_item_list_get_items (set, &error);

  if (! error)
    {
      GList *selected;
      GList *remove = NULL;
      GList *iter;
      GType  item_type = clay_item_list_get_item_type (set);

      if (item_type == CLAY_TYPE_LAYER)
        selected = clay_image_get_selected_layers (image);
      else if (item_type == CLAY_TYPE_CHANNEL)
        selected = clay_image_get_selected_channels (image);
      else if (item_type == CLAY_TYPE_PATH)
        selected = clay_image_get_selected_paths (image);
      else
        g_return_if_reached ();

      selected = g_list_copy (selected);

      /* Remove items in selected but not in items. */
      for (iter = selected; iter; iter = iter->next)
        {
          if (! g_list_find (items, iter->data))
            remove = g_list_prepend (remove, iter);
        }
      for (iter = remove; iter; iter = iter->next)
        selected = g_list_delete_link (selected, iter->data);
      g_list_free (remove);

      /* Finally select the intersection. */
      if (item_type == CLAY_TYPE_LAYER)
        clay_image_set_selected_layers (image, selected);
      else if (item_type == CLAY_TYPE_CHANNEL)
        clay_image_set_selected_channels (image, selected);
      else if (item_type == CLAY_TYPE_PATH)
        clay_image_set_selected_paths (image, items);

      g_list_free (selected);
    }
  g_clear_error (&error);
}


/*  hidden items  */

/* Sometimes you want to create a channel or other types of drawables to
 * work on them without adding them to the public trees and to be
 * visible in the GUI. This is when you create hidden items. No undo
 * steps will ever be created either for any processing on these items.
 *
 * Note that you are expected to manage the hidden items properly. In
 * particular, once you are done with them, remove them with
 * clay_image_remove_hidden_item() and free them.
 * @image is not assuming ownership of @item.
 */
gboolean
clay_image_add_hidden_item (ClayImage *image,
                            ClayItem  *item)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (! clay_item_is_attached (item), FALSE);
  g_return_val_if_fail (clay_item_get_image (item) == image, FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->hidden_items = g_list_prepend (private->hidden_items, item);

  return TRUE;
}

void
clay_image_remove_hidden_item (ClayImage *image,
                               ClayItem  *item)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_get_image (item) == image);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (g_list_find (private->hidden_items, item) != NULL);

  private->hidden_items = g_list_remove (private->hidden_items, item);
}

gboolean
clay_image_is_hidden_item (ClayImage *image,
                           ClayItem  *item)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (clay_item_get_image (item) == image, FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return (g_list_find (private->hidden_items, item) != NULL);
}

gboolean
clay_image_coords_in_active_pickable (ClayImage        *image,
                                      const ClayCoords *coords,
                                      gboolean          show_all,
                                      gboolean          sample_merged,
                                      gboolean          selected_only)
{
  gint     x, y;
  gboolean in_pickable = FALSE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  x = floor (coords->x);
  y = floor (coords->y);

  if (sample_merged)
    {
      if (show_all || (x >= 0 && x < clay_image_get_width  (image) &&
                       y >= 0 && y < clay_image_get_height (image)))
        {
          in_pickable = TRUE;
        }
    }
  else
    {
      GList *drawables = clay_image_get_selected_drawables (image);
      GList *iter;

      for (iter = drawables; iter; iter = iter->next)
        {
          ClayItem *item = iter->data;
          gint      off_x, off_y;
          gint      d_x, d_y;

          clay_item_get_offset (item, &off_x, &off_y);

          d_x = x - off_x;
          d_y = y - off_y;

          if (d_x >= 0 && d_x < clay_item_get_width  (item) &&
              d_y >= 0 && d_y < clay_item_get_height (item))
            {
              in_pickable = TRUE;
              break;
            }
        }
      g_list_free (drawables);
    }

  if (in_pickable && selected_only)
    {
      ClayChannel *selection = clay_image_get_mask (image);

      if (! clay_channel_is_empty (selection) &&
          ! clay_pickable_get_opacity_at (CLAY_PICKABLE (selection),
                                          x, y))
        {
          in_pickable = FALSE;
        }
    }

  return in_pickable;
}

void
clay_image_invalidate_previews (ClayImage *image)
{
  ClayItemStack *layers;
  ClayItemStack *channels;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  layers   = CLAY_ITEM_STACK (clay_image_get_layers (image));
  channels = CLAY_ITEM_STACK (clay_image_get_channels (image));

  clay_item_stack_invalidate_previews (layers);
  clay_item_stack_invalidate_previews (channels);
}

/* Sets the image into a "converting" state, which is there to warn other code
 * (such as shell render code) that the image properties might be in an
 * inconsistent state. For instance when converting to another precision with
 * clay_image_convert_precision(), the babl format may be updated first, and the
 * profile later, after all drawables are converted. Rendering the image
 * in-between would at best render broken previews (at worst, crash, e.g.
 * because we depend on allocated data which might have become too small).
 */
void
clay_image_set_converting (ClayImage *image,
                           gboolean   converting)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_object_set (image,
                "converting", converting,
                NULL);
}

gboolean
clay_image_get_converting (ClayImage *image)
{
  ClayImagePrivate *private;
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return private->converting;
}

void
clay_image_set_from_command_line (ClayImage *image,
                                  gboolean   from_command_line)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->from_command_line = from_command_line;
}

gboolean
clay_image_get_from_command_line (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return private->from_command_line;
}
