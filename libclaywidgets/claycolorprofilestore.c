/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayprofilestore.c
 * Copyright (C) 2004-2008  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "claywidgetstypes.h"

#include "claycolorprofilestore.h"
#include "claycolorprofilestore-private.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claycolorprofilestore
 * @title: ClayColorProfileStore
 * @short_description: A #GtkListStore subclass that keep color profiles.
 *
 * A #GtkListStore subclass that keep color profiles.
 **/


#define HISTORY_SIZE  8

enum
{
  PROP_0,
  PROP_HISTORY
};


struct _ClayColorProfileStore
{
  GtkListStore  parent_instance;

  GFile        *history;
};


static void      clay_color_profile_store_constructed    (GObject               *object);
static void      clay_color_profile_store_dispose        (GObject               *object);
static void      clay_color_profile_store_finalize       (GObject               *object);
static void      clay_color_profile_store_set_property   (GObject               *object,
                                                          guint                  property_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void      clay_color_profile_store_get_property   (GObject               *object,
                                                          guint                  property_id,
                                                          GValue                *value,
                                                          GParamSpec            *pspec);

static gboolean  clay_color_profile_store_history_insert (ClayColorProfileStore *store,
                                                          GtkTreeIter           *iter,
                                                          GFile                 *file,
                                                          const gchar           *label,
                                                          gint                   index);
static void      clay_color_profile_store_get_separator  (ClayColorProfileStore  *store,
                                                          GtkTreeIter            *iter,
                                                          gboolean                top);
static gboolean  clay_color_profile_store_save           (ClayColorProfileStore  *store,
                                                          GFile                  *file,
                                                          GError                **error);
static gboolean  clay_color_profile_store_load           (ClayColorProfileStore  *store,
                                                          GFile                  *file,
                                                          GError                **error);


G_DEFINE_TYPE (ClayColorProfileStore, clay_color_profile_store, GTK_TYPE_LIST_STORE)

#define parent_class clay_color_profile_store_parent_class


static void
clay_color_profile_store_class_init (ClayColorProfileStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_color_profile_store_constructed;
  object_class->dispose      = clay_color_profile_store_dispose;
  object_class->finalize     = clay_color_profile_store_finalize;
  object_class->set_property = clay_color_profile_store_set_property;
  object_class->get_property = clay_color_profile_store_get_property;

  /**
   * ClayColorProfileStore:history:
   *
   * #GFile of the color history used to populate the profile store.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_HISTORY,
                                   g_param_spec_object ("history",
                                                        "History",
                                                        "Filen of the color history used to populate the profile store",
                                                        G_TYPE_FILE,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_color_profile_store_init (ClayColorProfileStore *store)
{
  GType types[] =
    {
      G_TYPE_INT,    /*  CLAY_COLOR_PROFILE_STORE_ITEM_TYPE  */
      G_TYPE_STRING, /*  CLAY_COLOR_PROFILE_STORE_LABEL      */
      G_TYPE_FILE,   /*  CLAY_COLOR_PROFILE_STORE_FILE       */
      G_TYPE_INT     /*  CLAY_COLOR_PROFILE_STORE_INDEX      */
    };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);
}

static void
clay_color_profile_store_constructed (GObject *object)
{
  ClayColorProfileStore *store = CLAY_COLOR_PROFILE_STORE (object);
  GtkTreeIter            iter;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      CLAY_COLOR_PROFILE_STORE_ITEM_TYPE,
                      CLAY_COLOR_PROFILE_STORE_ITEM_DIALOG,
                      CLAY_COLOR_PROFILE_STORE_LABEL,
                      _("Select color profile from disk..."),
                      -1);

  if (store->history)
    clay_color_profile_store_load (store, store->history, NULL);
}

