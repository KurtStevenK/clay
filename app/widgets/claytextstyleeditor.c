/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"

#include "tools/tools-types.h"
#include "tools/tool_manager.h"
#include "tools/claytexttool.h"

#include "text/claytext.h"
#include "text/clayfont.h"

#include "claycolorpanel.h"
#include "claycontainerentry.h"
#include "claycontainerview.h"
#include "claytextbuffer.h"
#include "claytextstyleeditor.h"
#include "claytexttag.h"
#include "claywidgets-constructors.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_TEXT,
  PROP_BUFFER,
  PROP_FONTS,
  PROP_RESOLUTION_X,
  PROP_RESOLUTION_Y
};


static void      clay_text_style_editor_constructed       (GObject             *object);
static void      clay_text_style_editor_dispose           (GObject             *object);
static void      clay_text_style_editor_finalize          (GObject             *object);
static void      clay_text_style_editor_set_property      (GObject             *object,
                                                           guint                property_id,
                                                           const GValue        *value,
                                                           GParamSpec          *pspec);
static void      clay_text_style_editor_get_property      (GObject             *object,
                                                           guint                property_id,
                                                           GValue              *value,
                                                           GParamSpec          *pspec);

static GtkWidget * clay_text_style_editor_create_toggle   (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *tag,
                                                           const gchar         *icon_name,
                                                           const gchar         *tooltip);

static void      clay_text_style_editor_clear_tags        (GtkButton           *button,
                                                           ClayTextStyleEditor *editor);

