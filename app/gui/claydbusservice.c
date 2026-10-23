/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2013       Michael Natterer <mitch@gimp.org>
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

#include "gui-types.h"

#include "core/clay.h"
#include "core/clay-batch.h"
#include "core/claycontainer.h"

#include "file/file-open.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "claydbusservice.h"


typedef struct
{
  GFile    *file;
  gboolean  as_new;

  gchar    *interpreter;
  gchar    *command;
} IdleData;

static void       clay_dbus_service_ui_iface_init  (ClayDBusServiceUIIface *iface);

static void       clay_dbus_service_dispose        (GObject               *object);
static void       clay_dbus_service_finalize       (GObject               *object);

static gboolean   clay_dbus_service_activate       (ClayDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation);
static gboolean   clay_dbus_service_open           (ClayDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   clay_dbus_service_open_as_new    (ClayDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   clay_dbus_service_batch_run      (ClayDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *batch_interpreter,
                                                    const gchar           *batch_command);

static void       clay_dbus_service_clay_opened    (Clay                  *clay,
                                                    GFile                 *file,
                                                    ClayDBusService       *service);

static gboolean   clay_dbus_service_queue_open     (ClayDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static gboolean   clay_dbus_service_queue_batch    (ClayDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);

static gboolean   clay_dbus_service_process_idle   (ClayDBusService       *service);
static IdleData * clay_dbus_service_open_data_new  (ClayDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static IdleData * clay_dbus_service_batch_data_new (ClayDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);
static void       clay_dbus_service_idle_data_free (IdleData              *data);


G_DEFINE_TYPE_WITH_CODE (ClayDBusService, clay_dbus_service,
                         CLAY_DBUS_SERVICE_TYPE_UI_SKELETON,
                         G_IMPLEMENT_INTERFACE (CLAY_DBUS_SERVICE_TYPE_UI,
                                                clay_dbus_service_ui_iface_init))

#define parent_class clay_dbus_service_parent_class


static void
clay_dbus_service_class_init (ClayDBusServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = clay_dbus_service_dispose;
  object_class->finalize = clay_dbus_service_finalize;
}

static void
clay_dbus_service_init (ClayDBusService *service)
{
  service->queue = g_queue_new ();
}

static void
clay_dbus_service_ui_iface_init (ClayDBusServiceUIIface *iface)
{
  iface->handle_activate    = clay_dbus_service_activate;
  iface->handle_open        = clay_dbus_service_open;
  iface->handle_open_as_new = clay_dbus_service_open_as_new;
  iface->handle_batch_run   = clay_dbus_service_batch_run;
}

GObject *
clay_dbus_service_new (Clay *clay)
{
  ClayDBusService *service;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  service = g_object_new (CLAY_TYPE_DBUS_SERVICE, NULL);

  service->clay = clay;

  g_signal_connect_object (clay, "image-opened",
                           G_CALLBACK (clay_dbus_service_clay_opened),
                           service, 0);

  return G_OBJECT (service);
}

static void
clay_dbus_service_dispose (GObject *object)
{
  ClayDBusService *service = CLAY_DBUS_SERVICE (object);

  if (service->source)
    {
      g_source_remove (g_source_get_id (service->source));
      service->source = NULL;
      service->timeout_source = FALSE;
    }

  while (! g_queue_is_empty (service->queue))
    {
      clay_dbus_service_idle_data_free (g_queue_pop_head (service->queue));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dbus_service_finalize (GObject *object)
{
  ClayDBusService *service = CLAY_DBUS_SERVICE (object);

  if (service->queue)
    {
      g_queue_free (service->queue);
      service->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gboolean
clay_dbus_service_activate (ClayDBusServiceUI     *service,
                            GDBusMethodInvocation *invocation)
{
  Clay *clay = CLAY_DBUS_SERVICE (service)->clay;

  /*  We want to be called again later in case that Clay is not fully
   *  started yet.
   */
  if (clay_is_restored (clay))
    {
      ClayObject *display;

      display = clay_container_get_first_child (clay->displays);

      if (display)
        clay_display_shell_present (clay_display_get_shell (CLAY_DISPLAY (display)));
    }

  clay_dbus_service_ui_complete_activate (service, invocation);

  return TRUE;
}

gboolean
clay_dbus_service_open (ClayDBusServiceUI     *service,
                        GDBusMethodInvocation *invocation,
                        const gchar           *uri)
{
  gboolean success;

  success = clay_dbus_service_queue_open (CLAY_DBUS_SERVICE (service),
                                          uri, FALSE);

  clay_dbus_service_ui_complete_open (service, invocation, success);

  return TRUE;
}

gboolean
clay_dbus_service_open_as_new (ClayDBusServiceUI     *service,
                               GDBusMethodInvocation *invocation,
                               const gchar           *uri)
{
  gboolean success;

  success = clay_dbus_service_queue_open (CLAY_DBUS_SERVICE (service),
                                          uri, TRUE);

  clay_dbus_service_ui_complete_open_as_new (service, invocation, success);

  return TRUE;
}

gboolean
clay_dbus_service_batch_run (ClayDBusServiceUI     *service,
                             GDBusMethodInvocation *invocation,
                             const gchar           *batch_interpreter,
                             const gchar           *batch_command)
{
  gboolean success;

  success = clay_dbus_service_queue_batch (CLAY_DBUS_SERVICE (service),
                                           batch_interpreter,
                                           batch_command);

  clay_dbus_service_ui_complete_batch_run (service, invocation, success);

  return TRUE;
}

static void
clay_dbus_service_clay_opened (Clay            *clay,
                               GFile           *file,
                               ClayDBusService *service)
{
  gchar *uri = g_file_get_uri (file);

  g_signal_emit_by_name (service, "opened", uri);

  g_free (uri);
}

/*
 * Adds a request to open a file to the end of the queue and
 * starts an idle source if it is not already running.
 */
static gboolean
clay_dbus_service_queue_open (ClayDBusService *service,
                              const gchar     *uri,
                              gboolean         as_new)
{
  g_queue_push_tail (service->queue,
                     clay_dbus_service_open_data_new (service, uri, as_new));

  if (! service->source)
    {
      service->source = g_idle_source_new ();
      service->timeout_source = FALSE;

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) clay_dbus_service_process_idle,
                             service,
                             NULL);
      g_source_attach (service->source, NULL);
      g_source_unref (service->source);
    }

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  return TRUE;
}

/*
 * Adds a request to run a batch command to the end of the queue and
 * starts an idle source if it is not already running.
 */
static gboolean
clay_dbus_service_queue_batch (ClayDBusService *service,
                               const gchar     *interpreter,
                               const gchar     *command)
{
  g_queue_push_tail (service->queue,
                     clay_dbus_service_batch_data_new (service,
                                                       interpreter,
                                                       command));

  if (! service->source)
    {
      service->source = g_idle_source_new ();
      service->timeout_source = FALSE;

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) clay_dbus_service_process_idle,
                             service,
                             NULL);
      g_source_attach (service->source, NULL);
      g_source_unref (service->source);
    }

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  return TRUE;
}

/*
 * Idle callback that removes the first request from the queue and
 * handles it. If there are no more requests, the idle source is
 * removed.
 */
static gboolean
clay_dbus_service_process_idle (ClayDBusService *service)
{
  IdleData *data;

  if (! clay_is_restored (service->clay))
    {
      if (! service->timeout_source)
        {
          /* We are probably starting the program. No need to spam Clay with
           * an idle handler (which might make Clay slower to start even
           * with low priority).
           * Instead let's add a timeout of half a second.
           */
          service->source = g_timeout_source_new (500);
          service->timeout_source = TRUE;

          g_source_set_priority (service->source, G_PRIORITY_LOW);
          g_source_set_callback (service->source,
                                 (GSourceFunc) clay_dbus_service_process_idle,
                                 service,
                                 NULL);
          g_source_attach (service->source, NULL);
          g_source_unref (service->source);

          return G_SOURCE_REMOVE;
        }
      else
        {
          return G_SOURCE_CONTINUE;
        }
    }

  /* Process data as a FIFO. */
  data = g_queue_pop_head (service->queue);

  if (data)
    {
      if (data->file)
        file_open_from_command_line (service->clay, data->file, data->as_new,
                                     NULL /* FIXME monitor */);

      if (data->command)
        {
          const gchar *commands[2] = {data->command, 0};

          clay_batch_run (service->clay, data->interpreter,
                          commands);
        }

      clay_dbus_service_idle_data_free (data);

      if (service->timeout_source)
        {
          /* Now Clay is fully functional and can respond quickly to
           * DBus calls. Switch to a usual idle source.
           */
          service->source = g_idle_source_new ();
          service->timeout_source = FALSE;

          g_source_set_priority (service->source, G_PRIORITY_LOW);
          g_source_set_callback (service->source,
                                 (GSourceFunc) clay_dbus_service_process_idle,
                                 service,
                                 NULL);
          g_source_attach (service->source, NULL);
          g_source_unref (service->source);

          return G_SOURCE_REMOVE;
        }
      else
        {
          return G_SOURCE_CONTINUE;
        }
    }

  service->source = NULL;

  return G_SOURCE_REMOVE;
}

static IdleData *
clay_dbus_service_open_data_new (ClayDBusService *service,
                                 const gchar     *uri,
                                 gboolean         as_new)
{
  IdleData *data    = g_slice_new (IdleData);

  data->file        = g_file_new_for_uri (uri);
  data->as_new      = as_new;
  data->interpreter = NULL;
  data->command     = NULL;

  return data;
}

static IdleData *
clay_dbus_service_batch_data_new (ClayDBusService *service,
                                  const gchar     *interpreter,
                                  const gchar     *command)
{
  IdleData *data = g_slice_new (IdleData);

  data->file     = NULL;
  data->as_new   = FALSE;

  data->command  = g_strdup (command);

  if (g_strcmp0 (interpreter, "") == 0)
    data->interpreter = NULL;
  else
    data->interpreter = g_strdup (interpreter);

  return data;
}

static void
clay_dbus_service_idle_data_free (IdleData *data)
{
  if (data->file)
    g_object_unref (data->file);

  if (data->command)
    g_free (data->command);
  if (data->interpreter)
    g_free (data->interpreter);

  g_slice_free (IdleData, data);
}
