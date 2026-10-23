/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydocumentview.h
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

#pragma once

#include "claycontainereditor.h"


#define CLAY_TYPE_DOCUMENT_VIEW            (clay_document_view_get_type ())
#define CLAY_DOCUMENT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCUMENT_VIEW, ClayDocumentView))
#define CLAY_DOCUMENT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCUMENT_VIEW, ClayDocumentViewClass))
#define CLAY_IS_DOCUMENT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCUMENT_VIEW))
#define CLAY_IS_DOCUMENT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCUMENT_VIEW))
#define CLAY_DOCUMENT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCUMENT_VIEW, ClayDocumentViewClass))


typedef struct _ClayDocumentViewClass  ClayDocumentViewClass;

struct _ClayDocumentView
{
  ClayContainerEditor  parent_instance;

  GtkWidget           *open_button;
  GtkWidget           *remove_button;
  GtkWidget           *refresh_button;
};

struct _ClayDocumentViewClass
{
  ClayContainerEditorClass  parent_class;
};


GType       clay_document_view_get_type (void);

GtkWidget * clay_document_view_new      (ClayViewType     view_type,
                                         ClayContainer   *container,
                                         ClayContext     *context,
                                         gint             view_size,
                                         gint             view_border_width,
                                         ClayMenuFactory *menu_factory);
