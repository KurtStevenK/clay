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

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayimage-crop.h"
#include "core/clayimage-undo.h"
#include "core/clayitem.h"
#include "core/claylayer.h"
#include "core/clayrasterizable.h"
#include "core/claytoolinfo.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolrectangle.h"

#include "claycropoptions.h"
#include "claycroptool.h"
#include "clayrectangleoptions.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


static void      clay_crop_tool_constructed                (GObject              *object);
static void      clay_crop_tool_dispose                    (GObject              *object);

static void      clay_crop_tool_control                    (ClayTool             *tool,
                                                            ClayToolAction        action,
                                                            ClayDisplay          *display);
static void      clay_crop_tool_button_press               (ClayTool             *tool,
                                                            const ClayCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            ClayButtonPressType   press_type,
                                                            ClayDisplay          *display);
static void      clay_crop_tool_button_release             (ClayTool             *tool,
                                                            const ClayCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            ClayButtonReleaseType release_type,
                                                            ClayDisplay          *display);
static void      clay_crop_tool_motion                     (ClayTool             *tool,
                                                            const ClayCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            ClayDisplay          *display);
static void      clay_crop_tool_options_notify             (ClayTool             *tool,
                                                            ClayToolOptions      *options,
                                                            const GParamSpec     *pspec);

static void      clay_crop_tool_rectangle_changed          (ClayToolWidget       *rectangle,
                                                            ClayCropTool         *crop_tool);
static void      clay_crop_tool_rectangle_response         (ClayToolWidget       *rectangle,
                                                            gint                  response_id,
                                                            ClayCropTool         *crop_tool);
static void      clay_crop_tool_rectangle_change_complete  (ClayToolRectangle    *rectangle,
                                                            ClayCropTool         *crop_tool);

static void      clay_crop_tool_start                      (ClayCropTool         *crop_tool,
                                                            ClayDisplay          *display);
static void      clay_crop_tool_commit                     (ClayCropTool         *crop_tool);
static void      clay_crop_tool_halt                       (ClayCropTool         *crop_tool);

static void      clay_crop_tool_update_option_defaults     (ClayCropTool         *crop_tool,
                                                            gboolean              ignore_pending);
static ClayRectangleConstraint
                 clay_crop_tool_get_constraint             (ClayCropTool         *crop_tool);

static void      clay_crop_tool_image_changed              (ClayCropTool         *crop_tool,
                                                            ClayImage            *image,
                                                            ClayContext          *context);
static void      clay_crop_tool_image_size_changed         (ClayCropTool         *crop_tool);
static void   clay_crop_tool_image_selected_layers_changed (ClayCropTool         *crop_tool);
static void      clay_crop_tool_layer_size_changed         (ClayCropTool         *crop_tool);

static void      clay_crop_tool_auto_shrink                (ClayCropTool         *crop_tool);


G_DEFINE_TYPE (ClayCropTool, clay_crop_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_crop_tool_parent_class


/*  public functions  */

void
clay_crop_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_CROP_TOOL,
                CLAY_TYPE_CROP_OPTIONS,
                clay_crop_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                CLAY_CONTEXT_PROP_MASK_PATTERN,
                "clay-crop-tool",
                _("Crop"),
                _("Crop Tool: Remove edge areas from image or layer"),
                N_("_Crop"), "<shift>C",
                NULL, CLAY_HELP_TOOL_CROP,
                CLAY_ICON_TOOL_CROP,
                data);
}

static void
clay_crop_tool_class_init (ClayCropToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->constructed  = clay_crop_tool_constructed;
  object_class->dispose      = clay_crop_tool_dispose;

  tool_class->control        = clay_crop_tool_control;
  tool_class->button_press   = clay_crop_tool_button_press;
  tool_class->button_release = clay_crop_tool_button_release;
  tool_class->motion         = clay_crop_tool_motion;
  tool_class->options_notify = clay_crop_tool_options_notify;
  tool_class->is_destructive = FALSE;
}

