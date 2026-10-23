/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablechooser.c
 * Copyright (C) 2023 Jehan
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

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claypickable.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claypickablechooser.h"
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
  PROP_PICKABLE_TYPE,
  PROP_PICKABLE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _ClayPickableChooserPrivate
{
  GType         pickable_type;
  ClayPickable *pickable;
  ClayContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *image_view;
  GtkWidget    *layer_view;
  GtkWidget    *channel_view;
  GtkWidget    *layer_label;
};


static void   clay_pickable_chooser_constructed    (GObject             *object);
static void   clay_pickable_chooser_finalize       (GObject             *object);
static void   clay_pickable_chooser_set_property   (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void   clay_pickable_chooser_get_property   (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static void   clay_pickable_chooser_image_changed  (ClayContext         *context,
                                                    ClayImage           *image,
                                                    ClayPickableChooser *chooser);
static void   clay_pickable_chooser_item_activated (ClayContainerView   *view,
                                                    ClayPickable        *pickable,
                                                    ClayPickableChooser *chooser);
static void   clay_pickable_chooser_items_selected (ClayContainerView   *view,
                                                    ClayPickableChooser *chooser);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPickableChooser, clay_pickable_chooser, GTK_TYPE_FRAME)

#define parent_class clay_pickable_chooser_parent_class

static guint signals[LAST_SIGNAL];

static void
clay_pickable_chooser_class_init (ClayPickableChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_pickable_chooser_constructed;
  object_class->finalize     = clay_pickable_chooser_finalize;
  object_class->get_property = clay_pickable_chooser_get_property;
  object_class->set_property = clay_pickable_chooser_set_property;

  /**
   * ClayPickableChooser::activate:
   * @chooser:
   *
   * Emitted when a pickable is activated, which is mostly forwarding when
   * "item-activated" signal is emitted from any of either the image, layer or
   * channel view. E.g. this happens when one double-click on one of the
   * pickables.
   */
  signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPickableChooserClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_OBJECT);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE_TYPE,
                                   g_param_spec_gtype ("pickable-type",
                                                       NULL, NULL,
                                                       CLAY_TYPE_PICKABLE,
                                                       CLAY_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable",
                                                        NULL, NULL,
                                                        CLAY_TYPE_PICKABLE,
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
clay_pickable_chooser_init (ClayPickableChooser *chooser)
{
  chooser->priv = clay_pickable_chooser_get_instance_private (chooser);

  chooser->priv->view_size         = CLAY_VIEW_SIZE_SMALL;
  chooser->priv->view_border_width = 1;

  chooser->priv->layer_view        = NULL;
  chooser->priv->channel_view      = NULL;
}

static void
clay_pickable_chooser_constructed (GObject *object)
{
  ClayPickableChooser *chooser = CLAY_PICKABLE_CHOOSER (object);
  GtkWidget           *hbox;
  GtkWidget           *vbox;
  GtkWidget           *label;
  GtkWidget           *notebook;
  ClayImage           *image;

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
                           G_CALLBACK (clay_pickable_chooser_item_activated),
                           G_OBJECT (chooser), 0);
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (clay_pickable_chooser_items_selected),
                           G_OBJECT (chooser), 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  chooser->priv->layer_label = label =
    gtk_label_new (_("Select an image in the left pane"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_set_visible (notebook, TRUE);

  if (g_type_is_a (CLAY_TYPE_LAYER, chooser->priv->pickable_type))
    {
      chooser->priv->layer_view =
        clay_container_tree_view_new (NULL,
                                      chooser->priv->context,
                                      chooser->priv->view_size,
                                      chooser->priv->view_border_width);
      gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (CLAY_CONTAINER_TREE_VIEW (chooser->priv->layer_view)->view),
                                        TRUE);
      clay_container_box_set_size_request (CLAY_CONTAINER_BOX (chooser->priv->layer_view),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                chooser->priv->layer_view,
                                gtk_label_new (_("Layers")));
      gtk_widget_set_visible (chooser->priv->layer_view, TRUE);

      g_signal_connect_object (chooser->priv->layer_view, "item-activated",
                               G_CALLBACK (clay_pickable_chooser_item_activated),
                               G_OBJECT (chooser), 0);
      g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                               G_CALLBACK (clay_pickable_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  if (g_type_is_a (CLAY_TYPE_CHANNEL, chooser->priv->pickable_type))
    {
      chooser->priv->channel_view =
        clay_container_tree_view_new (NULL,
                                      chooser->priv->context,
                                      chooser->priv->view_size,
                                      chooser->priv->view_border_width);
      clay_container_box_set_size_request (CLAY_CONTAINER_BOX (chooser->priv->channel_view),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width),
                                           4 * (chooser->priv->view_size +
                                                2 * chooser->priv->view_border_width));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                chooser->priv->channel_view,
                                gtk_label_new (_("Channels")));
      gtk_widget_set_visible (chooser->priv->channel_view, TRUE);

      g_signal_connect_object (chooser->priv->channel_view, "item-activated",
                               G_CALLBACK (clay_pickable_chooser_item_activated),
                               G_OBJECT (chooser), 0);
      g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                               G_CALLBACK (clay_pickable_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }

  g_signal_connect_object (chooser->priv->context, "image-changed",
                           G_CALLBACK (clay_pickable_chooser_image_changed),
                           G_OBJECT (chooser), 0);

  image = clay_context_get_image (chooser->priv->context);
  clay_pickable_chooser_image_changed (chooser->priv->context, image, chooser);
}

static void
clay_pickable_chooser_finalize (GObject *object)
{
  ClayPickableChooser *chooser = CLAY_PICKABLE_CHOOSER (object);

  g_clear_object (&chooser->priv->pickable);
  g_clear_object (&chooser->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pickable_chooser_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayPickableChooser *chooser = CLAY_PICKABLE_CHOOSER (object);

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

    case PROP_PICKABLE_TYPE:
      g_return_if_fail (g_value_get_gtype (value) == CLAY_TYPE_LAYER    ||
                        g_value_get_gtype (value) == CLAY_TYPE_CHANNEL  ||
                        g_value_get_gtype (value) == CLAY_TYPE_DRAWABLE ||
                        g_value_get_gtype (value) == CLAY_TYPE_IMAGE    ||
                        g_value_get_gtype (value) == CLAY_TYPE_PICKABLE);
      chooser->priv->pickable_type = g_value_get_gtype (value);
      break;

    case PROP_PICKABLE:
      g_return_if_fail (g_value_get_object (value) == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (g_value_get_object (value)),
                                     chooser->priv->pickable_type));
      clay_pickable_chooser_set_pickable (chooser, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pickable_chooser_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayPickableChooser *chooser = CLAY_PICKABLE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, chooser->priv->context);
      break;

    case PROP_PICKABLE_TYPE:
      g_value_set_gtype (value, chooser->priv->pickable_type);
      break;

    case PROP_PICKABLE:
      g_value_set_object (value, chooser->priv->pickable);
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
clay_pickable_chooser_new (ClayContext *context,
                           GType        pickable_type,
                           gint         view_size,
                           gint         view_border_width)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (CLAY_TYPE_PICKABLE_CHOOSER,
                       "context",           context,
                       "pickable-type",     pickable_type,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

ClayPickable *
clay_pickable_chooser_get_pickable (ClayPickableChooser *chooser)
{
  g_return_val_if_fail (CLAY_IS_PICKABLE_CHOOSER (chooser), NULL);

  return chooser->priv->pickable;
}

void
clay_pickable_chooser_set_pickable (ClayPickableChooser *chooser,
                                    ClayPickable        *pickable)
{
  if (! gtk_widget_in_destruction (GTK_WIDGET (chooser)))
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->image_view,
                                            G_CALLBACK (clay_pickable_chooser_items_selected),
                                            chooser);
      if (chooser->priv->layer_view != NULL)
        g_signal_handlers_disconnect_by_func (chooser->priv->layer_view,
                                              G_CALLBACK (clay_pickable_chooser_items_selected),
                                              chooser);
      else
        g_return_if_fail (! CLAY_IS_LAYER (pickable));

      if (chooser->priv->channel_view != NULL)
        g_signal_handlers_disconnect_by_func (chooser->priv->channel_view,
                                              G_CALLBACK (clay_pickable_chooser_items_selected),
                                              chooser);
      else
        g_return_if_fail (! CLAY_IS_CHANNEL (pickable));

      if (CLAY_IS_IMAGE (pickable))
        {
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (chooser->priv->image_view),
                                              CLAY_VIEWABLE (pickable));
          clay_context_set_image (chooser->priv->context, CLAY_IMAGE (pickable));
        }
      else if (CLAY_IS_LAYER (pickable))
        {
          clay_context_set_image (chooser->priv->context, clay_item_get_image (CLAY_ITEM (pickable)));
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (chooser->priv->layer_view),
                                              CLAY_VIEWABLE (pickable));
        }
      else if (CLAY_IS_CHANNEL (pickable))
        {
          clay_context_set_image (chooser->priv->context, clay_item_get_image (CLAY_ITEM (pickable)));
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (chooser->priv->channel_view),
                                              CLAY_VIEWABLE (pickable));
        }
      else
        {
          g_return_if_fail (pickable == NULL);
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (chooser->priv->image_view), NULL);
          clay_context_set_image (chooser->priv->context, NULL);
        }
      g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                               G_CALLBACK (clay_pickable_chooser_items_selected),
                               G_OBJECT (chooser), 0);
      if (chooser->priv->layer_view != NULL)
        g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                                 G_CALLBACK (clay_pickable_chooser_items_selected),
                                 G_OBJECT (chooser), 0);
      if (chooser->priv->channel_view != NULL)
        g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                                 G_CALLBACK (clay_pickable_chooser_items_selected),
                                 G_OBJECT (chooser), 0);
    }

  if (pickable != chooser->priv->pickable)
    {
      g_clear_object (&chooser->priv->pickable);
      chooser->priv->pickable = (pickable != NULL ? g_object_ref (pickable) : NULL);
      g_object_notify (G_OBJECT (chooser), "pickable");
    }
}


