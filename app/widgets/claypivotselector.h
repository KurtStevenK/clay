/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypivotselector.h
 * Copyright (C) 2019 Ell
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

#pragma once


#define CLAY_TYPE_PIVOT_SELECTOR            (clay_pivot_selector_get_type ())
#define CLAY_PIVOT_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PIVOT_SELECTOR, ClayPivotSelector))
#define CLAY_PIVOT_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PIVOT_SELECTOR, ClayPivotSelectorClass))
#define CLAY_IS_PIVOT_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_PIVOT_SELECTOR))
#define CLAY_IS_PIVOT_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PIVOT_SELECTOR))
#define CLAY_PIVOT_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PIVOT_SELECTOR, ClayPivotSelectorClass))


typedef struct _ClayPivotSelectorPrivate ClayPivotSelectorPrivate;
typedef struct _ClayPivotSelectorClass   ClayPivotSelectorClass;

struct _ClayPivotSelector
{
  GtkGrid                   parent_instance;

  ClayPivotSelectorPrivate *priv;
};

struct _ClayPivotSelectorClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void (* changed) (ClayPivotSelector *selector);
};


GType       clay_pivot_selector_get_type     (void);

GtkWidget * clay_pivot_selector_new          (gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);

void        clay_pivot_selector_set_position (ClayPivotSelector *selector,
                                              gdouble            x,
                                              gdouble            y);
void        clay_pivot_selector_get_position (ClayPivotSelector *selector,
                                              gdouble           *x,
                                              gdouble           *y);

void        clay_pivot_selector_set_bounds   (ClayPivotSelector *selector,
                                              gdouble            left,
                                              gdouble            top,
                                              gdouble            right,
                                              gdouble            bottom);
void        clay_pivot_selector_get_bounds   (ClayPivotSelector *selector,
                                              gdouble           *left,
                                              gdouble           *top,
                                              gdouble           *right,
                                              gdouble           *bottom);
