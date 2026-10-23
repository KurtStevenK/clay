/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylanguagestore.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "claylanguagestore.h"
#ifdef HAVE_ISO_CODES
#include "claylanguagestore-data.h"
#endif

#include "clay-intl.h"


static void   clay_language_store_constructed (GObject           *object);

static void   clay_language_store_real_add    (ClayLanguageStore *store,
                                               const gchar       *label,
                                               const gchar       *code);

static gint   clay_language_store_sort        (GtkTreeModel      *model,
                                               GtkTreeIter       *a,
                                               GtkTreeIter       *b,
                                               gpointer           userdata);


G_DEFINE_TYPE (ClayLanguageStore, clay_language_store, GTK_TYPE_LIST_STORE)

#define parent_class clay_language_store_parent_class


static void
clay_language_store_class_init (ClayLanguageStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = clay_language_store_constructed;

  klass->add                = clay_language_store_real_add;
}

static void
clay_language_store_init (ClayLanguageStore *store)
{
  GType column_types[2] = { G_TYPE_STRING, G_TYPE_STRING };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (column_types), column_types);

  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
                                   CLAY_LANGUAGE_STORE_LABEL,
                                   clay_language_store_sort, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        CLAY_LANGUAGE_STORE_LABEL,
                                        GTK_SORT_ASCENDING);
}

static void
clay_language_store_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

#ifdef HAVE_ISO_CODES
  for (gint i = 0; i < CLAY_ALL_LANGS_SIZE; i++)
    {
      ClayLanguageDef  def            = ClayAllLanguages[i];
      gchar           *localized_name = g_strdup (dgettext ("iso_639_3", def.name));
      gchar           *semicolon;

      /*  there might be several language names; use the first one  */
      semicolon = strchr (localized_name, ';');

      if (semicolon)
        {
          gchar *temp = localized_name;

          localized_name = g_strndup (localized_name, semicolon - localized_name);
          g_free (temp);
        }

      CLAY_LANGUAGE_STORE_GET_CLASS (object)->add (CLAY_LANGUAGE_STORE (object),
                                                   localized_name, def.code);
      g_free (localized_name);
    }
#endif
}

static void
clay_language_store_real_add (ClayLanguageStore *store,
                              const gchar       *label,
                              const gchar       *code)
{
  GtkTreeIter iter;

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      CLAY_LANGUAGE_STORE_LABEL, label,
                      CLAY_LANGUAGE_STORE_CODE,  code,
                      -1);
}

static gint
clay_language_store_sort (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata)
{
  GValue avalue = G_VALUE_INIT;
  GValue bvalue = G_VALUE_INIT;
  gint   cmp    = 0;

  /*  keep system language at the top of the list  */
  gtk_tree_model_get_value (model, a, CLAY_LANGUAGE_STORE_CODE, &avalue);
  gtk_tree_model_get_value (model, b, CLAY_LANGUAGE_STORE_CODE, &bvalue);

  if (g_strcmp0 ("", g_value_get_string (&avalue)) == 0)
    cmp = -1;

  if (g_strcmp0 ("", g_value_get_string (&bvalue)) == 0)
    cmp = 1;

  g_value_unset (&avalue);
  g_value_unset (&bvalue);

  if (cmp)
    return cmp;

  /*  sort labels alphabetically  */
  gtk_tree_model_get_value (model, a, CLAY_LANGUAGE_STORE_LABEL, &avalue);
  gtk_tree_model_get_value (model, b, CLAY_LANGUAGE_STORE_LABEL, &bvalue);

  cmp = g_utf8_collate (g_value_get_string (&avalue),
                        g_value_get_string (&bvalue));

  g_value_unset (&avalue);
  g_value_unset (&bvalue);

  return cmp;
}

GtkListStore *
clay_language_store_new (void)
{
  return g_object_new (CLAY_TYPE_LANGUAGE_STORE, NULL);
}

gboolean
clay_language_store_lookup (ClayLanguageStore *store,
                            const gchar       *code,
                            GtkTreeIter       *iter)
{
  GtkTreeModel *model;
  const gchar  *hyphen;
  gint          len;
  gboolean      iter_valid;

  g_return_val_if_fail (CLAY_IS_LANGUAGE_STORE (store), FALSE);
  g_return_val_if_fail (code != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  /*  We accept the code in RFC-3066 format here and only look at what's
   *  before the first hyphen.
   */
  hyphen = strchr (code, '-');

  if (hyphen)
    len = hyphen - code;
  else
    len = strlen (code);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gchar *value;

      gtk_tree_model_get (model, iter,
                          CLAY_LANGUAGE_STORE_CODE, &value,
                          -1);

      if (value && strncmp (code, value, len) == 0)
        {
          g_free (value);
          break;
        }

      g_free (value);
    }

  return iter_valid;
}
