/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayunitstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "claywidgetstypes.h"

#include "clayunitstore.h"


/**
 * SECTION: clayunitstore
 * @title: ClayUnitStore
 * @short_description: A model for units
 *
 * A model for #ClayUnit views
 **/


enum
{
  PROP_0,
  PROP_NUM_VALUES,
  PROP_HAS_PIXELS,
  PROP_HAS_PERCENT,
  PROP_SHORT_FORMAT,
  PROP_LONG_FORMAT
};

typedef struct _ClayUnitStorePrivate
{
  gint        num_values;
  gboolean    has_pixels;
  gboolean    has_percent;

  gchar      *short_format;
  gchar      *long_format;

  gdouble    *values;
  gdouble    *resolutions;

  ClayUnitID  synced_ID;
} ClayUnitStorePrivate;

#define GET_PRIVATE(obj) ((ClayUnitStorePrivate *) clay_unit_store_get_instance_private ((ClayUnitStore *) (obj)))


static void         clay_unit_store_tree_model_init (GtkTreeModelIface *iface);

static void         clay_unit_store_finalize        (GObject      *object);
static void         clay_unit_store_set_property    (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void         clay_unit_store_get_property    (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static GtkTreeModelFlags clay_unit_store_get_flags  (GtkTreeModel *tree_model);
static gint         clay_unit_store_get_n_columns   (GtkTreeModel *tree_model);
static GType        clay_unit_store_get_column_type (GtkTreeModel *tree_model,
                                                     gint          index);
static gboolean     clay_unit_store_get_iter        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreePath  *path);
static GtkTreePath *clay_unit_store_get_path        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static void    clay_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     gint          column,
                                                     GValue       *value);
static gboolean     clay_unit_store_iter_next       (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     clay_unit_store_iter_children   (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent);
static gboolean     clay_unit_store_iter_has_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gint         clay_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     clay_unit_store_iter_nth_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent,
                                                     gint          n);
static gboolean     clay_unit_store_iter_parent     (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *child);


G_DEFINE_TYPE_WITH_CODE (ClayUnitStore, clay_unit_store, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayUnitStore)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                clay_unit_store_tree_model_init))

#define parent_class clay_unit_store_parent_class


static GType column_types[CLAY_UNIT_STORE_UNIT_COLUMNS] =
{
  G_TYPE_INVALID,
  G_TYPE_DOUBLE,
  G_TYPE_INT,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
};


