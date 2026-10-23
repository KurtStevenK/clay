/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimagechooser.c
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayviewable.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "clayimagechooser.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


enum
{
  ACTIVATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_IMAGE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _ClayImageChooserPrivate
{
  ClayImage    *image;
  ClayContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *image_view;
};


static void   clay_image_chooser_constructed    (GObject             *object);
static void   clay_image_chooser_finalize       (GObject             *object);
static void   clay_image_chooser_set_property   (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void   clay_image_chooser_get_property   (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void   clay_image_chooser_item_activated (ClayContainerView   *view,
                                                 ClayImage           *image,
                                                 ClayImageChooser    *chooser);
static void   clay_image_chooser_items_selected (ClayContainerView   *view,
                                                 ClayImageChooser    *chooser);


G_DEFINE_TYPE_WITH_PRIVATE (ClayImageChooser, clay_image_chooser, GTK_TYPE_FRAME)

#define parent_class clay_image_chooser_parent_class

static guint signals[LAST_SIGNAL];

static void
clay_image_chooser_class_init (ClayImageChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_image_chooser_constructed;
  object_class->finalize     = clay_image_chooser_finalize;
  object_class->get_property = clay_image_chooser_get_property;
  object_class->set_property = clay_image_chooser_set_property;

  /**
   * ClayImageChooser::activate:
   * @chooser:
   *
   * Emitted when an image is activated, which is mostly forwarding when
   * "item-activated" signal is emitted from the image view.
   */
  signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageChooserClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_OBJECT);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_int ("view-size",
                                                     NULL, NULL,
                                                     1, CLAY_VIEWABLE_MAX_PREVIEW_SIZE,
                                                     CLAY_VIEW_SIZE_MEDIUM,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_VIEW_BORDER_WIDTH,
                                   g_param_spec_int ("view-border-width",
                                                     NULL, NULL,
                                                     0,
                                                     CLAY_VIEW_MAX_BORDER_WIDTH,
                                                     1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
clay_image_chooser_init (ClayImageChooser *chooser)
{
  chooser->priv = clay_image_chooser_get_instance_private (chooser);

  chooser->priv->view_size         = CLAY_VIEW_SIZE_SMALL;
  chooser->priv->view_border_width = 1;
}

static void
clay_image_chooser_constructed (GObject *object)
{
  ClayImageChooser *chooser = CLAY_IMAGE_CHOOSER (object);
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *label;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CONTEXT (chooser->priv->context));

  gtk_frame_set_shadow_type (GTK_FRAME (chooser), GTK_SHADOW_OUT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (chooser), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("Images"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  chooser->priv->image_view =
    clay_container_tree_view_new (chooser->priv->context->clay->images,
                                  chooser->priv->context,
                                  chooser->priv->view_size,
                                  chooser->priv->view_border_width);
  clay_container_box_set_size_request (CLAY_CONTAINER_BOX (chooser->priv->image_view),
                                       4 * (chooser->priv->view_size +
                                            2 * chooser->priv->view_border_width),
                                       4 * (chooser->priv->view_size +
                                            2 * chooser->priv->view_border_width));
  gtk_box_pack_start (GTK_BOX (vbox), chooser->priv->image_view, TRUE, TRUE, 0);
  gtk_widget_set_visible (chooser->priv->image_view, TRUE);

  g_signal_connect_object (chooser->priv->image_view, "item-activated",
                           G_CALLBACK (clay_image_chooser_item_activated),
                           G_OBJECT (chooser), 0);
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (clay_image_chooser_items_selected),
                           G_OBJECT (chooser), 0);
}

static void
clay_image_chooser_finalize (GObject *object)
{
  ClayImageChooser *chooser = CLAY_IMAGE_CHOOSER (object);

  g_clear_object (&chooser->priv->image);
  g_clear_object (&chooser->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_image_chooser_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayImageChooser *chooser = CLAY_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      chooser->priv->context = g_value_dup_object (value);
      break;

    case PROP_VIEW_SIZE:
      chooser->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      chooser->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_IMAGE:
      clay_image_chooser_set_image (chooser, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_chooser_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayImageChooser *chooser = CLAY_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, chooser->priv->context);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, chooser->priv->image);
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, chooser->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, chooser->priv->view_border_width);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_image_chooser_new (ClayContext *context,
                        gint         view_size,
                        gint         view_border_width)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (CLAY_TYPE_IMAGE_CHOOSER,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

ClayImage *
clay_image_chooser_get_image (ClayImageChooser *chooser)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_CHOOSER (chooser), NULL);

  return chooser->priv->image;
}

void
clay_image_chooser_set_image (ClayImageChooser *chooser,
                              ClayImage        *image)
{
  g_return_if_fail (CLAY_IS_IMAGE_CHOOSER (chooser));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  if (! gtk_widget_in_destruction (GTK_WIDGET (chooser)))
    {
      g_signal_handlers_block_by_func (chooser->priv->image_view,
                                       clay_image_chooser_items_selected,
                                       chooser);

      if (CLAY_IS_IMAGE (image))
        {
          clay_context_set_image (chooser->priv->context, image);
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (chooser->priv->image_view),
                                              CLAY_VIEWABLE (image));
        }

      g_signal_handlers_unblock_by_func (chooser->priv->image_view,
                                         clay_image_chooser_items_selected,
                                         chooser);
    }

  if (image != chooser->priv->image)
    {
      g_set_object (&chooser->priv->image, image);
      g_object_notify (G_OBJECT (chooser), "image");
    }
}


/*  private functions  */

static void
clay_image_chooser_item_activated (ClayContainerView *view,
                                   ClayImage         *image,
                                   ClayImageChooser  *chooser)
{
  g_signal_emit (chooser, signals[ACTIVATE], 0, image);
}

static void
clay_image_chooser_items_selected (ClayContainerView *view,
                                   ClayImageChooser  *chooser)
{
  ClayImage *image = NULL;
  gint       n_items;
  GList     *items;

  n_items = clay_container_view_get_selected (view, &items);

  g_return_if_fail (n_items <= 1);

  if (items)
    image = items->data;

  clay_image_chooser_set_image (chooser, image);
}
