/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayscaleentry.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>,
 *               2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
 *               2020 Jehan
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

#include <libclaywidgets/claylabelspin.h>

#ifndef __CLAY_SCALE_ENTRY_H__
#define __CLAY_SCALE_ENTRY_H__

G_BEGIN_DECLS

#define CLAY_TYPE_SCALE_ENTRY (clay_scale_entry_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayScaleEntry, clay_scale_entry, CLAY, SCALE_ENTRY, ClayLabelSpin)

struct _ClayScaleEntryClass
{
  ClayLabelSpinClass parent_class;

  /*  Class methods  */
  GtkWidget     * (* new_range_widget) (GtkAdjustment  *adjustment);

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

GtkWidget     * clay_scale_entry_new             (const gchar *text,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  guint        digits);

GtkWidget     * clay_scale_entry_get_range       (ClayScaleEntry *entry);

void            clay_scale_entry_set_bounds      (ClayScaleEntry *entry,
                                                  gdouble         lower,
                                                  gdouble         upper,
                                                  gboolean        limit_scale);

void            clay_scale_entry_set_logarithmic (ClayScaleEntry *entry,
                                                  gboolean        logarithmic);
gboolean        clay_scale_entry_get_logarithmic (ClayScaleEntry *entry);


G_END_DECLS

#endif /* __CLAY_SCALE_ENTRY_H__ */