static void
clay_unit_store_class_init (ClayUnitStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  column_types[CLAY_UNIT_STORE_UNIT] = CLAY_TYPE_UNIT;

  object_class->finalize     = clay_unit_store_finalize;
  object_class->set_property = clay_unit_store_set_property;
  object_class->get_property = clay_unit_store_get_property;

  g_object_class_install_property (object_class, PROP_NUM_VALUES,
                                   g_param_spec_int ("num-values",
                                                     "Num Values",
                                                     "The number of values this store provides",
                                                     0, G_MAXINT, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HAS_PIXELS,
                                   g_param_spec_boolean ("has-pixels",
                                                         "Has Pixels",
                                                         "Whether the store has CLAY_UNIT_PIXELS",
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_PERCENT,
                                   g_param_spec_boolean ("has-percent",
                                                         "Has Percent",
                                                         "Whether the store has CLAY_UNIT_PERCENT",
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SHORT_FORMAT,
                                   g_param_spec_string ("short-format",
                                                        "Short Format",
                                                        "Format string for a short label",
                                                        "%a",
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_LONG_FORMAT,
                                   g_param_spec_string ("long-format",
                                                        "Long Format",
                                                        "Format string for a long label",
                                                        "%n",
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_unit_store_init (ClayUnitStore *store)
{
  ClayUnitStorePrivate *private;

  private = GET_PRIVATE (store);

  private->has_pixels   = TRUE;
  private->has_percent  = FALSE;
  private->short_format = g_strdup ("%a");
  private->long_format  = g_strdup ("%n");
  private->synced_ID    = 0;
}

static void
clay_unit_store_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = clay_unit_store_get_flags;
  iface->get_n_columns   = clay_unit_store_get_n_columns;
  iface->get_column_type = clay_unit_store_get_column_type;
  iface->get_iter        = clay_unit_store_get_iter;
  iface->get_path        = clay_unit_store_get_path;
  iface->get_value       = clay_unit_store_tree_model_get_value;
  iface->iter_next       = clay_unit_store_iter_next;
  iface->iter_children   = clay_unit_store_iter_children;
  iface->iter_has_child  = clay_unit_store_iter_has_child;
  iface->iter_n_children = clay_unit_store_iter_n_children;
  iface->iter_nth_child  = clay_unit_store_iter_nth_child;
  iface->iter_parent     = clay_unit_store_iter_parent;
}

static void
clay_unit_store_finalize (GObject *object)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->short_format, g_free);
  g_clear_pointer (&private->long_format,  g_free);

  g_clear_pointer (&private->values,      g_free);
  g_clear_pointer (&private->resolutions, g_free);
  private->num_values = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_unit_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_return_if_fail (private->num_values == 0);
      private->num_values = g_value_get_int (value);
      if (private->num_values)
        {
          private->values      = g_new0 (gdouble, private->num_values);
          private->resolutions = g_new0 (gdouble, private->num_values);
        }
      break;
    case PROP_HAS_PIXELS:
      clay_unit_store_set_has_pixels (CLAY_UNIT_STORE (object),
                                      g_value_get_boolean (value));
      break;
    case PROP_HAS_PERCENT:
      clay_unit_store_set_has_percent (CLAY_UNIT_STORE (object),
                                       g_value_get_boolean (value));
      break;
    case PROP_SHORT_FORMAT:
      g_free (private->short_format);
      private->short_format = g_value_dup_string (value);
      if (! private->short_format)
        private->short_format = g_strdup ("%a");
      break;
    case PROP_LONG_FORMAT:
      g_free (private->long_format);
      private->long_format = g_value_dup_string (value);
      if (! private->long_format)
        private->long_format = g_strdup ("%a");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_unit_store_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_value_set_int (value, private->num_values);
      break;
    case PROP_HAS_PIXELS:
      g_value_set_boolean (value, private->has_pixels);
      break;
    case PROP_HAS_PERCENT:
      g_value_set_boolean (value, private->has_percent);
      break;
    case PROP_SHORT_FORMAT:
      g_value_set_string (value, private->short_format);
      break;
    case PROP_LONG_FORMAT:
      g_value_set_string (value, private->long_format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkTreeModelFlags
clay_unit_store_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

static gint
clay_unit_store_get_n_columns (GtkTreeModel *tree_model)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);

  return CLAY_UNIT_STORE_UNIT_COLUMNS + private->num_values;
}

static GType
clay_unit_store_get_column_type (GtkTreeModel *tree_model,
                                 gint          index)
{
  g_return_val_if_fail (index >= 0, G_TYPE_INVALID);

  if (index < CLAY_UNIT_STORE_UNIT_COLUMNS)
    return column_types[index];

  return G_TYPE_DOUBLE;
}

static gboolean
clay_unit_store_get_iter (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter,
                          GtkTreePath  *path)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  index;
  ClayUnit             *unit    = NULL;

  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  index = gtk_tree_path_get_indices (path)[0];

  if (! private->has_pixels)
    index++;
  else if (index == 0)
    unit = clay_unit_pixel ();

  if (unit == NULL && private->has_percent)
    {
      if (private->has_pixels)
        {
          if (index == 1)
            unit = clay_unit_percent ();
        }
      else if (index == 0)
        {
          unit = clay_unit_percent ();
        }
      index--;
    }

  if (unit == NULL)
    unit = clay_unit_get_by_id (index);

  if (unit != NULL)
    {
      iter->user_data = unit;
      private->synced_ID = MAX (private->synced_ID, clay_unit_get_id (unit));
    }

  return (unit != NULL);
}

static GtkTreePath *
clay_unit_store_get_path (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GtkTreePath          *path    = gtk_tree_path_new ();
  ClayUnit             *unit    = iter->user_data;
  gint                  index;

  index = clay_unit_get_id (unit);

  if (! private->has_pixels)
    index--;

  if (private->has_percent)
    {
      index++;

      if (private->has_pixels)
        {
          if (unit == clay_unit_pixel ())
            index = 0;
          else if (unit == clay_unit_percent ())
            index = 1;
        }
      else
        {
          if (unit == clay_unit_percent ())
            index = 0;
        }
    }

  gtk_tree_path_append_index (path, index);

  return path;
}

static void
clay_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                      GtkTreeIter  *iter,
                                      gint          column,
                                      GValue       *value)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  ClayUnit             *unit;

  g_return_if_fail (column >= 0 &&
                    column < CLAY_UNIT_STORE_UNIT_COLUMNS + private->num_values);

  g_value_init (value,
                column < CLAY_UNIT_STORE_UNIT_COLUMNS ?
                column_types[column] :
                G_TYPE_DOUBLE);

  unit = iter->user_data;

  switch (column)
    {
    case CLAY_UNIT_STORE_UNIT:
      g_value_set_object (value, unit);
      break;
    case CLAY_UNIT_STORE_UNIT_FACTOR:
      g_value_set_double (value, clay_unit_get_factor (unit));
      break;
    case CLAY_UNIT_STORE_UNIT_DIGITS:
      g_value_set_int (value, clay_unit_get_digits (unit));
      break;
    case CLAY_UNIT_STORE_UNIT_NAME:
      g_value_set_static_string (value, clay_unit_get_name (unit));
      break;
    case CLAY_UNIT_STORE_UNIT_SYMBOL:
      g_value_set_static_string (value, clay_unit_get_symbol (unit));
      break;
    case CLAY_UNIT_STORE_UNIT_ABBREVIATION:
      g_value_set_static_string (value, clay_unit_get_abbreviation (unit));
      break;
    case CLAY_UNIT_STORE_UNIT_SHORT_FORMAT:
      g_value_take_string (value,
                           clay_unit_format_string (private->short_format,
                                                    unit));
      break;
    case CLAY_UNIT_STORE_UNIT_LONG_FORMAT:
      g_value_take_string (value,
                           clay_unit_format_string (private->long_format,
                                                    unit));
      break;

    default:
      column -= CLAY_UNIT_STORE_UNIT_COLUMNS;
      if (unit == clay_unit_pixel ())
        {
          g_value_set_double (value, private->values[column]);
        }
      else if (private->resolutions[column])
        {
          g_value_set_double (value,
                              private->values[column] *
                              clay_unit_get_factor (unit) /
                              private->resolutions[column]);
        }
      break;
    }
}

static gboolean
clay_unit_store_iter_next (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  ClayUnit             *unit    = iter->user_data;

  if (unit == clay_unit_pixel () && private->has_percent)
    unit = clay_unit_percent ();
  else if (unit == clay_unit_percent () || unit == clay_unit_pixel ())
    unit = clay_unit_inch ();
  else
    unit = clay_unit_get_by_id (clay_unit_get_id (unit) + 1);

  iter->user_data = unit;

  return (unit != NULL);
}

static gboolean
clay_unit_store_iter_children (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter,
                               GtkTreeIter  *parent)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  ClayUnit             *unit;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  if (private->has_pixels)
    unit = clay_unit_pixel ();
  else if (private->has_percent)
    unit = clay_unit_percent ();
  else
    unit = clay_unit_inch ();

  iter->user_data = unit;

  return TRUE;
}

