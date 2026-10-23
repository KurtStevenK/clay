/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablepopup.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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
#include "core/claypickable.h"
#include "core/clayviewable.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claypickablechooser.h"
#include "claypickablepopup.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_PICKABLE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH
};

struct _ClayPickablePopupPrivate
{
  ClayContext  *context;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *chooser;

  GtkWidget    *image_view;
  GtkWidget    *layer_view;
  GtkWidget    *channel_view;
  GtkWidget    *layer_label;
};


static void   clay_pickable_popup_constructed     (GObject             *object);
static void   clay_pickable_popup_finalize        (GObject             *object);
static void   clay_pickable_popup_set_property    (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   clay_pickable_popup_get_property    (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static void   clay_pickable_popup_activate        (ClayPickablePopup   *popup);
static void   clay_pickable_popup_notify_pickable (ClayPickablePopup   *popup);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPickablePopup, clay_pickable_popup,
                            CLAY_TYPE_POPUP)

#define parent_class clay_pickable_popup_parent_class


static void
clay_pickable_popup_class_init (ClayPickablePopupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_pickable_popup_constructed;
  object_class->finalize     = clay_pickable_popup_finalize;
  object_class->get_property = clay_pickable_popup_get_property;
  object_class->set_property = clay_pickable_popup_set_property;

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
clay_pickable_popup_init (ClayPickablePopup *popup)
{
  popup->priv = clay_pickable_popup_get_instance_private (popup);

  popup->priv->view_size         = CLAY_VIEW_SIZE_SMALL;
  popup->priv->view_border_width = 1;

  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);
}

static void
clay_pickable_popup_constructed (GObject *object)
{
  ClayPickablePopup *popup = CLAY_PICKABLE_POPUP (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CONTEXT (popup->priv->context));

  popup->priv->chooser = clay_pickable_chooser_new (popup->priv->context, CLAY_TYPE_PICKABLE,
                                                    popup->priv->view_size,
                                                    popup->priv->view_border_width);
  gtk_container_add (GTK_CONTAINER (popup), popup->priv->chooser);
  g_signal_connect_swapped (popup->priv->chooser, "notify::pickable",
                            G_CALLBACK (clay_pickable_popup_notify_pickable),
                            popup);
  g_signal_connect_swapped (popup->priv->chooser, "activate",
                            G_CALLBACK (clay_pickable_popup_activate),
                            popup);
  gtk_widget_set_visible (popup->priv->chooser, TRUE);
}

static void
clay_pickable_popup_finalize (GObject *object)
{
  ClayPickablePopup *popup = CLAY_PICKABLE_POPUP (object);

  g_clear_object (&popup->priv->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pickable_popup_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayPickablePopup *popup = CLAY_PICKABLE_POPUP (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      popup->priv->context = g_value_dup_object (value);
      break;

    case PROP_PICKABLE:
      clay_pickable_chooser_set_pickable (CLAY_PICKABLE_CHOOSER (popup->priv->chooser),
                                          g_value_get_object (value));
      break;

    case PROP_VIEW_SIZE:
      popup->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      popup->priv->view_border_width = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pickable_popup_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayPickablePopup *popup = CLAY_PICKABLE_POPUP (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, popup->priv->context);
      break;

    case PROP_PICKABLE:
      g_value_set_object (value, clay_pickable_popup_get_pickable (popup));
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, popup->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, popup->priv->view_border_width);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public functions */

GtkWidget *
clay_pickable_popup_new (ClayContext *context,
                         gint         view_size,
                         gint         view_border_width)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (CLAY_TYPE_PICKABLE_POPUP,
                       "type",              GTK_WINDOW_POPUP,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

ClayPickable *
clay_pickable_popup_get_pickable (ClayPickablePopup *popup)
{
  g_return_val_if_fail (CLAY_IS_PICKABLE_POPUP (popup), NULL);

  return clay_pickable_chooser_get_pickable (CLAY_PICKABLE_CHOOSER (popup->priv->chooser));
}


/* Private functions */

static void
clay_pickable_popup_activate (ClayPickablePopup *popup)
{
  g_signal_emit_by_name (popup, "confirm");
}

static void
clay_pickable_popup_notify_pickable (ClayPickablePopup *popup)
{
  g_object_notify (G_OBJECT (popup), "pickable");
}