static void
clay_crop_tool_init (ClayCropTool *crop_tool)
{
  ClayTool *tool = CLAY_TOOL (crop_tool);

  clay_tool_control_set_wants_click      (tool->control, TRUE);
  clay_tool_control_set_active_modifiers (tool->control,
                                          CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision        (tool->control,
                                          CLAY_CURSOR_PRECISION_PIXEL_BORDER);
  clay_tool_control_set_cursor           (tool->control,
                                          CLAY_CURSOR_CROSSHAIR_SMALL);
  clay_tool_control_set_tool_cursor      (tool->control,
                                          CLAY_TOOL_CURSOR_CROP);

  clay_draw_tool_set_default_status (CLAY_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a crop rectangle"));
}

static void
clay_crop_tool_constructed (GObject *object)
{
  ClayTool     *tool      = CLAY_TOOL (object);
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);
  ClayContext  *context;
  ClayToolInfo *tool_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tool_info = CLAY_TOOL (crop_tool)->tool_info;

  context = clay_get_user_context (tool_info->clay);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (clay_crop_tool_image_changed),
                           crop_tool,
                           G_CONNECT_SWAPPED);

  /* Make sure we are connected to "size-changed" for the initial
   * image.
   */
  clay_crop_tool_image_changed (crop_tool,
                                clay_context_get_image (context),
                                context);
}

static void
clay_crop_tool_dispose (GObject *object)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (object);

  /* Clean up current_image and current_layers. */
  clay_crop_tool_image_changed (crop_tool, NULL, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_crop_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_crop_tool_halt (crop_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_crop_tool_commit (crop_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_crop_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      clay_crop_tool_start (crop_tool, display);

      clay_tool_widget_hover (crop_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * property bindings would cause the rectangle to start with the
       * size from tool options.
       */
      clay_tool_rectangle_set_function (CLAY_TOOL_RECTANGLE (crop_tool->widget),
                                        CLAY_TOOL_RECTANGLE_CREATING);
    }

  if (clay_tool_widget_button_press (crop_tool->widget, coords, time, state,
                                     press_type))
    {
      crop_tool->grab_widget = crop_tool->widget;
    }

  clay_tool_control_activate (tool->control);
}

static void
clay_crop_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);

  clay_tool_control_halt (tool->control);

  if (crop_tool->grab_widget)
    {
      clay_tool_widget_button_release (crop_tool->grab_widget,
                                       coords, time, state, release_type);
      crop_tool->grab_widget = NULL;
    }

  clay_tool_push_status (tool, display, _("Click or press Enter to crop"));
}

static void
clay_crop_tool_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);

  if (crop_tool->grab_widget)
    {
      clay_tool_widget_motion (crop_tool->grab_widget, coords, time, state);
    }
}

static void
clay_crop_tool_options_notify (ClayTool         *tool,
                               ClayToolOptions  *options,
                               const GParamSpec *pspec)
{
  ClayCropTool *crop_tool = CLAY_CROP_TOOL (tool);

  if (! strcmp (pspec->name, "layer-only") ||
      ! strcmp (pspec->name, "allow-growing"))
    {
      if (crop_tool->widget)
        {
          clay_tool_rectangle_set_constraint (CLAY_TOOL_RECTANGLE (crop_tool->widget),
                                              clay_crop_tool_get_constraint (crop_tool));
        }
      else
        {
          clay_crop_tool_update_option_defaults (crop_tool, FALSE);
        }
    }
}

static void
clay_crop_tool_rectangle_changed (ClayToolWidget *rectangle,
                                  ClayCropTool   *crop_tool)
{
}

