/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claycontrollerlist.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#define CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libclaywidgets/claycontroller.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claycontrollereditor.h"
#include "claycontrollerlist.h"
#include "claycontrollerinfo.h"
#include "claycontrollerkeyboard.h"
#include "claycontrollermanager.h"
#include "claycontrollertype.h"
#include "claycontrollerwheel.h"
#include "claydialogfactory.h"
#include "clayhelp-ids.h"
#include "claymessagebox.h"
#include "claymessagedialog.h"
#include "claypropwidgets.h"
#include "clayrow.h"
#include "clayrow-utils.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CONTROLLER_MANAGER,
  N_PROPS
};

enum
{
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_TYPE,
  N_COLUMNS
};


static void clay_controller_list_constructed     (GObject            *object);
static void clay_controller_list_finalize        (GObject            *object);
static void clay_controller_list_set_property    (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void clay_controller_list_get_property    (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static ClayControllerType *
            clay_controller_list_get_selected_type
                                                 (ClayControllerList  *list);
static void clay_controller_list_type_row_selected
                                                 (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);
static void clay_controller_list_type_row_activated
                                                 (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);

static ClayControllerInfo *
            clay_controller_list_get_selected_controller
                                                 (ClayControllerList  *list);
static void clay_controller_list_row_selected    (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);
static void clay_controller_list_row_activated   (GtkListBox          *self,
                                                  GtkListBoxRow       *row,
                                                  gpointer             user_data);

static void clay_controller_list_add_clicked     (GtkWidget          *button,
                                                  ClayControllerList *list);
static void clay_controller_list_remove_clicked  (GtkWidget          *button,
                                                  ClayControllerList *list);

static void clay_controller_list_edit_clicked    (GtkWidget          *button,
                                                  ClayControllerList *list);
static void clay_controller_list_edit_destroy    (GtkWidget          *widget,
                                                  ClayControllerInfo *info);
static void clay_controller_list_up_clicked      (GtkWidget          *button,
                                                  ClayControllerList *list);
static void clay_controller_list_down_clicked    (GtkWidget          *button,
                                                  ClayControllerList *list);


G_DEFINE_TYPE (ClayControllerList, clay_controller_list, GTK_TYPE_BOX)

#define parent_class clay_controller_list_parent_class


static void
clay_controller_list_class_init (ClayControllerListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_controller_list_constructed;
  object_class->finalize     = clay_controller_list_finalize;
  object_class->set_property = clay_controller_list_set_property;
  object_class->get_property = clay_controller_list_get_property;

  g_object_class_install_property (object_class, PROP_CONTROLLER_MANAGER,
                                   g_param_spec_object ("controller-manager",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTROLLER_MANAGER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_controller_list_init (ClayControllerList *list)
{
  GtkWidget *hbox;
  GtkWidget *sw;
  GtkWidget *vbox;
  GtkWidget *image;
  GtkWidget *label;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (list),
                                  GTK_ORIENTATION_VERTICAL);

  list->controller_manager = NULL;

  list->hbox = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (list), hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (list->hbox), vbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Available Controllers"));
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_widget_set_visible (label, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_set_visible (sw, TRUE);

  list->available_controllers = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list->available_controllers),
                                             FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (list->available_controllers)),
                               "view");
  gtk_widget_set_visible (list->available_controllers, TRUE);
  gtk_container_add (GTK_CONTAINER (sw), list->available_controllers);

  g_signal_connect_object (list->available_controllers, "row-activated",
                           G_CALLBACK (clay_controller_list_type_row_activated),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->available_controllers, "row-selected",
                           G_CALLBACK (clay_controller_list_type_row_selected),
                           G_OBJECT (list), 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  g_set_weak_pointer (&list->add_button, gtk_button_new ());
  gtk_box_pack_start (GTK_BOX (vbox), list->add_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (list->add_button, FALSE);
  gtk_widget_set_visible (list->add_button, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->add_button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (list->add_button, "clicked",
                    G_CALLBACK (clay_controller_list_add_clicked),
                    list);

  g_set_weak_pointer (&list->remove_button, gtk_button_new ());
  gtk_box_pack_start (GTK_BOX (vbox), list->remove_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (list->remove_button, FALSE);
  gtk_widget_set_visible (list->remove_button, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (list->remove_button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (list->remove_button, "clicked",
                    G_CALLBACK (clay_controller_list_remove_clicked),
                    list);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (list->hbox), vbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Active Controllers"));
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_widget_set_visible (label, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  list->active_controllers = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list->active_controllers),
                                             FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (list->active_controllers)),
                               "view");
  gtk_box_pack_start (GTK_BOX (vbox), list->active_controllers, TRUE, TRUE, 0);
  gtk_widget_set_visible (list->active_controllers, TRUE);

  g_signal_connect_object (list->active_controllers, "row-selected",
                           G_CALLBACK (clay_controller_list_row_selected),
                           G_OBJECT (list), 0);
  g_signal_connect_object (list->active_controllers, "row-activated",
                           G_CALLBACK (clay_controller_list_row_activated),
                           G_OBJECT (list), 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_visible (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  list->edit_button = gtk_button_new_from_icon_name (CLAY_ICON_DOCUMENT_PROPERTIES,
                                                     GTK_ICON_SIZE_BUTTON);
  clay_help_set_help_data (list->edit_button,
                           _("Configure the selected controller"), NULL);
  gtk_widget_set_visible (list->edit_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), list->edit_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->edit_button, "clicked",
                           G_CALLBACK (clay_controller_list_edit_clicked),
                           list, 0);

  list->up_button = gtk_button_new_from_icon_name (CLAY_ICON_GO_UP,
                                                   GTK_ICON_SIZE_BUTTON);
  clay_help_set_help_data (list->up_button,
                           _("Move the selected controller up"), NULL);
  gtk_widget_set_visible (list->up_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), list->up_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->up_button, "clicked",
                           G_CALLBACK (clay_controller_list_up_clicked),
                           list, 0);

  list->down_button = gtk_button_new_from_icon_name (CLAY_ICON_GO_DOWN,
                                                     GTK_ICON_SIZE_BUTTON);
  clay_help_set_help_data (list->down_button,
                           _("Move the selected controller down"), NULL);
  gtk_widget_set_visible (list->down_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), list->down_button, TRUE, TRUE, 0);
  g_signal_connect_object (list->down_button, "clicked",
                           G_CALLBACK (clay_controller_list_down_clicked),
                           list, 0);

  gtk_widget_set_sensitive (list->edit_button, FALSE);
  gtk_widget_set_sensitive (list->up_button,   FALSE);
  gtk_widget_set_sensitive (list->down_button, FALSE);
}

static void
clay_controller_list_constructed (GObject *object)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (object);
  GListModel         *types;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  types = clay_controller_type_get_model ();
  gtk_list_box_bind_model (GTK_LIST_BOX (list->available_controllers),
                           types,
                           clay_row_create_for_context,
                           clay_get_user_context (clay_controller_manager_get_clay (list->controller_manager)),
                           NULL);
  g_object_unref (types);

  gtk_list_box_bind_model (GTK_LIST_BOX (list->active_controllers),
                           G_LIST_MODEL (list->controller_manager),
                           clay_row_create_for_context,
                           clay_get_user_context (clay_controller_manager_get_clay (list->controller_manager)),
                           NULL);
}

static void
clay_controller_list_finalize (GObject *object)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (object);

  g_clear_object (&list->controller_manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_controller_list_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_MANAGER:
      list->controller_manager = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_controller_list_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (object);

  switch (property_id)
    {
    case PROP_CONTROLLER_MANAGER:
      g_value_set_object (value, list->controller_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
clay_controller_list_new (ClayControllerManager *controller_manager)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_MANAGER (controller_manager), NULL);

  return g_object_new (CLAY_TYPE_CONTROLLER_LIST,
                       "controller-manager", controller_manager,
                       NULL);
}


/*  private functions  */

static ClayControllerType *
clay_controller_list_get_selected_type (ClayControllerList *list)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (list->available_controllers));
  if (row == NULL)
    return NULL;

  return CLAY_CONTROLLER_TYPE (clay_row_get_viewable (CLAY_ROW (row)));
}

static void
clay_controller_list_type_row_selected (GtkListBox    *self,
                                        GtkListBoxRow *row,
                                        gpointer       user_data)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (user_data);
  gchar              *tip = NULL;

  if (row != NULL)
    {
      ClayControllerType *type;

      type = clay_controller_list_get_selected_type (list);
      if (list->add_button)
        {
          tip =
            g_strdup_printf (_("Add '%s' to the list of active controllers"),
                             clay_object_get_name (type));
          gtk_widget_set_sensitive (list->add_button, TRUE);
        }
    }
  else
    {
      if (list->add_button)
        gtk_widget_set_sensitive (list->add_button, FALSE);
    }

  if (list->add_button)
    {
      clay_help_set_help_data (list->add_button, tip, NULL);
      g_free (tip);
    }
}

static void
clay_controller_list_type_row_activated (GtkListBox    *self,
                                         GtkListBoxRow *row,
                                         gpointer       user_data)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (user_data);

  if (gtk_widget_is_sensitive (list->add_button))
    gtk_button_clicked (GTK_BUTTON (list->add_button));
}

static ClayControllerInfo *
clay_controller_list_get_selected_controller (ClayControllerList *list)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (list->active_controllers));
  if (row == NULL)
    return NULL;

  return CLAY_CONTROLLER_INFO (clay_row_get_viewable (CLAY_ROW (row)));
}

