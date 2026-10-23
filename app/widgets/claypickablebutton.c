/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablebutton.c
 * Copyright (C) 2013 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claypickable.h"

#include "claydnd.h"
#include "clayview.h"
#include "clayviewrenderer.h"
#include "claypickablebutton.h"
#include "claypickablepopup.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_PICKABLE
};

struct _ClayPickableButtonPrivate
{
  gint          view_size;
  gint          view_border_width;

  ClayContext  *context;
  ClayPickable *pickable;

  GtkWidget    *view;

  GBinding     *popup_binding;
  GtkWidget    *popup;
  ClayPickable *initial_pickable;
};


static void     clay_pickable_button_constructed   (GObject            *object);
static void     clay_pickable_button_dispose       (GObject            *object);
static void     clay_pickable_button_finalize      (GObject            *object);
static void     clay_pickable_button_set_property  (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void     clay_pickable_button_get_property  (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static void     clay_pickable_button_clicked       (GtkButton          *button);

static void     clay_pickable_button_popup_confirm (ClayPickableButton *button);
static void     clay_pickable_button_popup_cancel  (ClayPickableButton *button);
static void     clay_pickable_button_drop_pickable (GtkWidget          *widget,
                                                    gint                x,
                                                    gint                y,
                                                    ClayViewable       *viewable,
                                                    gpointer            data);
static void     clay_pickable_button_notify_buffer (ClayPickable       *pickable,
                                                    const GParamSpec   *pspec,
                                                    ClayPickableButton *button);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPickableButton, clay_pickable_button,
                            CLAY_TYPE_BUTTON)

#define parent_class clay_pickable_button_parent_class


static void
clay_pickable_button_class_init (ClayPickableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->constructed  = clay_pickable_button_constructed;
  object_class->dispose      = clay_pickable_button_dispose;
  object_class->finalize     = clay_pickable_button_finalize;
  object_class->get_property = clay_pickable_button_get_property;
  object_class->set_property = clay_pickable_button_set_property;

  button_class->clicked      = clay_pickable_button_clicked;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable",
                                                        NULL, NULL,
                                                        CLAY_TYPE_PICKABLE,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_pickable_button_init (ClayPickableButton *button)
{
  button->private = clay_pickable_button_get_instance_private (button);

  button->private->view_size         = CLAY_VIEW_SIZE_LARGE;
  button->private->view_border_width = 1;
  button->private->popup_binding     = NULL;

  clay_dnd_viewable_dest_add  (GTK_WIDGET (button), CLAY_TYPE_LAYER,
                               clay_pickable_button_drop_pickable,
                               NULL);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (button), CLAY_TYPE_LAYER_MASK,
                               clay_pickable_button_drop_pickable,
                               NULL);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (button), CLAY_TYPE_CHANNEL,
                               clay_pickable_button_drop_pickable,
                               NULL);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (button), CLAY_TYPE_IMAGE,
                               clay_pickable_button_drop_pickable,
                               NULL);
}

static void
clay_pickable_button_constructed (GObject *object)
{
  ClayPickableButton *button = CLAY_PICKABLE_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CONTEXT (button->private->context));

  button->private->view =
    clay_view_new_by_types (button->private->context,
                            CLAY_TYPE_VIEW,
                            CLAY_TYPE_VIEWABLE,
                            button->private->view_size,
                            button->private->view_border_width,
                            FALSE);
  gtk_container_add (GTK_CONTAINER (button), button->private->view);
  gtk_widget_set_visible (button->private->view, TRUE);
}

