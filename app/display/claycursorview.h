/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycursorview.h
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
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

#include "widgets/clayeditor.h"


#define CLAY_TYPE_CURSOR_VIEW            (clay_cursor_view_get_type ())
#define CLAY_CURSOR_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CURSOR_VIEW, ClayCursorView))
#define CLAY_CURSOR_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CURSOR_VIEW, ClayCursorViewClass))
#define CLAY_IS_CURSOR_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CURSOR_VIEW))
#define CLAY_IS_CURSOR_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CURSOR_VIEW))
#define CLAY_CURSOR_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CURSOR_VIEW, ClayCursorViewClass))


typedef struct _ClayCursorViewClass   ClayCursorViewClass;
typedef struct _ClayCursorViewPrivate ClayCursorViewPrivate;

struct _ClayCursorView
{
  ClayEditor             parent_instance;

  ClayCursorViewPrivate *priv;
};

struct _ClayCursorViewClass
{
  ClayEditorClass  parent_class;
};


GType       clay_cursor_view_get_type          (void);

GtkWidget * clay_cursor_view_new               (Clay            *clay,
                                                ClayMenuFactory *menu_factory);

void        clay_cursor_view_set_sample_merged (ClayCursorView  *view,
                                                gboolean         sample_merged);
gboolean    clay_cursor_view_get_sample_merged (ClayCursorView  *view);

void        clay_cursor_view_update_cursor     (ClayCursorView  *view,
                                                ClayImage       *image,
                                                ClayUnit        *shell_unit,
                                                gboolean         shell_show_all,
                                                gdouble          x,
                                                gdouble          y);
void        clay_cursor_view_clear_cursor      (ClayCursorView  *view);
