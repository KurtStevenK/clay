/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clay-gui.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "claydisplay.h"
#include "claydrawable.h"
#include "clayimage.h"
#include "clayprogress.h"
#include "clayresource.h"
#include "claywaitable.h"

#include "about.h"

#include "clay-intl.h"


void
clay_gui_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay->gui.ungrab                    = NULL;
  clay->gui.set_busy                  = NULL;
  clay->gui.unset_busy                = NULL;
  clay->gui.show_message              = NULL;
  clay->gui.help                      = NULL;
  clay->gui.get_program_class         = NULL;
  clay->gui.get_display_name          = NULL;
  clay->gui.get_user_time             = NULL;
  clay->gui.get_theme_dir             = NULL;
  clay->gui.get_icon_theme_dir        = NULL;
  clay->gui.display_get_window_handle = NULL;
  clay->gui.display_create            = NULL;
  clay->gui.display_delete            = NULL;
  clay->gui.displays_reconnect        = NULL;
  clay->gui.progress_new              = NULL;
  clay->gui.progress_free             = NULL;
  clay->gui.pdb_dialog_set            = NULL;
  clay->gui.pdb_dialog_close          = NULL;
  clay->gui.recent_list_add_file      = NULL;
  clay->gui.recent_list_load          = NULL;
  clay->gui.get_mount_operation       = NULL;
  clay->gui.query_profile_policy      = NULL;
  clay->gui.query_rotation_policy     = NULL;
}

void
clay_gui_ungrab (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->gui.ungrab)
    clay->gui.ungrab (clay);
}

void
clay_set_busy (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  /* FIXME: clay_busy HACK */
  clay->busy++;

  if (clay->busy == 1)
    {
      if (clay->gui.set_busy)
        clay->gui.set_busy (clay);
    }
}

static gboolean
clay_idle_unset_busy (gpointer data)
{
  Clay *clay = data;

  clay_unset_busy (clay);

  clay->busy_idle_id = 0;

  return FALSE;
}

void
clay_set_busy_until_idle (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (! clay->busy_idle_id)
    {
      clay_set_busy (clay);

      clay->busy_idle_id = g_idle_add_full (G_PRIORITY_HIGH,
                                            clay_idle_unset_busy, clay,
                                            NULL);
    }
}

void
clay_unset_busy (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (clay->busy > 0);

  /* FIXME: clay_busy HACK */
  clay->busy--;

  if (clay->busy == 0)
    {
      if (clay->gui.unset_busy)
        clay->gui.unset_busy (clay);
    }
}

void
clay_show_message (Clay                *clay,
                   GObject             *handler,
                   ClayMessageSeverity  severity,
                   const gchar         *domain,
                   const gchar         *message)
{
  const gchar *desc = (severity == CLAY_MESSAGE_ERROR) ? "Error" : "Message";

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (message != NULL);

  if (! domain)
    domain = CLAY_ACRONYM;

  if (! clay->console_messages)
    {
      if (clay->gui.show_message)
        {
          clay->gui.show_message (clay, handler, severity,
                                  domain, message);
          return;
        }
      else if (CLAY_IS_PROGRESS (handler) &&
               clay_progress_message (CLAY_PROGRESS (handler), clay,
                                      severity, domain, message))
        {
          /* message has been handled by ClayProgress */
          return;
        }
    }

  clay_enum_get_value (CLAY_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);
  g_printerr ("%s-%s: %s\n\n", domain, desc, message);
}

void
clay_wait (Clay         *clay,
           ClayWaitable *waitable,
           const gchar  *format,
           ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_WAITABLE (waitable));
  g_return_if_fail (format != NULL);

  if (clay_waitable_wait_for (waitable, 0.5 * G_TIME_SPAN_SECOND))
    return;

  va_start (args, format);

  message = g_strdup_vprintf (format, args);

  va_end (args);

  if (! clay->console_messages &&
      clay->gui.wait           &&
      clay->gui.wait (clay, waitable, message))
    {
      return;
    }

  /* Translator:  This message is displayed while Clay is waiting for
   * some operation to finish.  The %s argument is a message describing
   * the operation.
   */
  g_printerr (_("Please wait: %s\n"), message);

  clay_waitable_wait (waitable);

  g_free (message);
}