static void
clay_controller_list_row_selected (GtkListBox    *self,
                                   GtkListBoxRow *row,
                                   gpointer       user_data)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (user_data);
  ClayControllerInfo *info;
  gboolean            selected;

  info = clay_controller_list_get_selected_controller (list);
  selected = (info != NULL);

  if (list->remove_button)
    {
      gchar *tip = NULL;

      gtk_widget_set_sensitive (list->remove_button, selected);

      if (selected)
        tip =
          g_strdup_printf (_("Remove '%s' from the list of active controllers"),
                           clay_object_get_name (CLAY_OBJECT (info)));

      clay_help_set_help_data (list->remove_button, tip, NULL);
      g_free (tip);
    }

  gtk_widget_set_sensitive (list->edit_button, selected);
  gtk_widget_set_sensitive (list->up_button,   selected);
  gtk_widget_set_sensitive (list->down_button, selected);
}

static void
clay_controller_list_row_activated (GtkListBox    *self,
                                    GtkListBoxRow *row,
                                    gpointer       user_data)
{
  ClayControllerList *list = CLAY_CONTROLLER_LIST (user_data);

  if (gtk_widget_is_sensitive (list->edit_button))
    gtk_button_clicked (GTK_BUTTON (list->edit_button));
}

static void
clay_controller_list_add_clicked (GtkWidget          *button,
                                  ClayControllerList *list)
{
  ClayControllerInfo *info;
  Clay               *clay;
  ClayControllerType *type;
  GType               gtype;
  gint                position;
  GtkListBoxRow      *row;

  clay = clay_controller_manager_get_clay (list->controller_manager);

  type = clay_controller_list_get_selected_type (list);
  g_return_if_fail (type != NULL);

  gtype = clay_controller_type_get_gtype (type);
  if (gtype == CLAY_TYPE_CONTROLLER_KEYBOARD &&
      clay_controller_manager_get_keyboard (list->controller_manager) != NULL)
    {
      clay_message_literal (clay,
                            G_OBJECT (button), CLAY_MESSAGE_WARNING,
                            _("There can only be one active keyboard "
                              "controller.\n\n"
                              "You already have a keyboard controller in "
                              "your list of active controllers."));
      return;
    }
  else if (gtype == CLAY_TYPE_CONTROLLER_WHEEL &&
           clay_controller_manager_get_wheel (list->controller_manager) != NULL)
    {
      clay_message_literal (clay,
                            G_OBJECT (button), CLAY_MESSAGE_WARNING,
                            _("There can only be one active wheel "
                              "controller.\n\n"
                              "You already have a wheel controller in "
                              "your list of active controllers."));
      return;
    }

  info = clay_controller_info_new (gtype);
  clay_container_add (CLAY_CONTAINER (list->controller_manager),
                      CLAY_OBJECT (info));
  g_object_unref (info);

  /* Make sure it's selected for when we press "edit" */
  position =
    clay_container_get_child_index (CLAY_CONTAINER (list->controller_manager),
                                    CLAY_OBJECT (info));

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       position);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);

  clay_controller_list_edit_clicked (list->edit_button, list);
}

