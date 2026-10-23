/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claywidgets.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_WIDGETS_H__
#define __CLAY_WIDGETS_H__

#define __CLAY_WIDGETS_H_INSIDE__

#include <libclaywidgets/claywidgetstypes.h>

#include <libclaywidgets/claybrowser.h>
#include <libclaywidgets/claybusybox.h>
#include <libclaywidgets/claybutton.h>
#include <libclaywidgets/claycairo-utils.h>
#include <libclaywidgets/claycellrenderercolor.h>
#include <libclaywidgets/claycellrenderertoggle.h>
#include <libclaywidgets/claychainbutton.h>
#include <libclaywidgets/claycolorarea.h>
#include <libclaywidgets/claycolorbutton.h>
#include <libclaywidgets/claycolordisplay.h>
#include <libclaywidgets/claycolordisplaystack.h>
#include <libclaywidgets/claycolorhexentry.h>
#include <libclaywidgets/claycolornotebook.h>
#include <libclaywidgets/claycolorprofilechooserdialog.h>
#include <libclaywidgets/claycolorprofilecombobox.h>
#include <libclaywidgets/claycolorprofilestore.h>
#include <libclaywidgets/claycolorprofileview.h>
#include <libclaywidgets/claycolorscale.h>
#include <libclaywidgets/claycolorscaleentry.h>
#include <libclaywidgets/claycolorscales.h>
#include <libclaywidgets/claycolorselector.h>
#include <libclaywidgets/claycolorselect.h>
#include <libclaywidgets/claycolorselection.h>
#include <libclaywidgets/claydialog.h>
#include <libclaywidgets/clayenumcombobox.h>
#include <libclaywidgets/clayenumlabel.h>
#include <libclaywidgets/clayenumstore.h>
#include <libclaywidgets/clayenumwidgets.h>
#include <libclaywidgets/clayfilechooser.h>
#include <libclaywidgets/clayframe.h>
#include <libclaywidgets/clayhelpui.h>
#include <libclaywidgets/clayhintbox.h>
#include <libclaywidgets/clayicons.h>
#include <libclaywidgets/clayintcombobox.h>
#include <libclaywidgets/clayintradioframe.h>
#include <libclaywidgets/clayintstore.h>
#include <libclaywidgets/claylabelcolor.h>
#include <libclaywidgets/claylabeled.h>
#include <libclaywidgets/claylabelentry.h>
#include <libclaywidgets/claylabelintwidget.h>
#include <libclaywidgets/claylabelspin.h>
#include <libclaywidgets/claylabelstringwidget.h>
#include <libclaywidgets/claymemsizeentry.h>
#include <libclaywidgets/claynumberpairentry.h>
#include <libclaywidgets/clayoffsetarea.h>
#include <libclaywidgets/claypageselector.h>
#include <libclaywidgets/claypatheditor.h>
#include <libclaywidgets/claypickbutton.h>
#include <libclaywidgets/claypreview.h>
#include <libclaywidgets/claypreviewarea.h>
#include <libclaywidgets/claypropwidgets.h>
#include <libclaywidgets/clayquerybox.h>
#include <libclaywidgets/clayruler.h>
#include <libclaywidgets/clayscaleentry.h>
#include <libclaywidgets/clayscrolledpreview.h>
#include <libclaywidgets/claysizeentry.h>
#include <libclaywidgets/clayspinbutton.h>
#include <libclaywidgets/clayspinscale.h>
#include <libclaywidgets/claystringcombobox.h>
#include <libclaywidgets/clayunitcombobox.h>
#include <libclaywidgets/clayunitstore.h>
#include <libclaywidgets/claywidgets-error.h>
#include <libclaywidgets/claywidgetsutils.h>
#include <libclaywidgets/clayzoommodel.h>

#undef __CLAY_WIDGETS_H_INSIDE__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 *  Widget Constructors
 */