static void
clay_crop_tool_rectangle_response (ClayToolWidget *rectangle,
                                   gint            response_id,
                                   ClayCropTool   *crop_tool)
{
  ClayTool *tool = CLAY_TOOL (crop_tool);

  switch (response_id)
    {
    case CLAY_TOOL_WIDGET_RESPONSE_CONFIRM:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_CANCEL:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_crop_tool_rectangle_change_complete (ClayToolRectangle *rectangle,
                                          ClayCropTool      *crop_tool)
{
  clay_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
clay_crop_tool_start (ClayCropTool *crop_tool,
                      ClayDisplay  *display)
{
  static const gchar *properties[] =
  {
    "highlight",
    "highlight-opacity",
    "guide",
    "x",
    "y",
    "width",
    "height",
    "fixed-rule-active",
    "fixed-rule",
    "desired-fixed-width",
    "desired-fixed-height",
    "desired-fixed-size-width",
    "desired-fixed-size-height",
    "aspect-numerator",
    "aspect-denominator",
    "fixed-center"
  };

  ClayTool         *tool    = CLAY_TOOL (crop_tool);
  ClayDisplayShell *shell   = clay_display_get_shell (display);
  ClayCropOptions  *options = CLAY_CROP_TOOL_GET_OPTIONS (crop_tool);
  ClayToolWidget   *widget;
  gint              i;

  tool->display = display;

  crop_tool->widget = widget = clay_tool_rectangle_new (shell);

  g_object_set (widget,
                "status-title", _("Crop to: "),
                NULL);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      crop_tool->bindings = g_list_prepend (crop_tool->bindings, binding);
    }

  clay_rectangle_options_connect (CLAY_RECTANGLE_OPTIONS (options),
                                  clay_display_get_image (shell->display),
                                  G_CALLBACK (clay_crop_tool_auto_shrink),
                                  crop_tool);

  clay_tool_rectangle_set_constraint (CLAY_TOOL_RECTANGLE (widget),
                                      clay_crop_tool_get_constraint (crop_tool));

  g_signal_connect (widget, "changed",
                    G_CALLBACK (clay_crop_tool_rectangle_changed),
                    crop_tool);
  g_signal_connect (widget, "response",
                    G_CALLBACK (clay_crop_tool_rectangle_response),
                    crop_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (clay_crop_tool_rectangle_change_complete),
                    crop_tool);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

static void
clay_crop_tool_commit (ClayCropTool *crop_tool)
{
  ClayTool *tool = CLAY_TOOL (crop_tool);

  if (tool->display)
    {
      ClayCropOptions *options = CLAY_CROP_TOOL_GET_OPTIONS (tool);
      ClayImage       *image   = clay_display_get_image (tool->display);
      gdouble          x, y;
      gdouble          x2, y2;
      gint             w, h;

      clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (crop_tool->widget),
                                           &x, &y, &x2, &y2);
      w = x2 - x;
      h = y2 - y;

      clay_tool_pop_status (tool, tool->display);

      /* if rectangle exists, crop it */
      if (w > 0 && h > 0)
        {
          if (options->layer_only)
            {
              GList    *layers = clay_image_get_selected_layers (image);
              GList    *iter;
              gint      off_x, off_y;
              gchar    *undo_text;
              gboolean  has_non_raster_layers = FALSE;

              if (! layers)
                {
                  clay_tool_message_literal (tool, tool->display,
                                             _("There are no selected layers to crop."));
                  return;
                }

              for (iter = layers; iter; iter = iter->next)
                if (! clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL))
                  break;

              if (iter == NULL)
                {
                  clay_tool_message_literal (tool, tool->display,
                                             _("All selected layers' pixels are locked."));
                  clay_tools_blink_lock_box (tool->display->clay, CLAY_ITEM (layers->data));
                  return;
                }

              undo_text = ngettext ("Resize Layer", "Resize %d layers",
                                    g_list_length (layers));
              undo_text = g_strdup_printf (undo_text, g_list_length (layers));
              clay_image_undo_group_start (image,
                                           CLAY_UNDO_GROUP_IMAGE_CROP,
                                           undo_text);
              g_free (undo_text);
              for (iter = layers; iter; iter = iter->next)
                {
                  /* Don't crop non-raster layers */
                  if (CLAY_IS_RASTERIZABLE (iter->data) &&
                      ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (iter->data)))
                    {
                      has_non_raster_layers = TRUE;
                      continue;
                    }

                  clay_item_get_offset (CLAY_ITEM (iter->data), &off_x, &off_y);

                  off_x -= x;
                  off_y -= y;

                  /* If we have the crop tool set to allow growing and to fill
                   * with transparency, and the crop rectangle is larger than
                   * the layer, then we add transparency if there is none */
                  if (options->allow_growing                                 &&
                      (clay_item_get_width (CLAY_ITEM (iter->data)) < w      ||
                       clay_item_get_width (CLAY_ITEM (iter->data)) < h)     &&
                      options->fill_type == CLAY_FILL_TRANSPARENT            &&
                      ! clay_drawable_has_alpha (CLAY_DRAWABLE (iter->data)))
                    {
                      clay_layer_add_alpha (CLAY_LAYER (iter->data));
                    }

                  clay_item_resize (CLAY_ITEM (iter->data),
                                    CLAY_CONTEXT (options), options->fill_type,
                                    w, h, off_x, off_y);
                }
              clay_image_undo_group_end (image);

              if (has_non_raster_layers)
                {
                  gchar *menu_path = _("Layer > Rasterize");
                  gchar *message   = NULL;

                  /* TODO: Translate after string freeze */
                  message = g_strdup_printf ("Non-raster layers must be rasterized (%s).",
                                             menu_path);

                  clay_tool_message_literal (tool, tool->display, message);
                  g_free (message);
                }
            }
          else
            {
              clay_image_crop (image,
                               CLAY_CONTEXT (options), CLAY_FILL_TRANSPARENT,
                               x, y, w, h, options->delete_pixels);
            }

          clay_image_flush (image);
        }
    }
}

