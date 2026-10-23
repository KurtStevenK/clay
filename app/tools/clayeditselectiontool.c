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
#include <stdarg.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/clayboundary.h"
#include "core/claygrouplayer.h"
#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-item-list.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/clayprojection.h"
#include "core/clayselection.h"
#include "core/clayundostack.h"

#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/claydisplayshell-selection.h"
#include "display/claydisplayshell-transform.h"

#include "claydrawtool.h"
#include "clayeditselectiontool.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"
#include "tool_manager.h"

#include "clay-intl.h"


#define ARROW_VELOCITY 25


typedef struct _ClayEditSelectionTool      ClayEditSelectionTool;
typedef struct _ClayEditSelectionToolClass ClayEditSelectionToolClass;

struct _ClayEditSelectionTool
{
  ClayDrawTool        parent_instance;

  gdouble             start_x;         /*  Coords where button was pressed   */
  gdouble             start_y;

  gint                last_x;          /*  Last x and y coords               */
  gint                last_y;

  gint                current_x;       /*  Current x and y coords            */
  gint                current_y;

  gint                cuml_x;          /*  Cumulative changes to x and y     */
  gint                cuml_y;

  gint                sel_x;           /*  Bounding box of selection mask    */
  gint                sel_y;           /*  Bounding box of selection mask    */
  gint                sel_width;
  gint                sel_height;

  gint                num_segs_in;     /*  Num seg in selection boundary     */
  gint                num_segs_out;    /*  Num seg in selection boundary     */
  ClayBoundSeg       *segs_in;         /*  Pointer to the channel sel. segs  */
  ClayBoundSeg       *segs_out;        /*  Pointer to the channel sel. segs  */

  gdouble             center_x;        /*  Where to draw the mark of center  */
  gdouble             center_y;

  ClayTranslateMode   edit_mode;       /*  Translate the mask or layer?      */

  GList              *live_items;      /*  Items that are transformed live   */
  GList              *delayed_items;   /*  Items that are transformed later  */

  gboolean            first_move;      /*  Don't push undos after the first  */

  gboolean            propagate_release;

  gboolean            constrain;       /*  Constrain the movement            */

  gdouble             last_motion_x;   /*  Previous coords sent to _motion   */
  gdouble             last_motion_y;

  gboolean            saved_show_selection;
};

struct _ClayEditSelectionToolClass
{
  ClayDrawToolClass   parent_class;
};


static void       clay_edit_selection_tool_finalize            (GObject               *object);

static void       clay_edit_selection_tool_button_release      (ClayTool              *tool,
                                                                const ClayCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                ClayButtonReleaseType  release_type,
                                                                ClayDisplay           *display);
static void       clay_edit_selection_tool_motion              (ClayTool              *tool,
                                                                const ClayCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                ClayDisplay           *display);
static void       clay_edit_selection_tool_active_modifier_key (ClayTool              *tool,
                                                                GdkModifierType        key,
                                                                gboolean               press,
                                                                GdkModifierType        state,
                                                                ClayDisplay           *display);
static void       clay_edit_selection_tool_draw                (ClayDrawTool          *tool);

static GList    * clay_edit_selection_tool_get_selected_items  (ClayEditSelectionTool *edit_select,
                                                                ClayImage             *image);
static void       clay_edit_selection_tool_calc_coords         (ClayEditSelectionTool *edit_select,
                                                                ClayImage             *image,
                                                                gdouble                x,
                                                                gdouble                y);
static void       clay_edit_selection_tool_start_undo_group    (ClayEditSelectionTool *edit_select,
                                                                ClayImage             *image);