static void
clay_pickable_button_dispose (GObject *object)
{
  ClayPickableButton *button = CLAY_PICKABLE_BUTTON (object);

  clay_pickable_button_set_pickable (button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_pickable_button_finalize (GObject *object)
{
  ClayPickableButton *button = CLAY_PICKABLE_BUTTON (object);

  g_clear_object (&button->private->context);
  g_clear_object (&button->private->initial_pickable);
  if (button->private->popup)
    gtk_widget_destroy (button->private->popup);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pickable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayPickableButton *button = CLAY_PICKABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      button->private->context = g_value_dup_object (value);
      break;
    case PROP_PICKABLE:
      clay_pickable_button_set_pickable (button, g_value_get_object (value));
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pickable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayPickableButton *button = CLAY_PICKABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, button->private->context);
      break;
    case PROP_PICKABLE:
      g_value_set_object (value, button->private->pickable);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pickable_button_clicked (GtkButton *button)
{
  ClayPickableButton *pickable_button = CLAY_PICKABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = clay_pickable_popup_new (pickable_button->private->context,
                                   pickable_button->private->view_size,
                                   pickable_button->private->view_border_width);

  g_signal_connect_swapped (popup, "confirm",
                            G_CALLBACK (clay_pickable_button_popup_confirm),
                            button);
  g_signal_connect_swapped (popup, "cancel",
                            G_CALLBACK (clay_pickable_button_popup_cancel),
                            button);
  pickable_button->private->popup_binding = g_object_bind_property (G_OBJECT (button), "pickable",
                                                                    G_OBJECT (popup),  "pickable",
                                                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  pickable_button->private->initial_pickable = pickable_button->private->pickable;
  if (pickable_button->private->initial_pickable)
    g_object_ref (pickable_button->private->initial_pickable);

  pickable_button->private->popup = popup;
  g_signal_connect (popup, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &pickable_button->private->popup);
  clay_popup_show (CLAY_POPUP (popup), GTK_WIDGET (button));
}

static void
clay_pickable_button_popup_confirm (ClayPickableButton *button)
{
  g_clear_pointer (&button->private->popup_binding, g_binding_unbind);
  g_clear_object (&button->private->initial_pickable);
}

static void
clay_pickable_button_popup_cancel (ClayPickableButton *button)
{
  clay_pickable_button_set_pickable (button, button->private->initial_pickable);
  clay_pickable_button_popup_confirm (button);
}
static void
clay_pickable_button_drop_pickable (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    ClayViewable *viewable,
                                    gpointer      data)
{
  clay_pickable_button_set_pickable (CLAY_PICKABLE_BUTTON (widget),
                                     CLAY_PICKABLE (viewable));
}

static void
clay_pickable_button_notify_buffer (ClayPickable       *pickable,
                                    const GParamSpec   *pspec,
                                    ClayPickableButton *button)
{
  GeglBuffer *buffer = clay_pickable_get_buffer (pickable);

  if (buffer)
    g_object_notify (G_OBJECT (button), "pickable");
  else
    clay_pickable_button_set_pickable (button, NULL);
}


/*  public functions  */

GtkWidget *
clay_pickable_button_new (ClayContext *context,
                          gint         view_size,
                          gint         view_border_width)
{
  ClayPickableButton *button;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  button = g_object_new (CLAY_TYPE_PICKABLE_BUTTON,
                         "context", context,
                         NULL);

  button->private->view_size         = view_size;
  button->private->view_border_width = view_border_width;

  return GTK_WIDGET (button);
}

ClayPickable *
clay_pickable_button_get_pickable (ClayPickableButton *button)
{
  g_return_val_if_fail (CLAY_IS_PICKABLE_BUTTON (button), NULL);

  return button->private->pickable;
}

void
clay_pickable_button_set_pickable (ClayPickableButton *button,
                                   ClayPickable       *pickable)
{
  g_return_if_fail (CLAY_IS_PICKABLE_BUTTON (button));

  if (pickable != button->private->pickable)
    {
      if (button->private->pickable)
        g_signal_handlers_disconnect_by_func (button->private->pickable,
                                              clay_pickable_button_notify_buffer,
                                              button);

      g_set_object (&button->private->pickable, pickable);

      if (button->private->pickable)
        g_signal_connect (button->private->pickable, "notify::buffer",
                          G_CALLBACK (clay_pickable_button_notify_buffer),
                          button);

      clay_view_set_viewable (CLAY_VIEW (button->private->view),
                              CLAY_VIEWABLE (pickable));

      g_object_notify (G_OBJECT (button), "pickable");
    }
}
