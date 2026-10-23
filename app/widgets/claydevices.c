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

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/clayerror.h"
#include "core/claygradient.h"
#include "core/claylist.h"
#include "core/claypattern.h"
#include "core/claytoolinfo.h"

#include "claydeviceinfo.h"
#include "claydevicemanager.h"
#include "claydevices.h"

#include "clay-intl.h"


#define CLAY_DEVICE_MANAGER_DATA_KEY "clay-device-manager"


static gboolean devicerc_deleted = FALSE;


static void clay_devices_restore_on_focused_once (Clay *clay);


/*  public functions  */

void
clay_devices_init (Clay *clay)
{
  ClayDeviceManager *manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  manager = g_object_get_data (G_OBJECT (clay), CLAY_DEVICE_MANAGER_DATA_KEY);

  g_return_if_fail (manager == NULL);

  manager = clay_device_manager_new (clay);

  g_object_set_data_full (G_OBJECT (clay),
                          CLAY_DEVICE_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) g_object_unref);
}

void
clay_devices_exit (Clay *clay)
{
  ClayDeviceManager *manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  manager = clay_devices_get_manager (clay);

  g_return_if_fail (CLAY_IS_DEVICE_MANAGER (manager));

  g_object_set_data (G_OBJECT (clay), CLAY_DEVICE_MANAGER_DATA_KEY, NULL);
}

void
clay_devices_restore (Clay *clay)
{
  if (clay_has_focused_once (clay))
    clay_devices_restore_on_focused_once (clay);
  else
    g_signal_connect (clay, "focused-once",
                      G_CALLBACK (clay_devices_restore_on_focused_once),
                      NULL);
}

