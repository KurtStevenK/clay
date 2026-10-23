/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayspinscale.h
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_SPIN_SCALE_H__
#define __CLAY_SPIN_SCALE_H__

#include <libclaywidgets/clayspinbutton.h>

G_BEGIN_DECLS


#define CLAY_TYPE_SPIN_SCALE (clay_spin_scale_get_type ())
G_DECLARE_FINAL_TYPE (ClaySpinScale, clay_spin_scale, CLAY, SPIN_SCALE, ClaySpinButton)


GtkWidget   * clay_spin_scale_new                 (GtkAdjustment *adjustment,
                                                   const gchar   *label,
                                                   gint           digits);

void          clay_spin_scale_set_label           (ClaySpinScale *scale,
                                                   const gchar   *label);
const gchar * clay_spin_scale_get_label           (ClaySpinScale *scale);

void          clay_spin_scale_set_scale_limits    (ClaySpinScale *scale,
                                                   gdouble        lower,
                                                   gdouble        upper);
void          clay_spin_scale_unset_scale_limits  (ClaySpinScale *scale);
gboolean      clay_spin_scale_get_scale_limits    (ClaySpinScale *scale,
                                                   gdouble       *lower,
                                                   gdouble       *upper);

void          clay_spin_scale_set_gamma           (ClaySpinScale *scale,
                                                   gdouble        gamma);
gdouble       clay_spin_scale_get_gamma           (ClaySpinScale *scale);

void          clay_spin_scale_set_constrain_drag  (ClaySpinScale *scale,
                                                   gboolean       constrain);
gboolean      clay_spin_scale_get_constrain_drag  (ClaySpinScale *scale);

const guint   clay_spin_scale_get_mnemonic_keyval (ClaySpinScale *scale);

G_END_DECLS

#endif  /*  __CLAY_SPIN_SCALE_H__  */