static void
clay_color_profile_store_dispose (GObject *object)
{
  ClayColorProfileStore *store = CLAY_COLOR_PROFILE_STORE (object);

  if (store->history)
    clay_color_profile_store_save (store, store->history, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_color_profile_store_finalize (GObject *object)
{
  ClayColorProfileStore *store = CLAY_COLOR_PROFILE_STORE (object);

  g_clear_object (&store->history);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_profile_store_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayColorProfileStore *store = CLAY_COLOR_PROFILE_STORE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_return_if_fail (store->history == NULL);
      store->history = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_profile_store_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayColorProfileStore *store = CLAY_COLOR_PROFILE_STORE (object);

  switch (property_id)
    {
    case PROP_HISTORY:
      g_value_set_object (value, store->history);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * clay_color_profile_store_new:
 * @history: #GFile of the profilerc (or %NULL for no history)
 *
 * Creates a new #ClayColorProfileStore object and populates it with
 * last used profiles read from the file @history. The updated history
 * is written back to disk when the store is disposed.
 *
 * The #GFile passed as @history is typically created using the
 * following code snippet:
 * <informalexample><programlisting>
 *  gchar *history = clay_personal_rc_file ("profilerc");
 * </programlisting></informalexample>
 *
 * Returns: a new #ClayColorProfileStore
 *
 * Since: 2.4
 **/
GtkListStore *
clay_color_profile_store_new (GFile *history)
{
  g_return_val_if_fail (history == NULL || G_IS_FILE (history), NULL);

  return g_object_new (CLAY_TYPE_COLOR_PROFILE_STORE,
                       "history", history,
                       NULL);
}

/**
 * clay_color_profile_store_add_file:
 * @store: a #ClayColorProfileStore
 * @file:  #GFile of the profile to add (or %NULL)
 * @label: label to use for the profile
 *         (may only be %NULL if @file is %NULL)
 *
 * Adds a color profile item to the #ClayColorProfileStore. Items
 * added with this function will be kept at the top, separated from
 * the history of last used color profiles.
 *
 * This function is often used to add a selectable item for the %NULL
 * file. If you pass %NULL for both @file and @label, the @label will
 * be set to the string "None" for you (and translated for the user).
 *
 * Since: 2.10
 **/
void
clay_color_profile_store_add_file (ClayColorProfileStore *store,
                                   GFile                 *file,
                                   const gchar           *label)
{
  GtkTreeIter separator;
  GtkTreeIter iter;

  g_return_if_fail (CLAY_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (label != NULL || file == NULL);
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (! file && ! label)
    label = C_("profile", "None");

  clay_color_profile_store_get_separator (store, &separator, TRUE);

  gtk_list_store_insert_before (GTK_LIST_STORE (store), &iter, &separator);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      CLAY_COLOR_PROFILE_STORE_ITEM_TYPE,
                      CLAY_COLOR_PROFILE_STORE_ITEM_FILE,
                      CLAY_COLOR_PROFILE_STORE_FILE,  file,
                      CLAY_COLOR_PROFILE_STORE_LABEL, label,
                      CLAY_COLOR_PROFILE_STORE_INDEX, -1,
                      -1);
}

/**
 * _clay_color_profile_store_history_add:
 * @store: a #ClayColorProfileStore
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile (or %NULL)
 * @iter:  a #GtkTreeIter
 *
 * Returns: %TRUE if the iter is valid and pointing to the item
 *
 * Since: 2.4
 **/
gboolean
_clay_color_profile_store_history_add (ClayColorProfileStore *store,
                                       GFile                 *file,
                                       const gchar           *label,
                                       GtkTreeIter           *iter)
{
  GtkTreeModel *model;
  gboolean      iter_valid;
  gint          max = -1;

  g_return_val_if_fail (CLAY_IS_COLOR_PROFILE_STORE (store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint   type;
      gint   index;
      GFile *this;

      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_INDEX,     &index,
                          -1);

      if (type != CLAY_COLOR_PROFILE_STORE_ITEM_FILE)
        continue;

      if (index > max)
        max = index;

      /*  check if we found a filename match  */
      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_FILE, &this,
                          -1);

      if ((this && file && g_file_equal (this, file)) ||
          (! this && ! file))
        {
          /*  update the label  */
          if (label && *label)
            gtk_list_store_set (GTK_LIST_STORE (store), iter,
                                CLAY_COLOR_PROFILE_STORE_LABEL, label,
                                -1);

          if (this)
            g_object_unref (this);

          return TRUE;
        }

      if (this)
        g_object_unref (this);
    }

  if (! file)
    return FALSE;

  if (label && *label)
    {
      iter_valid = clay_color_profile_store_history_insert (store, iter,
                                                            file, label,
                                                            ++max);
    }
  else
    {
      const gchar *utf8     = clay_file_get_utf8_name (file);
      gchar       *basename = g_path_get_basename (utf8);

      iter_valid = clay_color_profile_store_history_insert (store, iter,
                                                            file, basename,
                                                            ++max);
      g_free (basename);
    }

  return iter_valid;
}

/**
 * _clay_color_profile_store_history_find_profile:
 * @store:   a #ClayColorProfileStore
 * @profile: a #ClayColorProfile to find (or %NULL)
 * @iter:    a #GtkTreeIter
 *
 * Returns: %TRUE if the iter is valid and pointing to the item
 *
 * Since: 3.0
 **/
gboolean
_clay_color_profile_store_history_find_profile (ClayColorProfileStore *store,
                                                ClayColorProfile      *profile,
                                                GtkTreeIter           *iter)
{
  GtkTreeModel *model;
  gboolean      iter_valid;
  gint          max = -1;

  g_return_val_if_fail (CLAY_IS_COLOR_PROFILE_STORE (store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint              type;
      gint              index;
      GFile            *file;
      ClayColorProfile *combo_profile = NULL;

      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_INDEX,     &index,
                          -1);

      if (type != CLAY_COLOR_PROFILE_STORE_ITEM_FILE)
        continue;

      if (index > max)
        max = index;

      /*  check if we found a filename match  */
      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_FILE, &file,
                          -1);

      /* Convert file to ClayColorProfile */
      if (file)
        combo_profile = clay_color_profile_new_from_file (file, NULL);

      if ((combo_profile && profile &&
           clay_color_profile_is_equal  (profile, combo_profile)) ||
          (! file && ! profile))
        {
          if (file)
            g_object_unref (file);
          if (combo_profile)
            g_object_unref (combo_profile);

          return TRUE;
        }

      if (file)
        g_object_unref (file);
      if (combo_profile)
        g_object_unref (combo_profile);
    }

  if (! profile)
    return FALSE;

  return iter_valid;
}

/**
 * _clay_color_profile_store_history_reorder
 * @store: a #ClayColorProfileStore
 * @iter:  a #GtkTreeIter
 *
 * Moves the entry pointed to by @iter to the front of the MRU list.
 *
 * Since: 2.4
 **/
void
_clay_color_profile_store_history_reorder (ClayColorProfileStore *store,
                                           GtkTreeIter           *iter)
{
  GtkTreeModel *model;
  gint          index;
  gboolean      iter_valid;

  g_return_if_fail (CLAY_IS_COLOR_PROFILE_STORE (store));
  g_return_if_fail (iter != NULL);

  model = GTK_TREE_MODEL (store);

  gtk_tree_model_get (model, iter,
                      CLAY_COLOR_PROFILE_STORE_INDEX, &index,
                      -1);

  if (index == 0)
    return;  /* already at the top */

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint type;
      gint this_index;

      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_INDEX,     &this_index,
                          -1);

      if (type == CLAY_COLOR_PROFILE_STORE_ITEM_FILE && this_index > -1)
        {
          if (this_index < index)
            {
              this_index++;
            }
          else if (this_index == index)
            {
              this_index = 0;
            }

          gtk_list_store_set (GTK_LIST_STORE (store), iter,
                              CLAY_COLOR_PROFILE_STORE_INDEX, this_index,
                              -1);
        }
    }
}

