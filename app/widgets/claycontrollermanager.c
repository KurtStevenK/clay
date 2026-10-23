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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#define CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libclaywidgets/claycontroller.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claylist.h"

#include "clayaction.h"
#include "clayactiongroup.h"
#include "claycontrollerinfo.h"
#include "claycontrollerkeyboard.h"
#include "claycontrollermanager.h"
#include "claycontrollerwheel.h"
#include "clayenumaction.h"
#include "clayuimanager.h"


struct _ClayControllerManager
{
  ClayList        parent_instance;

  Clay           *clay;

  GQuark          event_mapped_id;
  ClayController *wheel;
  ClayController *keyboard;
  ClayUIManager  *ui_manager;
};


/*  local function prototypes  */

static void       clay_controller_manager_finalize (GObject       *object);

static void       clay_controller_manager_add      (ClayContainer *container,
                                                    ClayObject    *object);
static void       clay_controller_manager_remove   (ClayContainer *container,
                                                    ClayObject    *object);

static gboolean   clay_controller_manager_event_mapped  (ClayControllerInfo        *info,
                                                         ClayController            *controller,
                                                         const ClayControllerEvent *event,
                                                         const gchar               *action_name,
                                                         ClayControllerManager     *manager);

static gboolean   clay_controller_search_invalid        (ClayObject                *object,
                                                         gpointer                   user_data);


G_DEFINE_TYPE (ClayControllerManager, clay_controller_manager,
               CLAY_TYPE_LIST)

#define parent_class clay_controller_manager_parent_class


static void
clay_controller_manager_class_init (ClayControllerManagerClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerClass *container_class = CLAY_CONTAINER_CLASS (klass);

  object_class->finalize  = clay_controller_manager_finalize;

  container_class->add    = clay_controller_manager_add;
  container_class->remove = clay_controller_manager_remove;
}

static void
clay_controller_manager_init (ClayControllerManager *self)
{
  g_type_class_ref (CLAY_TYPE_CONTROLLER_WHEEL);
  g_type_class_ref (CLAY_TYPE_CONTROLLER_KEYBOARD);
}

static void
clay_controller_manager_finalize (GObject *object)
{
  g_type_class_unref (g_type_class_peek (CLAY_TYPE_CONTROLLER_WHEEL));
  g_type_class_unref (g_type_class_peek (CLAY_TYPE_CONTROLLER_KEYBOARD));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_controller_manager_add (ClayContainer *container,
                             ClayObject    *object)
{
  ClayControllerManager *self = CLAY_CONTROLLER_MANAGER (container);
  ClayControllerInfo    *info = CLAY_CONTROLLER_INFO (object);

  g_signal_connect_object (info, "event-mapped",
                           G_CALLBACK (clay_controller_manager_event_mapped),
                           G_OBJECT (self), 0);

  if (CLAY_IS_CONTROLLER_WHEEL (info->controller))
    self->wheel = info->controller;
  else if (CLAY_IS_CONTROLLER_KEYBOARD (info->controller))
    self->keyboard = info->controller;

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);
}

static void
clay_controller_manager_remove (ClayContainer *container,
                                ClayObject    *object)
{
  ClayControllerManager *self = CLAY_CONTROLLER_MANAGER (container);
  ClayControllerInfo    *info = CLAY_CONTROLLER_INFO (object);

  if (info->controller == self->wheel)
    self->wheel = NULL;

  else if (info->controller == self->keyboard)
    self->keyboard = NULL;

  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);
}


/*  public functions  */

ClayControllerManager *
clay_controller_manager_new (Clay *clay)
{
  ClayControllerManager *self;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  self = g_object_new (CLAY_TYPE_CONTROLLER_MANAGER,
                       "child-type", CLAY_TYPE_CONTROLLER_INFO,
                       NULL);

  self->clay = clay;

  return self;
}

