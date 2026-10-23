/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasgroup.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "claycanvasitem.h"


#define CLAY_TYPE_CANVAS_GROUP            (clay_canvas_group_get_type ())
#define CLAY_CANVAS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CANVAS_GROUP, ClayCanvasGroup))
#define CLAY_CANVAS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CANVAS_GROUP, ClayCanvasGroupClass))
#define CLAY_IS_CANVAS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CANVAS_GROUP))
#define CLAY_IS_CANVAS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CANVAS_GROUP))
#define CLAY_CANVAS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CANVAS_GROUP, ClayCanvasGroupClass))


typedef struct _ClayCanvasGroupPrivate ClayCanvasGroupPrivate;
typedef struct _ClayCanvasGroupClass   ClayCanvasGroupClass;

struct _ClayCanvasGroup
{
  ClayCanvasItem          parent_instance;

  ClayCanvasGroupPrivate *priv;

};

struct _ClayCanvasGroupClass
{
  ClayCanvasItemClass  parent_class;
};


GType            clay_canvas_group_get_type           (void);

ClayCanvasItem * clay_canvas_group_new                (ClayDisplayShell *shell);

void             clay_canvas_group_add_item           (ClayCanvasGroup  *group,
                                                       ClayCanvasItem   *item);
void             clay_canvas_group_remove_item        (ClayCanvasGroup  *group,
                                                       ClayCanvasItem   *item);

void             clay_canvas_group_set_group_stroking (ClayCanvasGroup  *group,
                                                       gboolean          group_stroking);
void             clay_canvas_group_set_group_filling  (ClayCanvasGroup  *group,
                                                       gboolean          group_filling);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayCanvasGroup, g_object_unref);
