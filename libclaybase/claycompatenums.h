/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_COMPAT_ENUMS_H__
#define __CLAY_COMPAT_ENUMS_H__


G_BEGIN_DECLS

/*  These enums exist only for compatibility, their nicks are needed
 *  for config file parsing; they are registered in clay_base_init().
 */


#if 0
/*  Leave one enum here for documentation purposes, remove it as
 *  soon as we need a real compat enum again, also don't have
 *  that "skip" on the compat enum...
 */
#define CLAY_TYPE_ADD_MASK_TYPE_COMPAT (clay_add_mask_type_compat_get_type ())

GType clay_add_mask_type_compat_get_type (void);

typedef enum /*< skip >*/
{
  CLAY_ADD_WHITE_MASK          = CLAY_ADD_MASK_WHITE,
  CLAY_ADD_BLACK_MASK          = CLAY_ADD_MASK_BLACK,
  CLAY_ADD_ALPHA_MASK          = CLAY_ADD_MASK_ALPHA,
  CLAY_ADD_ALPHA_TRANSFER_MASK = CLAY_ADD_MASK_ALPHA_TRANSFER,
  CLAY_ADD_SELECTION_MASK      = CLAY_ADD_MASK_SELECTION,
  CLAY_ADD_COPY_MASK           = CLAY_ADD_MASK_COPY,
  CLAY_ADD_CHANNEL_MASK        = CLAY_ADD_MASK_CHANNEL
} ClayAddMaskTypeCompat;
#endif


G_END_DECLS

#endif  /* __CLAY_COMPAT_ENUMS_H__ */
