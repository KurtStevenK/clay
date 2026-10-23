/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayitemcombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006 Simon Budig <simon@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"

#include "clayuitypes.h"
#include "clayitemcombobox.h"
#include "claypixbuf.h"


/**
 * SECTION: clayitemcombobox
 * @title: ClayItemComboBox
 * @short_description: Widgets providing popup menus of items.
 *
 * Widgets providing popup menus of items (layers, channels,
 * drawables, paths).
 **/


#define THUMBNAIL_SIZE  24
#define WIDTH_REQUEST  200


#define GET_PRIVATE(obj) (g_object_get_data (G_OBJECT (obj), "clay-item-combo-box-private"))


typedef struct _ClayItemComboBoxPrivate   ClayItemComboBoxPrivate;

struct _ClayItemComboBoxPrivate
{
  ClayItemConstraintFunc  constraint;
  gpointer                data;
};

typedef struct _ClayDrawableComboBoxClass ClayDrawableComboBoxClass;
typedef struct _ClayChannelComboBoxClass  ClayChannelComboBoxClass;
typedef struct _ClayLayerComboBoxClass    ClayLayerComboBoxClass;
typedef struct _ClayPathComboBoxClass     ClayPathComboBoxClass;

struct _ClayDrawableComboBox
{
  ClayIntComboBox  parent_instance;
};

struct _ClayDrawableComboBoxClass
{
  ClayIntComboBoxClass  parent_class;
};

struct _ClayChannelComboBox
{
  ClayIntComboBox  parent_instance;
};

struct _ClayChannelComboBoxClass
{
  ClayIntComboBoxClass  parent_class;
};

struct _ClayLayerComboBox
{
  ClayIntComboBox  parent_instance;
};

struct _ClayLayerComboBoxClass
{
  ClayIntComboBoxClass  parent_class;
};

struct _ClayPathComboBox
{
  ClayIntComboBox  parent_instance;
};

struct _ClayPathComboBoxClass
{
  ClayIntComboBoxClass  parent_class;
};


static GtkWidget * clay_item_combo_box_new (GType                       type,
                                            ClayItemConstraintFunc      constraint,
                                            gpointer                    data,
                                            GDestroyNotify              data_destroy);

static void  clay_item_combo_box_populate  (ClayIntComboBox            *combo_box);
static void  clay_item_combo_box_model_add (ClayIntComboBox            *combo_box,
                                            GtkListStore               *store,
                                            ClayImage                  *image,
                                            GList                      *items,
                                            gint                        tree_level);

static void  clay_item_combo_box_drag_data_received (GtkWidget         *widget,
                                                     GdkDragContext    *context,
                                                     gint               x,
                                                     gint               y,
                                                     GtkSelectionData  *selection,
                                                     guint              info,
                                                     guint              time);

static void  clay_item_combo_box_changed   (ClayIntComboBox *combo_box);


#ifndef PLATFORM_OSX
static const GtkTargetEntry targets[] =
{
  { "application/x-clay-channel-id", 0 },
  { "application/x-clay-layer-id",   0 },
  { "application/x-clay-path-id",    0 }
};
#else
/* on macOS, GDK turns any custom target containing a "/" to a UTI-like
   format, but not the other way around. So, rewrite "/" to "-" to work
   (see app/widgets/claydnd.c) */
static const GtkTargetEntry targets[] =
{
  { "application-x-clay-channel-id", 0 },
  { "application-x-clay-layer-id",   0 },
  { "application-x-clay-path-id",    0 }
};
#endif


G_DEFINE_TYPE (ClayDrawableComboBox, clay_drawable_combo_box,
               CLAY_TYPE_INT_COMBO_BOX)

static void
clay_drawable_combo_box_class_init (ClayDrawableComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = clay_item_combo_box_drag_data_received;
}

