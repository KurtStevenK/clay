/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayspinbutton.h
 * Copyright (C) 2018 Ell
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

#ifndef __CLAY_SPIN_BUTTON_H__
#define __CLAY_SPIN_BUTTON_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_SPIN_BUTTON (clay_spin_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClaySpinButton, clay_spin_button, CLAY, SPIN_BUTTON, GtkSpinButton)


struct _ClaySpinButtonClass
{
  GtkSpinButtonClass  parent_class;

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


GtkWidget * clay_spin_button_new            (GtkAdjustment *adjustment,
                                             gdouble        climb_rate,
                                             guint          digits);
GtkWidget * clay_spin_button_new_with_range (gdouble        min,
                                             gdouble        max,
                                             gdouble        step);


G_END_DECLS

#endif /* __CLAY_SPIN_BUTTON_H__ */
