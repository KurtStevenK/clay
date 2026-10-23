/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselectorpalette.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claycontext.h"
#include "core/claypalette.h"

#include "claycolorselectorpalette.h"
#include "clayhelp-ids.h"
#include "claypaletteview.h"
#include "clayviewrendererpalette.h"

#include "clay-intl.h"


static void   clay_color_selector_palette_set_color  (ClayColorSelector *selector,
                                                      GeglColor         *color);
static void   clay_color_selector_palette_set_config (ClayColorSelector *selector,
                                                      ClayColorConfig   *config);


G_DEFINE_TYPE (ClayColorSelectorPalette, clay_color_selector_palette,
               CLAY_TYPE_COLOR_SELECTOR)

#define parent_class clay_color_selector_palette_parent_class


static void
clay_color_selector_palette_class_init (ClayColorSelectorPaletteClass *klass)
{
  ClayColorSelectorClass *selector_class = CLAY_COLOR_SELECTOR_CLASS (klass);

  selector_class->name       = _("Palette");
  selector_class->help_id    = CLAY_HELP_COLORSELECTOR_PALETTE;
  selector_class->icon_name  = CLAY_ICON_PALETTE;
  selector_class->set_color  = clay_color_selector_palette_set_color;
  selector_class->set_config = clay_color_selector_palette_set_config;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass),
                                 "ClayColorSelectorPalette");
}

static void
clay_color_selector_palette_init (ClayColorSelectorPalette *select)
{
}

static void
clay_color_selector_palette_set_color (ClayColorSelector *selector,
                                       GeglColor         *color)
{
  ClayColorSelectorPalette *select = CLAY_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      ClayPalette *palette = clay_context_get_palette (select->context);

      if (palette && clay_palette_get_n_colors (palette) > 0)
        {
          ClayPaletteEntry *entry;

          entry = clay_palette_find_entry (palette, color,
                                           CLAY_PALETTE_VIEW (select->view)->selected);

          if (entry)
            clay_palette_view_select_entry (CLAY_PALETTE_VIEW (select->view),
                                            entry);
        }
    }
}

static void
clay_color_selector_palette_palette_changed (ClayContext              *context,
                                             ClayPalette              *palette,
                                             ClayColorSelectorPalette *select)
{
  clay_view_set_viewable (CLAY_VIEW (select->view), CLAY_VIEWABLE (palette));

  if (palette != NULL)
    {
      gchar *palette_name;

      g_object_get (palette, "name", &palette_name, NULL);
      gtk_label_set_text (GTK_LABEL (select->name_label), palette_name);
      g_free (palette_name);
    }
}

static void
clay_color_selector_palette_entry_clicked (ClayPaletteView   *view,
                                           ClayPaletteEntry  *entry,
                                           GdkModifierType    state,
                                           ClayColorSelector *selector)
{
  clay_color_selector_set_color (selector, entry->color);
}

static void
clay_color_selector_palette_set_config (ClayColorSelector *selector,
                                        ClayColorConfig   *config)
{
  ClayColorSelectorPalette *select = CLAY_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      g_signal_handlers_disconnect_by_func (select->context,
                                            clay_color_selector_palette_palette_changed,
                                            select);
      clay_view_renderer_set_context (CLAY_VIEW (select->view)->renderer,
                                      NULL);

      g_clear_object (&select->context);
    }

  if (config)
    select->context = g_object_get_data (G_OBJECT (config), "clay-context");

  if (select->context)
    {
      g_object_ref (select->context);

      if (! select->view)
        {
          select->view = clay_view_new_full_by_types (select->context,
                                                      CLAY_TYPE_PALETTE_VIEW,
                                                      CLAY_TYPE_PALETTE,
                                                      100, 100, 0,
                                                      FALSE, TRUE, FALSE);
          clay_view_set_expand (CLAY_VIEW (select->view), TRUE);
          clay_view_renderer_palette_set_cell_size
            (CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (select->view)->renderer),
             -1);
          clay_view_renderer_palette_set_draw_grid
            (CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (select->view)->renderer),
             TRUE);
          gtk_box_pack_start (GTK_BOX (select), select->view, TRUE, TRUE, 0);
          gtk_widget_set_visible (select->view, TRUE);

          g_signal_connect (select->view, "entry-clicked",
                            G_CALLBACK (clay_color_selector_palette_entry_clicked),
                            select);

          select->name_label = gtk_label_new (NULL);
          gtk_label_set_ellipsize (GTK_LABEL (select->name_label),
                                              PANGO_ELLIPSIZE_END);
          gtk_widget_set_halign (select->name_label, GTK_ALIGN_START);

          gtk_box_pack_start (GTK_BOX (select), select->name_label, FALSE,
                              FALSE, 6);
          gtk_widget_set_visible (select->name_label, TRUE);
        }
      else
        {
          clay_view_renderer_set_context (CLAY_VIEW (select->view)->renderer,
                                          select->context);
        }

      g_signal_connect_object (select->context, "palette-changed",
                               G_CALLBACK (clay_color_selector_palette_palette_changed),
                               select, 0);

      clay_color_selector_palette_palette_changed (select->context,
                                                   clay_context_get_palette (select->context),
                                                   select);
    }
}
