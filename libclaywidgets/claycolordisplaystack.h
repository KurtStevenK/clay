/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolordisplaystack.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_DISPLAY_STACK_H__
#define __CLAY_COLOR_DISPLAY_STACK_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define CLAY_TYPE_COLOR_DISPLAY_STACK (clay_color_display_stack_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorDisplayStack,
                      clay_color_display_stack,
                      CLAY, COLOR_DISPLAY_STACK,
                      GObject)


ClayColorDisplayStack * clay_color_display_stack_new      (void);
ClayColorDisplayStack * clay_color_display_stack_clone    (ClayColorDisplayStack *stack);

void    clay_color_display_stack_changed         (ClayColorDisplayStack *stack);

GList * clay_color_display_stack_get_filters     (ClayColorDisplayStack *stack);

void    clay_color_display_stack_add             (ClayColorDisplayStack *stack,
                                                  ClayColorDisplay      *display);
void    clay_color_display_stack_remove          (ClayColorDisplayStack *stack,
                                                  ClayColorDisplay      *display);
void    clay_color_display_stack_reorder_up      (ClayColorDisplayStack *stack,
                                                  ClayColorDisplay      *display);
void    clay_color_display_stack_reorder_down    (ClayColorDisplayStack *stack,
                                                  ClayColorDisplay      *display);
void    clay_color_display_stack_convert_buffer  (ClayColorDisplayStack *stack,
                                                  GeglBuffer            *buffer,
                                                  GeglRectangle         *area);


G_END_DECLS

#endif /* __CLAY_COLOR_DISPLAY_STACK_H__ */
