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

#include "display-types.h"

#include "config/clayguiconfig.h"

#include "core/clayimage.h"

#include "widgets/claycursor.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydockcontainer.h"
#include "widgets/claysessioninfo.h"

#include "claycanvascursor.h"
#include "claydisplay.h"
#include "claycursorview.h"
#include "claydisplayshell.h"
#include "claydisplayshell-cursor.h"
#include "claydisplayshell-transform.h"
#include "claystatusbar.h"


static void  clay_display_shell_real_set_cursor (ClayDisplayShell   *shell,
                                                 ClayCursorType      cursor_type,
                                                 ClayToolCursorType  tool_cursor,
                                                 ClayCursorModifier  modifier,
                                                 gboolean            always_install);


/*  public functions  */

void
clay_display_shell_set_cursor (ClayDisplayShell   *shell,
                               ClayCursorType      cursor_type,
                               ClayToolCursorType  tool_cursor,
                               ClayCursorModifier  modifier)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      clay_display_shell_real_set_cursor (shell,
                                          cursor_type,
                                          tool_cursor,
                                          modifier,
                                          FALSE);
    }
}

void
clay_display_shell_unset_cursor (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      clay_display_shell_real_set_cursor (shell,
                                          (ClayCursorType) -1, 0, 0, FALSE);
    }
}

void
clay_display_shell_set_override_cursor (ClayDisplayShell *shell,
                                        ClayCursorType    cursor_type)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor ||
      (shell->using_override_cursor &&
       shell->override_cursor != cursor_type))
    {
      shell->override_cursor       = cursor_type;
      shell->using_override_cursor = TRUE;

      clay_cursor_set (shell->canvas,
                       shell->cursor_handedness,
                       cursor_type,
                       CLAY_TOOL_CURSOR_NONE,
                       CLAY_CURSOR_MODIFIER_NONE);
    }
}

void
clay_display_shell_unset_override_cursor (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (shell->using_override_cursor)
    {
      shell->using_override_cursor = FALSE;

      clay_display_shell_real_set_cursor (shell,
                                          shell->current_cursor,
                                          shell->tool_cursor,
                                          shell->cursor_modifier,
                                          TRUE);
    }
}

void
clay_display_shell_update_software_cursor (ClayDisplayShell    *shell,
                                           ClayCursorPrecision  precision,
                                           gint                 display_x,
                                           gint                 display_y,
                                           gdouble              image_x,
                                           gdouble              image_y)
{
  ClayImageWindow   *image_window;
  ClayDialogFactory *factory;
  ClayStatusbar     *statusbar;
  GtkWidget         *widget;
  ClayImage         *image;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  image = clay_display_get_image (shell->display);

  if (shell->draw_cursor &&
      shell->proximity   &&
      display_x >= 0     &&
      display_y >= 0)
    {
      clay_canvas_item_begin_change (shell->cursor);

      clay_canvas_cursor_set (shell->cursor,
                              display_x,
                              display_y);
      clay_canvas_item_set_visible (shell->cursor, TRUE);

      clay_canvas_item_end_change (shell->cursor);
    }
  else
    {
      clay_canvas_item_set_visible (shell->cursor, FALSE);
    }

  /*  use the passed image_coords for the statusbar because they are
   *  possibly snapped...
   */
  statusbar = clay_display_shell_get_statusbar (shell);

  clay_statusbar_update_cursor (statusbar, precision, image_x, image_y);

  image_window = clay_display_shell_get_window (shell);
  factory = clay_dock_container_get_dialog_factory (CLAY_DOCK_CONTAINER (image_window));

  widget = clay_dialog_factory_find_widget (factory, "clay-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        {
          gint t_x = -1;
          gint t_y = -1;

          /*  ...but use the unsnapped display_coords for the info window  */
          if (display_x >= 0 && display_y >= 0)
            clay_display_shell_untransform_xy (shell, display_x, display_y,
                                               &t_x, &t_y, FALSE);

          clay_cursor_view_update_cursor (CLAY_CURSOR_VIEW (cursor_view),
                                          image, shell->unit, shell->show_all,
                                          t_x, t_y);
        }
    }
}

void
clay_display_shell_clear_software_cursor (ClayDisplayShell *shell)
{
  ClayImageWindow   *image_window;
  ClayDialogFactory *factory;
  ClayStatusbar     *statusbar;
  GtkWidget         *widget;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  clay_canvas_item_set_visible (shell->cursor, FALSE);

  statusbar = clay_display_shell_get_statusbar (shell);

  clay_statusbar_clear_cursor (statusbar);

  image_window = clay_display_shell_get_window (shell);
  factory = clay_dock_container_get_dialog_factory (CLAY_DOCK_CONTAINER (image_window));

  widget = clay_dialog_factory_find_widget (factory, "clay-cursor-view");

  if (widget)
    {
      GtkWidget *cursor_view = gtk_bin_get_child (GTK_BIN (widget));

      if (cursor_view)
        clay_cursor_view_clear_cursor (CLAY_CURSOR_VIEW (cursor_view));
    }
}


/*  private functions  */

static void
clay_display_shell_real_set_cursor (ClayDisplayShell   *shell,
                                    ClayCursorType      cursor_type,
                                    ClayToolCursorType  tool_cursor,
                                    ClayCursorModifier  modifier,
                                    gboolean            always_install)
{
  ClayHandedness cursor_handedness;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (cursor_type == (ClayCursorType) -1)
    {
      shell->current_cursor = cursor_type;

      if (gtk_widget_is_drawable (shell->canvas))
        gdk_window_set_cursor (gtk_widget_get_window (shell->canvas), NULL);

      return;
    }

  if (cursor_type != CLAY_CURSOR_NONE &&
      cursor_type != CLAY_CURSOR_BAD  &&
      cursor_type != CLAY_CURSOR_SINGLE_DOT)
    {
      switch (shell->display->config->cursor_mode)
        {
        case CLAY_CURSOR_MODE_TOOL_ICON:
          break;

        case CLAY_CURSOR_MODE_TOOL_CROSSHAIR:
          if (cursor_type < CLAY_CURSOR_CORNER_TOP ||
              cursor_type > CLAY_CURSOR_SIDE_TOP_LEFT)
            {
              /* the corner and side cursors count as crosshair, so leave
               * them and override everything else
               */
              cursor_type = CLAY_CURSOR_CROSSHAIR_SMALL;
            }
          break;

        case CLAY_CURSOR_MODE_CROSSHAIR:
          cursor_type = CLAY_CURSOR_CROSSHAIR;
          tool_cursor = CLAY_TOOL_CURSOR_NONE;

          if (modifier != CLAY_CURSOR_MODIFIER_BAD)
            {
              /* the bad modifier is always shown */
              modifier = CLAY_CURSOR_MODIFIER_NONE;
            }
          break;
        }
    }

  cursor_type = clay_cursor_rotate (cursor_type, shell->rotate_angle);

  cursor_handedness = CLAY_GUI_CONFIG (shell->display->config)->cursor_handedness;

  if (shell->cursor_handedness != cursor_handedness ||
      shell->current_cursor    != cursor_type       ||
      shell->tool_cursor       != tool_cursor       ||
      shell->cursor_modifier   != modifier          ||
      always_install)
    {
      shell->cursor_handedness = cursor_handedness;
      shell->current_cursor    = cursor_type;
      shell->tool_cursor       = tool_cursor;
      shell->cursor_modifier   = modifier;

      clay_cursor_set (shell->canvas,
                       cursor_handedness,
                       cursor_type, tool_cursor, modifier);
    }
}
