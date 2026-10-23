/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claysizeentry.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_SIZE_ENTRY_H__
#define __CLAY_SIZE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_SIZE_ENTRY (clay_size_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClaySizeEntry, clay_size_entry, CLAY, SIZE_ENTRY, GtkGrid)


/* For information look into the C source or the html documentation */

GtkWidget * clay_size_entry_new (gint                       number_of_fields,
                                 ClayUnit                  *unit,
                                 const gchar               *unit_format,
                                 gboolean                   menu_show_pixels,
                                 gboolean                   menu_show_percent,
                                 gboolean                   show_refval,
                                 gint                       spinbutton_width,
                                 ClaySizeEntryUpdatePolicy  update_policy);

void        clay_size_entry_add_field  (ClaySizeEntry   *gse,
                                        GtkSpinButton   *value_spinbutton,
                                        GtkSpinButton   *refval_spinbutton);

ClaySizeEntryUpdatePolicy
            clay_size_entry_get_update_policy     (ClaySizeEntry *gse);
gint        clay_size_entry_get_n_fields          (ClaySizeEntry *gse);
GtkWidget * clay_size_entry_get_unit_combo        (ClaySizeEntry *gse);

GtkWidget * clay_size_entry_attach_label          (ClaySizeEntry *gse,
                                                   const gchar   *text,
                                                   gint           row,
                                                   gint           column,
                                                   gfloat         alignment);

void        clay_size_entry_set_resolution        (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        resolution,
                                                   gboolean       keep_size);

void        clay_size_entry_set_size              (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

void        clay_size_entry_set_value_boundaries  (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

gdouble     clay_size_entry_get_value             (ClaySizeEntry *gse,
                                                   gint           field);
void        clay_size_entry_set_value             (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        value);

void        clay_size_entry_set_refval_boundaries (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);
void        clay_size_entry_set_refval_digits     (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gint           digits);

gdouble     clay_size_entry_get_refval            (ClaySizeEntry *gse,
                                                   gint           field);
void        clay_size_entry_set_refval            (ClaySizeEntry *gse,
                                                   gint           field,
                                                   gdouble        refval);

ClayUnit  * clay_size_entry_get_unit              (ClaySizeEntry *gse);
void        clay_size_entry_set_unit              (ClaySizeEntry *gse,
                                                   ClayUnit      *unit);
void        clay_size_entry_show_unit_menu        (ClaySizeEntry *gse,
                                                   gboolean       show);

void        clay_size_entry_set_pixel_digits      (ClaySizeEntry *gse,
                                                   gint           digits);

void        clay_size_entry_grab_focus            (ClaySizeEntry *gse);
void        clay_size_entry_set_activates_default (ClaySizeEntry *gse,
                                                   gboolean       setting);
GtkWidget * clay_size_entry_get_help_widget       (ClaySizeEntry *gse,
                                                   gint           field);


G_END_DECLS

#endif /* __CLAY_SIZE_ENTRY_H__ */