void
clay_help (Clay         *clay,
           ClayProgress *progress,
           const gchar  *help_domain,
           const gchar  *help_id)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  if (clay->gui.help)
    clay->gui.help (clay, progress, help_domain, help_id);
}

const gchar *
clay_get_program_class (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->gui.get_program_class)
    return clay->gui.get_program_class (clay);

  return NULL;
}

gchar *
clay_get_display_name (Clay     *clay,
                       gint      display_id,
                       GObject **monitor,
                       gint     *monitor_number)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (monitor != NULL, NULL);
  g_return_val_if_fail (monitor_number != NULL, NULL);

  if (clay->gui.get_display_name)
    return clay->gui.get_display_name (clay, display_id,
                                       monitor, monitor_number);

  *monitor = NULL;

  return NULL;
}

/**
 * clay_get_user_time:
 * @clay:
 *
 * Returns the timestamp of the last user interaction. The timestamp is
 * taken from events caused by user interaction such as key presses or
 * pointer movements. See gdk_x11_display_get_user_time().
 *
 * Returns: the timestamp of the last user interaction
 */
guint32
clay_get_user_time (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  if (clay->gui.get_user_time)
    return clay->gui.get_user_time (clay);

  return 0;
}

GFile *
clay_get_theme_dir (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->gui.get_theme_dir)
    return clay->gui.get_theme_dir (clay);

  return NULL;
}

GFile *
clay_get_icon_theme_dir (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->gui.get_icon_theme_dir)
    return clay->gui.get_icon_theme_dir (clay);

  return NULL;
}

ClayObject *
clay_get_window_strategy (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->gui.get_window_strategy)
    return clay->gui.get_window_strategy (clay);

  return NULL;
}

ClayDisplay *
clay_get_empty_display (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->gui.get_empty_display)
    return clay->gui.get_empty_display (clay);

  return NULL;
}

GBytes *
clay_get_display_window_handle (Clay        *clay,
                                ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  if (clay->gui.display_get_window_handle)
    return clay->gui.display_get_window_handle (display);

  return NULL;
}

ClayDisplay *
clay_create_display (Clay      *clay,
                     ClayImage *image,
                     ClayUnit  *unit,
                     gdouble    scale,
                     GObject   *monitor)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), NULL);

  if (clay->gui.display_create)
    return clay->gui.display_create (clay, image, unit, scale, monitor);

  return NULL;
}

void
clay_delete_display (Clay        *clay,
                     ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  if (clay->gui.display_delete)
    clay->gui.display_delete (display);
}

void
clay_reconnect_displays (Clay      *clay,
                         ClayImage *old_image,
                         ClayImage *new_image)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_IMAGE (old_image));
  g_return_if_fail (CLAY_IS_IMAGE (new_image));

  if (clay->gui.displays_reconnect)
    clay->gui.displays_reconnect (clay, old_image, new_image);
}

ClayProgress *
clay_new_progress (Clay        *clay,
                   ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (display == NULL || CLAY_IS_DISPLAY (display), NULL);

  if (clay->gui.progress_new)
    return clay->gui.progress_new (clay, display);

  return NULL;
}

void
clay_free_progress (Clay         *clay,
                    ClayProgress *progress)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_PROGRESS (progress));

  if (clay->gui.progress_free)
    clay->gui.progress_free (clay, progress);
}

