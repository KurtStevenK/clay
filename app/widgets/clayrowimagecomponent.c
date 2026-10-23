/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowimagecomponent.c
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clayimage.h"

#include "claycomponenteditor.h"
#include "claydnd.h"
#include "clayeditor.h"
#include "clayrowimagecomponent.h"
#include "clayview.h"
#include "clayviewrendererimage.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


typedef struct _ClayRowImageComponentPrivate ClayRowImageComponentPrivate;

struct _ClayRowImageComponentPrivate
{
  ClayChannelType  channel;

  GtkWidget       *visible_toggle;
  GtkWidget       *visible_icon;
};

#define GET_PRIVATE(obj) \
  ((ClayRowImageComponentPrivate *) \
   clay_row_image_component_get_instance_private ((ClayRowImageComponent *) obj))


static void   clay_row_image_component_constructed  (GObject         *object);
static void   clay_row_image_component_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   clay_row_image_component_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static gboolean clay_row_image_component_button_press_event
                                                    (GtkWidget       *widget,
                                                     GdkEventButton  *bevent);

static void   clay_row_image_component_set_viewable (ClayRow         *row,
                                                     ClayViewable    *viewable);
static void   clay_row_image_component_name_changed (ClayRow         *row);

static void   clay_row_component_visibility_changed (ClayImage       *image,
                                                     ClayChannelType  channel,
                                                     ClayRow         *row);
static void   clay_row_component_visibility_toggled (GtkToggleButton *button,
                                                     ClayRow         *row);
static ClayImage * clay_row_image_component_drag_component
                                                    (GtkWidget       *widget,
                                                     ClayContext    **context,
                                                     ClayChannelType *channel,
                                                     gpointer         data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayRowImageComponent,
                            clay_row_image_component,
                            CLAY_TYPE_ROW_IMAGE)

#define parent_class clay_row_image_component_parent_class


static void
clay_row_image_component_class_init (ClayRowImageComponentClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  ClayRowClass   *row_class    = CLAY_ROW_CLASS (klass);

  object_class->constructed        = clay_row_image_component_constructed;
  object_class->set_property       = clay_row_image_component_set_property;
  object_class->get_property       = clay_row_image_component_get_property;

  widget_class->button_press_event = clay_row_image_component_button_press_event;

  row_class->set_viewable          = clay_row_image_component_set_viewable;
  row_class->name_changed          = clay_row_image_component_name_changed;

  obj_props[PROP_CHANNEL] =
    g_param_spec_enum ("channel",
                       NULL, NULL,
                       CLAY_TYPE_CHANNEL_TYPE,
                       CLAY_CHANNEL_RED,
                       CLAY_PARAM_READWRITE |
                       G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
clay_row_image_component_init (ClayRowImageComponent *row)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (row);

  priv->channel = -1; /* so the initial set_channel() always sets the label */

  priv->visible_toggle = _clay_row_add_toggle (CLAY_ROW (row),
                                               CLAY_ICON_VISIBLE,
                                               &priv->visible_icon);

  g_signal_connect (priv->visible_toggle, "toggled",
                    G_CALLBACK (clay_row_component_visibility_toggled),
                    row);
}

static void
clay_row_image_component_constructed (GObject *object)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (object);
  GtkWidget                    *view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view = _clay_row_get_view (CLAY_ROW (object));
  if (view)
    {
      CLAY_VIEW_RENDERER_IMAGE (CLAY_VIEW (view)->renderer)->channel =
        priv->channel;
    }
}

