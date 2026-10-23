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

#ifndef  __CLAY_MYBRUSH_CORE_H__
#define  __CLAY_MYBRUSH_CORE_H__


#include "claypaintcore.h"


#define CLAY_TYPE_MYBRUSH_CORE            (clay_mybrush_core_get_type ())
#define CLAY_MYBRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MYBRUSH_CORE, ClayMybrushCore))
#define CLAY_MYBRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MYBRUSH_CORE, ClayMybrushCoreClass))
#define CLAY_IS_MYBRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MYBRUSH_CORE))
#define CLAY_IS_MYBRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MYBRUSH_CORE))
#define CLAY_MYBRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MYBRUSH_CORE, ClayMybrushCoreClass))


typedef struct _ClayMybrushCorePrivate ClayMybrushCorePrivate;
typedef struct _ClayMybrushCoreClass   ClayMybrushCoreClass;

struct _ClayMybrushCore
{
  ClayPaintCore           parent_instance;

  ClayMybrushCorePrivate *private;
};

struct _ClayMybrushCoreClass
{
  ClayPaintCoreClass  parent_class;
};


void    clay_mybrush_core_register (Clay                      *clay,
                                    ClayPaintRegisterCallback  callback);

GType   clay_mybrush_core_get_type (void);


#endif  /*  __CLAY_MYBRUSH_CORE_H__  */
