/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2017 Sébastien Fourey & David Tchumperlé
 * Copyright (C) 2018 Jehan
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

#include "clayobject.h"


#define CLAY_TYPE_LINE_ART            (clay_line_art_get_type ())
#define CLAY_LINE_ART(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LINE_ART, ClayLineArt))
#define CLAY_LINE_ART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LINE_ART, ClayLineArtClass))
#define CLAY_IS_LINE_ART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LINE_ART))
#define CLAY_IS_LINE_ART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LINE_ART))
#define CLAY_LINE_ART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LINE_ART, ClayLineArtClass))


typedef struct _ClayLineArtClass   ClayLineArtClass;
typedef struct _ClayLineArtPrivate ClayLineArtPrivate;

struct _ClayLineArt
{
  ClayObject          parent_instance;

  ClayLineArtPrivate *priv;
};

struct _ClayLineArtClass
{
  ClayObjectClass     parent_class;

  /* Signals */

  void (* computing_start) (ClayLineArt *line_art);
  void (* computing_end)   (ClayLineArt *line_art);
};


GType                clay_line_art_get_type         (void);

ClayLineArt        * clay_line_art_new              (void);

void                 clay_line_art_bind_gap_length  (ClayLineArt  *line_art,
                                                     gboolean      bound);

void                 clay_line_art_set_input        (ClayLineArt  *line_art,
                                                     ClayPickable *pickable);
ClayPickable       * clay_line_art_get_input        (ClayLineArt  *line_art);
void                 clay_line_art_freeze           (ClayLineArt  *line_art);
void                 clay_line_art_thaw             (ClayLineArt  *line_art);
gboolean             clay_line_art_is_frozen        (ClayLineArt  *line_art);

GeglBuffer         * clay_line_art_get              (ClayLineArt  *line_art,
                                                     gfloat      **distmap);
