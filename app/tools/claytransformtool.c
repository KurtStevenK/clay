/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claydrawable-transform.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/clayimage-item-list.h"
#include "core/clayimage-transform.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/clayprogress.h"
#include "core/clay-transform-resize.h"

#include "path/claypath.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "widgets/claymessagedialog.h"
#include "widgets/claymessagebox.h"
#include "widgets/claywidgets-utils.h"

#include "claytoolcontrol.h"
#include "claytools-utils.h"
#include "claytransformoptions.h"
#include "claytransformtool.h"

#include "clay-intl.h"


/* the minimal ratio between the transformed item size and the image size,
 * above which confirmation is required.
 */
#define MIN_CONFIRMATION_RATIO 10


/*  local function prototypes  */

static void                     clay_transform_tool_control            (ClayTool           *tool,
                                                                        ClayToolAction      action,
                                                                        ClayDisplay        *display);

static gchar                  * clay_transform_tool_real_get_undo_desc (ClayTransformTool  *tr_tool);
static ClayTransformDirection   clay_transform_tool_real_get_direction (ClayTransformTool  *tr_tool);
static GeglBuffer             * clay_transform_tool_real_transform     (ClayTransformTool  *tr_tool,
                                                                        GList              *objects,
                                                                        GeglBuffer         *orig_buffer,
                                                                        gint                orig_offset_x,
                                                                        gint                orig_offset_y,
                                                                        ClayColorProfile  **buffer_profile,
                                                                        gint               *new_offset_x,
                                                                        gint               *new_offset_y);

static void                     clay_transform_tool_halt               (ClayTransformTool  *tr_tool);

static gboolean                 clay_transform_tool_confirm            (ClayTransformTool  *tr_tool,
                                                                        ClayDisplay        *display);


G_DEFINE_TYPE (ClayTransformTool, clay_transform_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_transform_tool_parent_class


/*  private functions  */


static void
clay_transform_tool_class_init (ClayTransformToolClass *klass)
{
  ClayToolClass *tool_class = CLAY_TOOL_CLASS (klass);

  tool_class->control        = clay_transform_tool_control;
  tool_class->is_destructive = FALSE;

  klass->recalc_matrix = NULL;
  klass->get_undo_desc = clay_transform_tool_real_get_undo_desc;
  klass->get_direction = clay_transform_tool_real_get_direction;
  klass->transform     = clay_transform_tool_real_transform;

  klass->undo_desc     = _("Transform");
  klass->progress_text = _("Transforming");
}

static void
clay_transform_tool_init (ClayTransformTool *tr_tool)
{
  clay_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;

  tr_tool->restore_type = FALSE;
}

static void
clay_transform_tool_control (ClayTool       *tool,
                             ClayToolAction  action,
                             ClayDisplay    *display)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_transform_tool_halt (tr_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
clay_transform_tool_real_get_undo_desc (ClayTransformTool *tr_tool)
{
  return g_strdup (CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->undo_desc);
}

static ClayTransformDirection
clay_transform_tool_real_get_direction (ClayTransformTool *tr_tool)
{
  ClayTransformOptions *options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  return options->direction;
}

static GeglBuffer *
clay_transform_tool_real_transform (ClayTransformTool *tr_tool,
                                    GList             *objects,
                                    GeglBuffer        *orig_buffer,
                                    gint               orig_offset_x,
                                    gint               orig_offset_y,
                                    ClayColorProfile **buffer_profile,
                                    gint              *new_offset_x,
                                    gint              *new_offset_y)
{
  ClayTransformToolClass *klass   = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool);
  ClayTool               *tool    = CLAY_TOOL (tr_tool);
  ClayTransformOptions   *options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tool);
  ClayContext            *context = CLAY_CONTEXT (options);
  GeglBuffer             *ret     = NULL;
  ClayTransformResize     clip    = options->clip;
  ClayTransformDirection  direction;
  ClayProgress           *progress;

  direction = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (tr_tool);

  progress = clay_progress_start (CLAY_PROGRESS (tool), FALSE,
                                  "%s", klass->progress_text);

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of
       *  normal drawables.
       */

      ret = clay_drawable_transform_buffer_affine (objects->data,
                                                   context,
                                                   orig_buffer,
                                                   orig_offset_x,
                                                   orig_offset_y,
                                                   &tr_tool->transform,
                                                   direction,
                                                   options->interpolation,
                                                   clip,
                                                   buffer_profile,
                                                   new_offset_x,
                                                   new_offset_y,
                                                   progress);
    }
  else if (g_list_length (objects) == 1 && CLAY_IS_IMAGE (objects->data))
    {
      /*  this happens for images  */

      clay_image_transform (objects->data, context,
                            &tr_tool->transform,
                            direction,
                            options->interpolation,
                            clip,
                            progress);
    }
  else
    {
      GList *items;

      /*  this happens for entire drawables, paths and layer groups  */
      g_return_val_if_fail (g_list_length (objects) > 0, NULL);

      items = clay_image_item_list_filter (g_list_copy (objects));

      clay_image_item_list_transform (clay_item_get_image (objects->data),
                                      items, context,
                                      &tr_tool->transform,
                                      direction,
                                      options->interpolation,
                                      clip,
                                      progress);
      g_list_free (items);
    }

  if (progress)
    clay_progress_end (progress);

  return ret;
}

