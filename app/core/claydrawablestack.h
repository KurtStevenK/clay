/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claydrawablestack.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include "clayitemstack.h"


#define CLAY_TYPE_DRAWABLE_STACK (clay_drawable_stack_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDrawableStack,
                          clay_drawable_stack,
                          CLAY, DRAWABLE_STACK,
                          ClayItemStack)


struct _ClayDrawableStackClass
{
  ClayItemStackClass  parent_class;

  void (* update) (ClayDrawableStack *stack,
                   gint               x,
                   gint               y,
                   gint               width,
                   gint               height);
};


ClayContainer * clay_drawable_stack_new    (GType              drawable_type);


/*  protected  */

void            clay_drawable_stack_update (ClayDrawableStack *stack,
                                            gint               x,
                                            gint               y,
                                            gint               width,
                                            gint               height);