gboolean
clay_pdb_dialog_new (Clay          *clay,
                     ClayContext   *context,
                     ClayProgress  *progress,
                     GType          contents_type,
                     GBytes        *parent_handle,
                     const gchar   *title,
                     const gchar   *callback_name,
                     ClayObject    *object,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (g_type_is_a (contents_type, CLAY_TYPE_RESOURCE) ||
                        g_type_is_a (contents_type, CLAY_TYPE_ITEM)     ||
                        g_type_is_a (contents_type, CLAY_TYPE_IMAGE), FALSE);
  g_return_val_if_fail (object == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (object), contents_type), FALSE);
  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (clay->gui.pdb_dialog_new)
    {
      va_list args;

      va_start (args, object);

      retval = clay->gui.pdb_dialog_new (clay, context, progress,
                                         contents_type, parent_handle, title,
                                         callback_name, object, args);

      va_end (args);
    }

  return retval;
}

gboolean
clay_pdb_dialog_set (Clay        *clay,
                     GType        contents_type,
                     const gchar *callback_name,
                     ClayObject  *object,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (g_type_is_a (contents_type, CLAY_TYPE_RESOURCE) ||
                        contents_type == CLAY_TYPE_DRAWABLE             ||
                        contents_type == CLAY_TYPE_ITEM                 ||
                        contents_type == CLAY_TYPE_IMAGE, FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);
  g_return_val_if_fail (object == NULL || g_type_is_a (G_TYPE_FROM_INSTANCE (object), contents_type), FALSE);

  if (clay->gui.pdb_dialog_set)
    {
      va_list args;

      va_start (args, object);

      retval = clay->gui.pdb_dialog_set (clay, contents_type, callback_name,
                                         object, args);

      va_end (args);
    }

  return retval;
}

gboolean
clay_pdb_dialog_close (Clay          *clay,
                       GType          contents_type,
                       const gchar   *callback_name)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (g_type_is_a (contents_type, CLAY_TYPE_RESOURCE) ||
                        contents_type == CLAY_TYPE_DRAWABLE             ||
                        contents_type == CLAY_TYPE_ITEM                 ||
                        contents_type == CLAY_TYPE_IMAGE, FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (clay->gui.pdb_dialog_close)
    return clay->gui.pdb_dialog_close (clay, contents_type, callback_name);

  return FALSE;
}

gboolean
clay_recent_list_add_file (Clay        *clay,
                           GFile       *file,
                           const gchar *mime_type)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  if (clay->gui.recent_list_add_file)
    return clay->gui.recent_list_add_file (clay, file, mime_type);

  return FALSE;
}

void
clay_recent_list_load (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->gui.recent_list_load)
    clay->gui.recent_list_load (clay);
}

GMountOperation *
clay_get_mount_operation (Clay         *clay,
                          ClayProgress *progress)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);

  if (clay->gui.get_mount_operation)
    return clay->gui.get_mount_operation (clay, progress);

  return g_mount_operation_new ();
}

ClayColorProfilePolicy
clay_query_profile_policy (Clay                      *clay,
                           ClayImage                 *image,
                           ClayContext               *context,
                           ClayColorProfile         **dest_profile,
                           ClayColorRenderingIntent  *intent,
                           gboolean                  *bpc,
                           gboolean                  *dont_ask)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), CLAY_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), CLAY_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), CLAY_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (dest_profile != NULL, CLAY_COLOR_PROFILE_POLICY_KEEP);

  if (clay->gui.query_profile_policy)
    return clay->gui.query_profile_policy (clay, image, context,
                                           dest_profile,
                                           intent, bpc,
                                           dont_ask);

  return CLAY_COLOR_PROFILE_POLICY_KEEP;
}

ClayMetadataRotationPolicy
clay_query_rotation_policy (Clay        *clay,
                            ClayImage   *image,
                            ClayContext *context,
                            gboolean    *dont_ask)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), CLAY_METADATA_ROTATION_POLICY_ROTATE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), CLAY_METADATA_ROTATION_POLICY_ROTATE);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), CLAY_METADATA_ROTATION_POLICY_ROTATE);

  if (clay->gui.query_rotation_policy)
    return clay->gui.query_rotation_policy (clay, image, context, dont_ask);

  return CLAY_METADATA_ROTATION_POLICY_ROTATE;
}