static void
clay_transform_tool_halt (ClayTransformTool *tr_tool)
{
  ClayTransformOptions *options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  tr_tool->x1 = 0;
  tr_tool->y1 = 0;
  tr_tool->x2 = 0;
  tr_tool->y2 = 0;

  if (tr_tool->restore_type)
    {
      g_object_set (options,
                    "type", tr_tool->saved_type,
                    NULL);

      tr_tool->restore_type = FALSE;
    }
}

static gboolean
clay_transform_tool_confirm (ClayTransformTool *tr_tool,
                             ClayDisplay       *display)
{
  ClayTransformOptions *options          = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  ClayDisplayShell     *shell            = clay_display_get_shell (display);
  ClayImage            *image            = clay_display_get_image (display);
  GList                *selected_objects;
  gdouble               max_ratio        = 0.0;
  ClayObject           *max_ratio_object = NULL;

  selected_objects = clay_transform_tool_get_selected_objects (tr_tool, display);

  if (CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    {
      ClayMatrix3             transform;
      ClayTransformDirection  direction;
      GeglRectangle           selection_bounds;
      gboolean                selection_empty = TRUE;
      GList                  *objects         = NULL;
      GList                  *iter;

      transform = tr_tool->transform;
      direction = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (
        tr_tool);

      if (direction == CLAY_TRANSFORM_BACKWARD)
        clay_matrix3_invert (&transform);

      if (options->type == CLAY_TRANSFORM_TYPE_LAYER)
        {
          for (iter = selected_objects; iter; iter = iter->next)
            if (! clay_viewable_get_children (CLAY_VIEWABLE (iter->data)))
          {
            if (clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                                  &selection_bounds.x,     &selection_bounds.y,
                                  &selection_bounds.width, &selection_bounds.height))
              {
                selection_empty = FALSE;
                break;
              }
          }
        }

      if (selection_empty  &&
          selected_objects &&
          CLAY_IS_ITEM (selected_objects->data))
        {
          objects = clay_image_item_list_filter (g_list_copy (selected_objects));
          g_list_free (selected_objects);
        }
      else
        {
          objects = selected_objects;
        }

      if (options->type == CLAY_TRANSFORM_TYPE_IMAGE)
        {
          objects = g_list_concat (
            objects,
            clay_image_item_list_get_list (image,
                                           CLAY_ITEM_TYPE_ALL,
                                           CLAY_ITEM_SET_ALL));
        }

      for (iter = objects; iter; iter = g_list_next (iter))
        {
          ClayObject          *object = iter->data;
          ClayTransformResize  clip   = options->clip;
          GeglRectangle        orig_bounds;
          GeglRectangle        new_bounds;
          gdouble              ratio  = 0.0;

          if (CLAY_IS_DRAWABLE (object))
            {
              if (selection_empty)
                {
                  ClayItem *item = CLAY_ITEM (object);

                  clay_item_get_offset (item, &orig_bounds.x, &orig_bounds.y);

                  orig_bounds.width  = clay_item_get_width  (item);
                  orig_bounds.height = clay_item_get_height (item);

                  clip = clay_item_get_clip (item, clip);
                }
              else
                {
                  orig_bounds = selection_bounds;
                }
            }
          else if (CLAY_IS_ITEM (object))
            {
              ClayItem *item = CLAY_ITEM (object);

              clay_item_bounds (item,
                                &orig_bounds.x,     &orig_bounds.y,
                                &orig_bounds.width, &orig_bounds.height);

              clip = clay_item_get_clip (item, clip);
            }
          else
            {
              ClayImage *image;

              g_return_val_if_fail (CLAY_IS_IMAGE (object), FALSE);

              image = CLAY_IMAGE (object);

              orig_bounds.x      = 0;
              orig_bounds.y      = 0;
              orig_bounds.width  = clay_image_get_width  (image);
              orig_bounds.height = clay_image_get_height (image);
            }

          clay_transform_resize_boundary (&transform, clip,

                                          orig_bounds.x,
                                          orig_bounds.y,
                                          orig_bounds.x + orig_bounds.width,
                                          orig_bounds.y + orig_bounds.height,

                                          &new_bounds.x,
                                          &new_bounds.y,
                                          &new_bounds.width,
                                          &new_bounds.height);

          new_bounds.width  -= new_bounds.x;
          new_bounds.height -= new_bounds.y;

          if (new_bounds.width > orig_bounds.width)
            {
              ratio = MAX (ratio,
                           (gdouble) new_bounds.width /
                           (gdouble) clay_image_get_width (image));
            }

          if (new_bounds.height > orig_bounds.height)
            {
              ratio = MAX (ratio,
                           (gdouble) new_bounds.height /
                           (gdouble) clay_image_get_height (image));
            }

          if (ratio > max_ratio)
            {
              max_ratio        = ratio;
              max_ratio_object = object;
            }
        }

      g_list_free (objects);
    }

  if (max_ratio > MIN_CONFIRMATION_RATIO)
    {
      GtkWidget *dialog;
      gint       response;

      dialog = clay_message_dialog_new (_("Confirm Transformation"),
                                        CLAY_ICON_DIALOG_WARNING,
                                        GTK_WIDGET (shell),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        clay_standard_help_func, NULL,

                                        _("_Cancel"),    GTK_RESPONSE_CANCEL,
                                        _("_Transform"), GTK_RESPONSE_OK,

                                        NULL);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      if (CLAY_IS_ITEM (max_ratio_object))
        {
          clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large item."));

          clay_message_box_set_text (
            CLAY_MESSAGE_DIALOG (dialog)->box,
            _("Applying the transformation will result "
              "in an item that is over %g times larger "
              "than the image."),
              floor (max_ratio));
        }
      else
        {
          clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large image."));

          clay_message_box_set_text (
            CLAY_MESSAGE_DIALOG (dialog)->box,
            _("Applying the transformation will enlarge "
              "the image by a factor of %g."),
              floor (max_ratio));
        }

      response = gtk_dialog_run (GTK_DIALOG (dialog));

      gtk_widget_destroy (dialog);

      if (response != GTK_RESPONSE_OK)
        return FALSE;
    }

  return TRUE;
}


