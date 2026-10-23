/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claytemplate.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "clayviewable.h"


#define CLAY_TEMPLATE_PARAM_COPY_FIRST (1 << (8 + G_PARAM_USER_SHIFT))

#ifdef CLAY_UNSTABLE
/* Uncommon ratio, with at least one odd value, to encourage testing
 * Clay with unusual numbers.
 * It also has to be a higher resolution, to push Clay a little further
 * in tests. */
#define CLAY_DEFAULT_IMAGE_WIDTH   2001
#define CLAY_DEFAULT_IMAGE_HEIGHT  1984
#else
/* 1366x768 is the most common screen resolution in 2016.
 * 1920x1080 is the second most common.
 * Since Clay targets advanced graphics artists, let's go for the
 * highest common dimension.
 */
#define CLAY_DEFAULT_IMAGE_WIDTH   1920
#define CLAY_DEFAULT_IMAGE_HEIGHT  1080
#endif


#define CLAY_TYPE_TEMPLATE (clay_template_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayTemplate,
                          clay_template,
                          CLAY, TEMPLATE,
                          ClayViewable)


struct _ClayTemplateClass
{
  ClayViewableClass  parent_class;
};


ClayTemplate      * clay_template_new                 (const gchar  *name);

void                clay_template_set_from_image      (ClayTemplate *template,
                                                       ClayImage    *image);

gint                clay_template_get_width           (ClayTemplate *template);
gint                clay_template_get_height          (ClayTemplate *template);
ClayUnit          * clay_template_get_unit            (ClayTemplate *template);

gdouble             clay_template_get_resolution_x    (ClayTemplate *template);
gdouble             clay_template_get_resolution_y    (ClayTemplate *template);
ClayUnit          * clay_template_get_resolution_unit (ClayTemplate *template);

ClayImageBaseType   clay_template_get_base_type       (ClayTemplate *template);
ClayPrecision       clay_template_get_precision       (ClayTemplate *template);

ClayColorProfile  * clay_template_get_color_profile   (ClayTemplate *template);
ClayColorProfile  * clay_template_get_simulation_profile
                                                      (ClayTemplate *template);
ClayColorRenderingIntent clay_template_get_simulation_intent
                                                      (ClayTemplate *template);
gboolean            clay_template_get_simulation_bpc  (ClayTemplate *template);

ClayFillType        clay_template_get_fill_type       (ClayTemplate *template);

const gchar       * clay_template_get_comment         (ClayTemplate *template);

guint64             clay_template_get_initial_size    (ClayTemplate *template);
