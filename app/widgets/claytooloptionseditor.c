/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooloptionseditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claylist.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"

#include "claydnd.h"
#include "claydocked.h"
#include "clayhelp-ids.h"
#include "claymenufactory.h"
#include "claypropwidgets.h"
#include "clayview.h"
#include "clayviewrenderer.h"
#include "claytooloptionseditor.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"

enum
{
  PROP_0,
  PROP_CLAY,
};

struct _ClayToolOptionsEditorPrivate
{
  Clay            *clay;

  GtkWidget       *scrolled_window;
  GtkWidget       *options_vbox;
  GtkWidget       *title_label;

  GtkWidget       *save_button;
  GtkWidget       *restore_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  ClayToolOptions *visible_tool_options;
};


static void        clay_tool_options_editor_docked_iface_init (ClayDockedInterface   *iface);
static void        clay_tool_options_editor_constructed       (GObject               *object);
static void        clay_tool_options_editor_dispose           (GObject               *object);
static void        clay_tool_options_editor_set_property      (GObject               *object,
                                                               guint                  property_id,
                                                               const GValue          *value,
                                                               GParamSpec            *pspec);
static void        clay_tool_options_editor_get_property      (GObject               *object,
                                                               guint                  property_id,
                                                               GValue                *value,
                                                               GParamSpec            *pspec);

static GtkWidget * clay_tool_options_editor_get_preview       (ClayDocked            *docked,
                                                               ClayContext           *context,
                                                               GtkIconSize            size);
static gchar     * clay_tool_options_editor_get_title         (ClayDocked            *docked);
static gboolean    clay_tool_options_editor_get_prefer_icon   (ClayDocked            *docked);
static void        clay_tool_options_editor_save_clicked      (GtkWidget             *widget,
                                                               ClayToolOptionsEditor *editor);
static void        clay_tool_options_editor_restore_clicked   (GtkWidget             *widget,
                                                               ClayToolOptionsEditor *editor);
static void        clay_tool_options_editor_delete_clicked    (GtkWidget             *widget,
                                                               ClayToolOptionsEditor *editor);
static void        clay_tool_options_editor_drop_tool         (GtkWidget             *widget,
                                                               gint                   x,
                                                               gint                   y,
                                                               ClayViewable          *viewable,
                                                               gpointer               data);
static void        clay_tool_options_editor_tool_changed      (ClayContext           *context,
                                                               ClayToolInfo          *tool_info,
                                                               ClayToolOptionsEditor *editor);
static void        clay_tool_options_editor_presets_update    (ClayToolOptionsEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClayToolOptionsEditor, clay_tool_options_editor,
                         CLAY_TYPE_EDITOR,
                         G_ADD_PRIVATE (ClayToolOptionsEditor)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_tool_options_editor_docked_iface_init))

#define parent_class clay_tool_options_editor_parent_class