static void
clay_controller_list_remove_clicked (GtkWidget          *button,
                                     ClayControllerList *list)
{
  GtkWidget          *dialog;
  ClayControllerInfo *info;
  const gchar        *name;

#define RESPONSE_DISABLE 1

  dialog = clay_message_dialog_new (_("Remove Controller?"),
                                    CLAY_ICON_DIALOG_WARNING,
                                    GTK_WIDGET (list), GTK_DIALOG_MODAL,
                                    NULL, NULL,

                                    _("_Disable Controller"), RESPONSE_DISABLE,
                                    _("_Cancel"),             GTK_RESPONSE_CANCEL,
                                    _("_Remove Controller"),  GTK_RESPONSE_OK,

                                    NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           RESPONSE_DISABLE,
                                           -1);

  info = clay_controller_list_get_selected_controller (list);
  name = clay_object_get_name (info);
  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("Remove Controller '%s'?"), name);

  clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                             "%s",
                             _("Removing this controller from the list of "
                               "active controllers will permanently delete "
                               "all event mappings you have configured.\n\n"
                               "Selecting \"Disable Controller\" will disable "
                               "the controller without removing it."));

  switch (clay_dialog_run (CLAY_DIALOG (dialog)))
    {
    case RESPONSE_DISABLE:
      clay_controller_info_set_enabled (info, FALSE);
      break;

    case GTK_RESPONSE_OK:
      {
        GtkWidget     *editor_dialog;

        editor_dialog = g_object_get_data (G_OBJECT (info),
                                           "clay-controller-editor-dialog");

        if (editor_dialog)
          gtk_dialog_response (GTK_DIALOG (editor_dialog),
                               GTK_RESPONSE_DELETE_EVENT);

        clay_container_remove (CLAY_CONTAINER (list->controller_manager),
                               CLAY_OBJECT (info));
      }
      break;

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}