static gboolean
clay_color_profile_store_history_insert (ClayColorProfileStore *store,
                                         GtkTreeIter           *iter,
                                         GFile                 *file,
                                         const gchar           *label,
                                         gint                   index)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   sibling;
  gboolean      iter_valid;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (label != NULL, FALSE);
  g_return_val_if_fail (index > -1, FALSE);

  clay_color_profile_store_get_separator (store, iter, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &sibling))
    {
      gint type;
      gint this_index;

      gtk_tree_model_get (model, &sibling,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_INDEX,     &this_index,
                          -1);

      if (type == CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM)
        {
          gtk_list_store_insert_before (GTK_LIST_STORE (store),
                                        iter, &sibling);
          break;
        }

      if (type == CLAY_COLOR_PROFILE_STORE_ITEM_FILE && this_index > -1)
        {
          gchar *this_label;

          gtk_tree_model_get (model, &sibling,
                              CLAY_COLOR_PROFILE_STORE_LABEL, &this_label,
                              -1);

          if (this_label && g_utf8_collate (label, this_label) < 0)
            {
              gtk_list_store_insert_before (GTK_LIST_STORE (store),
                                            iter, &sibling);
              g_free (this_label);
              break;
            }

          g_free (this_label);
        }
    }

  if (iter_valid)
    gtk_list_store_set (GTK_LIST_STORE (store), iter,
                        CLAY_COLOR_PROFILE_STORE_ITEM_TYPE,
                        CLAY_COLOR_PROFILE_STORE_ITEM_FILE,
                        CLAY_COLOR_PROFILE_STORE_FILE,  file,
                        CLAY_COLOR_PROFILE_STORE_LABEL, label,
                        CLAY_COLOR_PROFILE_STORE_INDEX, index,
                        -1);

  return iter_valid;
}

