/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayresolutionentry.h
 * Copyright (C) 2024 Jehan
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_RESOLUTION_ENTRY__
#define __CLAY_RESOLUTION_ENTRY__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/*
 * This widget was initially part of libclaywidgets. Then it got removed and
 * moved as private code to file-pdf-load plug-in.
 *
 * I'm adding it back for Clay 3 release as its own code but not making it
 * public (only to be used by ClayVectorLoadProcedure so far) so that we don't
 * have to make sure the API is really right.
 *
 * TODO: we want to eventually get this back to public API in libclaywidgets,
 * with a much nicer API and also adding a property widget function.
 */
#define CLAY_TYPE_RESOLUTION_ENTRY (clay_resolution_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayResolutionEntry, clay_resolution_entry, CLAY, RESOLUTION_ENTRY, GtkGrid)

typedef struct _ClayResolutionEntry ClayResolutionEntry;

GtkWidget * clay_prop_resolution_entry_new          (GObject                  *config,
                                                     const gchar              *width_prop,
                                                     const gchar              *height_prop,
                                                     const gchar              *ppi_prop,
                                                     const gchar              *unit_prop);
GtkWidget * clay_resolution_entry_new               (const gchar              *width_label,
                                                     gint                      width,
                                                     const gchar              *height_label,
                                                     gint                      height,
                                                     const gchar              *res_label,
                                                     gdouble                   pixel_density,
                                                     ClayUnit                 *display_unit);

void        clay_resolution_entry_set_width         (ClayResolutionEntry      *entry,
                                                     gint                      width);
void        clay_resolution_entry_set_height        (ClayResolutionEntry      *entry,
                                                     gint                      height);
void        clay_resolution_entry_set_pixel_density (ClayResolutionEntry      *entry,
                                                     gdouble                   ppi);
void        clay_resolution_entry_set_unit          (ClayResolutionEntry      *entry,
                                                     ClayUnit                 *unit);
void        clay_resolution_entry_set_keep_ratio    (ClayResolutionEntry      *entry,
                                                     gboolean                  keep_ratio);

gint        clay_resolution_entry_get_width         (ClayResolutionEntry      *entry);
gint        clay_resolution_entry_get_height        (ClayResolutionEntry      *entry);
gdouble     clay_resolution_entry_get_density       (ClayResolutionEntry      *entry);
ClayUnit  * clay_resolution_entry_get_unit          (ClayResolutionEntry      *entry);
gboolean    clay_resolution_entry_get_keep_ratio    (ClayResolutionEntry      *entry);

G_END_DECLS

#endif /* __CLAY_RESOLUTION_ENTRY__ */
