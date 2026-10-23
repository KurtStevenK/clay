/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claywidgets-error.h
 * Copyright (C) 2008  Martin Nordholts <martinn@svn.gnome.org>
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

#ifndef __CLAY_WIDGETS_ERROR_H__
#define __CLAY_WIDGETS_ERROR_H__

G_BEGIN_DECLS


/**
 * ClayWidgetsError:
 * @CLAY_WIDGETS_PARSE_ERROR: A parse error has occurred
 *
 * Types of errors returned by libclaywidgets functions
 **/
typedef enum
{
  CLAY_WIDGETS_PARSE_ERROR
} ClayWidgetsError;


/**
 * CLAY_WIDGETS_ERROR:
 *
 * The Clay widgets error domain.
 *
 * Since: 2.8
 */
#define CLAY_WIDGETS_ERROR (clay_widgets_error_quark ())

GQuark  clay_widgets_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __CLAY_WIDGETS_ERROR_H__ */
