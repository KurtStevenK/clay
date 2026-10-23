/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "clayauxitem.h"


#define CLAY_GUIDE_POSITION_UNDEFINED G_MININT


#define CLAY_TYPE_GUIDE            (clay_guide_get_type ())
#define CLAY_GUIDE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GUIDE, ClayGuide))
#define CLAY_GUIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GUIDE, ClayGuideClass))
#define CLAY_IS_GUIDE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GUIDE))
#define CLAY_IS_GUIDE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GUIDE))
#define CLAY_GUIDE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GUIDE, ClayGuideClass))


typedef struct _ClayGuidePrivate ClayGuidePrivate;
typedef struct _ClayGuideClass   ClayGuideClass;

struct _ClayGuide
{
  ClayAuxItem       parent_instance;

  ClayGuidePrivate *priv;
};

struct _ClayGuideClass
{
  ClayAuxItemClass  parent_class;
};


GType               clay_guide_get_type         (void);

ClayGuide *         clay_guide_new              (ClayOrientationType  orientation,
                                                 guint32              guide_ID);
ClayGuide *         clay_guide_custom_new       (ClayOrientationType  orientation,
                                                 guint32              guide_ID,
                                                 ClayGuideStyle       guide_style);

ClayOrientationType clay_guide_get_orientation  (ClayGuide           *guide);
void                clay_guide_set_orientation  (ClayGuide           *guide,
                                                 ClayOrientationType  orientation);

gint                clay_guide_get_position     (ClayGuide           *guide);
void                clay_guide_set_position     (ClayGuide           *guide,
                                                 gint                 position);

ClayGuideStyle      clay_guide_get_style        (ClayGuide           *guide);
gboolean            clay_guide_is_custom        (ClayGuide           *guide);
