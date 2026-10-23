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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"
#include "core/claytoolpreset.h"

#include "claydocked.h"
#include "claytoolpreseteditor.h"
#include "claymenufactory.h"
#include "claypropwidgets.h"

#include "clay-intl.h"


struct _ClayToolPresetEditorPrivate
{
  ClayToolPreset *tool_preset_model;

  GtkWidget      *tool_icon;
  GtkWidget      *tool_label;

  GtkWidget      *fg_bg_toggle;
  GtkWidget      *opacity_paint_mode_toggle;
  GtkWidget      *brush_toggle;
  GtkWidget      *dynamics_toggle;
  GtkWidget      *mybrush_toggle;
  GtkWidget      *gradient_toggle;
  GtkWidget      *pattern_toggle;
  GtkWidget      *palette_toggle;
  GtkWidget      *font_toggle;
};


/*  local function prototypes  */

static void   clay_tool_preset_editor_constructed    (GObject              *object);
static void   clay_tool_preset_editor_finalize       (GObject              *object);

static void   clay_tool_preset_editor_style_updated (GtkWidget             *widget);

static void   clay_tool_preset_editor_set_data       (ClayDataEditor       *editor,
                                                      ClayData             *data);

static void   clay_tool_preset_editor_sync_data      (ClayToolPresetEditor *editor);
static void   clay_tool_preset_editor_notify_model   (ClayToolPreset       *options,
                                                      const GParamSpec     *pspec,
                                                      ClayToolPresetEditor *editor);
static void   clay_tool_preset_editor_notify_data    (ClayToolPreset       *options,
                                                      const GParamSpec     *pspec,
                                                      ClayToolPresetEditor *editor);



G_DEFINE_TYPE_WITH_CODE (ClayToolPresetEditor, clay_tool_preset_editor,
                         CLAY_TYPE_DATA_EDITOR,
                         G_ADD_PRIVATE (ClayToolPresetEditor)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED, NULL))

#define parent_class clay_tool_preset_editor_parent_class


static void
clay_tool_preset_editor_class_init (ClayToolPresetEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayDataEditorClass *editor_class = CLAY_DATA_EDITOR_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = clay_tool_preset_editor_constructed;
  object_class->finalize    = clay_tool_preset_editor_finalize;

  editor_class->set_data    = clay_tool_preset_editor_set_data;
  editor_class->title       = _("Tool Preset Editor");

  widget_class->style_updated = clay_tool_preset_editor_style_updated;
}

static void
clay_tool_preset_editor_init (ClayToolPresetEditor *editor)
{
  editor->priv = clay_tool_preset_editor_get_instance_private (editor);
}

