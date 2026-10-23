/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#pragma once

G_BEGIN_DECLS

#define CLAY_TYPE_COLOR_WHEEL            (clay_color_wheel_get_type ())
#define CLAY_COLOR_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_WHEEL, ClayColorWheel))
#define CLAY_COLOR_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_WHEEL, ClayColorWheelClass))
#define CLAY_IS_COLOR_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_WHEEL))
#define CLAY_IS_COLOR_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_WHEEL))
#define CLAY_COLOR_WHEEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_WHEEL, ClayColorWheelClass))


typedef struct _ClayColorWheel      ClayColorWheel;
typedef struct _ClayColorWheelClass ClayColorWheelClass;

struct _ClayColorWheel
{
  GtkWidget parent_instance;
};

struct _ClayColorWheelClass
{
  GtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (ClayColorWheel   *wheel);

  /* Keybindings */
  void (* move)    (ClayColorWheel   *wheel,
                    GtkDirectionType  type);

  /* Padding for future expansion */
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
};


void        color_wheel_register_type          (GTypeModule     *module);

GType       clay_color_wheel_get_type          (void);
GtkWidget * clay_color_wheel_new               (void);

void        clay_color_wheel_set_color         (ClayColorWheel  *wheel,
                                                double           h,
                                                double           s,
                                                double           v);
void        clay_color_wheel_get_color         (ClayColorWheel  *wheel,
                                                gfloat          *h,
                                                gfloat          *s,
                                                gfloat          *v);

void        clay_color_wheel_set_ring_fraction (ClayColorWheel  *wheel,
                                                gdouble          fraction);
gdouble     clay_color_wheel_get_ring_fraction (ClayColorWheel  *wheel);

void        clay_color_wheel_set_format        (ClayColorWheel  *wheel,
                                                const Babl      *format);
void        clay_color_wheel_set_color_config  (ClayColorWheel  *wheel,
                                                ClayColorConfig *config);

gboolean    clay_color_wheel_is_adjusting      (ClayColorWheel  *wheel);

G_END_DECLS
