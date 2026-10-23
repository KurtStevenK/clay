/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypaletteview.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claypalette.h"
#include "core/claymarshal.h"

#include "claydnd.h"
#include "claypaletteview.h"
#include "clayviewrendererpalette.h"


enum
{
  ENTRY_CLICKED,
  ENTRY_SELECTED,
  ENTRY_ACTIVATED,
  COLOR_DROPPED,
  LAST_SIGNAL
};


static gboolean clay_palette_view_draw           (GtkWidget        *widget,
                                                  cairo_t          *cr);
static gboolean clay_palette_view_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean clay_palette_view_key_press      (GtkWidget        *widget,
                                                  GdkEventKey      *kevent);
static gboolean clay_palette_view_focus          (GtkWidget        *widget,
                                                  GtkDirectionType  direction);
static void     clay_palette_view_set_viewable   (ClayView         *view,
                                                  ClayViewable     *old_viewable,
                                                  ClayViewable     *new_viewable);
static ClayPaletteEntry *
                clay_palette_view_find_entry     (ClayPaletteView *view,
                                                  gint             x,
                                                  gint             y);
static void     clay_palette_view_expose_entry   (ClayPaletteView  *view,
                                                  ClayPaletteEntry *entry);
static void     clay_palette_view_invalidate     (ClayPalette      *palette,
                                                  ClayPaletteView  *view);
static void     clay_palette_view_drag_color     (GtkWidget        *widget,
                                                  GeglColor       **color,
                                                  gpointer          data);
static void     clay_palette_view_drop_color     (GtkWidget        *widget,
                                                  gint              x,
                                                  gint              y,
                                                  GeglColor        *color,
                                                  gpointer          data);


G_DEFINE_TYPE (ClayPaletteView, clay_palette_view, CLAY_TYPE_VIEW)

#define parent_class clay_palette_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
clay_palette_view_class_init (ClayPaletteViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  ClayViewClass  *view_class   = CLAY_VIEW_CLASS (klass);

  view_signals[ENTRY_CLICKED] =
    g_signal_new ("entry-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPaletteViewClass, entry_clicked),
                  NULL, NULL,
                  clay_marshal_VOID__POINTER_ENUM,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  GDK_TYPE_MODIFIER_TYPE);

  view_signals[ENTRY_SELECTED] =
    g_signal_new ("entry-selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPaletteViewClass, entry_selected),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPaletteViewClass, entry_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[COLOR_DROPPED] =
    g_signal_new ("color-dropped",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPaletteViewClass, color_dropped),
                  NULL, NULL,
                  clay_marshal_VOID__POINTER_BOXED,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  GEGL_TYPE_COLOR);

  widget_class->draw               = clay_palette_view_draw;
  widget_class->button_press_event = clay_palette_view_button_press;
  widget_class->key_press_event    = clay_palette_view_key_press;
  widget_class->focus              = clay_palette_view_focus;

  view_class->set_viewable         = clay_palette_view_set_viewable;
}

static void
clay_palette_view_init (ClayPaletteView *view)
{
  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);

  view->selected  = NULL;
  view->dnd_entry = NULL;
}

static gboolean
clay_palette_view_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  ClayPaletteView *pal_view = CLAY_PALETTE_VIEW (widget);
  ClayView        *view     = CLAY_VIEW (widget);
  ClayPalette     *palette;

  palette = CLAY_PALETTE (CLAY_VIEW (view)->renderer->viewable);

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  if (view->renderer->viewable && pal_view->selected)
    {
      ClayViewRendererPalette *renderer;
      GtkAllocation            allocation;
      gint                     pos, row, col;

      renderer = CLAY_VIEW_RENDERER_PALETTE (view->renderer);

      gtk_widget_get_allocation (widget, &allocation);

      pos = clay_palette_get_entry_position (palette, pal_view->selected);

      row = pos / renderer->columns;
      col = pos % renderer->columns;

      cairo_rectangle (cr,
                       col * renderer->cell_width  + 0.5,
                       row * renderer->cell_height + 0.5,
                       renderer->cell_width,
                       renderer->cell_height);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
      cairo_stroke_preserve (cr);

      if (clay_cairo_set_focus_line_pattern (cr, widget))
        {
          cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
          cairo_stroke (cr);
        }
    }

  return FALSE;
}