/*  public functions  */


gboolean
clay_transform_tool_bounds (ClayTransformTool *tr_tool,
                            ClayDisplay       *display)
{
  ClayTransformOptions *options;
  ClayDisplayShell     *shell;
  ClayImage            *image;
  gboolean              non_empty = TRUE;

  g_return_val_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool), FALSE);

  options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  image   = clay_display_get_image (display);
  shell   = clay_display_get_shell (display);

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  switch (options->type)
    {
    case CLAY_TRANSFORM_TYPE_LAYER:
      {
        GList *drawables;
        GList *iter;
        gint   x      = G_MAXINT;
        gint   y      = G_MAXINT;
        gint   width  = G_MININT;
        gint   height = G_MININT;

        drawables = clay_image_get_selected_drawables (image);
        non_empty = FALSE;

        for (iter = drawables; iter; iter = iter->next)
          {
            gint     offset_x;
            gint     offset_y;
            gint     mask_off_x;
            gint     mask_off_y;
            gint     mask_width;
            gint     mask_height;
            gboolean has_pixels;

            clay_item_get_offset (CLAY_ITEM (iter->data), &offset_x,
                                  &offset_y);
            /* This is the bounds relatively to the drawable. */
            has_pixels = clay_item_mask_intersect (CLAY_ITEM (iter->data),
                                                   &mask_off_x, &mask_off_y,
                                                   &mask_width, &mask_height);

            if (has_pixels)
              non_empty = TRUE;

            /* The bounds relatively to the image. */
            x      = MIN (offset_x + mask_off_x, x);
            y      = MIN (offset_y + mask_off_y, y);
            width  = MAX (offset_x + mask_off_x + mask_width, width);
            height = MAX (offset_y + mask_off_y + mask_height, height);
          }

        tr_tool->x1 = x;
        tr_tool->y1 = y;
        tr_tool->x2 = width;
        tr_tool->y2 = height;

        g_list_free (drawables);
      }
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
      {
        clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                          &tr_tool->x1, &tr_tool->y1,
                          &tr_tool->x2, &tr_tool->y2);
        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }
      break;

    case CLAY_TRANSFORM_TYPE_PATH:
      {
        ClayChannel *selection = clay_image_get_mask (image);

        /* if selection is not empty, use its bounds to perform the
         * transformation of the path
         */

        if (! clay_channel_is_empty (selection))
          {
            clay_item_bounds (CLAY_ITEM (selection),
                              &tr_tool->x1, &tr_tool->y1,
                              &tr_tool->x2, &tr_tool->y2);

            tr_tool->x2 += tr_tool->x1;
            tr_tool->y2 += tr_tool->y1;
          }
        else
          {
            GList *iter;

            /* without selection, test the emptiness of the path bounds :
             * if empty, use the canvas bounds
             * else use the path bounds
             */

            tr_tool->x1 = G_MAXINT;
            tr_tool->y1 = G_MAXINT;
            tr_tool->x2 = G_MININT;
            tr_tool->y2 = G_MININT;

            for (iter = clay_image_get_selected_paths (image); iter; iter = iter->next)
              {
                ClayItem *item   = iter->data;
                gint      x;
                gint      y;
                gint      width;
                gint      height;

                if (clay_item_bounds (item, &x, &y, &width, &height))
                  {
                    tr_tool->x1 = MIN (tr_tool->x1, x);
                    tr_tool->y1 = MIN (tr_tool->y1, y);
                    tr_tool->x2 = MAX (tr_tool->x2, x + width);
                    tr_tool->y2 = MAX (tr_tool->y2, y + height);
                  }
              }

            if (tr_tool->x2 <= tr_tool->x1 || tr_tool->y2 <= tr_tool->y1)
              {
                tr_tool->x1 = 0;
                tr_tool->y1 = 0;
                tr_tool->x2 = clay_image_get_width (image);
                tr_tool->y2 = clay_image_get_height (image);
              }
          }
      }

      break;

    case CLAY_TRANSFORM_TYPE_IMAGE:
      if (! shell->show_all)
        {
          tr_tool->x1 = 0;
          tr_tool->y1 = 0;
          tr_tool->x2 = clay_image_get_width  (image);
          tr_tool->y2 = clay_image_get_height (image);
        }
      else
        {
          GeglRectangle bounding_box;

          bounding_box = clay_display_shell_get_bounding_box (shell);

          tr_tool->x1 = bounding_box.x;
          tr_tool->y1 = bounding_box.y;
          tr_tool->x2 = bounding_box.x + bounding_box.width;
          tr_tool->y2 = bounding_box.y + bounding_box.height;
        }
      break;
    }

  return non_empty;
}