static void
clay_crop_tool_halt (ClayCropTool *crop_tool)
{
  ClayTool         *tool    = CLAY_TOOL (crop_tool);
  ClayCropOptions  *options = CLAY_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (tool->display)
    {
      ClayDisplayShell *shell = clay_display_get_shell (tool->display);

      clay_display_shell_set_highlight (shell, NULL, 0.0);

      clay_rectangle_options_disconnect (CLAY_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (clay_crop_tool_auto_shrink),
                                         crop_tool);
    }

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (crop_tool->bindings, (GDestroyNotify) g_object_unref);
  crop_tool->bindings = NULL;

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&crop_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  clay_crop_tool_update_option_defaults (crop_tool, TRUE);
}

/**
 * clay_crop_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
clay_crop_tool_update_option_defaults (ClayCropTool *crop_tool,
                                       gboolean      ignore_pending)
{
  ClayTool             *tool      = CLAY_TOOL (crop_tool);
  ClayToolRectangle    *rectangle = CLAY_TOOL_RECTANGLE (crop_tool->widget);
  ClayRectangleOptions *options;

  options = CLAY_RECTANGLE_OPTIONS (CLAY_TOOL_GET_OPTIONS (tool));

  if (rectangle && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Aspect ratio to the same as the current
       * pending rectangle width/height.
       */

      clay_tool_rectangle_pending_size_set (rectangle,
                                            G_OBJECT (options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      /* There is no pending rectangle, set default Fixed: Aspect
       * ratio to that of the current image/layer.
       */

      if (! rectangle)
        {
          /* ugly hack: if we don't have a widget, construct a temporary one
           * so that we can use it to call
           * clay_tool_rectangle_constraint_size_set().
           */

          ClayContext *context = clay_get_user_context (tool->tool_info->clay);
          ClayDisplay *display = clay_context_get_display (context);

          if (display)
            {
              ClayDisplayShell *shell = clay_display_get_shell (display);

              rectangle = CLAY_TOOL_RECTANGLE (clay_tool_rectangle_new (shell));

              clay_tool_rectangle_set_constraint (
                rectangle, clay_crop_tool_get_constraint (crop_tool));
            }
        }

      if (rectangle)
        {
          clay_tool_rectangle_constraint_size_set (rectangle,
                                                   G_OBJECT (options),
                                                   "default-aspect-numerator",
                                                   "default-aspect-denominator");

          if (! crop_tool->widget)
            g_object_unref (rectangle);
        }

      g_object_set (G_OBJECT (options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

static ClayRectangleConstraint
clay_crop_tool_get_constraint (ClayCropTool *crop_tool)
{
  ClayCropOptions *crop_options = CLAY_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (crop_options->allow_growing)
    {
      return CLAY_RECTANGLE_CONSTRAIN_NONE;
    }
  else
    {
      return crop_options->layer_only ? CLAY_RECTANGLE_CONSTRAIN_DRAWABLE :
                                        CLAY_RECTANGLE_CONSTRAIN_IMAGE;
    }
}

static void
clay_crop_tool_image_changed (ClayCropTool *crop_tool,
                              ClayImage    *image,
                              ClayContext  *context)
{
  if (crop_tool->current_image)
    {
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            clay_crop_tool_image_size_changed,
                                            NULL);
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            clay_crop_tool_image_selected_layers_changed,
                                            NULL);
    }

  g_set_weak_pointer (&crop_tool->current_image, image);

  if (crop_tool->current_image)
    {
      g_signal_connect_object (crop_tool->current_image, "size-changed",
                               G_CALLBACK (clay_crop_tool_image_size_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (crop_tool->current_image, "selected-layers-changed",
                               G_CALLBACK (clay_crop_tool_image_selected_layers_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
    }

  /* Make sure we are connected to "size-changed" for the initial
   * layer.
   */
  clay_crop_tool_image_selected_layers_changed (crop_tool);

  clay_crop_tool_update_option_defaults (CLAY_CROP_TOOL (crop_tool), FALSE);
}

static void
clay_crop_tool_image_size_changed (ClayCropTool *crop_tool)
{
  clay_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
clay_crop_tool_image_selected_layers_changed (ClayCropTool *crop_tool)
{
  GList *iter;

  if (crop_tool->current_layers)
    {
      for (iter = crop_tool->current_layers; iter; iter = iter->next)
        {
          if (iter->data)
            {
              g_signal_handlers_disconnect_by_func (iter->data,
                                                    clay_crop_tool_layer_size_changed,
                                                    NULL);

              g_clear_weak_pointer (&iter->data);
            }
        }
      g_list_free (crop_tool->current_layers);
      crop_tool->current_layers = NULL;
    }

  if (crop_tool->current_image)
    {
      crop_tool->current_layers = clay_image_get_selected_layers (crop_tool->current_image);
      crop_tool->current_layers = g_list_copy (crop_tool->current_layers);
    }
  else
    {
      crop_tool->current_layers = NULL;
    }

  if (crop_tool->current_layers)
    {
      for (iter = crop_tool->current_layers; iter; iter = iter->next)
        {
          /* NOT g_set_weak_pointer() because the pointer is already set */
          g_object_add_weak_pointer (G_OBJECT (iter->data),
                                     (gpointer) &iter->data);

          g_signal_connect_object (iter->data, "size-changed",
                                   G_CALLBACK (clay_crop_tool_layer_size_changed),
                                   crop_tool,
                                   G_CONNECT_SWAPPED);
        }
    }

  clay_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
clay_crop_tool_layer_size_changed (ClayCropTool *crop_tool)
{
  clay_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
clay_crop_tool_auto_shrink (ClayCropTool *crop_tool)
{
  gboolean shrink_merged ;

  g_object_get (clay_tool_get_options (CLAY_TOOL (crop_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  clay_tool_rectangle_auto_shrink (CLAY_TOOL_RECTANGLE (crop_tool->widget),
                                   shrink_merged);
}
