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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clayasync.h"
#include "core/claycancelable.h"
#include "core/claycontainer.h"
#include "core/claydrawable-bucket-fill.h"
#include "core/claydrawable-edit.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/clayfilloptions.h"
#include "core/clayimage.h"
#include "core/clayimageproxy.h"
#include "core/clayitem.h"
#include "core/claylineart.h"
#include "core/claylinklayer.h"
#include "core/claypickable.h"
#include "core/claypickable-contiguous-region.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"
#include "core/claytoolinfo.h"
#include "core/claywaitable.h"

#include "gegl/clay-gegl-nodes.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "path/clayvectorlayer.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "claybucketfilloptions.h"
#include "claybucketfilltool.h"
#include "claycoloroptions.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


struct _ClayBucketFillToolPrivate
{
  ClayLineArt        *line_art;
  ClayImage          *line_art_image;
  ClayDisplayShell   *line_art_shell;
  GList              *line_art_bindings;

  /* For preview */
  GeglNode           *graph;
  GeglNode           *fill_node;
  GeglNode           *offset_node;

  GeglBuffer         *fill_mask;

  ClayDrawableFilter *filter;

  /* Temp property save */
  ClayBucketFillMode  fill_mode;
  ClayBucketFillArea  fill_area;
};


/*  local function prototypes  */

static void     clay_bucket_fill_tool_constructed      (GObject               *object);
static void     clay_bucket_fill_tool_finalize         (GObject               *object);

