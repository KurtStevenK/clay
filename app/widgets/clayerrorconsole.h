/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayerrorconsole.h
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

#pragma once

#include "clayeditor.h"


#define CLAY_TYPE_ERROR_CONSOLE            (clay_error_console_get_type ())
#define CLAY_ERROR_CONSOLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ERROR_CONSOLE, ClayErrorConsole))
#define CLAY_ERROR_CONSOLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ERROR_CONSOLE, ClayErrorConsoleClass))
#define CLAY_IS_ERROR_CONSOLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ERROR_CONSOLE))
#define CLAY_IS_ERROR_CONSOLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ERROR_CONSOLE))
#define CLAY_ERROR_CONSOLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ERROR_CONSOLE, ClayErrorConsoleClass))


typedef struct _ClayErrorConsoleClass ClayErrorConsoleClass;

struct _ClayErrorConsole
{
  ClayEditor            parent_instance;

  Clay                 *clay;

  GtkTextBuffer        *text_buffer;
  GtkWidget            *text_view;

  GtkWidget            *clear_button;
  GtkWidget            *save_button;

  GtkFileChooserNative *file_dialog;
  gboolean              save_selection;

  gboolean              highlight[CLAY_MESSAGE_ERROR + 1];
};

struct _ClayErrorConsoleClass
{
  ClayEditorClass  parent_class;
};


GType       clay_error_console_get_type (void);

GtkWidget * clay_error_console_new      (Clay                *clay,
                                         ClayMenuFactory     *menu_factory);

void        clay_error_console_add      (ClayErrorConsole    *console,
                                         ClayMessageSeverity  severity,
                                         const gchar         *domain,
                                         const gchar         *message);