static gboolean
clay_palette_view_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  ClayPaletteView  *view = CLAY_PALETTE_VIEW (widget);
  ClayPaletteEntry *entry;

  if (gtk_widget_get_can_focus (widget) && ! gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  entry = clay_palette_view_find_entry (view, bevent->x, bevent->y);

  view->dnd_entry = entry;

  if (! entry || bevent->button == 2)
    return TRUE;

  if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS)
    g_signal_emit (view, view_signals[ENTRY_CLICKED], 0,
                   entry, bevent->state);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      if (entry != view->selected)
        clay_palette_view_select_entry (view, entry);

      /* Usually the menu is provided by a ClayEditor.
       * Make sure it's also run by returning FALSE here */
      return FALSE;
    }
  else if (bevent->button == 1)
    {
      if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS)
        {
          clay_palette_view_select_entry (view, entry);
        }
      else if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_2BUTTON_PRESS && entry == view->selected)
        {
          g_signal_emit (view, view_signals[ENTRY_ACTIVATED], 0, entry);
        }
    }

  return TRUE;
}

static gboolean
clay_palette_view_key_press (GtkWidget   *widget,
                             GdkEventKey *kevent)
{
  ClayPaletteView *view = CLAY_PALETTE_VIEW (widget);

  if (view->selected &&
      (kevent->keyval == GDK_KEY_space    ||
       kevent->keyval == GDK_KEY_KP_Space ||
       kevent->keyval == GDK_KEY_Return   ||
       kevent->keyval == GDK_KEY_KP_Enter ||
       kevent->keyval == GDK_KEY_ISO_Enter))
    {
      g_signal_emit (view, view_signals[ENTRY_CLICKED], 0,
                     view->selected, kevent->state);
    }

  return FALSE;
}

static gboolean
clay_palette_view_focus (GtkWidget        *widget,
                         GtkDirectionType  direction)
{
  ClayPaletteView *view = CLAY_PALETTE_VIEW (widget);
  ClayPalette     *palette;

  palette = CLAY_PALETTE (CLAY_VIEW (view)->renderer->viewable);

  if (gtk_widget_get_can_focus (widget) &&
      ! gtk_widget_has_focus (widget))
    {
      gtk_widget_grab_focus (widget);

      if (! view->selected &&
          palette && clay_palette_get_n_colors (palette) > 0)
        {
          ClayPaletteEntry *entry = clay_palette_get_entry (palette, 0);

          clay_palette_view_select_entry (view, entry);
        }

      return TRUE;
    }

  if (view->selected)
    {
      ClayViewRendererPalette *renderer;
      gint                     skip = 0;

      renderer = CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (view)->renderer);

      switch (direction)
        {
        case GTK_DIR_UP:
          skip = -renderer->columns;
          break;
        case GTK_DIR_DOWN:
          skip = renderer->columns;
          break;
        case GTK_DIR_LEFT:
          skip = -1;
          break;
        case GTK_DIR_RIGHT:
          skip = 1;
          break;

        case GTK_DIR_TAB_FORWARD:
        case GTK_DIR_TAB_BACKWARD:
          return FALSE;
        }

      if (skip != 0)
        {
          ClayPaletteEntry *entry;
          ClayPalette      *palette;
          gint              position;

          palette = CLAY_PALETTE (CLAY_VIEW (view)->renderer->viewable);
          position = clay_palette_get_entry_position (palette, view->selected);
          position += skip;

          entry = clay_palette_get_entry (palette, position);

          if (entry)
            clay_palette_view_select_entry (view, entry);
        }

      return TRUE;
    }

  return FALSE;
}

static void
clay_palette_view_set_viewable (ClayView     *view,
                                ClayViewable *old_viewable,
                                ClayViewable *new_viewable)
{
  ClayPaletteView *pal_view = CLAY_PALETTE_VIEW (view);

  pal_view->dnd_entry = NULL;

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            clay_palette_view_invalidate,
                                            view);

      if (! new_viewable)
        {
          clay_dnd_color_source_remove (GTK_WIDGET (view));
          clay_dnd_color_dest_remove (GTK_WIDGET (view));
        }
    }

  CLAY_VIEW_CLASS (parent_class)->set_viewable (view,
                                                old_viewable, new_viewable);

  if (new_viewable)
    {
      g_signal_connect (new_viewable, "invalidate-preview",
                        G_CALLBACK (clay_palette_view_invalidate),
                        view);

      /*  unset the palette drag handler set by ClayView  */
      clay_dnd_viewable_source_remove (GTK_WIDGET (view), CLAY_TYPE_PALETTE);

      if (! old_viewable)
        {
          clay_dnd_color_source_add (GTK_WIDGET (view),
                                     clay_palette_view_drag_color,
                                     view);
          clay_dnd_color_dest_add (GTK_WIDGET (view),
                                   clay_palette_view_drop_color,
                                   view);
        }

      clay_palette_view_select_entry (pal_view, NULL);
    }
}


/*  public functions  */

