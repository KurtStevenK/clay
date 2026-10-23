/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claytextlayer.h
 * Copyright (C) 2022 Jehan
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_TEXT_LAYER_H__
#define __CLAY_TEXT_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libclay/claylayer.h>


#define CLAY_TYPE_TEXT_LAYER (clay_text_layer_get_type ())
G_DECLARE_FINAL_TYPE (ClayTextLayer, clay_text_layer, CLAY, TEXT_LAYER, ClayLayer)


ClayTextLayer * clay_text_layer_get_by_id (gint32       layer_id);


G_END_DECLS

#endif /* __CLAY_TEXT_LAYER_H__ */