/*  private functions  */

static void
clay_pickable_chooser_image_changed (ClayContext         *context,
                                     ClayImage           *image,
                                     ClayPickableChooser *chooser)
{
  ClayContainer *layers   = NULL;
  ClayContainer *channels = NULL;

  if (image)
    {
      gchar *desc;

      layers   = clay_image_get_layers (image);
      channels = clay_image_get_channels (image);

      desc = clay_viewable_get_description (CLAY_VIEWABLE (image), NULL);
      gtk_label_set_text (GTK_LABEL (chooser->priv->layer_label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (chooser->priv->layer_label),
                          _("Select an image in the left pane"));
    }

  g_signal_handlers_disconnect_by_func (chooser->priv->image_view,
                                        G_CALLBACK (clay_pickable_chooser_items_selected),
                                        chooser);
  if (chooser->priv->layer_view != NULL)
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->layer_view,
                                            G_CALLBACK (clay_pickable_chooser_items_selected),
                                            chooser);
      clay_container_view_set_container (CLAY_CONTAINER_VIEW (chooser->priv->layer_view),
                                         layers);
      g_signal_connect_object (chooser->priv->layer_view, "selection-changed",
                               G_CALLBACK (clay_pickable_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }
  if (chooser->priv->channel_view != NULL)
    {
      g_signal_handlers_disconnect_by_func (chooser->priv->channel_view,
                                            G_CALLBACK (clay_pickable_chooser_items_selected),
                                            chooser);
      clay_container_view_set_container (CLAY_CONTAINER_VIEW (chooser->priv->channel_view),
                                         channels);
      g_signal_connect_object (chooser->priv->channel_view, "selection-changed",
                               G_CALLBACK (clay_pickable_chooser_items_selected),
                               G_OBJECT (chooser), 0);
    }
  g_signal_connect_object (chooser->priv->image_view, "selection-changed",
                           G_CALLBACK (clay_pickable_chooser_items_selected),
                           G_OBJECT (chooser), 0);
}

static void
clay_pickable_chooser_item_activated (ClayContainerView   *view,
                                      ClayPickable        *pickable,
                                      ClayPickableChooser *chooser)
{
  g_signal_emit (chooser, signals[ACTIVATE], 0, pickable);
}

static void
clay_pickable_chooser_items_selected (ClayContainerView   *view,
                                      ClayPickableChooser *chooser)
{
  ClayPickable *pickable = NULL;
  gint          n_items;
  GList        *items;

  n_items = clay_container_view_get_selected (view, &items);

  g_return_if_fail (n_items <= 1);

  if (items)
    pickable = items->data;

  clay_pickable_chooser_set_pickable (chooser, pickable);
}
