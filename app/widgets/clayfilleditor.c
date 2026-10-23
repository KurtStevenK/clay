/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayfilleditor.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clayfilloptions.h"

#include "claycolorpanel.h"
#include "clayfilleditor.h"
#include "claypropwidgets.h"
#include "clayviewablebox.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_EDIT_CONTEXT,
  PROP_USE_CUSTOM_STYLE
};


static void   clay_fill_editor_constructed  (GObject             *object);
static void   clay_fill_editor_finalize     (GObject             *object);
static void   clay_fill_editor_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void   clay_fill_editor_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static void   clay_fill_editor_color_button_clicked
                                            (ClayColorPanel      *panel,
                                             ClayFillEditor      *editor);
static void   clay_fill_editor_color_button_response
                                            (ClayColorPanel      *panel,
                                             ClayColorDialogState state,
                                             ClayFillEditor      *editor);


G_DEFINE_TYPE (ClayFillEditor, clay_fill_editor, GTK_TYPE_BOX)

#define parent_class clay_fill_editor_parent_class


static void
clay_fill_editor_class_init (ClayFillEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_fill_editor_constructed;
  object_class->finalize     = clay_fill_editor_finalize;
  object_class->set_property = clay_fill_editor_set_property;
  object_class->get_property = clay_fill_editor_get_property;

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options",
                                                        NULL, NULL,
                                                        CLAY_TYPE_FILL_OPTIONS,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_EDIT_CONTEXT,
                                   g_param_spec_boolean ("edit-context",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USE_CUSTOM_STYLE,
                                   g_param_spec_boolean ("use-custom-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_fill_editor_init (ClayFillEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static void
clay_fill_editor_constructed (GObject *object)
{
  ClayFillEditor *editor = CLAY_FILL_EDITOR (object);
  GtkWidget      *box;
  GtkWidget      *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_FILL_OPTIONS (editor->options));

  if (editor->use_custom_style)
    box = clay_prop_enum_radio_box_new (G_OBJECT (editor->options),
                                        "custom-style", 0, 0);
  else
    box = clay_prop_enum_radio_box_new (G_OBJECT (editor->options), "style",
                                        0, 0);

  gtk_box_pack_start (GTK_BOX (editor), box, FALSE, FALSE, 0);

  if (editor->edit_context)
    {
      GtkWidget *color_button;
      GtkWidget *pattern_box;

      if (editor->use_custom_style)
        {
          color_button = clay_prop_color_button_new (G_OBJECT (editor->options),
                                                     "foreground",
                                                     _("Fill Color"), TRUE,
                                                     1, 24,
                                                     CLAY_COLOR_AREA_SMALL_CHECKS);
          clay_color_panel_set_context (CLAY_COLOR_PANEL (color_button),
                                        CLAY_CONTEXT (editor->options));
          clay_enum_radio_box_add (GTK_BOX (box), color_button,
                                   CLAY_CUSTOM_STYLE_SOLID_COLOR, FALSE);

          g_signal_connect_object (CLAY_COLOR_PANEL (color_button), "clicked",
                                   G_CALLBACK (clay_fill_editor_color_button_clicked),
                                   editor, 0);
          g_signal_connect_object (CLAY_COLOR_PANEL (color_button), "response",
                                   G_CALLBACK (clay_fill_editor_color_button_response),
                                   editor, 0);
        }
      else
        {
          color_button = clay_prop_color_button_new (G_OBJECT (editor->options),
                                                     "foreground",
                                                     _("Fill Color"), TRUE,
                                                     1, 24,
                                                     CLAY_COLOR_AREA_SMALL_CHECKS);
          clay_color_panel_set_context (CLAY_COLOR_PANEL (color_button),
                                        CLAY_CONTEXT (editor->options));
          clay_enum_radio_box_add (GTK_BOX (box), color_button,
                                   CLAY_FILL_STYLE_FG_COLOR, FALSE);
          clay_color_button_set_update (CLAY_COLOR_BUTTON (color_button), TRUE);

          color_button = clay_prop_color_button_new (G_OBJECT (editor->options),
                                                     "background",
                                                     _("Fill BG Color"), TRUE,
                                                     1, 24,
                                                     CLAY_COLOR_AREA_SMALL_CHECKS);
          clay_color_panel_set_context (CLAY_COLOR_PANEL (color_button),
                                        CLAY_CONTEXT (editor->options));
          clay_enum_radio_box_add (GTK_BOX (box), color_button,
                                   CLAY_FILL_STYLE_BG_COLOR, FALSE);
        }
      clay_color_button_set_update (CLAY_COLOR_BUTTON (color_button), TRUE);

      pattern_box = clay_prop_pattern_box_new (NULL,
                                               CLAY_CONTEXT (editor->options),
                                               NULL, 2,
                                               "pattern-view-type",
                                               "pattern-view-size");
      clay_enum_radio_box_add (GTK_BOX (box), pattern_box,
                               (editor->use_custom_style ?
                                (gint) CLAY_CUSTOM_STYLE_PATTERN :
                                (gint) CLAY_FILL_STYLE_PATTERN),
                               FALSE);
    }

  button = clay_prop_check_button_new (G_OBJECT (editor->options),
                                       "antialias",
                                       _("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (editor), button, FALSE, FALSE, 0);
}

static void
clay_fill_editor_finalize (GObject *object)
{
  ClayFillEditor *editor = CLAY_FILL_EDITOR (object);

  g_clear_object (&editor->options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_fill_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayFillEditor *editor = CLAY_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      editor->options = g_value_dup_object (value);
      break;

    case PROP_EDIT_CONTEXT:
      editor->edit_context = g_value_get_boolean (value);
      break;

    case PROP_USE_CUSTOM_STYLE:
      editor->use_custom_style = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_fill_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayFillEditor *editor = CLAY_FILL_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, editor->options);
      break;

    case PROP_EDIT_CONTEXT:
      g_value_set_boolean (value, editor->edit_context);
      break;

    case PROP_USE_CUSTOM_STYLE:
      g_value_set_boolean (value, editor->use_custom_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_fill_editor_new (ClayFillOptions *options,
                      gboolean         edit_context,
                      gboolean         use_custom_style)
{
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);

  return g_object_new (CLAY_TYPE_FILL_EDITOR,
                       "options",          options,
                       "edit-context",     edit_context ? TRUE : FALSE,
                       "use-custom-style", use_custom_style ? TRUE : FALSE,
                       NULL);
}

static void
clay_fill_editor_color_button_clicked (ClayColorPanel *panel,
                                       ClayFillEditor *editor)
{
  ClayFillOptions *options;

  g_return_if_fail (CLAY_IS_FILL_EDITOR (editor));

  options = editor->options;

  clay_fill_options_enable_color_history (options, FALSE);
}

static void
clay_fill_editor_color_button_response (ClayColorPanel      *panel,
                                        ClayColorDialogState state,
                                        ClayFillEditor      *editor)
{
  ClayFillOptions *options;

  g_return_if_fail (CLAY_IS_FILL_EDITOR (editor));

  options = editor->options;

  clay_fill_options_enable_color_history (options, TRUE);
}
