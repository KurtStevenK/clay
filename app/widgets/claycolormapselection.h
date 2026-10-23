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

#pragma once


#define CLAY_TYPE_COLORMAP_SELECTION            (clay_colormap_selection_get_type ())
#define CLAY_COLORMAP_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLORMAP_SELECTION, ClayColormapSelection))
#define CLAY_COLORMAP_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLORMAP_SELECTION, ClayColormapSelectionClass))
#define CLAY_IS_COLORMAP_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLORMAP_SELECTION))
#define CLAY_IS_COLORMAP_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLORMAP_SELECTION))
#define CLAY_COLORMAP_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLORMAP_SELECTION, ClayColormapSelectionClass))


typedef struct _ClayColormapSelectionClass ClayColormapSelectionClass;

struct _ClayColormapSelection
{
  GtkBox  parent_instance;

  ClayContext     *context;
  ClayImage       *active_image;
  ClayPalette     *active_palette;

  GtkWidget       *view;
  gint             col_index;

  PangoLayout     *layout;

  GtkAdjustment   *index_adjustment;
  GtkWidget       *index_spinbutton;
  GtkWidget       *color_entry;

  GtkWidget       *color_dialog;
  GtkWidget       *right_vbox;
};

struct _ClayColormapSelectionClass
{
  GtkBoxClass  parent_class;

  void (* color_clicked)   (ClayColormapSelection *selection,
                            ClayPaletteEntry      *entry,
                            GdkModifierType        state);
  void (* color_activated) (ClayColormapSelection *selection,
                            ClayPaletteEntry      *entry);
};


GType       clay_colormap_selection_get_type   (void);

GtkWidget * clay_colormap_selection_new        (ClayContext *context);

gint        clay_colormap_selection_get_index  (ClayColormapSelection  *selection,
                                                GeglColor              *search);
gboolean    clay_colormap_selection_set_index  (ClayColormapSelection  *selection,
                                                gint                    index,
                                                GeglColor              *color);

gint        clay_colormap_selection_max_index  (ClayColormapSelection *selection);

ClayPaletteEntry * clay_colormap_selection_get_selected_entry (ClayColormapSelection *selection);

void               clay_colormap_selection_get_entry_rect (ClayColormapSelection *selection,
                                                           ClayPaletteEntry      *entry,
                                                           GdkRectangle          *rect);


