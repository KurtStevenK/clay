/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytranslationstore.c
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <locale.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "clayhelp.h"
#ifdef HAVE_ISO_CODES
#include "claylanguagestore-data.h"
#endif
#include "claytranslationstore.h"

#include "clay-intl.h"

static const gchar *system_lang = NULL;

enum
{
  PROP_0,
  PROP_MANUAL_L18N,
  PROP_EMPTY_LABEL
};

struct _ClayTranslationStoreClass
{
  ClayLanguageStoreClass  parent_class;
};

struct _ClayTranslationStore
{
  ClayLanguageStore  parent_instance;

  gboolean           manual_l18n;
  gchar             *empty_label;
};


static void   clay_translation_store_constructed  (GObject           *object);
static void   clay_translation_store_set_property (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   clay_translation_store_get_property (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);


G_DEFINE_TYPE (ClayTranslationStore, clay_translation_store,
               CLAY_TYPE_LANGUAGE_STORE)

#define parent_class clay_translation_store_parent_class


static void
clay_translation_store_class_init (ClayTranslationStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_translation_store_constructed;
  object_class->set_property = clay_translation_store_set_property;
  object_class->get_property = clay_translation_store_get_property;

  g_object_class_install_property (object_class, PROP_MANUAL_L18N,
                                   g_param_spec_boolean ("manual-l18n", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_EMPTY_LABEL,
                                   g_param_spec_string ("empty-label", NULL, NULL,
                                                         NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_translation_store_init (ClayTranslationStore *store)
{
}

static void
clay_translation_store_constructed (GObject *object)
{
  GList *sublist = NULL;
#ifdef HAVE_ISO_CODES
  gchar *mo_path = NULL;
#endif

  if (CLAY_TRANSLATION_STORE (object)->manual_l18n)
    sublist = clay_help_get_installed_languages ();

  if (CLAY_TRANSLATION_STORE (object)->manual_l18n &&
      CLAY_TRANSLATION_STORE (object)->empty_label)
    {
      CLAY_LANGUAGE_STORE_GET_CLASS (object)->add (CLAY_LANGUAGE_STORE (object),
                                                   CLAY_TRANSLATION_STORE (object)->empty_label,
                                                   "");
    }
#ifdef HAVE_ISO_CODES
  for (gint i = 0; i < CLAY_L10N_LANGS_SIZE; i++)
    {
      ClayLanguageDef def = ClayL10nLanguages[i];

      if (! CLAY_TRANSLATION_STORE (object)->manual_l18n ||
          g_list_find_custom (sublist, def.code, (GCompareFunc) g_strcmp0))
        {
          /* there might not be all languages; only show the installed (see #15468) */
          mo_path = g_strdup_printf ("%s/share/locale/%s/LC_MESSAGES/%s.mo", clay_installation_directory(), def.code, GETTEXT_PACKAGE);
          if (g_file_test (mo_path, G_FILE_TEST_EXISTS) || g_strcmp0 (def.code, "en_US") == 0)
            {
              CLAY_LANGUAGE_STORE_GET_CLASS (object)->add (CLAY_LANGUAGE_STORE (object),
                                                           def.name, def.code);
            }
          g_free(mo_path);
        }
    }

  g_return_if_fail (system_lang != NULL);

  CLAY_LANGUAGE_STORE_GET_CLASS (object)->add (CLAY_LANGUAGE_STORE (object),
                                               system_lang, "");
#endif

  g_list_free_full (sublist, g_free);
}

static void
clay_translation_store_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayTranslationStore *store = CLAY_TRANSLATION_STORE (object);

  switch (property_id)
    {
    case PROP_MANUAL_L18N:
      store->manual_l18n = g_value_get_boolean (value);
      break;
    case PROP_EMPTY_LABEL:
      store->empty_label = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_translation_store_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayTranslationStore *store = CLAY_TRANSLATION_STORE (object);

  switch (property_id)
    {
    case PROP_MANUAL_L18N:
      g_value_set_boolean (value, store->manual_l18n);
      break;
    case PROP_EMPTY_LABEL:
      g_value_set_string (value, store->empty_label);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
clay_translation_store_initialize (const gchar *system_lang_l10n)
{
  g_return_if_fail (system_lang_l10n != NULL);

  if (system_lang != NULL)
    {
      g_warning ("clay_translation_store_initialize() must be run only once.");
      return;
    }

  system_lang = system_lang_l10n;
}

GtkListStore *
clay_translation_store_new (gboolean     manual_l18n,
                            const gchar *empty_label)
{
  return g_object_new (CLAY_TYPE_TRANSLATION_STORE,
                       "manual-l18n", manual_l18n,
                       "empty-label", empty_label,
                       NULL);
}