static void
clay_row_image_component_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayRowImageComponent *row = CLAY_ROW_IMAGE_COMPONENT (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      clay_row_image_component_set_channel (row, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_row_image_component_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  ClayRowImageComponent *row = CLAY_ROW_IMAGE_COMPONENT (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, clay_row_image_component_get_channel (row));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_row_image_component_button_press_event (GtkWidget      *widget,
                                             GdkEventButton *bevent)
{
  ClayRowImageComponentPrivate *priv   = GET_PRIVATE (widget);
  GdkEvent                     *event  = (GdkEvent *) bevent;
  GtkWidget                    *parent;
  GtkWidget                    *editor;

  parent = gtk_widget_get_parent (widget);
  editor = gtk_widget_get_ancestor (parent, CLAY_TYPE_COMPONENT_EDITOR);
  if (! editor)
    return FALSE;

  CLAY_COMPONENT_EDITOR (editor)->clicked_component = priv->channel;

  if (gdk_event_triggers_context_menu (event))
    {
      return clay_editor_popup_menu_at_pointer (CLAY_EDITOR (editor),
                                                event);
    }
  else if (bevent->button == 1)
    {
      if (gtk_list_box_row_is_selected (GTK_LIST_BOX_ROW (widget)))
        {
          gtk_list_box_unselect_row (GTK_LIST_BOX (parent),
                                     GTK_LIST_BOX_ROW (widget));
        }
      else
        {
          gtk_list_box_select_row (GTK_LIST_BOX (parent),
                                   GTK_LIST_BOX_ROW (widget));
        }
    }

  return TRUE;
}


static void
clay_row_image_component_set_viewable (ClayRow      *row,
                                       ClayViewable *viewable)
{
  ClayRowImageComponentPrivate *priv         = GET_PRIVATE (row);
  ClayViewable                 *old_viewable = clay_row_get_viewable (row);

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            clay_row_component_visibility_changed,
                                            row);

      clay_dnd_component_source_remove (GTK_WIDGET (row));
    }


  CLAY_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      /*  remove the generic sources added by ClayRow  */
      if (clay_dnd_viewable_source_remove (GTK_WIDGET (row), CLAY_TYPE_IMAGE))
        {
          clay_dnd_pixbuf_source_remove (GTK_WIDGET (row));
          gtk_drag_source_unset (GTK_WIDGET (row));
        }

      clay_dnd_component_source_add (GTK_WIDGET (row),
                                     clay_row_image_component_drag_component,
                                     NULL);

      g_signal_connect (viewable, "component-visibility-changed",
                        G_CALLBACK (clay_row_component_visibility_changed),
                        row);

      clay_row_component_visibility_changed (CLAY_IMAGE (viewable),
                                             priv->channel,
                                             row);
    }
}

static void
clay_row_image_component_name_changed (ClayRow *row)
{
  /*  do nothing and don't chain up, we set our own label  */
}


/*  public functions  */

void
clay_row_image_component_set_channel (ClayRowImageComponent *row,
                                      ClayChannelType        channel)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (CLAY_IS_ROW_IMAGE_COMPONENT (row));

  if (channel != priv->channel)
    {
      GtkWidget   *view;
      GtkWidget   *label;
      const gchar *desc;

      priv->channel = channel;

      view = _clay_row_get_view (CLAY_ROW (row));
      if (view)
        {
          CLAY_VIEW_RENDERER_IMAGE (CLAY_VIEW (view)->renderer)->channel =
            priv->channel;
        }

      clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, channel,
                           NULL, NULL, &desc, NULL);
      label = _clay_row_get_label (CLAY_ROW (row));
      gtk_label_set_text (GTK_LABEL (label), desc);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_CHANNEL]);

      clay_row_component_visibility_changed (CLAY_IMAGE (clay_row_get_viewable (CLAY_ROW (row))),
                                             priv->channel,
                                             CLAY_ROW (row));
    }
}

ClayChannelType
clay_row_image_component_get_channel (ClayRowImageComponent *row)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW_IMAGE_COMPONENT (row), CLAY_CHANNEL_RED);

  return priv->channel;
}


/*  private functions  */

static void
clay_row_component_visibility_changed (ClayImage       *image,
                                       ClayChannelType  channel,
                                       ClayRow         *row)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (row);

  if (channel == priv->channel)
    {
      gboolean visible;

      g_signal_handlers_block_by_func (priv->visible_toggle,
                                       clay_row_component_visibility_toggled,
                                       row);

      visible = clay_image_get_component_visible (image, channel);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->visible_toggle),
                                    visible);
      gtk_widget_set_opacity (priv->visible_icon, visible ? 1.0 : 0.0);

      g_signal_handlers_unblock_by_func (priv->visible_toggle,
                                         clay_row_component_visibility_toggled,
                                         row);
    }
}

static void
clay_row_component_visibility_toggled (GtkToggleButton *button,
                                       ClayRow         *row)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (row);
  ClayImage                    *image;
  gboolean                      active;

  image  = CLAY_IMAGE (clay_row_get_viewable (row));
  active = gtk_toggle_button_get_active (button);

  clay_image_set_component_visible (image, priv->channel, active);
  clay_image_flush (image);
}

static ClayImage *
clay_row_image_component_drag_component (GtkWidget        *widget,
                                         ClayContext     **context,
                                         ClayChannelType  *channel,
                                         gpointer          data)
{
  ClayRowImageComponentPrivate *priv = GET_PRIVATE (widget);
  ClayViewable                 *viewable;

  viewable = clay_row_get_viewable (CLAY_ROW (widget));

  if (viewable)
    {
      if (channel)
        *channel = priv->channel;

      if (context)
        *context = clay_row_get_context (CLAY_ROW (widget));

      return CLAY_IMAGE (viewable);
    }

  return NULL;
}