void
clay_transform_tool_recalc_matrix (ClayTransformTool *tr_tool,
                                   ClayDisplay       *display)
{
  g_return_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  if (tr_tool->x1 == tr_tool->x2 && tr_tool->y1 == tr_tool->y2)
    clay_transform_tool_bounds (tr_tool, display);

  if (CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool);
}

GList *
clay_transform_tool_get_selected_objects (ClayTransformTool  *tr_tool,
                                          ClayDisplay        *display)
{
  ClayTransformOptions *options;
  ClayImage            *image;
  GList                *objects = NULL;

  g_return_val_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  image = clay_display_get_image (display);

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  if (tr_tool->objects)
    return g_list_copy (tr_tool->objects);

  switch (options->type)
    {
    case CLAY_TRANSFORM_TYPE_LAYER:
      objects = clay_image_get_selected_drawables (image);
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
      if (! clay_channel_is_empty (clay_image_get_mask (image)))
        objects = g_list_prepend (NULL, clay_image_get_mask (image));
      break;

    case CLAY_TRANSFORM_TYPE_PATH:
      objects = g_list_copy (clay_image_get_selected_paths (image));
      break;

    case CLAY_TRANSFORM_TYPE_IMAGE:
      objects = g_list_prepend (NULL, image);
      break;
    }

  return objects;
}

