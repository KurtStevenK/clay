/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushfactoryview.c
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

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claybrush.h"
#include "core/claybrushgenerated.h"
#include "core/claydatafactory.h"

#include "claybrushfactoryview.h"
#include "claycontainerview.h"
#include "clayeditor.h"
#include "claymenufactory.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"

enum
{
  SPACING_CHANGED,
  LAST_SIGNAL
};


static void   clay_brush_factory_view_dispose         (GObject              *object);

static void   clay_brush_factory_view_select_item     (ClayContainerEditor  *editor,
                                                       ClayViewable         *viewable);

static void   clay_brush_factory_view_spacing_changed (ClayBrush            *brush,
                                                       ClayBrushFactoryView *view);
static void   clay_brush_factory_view_spacing_update  (GtkAdjustment        *adjustment,
                                                       ClayBrushFactoryView *view);


G_DEFINE_TYPE (ClayBrushFactoryView, clay_brush_factory_view,
               CLAY_TYPE_DATA_FACTORY_VIEW)

#define parent_class clay_brush_factory_view_parent_class

static guint clay_brush_factory_view_signals[LAST_SIGNAL] = { 0 };


static void
clay_brush_factory_view_class_init (ClayBrushFactoryViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  object_class->dispose     = clay_brush_factory_view_dispose;

  editor_class->select_item = clay_brush_factory_view_select_item;

  /**
   * ClayBrushFactoryView::spacing-changed:
   * @view: the #ClayBrushFactoryView.
   *
   * Emitted when the spacing changed.
   */
  clay_brush_factory_view_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayBrushFactoryViewClass, spacing_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
clay_brush_factory_view_init (ClayBrushFactoryView *view)
{
  view->spacing_adjustment = gtk_adjustment_new (0.0, 1.0, 5000.0,
                                                 1.0, 10.0, 0.0);

  view->spacing_scale = clay_spin_scale_new (view->spacing_adjustment,
                                             _("Spacing"), 1);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (view->spacing_scale),
                                   1.0, 200.0);
  clay_help_set_help_data (view->spacing_scale,
                           _("Percentage of width of brush"),
                           NULL);

  g_signal_connect (view->spacing_adjustment, "value-changed",
                    G_CALLBACK (clay_brush_factory_view_spacing_update),
                    view);

  clay_data_factory_view_show_follow_theme_toggle (CLAY_DATA_FACTORY_VIEW (view), TRUE);
}

static void
clay_brush_factory_view_dispose (GObject *object)
{
  ClayBrushFactoryView *view   = CLAY_BRUSH_FACTORY_VIEW (object);
  ClayContainerEditor  *editor = CLAY_CONTAINER_EDITOR (object);

  if (view->spacing_changed_handler_id)
    {
      ClayDataFactory *factory;
      ClayContainer   *container;

      factory   = clay_data_factory_view_get_data_factory (CLAY_DATA_FACTORY_VIEW (editor));
      container = clay_data_factory_get_container (factory);

      clay_container_remove_handler (container,
                                     view->spacing_changed_handler_id);

      view->spacing_changed_handler_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
clay_brush_factory_view_new (ClayViewType     view_type,
                             ClayDataFactory *factory,
                             ClayContext     *context,
                             gboolean         change_brush_spacing,
                             gint             view_size,
                             gint             view_border_width,
                             ClayMenuFactory *menu_factory)
{
  ClayBrushFactoryView *factory_view;
  ClayContainerEditor  *editor;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = g_object_new (CLAY_TYPE_BRUSH_FACTORY_VIEW,
                               "view-type",         view_type,
                               "data-factory",      factory,
                               "context",           context,
                               "view-size",         view_size,
                               "view-border-width", view_border_width,
                               "menu-factory",      menu_factory,
                               "menu-identifier",   "<Brushes>",
                               "ui-path",           "/brushes-popup",
                               "action-group",      "brushes",
                               NULL);

  factory_view->change_brush_spacing = change_brush_spacing;

  editor = CLAY_CONTAINER_EDITOR (factory_view);

  clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                 "brushes", "brushes-open-as-image",
                                 NULL);

  gtk_box_pack_end (GTK_BOX (editor->view), factory_view->spacing_scale,
                    FALSE, FALSE, 0);
  gtk_widget_set_visible (factory_view->spacing_scale, change_brush_spacing);

  factory_view->spacing_changed_handler_id =
    clay_container_add_handler (clay_data_factory_get_container (factory), "spacing-changed",
                                G_CALLBACK (clay_brush_factory_view_spacing_changed),
                                factory_view);

  return GTK_WIDGET (factory_view);
}

static void
clay_brush_factory_view_select_item (ClayContainerEditor *editor,
                                     ClayViewable        *viewable)
{
  ClayBrushFactoryView *view = CLAY_BRUSH_FACTORY_VIEW (editor);
  ClayContainer        *container;
  gboolean              spacing_sensitive = FALSE;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      ClayBrush *brush = CLAY_BRUSH (viewable);

      spacing_sensitive = TRUE;

      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       clay_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                clay_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         clay_brush_factory_view_spacing_update,
                                         view);
    }

  gtk_widget_set_sensitive (view->spacing_scale, spacing_sensitive);
}

static void
clay_brush_factory_view_spacing_changed (ClayBrush            *brush,
                                         ClayBrushFactoryView *view)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (view);
  ClayContext         *context;

  context = clay_container_view_get_context (editor->view);

  if (brush == clay_context_get_brush (context))
    {
      g_signal_handlers_block_by_func (view->spacing_adjustment,
                                       clay_brush_factory_view_spacing_update,
                                       view);

      gtk_adjustment_set_value (view->spacing_adjustment,
                                clay_brush_get_spacing (brush));

      g_signal_handlers_unblock_by_func (view->spacing_adjustment,
                                         clay_brush_factory_view_spacing_update,
                                         view);
    }
}

static void
clay_brush_factory_view_spacing_update (GtkAdjustment        *adjustment,
                                        ClayBrushFactoryView *view)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (view);
  ClayContext         *context;
  ClayBrush           *brush;

  context = clay_container_view_get_context (editor->view);

  brush = clay_context_get_brush (context);

  if (brush && view->change_brush_spacing)
    {
      g_signal_handlers_block_by_func (brush,
                                       clay_brush_factory_view_spacing_changed,
                                       view);

      clay_brush_set_spacing (brush, gtk_adjustment_get_value (adjustment));

      g_signal_handlers_unblock_by_func (brush,
                                         clay_brush_factory_view_spacing_changed,
                                         view);
    }
  g_signal_emit (view, clay_brush_factory_view_signals[SPACING_CHANGED], 0);
}
