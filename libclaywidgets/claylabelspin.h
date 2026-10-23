/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorscaleentry.h
 * Copyright (C) 2020 Jehan
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

#ifndef __CLAY_LABEL_SPIN_H__
#define __CLAY_LABEL_SPIN_H__

#include <libclaywidgets/claylabeled.h>

G_BEGIN_DECLS

#define CLAY_TYPE_LABEL_SPIN (clay_label_spin_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayLabelSpin, clay_label_spin, CLAY, LABEL_SPIN, ClayLabeled)

struct _ClayLabelSpinClass
{
  ClayLabeledClass   parent_class;

  /*  Signals */
  void            (* value_changed)    (ClayLabelSpin *spin);

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

GtkWidget  * clay_label_spin_new             (const gchar   *text,
                                              gdouble        value,
                                              gdouble        lower,
                                              gdouble        upper,
                                              gint           digits);

void         clay_label_spin_set_value       (ClayLabelSpin *spin,
                                              gdouble        value);
gdouble      clay_label_spin_get_value       (ClayLabelSpin *spin);

void         clay_label_spin_set_increments  (ClayLabelSpin *spin,
                                              gdouble        step,
                                              gdouble        page);
void         clay_label_spin_set_digits      (ClayLabelSpin *spin,
                                              gint           digits);

GtkWidget  * clay_label_spin_get_spin_button (ClayLabelSpin *spin);

G_END_DECLS

#endif /* __CLAY_LABEL_SPIN_H__ */
