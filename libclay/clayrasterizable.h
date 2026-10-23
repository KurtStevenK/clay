/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayrasterizable.h
 * Copyright (C) 2025 Jehan
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_RASTERIZABLE_H__
#define __CLAY_RASTERIZABLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libclay/claydrawable.h>


#define CLAY_TYPE_RASTERIZABLE (clay_rasterizable_get_type ())
G_DECLARE_INTERFACE (ClayRasterizable, clay_rasterizable, CLAY, RASTERIZABLE, ClayDrawable)


struct _ClayRasterizableInterface
{
  GTypeInterface base_iface;
};


G_END_DECLS

#endif /* __CLAY_RASTERIZABLE_H__ */
