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

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylist.h"

#include "claydisplay.h"
#include "claydisplay-foreach.h"
#include "claydisplayshell.h"
#include "claydisplayshell-cursor.h"
#include "clayimagewindow.h"


gboolean
clay_displays_dirty (Clay *clay)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplay *display = list->data;
      ClayImage   *image   = clay_display_get_image (display);

      if (image && clay_image_is_dirty (image))
        return TRUE;
    }

  return FALSE;
}

static void
clay_displays_image_dirty_callback (ClayImage     *image,
                                    ClayDirtyMask  dirty_mask,
                                    ClayContainer *container)
{
  if (clay_image_is_dirty (image)              &&
      clay_image_get_display_count (image) > 0 &&
      ! clay_container_have (container, CLAY_OBJECT (image)))
    clay_container_add (container, CLAY_OBJECT (image));
}

static void
clay_displays_dirty_images_disconnect (ClayContainer *dirty_container,
                                       ClayContainer *global_container)
{
  GQuark handler;

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "clean-handler"));
  clay_container_remove_handler (global_container, handler);

  handler = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dirty_container),
                                                "dirty-handler"));
  clay_container_remove_handler (global_container, handler);
}

static void
clay_displays_image_clean_callback (ClayImage     *image,
                                    ClayDirtyMask  dirty_mask,
                                    ClayContainer *container)
{
  if (! clay_image_is_dirty (image))
    clay_container_remove (container, CLAY_OBJECT (image));
}

ClayContainer *
clay_displays_get_dirty_images (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay_displays_dirty (clay))
    {
      ClayContainer *container = clay_list_new_weak (CLAY_TYPE_IMAGE, FALSE);
      GList         *list;
      GQuark         handler;

      handler =
        clay_container_add_handler (clay->images, "clean",
                                    G_CALLBACK (clay_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "clean-handler",
                         GINT_TO_POINTER (handler));

      handler =
        clay_container_add_handler (clay->images, "dirty",
                                    G_CALLBACK (clay_displays_image_dirty_callback),
                                    container);
      g_object_set_data (G_OBJECT (container), "dirty-handler",
                         GINT_TO_POINTER (handler));

      g_signal_connect_object (container, "disconnect",
                               G_CALLBACK (clay_displays_dirty_images_disconnect),
                               G_OBJECT (clay->images), 0);

      clay_container_add_handler (container, "clean",
                                  G_CALLBACK (clay_displays_image_clean_callback),
                                  container);
      clay_container_add_handler (container, "dirty",
                                  G_CALLBACK (clay_displays_image_clean_callback),
                                  container);

      for (list = clay_get_image_iter (clay);
           list;
           list = g_list_next (list))
        {
          ClayImage *image = list->data;

          if (clay_image_is_dirty (image) &&
              clay_image_get_display_count (image) > 0)
            clay_container_add (container, CLAY_OBJECT (image));
        }

      return container;
    }

  return NULL;
}

/**
 * clay_displays_delete:
 * @clay:
 *
 * Calls clay_display_delete() an all displays in the display list.
 * This closes all displays, including the first one which is usually
 * kept open.
 */
void
clay_displays_delete (Clay *clay)
{
  /*  this removes the ClayDisplay from the list, so do a while loop
   *  "around" the first element to get them all
   */
  while (! clay_container_is_empty (clay->displays))
    {
      ClayDisplay *display = clay_get_display_iter (clay)->data;

      clay_display_delete (display);
    }
}

/**
 * clay_displays_close:
 * @clay:
 *
 * Calls clay_display_close() an all displays in the display list. The
 * first display will remain open without an image.
 */
void
clay_displays_close (Clay *clay)
{
  GList *list;
  GList *iter;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  list = g_list_copy (clay_get_display_iter (clay));

  for (iter = list; iter; iter = g_list_next (iter))
    {
      ClayDisplay *display = iter->data;

      clay_display_close (display);
    }

  g_list_free (list);
}

void
clay_displays_reconnect (Clay      *clay,
                         ClayImage *old,
                         ClayImage *new)
{
  GList *contexts = NULL;
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_IMAGE (old));
  g_return_if_fail (CLAY_IS_IMAGE (new));

  /*  check which contexts refer to old_image  */
  for (list = clay->context_list; list; list = g_list_next (list))
    {
      ClayContext *context = list->data;

      if (clay_context_get_image (context) == old)
        contexts = g_list_prepend (contexts, list->data);
    }

  /*  set the new_image on the remembered contexts (in reverse order,
   *  since older contexts are usually the parents of newer
   *  ones). Also, update the contexts before the displays, or we
   *  might run into menu update functions that would see an
   *  inconsistent state (display = new, context = old), and thus
   *  inadvertently call actions as if the user had selected a menu
   *  item.
   */
  g_list_foreach (contexts, (GFunc) clay_context_set_image, new);
  g_list_free (contexts);

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplay *display = list->data;

      if (clay_display_get_image (display) == old)
        clay_display_set_image (display, new);
    }
}

gint
clay_displays_get_num_visible (Clay *clay)
{
  GList *list;
  gint   visible = 0;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplay      *display = list->data;
      ClayDisplayShell *shell   = clay_display_get_shell (display);

      if (gtk_widget_is_drawable (GTK_WIDGET (shell)))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

          if (GTK_IS_WINDOW (toplevel))
            {
              GdkWindow      *window = gtk_widget_get_window (toplevel);
              GdkWindowState  state  = gdk_window_get_state (window);

              if ((state & (GDK_WINDOW_STATE_WITHDRAWN |
                            GDK_WINDOW_STATE_ICONIFIED)) == 0)
                {
                  visible++;
                }
            }
        }
    }

  return visible;
}

void
clay_displays_set_busy (Clay *clay)
{
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplayShell *shell =
        clay_display_get_shell (CLAY_DISPLAY (list->data));

      clay_display_shell_set_override_cursor (shell, (ClayCursorType) GDK_WATCH);
    }
}

void
clay_displays_unset_busy (Clay *clay)
{
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplayShell *shell =
        clay_display_get_shell (CLAY_DISPLAY (list->data));

      clay_display_shell_unset_override_cursor (shell);
    }
}

/**
 * clay_displays_accept_focus_events:
 * @clay:
 *
 * When this returns %FALSE, we should ignore focus events. In
 * particular, I had weird cases in multi-window mode, before a new
 * image window was actually displayed on screen, there seemed to be
 * infinite loops of focus events switching the active window between
 * the current active one and the one being created, with flickering
 * window title and the new display never actually displayed until we
 * broke the loop by manually getting out of focus (see #11957).
 * This is why when any of the display is not fully drawn yet (which
 * may mean it is either being created or moved to a new window), we
 * temporarily ignore focus events on all displays.
 */
gboolean
clay_displays_accept_focus_events (Clay *clay)
{
  GList    *windows;
  gboolean  accept = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  windows = clay_get_image_windows (clay);
  for (GList *iter = windows; iter; iter = iter->next)
    {
      ClayDisplayShell *shell = clay_image_window_get_active_shell (iter->data);

      if (shell && ! clay_display_shell_is_drawn (shell))
        {
          accept = FALSE;
          break;
        }
    }
  g_list_free (windows);

  return accept;
}
