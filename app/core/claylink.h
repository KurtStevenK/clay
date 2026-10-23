/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayLink
 * Copyright (C) 2019 Jehan
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

#include "clayitem.h"


#define CLAY_TYPE_LINK            (clay_link_get_type ())
#define CLAY_LINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LINK, ClayLink))
#define CLAY_LINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LINK, ClayLinkClass))
#define CLAY_IS_LINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LINK))
#define CLAY_IS_LINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LINK))
#define CLAY_LINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LINK, ClayLinkClass))


typedef struct _ClayLinkClass   ClayLinkClass;
typedef struct _ClayLinkPrivate ClayLinkPrivate;

struct _ClayLink
{
  ClayObject         parent_instance;

  ClayLinkPrivate   *p;
};

struct _ClayLinkClass
{
  ClayObjectClass    parent_class;

  void   (* changed)              (ClayLink  *link);
};


GType                 clay_link_get_type          (void);

ClayLink            * clay_link_new               (Clay           *clay,
                                                   GFile          *file,
                                                   gint            vector_width,
                                                   gint            vector_height,
                                                   gboolean        keep_ratio,
                                                   ClayProgress  *progress,
                                                   GError       **error);
ClayLink            * clay_link_duplicate         (ClayLink       *link);

GFile               * clay_link_get_file          (ClayLink       *link,
                                                   GFile          *parent,
                                                   gchar         **path);
void                  clay_link_set_file          (ClayLink       *layer,
                                                   GFile          *file,
                                                   gint            vector_width,
                                                   gint            vector_height,
                                                   gboolean        keep_ratio,
                                                   ClayProgress   *progress,
                                                   GError        **error);
gboolean              clay_link_get_absolute_path (ClayLink       *link);
void                  clay_link_set_absolute_path (ClayLink       *link,
                                                   gboolean        absolute_path);

const gchar         * clay_link_get_mime_type     (ClayLink       *link);

void                  clay_link_freeze            (ClayLink       *link);
void                  clay_link_thaw              (ClayLink       *link);
gboolean              clay_link_is_monitored      (ClayLink       *link);

gboolean              clay_link_is_broken         (ClayLink       *link);

void                  clay_link_set_size          (ClayLink       *link,
                                                   gint            width,
                                                   gint            height,
                                                   gboolean        keep_ratio);
void                  clay_link_get_size          (ClayLink       *link,
                                                   gint           *width,
                                                   gint           *height);
ClayImageBaseType     clay_link_get_base_type     (ClayLink      *link);
ClayPrecision         clay_link_get_precision     (ClayLink      *link);
ClayPlugInProcedure * clay_link_get_load_proc     (ClayLink      *link);

gboolean              clay_link_is_vector         (ClayLink       *link);

GeglBuffer          * clay_link_get_buffer        (ClayLink       *link);