static void     clay_bucket_fill_tool_button_press     (ClayTool              *tool,
                                                        const ClayCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        ClayButtonPressType    press_type,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_motion           (ClayTool              *tool,
                                                        const ClayCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_button_release   (ClayTool              *tool,
                                                        const ClayCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        ClayButtonReleaseType  release_type,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_modifier_key     (ClayTool              *tool,
                                                        GdkModifierType        key,
                                                        gboolean               press,
                                                        GdkModifierType        state,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_cursor_update    (ClayTool              *tool,
                                                        const ClayCoords      *coords,
                                                        GdkModifierType        state,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_options_notify   (ClayTool              *tool,
                                                        ClayToolOptions       *options,
                                                        const GParamSpec      *pspec);

static void clay_bucket_fill_tool_line_art_computing_start (ClayBucketFillTool *tool);
static void clay_bucket_fill_tool_line_art_computing_end   (ClayBucketFillTool *tool);

static gboolean clay_bucket_fill_tool_coords_in_active_pickable
                                                       (ClayBucketFillTool    *tool,
                                                        ClayDisplay           *display,
                                                        const ClayCoords      *coords);

static void     clay_bucket_fill_tool_start            (ClayBucketFillTool    *tool,
                                                        const ClayCoords      *coords,
                                                        ClayDisplay           *display);
static void     clay_bucket_fill_tool_preview          (ClayBucketFillTool    *tool,
                                                        const ClayCoords      *coords,
                                                        ClayDisplay           *display,
                                                        ClayFillOptions       *fill_options);
static void     clay_bucket_fill_tool_commit           (ClayBucketFillTool    *tool);
static void     clay_bucket_fill_tool_halt             (ClayBucketFillTool    *tool);
static void     clay_bucket_fill_tool_filter_flush     (ClayDrawableFilter    *filter,
                                                        ClayTool              *tool);
static void     clay_bucket_fill_tool_create_graph     (ClayBucketFillTool    *tool);

static void     clay_bucket_fill_tool_reset_line_art   (ClayBucketFillTool    *tool);


G_DEFINE_TYPE_WITH_PRIVATE (ClayBucketFillTool, clay_bucket_fill_tool,
                            CLAY_TYPE_COLOR_TOOL)

#define parent_class clay_bucket_fill_tool_parent_class


void
clay_bucket_fill_tool_register (ClayToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (CLAY_TYPE_BUCKET_FILL_TOOL,
                CLAY_TYPE_BUCKET_FILL_OPTIONS,
                clay_bucket_fill_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                CLAY_CONTEXT_PROP_MASK_OPACITY    |
                CLAY_CONTEXT_PROP_MASK_PAINT_MODE |
                CLAY_CONTEXT_PROP_MASK_PATTERN,
                "clay-bucket-fill-tool",
                _("Bucket Fill"),
                _("Bucket Fill Tool: Fill selected area with a color or pattern"),
                N_("_Bucket Fill"), "<shift>B",
                NULL, CLAY_HELP_TOOL_BUCKET_FILL,
                CLAY_ICON_TOOL_BUCKET_FILL,
                data);
}

static void
clay_bucket_fill_tool_class_init (ClayBucketFillToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->constructed  = clay_bucket_fill_tool_constructed;
  object_class->finalize     = clay_bucket_fill_tool_finalize;

  tool_class->button_press   = clay_bucket_fill_tool_button_press;
  tool_class->motion         = clay_bucket_fill_tool_motion;
  tool_class->button_release = clay_bucket_fill_tool_button_release;
  tool_class->modifier_key   = clay_bucket_fill_tool_modifier_key;
  tool_class->cursor_update  = clay_bucket_fill_tool_cursor_update;
  tool_class->options_notify = clay_bucket_fill_tool_options_notify;
}

static void
clay_bucket_fill_tool_init (ClayBucketFillTool *bucket_fill_tool)
{
  ClayTool *tool = CLAY_TOOL (bucket_fill_tool);

  clay_tool_control_set_scroll_lock     (tool->control, TRUE);
  clay_tool_control_set_wants_click     (tool->control, TRUE);
  clay_tool_control_set_tool_cursor     (tool->control,
                                         CLAY_TOOL_CURSOR_BUCKET_FILL);
  clay_tool_control_set_action_opacity  (tool->control,
                                         "context-opacity-set");
  clay_tool_control_set_action_object_1 (tool->control,
                                         "context-pattern-select-set");

  bucket_fill_tool->priv =
    clay_bucket_fill_tool_get_instance_private (bucket_fill_tool);
}

static void
clay_bucket_fill_tool_constructed (GObject *object)
{
  ClayTool              *tool        = CLAY_TOOL (object);
  ClayBucketFillTool    *bucket_tool = CLAY_BUCKET_FILL_TOOL (object);
  ClayBucketFillOptions *options     = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Clay                  *clay        = CLAY_CONTEXT (options)->clay;
  ClayContext           *context     = clay_get_user_context (clay);
  GList                 *bindings    = NULL;
  GBinding              *binding;
  ClayLineArt           *line_art;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_tool_control_set_motion_mode (tool->control,
                                     options->fill_area == CLAY_BUCKET_FILL_LINE_ART ?
                                     CLAY_MOTION_MODE_EXACT : CLAY_MOTION_MODE_COMPRESS);

  line_art = clay_context_take_line_art (context);
  binding = g_object_bind_property (options,  "fill-transparent",
                                    line_art, "select-transparent",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-threshold",
                                    line_art, "threshold",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-grow",
                                    line_art, "max-grow",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-automatic-closure",
                                    line_art, "automatic-closure",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-gap-length",
                                    line_art, "spline-max-length",
                                    G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-gap-length",
                                    line_art, "segment-max-length",
                                    G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT);
  bindings = g_list_prepend (bindings, binding);
  g_signal_connect_swapped (line_art, "computing-start",
                            G_CALLBACK (clay_bucket_fill_tool_line_art_computing_start),
                            tool);
  g_signal_connect_swapped (line_art, "computing-end",
                            G_CALLBACK (clay_bucket_fill_tool_line_art_computing_end),
                            tool);
  clay_line_art_bind_gap_length (line_art, TRUE);
  bucket_tool->priv->line_art = line_art;
  bucket_tool->priv->line_art_bindings = bindings;

  clay_bucket_fill_tool_reset_line_art (bucket_tool);

  g_signal_connect_swapped (options, "notify::line-art-source",
                            G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                            tool);
  g_signal_connect_swapped (context, "display-changed",
                            G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                            tool);

  CLAY_COLOR_TOOL (tool)->pick_target =
    (options->fill_mode == CLAY_BUCKET_FILL_BG) ?
    CLAY_COLOR_PICK_TARGET_BACKGROUND : CLAY_COLOR_PICK_TARGET_FOREGROUND;
}

static void
clay_bucket_fill_tool_finalize (GObject *object)
{
  ClayBucketFillTool    *tool     = CLAY_BUCKET_FILL_TOOL (object);
  ClayBucketFillOptions *options  = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Clay                  *clay     = CLAY_CONTEXT (options)->clay;
  ClayContext           *context  = clay_get_user_context (clay);

  if (tool->priv->line_art_image)
    {
      g_signal_handlers_disconnect_by_data (clay_image_get_layers (tool->priv->line_art_image), tool);
      g_signal_handlers_disconnect_by_data (tool->priv->line_art_image, tool);
      tool->priv->line_art_image = NULL;
    }
  if (tool->priv->line_art_shell)
    {
      g_signal_handlers_disconnect_by_func (
        tool->priv->line_art_shell,
        clay_bucket_fill_tool_reset_line_art,
        tool);
      tool->priv->line_art_shell = NULL;
    }

  /* We don't free the line art object, but gives temporary ownership to
   * the user context which will free it if a timer runs out.
   *
   * This way, we allow people to not suffer a new computational delay
   * if for instance they just needed to switch tools for a few seconds
   * while the source layer stays the same.
   */
  g_signal_handlers_disconnect_by_data (tool->priv->line_art, tool);
  g_list_free_full (tool->priv->line_art_bindings,
                    (GDestroyNotify) g_binding_unbind);
  clay_context_store_line_art (context, tool->priv->line_art);
  tool->priv->line_art = NULL;

  g_signal_handlers_disconnect_by_data (options, tool);
  g_signal_handlers_disconnect_by_data (context, tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_bucket_fill_tool_coords_in_active_pickable (ClayBucketFillTool *tool,
                                                 ClayDisplay        *display,
                                                 const ClayCoords   *coords)
{
  ClayBucketFillOptions *options       = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell      *shell         = clay_display_get_shell (display);
  ClayImage             *image         = clay_display_get_image (display);
  gboolean               sample_merged = FALSE;

  switch (options->fill_area)
    {
    case CLAY_BUCKET_FILL_SELECTION:
      break;

    case CLAY_BUCKET_FILL_SIMILAR_COLORS:
      sample_merged = options->sample_merged;
      break;

    case CLAY_BUCKET_FILL_LINE_ART:
      sample_merged = options->line_art_source ==
                      CLAY_LINE_ART_SOURCE_SAMPLE_MERGED;
      break;
    }

  return clay_image_coords_in_active_pickable (image, coords,
                                               shell->show_all, sample_merged,
                                               TRUE);
}

static void
clay_bucket_fill_tool_start (ClayBucketFillTool *tool,
                             const ClayCoords   *coords,
                             ClayDisplay        *display)
{
  ClayBucketFillOptions *options   = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayContext           *context   = CLAY_CONTEXT (options);
  ClayImage             *image     = clay_display_get_image (display);
  GList                 *drawables = clay_image_get_selected_drawables (image);

  g_return_if_fail (! tool->priv->filter);
  g_return_if_fail (g_list_length (drawables) == 1);

  clay_line_art_freeze (tool->priv->line_art);

  CLAY_TOOL (tool)->display  = display;
  g_list_free (CLAY_TOOL (tool)->drawables);
  CLAY_TOOL (tool)->drawables = drawables;

  clay_bucket_fill_tool_create_graph (tool);

  tool->priv->filter = clay_drawable_filter_new (drawables->data, _("Bucket fill"),
                                                 tool->priv->graph,
                                                 CLAY_ICON_TOOL_BUCKET_FILL);
  clay_drawable_filter_set_temporary (tool->priv->filter, TRUE);

  clay_drawable_filter_set_region (tool->priv->filter,
                                   CLAY_FILTER_REGION_DRAWABLE);

  /* We only set these here, and don't need to update it since we assume
   * the settings can't change while the fill started.
   */
  clay_drawable_filter_set_mode (tool->priv->filter,
                                 clay_context_get_paint_mode (context),
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 clay_layer_mode_get_paint_composite_mode (clay_context_get_paint_mode (context)));
  clay_drawable_filter_set_opacity (tool->priv->filter,
                                    clay_context_get_opacity (context));

  g_signal_connect (tool->priv->filter, "flush",
                    G_CALLBACK (clay_bucket_fill_tool_filter_flush),
                    tool);
}

static void
clay_bucket_fill_tool_preview (ClayBucketFillTool *tool,
                               const ClayCoords   *coords,
                               ClayDisplay        *display,
                               ClayFillOptions    *fill_options)
{
  ClayBucketFillOptions *options   = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell      *shell     = clay_display_get_shell (display);
  ClayImage             *image     = clay_display_get_image (display);
  GList                 *drawables = clay_image_get_selected_drawables (image);
  ClayDrawable          *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  if (tool->priv->filter)
    {
      GeglBuffer *fill     = NULL;
      gdouble     x        = coords->x;
      gdouble     y        = coords->y;

      if (options->fill_area == CLAY_BUCKET_FILL_SIMILAR_COLORS)
        {
          if (! options->sample_merged)
            {
              gint off_x, off_y;

              clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

              x -= (gdouble) off_x;
              y -= (gdouble) off_y;
            }
          fill = clay_drawable_get_bucket_fill_buffer (drawable,
                                                       fill_options,
                                                       options->fill_transparent,
                                                       options->fill_criterion,
                                                       options->threshold / 255.0,
                                                       shell->show_all,
                                                       options->sample_merged,
                                                       options->diagonal_neighbors,
                                                       x, y,
                                                       &tool->priv->fill_mask,
                                                       &x, &y, NULL, NULL);
        }
      else
        {
          gint source_off_x = 0;
          gint source_off_y = 0;

          if (options->line_art_source != CLAY_LINE_ART_SOURCE_SAMPLE_MERGED)
            {
              ClayPickable *input;

              input = clay_line_art_get_input (tool->priv->line_art);
              g_return_if_fail (CLAY_IS_ITEM (input));

              clay_item_get_offset (CLAY_ITEM (input), &source_off_x, &source_off_y);

              x -= (gdouble) source_off_x;
              y -= (gdouble) source_off_y;
            }
          fill = clay_drawable_get_line_art_fill_buffer (drawable,
                                                         tool->priv->line_art,
                                                         fill_options,
                                                         options->line_art_source ==
                                                         CLAY_LINE_ART_SOURCE_SAMPLE_MERGED,
                                                         options->fill_as_line_art &&
                                                         options->fill_mode != CLAY_BUCKET_FILL_PATTERN,
                                                         options->fill_as_line_art_threshold / 255.0,
                                                         options->line_art_stroke,
                                                         options->stroke_options,
                                                         x, y,
                                                         &tool->priv->fill_mask,
                                                         &x, &y, NULL, NULL);
          if (options->line_art_source != CLAY_LINE_ART_SOURCE_SAMPLE_MERGED)
            {
              gint off_x, off_y;

              clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

              x -= (gdouble) off_x - (gdouble) source_off_x;
              y -= (gdouble) off_y - (gdouble) source_off_y;
            }
        }
      if (fill)
        {
          gegl_node_set (tool->priv->fill_node,
                         "buffer", fill,
                         NULL);
          gegl_node_set (tool->priv->offset_node,
                         "x", x,
                         "y", y,
                         NULL);
          clay_drawable_filter_apply (tool->priv->filter, NULL);
          g_object_unref (fill);
        }
    }
}

static void
clay_bucket_fill_tool_commit (ClayBucketFillTool *tool)
{
  if (tool->priv->filter)
    {
      clay_drawable_filter_commit (tool->priv->filter, FALSE,
                                   CLAY_PROGRESS (tool), FALSE);
      clay_image_flush (clay_display_get_image (CLAY_TOOL (tool)->display));
    }
}

static void
clay_bucket_fill_tool_halt (ClayBucketFillTool *tool)
{
  if (tool->priv->graph)
    {
      g_clear_object (&tool->priv->graph);
      tool->priv->fill_node   = NULL;
      tool->priv->offset_node = NULL;
    }

  if (tool->priv->filter)
    {
      clay_drawable_filter_abort (tool->priv->filter);
      g_clear_object (&tool->priv->filter);
    }

  g_clear_object (&tool->priv->fill_mask);

  if (clay_line_art_is_frozen (tool->priv->line_art))
    clay_line_art_thaw (tool->priv->line_art);

  CLAY_TOOL (tool)->display   = NULL;
  g_list_free (CLAY_TOOL (tool)->drawables);
  CLAY_TOOL (tool)->drawables = NULL;
}

static void
clay_bucket_fill_tool_filter_flush (ClayDrawableFilter *filter,
                                    ClayTool           *tool)
{
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

static void
clay_bucket_fill_tool_create_graph (ClayBucketFillTool *tool)
{
  GeglNode *graph;
  GeglNode *output;
  GeglNode *fill_node;
  GeglNode *offset_node;

  g_return_if_fail (! tool->priv->graph     &&
                    ! tool->priv->fill_node &&
                    ! tool->priv->offset_node);

  graph = gegl_node_new ();

  fill_node = gegl_node_new_child (graph,
                                   "operation", "gegl:buffer-source",
                                   NULL);
  offset_node = gegl_node_new_child (graph,
                                     "operation", "gegl:translate",
                                     NULL);
  output = gegl_node_get_output_proxy (graph, "output");
  gegl_node_link_many (fill_node, offset_node, output, NULL);

  tool->priv->graph       = graph;
  tool->priv->fill_node   = fill_node;
  tool->priv->offset_node = offset_node;
}

static void
clay_bucket_fill_tool_button_press (ClayTool            *tool,
                                    const ClayCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    ClayButtonPressType  press_type,
                                    ClayDisplay         *display)
{
  ClayBucketFillTool    *bucket_tool = CLAY_BUCKET_FILL_TOOL (tool);
  ClayBucketFillOptions *options     = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayGuiConfig         *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage             *image       = clay_display_get_image (display);
  GList                 *drawables   = clay_image_get_selected_drawables (image);
  ClayDrawable          *drawable;

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        clay_tool_message_literal (tool, display,
                                   _("Cannot fill multiple layers. Select only one layer."));
      else
        clay_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);
      return;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
    {
      clay_tool_message_literal (tool, display,
                                 _("Cannot modify the pixels of layer groups."));
      return;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      clay_tool_message_literal (tool, display,
                                 _("The active layer is not visible."));
      return;
    }

  if (options->fill_area == CLAY_BUCKET_FILL_LINE_ART &&
      ! clay_line_art_get_input (bucket_tool->priv->line_art))
    {
      clay_tool_message_literal (tool, display,
                                 _("No valid line art source selected."));
      clay_blink_dockable (display->clay, "clay-tool-options", "line-art-source", NULL, NULL);
      return;
    }

  if (press_type == CLAY_BUTTON_PRESS_NORMAL &&
      clay_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords))
    {
      ClayContext     *context  = CLAY_CONTEXT (options);
      ClayFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = clay_fill_options_new (image->clay, NULL, FALSE);

      if (clay_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          clay_fill_options_set_antialias (fill_options, options->antialias);
          clay_fill_options_set_feather (fill_options, options->feather,
                                         options->feather_radius);

          clay_context_set_opacity (CLAY_CONTEXT (fill_options),
                                    clay_context_get_opacity (context));
          clay_context_set_paint_mode (CLAY_CONTEXT (fill_options),
                                       clay_context_get_paint_mode (context));

          if (options->fill_area == CLAY_BUCKET_FILL_SELECTION)
            {
              clay_drawable_edit_fill (drawable, fill_options, NULL);
              clay_image_flush (image);
            }
          else /* CLAY_BUCKET_FILL_SIMILAR_COLORS || CLAY_BUCKET_FILL_LINE_ART */
            {
              clay_bucket_fill_tool_start (bucket_tool, coords, display);
              clay_bucket_fill_tool_preview (bucket_tool, coords, display,
                                             fill_options);
            }
        }
      else
        {
          clay_message_literal (display->clay, G_OBJECT (display),
                                CLAY_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }

  CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);
}

static void
clay_bucket_fill_tool_motion (ClayTool         *tool,
                              const ClayCoords *coords,
                              guint32           time,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayBucketFillTool    *bucket_tool = CLAY_BUCKET_FILL_TOOL (tool);
  ClayBucketFillOptions *options     = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayImage             *image       = clay_display_get_image (display);

  CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    return;

  if (clay_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords) &&
      /* Fill selection only needs to happen once. */
      options->fill_area != CLAY_BUCKET_FILL_SELECTION)
    {
      ClayContext     *context  = CLAY_CONTEXT (options);
      ClayFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = clay_fill_options_new (image->clay, NULL, FALSE);

      if (clay_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          clay_fill_options_set_antialias (fill_options, options->antialias);
          clay_fill_options_set_feather (fill_options, options->feather,
                                         options->feather_radius);

          clay_context_set_opacity (CLAY_CONTEXT (fill_options),
                                    clay_context_get_opacity (context));
          clay_context_set_paint_mode (CLAY_CONTEXT (fill_options),
                                       clay_context_get_paint_mode (context));

          clay_bucket_fill_tool_preview (bucket_tool, coords, display,
                                         fill_options);
        }
      else
        {
          clay_message_literal (display->clay, G_OBJECT (display),
                                CLAY_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }
}

static void
clay_bucket_fill_tool_button_release (ClayTool              *tool,
                                      const ClayCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      ClayButtonReleaseType  release_type,
                                      ClayDisplay           *display)
{
  ClayBucketFillTool    *bucket_tool = CLAY_BUCKET_FILL_TOOL (tool);
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  if (release_type != CLAY_BUTTON_RELEASE_CANCEL)
    clay_bucket_fill_tool_commit (bucket_tool);

  if (options->fill_area != CLAY_BUCKET_FILL_SELECTION)
    clay_bucket_fill_tool_halt (bucket_tool);

  CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static void
clay_bucket_fill_tool_modifier_key (ClayTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    ClayDisplay     *display)
{
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (key == GDK_MOD1_MASK)
    {
      if (press)
        {
          CLAY_BUCKET_FILL_TOOL (tool)->priv->fill_mode = options->fill_mode;

          switch (options->fill_mode)
            {
            case CLAY_BUCKET_FILL_FG:
              g_object_set (options, "fill-mode", CLAY_BUCKET_FILL_BG, NULL);
              break;

            default:
              /* CLAY_BUCKET_FILL_BG || CLAY_BUCKET_FILL_PATTERN */
              g_object_set (options, "fill-mode", CLAY_BUCKET_FILL_FG, NULL);
              break;

              break;
            }
        }
      else /* release */
        {
          g_object_set (options, "fill-mode",
                        CLAY_BUCKET_FILL_TOOL (tool)->priv->fill_mode,
                        NULL);
        }
    }
  else if (key == clay_get_toggle_behavior_mask ())
    {
      ClayToolInfo *info = clay_get_tool_info (display->clay,
                                               "clay-color-picker-tool");

      if (! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
        {
          switch (CLAY_COLOR_TOOL (tool)->pick_target)
            {
            case CLAY_COLOR_PICK_TARGET_BACKGROUND:
              clay_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "background color"));
              break;

            case CLAY_COLOR_PICK_TARGET_FOREGROUND:
            default:
              clay_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "foreground color"));
              break;
            }

          CLAY_TOOL (tool)->display = display;
          clay_color_tool_enable (CLAY_COLOR_TOOL (tool),
                                  CLAY_COLOR_OPTIONS (info->tool_options));
        }
      else
        {
          clay_tool_pop_status (tool, display);
          clay_color_tool_disable (CLAY_COLOR_TOOL (tool));
          CLAY_TOOL (tool)->display = NULL;
        }
    }
  else if (key == clay_get_extend_selection_mask ())
    {
      if (press)
        {
          CLAY_BUCKET_FILL_TOOL (tool)->priv->fill_area = options->fill_area;

          switch (options->fill_area)
            {
            case CLAY_BUCKET_FILL_SIMILAR_COLORS:
              g_object_set (options,
                            "fill-area", CLAY_BUCKET_FILL_SELECTION,
                            NULL);
              break;

            default:
              /* CLAY_BUCKET_FILL_SELECTION || CLAY_BUCKET_FILL_LINE_ART */
              g_object_set (options,
                            "fill-area", CLAY_BUCKET_FILL_SIMILAR_COLORS,
                            NULL);
              break;
            }
        }
      else /* release */
        {
          g_object_set (options, "fill-area",
                        CLAY_BUCKET_FILL_TOOL (tool)->priv->fill_area,
                        NULL);
        }
    }
}

static void
clay_bucket_fill_tool_cursor_update (ClayTool         *tool,
                                     const ClayCoords *coords,
                                     GdkModifierType   state,
                                     ClayDisplay      *display)
{
  ClayBucketFillTool    *bucket_tool = CLAY_BUCKET_FILL_TOOL (tool);
  ClayBucketFillOptions *options     = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayGuiConfig         *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayCursorModifier     modifier    = CLAY_CURSOR_MODIFIER_BAD;
  ClayImage             *image       = clay_display_get_image (display);

  if (clay_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords))
    {
      GList        *drawables = clay_image_get_selected_drawables (image);
      ClayDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        drawable = drawables->data;

      if (drawable                                                   &&
          ! clay_viewable_get_children (CLAY_VIEWABLE (drawable))    &&
          ! clay_item_is_content_locked (CLAY_ITEM (drawable), NULL) &&
          (clay_item_is_visible (CLAY_ITEM (drawable)) ||
           config->edit_non_visible))
        {
          switch (options->fill_mode)
            {
            case CLAY_BUCKET_FILL_FG:
              modifier = CLAY_CURSOR_MODIFIER_FOREGROUND;
              break;

            case CLAY_BUCKET_FILL_BG:
              modifier = CLAY_CURSOR_MODIFIER_BACKGROUND;
              break;

            case CLAY_BUCKET_FILL_PATTERN:
              modifier = CLAY_CURSOR_MODIFIER_PATTERN;
              break;
            }
        }

      g_list_free (drawables);
    }

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_bucket_fill_tool_options_notify (ClayTool         *tool,
                                      ClayToolOptions  *options,
                                      const GParamSpec *pspec)
{
  ClayBucketFillTool    *bucket_tool    = CLAY_BUCKET_FILL_TOOL (tool);
  ClayBucketFillOptions *bucket_options = CLAY_BUCKET_FILL_OPTIONS (options);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "fill-area"))
    {
      /* We want more motion events when the tool is used in a paint tool
       * fashion. Unfortunately we only set exact mode in line art fill,
       * because we can't as easily remove events from the similar color
       * mode just because a point has already been selected  (unless
       * threshold were 0, but that's an edge case).
       */
      clay_tool_control_set_motion_mode (tool->control,
                                         bucket_options->fill_area == CLAY_BUCKET_FILL_LINE_ART ?
                                         CLAY_MOTION_MODE_EXACT : CLAY_MOTION_MODE_COMPRESS);

      clay_bucket_fill_tool_reset_line_art (bucket_tool);
    }
  else if (! strcmp (pspec->name, "fill-mode"))
    {
      if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
        clay_tool_pop_status (tool, tool->display);

      switch (bucket_options->fill_mode)
        {
        case CLAY_BUCKET_FILL_BG:
          CLAY_COLOR_TOOL (tool)->pick_target = CLAY_COLOR_PICK_TARGET_BACKGROUND;
          if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
            clay_tool_push_status (tool, tool->display,
                                   _("Click in any image to pick the "
                                     "background color"));
          break;

        case CLAY_BUCKET_FILL_FG:
        default:
          CLAY_COLOR_TOOL (tool)->pick_target = CLAY_COLOR_PICK_TARGET_FOREGROUND;
          if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
            clay_tool_push_status (tool, tool->display,
                                   _("Click in any image to pick the "
                                     "foreground color"));
          break;
        }
    }
}

static void
clay_bucket_fill_tool_line_art_computing_start (ClayBucketFillTool *tool)
{
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  gtk_widget_set_visible (options->line_art_busy_box, TRUE);
}

static void
clay_bucket_fill_tool_line_art_computing_end (ClayBucketFillTool *tool)
{
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  gtk_widget_set_visible (options->line_art_busy_box, FALSE);
}

static void
clay_bucket_fill_tool_reset_line_art (ClayBucketFillTool *tool)
{
  ClayBucketFillOptions *options  = CLAY_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  ClayLineArt           *line_art = tool->priv->line_art;
  ClayDisplayShell      *shell    = NULL;
  ClayImage             *image    = NULL;

  if (options->fill_area == CLAY_BUCKET_FILL_LINE_ART)
    {
      ClayContext *context;
      ClayDisplay *display;

      context = clay_get_user_context (CLAY_CONTEXT (options)->clay);
      display = clay_context_get_display (context);

      if (display)
        {
          shell = clay_display_get_shell (display);
          image = clay_display_get_image (display);
        }
    }

  if (image != tool->priv->line_art_image)
    {
      if (tool->priv->line_art_image)
        {
          g_signal_handlers_disconnect_by_data (clay_image_get_layers (tool->priv->line_art_image), tool);
          g_signal_handlers_disconnect_by_data (tool->priv->line_art_image, tool);
        }

      tool->priv->line_art_image = image;

      if (image)
        {
          g_signal_connect_swapped (image, "selected-layers-changed",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (image, "selected-channels-changed",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);

          g_signal_connect_swapped (clay_image_get_layers (image), "add",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (clay_image_get_layers (image), "remove",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (clay_image_get_layers (image), "reorder",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);
        }
    }

  if (shell != tool->priv->line_art_shell)
    {
      if (tool->priv->line_art_shell)
        {
          g_signal_handlers_disconnect_by_func (
            tool->priv->line_art_shell,
            clay_bucket_fill_tool_reset_line_art,
            tool);
        }

      tool->priv->line_art_shell = shell;

      if (shell)
        {
          g_signal_connect_swapped (shell, "notify::show-all",
                                    G_CALLBACK (clay_bucket_fill_tool_reset_line_art),
                                    tool);
        }
    }

  if (image)
    {
      GList        *drawables = clay_image_get_selected_drawables (image);
      ClayDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        {
          drawable = drawables->data;
          if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
            drawable = NULL;
        }
      g_list_free (drawables);

      if (options->line_art_source == CLAY_LINE_ART_SOURCE_SAMPLE_MERGED)
        {
          ClayImageProxy *image_proxy = clay_image_proxy_new (image);

          clay_image_proxy_set_show_all (image_proxy, shell->show_all);

          clay_line_art_set_input (line_art, CLAY_PICKABLE (image_proxy));

          g_object_unref (image_proxy);
        }
      else if (drawable)
        {
          ClayItem      *parent;
          ClayContainer *container;
          ClayObject    *neighbour = NULL;
          ClayPickable  *source    = NULL;
          gint           index;

          parent = clay_item_get_parent (CLAY_ITEM (drawable));
          if (parent)
            container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
          else
            container = clay_image_get_layers (image);

          index = clay_item_get_index (CLAY_ITEM (drawable));

          if (options->line_art_source == CLAY_LINE_ART_SOURCE_ACTIVE_LAYER)
            source = CLAY_PICKABLE (drawable);
          else if (options->line_art_source == CLAY_LINE_ART_SOURCE_LOWER_LAYER)
            neighbour = clay_container_get_child_by_index (container, index + 1);
          else if (options->line_art_source == CLAY_LINE_ART_SOURCE_UPPER_LAYER)
            neighbour = clay_container_get_child_by_index (container, index - 1);

          source = neighbour ? CLAY_PICKABLE (neighbour) : source;
          clay_line_art_set_input (line_art, source);
        }
      else
        {
          clay_line_art_set_input (line_art, NULL);
        }
    }
  else
    {
      clay_line_art_set_input (line_art, NULL);
    }
}