GList *
clay_transform_tool_check_selected_objects (ClayTransformTool  *tr_tool,
                                            ClayDisplay        *display,
                                            GError            **error)
{
  ClayTransformOptions *options;
  GList                *objects;
  GList                *iter;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;
  ClayItem             *locked_item    = NULL;
  ClayGuiConfig        *config         = CLAY_GUI_CONFIG (display->clay->config);

  g_return_val_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  objects = clay_transform_tool_get_selected_objects (tr_tool, display);

  switch (options->type)
    {
    case CLAY_TRANSFORM_TYPE_LAYER:
      null_message = _("There is no layer to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          ClayItem *item = iter->data;

          if (clay_item_is_content_locked (item, &locked_item))
            locked_message = _("A selected layer's pixels are locked.");
          else if (clay_item_is_position_locked (item, &locked_item))
            locked_message = _("A selected layer's position and size are locked.");

          if (! clay_item_is_visible (item) &&
              ! config->edit_non_visible &&
              ! g_list_find (tr_tool->objects, item)) /* see bug #759194 */
            {
              g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                   _("A selected layer is not visible."));
              return NULL;
            }

          if (! clay_transform_tool_bounds (tr_tool, display))
            {
              g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                   _("The selection does not intersect with a selected layer."));
              return NULL;
            }
        }
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
      null_message = _("There is no selection to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          ClayItem *item = iter->data;

          /* cannot happen, so don't translate these messages */
          if (clay_item_is_content_locked (item, &locked_item))
            locked_message = "The selection's pixels are locked.";
          else if (clay_item_is_position_locked (item, &locked_item))
            locked_message = "The selection's position and size are locked.";
        }
      break;

    case CLAY_TRANSFORM_TYPE_PATH:
      null_message = _("There is no path to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          ClayItem *item = iter->data;

          if (clay_item_is_content_locked (item, &locked_item))
            locked_message = _("The selected path's strokes are locked.");
          else if (clay_item_is_position_locked (item, &locked_item))
            locked_message = _("The selected path's position is locked.");
          else if (! clay_path_get_n_strokes (CLAY_PATH (item)))
            locked_message = _("The selected path has no strokes.");
        }
      break;

    case CLAY_TRANSFORM_TYPE_IMAGE:
      /* cannot happen, so don't translate this message */
      null_message = "There is no image to transform.";
      break;
    }

  if (! objects)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED, null_message);
      if (error)
        {
          clay_tools_show_tool_options (display->clay);
          clay_widget_blink (options->type_box);
        }
      return NULL;
    }

  if (locked_message)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED, locked_message);
      if (error)
        {
          if (locked_item == NULL)
            locked_item = CLAY_ITEM (objects->data);

          clay_tools_blink_lock_box (display->clay, locked_item);
        }
      return NULL;
    }

  return objects;
}