G_DEFINE_TYPE (ClayEditSelectionTool, clay_edit_selection_tool,
               CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_edit_selection_tool_parent_class


static void
clay_edit_selection_tool_class_init (ClayEditSelectionToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class   = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_class   = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = clay_edit_selection_tool_finalize;

  tool_class->button_release      = clay_edit_selection_tool_button_release;
  tool_class->motion              = clay_edit_selection_tool_motion;
  tool_class->active_modifier_key = clay_edit_selection_tool_active_modifier_key;

  draw_class->draw                = clay_edit_selection_tool_draw;
}

static void
clay_edit_selection_tool_init (ClayEditSelectionTool *edit_select)
{
  ClayTool *tool = CLAY_TOOL (edit_select);

  edit_select->first_move           = TRUE;
  edit_select->saved_show_selection = FALSE;

  clay_tool_control_set_active_modifiers (tool->control,
                                          CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
clay_edit_selection_tool_finalize (GObject *object)
{
  ClayEditSelectionTool *edit_select = CLAY_EDIT_SELECTION_TOOL (object);

  g_clear_pointer (&edit_select->segs_in, g_free);
  edit_select->num_segs_in = 0;

  g_clear_pointer (&edit_select->segs_out, g_free);
  edit_select->num_segs_out = 0;

  g_clear_pointer (&edit_select->live_items,    g_list_free);
  g_clear_pointer (&edit_select->delayed_items, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
clay_edit_selection_tool_start (ClayTool          *parent_tool,
                                ClayDisplay       *display,
                                const ClayCoords  *coords,
                                ClayTranslateMode  edit_mode,
                                gboolean           propagate_release)
{
  ClayEditSelectionTool *edit_select;
  ClayTool              *tool;
  ClayDisplayShell      *shell;
  ClayImage             *image;
  GList                 *selected_items;
  GList                 *iter;
  GList                 *list;
  gint                   off_x       = G_MAXINT;
  gint                   off_y       = G_MAXINT;

  edit_select = g_object_new (CLAY_TYPE_EDIT_SELECTION_TOOL,
                              "tool-info", parent_tool->tool_info,
                              NULL);

  edit_select->propagate_release = propagate_release;

  tool = CLAY_TOOL (edit_select);

  shell = clay_display_get_shell (display);
  image = clay_display_get_image (display);

  edit_select->saved_show_selection =
    clay_display_shell_get_show_selection (shell);

  /*  Make a check to see if it should be a floating selection translation  */
  if ((edit_mode == CLAY_TRANSLATE_MODE_MASK_TO_LAYER ||
       edit_mode == CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER) &&
      clay_image_get_floating_selection (image))
    {
      edit_mode = CLAY_TRANSLATE_MODE_FLOATING_SEL;
    }

  if (edit_mode == CLAY_TRANSLATE_MODE_LAYER)
    {
      GList *layers = clay_image_get_selected_layers (image);

      if (layers && clay_layer_is_floating_sel (layers->data))
        edit_mode = CLAY_TRANSLATE_MODE_FLOATING_SEL;
    }

  /* Turn off "marching ants" when moving selections or floating layers
   * for better performance. */
  if (edit_mode == CLAY_TRANSLATE_MODE_FLOATING_SEL)
    clay_display_shell_set_show_selection (shell, FALSE);

  edit_select->edit_mode = edit_mode;

  clay_edit_selection_tool_start_undo_group (edit_select, image);

  /* Remember starting point for use in constrained movement */
  edit_select->start_x = coords->x;
  edit_select->start_y = coords->y;

  selected_items = clay_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      clay_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  /* Manually set the last coords to the starting point */
  edit_select->last_x = coords->x - off_x;
  edit_select->last_y = coords->y - off_y;

  edit_select->constrain = FALSE;

  /* Find the active item's selection bounds */
  {
    ClayChannel        *channel;
    const ClayBoundSeg *segs_in;
    const ClayBoundSeg *segs_out;

    if (CLAY_IS_CHANNEL (selected_items->data))
      channel = CLAY_CHANNEL (selected_items->data);
    else
      channel = clay_image_get_mask (image);

    clay_channel_boundary (channel,
                           &segs_in, &segs_out,
                           &edit_select->num_segs_in,
                           &edit_select->num_segs_out,
                           0, 0, 0, 0);

    edit_select->segs_in = g_memdup2 (segs_in,
                                      edit_select->num_segs_in *
                                      sizeof (ClayBoundSeg));

    edit_select->segs_out = g_memdup2 (segs_out,
                                       edit_select->num_segs_out *
                                       sizeof (ClayBoundSeg));

    if (edit_select->edit_mode == CLAY_TRANSLATE_MODE_PATH)
      {
        edit_select->sel_x      = 0;
        edit_select->sel_y      = 0;
        edit_select->sel_width  = clay_image_get_width  (image);
        edit_select->sel_height = clay_image_get_height (image);
      }
    else
      {
        /*  find the bounding box of the selection mask - this is used
         *  for the case of a CLAY_TRANSLATE_MODE_MASK_TO_LAYER, where
         *  the translation will result in floating the selection mask
         *  and translating the resulting layer
         */
        clay_item_mask_intersect (selected_items->data,
                                  &edit_select->sel_x,
                                  &edit_select->sel_y,
                                  &edit_select->sel_width,
                                  &edit_select->sel_height);
      }
  }

  clay_edit_selection_tool_calc_coords (edit_select, image,
                                        coords->x, coords->y);

  {
    gint x = 0;
    gint y = 0;
    gint w = 0;
    gint h = 0;

    switch (edit_select->edit_mode)
      {
      case CLAY_TRANSLATE_MODE_CHANNEL:
      case CLAY_TRANSLATE_MODE_MASK:
      case CLAY_TRANSLATE_MODE_LAYER_MASK:
        edit_select->delayed_items = clay_image_item_list_filter (g_list_copy (selected_items));
        clay_image_item_list_bounds (image, edit_select->delayed_items, &x, &y, &w, &h);
        x += off_x;
        y += off_y;

        break;

      case CLAY_TRANSLATE_MODE_MASK_TO_LAYER:
      case CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
        /* MASK_TO_LAYER and MASK_COPY_TO_LAYER create a live_item later */
        x = edit_select->sel_x + off_x;
        y = edit_select->sel_y + off_y;
        w = edit_select->sel_width;
        h = edit_select->sel_height;
        break;

      case CLAY_TRANSLATE_MODE_LAYER:
      case CLAY_TRANSLATE_MODE_FLOATING_SEL:
      case CLAY_TRANSLATE_MODE_PATH:
        edit_select->live_items = clay_image_item_list_filter (g_list_copy (selected_items));
        clay_image_item_list_bounds (image, edit_select->live_items, &x, &y, &w, &h);
        break;
      }

    clay_tool_control_set_snap_offsets (tool->control,
                                        x - coords->x,
                                        y - coords->y,
                                        w, h);

    /* Save where to draw the mark of the center */
    edit_select->center_x = x + w / 2.0;
    edit_select->center_y = y + h / 2.0;
  }

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      ClayItem *item = list->data;

      clay_viewable_preview_freeze (CLAY_VIEWABLE (item));

      clay_item_start_transform (item, TRUE);
    }

  g_list_free (selected_items);

  tool_manager_push_tool (display->clay, tool);

  clay_tool_control_activate (tool->control);
  tool->display = display;

  /*  pause the current selection  */
  clay_display_shell_selection_pause (shell);

  /* initialize the statusbar display */
  clay_tool_push_status_coords (tool, display,
                                clay_tool_control_get_precision (tool->control),
                                _("Move: "), 0, ", ", 0, NULL);

  clay_draw_tool_start (CLAY_DRAW_TOOL (edit_select), display);
}


static void
clay_edit_selection_tool_button_release (ClayTool              *tool,
                                         const ClayCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         ClayButtonReleaseType  release_type,
                                         ClayDisplay           *display)
{
  ClayEditSelectionTool *edit_select = CLAY_EDIT_SELECTION_TOOL (tool);
  ClayDisplayShell      *shell       = clay_display_get_shell (display);
  ClayImage             *image       = clay_display_get_image (display);
  GList                 *list;

  /*  resume the current selection  */
  clay_display_shell_selection_resume (shell);

  clay_tool_pop_status (tool, display);

  clay_tool_control_halt (tool->control);

  /*  Stop and free the selection core  */
  clay_draw_tool_stop (CLAY_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (display->clay);

  /* move the items -- whether there has been movement or not!
   * (to ensure that there's something on the undo stack)
   */
  clay_image_item_list_translate (image,
                                  edit_select->delayed_items,
                                  edit_select->cuml_x,
                                  edit_select->cuml_y,
                                  TRUE);

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      ClayItem *item = list->data;

      clay_item_end_transform (item, TRUE);

      clay_viewable_preview_thaw (CLAY_VIEWABLE (item));
    }

  clay_image_undo_group_end (image);

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL ||
      (edit_select->cuml_x == 0 && edit_select->cuml_y == 0))
    {
      /* Operation cancelled or no movement - undo the undo-group! */
      clay_image_undo (image);
    }

  clay_image_flush (image);

  if (edit_select->propagate_release &&
      tool_manager_get_active (display->clay))
    {
      tool_manager_button_release_active (display->clay,
                                          coords, time, state,
                                          display);
    }

  g_object_unref (edit_select);

  shell->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;
  shell->equidistance_side_vertical   = CLAY_ARRANGE_HFILL;
  shell->snapped_side_horizontal      = CLAY_ARRANGE_HFILL;
  shell->snapped_side_vertical        = CLAY_ARRANGE_HFILL;

  clay_display_shell_set_show_selection (shell,
                                         edit_select->saved_show_selection);
}

static void
clay_edit_selection_tool_update_motion (ClayEditSelectionTool *edit_select,
                                        gdouble                new_x,
                                        gdouble                new_y,
                                        ClayDisplay           *display)
{
  ClayDrawTool     *draw_tool = CLAY_DRAW_TOOL (edit_select);
  ClayTool         *tool      = CLAY_TOOL (edit_select);
  ClayDisplayShell *shell     = clay_display_get_shell (display);
  ClayImage        *image     = clay_display_get_image (display);
  gint              dx;
  gint              dy;

  gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (shell)));

  clay_draw_tool_pause (draw_tool);

  if (edit_select->constrain)
    {
      clay_constrain_line (edit_select->start_x, edit_select->start_y,
                           &new_x, &new_y,
                           CLAY_CONSTRAIN_LINE_45_DEGREES, 0.0, 1.0, 1.0);
    }

  clay_edit_selection_tool_calc_coords (edit_select, image,
                                        new_x, new_y);

  dx = edit_select->current_x - edit_select->last_x;
  dy = edit_select->current_y - edit_select->last_y;

  /*  if there has been movement, move  */
  if (dx != 0 || dy != 0)
    {
      GList  *selected_items;
      GList  *list;
      GError *error = NULL;

      selected_items = clay_edit_selection_tool_get_selected_items (edit_select, image);

      edit_select->cuml_x += dx;
      edit_select->cuml_y += dy;

      switch (edit_select->edit_mode)
        {
        case CLAY_TRANSLATE_MODE_LAYER_MASK:
        case CLAY_TRANSLATE_MODE_MASK:
        case CLAY_TRANSLATE_MODE_PATH:
        case CLAY_TRANSLATE_MODE_CHANNEL:
          edit_select->last_x = edit_select->current_x;
          edit_select->last_y = edit_select->current_y;

          /*  fallthru  */

        case CLAY_TRANSLATE_MODE_LAYER:
          clay_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;

        case CLAY_TRANSLATE_MODE_MASK_TO_LAYER:
        case CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
          if (! clay_selection_float (CLAY_SELECTION (clay_image_get_mask (image)),
                                      selected_items,
                                      clay_get_user_context (display->clay),
                                      edit_select->edit_mode ==
                                      CLAY_TRANSLATE_MODE_MASK_TO_LAYER,
                                      0, 0, &error))
            {
              /* no region to float, abort safely */
              clay_message_literal (display->clay, G_OBJECT (display),
                                    CLAY_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
              clay_draw_tool_resume (draw_tool);

              return;
            }

          edit_select->last_x -= edit_select->sel_x;
          edit_select->last_y -= edit_select->sel_y;
          edit_select->sel_x   = 0;
          edit_select->sel_y   = 0;

          edit_select->edit_mode = CLAY_TRANSLATE_MODE_FLOATING_SEL;

          /* Selected items should have changed. */
          edit_select->live_items = clay_edit_selection_tool_get_selected_items (edit_select, image);

          for (list = edit_select->live_items; list; list = g_list_next (list))
            {
              ClayItem *item = list->data;

              clay_viewable_preview_freeze (CLAY_VIEWABLE (item));

              clay_item_start_transform (item, TRUE);
            }

          /*  fallthru  */

        case CLAY_TRANSLATE_MODE_FLOATING_SEL:
          clay_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;
        }

      edit_select->first_move = FALSE;

      g_list_free (selected_items);
    }

  clay_projection_flush (clay_image_get_projection (image));

  clay_tool_pop_status (tool, display);
  clay_tool_push_status_coords (tool, display,
                                clay_tool_control_get_precision (tool->control),
                                _("Move: "),
                                edit_select->cuml_x,
                                ", ",
                                edit_select->cuml_y,
                                NULL);

  clay_draw_tool_resume (draw_tool);
}


static void
clay_edit_selection_tool_motion (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 ClayDisplay      *display)
{
  ClayEditSelectionTool *edit_select = CLAY_EDIT_SELECTION_TOOL (tool);

  edit_select->last_motion_x = coords->x;
  edit_select->last_motion_y = coords->y;

  clay_edit_selection_tool_update_motion (edit_select,
                                          coords->x, coords->y,
                                          display);
}

static void
clay_edit_selection_tool_active_modifier_key (ClayTool        *tool,
                                              GdkModifierType  key,
                                              gboolean         press,
                                              GdkModifierType  state,
                                              ClayDisplay     *display)
{
  ClayEditSelectionTool *edit_select = CLAY_EDIT_SELECTION_TOOL (tool);

  edit_select->constrain = (state & clay_get_constrain_behavior_mask () ?
                            TRUE : FALSE);

  /* If we didn't came here due to a mouse release and we've moved before,
   * immediately update the position of the thing we move.
   */
  if ((state & GDK_BUTTON1_MASK) &&
      ! edit_select->first_move)
    {
      clay_edit_selection_tool_update_motion (edit_select,
                                              edit_select->last_motion_x,
                                              edit_select->last_motion_y,
                                              display);
    }
}

static void
clay_edit_selection_tool_draw (ClayDrawTool *draw_tool)
{
  ClayEditSelectionTool *edit_select  = CLAY_EDIT_SELECTION_TOOL (draw_tool);
  ClayDisplay           *display      = CLAY_TOOL (draw_tool)->display;
  ClayImage             *image        = clay_display_get_image (display);
  ClayDisplayShell      *shell        = clay_display_get_shell (display);
  ClayUnit              *unit         = clay_display_shell_get_unit (shell);
  const gchar           *abbreviation = clay_unit_get_abbreviation (unit);
  GList                 *selected_items;
  GList                 *iter;
  gdouble                xresolution, yresolution;
  gint                   off_x        = G_MAXINT;
  gint                   off_y        = G_MAXINT;

  clay_image_get_resolution (image, &xresolution, &yresolution);
  selected_items = clay_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      clay_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  switch (edit_select->edit_mode)
    {
    case CLAY_TRANSLATE_MODE_CHANNEL:
    case CLAY_TRANSLATE_MODE_LAYER_MASK:
    case CLAY_TRANSLATE_MODE_MASK:
      {
        gboolean floating_sel = FALSE;

        if (edit_select->edit_mode == CLAY_TRANSLATE_MODE_MASK)
          {
            GList *layers = clay_image_get_selected_layers (image);

            if (g_list_length (layers) == 1)
              floating_sel = clay_layer_is_floating_sel (layers->data);
          }

        if (! floating_sel && edit_select->segs_in)
          {
            clay_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_in,
                                         edit_select->num_segs_in,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }

        if (edit_select->segs_out)
          {
            clay_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_out,
                                         edit_select->num_segs_out,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }
        else if (edit_select->edit_mode != CLAY_TRANSLATE_MODE_MASK)
          {
            clay_draw_tool_add_rectangle (draw_tool,
                                          FALSE,
                                          edit_select->cuml_x + off_x,
                                          edit_select->cuml_y + off_y,
                                          clay_item_get_width  (selected_items->data),
                                          clay_item_get_height (selected_items->data));
          }
      }
      break;

    case CLAY_TRANSLATE_MODE_MASK_TO_LAYER:
    case CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      clay_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    edit_select->sel_x + off_x,
                                    edit_select->sel_y + off_y,
                                    edit_select->sel_width,
                                    edit_select->sel_height);
      break;

    case CLAY_TRANSLATE_MODE_LAYER:
    case CLAY_TRANSLATE_MODE_PATH:
        {
          GList             *translate_items;
          ClayAlignmentType  snapped_side_horizontal      = shell->snapped_side_horizontal;
          ClayAlignmentType  snapped_side_vertical        = shell->snapped_side_vertical;
          ClayAlignmentType  equidistance_side_horizontal = shell->equidistance_side_horizontal;
          ClayAlignmentType  equidistance_side_vertical   = shell->equidistance_side_vertical;
          gint               x, y, w, h;

          translate_items = clay_image_item_list_filter (g_list_copy (selected_items));
          clay_image_item_list_bounds (image, translate_items, &x, &y, &w, &h);
          g_list_free (translate_items);

          clay_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

          if (snapped_side_horizontal != CLAY_ARRANGE_HFILL)
            {
              ClayLayer *snapped_layer_horizontal = shell->snapped_layer_horizontal;
              gint       gx, gy, gw, gh;

              clay_item_bounds (CLAY_ITEM (snapped_layer_horizontal), &gx, &gy, &gw, &gh);
              clay_item_get_offset (CLAY_ITEM (snapped_layer_horizontal), &gx, &gy);

              switch (snapped_side_horizontal)
                {
                case CLAY_ALIGN_LEFT:
                  clay_draw_tool_add_line (draw_tool, x, y+h/2, x, gy+gh/2);
                  break;
                case CLAY_ALIGN_RIGHT:
                  clay_draw_tool_add_line (draw_tool, x+w, y+h/2, x+w, gy+gh/2);
                  break;
                case CLAY_ALIGN_VCENTER:
                  clay_draw_tool_add_line (draw_tool, x+w/2, y+h/2, x+w/2, gy+gh/2);
                  break;

                default:
                  break;
                }
            }

          if (snapped_side_vertical != CLAY_ARRANGE_HFILL)
            {
              ClayLayer *snapped_layer_vertical = shell->snapped_layer_vertical;
              gint       gx, gy, gw, gh;

              clay_item_bounds (CLAY_ITEM (snapped_layer_vertical), &gx, &gy, &gw, &gh);
              clay_item_get_offset (CLAY_ITEM (snapped_layer_vertical), &gx, &gy);

              switch (snapped_side_vertical)
                {
                case CLAY_ALIGN_TOP:
                  clay_draw_tool_add_line (draw_tool, x+w/2, y, gx+gw/2, y);
                  break;
                case CLAY_ALIGN_BOTTOM:
                  clay_draw_tool_add_line (draw_tool, x+w/2, y+h, gx+gw/2, y+h);
                  break;
                case CLAY_ALIGN_HCENTER:
                  clay_draw_tool_add_line (draw_tool, x+w/2, y+h/2, gx+gw/2, y+h/2);
                  break;

                default:
                  break;
                }
            }

          if (equidistance_side_horizontal != CLAY_ARRANGE_HFILL)
            {
              ClayLayer       *near_layer1 = shell->near_layer_horizontal1;
              ClayLayer       *near_layer2 = shell->near_layer_horizontal2;
              GtkStyleContext *style       = gtk_widget_get_style_context (GTK_WIDGET (shell));
              gdouble          pixels_to_units;
              gdouble          font_size;
              gint             gx1 = 0;
              gint             gy1 = 0;
              gint             gw1 = 0;
              gint             gh1 = 0;
              gint             gx2 = 0;
              gint             gy2 = 0;
              gint             gw2 = 0;
              gint             gh2 = 0;
              gint             distance;
              gchar            distance_string[32];

              gtk_style_context_get (style, gtk_style_context_get_state (style),
                                     "font-size", &font_size, NULL);

              clay_item_bounds (CLAY_ITEM (near_layer1), &gx1, &gy1, &gw1, &gh1);
              clay_item_get_offset (CLAY_ITEM (near_layer1), &gx1, &gy1);

              clay_item_bounds (CLAY_ITEM (near_layer2), &gx2, &gy2, &gw2, &gh2);
              clay_item_get_offset (CLAY_ITEM (near_layer2), &gx2, &gy2);

              clay_draw_tool_add_rectangle (draw_tool, FALSE, gx1, gy1, gw1, gh1);
              clay_draw_tool_add_rectangle (draw_tool, FALSE, gx2, gy2, gw2, gh2);

              switch (equidistance_side_horizontal)
                {
                case CLAY_ALIGN_LEFT:
                  distance = x - (gx1+gw1);
                  pixels_to_units = clay_pixels_to_units ((gdouble) distance, unit, xresolution);

                  clay_draw_tool_add_line (draw_tool, x, y+h/2, gx1+gw1, y+h/2);
                  clay_draw_tool_add_line (draw_tool, gx1, gy1+gh1/2, gx2+gw2, gy1+gh1/2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  clay_draw_tool_add_text (draw_tool, x - distance/2, y+h/2, font_size, distance_string);
                  clay_draw_tool_add_text (draw_tool, gx1 - distance/2, gy1+gh1/2, font_size, distance_string);

                  break;

                case CLAY_ALIGN_RIGHT:
                  distance = gx1 - (x+w);
                  pixels_to_units = clay_pixels_to_units ((gdouble) distance, unit, xresolution);

                  clay_draw_tool_add_line (draw_tool, x+w, y+h/2, gx1, y+h/2);
                  clay_draw_tool_add_line (draw_tool, gx1+gw1, gy1+gh1/2, gx2, gy1+gh1/2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  clay_draw_tool_add_text (draw_tool, (x+w) + distance/2, y+h/2, font_size, distance_string);
                  clay_draw_tool_add_text (draw_tool, (gx1+gw1) + distance/2, gy1+gh1/2, font_size, distance_string);

                  break;

                default:
                  break;
                }
            }

          if (equidistance_side_vertical != CLAY_ARRANGE_HFILL)
            {
              ClayLayer       *near_layer1 = shell->near_layer_vertical1;
              ClayLayer       *near_layer2 = shell->near_layer_vertical2;
              GtkStyleContext *style       = gtk_widget_get_style_context (GTK_WIDGET (shell));
              gdouble          pixels_to_units;
              gdouble          font_size;
              gint             gx1 = 0;
              gint             gy1 = 0;
              gint             gw1 = 0;
              gint             gh1 = 0;
              gint             gx2 = 0;
              gint             gy2 = 0;
              gint             gw2 = 0;
              gint             gh2 = 0;
              gint             distance;
              gchar            distance_string[32];

              gtk_style_context_get (style, gtk_style_context_get_state (style),
                                     "font-size", &font_size, NULL);

              clay_item_bounds (CLAY_ITEM (near_layer1), &gx1, &gy1, &gw1, &gh1);
              clay_item_get_offset (CLAY_ITEM (near_layer1), &gx1, &gy1);

              clay_item_bounds (CLAY_ITEM (near_layer2), &gx2, &gy2, &gw2, &gh2);
              clay_item_get_offset (CLAY_ITEM (near_layer2), &gx2, &gy2);

              clay_draw_tool_add_rectangle (draw_tool, FALSE, gx1, gy1, gw1, gh1);
              clay_draw_tool_add_rectangle (draw_tool, FALSE, gx2, gy2, gw2, gh2);

              switch (equidistance_side_vertical)
                {
                case CLAY_ALIGN_TOP:
                  distance = y - (gy1+gh1);
                  pixels_to_units = clay_pixels_to_units ((gdouble) distance, unit, yresolution);

                  clay_draw_tool_add_line (draw_tool, x+w/2, y, x+w/2, gy1+gh1);
                  clay_draw_tool_add_line (draw_tool, gx1+gw1/2, gy1, gx1+gw1/2, gy2+gh2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  clay_draw_tool_add_text (draw_tool, x+w/2, y - distance/2, font_size, distance_string);
                  clay_draw_tool_add_text (draw_tool, gx1+gw1/2, gy1 - distance/2, font_size, distance_string);

                  break;

                case CLAY_ALIGN_BOTTOM:
                  distance = gy1 - (y+h);
                  pixels_to_units = clay_pixels_to_units ((gdouble) distance, unit, yresolution);

                  clay_draw_tool_add_line (draw_tool, x+w/2, y+h, x+w/2, gy1);
                  clay_draw_tool_add_line (draw_tool, gx1+gw1/2, gy1+gh1, gx1+gw1/2, gy2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  clay_draw_tool_add_text (draw_tool, x+w/2, (y+h) + distance/2, font_size, distance_string);
                  clay_draw_tool_add_text (draw_tool, gx1+gw1/2, (gy1+gh1) + distance/2, font_size, distance_string);

                  break;

                default:
                  break;
                }
            }
        }
      break;

    case CLAY_TRANSLATE_MODE_FLOATING_SEL:
      if (edit_select->segs_in)
        {
          clay_draw_tool_add_boundary (draw_tool,
                                       edit_select->segs_in,
                                       edit_select->num_segs_in,
                                       NULL,
                                       edit_select->cuml_x,
                                       edit_select->cuml_y);
        }
      break;
    }

  /* Mark the center because we snap to it */
  clay_draw_tool_add_handle (draw_tool,
                             CLAY_HANDLE_CROSS,
                             edit_select->center_x + edit_select->cuml_x,
                             edit_select->center_y + edit_select->cuml_y,
                             CLAY_TOOL_HANDLE_SIZE_SMALL,
                             CLAY_TOOL_HANDLE_SIZE_SMALL,
                             CLAY_HANDLE_ANCHOR_CENTER);

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  g_list_free (selected_items);
}

static GList *
clay_edit_selection_tool_get_selected_items (ClayEditSelectionTool *edit_select,
                                             ClayImage             *image)
{
  GList *selected_items;

  switch (edit_select->edit_mode)
    {
    case CLAY_TRANSLATE_MODE_PATH:
      selected_items = g_list_copy (clay_image_get_selected_paths (image));
      break;

    case CLAY_TRANSLATE_MODE_LAYER:
      selected_items = g_list_copy (clay_image_get_selected_layers (image));
      break;

    case CLAY_TRANSLATE_MODE_MASK:
      selected_items = g_list_prepend (NULL, clay_image_get_mask (image));
      break;

    default:
      selected_items = clay_image_get_selected_drawables (image);
      break;
    }

  return selected_items;
}

static void
clay_edit_selection_tool_calc_coords (ClayEditSelectionTool *edit_select,
                                      ClayImage             *image,
                                      gdouble                x,
                                      gdouble                y)
{
  GList    *selected_items;
  GList    *iter;
  gint      off_x = G_MAXINT;
  gint      off_y = G_MAXINT;
  gdouble   x1, y1;
  gdouble   dx, dy;

  selected_items = clay_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      clay_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  g_list_free (selected_items);

  dx = (x - off_x) - edit_select->last_x;
  dy = (y - off_y) - edit_select->last_y;

  x1 = edit_select->sel_x + dx;
  y1 = edit_select->sel_y + dy;

  edit_select->current_x = ((gint) floor (x1) -
                            (edit_select->sel_x - edit_select->last_x));
  edit_select->current_y = ((gint) floor (y1) -
                            (edit_select->sel_y - edit_select->last_y));
}

static void
clay_edit_selection_tool_start_undo_group (ClayEditSelectionTool *edit_select,
                                           ClayImage             *image)
{
  GList       *selected_items;
  const gchar *undo_desc = NULL;

  switch (edit_select->edit_mode)
    {
    case CLAY_TRANSLATE_MODE_PATH:
    case CLAY_TRANSLATE_MODE_CHANNEL:
    case CLAY_TRANSLATE_MODE_LAYER_MASK:
    case CLAY_TRANSLATE_MODE_MASK:
    case CLAY_TRANSLATE_MODE_LAYER:
      selected_items = clay_edit_selection_tool_get_selected_items (edit_select, image);
      undo_desc = CLAY_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      g_list_free (selected_items);
      break;

    case CLAY_TRANSLATE_MODE_MASK_TO_LAYER:
    case CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
    case CLAY_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      g_return_if_reached ();
    }

  clay_image_undo_group_start (image,
                               edit_select->edit_mode ==
                               CLAY_TRANSLATE_MODE_MASK ?
                               CLAY_UNDO_GROUP_MASK :
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               undo_desc);
}

static gint
process_event_queue_keys (GdkEventKey *kevent,
                          ... /* GdkKeyType, GdkModifierType, value ... 0 */)
{

#define FILTER_MAX_KEYS 50

  va_list          argp;
  GdkEvent        *event;
  GList           *event_list = NULL;
  GList           *list;
  guint            keys[FILTER_MAX_KEYS];
  GdkModifierType  modifiers[FILTER_MAX_KEYS];
  gint             values[FILTER_MAX_KEYS];
  gint             i      = 0;
  gint             n_keys = 0;
  gint             value  = 0;
  gboolean         done   = FALSE;
  GtkWidget       *orig_widget;

  va_start (argp, kevent);

  while (n_keys < FILTER_MAX_KEYS &&
         (keys[n_keys] = va_arg (argp, guint)) != 0)
    {
      modifiers[n_keys] = va_arg (argp, GdkModifierType);
      values[n_keys]    = va_arg (argp, gint);
      n_keys++;
    }

  va_end (argp);

  for (i = 0; i < n_keys; i++)
    if (kevent->keyval                 == keys[i] &&
        (kevent->state & modifiers[i]) == modifiers[i])
      value += values[i];

  orig_widget = gtk_get_event_widget ((GdkEvent *) kevent);

  while (gdk_events_pending () > 0 && ! done)
    {
      gboolean discard_event = FALSE;

      event = gdk_event_get ();

      if (! event || orig_widget != gtk_get_event_widget (event))
        {
          done = TRUE;
        }
      else
        {
          if (event->any.type == GDK_KEY_PRESS)
            {
              for (i = 0; i < n_keys; i++)
                if (event->key.keyval                 == keys[i] &&
                    (event->key.state & modifiers[i]) == modifiers[i])
                  {
                    discard_event = TRUE;
                    value += values[i];
                  }

              if (! discard_event)
                done = TRUE;
            }
          /* should there be more types here? */
          else if (event->any.type != GDK_KEY_RELEASE &&
                   event->any.type != GDK_MOTION_NOTIFY &&
                   event->any.type != GDK_EXPOSE)
            done = FALSE;
        }

      if (! event)
        ; /* Do nothing */
      else if (! discard_event)
        event_list = g_list_prepend (event_list, event);
      else
        gdk_event_free (event);
    }

  event_list = g_list_reverse (event_list);

  /* unget the unused events and free the list */
  for (list = event_list; list; list = g_list_next (list))
    {
      gdk_event_put ((GdkEvent *) list->data);
      gdk_event_free ((GdkEvent *) list->data);
    }

  g_list_free (event_list);

  return value;

#undef FILTER_MAX_KEYS
}

gboolean
clay_edit_selection_tool_key_press (ClayTool    *tool,
                                    GdkEventKey *kevent,
                                    ClayDisplay *display)
{
  ClayTransformType translate_type;

  if (kevent->state & GDK_MOD1_MASK)
    {
      translate_type = CLAY_TRANSFORM_TYPE_SELECTION;
    }
  else if (kevent->state & clay_get_toggle_behavior_mask ())
    {
      translate_type = CLAY_TRANSFORM_TYPE_PATH;
    }
  else
    {
      translate_type = CLAY_TRANSFORM_TYPE_LAYER;
    }

  return clay_edit_selection_tool_translate (tool, kevent, translate_type,
                                             display, NULL);
}

gboolean
clay_edit_selection_tool_translate (ClayTool          *tool,
                                    GdkEventKey       *kevent,
                                    ClayTransformType  translate_type,
                                    ClayDisplay       *display,
                                    GtkWidget        **type_box)
{
  gint               inc_x          = 0;
  gint               inc_y          = 0;
  ClayUndo          *undo;
  gboolean           push_undo      = TRUE;
  ClayImage         *image          = clay_display_get_image (display);
  ClayItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  ClayTranslateMode  edit_mode      = CLAY_TRANSLATE_MODE_MASK;
  ClayUndoType       undo_type      = CLAY_UNDO_GROUP_MASK;
  const gchar       *undo_desc      = NULL;
  GdkModifierType    extend_mask    = clay_get_extend_selection_mask ();
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  ClayItem          *locked_item    = NULL;
  GList             *iter;
  gint               velocity;

  /* bail out early if it is not an arrow key event */

  if (kevent->keyval != GDK_KEY_Left &&
      kevent->keyval != GDK_KEY_Right &&
      kevent->keyval != GDK_KEY_Up &&
      kevent->keyval != GDK_KEY_Down)
    return FALSE;

  /*  adapt arrow velocity to the zoom factor when holding <shift>  */
  velocity = (ARROW_VELOCITY /
              clay_zoom_model_get_factor (clay_display_get_shell (display)->zoom));
  velocity = MAX (1.0, velocity);

  /*  check the event queue for key events with the same modifier mask
   *  as the current event, allowing only extend_mask to vary between
   *  them.
   */
  inc_x = process_event_queue_keys (kevent,
                                    GDK_KEY_Left,
                                    kevent->state | extend_mask,
                                    -1 * velocity,

                                    GDK_KEY_Left,
                                    kevent->state & ~extend_mask,
                                    -1,

                                    GDK_KEY_Right,
                                    kevent->state | extend_mask,
                                    1 * velocity,

                                    GDK_KEY_Right,
                                    kevent->state & ~extend_mask,
                                    1,

                                    0);

  inc_y = process_event_queue_keys (kevent,
                                    GDK_KEY_Up,
                                    kevent->state | extend_mask,
                                    -1 * velocity,

                                    GDK_KEY_Up,
                                    kevent->state & ~extend_mask,
                                    -1,

                                    GDK_KEY_Down,
                                    kevent->state | extend_mask,
                                    1 * velocity,

                                    GDK_KEY_Down,
                                    kevent->state & ~extend_mask,
                                    1,

                                    0);

  switch (translate_type)
    {
    case CLAY_TRANSFORM_TYPE_SELECTION:
      active_item = CLAY_ITEM (clay_image_get_mask (image));

      if (clay_channel_is_empty (CLAY_CHANNEL (active_item)))
        active_item = NULL;

      edit_mode = CLAY_TRANSLATE_MODE_MASK;
      undo_type = CLAY_UNDO_GROUP_MASK;

      if (! active_item)
        {
          /* cannot happen, don't translate this message */
          null_message = "There is no selection to move.";
        }
      else if (clay_item_is_position_locked (active_item,
                                             &locked_item))
        {
          /* cannot happen, don't translate this message */
          locked_message = "The selection's position is locked.";
        }
      break;

    case CLAY_TRANSFORM_TYPE_PATH:
      selected_items = clay_image_get_selected_paths (image);
      selected_items = g_list_copy (selected_items);

      edit_mode = CLAY_TRANSLATE_MODE_PATH;
      undo_type = CLAY_UNDO_GROUP_ITEM_DISPLACE;

      if (selected_items == NULL)
        {
          null_message = _("There are no paths to move.");
        }
      else
        {
          for (iter = selected_items; iter; iter = iter->next)
            if (clay_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected path's position is locked.");
                break;
              }
        }
      break;

    case CLAY_TRANSFORM_TYPE_LAYER:
      selected_items = clay_image_get_selected_drawables (image);

      undo_type = CLAY_UNDO_GROUP_ITEM_DISPLACE;

      if (! selected_items)
        {
          null_message = _("There is no layer to move.");
        }
      else if (CLAY_IS_LAYER_MASK (selected_items->data))
        {
          edit_mode = CLAY_TRANSLATE_MODE_LAYER_MASK;

          if (clay_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
          else if (clay_item_is_content_locked (selected_items->data,
                                                &locked_item))
            {
              locked_message = _("The selected layer's pixels are locked.");
            }
        }
      else if (CLAY_IS_CHANNEL (selected_items->data))
        {
          edit_mode = CLAY_TRANSLATE_MODE_CHANNEL;

          for (iter = selected_items; iter; iter = iter->next)
            if (clay_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected channel's position is locked.");
                break;
              }
        }
      else if (clay_layer_is_floating_sel (selected_items->data))
        {
          edit_mode = CLAY_TRANSLATE_MODE_FLOATING_SEL;

          if (clay_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
        }
      else
        {
          edit_mode = CLAY_TRANSLATE_MODE_LAYER;

          for (iter = selected_items; iter; iter = iter->next)
            if (clay_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected layer's position is locked.");
                break;
              }
        }

      break;

    case CLAY_TRANSFORM_TYPE_IMAGE:
      g_return_val_if_reached (FALSE);
    }

  if (! active_item && ! selected_items)
    {
      clay_tool_message_literal (tool, display, null_message);
      if (type_box)
        {
          clay_tools_show_tool_options (display->clay);
          if (*type_box)
            clay_widget_blink (*type_box);
        }
      return TRUE;
    }
  else if (locked_message)
    {
      clay_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      clay_tools_blink_lock_box (display->clay, locked_item);

      g_list_free (selected_items);
      return TRUE;
    }

  if (inc_x == 0 && inc_y == 0)
    {
      g_list_free (selected_items);
      return TRUE;
    }
  else if (! selected_items)
    {
      selected_items = g_list_prepend (NULL, active_item);
    }

  switch (edit_mode)
    {
    case CLAY_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      undo_desc = CLAY_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      break;
    }

  /* compress undo */
  undo = clay_image_undo_can_compress (image, CLAY_TYPE_UNDO_STACK, undo_type);

  if (undo &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-tool") == (gpointer) tool &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-type") == GINT_TO_POINTER (edit_mode))
    {
      GList *undo_items;

      undo_items = g_object_get_data (G_OBJECT (undo), "edit-selection-items");

      if (clay_image_equal_selected_drawables (image, undo_items))
        {
          push_undo = FALSE;
        }
    }

  if (push_undo)
    {
      if (clay_image_undo_group_start (image, undo_type, undo_desc))
        {
          undo = clay_image_undo_can_compress (image,
                                               CLAY_TYPE_UNDO_STACK,
                                               undo_type);

          if (undo)
            {
              g_object_set_data (G_OBJECT (undo), "edit-selection-tool",
                                 tool);
              g_object_set_data_full (G_OBJECT (undo), "edit-selection-items",
                                      g_list_copy (selected_items),
                                      (GDestroyNotify) g_list_free);
              g_object_set_data (G_OBJECT (undo), "edit-selection-type",
                                 GINT_TO_POINTER (edit_mode));
            }
        }
    }

  clay_image_item_list_translate (clay_item_get_image (selected_items->data),
                                  selected_items, inc_x, inc_y, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
  else
    clay_undo_refresh_preview (undo,
                               clay_get_user_context (display->clay));

  clay_image_flush (image);
  g_list_free (selected_items);

  return TRUE;
}
