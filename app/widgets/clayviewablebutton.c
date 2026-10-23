/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewablebutton.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claycontainerpopup.h"
#include "claydialogfactory.h"
#include "claypropwidgets.h"
#include "clayviewrenderer.h"
#include "clayviewablebutton.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_VIEW_TYPE,
  PROP_POPUP_VIEW_SIZE
};


static void     clay_viewable_button_finalize     (GObject            *object);
static void     clay_viewable_button_set_property (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void     clay_viewable_button_get_property (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static gboolean clay_viewable_button_scroll_event (GtkWidget          *widget,
                                                   GdkEventScroll     *sevent);
static void     clay_viewable_button_clicked      (GtkButton          *button);

static void     clay_viewable_button_popup_closed (ClayContainerPopup *popup,
                                                   ClayViewableButton *button);


G_DEFINE_TYPE (ClayViewableButton, clay_viewable_button, CLAY_TYPE_BUTTON)

#define parent_class clay_viewable_button_parent_class


static void
clay_viewable_button_class_init (ClayViewableButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->finalize     = clay_viewable_button_finalize;
  object_class->get_property = clay_viewable_button_get_property;
  object_class->set_property = clay_viewable_button_set_property;

  widget_class->scroll_event = clay_viewable_button_scroll_event;

  button_class->clicked      = clay_viewable_button_clicked;

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_TYPE,
                                   g_param_spec_enum ("popup-view-type",
                                                      NULL, NULL,
                                                      CLAY_TYPE_VIEW_TYPE,
                                                      CLAY_VIEW_TYPE_LIST,
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_SIZE,
                                   g_param_spec_int ("popup-view-size",
                                                     NULL, NULL,
                                                     CLAY_VIEW_SIZE_TINY,
                                                     CLAY_VIEW_SIZE_GIGANTIC,
                                                     CLAY_VIEW_SIZE_SMALL,
                                                     CLAY_PARAM_READWRITE));
}

static void
clay_viewable_button_init (ClayViewableButton *button)
{
  button->popup_view_type   = CLAY_VIEW_TYPE_LIST;
  button->popup_view_size   = CLAY_VIEW_SIZE_SMALL;

  button->button_view_size  = CLAY_VIEW_SIZE_SMALL;
  button->view_border_width = 1;

  gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK);
}

static void
clay_viewable_button_finalize (GObject *object)
{
  ClayViewableButton *button = CLAY_VIEWABLE_BUTTON (object);

  g_clear_pointer (&button->dialog_identifier, g_free);
  g_clear_pointer (&button->dialog_icon_name,  g_free);
  g_clear_pointer (&button->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_viewable_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayViewableButton *button = CLAY_VIEWABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_POPUP_VIEW_TYPE:
      clay_viewable_button_set_view_type (button, g_value_get_enum (value));
      break;
    case PROP_POPUP_VIEW_SIZE:
      clay_viewable_button_set_view_size (button, g_value_get_int (value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_viewable_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayViewableButton *button = CLAY_VIEWABLE_BUTTON (object);

  switch (property_id)
    {
    case PROP_POPUP_VIEW_TYPE:
      g_value_set_enum (value, button->popup_view_type);
      break;
    case PROP_POPUP_VIEW_SIZE:
      g_value_set_int (value, button->popup_view_size);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_viewable_button_scroll_event (GtkWidget      *widget,
                                   GdkEventScroll *sevent)
{
  ClayViewableButton *button = CLAY_VIEWABLE_BUTTON (widget);
  ClayObject         *object;
  gint                index;

  object = clay_context_get_by_type (button->context,
                                     clay_container_get_child_type (button->container));

  index = clay_container_get_child_index (button->container, object);

  if (index != -1)
    {
      gint n_children;
      gint new_index = index;

      n_children = clay_container_get_n_children (button->container);

      if (sevent->direction == GDK_SCROLL_UP)
        {
          if (index > 0)
            new_index--;
          else
            new_index = n_children - 1;
        }
      else if (sevent->direction == GDK_SCROLL_DOWN)
        {
          if (index == (n_children - 1))
            new_index = 0;
          else
            new_index++;
        }

      if (new_index != index)
        {
          object = clay_container_get_child_by_index (button->container,
                                                      new_index);

          if (object)
            clay_context_set_by_type (button->context,
                                      clay_container_get_child_type (button->container),
                                      object);
        }
    }

  return TRUE;
}

static void
clay_viewable_button_clicked (GtkButton *button)
{
  ClayViewableButton *viewable_button = CLAY_VIEWABLE_BUTTON (button);
  GtkWidget          *popup;

  popup = clay_container_popup_new (viewable_button->container,
                                    viewable_button->context,
                                    viewable_button->popup_view_type,
                                    viewable_button->button_view_size,
                                    viewable_button->popup_view_size,
                                    viewable_button->view_border_width,
                                    viewable_button->dialog_factory,
                                    viewable_button->dialog_identifier,
                                    viewable_button->dialog_icon_name,
                                    viewable_button->dialog_tooltip);

  g_signal_connect (popup, "cancel",
                    G_CALLBACK (clay_viewable_button_popup_closed),
                    button);
  g_signal_connect (popup, "confirm",
                    G_CALLBACK (clay_viewable_button_popup_closed),
                    button);

  clay_popup_show (CLAY_POPUP (popup), GTK_WIDGET (button));
}

static void
clay_viewable_button_popup_closed (ClayContainerPopup *popup,
                                   ClayViewableButton *button)
{
  clay_viewable_button_set_view_type (button,
                                      clay_container_popup_get_view_type (popup));
  clay_viewable_button_set_view_size (button,
                                      clay_container_popup_get_view_size (popup));
}


/*  public functions  */

GtkWidget *
clay_viewable_button_new (ClayContainer     *container,
                          ClayContext       *context,
                          ClayViewType       view_type,
                          gint               button_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          ClayDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  ClayViewableButton *button;
  const gchar        *prop_name;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_BUTTON_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        CLAY_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  button = g_object_new (CLAY_TYPE_VIEWABLE_BUTTON,
                         "popup-view-type", view_type,
                         "popup-view-size", view_size,
                         NULL);

  button->container = container;
  button->context   = context;

  button->button_view_size  = button_view_size;
  button->view_border_width = view_border_width;

  if (dialog_factory)
    {
      button->dialog_factory    = dialog_factory;
      button->dialog_identifier = g_strdup (dialog_identifier);
      button->dialog_icon_name  = g_strdup (dialog_icon_name);
      button->dialog_tooltip    = g_strdup (dialog_tooltip);
    }

  prop_name = clay_context_type_to_prop_name (clay_container_get_child_type (container));

  button->view = clay_prop_view_new (G_OBJECT (context), prop_name,
                                     context, button->button_view_size);
  gtk_container_add (GTK_CONTAINER (button), button->view);

  return GTK_WIDGET (button);
}

ClayViewType
clay_viewable_button_get_view_type (ClayViewableButton *button)
{
  g_return_val_if_fail (CLAY_IS_VIEWABLE_BUTTON (button), CLAY_VIEW_TYPE_LIST);

  return button->popup_view_type;
}

void
clay_viewable_button_set_view_type (ClayViewableButton *button,
                                    ClayViewType        view_type)
{
  g_return_if_fail (CLAY_IS_VIEWABLE_BUTTON (button));

  if (view_type != button->popup_view_type)
    {
      button->popup_view_type = view_type;

      g_object_notify (G_OBJECT (button), "popup-view-type");
    }
}

gint
clay_viewable_button_get_view_size (ClayViewableButton *button)
{
  g_return_val_if_fail (CLAY_IS_VIEWABLE_BUTTON (button), CLAY_VIEW_SIZE_SMALL);

  return button->popup_view_size;
}

void
clay_viewable_button_set_view_size (ClayViewableButton *button,
                                    ClayViewSize        view_size)
{
  g_return_if_fail (CLAY_IS_VIEWABLE_BUTTON (button));

  if (view_size != button->popup_view_size)
    {
      button->popup_view_size = view_size;

      g_object_notify (G_OBJECT (button), "popup-view-size");
    }
}
