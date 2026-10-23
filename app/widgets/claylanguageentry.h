/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylanguageentry.h
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_LANGUAGE_ENTRY            (clay_language_entry_get_type ())
#define CLAY_LANGUAGE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LANGUAGE_ENTRY, ClayLanguageEntry))
#define CLAY_LANGUAGE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LANGUAGE_ENTRY, ClayLanguageEntryClass))
#define CLAY_IS_LANGUAGE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LANGUAGE_ENTRY))
#define CLAY_IS_LANGUAGE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LANGUAGE_ENTRY))
#define CLAY_LANGUAGE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LANGUAGE_ENTRY, ClayLanguageEntryClass))


typedef struct _ClayLanguageEntryClass  ClayLanguageEntryClass;

struct _ClayLanguageEntryClass
{
  GtkEntryClass  parent_class;
};


GType         clay_language_entry_get_type     (void);

GtkWidget   * clay_language_entry_new      (void);

const gchar * clay_language_entry_get_code (ClayLanguageEntry *entry);
gboolean      clay_language_entry_set_code (ClayLanguageEntry *entry,
                                            const gchar       *code);