static void      clay_text_style_editor_font_changed      (ClayContext         *context,
                                                           ClayFont            *font,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_font          (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *font_tag);
static void      clay_text_style_editor_set_default_font  (ClayTextStyleEditor *editor);

static void      clay_text_style_editor_color_response    (ClayColorPanel      *panel,
                                                           ClayColorDialogState state,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_color_changed     (ClayColorButton     *button,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_color         (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *color_tag);
static void      clay_text_style_editor_set_default_color (ClayTextStyleEditor *editor);

static void      clay_text_style_editor_tag_toggled       (GtkToggleButton     *toggle,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_toggle        (ClayTextStyleEditor *editor,
                                                           GtkToggleButton     *toggle,
                                                           gboolean             active);

static void      clay_text_style_editor_size_changed      (ClaySizeEntry       *entry,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_size          (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *size_tag);
static void      clay_text_style_editor_set_default_size  (ClayTextStyleEditor *editor);

static void      clay_text_style_editor_baseline_changed  (GtkAdjustment       *adjustment,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_baseline      (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *baseline_tag);

static void      clay_text_style_editor_kerning_changed   (GtkAdjustment       *adjustment,
                                                           ClayTextStyleEditor *editor);
static void      clay_text_style_editor_set_kerning       (ClayTextStyleEditor *editor,
                                                           GtkTextTag          *kerning_tag);

static void      clay_text_style_editor_update            (ClayTextStyleEditor *editor);
static gboolean  clay_text_style_editor_update_idle       (ClayTextStyleEditor *editor);

static gboolean  clay_text_style_editor_notify_enter_dnd_handler
                                                          (GtkWidget           *widget,
                                                           GdkEventCrossing     event,
                                                           gpointer             user_data);

static gboolean  clay_text_style_editor_notify_leave_dnd_handler
                                                          (GtkWidget           *widget,
                                                           GdkEventCrossing     event,
                                                           gpointer             user_data);

static void      clay_text_style_editor_restore_position_callback
                                                          (GtkWidget           *button,
                                                           ClayTextStyleEditor *editor);


G_DEFINE_TYPE (ClayTextStyleEditor, clay_text_style_editor,
               GTK_TYPE_BOX)

#define parent_class clay_text_style_editor_parent_class


static void
clay_text_style_editor_class_init (ClayTextStyleEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = clay_text_style_editor_constructed;
  object_class->dispose      = clay_text_style_editor_dispose;
  object_class->finalize     = clay_text_style_editor_finalize;
  object_class->set_property = clay_text_style_editor_set_property;
  object_class->get_property = clay_text_style_editor_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_object ("text",
                                                        NULL, NULL,
                                                        CLAY_TYPE_TEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        NULL, NULL,
                                                        CLAY_TYPE_TEXT_BUFFER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FONTS,
                                   g_param_spec_object ("fonts",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_RESOLUTION_X,
                                   g_param_spec_double ("resolution-x",
                                                        NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        1.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RESOLUTION_Y,
                                   g_param_spec_double ("resolution-y",
                                                        NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        1.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "ClayTextStyleEditor");
}

static void
clay_text_style_editor_init (ClayTextStyleEditor *editor)
{
  GtkWidget *image;
  GeglColor *color;
  GtkWidget *main_hbox;
  GtkWidget *content_vbox;
  GtkWidget *dnd_icon;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (editor), main_hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_hbox, TRUE);

  /* XXX: In GTK4, GtkEventBox will be deprecated
   * See: https://docs.gtk.org/gtk4/migrating-3to4.html#stop-using-gtkeventbox
   * In GTK4, all widgets will receive events directly,
   * so this widget will be converted to a simple GtkWidget
   * and event handling (click, motion, enter, leave) will be done
   * via signal handlers directly on the widget.
   */
  editor->dnd_handle = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_hbox), editor->dnd_handle, FALSE, FALSE, 0);
  gtk_widget_set_margin_start (editor->dnd_handle, 4);
  gtk_widget_set_margin_end (editor->dnd_handle, 4);
  gtk_widget_set_visible (editor->dnd_handle, TRUE);

  gtk_widget_add_events (editor->dnd_handle,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);

  g_signal_connect (editor->dnd_handle, "enter-notify-event",
                    G_CALLBACK (clay_text_style_editor_notify_enter_dnd_handler),
                    editor);
  g_signal_connect (editor->dnd_handle, "leave-notify-event",
                    G_CALLBACK (clay_text_style_editor_notify_leave_dnd_handler),
                    editor);

  /* TODO: update dnd_icon with a 6-dot one */
  dnd_icon = gtk_image_new_from_icon_name (CLAY_ICON_TOOL_MOVE,
                                           GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (editor->dnd_handle), dnd_icon);
  gtk_widget_set_visible (dnd_icon, TRUE);

  clay_help_set_help_data (editor->dnd_handle,
                           _("Drag to move the text style editor"),
                           NULL);

  content_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), content_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (content_vbox, TRUE);

  /*  upper row  */

  editor->upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (content_vbox), editor->upper_hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->upper_hbox, TRUE);

  editor->font_entry = clay_container_entry_new (NULL, NULL,
                                                 CLAY_VIEW_SIZE_SMALL, 1);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->font_entry,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->font_entry, TRUE);

  clay_help_set_help_data (editor->font_entry,
                           _("Change font of selected text"), NULL);

  editor->size_entry =
    clay_size_entry_new (1, clay_unit_pixel (), "%a", TRUE, FALSE, FALSE, 10,
                         CLAY_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->size_entry,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->size_entry, TRUE);

  clay_help_set_help_data (editor->size_entry,
                           _("Change size of selected text"), NULL);

  g_signal_connect (editor->size_entry, "value-changed",
                    G_CALLBACK (clay_text_style_editor_size_changed),
                    editor);

  color = gegl_color_new ("black");
  editor->color_button = clay_color_panel_new (_("Change color of selected text"),
                                               color,
                                               CLAY_COLOR_AREA_FLAT, 20, 20);
  clay_widget_set_fully_opaque (editor->color_button, TRUE);
  g_object_unref (color);

  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->color_button,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->color_button, TRUE);

  clay_help_set_help_data (editor->color_button,
                           _("Change color of selected text"), NULL);

  g_signal_connect (CLAY_COLOR_PANEL (editor->color_button), "response",
                    G_CALLBACK (clay_text_style_editor_color_response),
                    editor);

  /* Restore position button */
  editor->restore_position_button = clay_icon_button_new (CLAY_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (editor->restore_position_button),
                         GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (editor->restore_position_button))),
                                CLAY_ICON_RESET, GTK_ICON_SIZE_MENU);

  clay_help_set_help_data (editor->restore_position_button,
                           _("Restore On-Canvas Editor Position"), NULL);

  g_signal_connect (editor->restore_position_button, "clicked",
                    G_CALLBACK (clay_text_style_editor_restore_position_callback),
                    editor);

  gtk_box_pack_end (GTK_BOX (editor->upper_hbox), editor->restore_position_button,
                    FALSE, FALSE, 0);

  /*  lower row  */

  editor->lower_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (content_vbox), editor->lower_hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->lower_hbox, TRUE);

  editor->clear_button = gtk_button_new ();
  gtk_widget_set_can_focus (editor->clear_button, FALSE);
  gtk_box_pack_start (GTK_BOX (editor->lower_hbox), editor->clear_button,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->clear_button, TRUE);

  clay_help_set_help_data (editor->clear_button,
                           _("Clear style of selected text"), NULL);

  g_signal_connect (editor->clear_button, "clicked",
                    G_CALLBACK (clay_text_style_editor_clear_tags),
                    editor);

  image = gtk_image_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (editor->clear_button), image);
  gtk_widget_set_visible (image, TRUE);

  editor->kerning_adjustment = gtk_adjustment_new (0.0, -1000.0, 1000.0,
                                                   1.0, 10.0, 0.0);
  editor->kerning_spinbutton =
    clay_spin_button_new (editor->kerning_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->kerning_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->kerning_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->kerning_spinbutton, TRUE);

  clay_help_set_help_data (editor->kerning_spinbutton,
                           _("Change kerning of selected text"), NULL);

  g_signal_connect (editor->kerning_adjustment, "value-changed",
                    G_CALLBACK (clay_text_style_editor_kerning_changed),
                    editor);

  editor->baseline_adjustment = gtk_adjustment_new (0.0, -1000.0, 1000.0,
                                                    1.0, 10.0, 0.0);
  editor->baseline_spinbutton =
    clay_spin_button_new (editor->baseline_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->baseline_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->baseline_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->baseline_spinbutton, TRUE);

  clay_help_set_help_data (editor->baseline_spinbutton,
                           _("Change baseline of selected text"), NULL);

  g_signal_connect (editor->baseline_adjustment, "value-changed",
                    G_CALLBACK (clay_text_style_editor_baseline_changed),
                    editor);
}

