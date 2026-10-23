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

#include "core/claytooloptions.h"


#define CLAY_TYPE_ALIGN_OPTIONS            (clay_align_options_get_type ())
#define CLAY_ALIGN_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ALIGN_OPTIONS, ClayAlignOptions))
#define CLAY_ALIGN_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ALIGN_OPTIONS, ClayAlignOptionsClass))
#define CLAY_IS_ALIGN_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ALIGN_OPTIONS))
#define CLAY_IS_ALIGN_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ALIGN_OPTIONS))
#define CLAY_ALIGN_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ALIGN_OPTIONS, ClayAlignOptionsClass))


typedef struct _ClayAlignOptions        ClayAlignOptions;
typedef struct _ClayAlignOptionsPrivate ClayAlignOptionsPrivate;
typedef struct _ClayAlignOptionsClass   ClayAlignOptionsClass;

struct _ClayAlignOptions
{
  ClayToolOptions          parent_instance;

  ClayAlignReferenceType   align_reference;

  ClayAlignOptionsPrivate *priv;
};

struct _ClayAlignOptionsClass
{
  ClayToolOptionsClass  parent_class;

  void (* align_button_clicked) (ClayAlignOptions  *options,
                                 ClayAlignmentType  align_type);
};


GType       clay_align_options_get_type          (void);

GtkWidget * clay_align_options_gui               (ClayToolOptions  *tool_options);

GList     * clay_align_options_get_objects       (ClayAlignOptions *options);
void        clay_align_options_get_pivot         (ClayAlignOptions *options,
                                                  gdouble          *x,
                                                  gdouble          *y);

void        clay_align_options_pick_reference    (ClayAlignOptions *options,
                                                  GObject          *object);
GObject   * clay_align_options_get_reference     (ClayAlignOptions *options,
                                                  gboolean          blink_if_none);
gboolean    clay_align_options_align_contents    (ClayAlignOptions *options);

void        clay_align_options_pick_guide        (ClayAlignOptions *options,
                                                  ClayGuide        *guide,
                                                  gboolean          extend);