void
clay_controller_manager_restore (ClayControllerManager *self,
                                 ClayUIManager         *ui_manager)
{
  GFile      *file;
  ClayObject *found;
  GError     *error = NULL;

  g_return_if_fail (CLAY_IS_CONTROLLER_MANAGER (self));
  g_return_if_fail (CLAY_IS_UI_MANAGER (ui_manager));
  g_return_if_fail (self->ui_manager == NULL);

  self->ui_manager = ui_manager;

  file = clay_directory_file ("controllerrc", NULL);

  if (self->clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_deserialize_file (CLAY_CONFIG (self),
                                      file, NULL, &error))
    {
      if (error->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          if (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"))
            {
              gchar *path;
              path = g_build_filename (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"),
                                       "etc", "controllerrc", NULL);
              file = g_file_new_for_path (path);
              g_free (path);
            }
          else
            {
              file = clay_sysconf_directory_file ("controllerrc", NULL);
            }

          if (! clay_config_deserialize_file (CLAY_CONFIG (self),
                                              file, NULL, &error))
            {
              clay_message_literal (self->clay, NULL, CLAY_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          clay_message_literal (self->clay, NULL, CLAY_MESSAGE_ERROR,
                                error->message);
        }

      g_clear_error (&error);
    }

  clay_list_reverse (CLAY_LIST (self));

  while ((found = clay_container_search (CLAY_CONTAINER (self),
                                        (ClayContainerSearchFunc) clay_controller_search_invalid,
                                        NULL)))
    /* Clean out the manager off invalid controllers. This should not
     * happen, even with removed controllers (e.g. ClayControllerMouse),
     * which our migration code should take care of (and if it doesn't,
     * the whole controllerrc ends up invalid).
     * That's still good sanitization, as a purposely created invalid
     * controllerrc with no (controller "<name>") line could add a NULL
     * controller which could crash the software.
     */
    clay_container_remove (CLAY_CONTAINER (self), found);

  g_object_unref (file);
}

void
clay_controller_manager_save (ClayControllerManager *self)
{
  const gchar *header =
    "Clay controllerrc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of controllerrc";

  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (CLAY_IS_CONTROLLER_MANAGER (self));

  file = clay_directory_file ("controllerrc", NULL);

  if (self->clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (self),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      clay_message_literal (self->clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

Clay *
clay_controller_manager_get_clay (ClayControllerManager *self)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_MANAGER (self), NULL);

  return self->clay;
}

ClayUIManager *
clay_controller_manager_get_ui_manager (ClayControllerManager *self)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_MANAGER (self), NULL);

  return self->ui_manager;
}

ClayController *
clay_controller_manager_get_wheel (ClayControllerManager *self)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_MANAGER (self), NULL);

  return self->wheel;
}

ClayController *
clay_controller_manager_get_keyboard (ClayControllerManager *self)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_MANAGER (self), NULL);

  return self->keyboard;
}


/*  private functions  */

static gboolean
clay_controller_manager_event_mapped (ClayControllerInfo        *info,
                                      ClayController            *controller,
                                      const ClayControllerEvent *event,
                                      const gchar               *action_name,
                                      ClayControllerManager     *manager)
{
  GList *list;

  for (list = clay_ui_manager_get_action_groups (manager->ui_manager);
       list;
       list = g_list_next (list))
    {
      ClayActionGroup *group = list->data;
      ClayAction      *action;

      action = clay_action_group_get_action (group, action_name);

      if (action)
        {
          switch (event->type)
            {
            case CLAY_CONTROLLER_EVENT_VALUE:
              if (G_VALUE_HOLDS_DOUBLE (&event->value.value) &&
                  CLAY_IS_ENUM_ACTION (action)               &&
                  CLAY_ENUM_ACTION (action)->value_variable)
                {
                  gdouble value = g_value_get_double (&event->value.value);

                  clay_action_emit_activate (CLAY_ACTION (action),
                                             g_variant_new_int32 (value * 1000));

                  break;
                }
              /* else fallthru */

            case CLAY_CONTROLLER_EVENT_TRIGGER:
            default:
              clay_action_activate (action);
              break;
            }

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
clay_controller_search_invalid (ClayObject *object,
                                gpointer    user_data)
{
  return (CLAY_CONTROLLER_INFO (object)->controller == NULL);
}