static void
clay_controller_list_edit_clicked (GtkWidget          *button,
                                   ClayControllerList *list)
{
  ClayControllerInfo *info;
  GtkWidget          *dialog;
  GtkWidget          *editor;
  Clay               *clay;

  info = clay_controller_list_get_selected_controller (list);
  dialog = g_object_get_data (G_OBJECT (info),
                              "clay-controller-editor-dialog");

  if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  dialog = clay_dialog_new (_("Configure Input Controller"),
                            "clay-controller-editor-dialog",
                            gtk_widget_get_toplevel (GTK_WIDGET (list)),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            clay_standard_help_func,
                            CLAY_HELP_PREFS_INPUT_CONTROLLERS,

                            _("_Close"), GTK_RESPONSE_CLOSE,

                            NULL);

  clay_dialog_factory_add_foreign (clay_dialog_factory_get_singleton (),
                                   "clay-controller-editor-dialog",
                                   dialog,
                                   clay_widget_get_monitor (button));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  clay = clay_controller_manager_get_clay (list->controller_manager);
  editor = clay_controller_editor_new (info,
                                       clay_get_user_context (clay));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_set_visible (editor, TRUE);

  g_object_set_data (G_OBJECT (info), "clay-controller-editor-dialog",
                     dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (clay_controller_list_edit_destroy),
                           G_OBJECT (info), 0);

  g_signal_connect_object (list, "destroy",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (list, "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);

  gtk_widget_set_visible (dialog, TRUE);
}

static void
clay_controller_list_edit_destroy (GtkWidget          *widget,
                                   ClayControllerInfo *info)
{
  g_object_set_data (G_OBJECT (info), "clay-controller-editor-dialog", NULL);
}

static void
clay_controller_list_up_clicked (GtkWidget          *button,
                                 ClayControllerList *list)
{
  ClayControllerInfo *info;
  GtkListBoxRow      *row;
  gint                old_position;

  info = clay_controller_list_get_selected_controller (list);

  old_position =
    clay_container_get_child_index (CLAY_CONTAINER (list->controller_manager),
                                    CLAY_OBJECT (info));
  if (old_position == 0)
    return;

  clay_container_reorder (CLAY_CONTAINER (list->controller_manager),
                          CLAY_OBJECT (info),
                          old_position - 1);

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       old_position - 1);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);
}

static void
clay_controller_list_down_clicked (GtkWidget          *button,
                                   ClayControllerList *list)
{
  ClayControllerInfo *info;
  GtkListBoxRow      *row;
  gint                old_position;
  gint                n_controllers;

  info = clay_controller_list_get_selected_controller (list);

  old_position =
    clay_container_get_child_index (CLAY_CONTAINER (list->controller_manager),
                                    CLAY_OBJECT (info));

  n_controllers =
    clay_container_get_n_children (CLAY_CONTAINER (list->controller_manager));

  if (old_position == n_controllers - 1)
    return;

  clay_container_reorder (CLAY_CONTAINER (list->controller_manager),
                          CLAY_OBJECT (info),
                          old_position + 1);

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list->active_controllers),
                                       old_position + 1);
  gtk_list_box_select_row (GTK_LIST_BOX (list->active_controllers), row);
}
