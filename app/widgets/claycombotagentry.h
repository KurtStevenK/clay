/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycombotagentry.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "claytagentry.h"


#define CLAY_TYPE_COMBO_TAG_ENTRY            (clay_combo_tag_entry_get_type ())
#define CLAY_COMBO_TAG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COMBO_TAG_ENTRY, ClayComboTagEntry))
#define CLAY_COMBO_TAG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COMBO_TAG_ENTRY, ClayComboTagEntryClass))
#define CLAY_IS_COMBO_TAG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COMBO_TAG_ENTRY))
#define CLAY_IS_COMBO_TAG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COMBO_TAG_ENTRY))
#define CLAY_COMBO_TAG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COMBO_TAG_ENTRY, ClayComboTagEntryClass))


typedef struct _ClayComboTagEntryClass  ClayComboTagEntryClass;

struct _ClayComboTagEntry
{
  ClayTagEntry  parent_instance;

  GtkWidget    *popup;
};

struct _ClayComboTagEntryClass
{
  ClayTagEntryClass  parent_class;
};


GType       clay_combo_tag_entry_get_type (void);

GtkWidget * clay_combo_tag_entry_new      (ClayTaggedContainer *container,
                                           ClayTagEntryMode     mode);
