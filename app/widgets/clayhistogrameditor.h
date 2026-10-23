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

#include "clayimageeditor.h"


#define CLAY_TYPE_HISTOGRAM_EDITOR            (clay_histogram_editor_get_type ())
#define CLAY_HISTOGRAM_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HISTOGRAM_EDITOR, ClayHistogramEditor))
#define CLAY_HISTOGRAM_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_HISTOGRAM_EDITOR, ClayHistogramEditorClass))
#define CLAY_IS_HISTOGRAM_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HISTOGRAM_EDITOR))
#define CLAY_IS_HISTOGRAM_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_HISTOGRAM_EDITOR))
#define CLAY_HISTOGRAM_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_HISTOGRAM_EDITOR, ClayHistogramEditorClass))


typedef struct _ClayHistogramEditorClass ClayHistogramEditorClass;

struct _ClayHistogramEditor
{
  ClayImageEditor       parent_instance;

  ClayTRCType           trc;

  ClayDrawable         *drawable;
  ClayHistogram        *histogram;
  ClayHistogram        *bg_histogram;

  guint                 idle_id;
  gboolean              recompute;

  ClayAsync            *calculate_async;
  gboolean              bg_pending;
  gboolean              update_pending;

  GtkWidget            *menu;
  GtkWidget            *box;
  GtkWidget            *labels[7];
  GtkWidget            *toggle;
};

struct _ClayHistogramEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_histogram_editor_get_type (void);

GtkWidget * clay_histogram_editor_new      (void);