/* specify radio buttons as va_list:
 *  const gchar  *label,
 *  gint          item_data,
 *  GtkWidget   **widget_ptr,
 */
GtkWidget * clay_int_radio_group_new (gboolean          in_frame,
                                      const gchar      *frame_title,
                                      GCallback         radio_button_callback,
                                      gpointer          radio_button_callback_data,
                                      GDestroyNotify    radio_button_callback_destroy,
                                      gint              initial, /* item_data */
                                      ...) G_GNUC_NULL_TERMINATED;

void        clay_int_radio_group_set_active (GtkRadioButton *radio_button,
                                             gint            item_data);


/**
 * CLAY_RANDOM_SEED_SPINBUTTON:
 * @hbox: The #GtkBox returned by clay_random_seed_new().
 *
 * Returns: the random_seed's #GtkSpinButton.
 **/
#define CLAY_RANDOM_SEED_SPINBUTTON(hbox) \
        (g_object_get_data (G_OBJECT (hbox), "spinbutton"))

/**
 * CLAY_RANDOM_SEED_SPINBUTTON_ADJ:
 * @hbox: The #GtkBox returned by clay_random_seed_new().
 *
 * Returns: the #GtkAdjustment of the random_seed's #GtkSpinButton.
 **/
#define CLAY_RANDOM_SEED_SPINBUTTON_ADJ(hbox)       \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (hbox), "spinbutton")))

/**
 * CLAY_RANDOM_SEED_TOGGLE:
 * @hbox: The #GtkBox returned by clay_random_seed_new().
 *
 * Returns: the random_seed's #GtkToggleButton.
 **/
#define CLAY_RANDOM_SEED_TOGGLE(hbox) \
        (g_object_get_data (G_OBJECT(hbox), "toggle"))

GtkWidget * clay_random_seed_new   (guint32            *seed,
                                    gboolean           *random_seed);

/**
 * CLAY_COORDINATES_CHAINBUTTON:
 * @sizeentry: The #ClaySizeEntry returned by clay_coordinates_new().
 *
 * Returns: the #ClayChainButton which is attached to the
 *          #ClaySizeEntry.
 **/
#define CLAY_COORDINATES_CHAINBUTTON(sizeentry) \
        (g_object_get_data (G_OBJECT (sizeentry), "chainbutton"))

GtkWidget * clay_coordinates_new   (ClayUnit           *unit,
                                    const gchar        *unit_format,
                                    gboolean            menu_show_pixels,
                                    gboolean            menu_show_percent,
                                    gint                spinbutton_width,
                                    ClaySizeEntryUpdatePolicy  update_policy,

                                    gboolean            chainbutton_active,
                                    gboolean            chain_constrains_ratio,

                                    const gchar        *xlabel,
                                    gdouble             x,
                                    gdouble             xres,
                                    gdouble             lower_boundary_x,
                                    gdouble             upper_boundary_x,
                                    gdouble             xsize_0,   /* % */
                                    gdouble             xsize_100, /* % */

                                    const gchar        *ylabel,
                                    gdouble             y,
                                    gdouble             yres,
                                    gdouble             lower_boundary_y,
                                    gdouble             upper_boundary_y,
                                    gdouble             ysize_0,   /* % */
                                    gdouble             ysize_100  /* % */);


/*
 *  Standard Callbacks
 */

void clay_toggle_button_update           (GtkWidget       *widget,
                                          gpointer         data);

void clay_radio_button_update            (GtkWidget       *widget,
                                          gpointer         data);

void clay_int_adjustment_update          (GtkAdjustment   *adjustment,
                                          gpointer         data);

void clay_uint_adjustment_update         (GtkAdjustment   *adjustment,
                                          gpointer         data);

void clay_float_adjustment_update        (GtkAdjustment   *adjustment,
                                          gpointer         data);

void clay_double_adjustment_update       (GtkAdjustment   *adjustment,
                                          gpointer         data);


G_END_DECLS

#endif /* __CLAY_WIDGETS_H__ */