static void
clay_tool_options_editor_class_init (ClayToolOptionsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_tool_options_editor_constructed;
  object_class->dispose      = clay_tool_options_editor_dispose;
  object_class->set_property = clay_tool_options_editor_set_property;
  object_class->get_property = clay_tool_options_editor_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_tool_options_editor_docked_iface_init (ClayDockedInterface *docked_iface)
{
  docked_iface->get_preview     = clay_tool_options_editor_get_preview;
  docked_iface->get_title       = clay_tool_options_editor_get_title;
  docked_iface->get_prefer_icon = clay_tool_options_editor_get_prefer_icon;
}

static void
clay_tool_options_editor_init (ClayToolOptionsEditor *editor)
{
  GtkScrolledWindow *scrolled_window;

  editor->p = clay_tool_options_editor_get_instance_private (editor);

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  clay_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              CLAY_TYPE_TOOL_INFO,
                              clay_tool_options_editor_drop_tool,
                              editor);

  /*  The label containing the tool options title */
  editor->p->title_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (editor->p->title_label), 0.0);
  clay_label_set_attributes (GTK_LABEL (editor->p->title_label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (editor), editor->p->title_label,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->p->title_label, TRUE);

  editor->p->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  scrolled_window = GTK_SCROLLED_WINDOW (editor->p->scrolled_window);

  gtk_scrolled_window_set_policy (scrolled_window,
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_overlay_scrolling (scrolled_window, FALSE);

  gtk_box_pack_start (GTK_BOX (editor), editor->p->scrolled_window,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (editor->p->scrolled_window, TRUE);

  /*  The vbox containing the tool options  */
  editor->p->options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->p->options_vbox);
  gtk_widget_set_visible (editor->p->options_vbox, TRUE);
}

static void
clay_tool_options_editor_constructed (GObject *object)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (object);
  ClayContext           *user_context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->p->save_button =
    clay_editor_add_button (CLAY_EDITOR (editor),
                            CLAY_ICON_DOCUMENT_SAVE,
                            _("Save Tool Preset..."),
                            CLAY_HELP_TOOL_OPTIONS_SAVE,
                            G_CALLBACK (clay_tool_options_editor_save_clicked),
                            NULL,
                            G_OBJECT (editor));

  editor->p->restore_button =
    clay_editor_add_button (CLAY_EDITOR (editor),
                            CLAY_ICON_DOCUMENT_REVERT,
                            _("Restore Tool Preset..."),
                            CLAY_HELP_TOOL_OPTIONS_RESTORE,
                            G_CALLBACK (clay_tool_options_editor_restore_clicked),
                            NULL,
                            G_OBJECT (editor));

  editor->p->delete_button =
    clay_editor_add_button (CLAY_EDITOR (editor),
                            CLAY_ICON_EDIT_DELETE,
                            _("Delete Tool Preset..."),
                            CLAY_HELP_TOOL_OPTIONS_DELETE,
                            G_CALLBACK (clay_tool_options_editor_delete_clicked),
                            NULL,
                            G_OBJECT (editor));

  editor->p->reset_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "tool-options",
                                   "tool-options-reset",
                                   "tool-options-reset-all",
                                   GDK_SHIFT_MASK,
                                   NULL);

  user_context = clay_get_user_context (editor->p->clay);

  g_signal_connect_object (user_context, "tool-changed",
                           G_CALLBACK (clay_tool_options_editor_tool_changed),
                           editor,
                           0);

  clay_tool_options_editor_tool_changed (user_context,
                                         clay_context_get_tool (user_context),
                                         editor);
}

static void
clay_tool_options_editor_dispose (GObject *object)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (object);

  if (editor->p->options_vbox)
    {
      GList *options;
      GList *list;

      options =
        gtk_container_get_children (GTK_CONTAINER (editor->p->options_vbox));

      for (list = options; list; list = g_list_next (list))
        {
          gtk_container_remove (GTK_CONTAINER (editor->p->options_vbox),
                                GTK_WIDGET (list->data));
        }

      g_list_free (options);
      editor->p->options_vbox = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_tool_options_editor_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_CLAY:
      editor->p->clay = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_options_editor_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, editor->p->clay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_tool_options_editor_get_preview (ClayDocked   *docked,
                                      ClayContext  *context,
                                      GtkIconSize   size)
{
  GtkWidget *view;
  gint       width;
  gint       height;

  gtk_icon_size_lookup (size, &width, &height);
  view = clay_prop_view_new (G_OBJECT (context), "tool", context, height);
  CLAY_VIEW (view)->renderer->size = -1;
  clay_view_renderer_set_size_full (CLAY_VIEW (view)->renderer,
                                    width, height, 0);

  return view;
}

static gchar *
clay_tool_options_editor_get_title (ClayDocked *docked)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (docked);
  ClayContext           *context;
  ClayToolInfo          *tool_info;

  context = clay_get_user_context (editor->p->clay);

  tool_info = clay_context_get_tool (context);

  return tool_info ? g_strdup (tool_info->label) : NULL;
}

static gboolean
clay_tool_options_editor_get_prefer_icon (ClayDocked *docked)
{
  /* We support get_preview() for tab tyles, but we prefer to show our
   * icon
   */
  return TRUE;
}


/*  public functions  */

GtkWidget *
clay_tool_options_editor_new (Clay            *clay,
                              ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_TOOL_OPTIONS_EDITOR,
                       "clay",            clay,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolOptions>",
                       "ui-path",         "/tool-options-popup",
                       NULL);
}

ClayToolOptions *
clay_tool_options_editor_get_tool_options (ClayToolOptionsEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_TOOL_OPTIONS_EDITOR (editor), NULL);

  return editor->p->visible_tool_options;
}

/*  private functions  */

static void
clay_tool_options_editor_menu_popup (ClayToolOptionsEditor *editor,
                                     GtkWidget             *button,
                                     const gchar           *path)
{
  ClayEditor *clay_editor = CLAY_EDITOR (editor);

  clay_ui_manager_update (clay_editor_get_ui_manager (clay_editor),
                          clay_editor_get_popup_data (clay_editor));

  clay_ui_manager_ui_popup_at_widget (clay_editor_get_ui_manager (clay_editor),
                                      path, NULL, NULL,
                                      button,
                                      GDK_GRAVITY_WEST,
                                      GDK_GRAVITY_NORTH_EAST,
                                      NULL,
                                      NULL, NULL);
}

