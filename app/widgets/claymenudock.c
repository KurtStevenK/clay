/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenudock.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylist.h"

#include "claydialogfactory.h"
#include "claydockable.h"
#include "claydockbook.h"
#include "claymenudock.h"

#include "clay-intl.h"


#define DEFAULT_MINIMAL_WIDTH  200


struct _ClayMenuDockPrivate
{
  gint make_sizeof_greater_than_zero;
};


static void   clay_menu_dock_style_updated (GtkWidget *widget);


G_DEFINE_TYPE_WITH_PRIVATE (ClayMenuDock, clay_menu_dock, CLAY_TYPE_DOCK)

#define parent_class clay_menu_dock_parent_class


static void
clay_menu_dock_class_init (ClayMenuDockClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->style_updated = clay_menu_dock_style_updated;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal-width",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_WIDTH,
                                                             CLAY_PARAM_READABLE));
}

static void
clay_menu_dock_init (ClayMenuDock *dock)
{
}

static void
clay_menu_dock_style_updated (GtkWidget *widget)
{
  gint minimal_width = -1;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "minimal-width", &minimal_width,
                        NULL);

  gtk_widget_set_size_request (widget, minimal_width, -1);
}

GtkWidget *
clay_menu_dock_new (void)
{
  return g_object_new (CLAY_TYPE_MENU_DOCK, NULL);
}
