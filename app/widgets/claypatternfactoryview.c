/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypatternfactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/clayviewable.h"

#include "clayeditor.h"
#include "claymenufactory.h"
#include "claypatternfactoryview.h"
#include "clayviewrenderer.h"


G_DEFINE_TYPE (ClayPatternFactoryView, clay_pattern_factory_view,
               CLAY_TYPE_DATA_FACTORY_VIEW)


static void
clay_pattern_factory_view_class_init (ClayPatternFactoryViewClass *klass)
{
}

static void
clay_pattern_factory_view_init (ClayPatternFactoryView *view)
{
}

GtkWidget *
clay_pattern_factory_view_new (ClayViewType      view_type,
                               ClayDataFactory  *factory,
                               ClayContext      *context,
                               gint              view_size,
                               gint              view_border_width,
                               ClayMenuFactory  *menu_factory)
{
  ClayPatternFactoryView *factory_view;
  ClayContainerEditor    *editor;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (CLAY_TYPE_PATTERN_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<Patterns>",
                               "ui-path",           "/patterns-popup",
                               "action-group",      "patterns",
                               NULL);

  editor = CLAY_CONTAINER_EDITOR (factory_view);

  clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                 "patterns", "patterns-open-as-image",
                                 NULL);

  gtk_widget_set_visible (clay_data_factory_view_get_edit_button (CLAY_DATA_FACTORY_VIEW (factory_view)), FALSE);

  return GTK_WIDGET (factory_view);
}
