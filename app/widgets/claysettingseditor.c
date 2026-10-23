/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysettingseditor.c
 * Copyright (C) 2008-2025 Michael Natterer <mitch@gimp.org>
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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "operations/clay-operation-config.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayviewable.h"

#include "claycontainerlistview.h"
#include "claycontainerview.h"
#include "claysettingseditor.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_CONFIG,
  PROP_CONTAINER
};


typedef struct _ClaySettingsEditorPrivate ClaySettingsEditorPrivate;

struct _ClaySettingsEditorPrivate
{
  Clay          *clay;
  GObject       *config;
  ClayContainer *container;
  GObject       *selected_setting;
  GQuark         name_changed_handler;

  GtkWidget     *view;
  GtkWidget     *import_button;
  GtkWidget     *export_button;
  GtkWidget     *delete_button;
};

#define GET_PRIVATE(item) ((ClaySettingsEditorPrivate *) clay_settings_editor_get_instance_private ((ClaySettingsEditor *) (item)))


static void   clay_settings_editor_constructed    (GObject             *object);
static void   clay_settings_editor_finalize       (GObject             *object);
static void   clay_settings_editor_set_property   (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   clay_settings_editor_get_property   (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static void   clay_settings_editor_selection_changed
                                                  (ClayContainerView   *view,
                                                   ClaySettingsEditor  *editor);
static void   clay_settings_editor_import_clicked (GtkWidget           *widget,
                                                   ClaySettingsEditor  *editor);
static void   clay_settings_editor_export_clicked (GtkWidget           *widget,
                                                   ClaySettingsEditor  *editor);
static void   clay_settings_editor_delete_clicked (GtkWidget           *widget,
                                                   ClaySettingsEditor  *editor);
static void   clay_settings_editor_name_changed   (ClaySettings        *setting,
                                                   ClaySettingsEditor  *editor);


G_DEFINE_TYPE_WITH_PRIVATE (ClaySettingsEditor, clay_settings_editor,
                            GTK_TYPE_BOX)

#define parent_class clay_settings_editor_parent_class


static void
clay_settings_editor_class_init (ClaySettingsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_settings_editor_constructed;
  object_class->finalize     = clay_settings_editor_finalize;
  object_class->set_property = clay_settings_editor_set_property;
  object_class->get_property = clay_settings_editor_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONFIG,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_settings_editor_init (ClaySettingsEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
clay_settings_editor_constructed (GObject *object)
{
  ClaySettingsEditor        *editor  = CLAY_SETTINGS_EDITOR (object);
  ClaySettingsEditorPrivate *private = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (private->clay));
  clay_assert (CLAY_IS_CONFIG (private->config));
  clay_assert (CLAY_IS_CONTAINER (private->container));

  private->view = clay_container_list_view_new (private->container,
                                                clay_get_user_context (private->clay),
                                                CLAY_VIEW_SIZE_SMALL, 0);
  gtk_widget_set_size_request (private->view, 200, 200);
  gtk_box_pack_start (GTK_BOX (editor), private->view, TRUE, TRUE, 0);
  gtk_widget_set_visible (private->view, TRUE);

  g_signal_connect (private->view, "selection-changed",
                    G_CALLBACK (clay_settings_editor_selection_changed),
                    editor);

  private->name_changed_handler =
    clay_container_add_handler (private->container, "name-changed",
                                G_CALLBACK (clay_settings_editor_name_changed),
                                editor);

  private->import_button =
    clay_editor_add_button (CLAY_EDITOR (private->view),
                            CLAY_ICON_DOCUMENT_OPEN,
                            _("Import presets from a file"),
                            NULL,
                            G_CALLBACK (clay_settings_editor_import_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->export_button =
    clay_editor_add_button (CLAY_EDITOR (private->view),
                            CLAY_ICON_DOCUMENT_SAVE,
                            _("Export the selected presets to a file"),
                            NULL,
                            G_CALLBACK (clay_settings_editor_export_clicked),
                            NULL,
                            G_OBJECT (editor));

  private->delete_button =
    clay_editor_add_button (CLAY_EDITOR (private->view),
                            CLAY_ICON_EDIT_DELETE,
                            _("Delete the selected preset"),
                            NULL,
                            G_CALLBACK (clay_settings_editor_delete_clicked),
                            NULL,
                            G_OBJECT (editor));

  gtk_widget_set_sensitive (private->delete_button, FALSE);
}

static void
clay_settings_editor_finalize (GObject *object)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (object);

  clay_container_remove_handler (private->container,
                                 private->name_changed_handler);

  g_clear_object (&private->config);
  g_clear_object (&private->container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_settings_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      private->clay = g_value_get_object (value); /* don't dup */
      break;

    case PROP_CONFIG:
      private->config = g_value_dup_object (value);
      break;

    case PROP_CONTAINER:
      private->container = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_settings_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, private->clay);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, private->config);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, private->container);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_settings_editor_selection_changed (ClayContainerView  *view,
                                        ClaySettingsEditor *editor)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   sensitive;

  private->selected_setting =
    G_OBJECT (clay_container_view_get_1_selected (view));

  sensitive = (private->selected_setting != NULL);

  gtk_widget_set_sensitive (private->export_button, sensitive);
  gtk_widget_set_sensitive (private->delete_button, sensitive);
}

static void
clay_settings_editor_import_clicked (GtkWidget          *widget,
                                     ClaySettingsEditor *editor)
{
}

static void
clay_settings_editor_export_clicked (GtkWidget          *widget,
                                     ClaySettingsEditor *editor)
{
}

static void
clay_settings_editor_delete_clicked (GtkWidget          *widget,
                                     ClaySettingsEditor *editor)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (editor);

  if (private->selected_setting)
    {
      ClayObject *new;

      new = clay_container_get_neighbor_of (private->container,
                                            CLAY_OBJECT (private->selected_setting));

      /*  don't select the separator  */
      if (new && ! clay_object_get_name (new))
        new = NULL;

      clay_container_remove (private->container,
                             CLAY_OBJECT (private->selected_setting));

      clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (private->view),
                                          CLAY_VIEWABLE (new));

      clay_operation_config_serialize (private->clay, private->container, NULL);
    }
}

static void
clay_settings_editor_name_changed (ClaySettings      *settings,
                                  ClaySettingsEditor *editor)
{
  ClaySettingsEditorPrivate *private = GET_PRIVATE (editor);

  clay_operation_config_serialize (private->clay, private->container,
                                   NULL);
}


/*  public functions  */

GtkWidget *
clay_settings_editor_new (Clay          *clay,
                          GObject       *config,
                          ClayContainer *container)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONFIG (config), NULL);
  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);

  return g_object_new (CLAY_TYPE_SETTINGS_EDITOR,
                       "clay",      clay,
                       "config",    config,
                       "container", container,
                       NULL);
}
