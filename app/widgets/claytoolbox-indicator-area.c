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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claycontext.h"
#include "core/claygradient.h"
#include "core/claypattern.h"

#include "claydnd.h"
#include "clayhelp-ids.h"
#include "claytoolbox.h"
#include "claytoolbox-indicator-area.h"
#include "clayview.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"


#define CELL_SPACING 2  /*  How much between brush and pattern cells  */


static void
brush_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       ClayToolbox     *toolbox)
{
  ClayContext *context = clay_toolbox_get_context (toolbox);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (context->clay)),
                                             context->clay,
                                             clay_dock_get_dialog_factory (CLAY_DOCK (toolbox)),
                                             clay_widget_get_monitor (widget),
                                             "clay-brush-grid|clay-brush-list");
}

static void
brush_preview_drop_brush (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          ClayViewable *viewable,
                          gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);

  clay_context_set_brush (context, CLAY_BRUSH (viewable));
}

static void
pattern_preview_clicked (GtkWidget       *widget,
                         GdkModifierType  state,
                         ClayToolbox     *toolbox)
{
  ClayContext *context = clay_toolbox_get_context (toolbox);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (context->clay)),
                                             context->clay,
                                             clay_dock_get_dialog_factory (CLAY_DOCK (toolbox)),
                                             clay_widget_get_monitor (widget),
                                             "clay-pattern-grid|clay-pattern-list");
}

static void
pattern_preview_drop_pattern (GtkWidget    *widget,
                              gint          x,
                              gint          y,
                              ClayViewable *viewable,
                              gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);

  clay_context_set_pattern (context, CLAY_PATTERN (viewable));
}

static void
gradient_preview_clicked (GtkWidget       *widget,
                          GdkModifierType  state,
                          ClayToolbox     *toolbox)
{
  ClayContext *context = clay_toolbox_get_context (toolbox);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (context->clay)),
                                             context->clay,
                                             clay_dock_get_dialog_factory (CLAY_DOCK (toolbox)),
                                             clay_widget_get_monitor (widget),
                                             "clay-gradient-list|clay-gradient-grid");
}

static void
gradient_preview_drop_gradient (GtkWidget    *widget,
                                gint          x,
                                gint          y,
                                ClayViewable *viewable,
                                gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);

  clay_context_set_gradient (context, CLAY_GRADIENT (viewable));
}


/*  public functions  */

GtkWidget *
clay_toolbox_indicator_area_create (ClayToolbox *toolbox)
{
  ClayContext *context;
  GtkWidget   *grid;
  GtkWidget   *brush_view;
  GtkWidget   *pattern_view;
  GtkWidget   *gradient_view;
  GtkIconSize  tool_icon_size;
  gint         pixel_size;
  gint         gradient_width;

  g_return_val_if_fail (CLAY_IS_TOOLBOX (toolbox), NULL);

  context = clay_toolbox_get_context (toolbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), CELL_SPACING);
  gtk_grid_set_column_spacing (GTK_GRID (grid), CELL_SPACING);

  clay_help_set_help_data (grid, NULL, CLAY_HELP_TOOLBOX_INDICATOR_AREA);

  gtk_widget_style_get (GTK_WIDGET (toolbox),
                        "tool-icon-size", &tool_icon_size,
                        NULL);
  gtk_icon_size_lookup (tool_icon_size, &pixel_size, NULL);
  gradient_width = pixel_size * (13/6.0f);

  /*  brush view  */

  brush_view =
    clay_view_new_full_by_types (context,
                                 CLAY_TYPE_VIEW, CLAY_TYPE_BRUSH,
                                 pixel_size, pixel_size, 1,
                                 FALSE, TRUE, TRUE);
  clay_view_set_viewable (CLAY_VIEW (brush_view),
                          CLAY_VIEWABLE (clay_context_get_brush (context)));
  gtk_grid_attach (GTK_GRID (grid), brush_view, 0, 0, 1, 1);
  gtk_widget_set_visible (brush_view, TRUE);

  clay_help_set_help_data (brush_view,
                           _("The active brush.\n"
                             "Click to open the Brush Dialog."), NULL);

  g_signal_connect_object (context, "brush-changed",
                           G_CALLBACK (clay_view_set_viewable),
                           brush_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (brush_view, "clicked",
                    G_CALLBACK (brush_preview_clicked),
                    toolbox);

  clay_dnd_viewable_dest_add (brush_view,
                              CLAY_TYPE_BRUSH,
                              brush_preview_drop_brush,
                              context);

  /*  pattern view  */

  pattern_view =
    clay_view_new_full_by_types (context,
                                 CLAY_TYPE_VIEW, CLAY_TYPE_PATTERN,
                                 pixel_size, pixel_size, 1,
                                 FALSE, TRUE, TRUE);
  clay_view_set_viewable (CLAY_VIEW (pattern_view),
                          CLAY_VIEWABLE (clay_context_get_pattern (context)));

  gtk_grid_attach (GTK_GRID (grid), pattern_view, 1, 0, 1, 1);
  gtk_widget_set_visible (pattern_view, TRUE);

  clay_help_set_help_data (pattern_view,
                           _("The active pattern.\n"
                             "Click to open the Pattern Dialog."), NULL);

  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (clay_view_set_viewable),
                           pattern_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (pattern_view, "clicked",
                    G_CALLBACK (pattern_preview_clicked),
                    toolbox);

  clay_dnd_viewable_dest_add (pattern_view,
                              CLAY_TYPE_PATTERN,
                              pattern_preview_drop_pattern,
                              context);

  /*  gradient view  */

  gradient_view =
    clay_view_new_full_by_types (context,
                                 CLAY_TYPE_VIEW, CLAY_TYPE_GRADIENT,
                                 gradient_width, (pixel_size / 2), 1,
                                 FALSE, TRUE, TRUE);
  clay_view_set_viewable (CLAY_VIEW (gradient_view),
                          CLAY_VIEWABLE (clay_context_get_gradient (context)));

  gtk_grid_attach (GTK_GRID (grid), gradient_view, 0, 1, 2, 1);
  gtk_widget_set_visible (gradient_view, TRUE);

  clay_help_set_help_data (gradient_view,
                           _("The active gradient.\n"
                             "Click to open the Gradient Dialog."), NULL);

  g_signal_connect_object (context, "gradient-changed",
                           G_CALLBACK (clay_view_set_viewable),
                           gradient_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (gradient_view, "clicked",
                    G_CALLBACK (gradient_preview_clicked),
                    toolbox);

  clay_dnd_viewable_dest_add (gradient_view,
                              CLAY_TYPE_GRADIENT,
                              gradient_preview_drop_gradient,
                              context);

  gtk_widget_set_visible (grid, TRUE);

  return grid;
}