static void
clay_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       ClayToolOptionsEditor *editor)
{
  if (gtk_widget_get_sensitive (editor->p->restore_button) /* evil but correct */)
    {
      clay_tool_options_editor_menu_popup (editor, widget,
                                           "/tool-options-popup/Tool Options Menu/Save Tool Preset");
    }
  else
    {
      clay_ui_manager_activate_action (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                                       "tool-options",
                                       "tool-options-save-new-preset");
    }
}

static void
clay_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          ClayToolOptionsEditor *editor)
{
  clay_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Tool Options Menu/Restore Tool Preset");
}

static void
clay_tool_options_editor_delete_clicked (GtkWidget             *widget,
                                         ClayToolOptionsEditor *editor)
{
  clay_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Tool Options Menu/Delete Tool Preset");
}

static void
clay_tool_options_editor_drop_tool (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    ClayViewable *viewable,
                                    gpointer      data)
{
  ClayToolOptionsEditor *editor = CLAY_TOOL_OPTIONS_EDITOR (data);
  ClayContext           *context;

  context = clay_get_user_context (editor->p->clay);

  clay_context_set_tool (context, CLAY_TOOL_INFO (viewable));
}

static void
clay_tool_options_editor_tool_changed (ClayContext           *context,
                                       ClayToolInfo          *tool_info,
                                       ClayToolOptionsEditor *editor)
{
  ClayContainer *presets;
  GtkWidget     *options_gui;

  if (tool_info && tool_info->tool_options == editor->p->visible_tool_options)
    return;

  if (editor->p->visible_tool_options)
    {
      presets = editor->p->visible_tool_options->tool_info->presets;

      if (presets)
        g_signal_handlers_disconnect_by_func (presets,
                                              clay_tool_options_editor_presets_update,
                                              editor);

      options_gui = clay_tools_get_tool_options_gui (editor->p->visible_tool_options);

      if (options_gui)
        gtk_widget_set_visible (options_gui, FALSE);

      editor->p->visible_tool_options = NULL;
    }

  if (tool_info && tool_info->tool_options)
    {
      presets = tool_info->presets;

      if (presets)
        {
          g_signal_connect_object (presets, "add",
                                   G_CALLBACK (clay_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "remove",
                                   G_CALLBACK (clay_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "thaw",
                                   G_CALLBACK (clay_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
        }

      options_gui = clay_tools_get_tool_options_gui (tool_info->tool_options);

      if (! gtk_widget_get_parent (options_gui))
        gtk_box_pack_start (GTK_BOX (editor->p->options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_set_visible (options_gui, TRUE);

      editor->p->visible_tool_options = tool_info->tool_options;

      clay_help_set_help_data (editor->p->scrolled_window, NULL,
                               tool_info->help_id);

      clay_tool_options_editor_presets_update (editor);
    }

  if (editor->p->title_label != NULL)
    {
      gchar *title;

      title = clay_docked_get_title (CLAY_DOCKED (editor));
      gtk_label_set_text (GTK_LABEL (editor->p->title_label), title);
      g_free (title);
    }

  clay_docked_title_changed (CLAY_DOCKED (editor));
}

static void
clay_tool_options_editor_presets_update (ClayToolOptionsEditor *editor)
{
  ClayToolInfo *tool_info         = editor->p->visible_tool_options->tool_info;
  gboolean      save_sensitive    = FALSE;
  gboolean      restore_sensitive = FALSE;
  gboolean      delete_sensitive  = FALSE;
  gboolean      reset_sensitive   = FALSE;

  if (tool_info->presets)
    {
      save_sensitive  = TRUE;
      reset_sensitive = TRUE;

      if (! clay_container_is_empty (tool_info->presets))
        {
          restore_sensitive = TRUE;
          delete_sensitive  = TRUE;
        }
    }

  gtk_widget_set_sensitive (editor->p->save_button,    save_sensitive);
  gtk_widget_set_sensitive (editor->p->restore_button, restore_sensitive);
  gtk_widget_set_sensitive (editor->p->delete_button,  delete_sensitive);
  gtk_widget_set_sensitive (editor->p->reset_button,   reset_sensitive);
}