static gboolean
clay_unit_store_iter_has_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter)
{
  return FALSE;
}

static gint
clay_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
  ClayUnitStorePrivate *private    = GET_PRIVATE (tree_model);
  ClayUnit             *unit;
  gint                  n_children = CLAY_UNIT_END;

  if (iter)
    return 0;

  do
    unit = clay_unit_get_by_id (n_children++);
  while (unit != NULL);

  n_children--;

  if (! private->has_pixels)
    n_children--;

  if (private->has_percent)
    n_children++;

  return n_children;
}

static gboolean
clay_unit_store_iter_nth_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent,
                                gint          n)
{
  ClayUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  n_children;

  if (parent)
    return FALSE;

  n_children = clay_unit_store_iter_n_children (tree_model, NULL);

  if (n >= 0 && n < n_children)
    {
      gint index = n;

      if (! private->has_pixels)
        index++;

      if (private->has_percent)
        {
          index--;

          if (private->has_pixels)
            {
              if (n == 0)
                index = CLAY_UNIT_PIXEL;
              else if (n == 1)
                index = CLAY_UNIT_PERCENT;
            }
          else
            {
              if (n == 0)
                index = CLAY_UNIT_PERCENT;
            }
        }

      iter->user_data = clay_unit_get_by_id (index);

      return TRUE;
    }

  return FALSE;
}