gboolean
clay_transform_tool_transform (ClayTransformTool *tr_tool,
                               ClayDisplay       *display)
{
  ClayTool             *tool;
  ClayTransformOptions *options;
  ClayImage            *image;
  GList                *selected_objects;
  GeglBuffer           *orig_buffer   = NULL;
  gint                  orig_offset_x = 0;
  gint                  orig_offset_y = 0;
  GeglBuffer           *new_buffer;
  gint                  new_offset_x;
  gint                  new_offset_y;
  ClayColorProfile     *buffer_profile;
  gchar                *undo_desc     = NULL;
  gboolean              new_layer     = FALSE;
  GError               *error         = NULL;

  g_return_val_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  tool    = CLAY_TOOL (tr_tool);
  options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tool);
  image   = clay_display_get_image (display);

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  selected_objects = clay_transform_tool_check_selected_objects (tr_tool, display,
                                                                 &error);

  if (! selected_objects)
    {
      clay_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  clay_transform_tool_recalc_matrix (tr_tool, display);

  if (! tr_tool->transform_valid)
    {
      clay_tool_message_literal (tool, display,
                                 _("The current transform is invalid"));
      return FALSE;
    }

  if (CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix &&
      clay_matrix3_is_identity (&tr_tool->transform))
    {
      /* No need to commit an identity transformation! */
      return TRUE;
    }

  if (! clay_transform_tool_confirm (tr_tool, display))
    return FALSE;

  clay_set_busy (display->clay);

  /*  We're going to dirty this image, but we want to keep the tool around  */
  clay_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case CLAY_TRANSFORM_TYPE_LAYER:
      {
        ClayChannel *mask      = clay_image_get_mask (image);
        gboolean     needs_cut = FALSE;
        GList       *iter;

        /*  cut if any one of them needs it, so that a mixed selection
         *  keeps behaving as it did
         */
        for (iter = selected_objects; iter; iter = iter->next)
          {
            if (clay_drawable_transform_needs_cut (iter->data, mask))
              needs_cut = TRUE;
          }

        if (needs_cut)
          {
            orig_buffer = clay_drawable_transform_cut (
              selected_objects,
              CLAY_CONTEXT (options),
              &orig_offset_x,
              &orig_offset_y,
              &new_layer);
          }
      }
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
    case CLAY_TRANSFORM_TYPE_PATH:
    case CLAY_TRANSFORM_TYPE_IMAGE:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (
    tr_tool,
    selected_objects,
    orig_buffer,
    orig_offset_x,
    orig_offset_y,
    &buffer_profile,
    &new_offset_x,
    &new_offset_y);

  if (orig_buffer)
    g_object_unref (orig_buffer);

  switch (options->type)
    {
    case CLAY_TRANSFORM_TYPE_LAYER:
      if (new_buffer)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          clay_drawable_transform_paste (CLAY_DRAWABLE (selected_objects->data),
                                         new_buffer, buffer_profile,
                                         new_offset_x, new_offset_y,
                                         new_layer, TRUE);
          g_object_unref (new_buffer);
        }
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
    case CLAY_TRANSFORM_TYPE_PATH:
    case CLAY_TRANSFORM_TYPE_IMAGE:
      /*  Nothing to be done  */
      break;
    }

  clay_image_undo_group_end (image);

  /*  We're done dirtying the image, and would like to be restarted if
   *  the image gets dirty while the tool exists
   */
  clay_tool_control_pop_preserve (tool->control);

  clay_unset_busy (display->clay);

  clay_image_flush (image);

  g_list_free (selected_objects);

  return TRUE;
}

void
clay_transform_tool_set_type (ClayTransformTool *tr_tool,
                              ClayTransformType  type)
{
  ClayTransformOptions *options;

  g_return_if_fail (CLAY_IS_TRANSFORM_TOOL (tr_tool));

  options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if (! tr_tool->restore_type)
    tr_tool->saved_type = options->type;

  tr_tool->restore_type = FALSE;

  g_object_set (options,
                "type", type,
                NULL);

  tr_tool->restore_type = TRUE;
}