static void
clay_color_profile_store_create_separator (ClayColorProfileStore *store,
                                           GtkTreeIter           *iter,
                                           gboolean               top)
{
  if (top)
    {
      gtk_list_store_prepend (GTK_LIST_STORE (store), iter);
    }
  else
    {
      GtkTreeModel *model = GTK_TREE_MODEL (store);
      GtkTreeIter   sibling;
      gboolean      iter_valid;

      for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &sibling))
        {
          gint type;

          gtk_tree_model_get (model, &sibling,
                              CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                              -1);

          if (type == CLAY_COLOR_PROFILE_STORE_ITEM_DIALOG)
            break;
        }

      if (iter_valid)
        gtk_list_store_insert_before (GTK_LIST_STORE (store), iter, &sibling);
    }

  gtk_list_store_set (GTK_LIST_STORE (store), iter,
                      CLAY_COLOR_PROFILE_STORE_ITEM_TYPE,
                      top ?
                      CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP :
                      CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM,
                      CLAY_COLOR_PROFILE_STORE_INDEX, -1,
                      -1);
}

static void
clay_color_profile_store_get_separator (ClayColorProfileStore *store,
                                        GtkTreeIter           *iter,
                                        gboolean               top)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  gboolean      iter_valid;
  gint          needle;

  needle = (top ?
            CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP :
            CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint type;

      gtk_tree_model_get (model, iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          -1);

      if (type == needle)
        return;
    }

  clay_color_profile_store_create_separator (store, iter, top);
}

static GTokenType
clay_color_profile_store_load_profile (ClayColorProfileStore *store,
                                       GScanner              *scanner,
                                       gint                   index)
{
  GtkTreeIter  iter;
  gchar       *label = NULL;
  gchar       *path  = NULL;

  if (clay_scanner_parse_string (scanner, &label) &&
      clay_scanner_parse_string (scanner, &path))
    {
      GFile *file = NULL;

      if (g_str_has_prefix (path, "file://"))
        {
          file = g_file_new_for_uri (path);
        }
      else
        {
          file = clay_file_new_for_config_path (path, NULL);
        }

      if (file)
        {
          if (g_file_query_file_type (file, 0, NULL) == G_FILE_TYPE_REGULAR)
            {
              clay_color_profile_store_history_insert (store, &iter,
                                                       file, label, index);
            }

          g_object_unref (file);
        }

      g_free (label);
      g_free (path);

      return G_TOKEN_RIGHT_PAREN;
    }

  g_free (label);
  g_free (path);

  return G_TOKEN_STRING;
}

static gboolean
clay_color_profile_store_load (ClayColorProfileStore  *store,
                               GFile                  *file,
                               GError                **error)
{
  GScanner   *scanner;
  GTokenType  token;
  gint        i = 0;

  scanner = clay_scanner_new_file (file, error);
  if (! scanner)
    return FALSE;

  g_scanner_scope_add_symbol (scanner, 0, "color-profile", NULL);

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          token = clay_color_profile_store_load_profile (store, scanner, i++);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  clay_scanner_unref (scanner);

  return TRUE;
}

static gboolean
clay_color_profile_store_save (ClayColorProfileStore  *store,
                               GFile                  *file,
                               GError                **error)
{
  ClayConfigWriter *writer;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar            *labels[HISTORY_SIZE] = { NULL, };
  GFile            *files[HISTORY_SIZE]  = { NULL, };
  gboolean          iter_valid;
  gint              i;

  writer = clay_config_writer_new_from_file (file,
                                             TRUE,
                                             "Clay color profile history",
                                             error);
  if (! writer)
    return FALSE;

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint type;
      gint index;

      gtk_tree_model_get (model, &iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_INDEX,     &index,
                          -1);

      if (type == CLAY_COLOR_PROFILE_STORE_ITEM_FILE &&
          index >= 0                                 &&
          index < HISTORY_SIZE)
        {
          if (labels[index] || files[index])
            g_warning ("%s: double index %d", G_STRFUNC, index);

          gtk_tree_model_get (model, &iter,
                              CLAY_COLOR_PROFILE_STORE_LABEL,
                              &labels[index],
                              CLAY_COLOR_PROFILE_STORE_FILE,
                              &files[index],
                              -1);
        }
    }


  for (i = 0; i < HISTORY_SIZE; i++)
    {
      if (files[i] && labels[i])
        {
          gchar *path = clay_file_get_config_path (files[i], NULL);

          if (path)
            {
              clay_config_writer_open (writer, "color-profile");
              clay_config_writer_string (writer, labels[i]);
              clay_config_writer_string (writer, path);
              clay_config_writer_close (writer);

              g_free (path);
            }
        }

      if (files[i])
        g_object_unref (files[i]);

      g_free (labels[i]);
    }

  return clay_config_writer_finish (writer,
                                    "end of color profile history", error);
}
