/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayimagecombobox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"

#include "clayuitypes.h"
#include "clayimagecombobox.h"
#include "claypixbuf.h"


/**
 * SECTION: clayimagecombobox
 * @title: ClayImageComboBox
 * @short_description: A widget providing a popup menu of images.
 *
 * A widget providing a popup menu of images.
 **/


#define THUMBNAIL_SIZE   24
#define WIDTH_REQUEST   200


struct _ClayImageComboBox
{
  ClayIntComboBox          parent_instance;

  ClayImageConstraintFunc  constraint;
  gpointer                 data;
  GDestroyNotify           data_destroy;
};


static void  clay_image_combo_box_finalize  (GObject                 *object);

static void  clay_image_combo_box_populate  (ClayImageComboBox       *combo_box);
static void  clay_image_combo_box_model_add (GtkListStore            *store,
                                             GList                   *images,
                                             ClayImageConstraintFunc  constraint,
                                             gpointer                 data);

static void  clay_image_combo_box_drag_data_received (GtkWidget        *widget,
                                                      GdkDragContext   *context,
                                                      gint              x,
                                                      gint              y,
                                                      GtkSelectionData *selection,
                                                      guint             info,
                                                      guint             time);

static void  clay_image_combo_box_changed   (ClayImageComboBox *combo_box);


#ifndef PLATFORM_OSX
static const GtkTargetEntry target = { "application/x-clay-image-id", 0 };
#else
/* on macOS, GDK turns any custom target containing a "/" to a UTI-like
   format, but not the other way around. So, rewrite "/" to "-" to work
   (see app/widgets/claydnd.c) */
static const GtkTargetEntry target = { "application-x-clay-image-id", 0 };
#endif


G_DEFINE_TYPE (ClayImageComboBox, clay_image_combo_box, CLAY_TYPE_INT_COMBO_BOX)

#define parent_class clay_image_combo_box_parent_class


static void
clay_image_combo_box_class_init (ClayImageComboBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize           = clay_image_combo_box_finalize;

  widget_class->drag_data_received = clay_image_combo_box_drag_data_received;
}

static void
clay_image_combo_box_init (ClayImageComboBox *combo_box)
{
  gtk_drag_dest_set (GTK_WIDGET (combo_box),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);
}

static void
clay_image_combo_box_finalize (GObject *object)
{
  ClayImageComboBox *combo = CLAY_IMAGE_COMBO_BOX (object);

  if (combo->data_destroy)
    combo->data_destroy (combo->data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * clay_image_combo_box_new:
 * @constraint: (nullable) (closure data):
 *                                A #ClayImageConstraintFunc or %NULL
 * @data:                         A pointer that is passed to @constraint
 * @data_destroy: (destroy data): Destroy function for @data.
 *
 * Creates a new #ClayIntComboBox filled with all currently opened
 * images. If a @constraint function is specified, it is called for
 * each image and only if the function returns %TRUE, the image is
 * added to the combobox.
 *
 * You should use clay_int_combo_box_connect() to initialize and
 * connect the combo. Use clay_int_combo_box_set_active() to get the
 * active image ID and clay_int_combo_box_get_active() to retrieve the
 * ID of the selected image.
 *
 * Returns: a new #ClayIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
clay_image_combo_box_new (ClayImageConstraintFunc constraint,
                          gpointer                data,
                          GDestroyNotify          data_destroy)
{
  ClayImageComboBox *combo_box;

  combo_box = g_object_new (CLAY_TYPE_IMAGE_COMBO_BOX,
                            "width-request", WIDTH_REQUEST,
                            "ellipsize",     PANGO_ELLIPSIZE_MIDDLE,
                            NULL);

  combo_box->constraint   = constraint;
  combo_box->data         = data;
  combo_box->data_destroy = data_destroy;

  clay_image_combo_box_populate (combo_box);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (clay_image_combo_box_changed),
                    NULL);

  return GTK_WIDGET (combo_box);
}

static void
clay_image_combo_box_populate (ClayImageComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GList        *images;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  images = clay_list_images ();

  clay_image_combo_box_model_add (GTK_LIST_STORE (model), images,
                                  combo_box->constraint,
                                  combo_box->data);

  g_list_free (images);

  if (gtk_tree_model_get_iter_first (model, &iter))
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
}

static void
clay_image_combo_box_model_add (GtkListStore            *store,
                                GList                   *images,
                                ClayImageConstraintFunc  constraint,
                                gpointer                 data)
{
  GtkTreeIter  iter;
  GList       *list;

  for (list = images; list; list = g_list_next (list))
    {
      ClayImage *image    = list->data;
      gint32     image_id = clay_image_get_id (image);

      if (! constraint || constraint (image, data))
        {
          gchar     *image_name = clay_image_get_name (image);
          gchar     *label;
          GdkPixbuf *thumb;

          label = g_strdup_printf ("%s-%d", image_name, image_id);

          g_free (image_name);

          thumb = clay_image_get_thumbnail (image,
                                            THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                            CLAY_PIXBUF_SMALL_CHECKS);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              CLAY_INT_STORE_VALUE,  image_id,
                              CLAY_INT_STORE_LABEL,  label,
                              CLAY_INT_STORE_PIXBUF, thumb,
                              -1);

          if (thumb)
            g_object_unref (thumb);

          g_free (label);
        }
    }
}

static void
clay_image_combo_box_drag_data_received (GtkWidget        *widget,
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
      g_warning ("%s: received invalid image ID data", G_STRFUNC);
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

static void
clay_image_combo_box_changed (ClayImageComboBox *combo_box)
{
  gint image_ID;

  if (clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo_box),
                                     &image_ID))
    {
      if (! clay_image_get_by_id (image_ID))
        {
          GtkTreeModel *model;

          model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

          g_signal_stop_emission_by_name (combo_box, "changed");

          gtk_list_store_clear (GTK_LIST_STORE (model));
          clay_image_combo_box_populate (combo_box);
        }
    }
}