void
clay_palette_view_select_entry (ClayPaletteView  *view,
                                ClayPaletteEntry *entry)
{
  g_return_if_fail (CLAY_IS_PALETTE_VIEW (view));

  if (entry == view->selected)
    return;

  if (view->selected)
    clay_palette_view_expose_entry (view, view->selected);

  view->selected = entry;

  if (view->selected)
    clay_palette_view_expose_entry (view, view->selected);

  g_signal_emit (view, view_signals[ENTRY_SELECTED], 0, view->selected);
}

ClayPaletteEntry *
clay_palette_view_get_selected_entry (ClayPaletteView *view)
{
  g_return_val_if_fail (CLAY_IS_PALETTE_VIEW (view), NULL);

  return view->selected;
}

void
clay_palette_view_get_entry_rect (ClayPaletteView  *view,
                                  ClayPaletteEntry *entry,
                                  GdkRectangle     *rect)
{
  ClayViewRendererPalette *renderer;
  ClayPalette             *palette;
  GtkAllocation            allocation;
  gint                     pos, row, col;

  g_return_if_fail (CLAY_IS_PALETTE_VIEW (view));
  g_return_if_fail (entry);
  g_return_if_fail (rect);

  gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);

  renderer = CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (view)->renderer);
  palette = CLAY_PALETTE (CLAY_VIEW_RENDERER (renderer)->viewable);
  pos = clay_palette_get_entry_position (palette, entry);
  row = pos / renderer->columns;
  col = pos % renderer->columns;

  rect->x = allocation.x + col * renderer->cell_width;
  rect->y = allocation.y + row * renderer->cell_height;
  rect->width = renderer->cell_width;
  rect->height = renderer->cell_height;
}


/*  private functions  */

static ClayPaletteEntry *
clay_palette_view_find_entry (ClayPaletteView *view,
                              gint             x,
                              gint             y)
{
  ClayPalette             *palette;
  ClayViewRendererPalette *renderer;
  ClayPaletteEntry        *entry = NULL;
  gint                     col, row;

  renderer = CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (view)->renderer);
  palette  = CLAY_PALETTE (CLAY_VIEW_RENDERER (renderer)->viewable);

  if (! palette || ! clay_palette_get_n_colors (palette))
    return NULL;

  col = x / renderer->cell_width;
  row = y / renderer->cell_height;

  if (col >= 0 && col < renderer->columns &&
      row >= 0 && row < renderer->rows)
    {
      entry = clay_palette_get_entry (palette,
                                      row * renderer->columns + col);
    }

  return entry;
}

static void
clay_palette_view_expose_entry (ClayPaletteView  *view,
                                ClayPaletteEntry *entry)
{
  ClayViewRendererPalette *renderer;
  gint                     pos, row, col;
  GtkWidget               *widget = GTK_WIDGET (view);
  GtkAllocation            allocation;
  ClayPalette             *palette;

  renderer = CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (view)->renderer);
  palette = CLAY_PALETTE (CLAY_VIEW_RENDERER (renderer)->viewable);

  g_return_if_fail (CLAY_IS_PALETTE (palette));

  gtk_widget_get_allocation (widget, &allocation);

  pos = clay_palette_get_entry_position (palette, entry);
  row = pos / renderer->columns;
  col = pos % renderer->columns;

  gtk_widget_queue_draw_area (GTK_WIDGET (view),
                              allocation.x + col * renderer->cell_width,
                              allocation.y + row * renderer->cell_height,
                              renderer->cell_width  + 1,
                              renderer->cell_height + 1);
}

static void
clay_palette_view_invalidate (ClayPalette     *palette,
                              ClayPaletteView *view)
{
  view->dnd_entry = NULL;

  if (view->selected &&
      ! g_list_find (clay_palette_get_colors (palette), view->selected))
    {
      clay_palette_view_select_entry (view, NULL);
    }
}

static void
clay_palette_view_drag_color (GtkWidget  *widget,
                              GeglColor **color,
                              gpointer    data)
{
  ClayPaletteView *view = CLAY_PALETTE_VIEW (data);

  if (view->dnd_entry)
    *color = gegl_color_duplicate (view->dnd_entry->color);
  else
    *color = gegl_color_new ("black");
}

static void
clay_palette_view_drop_color (GtkWidget *widget,
                              gint       x,
                              gint       y,
                              GeglColor *color,
                              gpointer   data)
{
  ClayPaletteView  *view = CLAY_PALETTE_VIEW (data);
  ClayPaletteEntry *entry;

  entry = clay_palette_view_find_entry (view, x, y);

  g_signal_emit (view, view_signals[COLOR_DROPPED], 0,
                 entry, color);
}
