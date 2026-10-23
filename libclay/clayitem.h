/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * clayitem.h
 * Copyright (C) Jehan
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

#ifndef __CLAY_ITEM_H__
#define __CLAY_ITEM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_ITEM (clay_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayItem, clay_item, CLAY, ITEM, GObject)


struct _ClayItemClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


gint32      clay_item_get_id          (ClayItem *item);
ClayItem  * clay_item_get_by_id       (gint32    item_id);

gboolean    clay_item_is_valid        (ClayItem *item);
gboolean    clay_item_is_drawable     (ClayItem *item);
gboolean    clay_item_is_layer        (ClayItem *item);
gboolean    clay_item_is_text_layer   (ClayItem *item);
gboolean    clay_item_is_vector_layer (ClayItem *item);
gboolean    clay_item_is_link_layer   (ClayItem *item);
gboolean    clay_item_is_group_layer  (ClayItem *item);
gboolean    clay_item_is_channel      (ClayItem *item);
gboolean    clay_item_is_layer_mask   (ClayItem *item);
gboolean    clay_item_is_selection    (ClayItem *item);
gboolean    clay_item_is_path         (ClayItem *item);

GList     * clay_item_list_children   (ClayItem *item);


G_END_DECLS

#endif /* __CLAY_ITEM_H__ */
