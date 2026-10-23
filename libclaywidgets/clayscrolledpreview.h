/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayscrolledpreview.h
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

#ifndef __CLAY_SCROLLED_PREVIEW_H__
#define __CLAY_SCROLLED_PREVIEW_H__

#include "claypreview.h"

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_SCROLLED_PREVIEW (clay_scrolled_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayScrolledPreview, clay_scrolled_preview, CLAY, SCROLLED_PREVIEW, ClayPreview)

struct _ClayScrolledPreviewClass
{
  ClayPreviewClass  parent_class;

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


void   clay_scrolled_preview_set_position    (ClayScrolledPreview  *preview,
                                              gint                  x,
                                              gint                  y);
void   clay_scrolled_preview_set_policy      (ClayScrolledPreview  *preview,
                                              GtkPolicyType         hscrollbar_policy,
                                              GtkPolicyType         vscrollbar_policy);

void   clay_scrolled_preview_get_adjustments (ClayScrolledPreview  *preview,
                                              GtkAdjustment       **hadj,
                                              GtkAdjustment       **vadj);

/*  only for use from derived widgets  */
void   clay_scrolled_preview_freeze          (ClayScrolledPreview  *preview);
void   clay_scrolled_preview_thaw            (ClayScrolledPreview  *preview);

/*  utility function for scrolled-window like clay widgets like the canvas  */
void   clay_scroll_adjustment_values         (GdkEventScroll       *sevent,
                                              GtkAdjustment        *hadj,
                                              GtkAdjustment        *vadj,
                                              gdouble              *hvalue,
                                              gdouble              *vvalue);


G_END_DECLS

#endif /* __CLAY_SCROLLED_PREVIEW_H__ */
