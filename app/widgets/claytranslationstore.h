/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytranslationstore.h
 * Copyright (C) 2009  Sven Neumann <sven@gimp.org>
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

#include "claylanguagestore.h"


#define CLAY_TYPE_TRANSLATION_STORE            (clay_translation_store_get_type ())
#define CLAY_TRANSLATION_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSLATION_STORE, ClayTranslationStore))
#define CLAY_TRANSLATION_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSLATION_STORE, ClayTranslationStoreClass))
#define CLAY_IS_TRANSLATION_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSLATION_STORE))
#define CLAY_IS_TRANSLATION_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRANSLATION_STORE))
#define CLAY_TRANSLATION_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSLATION_STORE, ClayTranslationStoreClass))


typedef struct _ClayTranslationStoreClass  ClayTranslationStoreClass;


GType          clay_translation_store_get_type   (void);

void           clay_translation_store_initialize (const gchar *system_lang_l10n);
GtkListStore * clay_translation_store_new        (gboolean     manual_l18n,
                                                  const gchar *empty_label);
