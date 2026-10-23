/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabeled.h
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

#ifndef __CLAY_LABELED_H__
#define __CLAY_LABELED_H__

G_BEGIN_DECLS

#define CLAY_TYPE_LABELED (clay_labeled_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayLabeled, clay_labeled, CLAY, LABELED, GtkGrid)

struct _ClayLabeledClass
{
  GtkGridClass       parent_class;

  /*  Signals        */

  void (* mnemonic_widget_changed) (ClayLabeled *labeled,
                                    GtkWidget   *widget);

  /*  Class methods  */

  /**
   * ClayLabelledClass::populate:
   *
   * Fill the #GtkGrid with any necessary widget and sets the
   * coordinates and dimensions the #GtkLabel should be attached to.
   * By default, @x, @y, @width and @height will be pre-filled with 0,
   * 0, 1 and 1 respectively, i.e. the top-left of the grid. There is no
   * need to edit these output variables unless your subclass wants the
   * label to be placed elsewhere.
   *
   * Returns: (transfer none): the #GtkWidget which the label must be
   *                           set as mnemonic to.
   **/
  GtkWidget     * (* populate)     (ClayLabeled *labeled,
                                    gint        *x,
                                    gint        *y,
                                    gint        *width,
                                    gint        *height);


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

GtkWidget     * clay_labeled_get_label (ClayLabeled *labeled);

const gchar   * clay_labeled_get_text  (ClayLabeled *labeled);
void            clay_labeled_set_text  (ClayLabeled *labeled,
                                        const gchar *text);


G_END_DECLS

#endif /* __CLAY_LABELED_H__ */