static gboolean
clay_unit_store_iter_parent (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter,
                             GtkTreeIter  *child)
{
  return FALSE;
}


ClayUnitStore *
clay_unit_store_new (gint  num_values)
{
  return g_object_new (CLAY_TYPE_UNIT_STORE,
                       "num-values", num_values,
                       NULL);
}

void
clay_unit_store_set_has_pixels (ClayUnitStore *store,
                                gboolean       has_pixels)
{
  ClayUnitStorePrivate *private;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_pixels = has_pixels ? TRUE : FALSE;

  if (has_pixels != private->has_pixels)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_pixels)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_pixels = has_pixels;

      if (has_pixels)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-pixels");
    }
}

gboolean
clay_unit_store_get_has_pixels (ClayUnitStore *store)
{
  ClayUnitStorePrivate *private;

  g_return_val_if_fail (CLAY_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_pixels;
}

void
clay_unit_store_set_has_percent (ClayUnitStore *store,
                                 gboolean       has_percent)
{
  ClayUnitStorePrivate *private;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_percent = has_percent ? TRUE : FALSE;

  if (has_percent != private->has_percent)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_percent)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_percent = has_percent;

      if (has_percent)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-percent");
    }
}

gboolean
clay_unit_store_get_has_percent (ClayUnitStore *store)
{
  ClayUnitStorePrivate *private;

  g_return_val_if_fail (CLAY_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_percent;
}

void
clay_unit_store_set_pixel_value (ClayUnitStore *store,
                                 gint           index,
                                 gdouble        value)
{
  ClayUnitStorePrivate *private;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->values[index] = value;
}

void
clay_unit_store_set_pixel_values (ClayUnitStore *store,
                                  gdouble        first_value,
                                  ...)
{
  ClayUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      private->values[i] = first_value;

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble);
    }

  va_end (args);
}

void
clay_unit_store_set_resolution (ClayUnitStore *store,
                                gint           index,
                                gdouble        resolution)
{
  ClayUnitStorePrivate *private;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->resolutions[index] = resolution;
}

void
clay_unit_store_set_resolutions  (ClayUnitStore *store,
                                  gdouble        first_resolution,
                                  ...)
{
  ClayUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_resolution);

  for (i = 0; i < private->num_values; )
    {
      private->resolutions[i] = first_resolution;

      if (++i < private->num_values)
        first_resolution = va_arg (args, gdouble);
    }

  va_end (args);
}

gdouble
clay_unit_store_get_nth_value (ClayUnitStore *store,
                               ClayUnit      *unit,
                               gint           index)
{
  ClayUnitStorePrivate *private;
  GtkTreeIter          iter;
  GValue               value = G_VALUE_INIT;

  g_return_val_if_fail (CLAY_IS_UNIT_STORE (store), 0.0);

  private = GET_PRIVATE (store);

  g_return_val_if_fail (index >= 0 && index < private->num_values, 0.0);

  iter.user_data = unit;

  clay_unit_store_tree_model_get_value (GTK_TREE_MODEL (store),
                                        &iter,
                                        CLAY_UNIT_STORE_FIRST_VALUE + index,
                                        &value);

  return g_value_get_double (&value);
}

void
clay_unit_store_get_values (ClayUnitStore *store,
                            ClayUnit      *unit,
                            gdouble       *first_value,
                            ...)
{
  ClayUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      if (first_value)
        *first_value = clay_unit_store_get_nth_value (store, unit, i);

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble *);
    }

  va_end (args);
}

void
_clay_unit_store_sync_units (ClayUnitStore *store)
{
  ClayUnitStorePrivate *private;
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;

  g_return_if_fail (CLAY_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);
  model   = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      ClayUnit *unit;

      gtk_tree_model_get (model, &iter,
                          CLAY_UNIT_STORE_UNIT, &unit,
                          -1);

      if (unit != clay_unit_percent () &&
          clay_unit_get_id (unit) > private->synced_ID)
        {
          GtkTreePath *path;

          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }

      g_object_unref (unit);
    }
}