void
clay_devices_save (Clay     *clay,
                   gboolean  always_save)
{
  ClayDeviceManager *manager;
  GFile             *file;
  GError            *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  manager = clay_devices_get_manager (clay);

  g_return_if_fail (CLAY_IS_DEVICE_MANAGER (manager));

  if (devicerc_deleted && ! always_save)
    return;

  file = clay_directory_file ("devicerc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! CLAY_GUI_CONFIG (clay->config)->devices_share_tool)
    {
      ClayDeviceInfo *current_device;

      current_device = clay_device_manager_get_current_device (manager);

      clay_device_info_save_tool (current_device);
    }

  if (! clay_config_serialize_to_file (CLAY_CONFIG (manager),
                                       file,
                                       "Clay devicerc",
                                       "end of devicerc",
                                       NULL,
                                       &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);

  devicerc_deleted = FALSE;
  clay_device_manager_reconfigure_pads (manager);
}

gboolean
clay_devices_clear (Clay    *clay,
                    GError **error)
{
  ClayDeviceManager *manager;
  GFile             *file;
  GError            *my_error = NULL;
  gboolean           success  = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  manager = clay_devices_get_manager (clay);

  g_return_val_if_fail (CLAY_IS_DEVICE_MANAGER (manager), FALSE);

  file = clay_directory_file ("devicerc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
    }
  else
    {
      devicerc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

ClayDeviceManager *
clay_devices_get_manager (Clay *clay)
{
  ClayDeviceManager *manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  manager = g_object_get_data (G_OBJECT (clay), CLAY_DEVICE_MANAGER_DATA_KEY);

  g_return_val_if_fail (CLAY_IS_DEVICE_MANAGER (manager), NULL);

  return manager;
}

GdkDevice *
clay_devices_get_from_event (Clay            *clay,
                             const GdkEvent  *event,
                             GdkDevice      **grab_device)
{
  GdkDevice *device;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (event != NULL, NULL);

  device = gdk_event_get_source_device (event);

  /*  initialize the default grab device to the event's device,
   *  because that is always either a master or a floating device,
   *  which is the types of devices we can make grabs on without
   *  disturbing side effects.
   */
  if (grab_device)
    *grab_device = gdk_event_get_device (event);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      switch (gdk_device_get_device_type (device))
        {
        case GDK_DEVICE_TYPE_MASTER:
          /*  this happens on focus synthesized focus changed events,
           *  and we can't do anything with the returned device, so
           *  return NULL
           */
          return NULL;

        case GDK_DEVICE_TYPE_SLAVE:
          /*  it makes no sense for us to distinguigh between
           *  different slave keyboards, so just always return
           *  their respective master
           */
          return gdk_device_get_associated_device (device);

        case GDK_DEVICE_TYPE_FLOATING:
          /*  we have no way of explicitly enabling floating
           *  keyboards, so we cannot get their events
           */
          g_return_val_if_reached (device);
       }
    }
  else
    {
      switch (gdk_device_get_device_type (device))
        {
        case GDK_DEVICE_TYPE_MASTER:
          /*  this can only happen for synthesized events which have
           *  no actual source, so return NULL to indicate that we
           *  cannot do anything with the event's device information
           */
          return NULL;

        case GDK_DEVICE_TYPE_SLAVE:
          /*  this is the tricky part: we do want to distingiugh slave
           *  devices, but only if we actually enabled them ourselves
           *  explicitly (like the pens of a tablet); however we
           *  usually don't enable the different incarnations of the
           *  mouse itself (like touchpad, trackpoint, usb mouse
           *  etc.), so for these return their respective master so
           *  its settings are used
           */
          if (gdk_device_get_mode (device) == GDK_MODE_DISABLED)
            {
              return gdk_device_get_associated_device (device);
            }

          return device;

        case GDK_DEVICE_TYPE_FLOATING:
          /*  we only get events for floating devices which have
           *  enabled ourselves, so return the floating device
           */
          return device;
        }
    }

  g_return_val_if_reached (device);
}

void
clay_devices_add_widget (Clay      *clay,
                         GtkWidget *widget)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_connect (widget, "motion-notify-event",
                    G_CALLBACK (clay_devices_check_callback),
                    clay);
}

gboolean
clay_devices_check_callback (GtkWidget *widget,
                             GdkEvent  *event,
                             Clay      *clay)
{
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  if (! clay->busy)
    {
      GdkDevice *device;

      device = clay_devices_get_from_event (clay, event, NULL);

      if (device)
        clay_devices_check_change (clay, device);
    }

  return FALSE;
}

gboolean
clay_devices_check_change (Clay      *clay,
                           GdkDevice *device)
{
  ClayDeviceManager *manager;
  ClayDeviceInfo    *device_info;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (GDK_IS_DEVICE (device), FALSE);

  manager = clay_devices_get_manager (clay);

  g_return_val_if_fail (CLAY_IS_DEVICE_MANAGER (manager), FALSE);

  device_info = clay_device_info_get_by_device (device);

  if (! device_info)
    device_info = clay_device_manager_get_current_device (manager);

  if (device_info &&
      device_info != clay_device_manager_get_current_device (manager))
    {
      clay_device_manager_set_current_device (manager, device_info);
      return TRUE;
    }

  return FALSE;
}


/* Private functions */

static void
clay_devices_restore_on_focused_once (Clay *clay)
{
  ClayDeviceManager *manager;
  GList             *list;
  GFile             *file;
  GError            *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_signal_handlers_disconnect_by_func (clay,
                                        G_CALLBACK (clay_devices_restore_on_focused_once),
                                        NULL);

  manager = clay_devices_get_manager (clay);

  g_return_if_fail (CLAY_IS_DEVICE_MANAGER (manager));

  for (list = CLAY_LIST (manager)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayDeviceInfo *device_info = list->data;

      clay_device_info_save_tool (device_info);
      clay_device_info_set_default_tool (device_info);
    }

  file = clay_directory_file ("devicerc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_deserialize_file (CLAY_CONFIG (manager),
                                      file,
                                      clay,
                                      &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);

      g_error_free (error);
      /* don't bail out here */
    }

  g_object_unref (file);

  for (list = CLAY_LIST (manager)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayDeviceInfo *device_info = list->data;

      if (! CLAY_TOOL_PRESET (device_info)->tool_options)
        {
          clay_device_info_save_tool (device_info);

          g_printerr ("%s: set default tool on loaded ClayDeviceInfo without tool options: %s\n",
                      G_STRFUNC, clay_object_get_name (device_info));
        }
    }

  if (! CLAY_GUI_CONFIG (clay->config)->devices_share_tool)
    {
      ClayDeviceInfo *current_device;

      current_device = clay_device_manager_get_current_device (manager);

      clay_device_info_restore_tool (current_device);
    }
}
