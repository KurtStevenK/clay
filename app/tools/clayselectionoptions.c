/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"

#include "widgets/claypropwidgets.h"
#include "widgets/claywidgets-utils.h"

#include "clayselectionoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_OPERATION,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS
};


static void          clay_selection_options_set_property  (GObject       *object,
                                                           guint          property_id,
                                                           const GValue  *value,
                                                           GParamSpec    *pspec);
static void          clay_selection_options_get_property  (GObject       *object,
                                                           guint          property_id,
                                                           GValue        *value,
                                                           GParamSpec    *pspec);

static const gchar * clay_selection_options_get_modifiers (ClayChannelOps   operation);
static void          clay_selection_options_style_updated (ClayGuiConfig   *config,
                                                           GParamSpec      *pspec,
                                                           GtkWidget       *box);



G_DEFINE_TYPE (ClaySelectionOptions, clay_selection_options,
               CLAY_TYPE_TOOL_OPTIONS)

#define parent_class clay_selection_options_parent_class


static void
clay_selection_options_class_init (ClaySelectionOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_selection_options_set_property;
  object_class->get_property = clay_selection_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_OPERATION,
                         "operation",
                         NULL, NULL,
                         CLAY_TYPE_CHANNEL_OPS,
                         CLAY_CHANNEL_OP_REPLACE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Smooth edges"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of selection edges"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_selection_options_init (ClaySelectionOptions *options)
{
}

static void
clay_selection_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClaySelectionOptions *options = CLAY_SELECTION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OPERATION:
      options->operation = g_value_get_enum (value);
      break;

    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;

    case PROP_FEATHER:
      options->feather = g_value_get_boolean (value);
      break;

    case PROP_FEATHER_RADIUS:
      options->feather_radius = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_selection_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClaySelectionOptions *options = CLAY_SELECTION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OPERATION:
      g_value_set_enum (value, options->operation);
      break;

    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;

    case PROP_FEATHER:
      g_value_set_boolean (value, options->feather);
      break;

    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, options->feather_radius);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_selection_options_gui (ClayToolOptions *tool_options)
{
  GObject              *config  = G_OBJECT (tool_options);
  ClaySelectionOptions *options = CLAY_SELECTION_OPTIONS (tool_options);
  GtkWidget            *vbox    = clay_tool_options_gui (tool_options);
  GtkWidget            *button;
  GtkWidget            *mode_box;

  /*  the selection operation radio buttons  */
  mode_box = clay_selection_options_get_mode_box (tool_options,
                                                  CLAY_CHANNEL_OP_ADD,
                                                  CLAY_CHANNEL_OP_INTERSECT);
  gtk_box_pack_start (GTK_BOX (vbox), mode_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (mode_box, TRUE);
  options->mode_box = mode_box;

  /*  the antialias toggle button  */
  button = clay_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  options->antialias_toggle = button;

  /*  the feather frame  */
  {
    GtkWidget *frame;
    GtkWidget *scale;

    /*  the feather radius scale  */
    scale = clay_prop_spin_scale_new (config, "feather-radius",
                                      1.0, 10.0, 1);

    frame = clay_prop_expanding_frame_new (config, "feather", NULL,
                                           scale, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  }

  return vbox;
}

GtkWidget *
clay_selection_options_get_mode_box (ClayToolOptions *tool_options,
                                     ClayChannelOps   min_op,
                                     ClayChannelOps   max_op)
{
  GObject       *config     = G_OBJECT (tool_options);
  ClayContext   *context    = CLAY_CONTEXT (tool_options);
  ClayGuiConfig *gui_config = CLAY_GUI_CONFIG (context->clay->config);
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *box;
  GList         *children;
  GList         *list;
  gint           i;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  box = clay_prop_enum_icon_box_new (config, "operation", "clay-selection",
                                     min_op, max_op);

  g_signal_connect_object (gui_config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (clay_selection_options_style_updated),
                           box, G_CONNECT_AFTER);
  g_signal_connect_object (gui_config,
                           "notify::custom-icon-size",
                           G_CALLBACK (clay_selection_options_style_updated),
                           box, G_CONNECT_AFTER);
  clay_selection_options_style_updated (gui_config, NULL, box);

  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);

  children = gtk_container_get_children (GTK_CONTAINER (box));

  /*  add modifier keys to the tooltips  */
  for (list = children, i = 0; list; list = list->next, i++)
    {
      GtkWidget   *button   = list->data;
      const gchar *modifier = clay_selection_options_get_modifiers (i);
      gchar       *tooltip;

      if (! modifier)
        continue;

      tooltip = gtk_widget_get_tooltip_text (button);

      if (tooltip)
        {
          gchar *tip = g_strdup_printf ("%s  <b>%s</b>", tooltip, modifier);

          clay_help_set_help_data_with_markup (button, tip, NULL);

          g_free (tip);
          g_free (tooltip);
        }
      else
        {
          clay_help_set_help_data (button, modifier, NULL);
        }
    }

  if (CLAY_CHANNEL_OP_REPLACE >= min_op &&
      CLAY_CHANNEL_OP_REPLACE <= max_op)
    /*  move CLAY_CHANNEL_OP_REPLACE to the front  */
    gtk_box_reorder_child (GTK_BOX (box),
                           GTK_WIDGET (children->next->next->data), 0);

  g_list_free (children);

  return hbox;
}

static const gchar *
clay_selection_options_get_modifiers (ClayChannelOps operation)
{
  GdkModifierType extend_mask;
  GdkModifierType modify_mask;
  GdkModifierType modifiers = 0;

  extend_mask = clay_get_extend_selection_mask ();
  modify_mask = clay_get_modify_selection_mask ();

  switch (operation)
    {
    case CLAY_CHANNEL_OP_ADD:
      modifiers = extend_mask;
      break;

    case CLAY_CHANNEL_OP_SUBTRACT:
      modifiers = modify_mask;
      break;

    case CLAY_CHANNEL_OP_REPLACE:
      modifiers = 0;
      break;

    case CLAY_CHANNEL_OP_INTERSECT:
      modifiers = extend_mask | modify_mask;
      break;
    }

  return clay_get_mod_string (modifiers);
}

static void
clay_selection_options_style_updated (ClayGuiConfig *config,
                                      GParamSpec    *pspec,
                                      GtkWidget     *box)
{
  GtkIconSize icon_size = GTK_ICON_SIZE_MENU;

  if (config->override_icon_size)
    {
      switch (config->custom_icon_size)
        {
        case CLAY_ICON_SIZE_LARGE:
          icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
          break;

        case CLAY_ICON_SIZE_HUGE:
          icon_size = GTK_ICON_SIZE_DND;
          break;

        case CLAY_ICON_SIZE_MEDIUM:
        case CLAY_ICON_SIZE_SMALL:
        default:
          icon_size = GTK_ICON_SIZE_MENU;
        }
    }

  clay_enum_icon_box_set_icon_size (box, icon_size);
}
