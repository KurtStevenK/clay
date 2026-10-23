/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooldialog.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "core/clayobject.h"
#include "core/claytoolinfo.h"

#include "widgets/claydialogfactory.h"

#include "claydisplayshell.h"
#include "claytooldialog.h"


typedef struct _ClayToolDialogPrivate ClayToolDialogPrivate;

struct _ClayToolDialogPrivate
{
  ClayDisplayShell *shell;
};

#define GET_PRIVATE(dialog) ((ClayToolDialogPrivate *) clay_tool_dialog_get_instance_private ((ClayToolDialog *) (dialog)))


static void   clay_tool_dialog_dispose     (GObject          *object);

static void   clay_tool_dialog_shell_unmap (ClayDisplayShell *shell,
                                            ClayToolDialog   *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolDialog, clay_tool_dialog,
                            CLAY_TYPE_VIEWABLE_DIALOG)


static void
clay_tool_dialog_class_init (ClayToolDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_tool_dialog_dispose;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ClayToolDialog");
}

static void
clay_tool_dialog_init (ClayToolDialog *dialog)
{
}

static void
clay_tool_dialog_dispose (GObject *object)
{
  ClayToolDialogPrivate *private = GET_PRIVATE (object);

  g_clear_weak_pointer (&private->shell);

  G_OBJECT_CLASS (clay_tool_dialog_parent_class)->dispose (object);
}


/**
 * clay_tool_dialog_new: (skip)
 * @tool_info: a #ClayToolInfo
 * @desc:      a string to use in the dialog header or %NULL to use the help
 *             field from #ClayToolInfo
 * @...:       a %NULL-terminated valist of button parameters as described in
 *             gtk_dialog_new_with_buttons().
 *
 * This function conveniently creates a #ClayViewableDialog using the
 * information stored in @tool_info. It also registers the tool with
 * the "toplevel" dialog factory.
 *
 * Returns: a new #ClayViewableDialog
 **/
GtkWidget *
clay_tool_dialog_new (ClayToolInfo *tool_info,
                      GdkMonitor   *monitor,
                      const gchar  *title,
                      const gchar  *description,
                      const gchar  *icon_name,
                      const gchar  *help_id,
                      ...)
{
  GtkWidget *dialog;
  gchar     *identifier;
  va_list    args;
  gboolean   use_header_bar;

  g_return_val_if_fail (CLAY_IS_TOOL_INFO (tool_info), NULL);

  if (! title)
    title = tool_info->label;

  if (! description)
    description = tool_info->tooltip;

  if (! help_id)
    help_id = tool_info->help_id;

  if (! icon_name)
    icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (CLAY_TYPE_TOOL_DIALOG,
                         "title",          title,
                         "role",           clay_object_get_name (tool_info),
                         "description",    description,
                         "icon-name",      icon_name,
                         "help-func",      clay_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                          NULL);

  va_start (args, help_id);
  clay_dialog_add_buttons_valist (CLAY_DIALOG (dialog), args);
  va_end (args);

  identifier = g_strconcat (clay_object_get_name (tool_info), "-dialog", NULL);

  clay_dialog_factory_add_foreign (clay_dialog_factory_get_singleton (),
                                   identifier,
                                   dialog,
                                   monitor);

  g_free (identifier);

  return dialog;
}

void
clay_tool_dialog_set_shell (ClayToolDialog   *tool_dialog,
                            ClayDisplayShell *shell)
{
  ClayToolDialogPrivate *private = GET_PRIVATE (tool_dialog);

  g_return_if_fail (CLAY_IS_TOOL_DIALOG (tool_dialog));
  g_return_if_fail (shell == NULL || CLAY_IS_DISPLAY_SHELL (shell));

  if (shell == private->shell)
    return;

  if (private->shell)
    {
      g_signal_handlers_disconnect_by_func (private->shell,
                                            clay_tool_dialog_shell_unmap,
                                            tool_dialog);

      gtk_window_set_transient_for (GTK_WINDOW (tool_dialog), NULL);
    }

  g_set_weak_pointer (&private->shell, shell);

  if (private->shell)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

      gtk_window_set_transient_for (GTK_WINDOW (tool_dialog),
                                    GTK_WINDOW (toplevel));

      g_signal_connect_object (private->shell, "unmap",
                               G_CALLBACK (clay_tool_dialog_shell_unmap),
                               tool_dialog, 0);
    }
}


/*  private functions  */

static void
clay_tool_dialog_shell_unmap (ClayDisplayShell *shell,
                              ClayToolDialog   *dialog)
{
  /*  the dialog being mapped while the shell is being unmapped
   *  happens when switching images in ClayImageWindow
   */
  if (gtk_widget_get_mapped (GTK_WIDGET (dialog)))
    g_signal_emit_by_name (dialog, "close");
}