static void
clay_tool_preset_editor_constructed (GObject *object)
{
  ClayToolPresetEditor *editor      = CLAY_TOOL_PRESET_EDITOR (object);
  ClayDataEditor       *data_editor = CLAY_DATA_EDITOR (editor);
  ClayToolPreset       *preset;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  preset = editor->priv->tool_preset_model =
    g_object_new (CLAY_TYPE_TOOL_PRESET,
                  "clay", data_editor->context->clay,
                  NULL);

  g_signal_connect (preset, "notify",
                    G_CALLBACK (clay_tool_preset_editor_notify_model),
                    editor);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  editor->priv->tool_icon = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->priv->tool_icon,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->priv->tool_icon, TRUE);

  editor->priv->tool_label = gtk_label_new ("");
  clay_label_set_attributes (GTK_LABEL (editor->priv->tool_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), editor->priv->tool_label,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->priv->tool_label, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  label = gtk_label_new (_("Icon:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  button = clay_prop_icon_picker_new (CLAY_VIEWABLE (preset),
                                      data_editor->context->clay);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = editor->priv->fg_bg_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-fg-bg", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->opacity_paint_mode_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-opacity-paint-mode", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->brush_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-brush", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->dynamics_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-dynamics", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->mybrush_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-mypaint-brush", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->gradient_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-gradient", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->pattern_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-pattern", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->palette_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-palette", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->font_toggle =
    clay_prop_check_button_new (G_OBJECT (preset), "use-font", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = clay_editor_add_action_button (CLAY_EDITOR (editor),
                                          "tool-preset-editor",
                                          "tool-preset-editor-save", NULL);

  button = clay_editor_add_action_button (CLAY_EDITOR (editor),
                                          "tool-preset-editor",
                                          "tool-preset-editor-restore", NULL);

  g_signal_connect_object (data_editor->context->clay->config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (clay_tool_preset_editor_style_updated),
                           editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (data_editor->context->clay->config,
                           "notify::custom-icon-size",
                           G_CALLBACK (clay_tool_preset_editor_style_updated),
                           editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  if (data_editor->data)
    clay_tool_preset_editor_sync_data (editor);
}

static void
clay_tool_preset_editor_finalize (GObject *object)
{
  ClayToolPresetEditor *editor = CLAY_TOOL_PRESET_EDITOR (object);

  g_clear_object (&editor->priv->tool_preset_model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_tool_preset_editor_style_updated (GtkWidget *widget)
{
  ClayToolPresetEditor *editor    = CLAY_TOOL_PRESET_EDITOR (widget);
  gint                  icon_size = 16;
  GtkIconSize           button_icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "button-icon-size", &button_icon_size,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_size, NULL);
  gtk_image_set_pixel_size (GTK_IMAGE (editor->priv->tool_icon), icon_size);
}


static void
clay_tool_preset_editor_set_data (ClayDataEditor *editor,
                                  ClayData       *data)
{
  ClayToolPresetEditor *preset_editor = CLAY_TOOL_PRESET_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          clay_tool_preset_editor_notify_data,
                                          editor);

  CLAY_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      g_signal_connect (editor->data, "notify",
                        G_CALLBACK (clay_tool_preset_editor_notify_data),
                        editor);

      if (preset_editor->priv->tool_preset_model)
        clay_tool_preset_editor_sync_data (preset_editor);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (editor), editor->data_editable);
}


/*  public functions  */

GtkWidget *
clay_tool_preset_editor_new (ClayContext     *context,
                             ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_TOOL_PRESET_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolPresetEditor>",
                       "ui-path",         "/tool-preset-editor-popup",
                       "data-factory",    context->clay->tool_preset_factory,
                       "context",         context,
                       "data",            clay_context_get_tool_preset (context),
                       NULL);
}


/*  private functions  */

static void
clay_tool_preset_editor_sync_data (ClayToolPresetEditor *editor)
{
  ClayToolPresetEditorPrivate *priv        = editor->priv;
  ClayDataEditor              *data_editor = CLAY_DATA_EDITOR (editor);
  ClayToolPreset              *preset;
  ClayToolInfo                *tool_info;
  ClayContextPropMask          serialize_props;
  const gchar                 *icon_name;
  gchar                       *label;

  g_signal_handlers_block_by_func (priv->tool_preset_model,
                                   clay_tool_preset_editor_notify_model,
                                   editor);

  clay_config_sync (G_OBJECT (data_editor->data),
                    G_OBJECT (priv->tool_preset_model),
                    CLAY_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (priv->tool_preset_model,
                                     clay_tool_preset_editor_notify_model,
                                     editor);

  if (! priv->tool_preset_model->tool_options)
    return;

  tool_info = priv->tool_preset_model->tool_options->tool_info;

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));
  label     = g_strdup_printf (_("%s Preset"), tool_info->label);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->tool_icon),
                                icon_name, GTK_ICON_SIZE_MENU);
  gtk_label_set_text (GTK_LABEL (priv->tool_label), label);

  g_free (label);

  preset = CLAY_TOOL_PRESET (data_editor->data);

  serialize_props =
    clay_context_get_serialize_properties (CLAY_CONTEXT (preset->tool_options));

  gtk_widget_set_sensitive (priv->fg_bg_toggle,
                            (serialize_props &
                             (CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                              CLAY_CONTEXT_PROP_MASK_BACKGROUND)) != 0);
  gtk_widget_set_sensitive (priv->opacity_paint_mode_toggle,
                            (serialize_props &
                             (CLAY_CONTEXT_PROP_MASK_OPACITY |
                              CLAY_CONTEXT_PROP_MASK_PAINT_MODE)) != 0);
  gtk_widget_set_sensitive (priv->brush_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_BRUSH) != 0);
  gtk_widget_set_sensitive (priv->dynamics_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_DYNAMICS) != 0);
  gtk_widget_set_sensitive (priv->mybrush_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_MYBRUSH) != 0);
  gtk_widget_set_sensitive (priv->gradient_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_GRADIENT) != 0);
  gtk_widget_set_sensitive (priv->pattern_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_PATTERN) != 0);
  gtk_widget_set_sensitive (priv->palette_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_PALETTE) != 0);
  gtk_widget_set_sensitive (priv->font_toggle,
                            (serialize_props &
                             CLAY_CONTEXT_PROP_MASK_FONT) != 0);
 }

static void
clay_tool_preset_editor_notify_model (ClayToolPreset       *options,
                                      const GParamSpec     *pspec,
                                      ClayToolPresetEditor *editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);

  if (data_editor->data)
    {
      g_signal_handlers_block_by_func (data_editor->data,
                                       clay_tool_preset_editor_notify_data,
                                       editor);

      clay_config_sync (G_OBJECT (editor->priv->tool_preset_model),
                        G_OBJECT (data_editor->data),
                        CLAY_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (data_editor->data,
                                         clay_tool_preset_editor_notify_data,
                                         editor);
    }
}

static void
clay_tool_preset_editor_notify_data (ClayToolPreset       *options,
                                     const GParamSpec     *pspec,
                                     ClayToolPresetEditor *editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);

  g_signal_handlers_block_by_func (editor->priv->tool_preset_model,
                                   clay_tool_preset_editor_notify_model,
                                   editor);

  clay_config_sync (G_OBJECT (data_editor->data),
                    G_OBJECT (editor->priv->tool_preset_model),
                    CLAY_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (editor->priv->tool_preset_model,
                                     clay_tool_preset_editor_notify_model,
                                     editor);
}