static void
clay_drawable_combo_box_init (ClayDrawableComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 2,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "clay-item-combo-box-private",
                          g_new0 (ClayItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * clay_drawable_combo_box_new:
 * @constraint: (nullable) (closure data):
 *                                A #ClayItemConstraintFunc or %NULL
 * @data:                         A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data
 *
 * Creates a new #ClayIntComboBox filled with all currently opened
 * drawables. If a @constraint function is specified, it is called for
 * each drawable and only if the function returns %TRUE, the drawable
 * is added to the combobox.
 *
 * You should use clay_int_combo_box_connect() to initialize and connect
 * the combo.  Use clay_int_combo_box_set_active() to get the active
 * drawable ID and clay_int_combo_box_get_active() to retrieve the ID
 * of the selected drawable.
 *
 * Returns: a new #ClayIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
clay_drawable_combo_box_new (ClayItemConstraintFunc constraint,
                             gpointer               data,
                             GDestroyNotify         data_destroy)
{
  return clay_item_combo_box_new (CLAY_TYPE_DRAWABLE_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (ClayChannelComboBox, clay_channel_combo_box,
               CLAY_TYPE_INT_COMBO_BOX)

static void
clay_channel_combo_box_class_init (ClayChannelComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = clay_item_combo_box_drag_data_received;
}

static void
clay_channel_combo_box_init (ClayChannelComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "clay-item-combo-box-private",
                          g_new0 (ClayItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * clay_channel_combo_box_new:
 * @constraint: (nullable) (closure data): A #ClayItemConstraintFunc or %NULL
 * @data:                                  A pointer that is passed to @constraint
 * @data_destroy: (destroy data):          Destroy function for @data
 *
 * Creates a new #ClayIntComboBox filled with all currently opened
 * channels. See clay_drawable_combo_box_new() for more information.
 *
 * Returns: a new #ClayIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
clay_channel_combo_box_new (ClayItemConstraintFunc constraint,
                            gpointer               data,
                            GDestroyNotify         data_destroy)
{
  return clay_item_combo_box_new (CLAY_TYPE_CHANNEL_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (ClayLayerComboBox, clay_layer_combo_box,
               CLAY_TYPE_INT_COMBO_BOX)

static void
clay_layer_combo_box_class_init (ClayLayerComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = clay_item_combo_box_drag_data_received;
}

static void
clay_layer_combo_box_init (ClayLayerComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 1, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "clay-item-combo-box-private",
                          g_new0 (ClayItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}

/**
 * clay_layer_combo_box_new:
 * @constraint: (nullable) (closure data): A #ClayItemConstraintFunc or %NULL
 * @data:                                  A pointer that is passed to @constraint
 * @data_destroy: (destroy data):          Destroy function for @data
 *
 * Creates a new #ClayIntComboBox filled with all currently opened
 * layers. See clay_drawable_combo_box_new() for more information.
 *
 * Returns: a new #ClayIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
clay_layer_combo_box_new (ClayItemConstraintFunc constraint,
                          gpointer               data,
                          GDestroyNotify         data_destroy)
{
  return clay_item_combo_box_new (CLAY_TYPE_LAYER_COMBO_BOX,
                                  constraint, data, data_destroy);
}


G_DEFINE_TYPE (ClayPathComboBox, clay_path_combo_box,
               CLAY_TYPE_INT_COMBO_BOX)

static void
clay_path_combo_box_class_init (ClayPathComboBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->drag_data_received = clay_item_combo_box_drag_data_received;
}

static void
clay_path_combo_box_init (ClayPathComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     targets + 2, 1,
                     GDK_ACTION_COPY);

  g_object_set_data_full (G_OBJECT (combo_box), "clay-item-combo-box-private",
                          g_new0 (ClayItemComboBoxPrivate, 1),
                          (GDestroyNotify) g_free);
}


/**
 * clay_path_combo_box_new:
 * @constraint: (nullable) (closure data): A #ClayItemConstraintFunc or %NULL
 * @data:                                  A pointer that is passed to @constraint
 * @data_destroy: (destroy data):          Destroy function for @data
 *
 * Creates a new #ClayIntComboBox filled with all currently opened
 * path objects. If a @constraint function is specified, it is called for
 * each path object and only if the function returns %TRUE, the path
 * object is added to the combobox.
 *
 * You should use clay_int_combo_box_connect() to initialize and connect
 * the combo.  Use clay_int_combo_box_set_active() to set the active
 * path ID and clay_int_combo_box_get_active() to retrieve the ID
 * of the selected path object.
 *
 * Returns: a new #ClayIntComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_path_combo_box_new (ClayItemConstraintFunc constraint,
                            gpointer               data,
                            GDestroyNotify         data_destroy)
{
  return clay_item_combo_box_new (CLAY_TYPE_PATH_COMBO_BOX,
                                  constraint, data, data_destroy);
}


static GtkWidget *
clay_item_combo_box_new (GType                  type,
                         ClayItemConstraintFunc constraint,
                         gpointer               data,
                         GDestroyNotify         data_destroy)
{
  ClayIntComboBox         *combo_box;
  ClayItemComboBoxPrivate *private;

  combo_box = g_object_new (type,
                            "width-request", WIDTH_REQUEST,
                            "ellipsize",     PANGO_ELLIPSIZE_MIDDLE,
                            NULL);

  private = GET_PRIVATE (combo_box);

  private->constraint = constraint;
  private->data       = data;

  if (data_destroy)
    g_object_weak_ref (G_OBJECT (combo_box), (GWeakNotify) data_destroy, data);

  clay_item_combo_box_populate (combo_box);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (clay_item_combo_box_changed),
                    NULL);

  return GTK_WIDGET (combo_box);
}

static void
clay_item_combo_box_populate (ClayIntComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GList        *images;
  GList        *list;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = clay_list_images ();

  for (list = images; list; list = g_list_next (list))
    {
      ClayImage *image = list->data;
      GList     *items;

      if (CLAY_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          CLAY_IS_LAYER_COMBO_BOX (combo_box))
        {
          items = clay_image_list_layers (image);
          clay_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }

      if (CLAY_IS_DRAWABLE_COMBO_BOX (combo_box) ||
          CLAY_IS_CHANNEL_COMBO_BOX (combo_box))
        {
          items = clay_image_list_channels (image);
          clay_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }

      if (CLAY_IS_PATH_COMBO_BOX (combo_box))
        {
          items = clay_image_list_paths (image);
          clay_item_combo_box_model_add (combo_box, GTK_LIST_STORE (model),
                                         image, items, 0);
          g_list_free (items);
        }
    }

  g_list_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
clay_item_combo_box_model_add (ClayIntComboBox *combo_box,
                               GtkListStore    *store,
                               ClayImage       *image,
                               GList           *items,
                               gint             tree_level)
{
  ClayItemComboBoxPrivate *private = GET_PRIVATE (combo_box);
  GtkTreeIter              iter;
  GList                   *list;
  gchar                   *indent;

  if (tree_level > 0)
    {
      indent = g_new (gchar, tree_level + 2);
      memset (indent, '-', tree_level);
      indent[tree_level] = ' ';
      indent[tree_level + 1] = '\0';
    }
  else
    {
      indent = g_strdup ("");
    }

  for (list = items; list; list = g_list_next (list))
    {
      ClayItem *item    = list->data;
      gint32    item_id = clay_item_get_id (item);

      if (! private->constraint ||
          private->constraint (image, item, private->data))
        {
          gchar     *image_name = clay_image_get_name (image);
          gchar     *item_name  = clay_item_get_name (item);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s%s-%d / %s-%d",
                                   indent, image_name,
                                   clay_image_get_id (image),
                                   item_name, item_id);

          g_free (item_name);
          g_free (image_name);

          if (CLAY_IS_PATH_COMBO_BOX (combo_box))
            thumb = NULL;
          else
            thumb = clay_drawable_get_thumbnail (CLAY_DRAWABLE (item),
                                                 THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                                 CLAY_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              CLAY_INT_STORE_VALUE,  item_id,
                              CLAY_INT_STORE_LABEL,  label,
                              CLAY_INT_STORE_PIXBUF, thumb,
                              -1);

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }

      if (clay_item_is_group (item))
        {
          GList *children;

          children = clay_item_list_children (item);
          clay_item_combo_box_model_add (combo_box, store,
                                         image, children,
                                         tree_level + 1);
          g_list_free (children);
        }
    }

  g_free (indent);
}

static void
clay_item_combo_box_drag_data_received (GtkWidget        *widget,
                                        GdkDragContext   *context,
                                        gint              x,
                                        gint              y,
                                        GtkSelectionData *selection,
                                        guint             info,
                                        guint             time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid item ID data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint pid;
      gint ID;

#ifndef _UCRT
      if (sscanf (str, "%i:%i", &pid, &ID) == 2 &&
          pid == clay_getpid ())
#else
      if (sscanf_s (str, "%i:%i", &pid, &ID) == 2 &&
          pid == clay_getpid ())
#endif
        {
          clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (widget), ID);
        }
    }

  g_free (str);
}

static gboolean
clay_item_combo_box_remove_items (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      data)
{
  gint    item_ID;
  GList **remove = data;

  gtk_tree_model_get (model, iter,
                      CLAY_INT_STORE_VALUE, &item_ID,
                      -1);

  if (item_ID > 0)
    *remove = g_list_prepend (*remove, g_memdup2 (iter, sizeof (GtkTreeIter)));

  return FALSE;
}

static void
clay_item_combo_box_changed (ClayIntComboBox *combo_box)
{
  gint item_ID;

  if (clay_int_combo_box_get_active (combo_box, &item_ID))
    {
      if (item_ID > 0 && ! clay_item_get_by_id (item_ID))
        {
          GtkTreeModel *model;
          GList        *remove = NULL;
          GList        *list;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_tree_model_foreach (model,
                                  clay_item_combo_box_remove_items,
                                  &remove);

          for (list = remove; list; list = g_list_next (list))
            gtk_list_store_remove (GTK_LIST_STORE (model), list->data);

          g_list_free_full (remove, (GDestroyNotify) g_free);

          clay_item_combo_box_populate (combo_box);
        }
    }
}