static void
clay_text_style_editor_constructed (GObject *object)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (editor->clay));
  clay_assert (CLAY_IS_CONTAINER (editor->fonts));
  clay_assert (CLAY_IS_TEXT (editor->text));
  clay_assert (CLAY_IS_TEXT_BUFFER (editor->buffer));

  editor->context = clay_context_new (editor->clay, "text style editor", NULL);

  g_signal_connect (editor->context, "font-changed",
                    G_CALLBACK (clay_text_style_editor_font_changed),
                    editor);

  clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (editor->size_entry), 0,
                                  editor->resolution_y, TRUE);

  /* use the global user context so we get the global FG/BG colors */
  clay_color_panel_set_context (CLAY_COLOR_PANEL (editor->color_button),
                                clay_get_user_context (editor->clay));

  clay_container_view_set_container (CLAY_CONTAINER_VIEW (editor->font_entry),
                                     editor->fonts);
  clay_container_view_set_context (CLAY_CONTAINER_VIEW (editor->font_entry),
                                   editor->context);

  clay_text_style_editor_create_toggle (editor, editor->buffer->bold_tag,
                                        CLAY_ICON_FORMAT_TEXT_BOLD,
                                        _("Bold"));
  clay_text_style_editor_create_toggle (editor, editor->buffer->italic_tag,
                                        CLAY_ICON_FORMAT_TEXT_ITALIC,
                                        _("Italic"));
  clay_text_style_editor_create_toggle (editor, editor->buffer->underline_tag,
                                        CLAY_ICON_FORMAT_TEXT_UNDERLINE,
                                        _("Underline"));
  clay_text_style_editor_create_toggle (editor, editor->buffer->strikethrough_tag,
                                        CLAY_ICON_FORMAT_TEXT_STRIKETHROUGH,
                                        _("Strikethrough"));

  g_signal_connect_swapped (editor->text, "notify::font",
                            G_CALLBACK (clay_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size",
                            G_CALLBACK (clay_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size-unit",
                            G_CALLBACK (clay_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::color",
                            G_CALLBACK (clay_text_style_editor_update),
                            editor);

  g_signal_connect_data (editor->buffer, "changed",
                         G_CALLBACK (clay_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "apply-tag",
                         G_CALLBACK (clay_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "remove-tag",
                         G_CALLBACK (clay_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "mark-set",
                         G_CALLBACK (clay_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  clay_text_style_editor_update (editor);
}

static void
clay_text_style_editor_dispose (GObject *object)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (object);

  if (editor->text)
    {
      g_signal_handlers_disconnect_by_func (editor->text,
                                            clay_text_style_editor_update,
                                            editor);
    }

  if (editor->buffer)
    {
      g_signal_handlers_disconnect_by_func (editor->buffer,
                                            clay_text_style_editor_update,
                                            editor);
    }

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_text_style_editor_font_changed,
                                            editor);
    }

  if (editor->update_idle_id)
    {
      g_source_remove (editor->update_idle_id);
      editor->update_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_text_style_editor_finalize (GObject *object)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (object);

  g_clear_object (&editor->context);
  g_clear_object (&editor->text);
  g_clear_object (&editor->buffer);
  g_clear_object (&editor->fonts);

  if (editor->toggles)
    {
      g_list_free (editor->toggles);
      editor->toggles = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_text_style_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_CLAY:
      editor->clay = g_value_get_object (value); /* don't ref */
      break;
    case PROP_TEXT:
      editor->text = g_value_dup_object (value);
      break;
    case PROP_BUFFER:
      editor->buffer = g_value_dup_object (value);
      break;
    case PROP_FONTS:
      editor->fonts = g_value_dup_object (value);
      break;
    case PROP_RESOLUTION_X:
      editor->resolution_x = g_value_get_double (value);
      break;
    case PROP_RESOLUTION_Y:
      editor->resolution_y = g_value_get_double (value);
      if (editor->size_entry)
        clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (editor->size_entry), 0,
                                        editor->resolution_y, TRUE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_style_editor_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, editor->clay);
      break;
    case PROP_TEXT:
      g_value_set_object (value, editor->text);
      break;
    case PROP_BUFFER:
      g_value_set_object (value, editor->buffer);
      break;
    case PROP_FONTS:
      g_value_set_object (value, editor->fonts);
      break;
    case PROP_RESOLUTION_X:
      g_value_set_double (value, editor->resolution_x);
      break;
    case PROP_RESOLUTION_Y:
      g_value_set_double (value, editor->resolution_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
clay_text_style_editor_new (Clay           *clay,
                            ClayText       *text,
                            ClayTextBuffer *buffer,
                            ClayContainer  *fonts,
                            gdouble         resolution_x,
                            gdouble         resolution_y)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT (text), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (resolution_x > 0.0, NULL);
  g_return_val_if_fail (resolution_y > 0.0, NULL);

  return g_object_new (CLAY_TYPE_TEXT_STYLE_EDITOR,
                       "clay",         clay,
                       "text",         text,
                       "buffer",       buffer,
                       "fonts",        fonts,
                       "resolution-x", resolution_x,
                       "resolution-y", resolution_y,
                       NULL);
}

GList *
clay_text_style_editor_list_tags (ClayTextStyleEditor  *editor,
                                  GList               **remove_tags)
{
  GList *toggles;
  GList *tags = NULL;

  g_return_val_if_fail (CLAY_IS_TEXT_STYLE_EDITOR (editor), NULL);
  g_return_val_if_fail (remove_tags != NULL, NULL);

  *remove_tags = NULL;

  for (toggles = editor->toggles; toggles; toggles = g_list_next (toggles))
    {
      GtkTextTag *tag = g_object_get_data (toggles->data, "tag");

      if (gtk_toggle_button_get_active (toggles->data))
        {
          tags = g_list_prepend (tags, tag);
        }
      else
        {
          *remove_tags = g_list_prepend (*remove_tags, tag);
        }
    }

  {
    GList   *list;
    gdouble  pixels;

    for (list = editor->buffer->size_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    pixels = clay_size_entry_get_refval (CLAY_SIZE_ENTRY (editor->size_entry), 0);

    if (pixels != 0.0)
      {
        GtkTextTag *tag;
        gdouble     points;

        points = clay_units_to_points (pixels,
                                       clay_unit_pixel (),
                                       editor->resolution_y);
        tag = clay_text_buffer_get_size_tag (editor->buffer,
                                             PANGO_SCALE * points);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList       *list;
    const gchar *font_name;

    for (list = editor->buffer->font_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    font_name = clay_context_get_font_name (editor->context);

    if (font_name)
      {
        GtkTextTag  *tag;

        tag = clay_text_buffer_get_font_tag (editor->buffer, font_name);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList     *list;
    GeglColor *color;

    for (list = editor->buffer->color_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    color = clay_color_button_get_color (CLAY_COLOR_BUTTON (editor->color_button));

    if (TRUE) /* FIXME should have "inconsistent" state as for font and size */
      {
        GtkTextTag *tag;

        tag = clay_text_buffer_get_color_tag (editor->buffer, color);
        tags = g_list_prepend (tags, tag);
      }

    g_object_unref (color);
  }

  *remove_tags = g_list_reverse (*remove_tags);

  return g_list_reverse (tags);
}

void
clay_text_style_show_restore_position_button (ClayTextStyleEditor *editor,
                                              gboolean             show)
{
  if (show)
    gtk_widget_set_visible (editor->restore_position_button, TRUE);
  else
    gtk_widget_set_visible (editor->restore_position_button, FALSE);
}


/*  private functions  */

static GtkWidget *
clay_text_style_editor_create_toggle (ClayTextStyleEditor *editor,
                                      GtkTextTag          *tag,
                                      const gchar         *icon_name,
                                      const gchar         *tooltip)
{
  GtkWidget *toggle;
  GtkWidget *image;

  toggle = gtk_toggle_button_new ();
  gtk_widget_set_can_focus (toggle, FALSE);
  gtk_box_pack_start (GTK_BOX (editor->lower_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (toggle, TRUE);

  clay_help_set_help_data (toggle, tooltip, NULL);

  editor->toggles = g_list_append (editor->toggles, toggle);
  g_object_set_data (G_OBJECT (toggle), "tag", tag);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (clay_text_style_editor_tag_toggled),
                    editor);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_set_visible (image, TRUE);

  return toggle;
}

static void
clay_text_style_editor_clear_tags (GtkButton           *button,
                                   ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gtk_text_buffer_begin_user_action (buffer);

      gtk_text_buffer_remove_all_tags (buffer, &start, &end);

      gtk_text_buffer_end_user_action (buffer);
    }
}

static void
clay_text_style_editor_font_changed (ClayContext         *context,
                                     ClayFont            *font,
                                     ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      clay_text_buffer_set_font (editor->buffer, &start, &end,
                                 clay_context_get_font_name (context));
    }

  insert_tags = clay_text_style_editor_list_tags (editor, &remove_tags);
  clay_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
clay_text_style_editor_set_font (ClayTextStyleEditor *editor,
                                 GtkTextTag          *font_tag)
{
  gchar *font = NULL;

  if (font_tag)
    font = clay_text_tag_get_font (font_tag);

  g_signal_handlers_block_by_func (editor->context,
                                   clay_text_style_editor_font_changed,
                                   editor);

  clay_context_set_font_name (editor->context, font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     clay_text_style_editor_font_changed,
                                     editor);

  g_free (font);
}

static void
clay_text_style_editor_set_default_font (ClayTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (editor->context,
                                   clay_text_style_editor_font_changed,
                                   editor);

  clay_context_set_font (editor->context, editor->text->font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     clay_text_style_editor_font_changed,
                                     editor);
}

static void
clay_text_style_editor_color_changed (ClayColorButton     *button,
                                      ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GeglColor   *color;
      GtkTextIter  start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
      color = clay_color_button_get_color (button);
      clay_text_buffer_set_color (editor->buffer, &start, &end, color);

      g_object_unref (color);
    }

  insert_tags = clay_text_style_editor_list_tags (editor, &remove_tags);
  clay_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
clay_text_style_editor_color_response (ClayColorPanel       *panel,
                                       ClayColorDialogState  state,
                                       ClayTextStyleEditor  *editor)
{
  if (state == CLAY_COLOR_DIALOG_OK)
    clay_text_style_editor_color_changed (CLAY_COLOR_BUTTON (panel),
                                          editor);

}

static void
clay_text_style_editor_set_color (ClayTextStyleEditor *editor,
                                  GtkTextTag          *color_tag)
{
  GeglColor *color = NULL;

  if (color_tag)
    clay_text_tag_get_fg_color (color_tag, &color);

  if (! color)
    color = gegl_color_new ("black");

  clay_color_button_set_color (CLAY_COLOR_BUTTON (editor->color_button),
                               color);
  g_clear_object (&color);

  /* FIXME should have "inconsistent" state as for font and size */
}

static void
clay_text_style_editor_set_default_color (ClayTextStyleEditor *editor)
{
  clay_color_button_set_color (CLAY_COLOR_BUTTON (editor->color_button), editor->text->color);
}

static void
clay_text_style_editor_tag_toggled (GtkToggleButton     *toggle,
                                    ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextTag    *tag    = g_object_get_data (G_OBJECT (toggle), "tag");
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gtk_text_buffer_begin_user_action (buffer);

      if (gtk_toggle_button_get_active (toggle))
        {
          gtk_text_buffer_apply_tag (buffer, tag, &start, &end);
        }
      else
        {
          gtk_text_buffer_remove_tag (buffer, tag, &start, &end);
        }

      gtk_text_buffer_end_user_action (buffer);
    }

  insert_tags = clay_text_style_editor_list_tags (editor, &remove_tags);
  clay_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
clay_text_style_editor_set_toggle (ClayTextStyleEditor *editor,
                                   GtkToggleButton     *toggle,
                                   gboolean             active)
{
  g_signal_handlers_block_by_func (toggle,
                                   clay_text_style_editor_tag_toggled,
                                   editor);

  gtk_toggle_button_set_active (toggle, active);

  g_signal_handlers_unblock_by_func (toggle,
                                     clay_text_style_editor_tag_toggled,
                                     editor);
}

static void
clay_text_style_editor_size_changed (ClaySizeEntry       *entry,
                                     ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;
  GtkTextIter    start, end;
  gdouble        points;

  points = clay_units_to_points (clay_size_entry_get_refval (entry, 0),
                                 clay_unit_pixel (), editor->resolution_y);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
    }
  else
    {
      gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
      gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
    }

  clay_text_buffer_set_size (editor->buffer, &start, &end,
                             PANGO_SCALE * points);

  insert_tags = clay_text_style_editor_list_tags (editor, &remove_tags);
  clay_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
clay_text_style_editor_set_size (ClayTextStyleEditor *editor,
                                 GtkTextTag          *size_tag)
{
  gint    size = 0;
  gdouble pixels;

  if (size_tag)
    size = clay_text_tag_get_size (size_tag);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   clay_text_style_editor_size_changed,
                                   editor);

  pixels = clay_units_to_pixels ((gdouble) size / PANGO_SCALE,
                                 clay_unit_point (),
                                 editor->resolution_y);
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (editor->size_entry), 0, pixels);

  if (size == 0)
    {
      GtkWidget *spinbutton;

      spinbutton = clay_size_entry_get_help_widget (CLAY_SIZE_ENTRY (editor->size_entry), 0);

      gtk_entry_set_text (GTK_ENTRY (spinbutton), "");
    }

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     clay_text_style_editor_size_changed,
                                     editor);
}

static void
clay_text_style_editor_set_default_size (ClayTextStyleEditor *editor)
{
  gdouble pixels = clay_units_to_pixels (editor->text->font_size,
                                         editor->text->unit,
                                         editor->resolution_y);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   clay_text_style_editor_size_changed,
                                   editor);

  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (editor->size_entry), 0, pixels);

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     clay_text_style_editor_size_changed,
                                     editor);
}

static void
clay_text_style_editor_baseline_changed (GtkAdjustment       *adjustment,
                                         ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextIter    start, end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      gtk_text_buffer_get_end_iter (buffer, &end);
    }

  clay_text_buffer_set_baseline (editor->buffer, &start, &end,
                                 gtk_adjustment_get_value (adjustment) *
                                 PANGO_SCALE);
}

static void
clay_text_style_editor_set_baseline (ClayTextStyleEditor *editor,
                                     GtkTextTag          *baseline_tag)
{
  gint baseline = 0;

  if (baseline_tag)
    baseline = clay_text_tag_get_baseline (baseline_tag);

  g_signal_handlers_block_by_func (editor->baseline_adjustment,
                                   clay_text_style_editor_baseline_changed,
                                   editor);

  if (gtk_adjustment_get_value (editor->baseline_adjustment) !=
      (gdouble) baseline / PANGO_SCALE)
    {
      gtk_adjustment_set_value (editor->baseline_adjustment,
                                (gdouble) baseline / PANGO_SCALE);
    }
  else
    {
      /* make sure the "" really gets replaced */
      g_signal_emit_by_name (editor->baseline_adjustment, "value-changed");
    }

  g_signal_handlers_unblock_by_func (editor->baseline_adjustment,
                                     clay_text_style_editor_baseline_changed,
                                     editor);
}

static void
clay_text_style_editor_kerning_changed (GtkAdjustment       *adjustment,
                                        ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextIter    start, end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      end = start;
      gtk_text_iter_forward_char (&end);
    }

  clay_text_buffer_set_kerning (editor->buffer, &start, &end,
                                gtk_adjustment_get_value (adjustment) *
                                PANGO_SCALE);
}

static void
clay_text_style_editor_set_kerning (ClayTextStyleEditor *editor,
                                    GtkTextTag          *kerning_tag)
{
  gint kerning = 0;

  if (kerning_tag)
    kerning = clay_text_tag_get_kerning (kerning_tag);

  g_signal_handlers_block_by_func (editor->kerning_adjustment,
                                   clay_text_style_editor_kerning_changed,
                                   editor);

  if (gtk_adjustment_get_value (editor->baseline_adjustment) !=
      (gdouble) kerning / PANGO_SCALE)
    {
      gtk_adjustment_set_value (editor->kerning_adjustment,
                                (gdouble) kerning / PANGO_SCALE);
    }
  else
    {
      /* make sure the "" really gets replaced */
      g_signal_emit_by_name (editor->kerning_adjustment, "value-changed");
    }

  g_signal_handlers_unblock_by_func (editor->kerning_adjustment,
                                     clay_text_style_editor_kerning_changed,
                                     editor);
}

static void
clay_text_style_editor_update (ClayTextStyleEditor *editor)
{
  if (editor->update_idle_id)
    g_source_remove (editor->update_idle_id);

  editor->update_idle_id =
    gdk_threads_add_idle ((GSourceFunc) clay_text_style_editor_update_idle,
                          editor);
}

static gboolean
clay_text_style_editor_update_idle (ClayTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);

  if (editor->update_idle_id)
    {
      g_source_remove (editor->update_idle_id);
      editor->update_idle_id = 0;
    }

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter  start, end;
      GtkTextIter  iter;
      GList       *list;
      gboolean     any_toggle_active = TRUE;
      gboolean     font_differs      = FALSE;
      gboolean     color_differs     = FALSE;
      gboolean     size_differs      = FALSE;
      gboolean     baseline_differs  = FALSE;
      gboolean     kerning_differs   = FALSE;
      GtkTextTag  *font_tag          = NULL;
      GtkTextTag  *color_tag         = NULL;
      GtkTextTag  *size_tag          = NULL;
      GtkTextTag  *baseline_tag      = NULL;
      GtkTextTag  *kerning_tag       = NULL;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
      gtk_text_iter_order (&start, &end);

      /*  first, switch all toggles on  */
      for (list = editor->toggles; list; list = g_list_next (list))
        {
          GtkToggleButton *toggle = list->data;

          clay_text_style_editor_set_toggle (editor, toggle, TRUE);
        }

      /*  and get some initial values  */
      font_tag     = clay_text_buffer_get_iter_font (editor->buffer,
                                                     &start, NULL);
      color_tag    = clay_text_buffer_get_iter_color (editor->buffer,
                                                      &start, NULL);
      size_tag     = clay_text_buffer_get_iter_size (editor->buffer,
                                                     &start, NULL);
      baseline_tag = clay_text_buffer_get_iter_baseline (editor->buffer,
                                                         &start, NULL);
      kerning_tag  = clay_text_buffer_get_iter_kerning (editor->buffer,
                                                        &start, NULL);

      for (iter = start;
           gtk_text_iter_in_range (&iter, &start, &end);
           gtk_text_iter_forward_cursor_position (&iter))
        {
          if (any_toggle_active)
            {
              any_toggle_active = FALSE;

              for (list = editor->toggles; list; list = g_list_next (list))
                {
                  GtkToggleButton *toggle = list->data;
                  GtkTextTag      *tag    = g_object_get_data (G_OBJECT (toggle),
                                                               "tag");

                  if (! gtk_text_iter_has_tag (&iter, tag))
                    {
                      clay_text_style_editor_set_toggle (editor, toggle, FALSE);
                    }
                  else
                    {
                      any_toggle_active = TRUE;
                    }
                }
            }

          if (! font_differs)
            {
              GtkTextTag *tag;

              tag = clay_text_buffer_get_iter_font (editor->buffer, &iter,
                                                    NULL);

              if (tag != font_tag)
                font_differs = TRUE;
            }

          if (! color_differs)
            {
              GtkTextTag *tag;

              tag = clay_text_buffer_get_iter_color (editor->buffer, &iter,
                                                     NULL);

              if (tag != color_tag)
                color_differs = TRUE;
            }

          if (! size_differs)
            {
              GtkTextTag *tag;

              tag = clay_text_buffer_get_iter_size (editor->buffer, &iter,
                                                    NULL);

              if (tag != size_tag)
                size_differs = TRUE;
            }

          if (! baseline_differs)
            {
              GtkTextTag *tag;

              tag = clay_text_buffer_get_iter_baseline (editor->buffer, &iter,
                                                        NULL);

              if (tag != baseline_tag)
                baseline_differs = TRUE;
            }

          if (! kerning_differs)
            {
              GtkTextTag *tag;

              tag = clay_text_buffer_get_iter_kerning (editor->buffer, &iter,
                                                       NULL);

              if (tag != kerning_tag)
                kerning_differs = TRUE;
            }

          if (! any_toggle_active &&
              color_differs       &&
              font_differs        &&
              size_differs        &&
              baseline_differs    &&
              kerning_differs)
            break;
       }

      if (font_differs)
        clay_text_style_editor_set_font (editor, NULL);
      else if (font_tag)
        clay_text_style_editor_set_font (editor, font_tag);
      else
        clay_text_style_editor_set_default_font (editor);

      if (color_differs)
        clay_text_style_editor_set_color (editor, NULL);
      else if (color_tag)
        clay_text_style_editor_set_color (editor, color_tag);
      else
        clay_text_style_editor_set_default_color (editor);

      if (size_differs)
        clay_text_style_editor_set_size (editor, NULL);
      else if (size_tag)
        clay_text_style_editor_set_size (editor, size_tag);
      else
        clay_text_style_editor_set_default_size (editor);

      if (baseline_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->baseline_spinbutton), "");
      else
        clay_text_style_editor_set_baseline (editor, baseline_tag);

      if (kerning_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->kerning_spinbutton), "");
      else
        clay_text_style_editor_set_kerning (editor, kerning_tag);
    }
  else /* no selection */
    {
      GtkTextIter  cursor;
      GSList      *tags;
      GSList      *tags_on;
      GSList      *tags_off;
      GList       *list;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      tags     = gtk_text_iter_get_tags (&cursor);
      tags_on  = gtk_text_iter_get_toggled_tags (&cursor, TRUE);
      tags_off = gtk_text_iter_get_toggled_tags (&cursor, FALSE);

      for (list = editor->buffer->font_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              clay_text_style_editor_set_font (editor, tag);
              break;
            }
        }

      if (! list)
        clay_text_style_editor_set_default_font (editor);

      for (list = editor->buffer->color_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              clay_text_style_editor_set_color (editor, tag);
              break;
            }
        }

      if (! list)
        clay_text_style_editor_set_default_color (editor);

      for (list = editor->buffer->size_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              clay_text_style_editor_set_size (editor, tag);
              break;
            }
        }

      if (! list)
        clay_text_style_editor_set_default_size (editor);

      for (list = editor->buffer->baseline_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              clay_text_style_editor_set_baseline (editor, tag);
              break;
            }
        }

      if (! list)
        clay_text_style_editor_set_baseline (editor, NULL);

      for (list = editor->toggles; list; list = g_list_next (list))
        {
          GtkToggleButton *toggle = list->data;
          GtkTextTag      *tag    = g_object_get_data (G_OBJECT (toggle),
                                                       "tag");

          clay_text_style_editor_set_toggle (editor, toggle,
                                             (g_slist_find (tags, tag) &&
                                              ! g_slist_find (tags_on, tag)) ||
                                             g_slist_find (tags_off, tag));
        }

      {
        GtkTextTag *tag;

        tag = clay_text_buffer_get_iter_kerning (editor->buffer, &cursor, NULL);
        clay_text_style_editor_set_kerning (editor, tag);
      }

      g_slist_free (tags);
      g_slist_free (tags_on);
      g_slist_free (tags_off);
    }

  if (editor->context->font_name &&
      g_strcmp0 (editor->context->font_name,
                 clay_font_get_lookup_name (clay_context_get_font (editor->context))))
    {
      /* A font is set, but is unavailable; change the help text. */
      gchar *help_text;

      help_text = g_strdup_printf (_("Font \"%s\" unavailable on this system"),
                                   editor->context->font_name);
      clay_help_set_help_data (editor->font_entry, help_text, NULL);
      g_free (help_text);
    }
  else
    {
      clay_help_set_help_data (editor->font_entry,
                               _("Change font of selected text"), NULL);
    }

  return FALSE;
}

static gboolean
clay_text_style_editor_notify_enter_dnd_handler (GtkWidget       *widget,
                                                 GdkEventCrossing event,
                                                 gpointer         user_data)
{
  GdkWindow *window = gtk_widget_get_window (widget);

  if (window)
    {
      GdkCursor *cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                                      GDK_FLEUR);

      gdk_window_set_cursor (window, cursor);

      if (cursor)
        g_object_unref (cursor);
    }

  return FALSE;
}

static gboolean
clay_text_style_editor_notify_leave_dnd_handler (GtkWidget       *widget,
                                                 GdkEventCrossing event,
                                                 gpointer         user_data)
{
  GdkWindow *window = gtk_widget_get_window (widget);

  if (window)
    gdk_window_set_cursor (window, NULL);

  return FALSE;
}

static void
clay_text_style_editor_restore_position_callback (GtkWidget           *button,
                                                  ClayTextStyleEditor *editor)
{
  ClayTool     *tool      = tool_manager_get_active (editor->clay);
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (tool);

  clay_text_tool_restore_on_canvas_editor_position (text_tool);
}
