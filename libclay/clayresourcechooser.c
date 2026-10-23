/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#ifdef PLATFORM_OSX
#import <AppKit/AppKit.h>
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"
#include "clayresourceselect-private.h"

#include "clayuitypes.h"
#include "clayresourcechooser.h"
#include "clayuimarshal.h"

#include "libclay-intl.h"


/**
 * SECTION: clayresourcechooser
 * @title: ClayResourceChooser
 * @short_description: Base class for buttons that popup a resource
 *                     selection dialog.
 *
 * A button which pops up a resource selection dialog.
 *
 * Responsibilities:
 *
 *     - implementing outer container widget,
 *     - managing clicks and popping up a remote chooser,
 *     - having a resource property,
 *     - signaling when user selects resource
 *     - receiving drag,
 *     - triggering draws of the button interior (by subclass) and draws of remote popup chooser.
 *
 * Collaborations:
 *
 *     - owned by ClayProcedureDialog via ClayPropWidget
 *     - resource property usually bound to a ClayConfig for a ClayPluginProcedure.
 *     - communicates using ClayResourceSelect with remote ClayPDBDialog,
 *       to choose an installed ClayResource owned by core.
 *
 * Subclass responsibilities:
 *
 *     - creating interior widgets
 *     - drawing the interior (a preview of the chosen resource)
 *     - declaring which interior widgets are drag destinations
 *     - declaring which interior widgets are clickable (generate "clicked" signal)
 *     - generate "clicked" (delegating to GtkButton or implementing from mouse events)
 *
 * Since: 3.0
 **/


enum
{
  RESOURCE_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LABEL,
  PROP_RESOURCE,
  N_PROPS
};


typedef struct
{
  ClayResource *resource;
  gchar        *title;
  gchar        *label;
  gchar        *callback;

  GtkWidget    *label_widget;
} ClayResourceChooserPrivate;


/*  local function prototypes  */

static void   clay_resource_chooser_constructed        (GObject             *object);
static void   clay_resource_chooser_dispose            (GObject             *object);
static void   clay_resource_chooser_finalize           (GObject             *object);

static void   clay_resource_chooser_set_property       (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void   clay_resource_chooser_get_property       (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);

static void   clay_resource_chooser_clicked            (ClayResourceChooser *self);

static void   clay_resource_chooser_callback           (ClayResource        *resource,
                                                        gboolean             dialog_closing,
                                                        gpointer             user_data);

static void   clay_resource_select_drag_data_received  (ClayResourceChooser *self,
                                                        GdkDragContext      *context,
                                                        gint                 x,
                                                        gint                 y,
                                                        GtkSelectionData    *selection,
                                                        guint                info,
                                                        guint                time);

static void   clay_resource_chooser_set_remote_dialog  (ClayResourceChooser *self,
                                                        ClayResource        *resource);


static guint resource_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *resource_button_props[N_PROPS] = { NULL, };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayResourceChooser, clay_resource_chooser, GTK_TYPE_BOX)


static void
clay_resource_chooser_class_init (ClayResourceChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_resource_chooser_constructed;
  object_class->dispose      = clay_resource_chooser_dispose;
  object_class->finalize     = clay_resource_chooser_finalize;
  object_class->set_property = clay_resource_chooser_set_property;
  object_class->get_property = clay_resource_chooser_get_property;

  klass->resource_set        = NULL;
  klass->draw_interior       = NULL;

  /**
   * ClayResourceChooser:title:
   *
   * The title to be used for the resource selection popup dialog.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the resource selection popup dialog",
                         "Resource Selection",
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayResourceChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayResourceChooser:resource:
   *
   * The currently selected resource.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_RESOURCE] =
    clay_param_spec_resource ("resource",
                              "Resource",
                              "The currently selected resource",
                              CLAY_TYPE_RESOURCE,
                              TRUE,  /* none_ok */
                              NULL,  /* no default for this property. */
                              FALSE,
                              CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS, resource_button_props);

  /**
   * ClayResourceChooser::resource-set:
   * @widget: the object which received the signal.
   * @resource: the currently selected resource.
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::resource-set signal is emitted when the user selects a resource.
   *
   * Since: 3.0
   */
  resource_button_signals[RESOURCE_SET] =
    g_signal_new ("resource-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayResourceChooserClass, resource_set),
                  NULL, NULL,
                  _clayui_marshal_VOID__POINTER_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_OBJECT,
                  G_TYPE_BOOLEAN);
}

static void
clay_resource_chooser_init (ClayResourceChooser *self)
{
  ClayResourceChooserPrivate *priv;

  priv = clay_resource_chooser_get_instance_private (CLAY_RESOURCE_CHOOSER (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (self), 6);

  priv->label_widget = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label_widget), 0.0);
  gtk_box_pack_start (GTK_BOX (self), priv->label_widget, FALSE, FALSE, 0);
}

static void
clay_resource_chooser_constructed (GObject *object)
{
  ClayResourceChooserPrivate *priv;

  priv = clay_resource_chooser_get_instance_private (CLAY_RESOURCE_CHOOSER (object));
  /* Set text of label widget now since underlying property might have changed.
   * Not when empty string:  set_text throws CRITICAL on empty string!
   */
  if (priv->label != NULL)
    {
      gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_widget), priv->label);
      gtk_widget_set_visible (GTK_WIDGET (priv->label_widget), TRUE);
    }

  G_OBJECT_CLASS (clay_resource_chooser_parent_class)->constructed (object);
}

static void
clay_resource_chooser_dispose (GObject *self)
{
  ClayResourceChooserPrivate *priv;
  ClayResourceChooserClass   *klass;

  priv  = clay_resource_chooser_get_instance_private (CLAY_RESOURCE_CHOOSER (self));
  klass = CLAY_RESOURCE_CHOOSER_GET_CLASS (self);

  if (priv->callback)
    {
      GType resource_type = klass->resource_type;

      if (resource_type == CLAY_TYPE_FONT)
        clay_fonts_close_popup (priv->callback);
      else if (resource_type == CLAY_TYPE_GRADIENT)
        clay_gradients_close_popup (priv->callback);
      else if (resource_type == CLAY_TYPE_BRUSH)
        clay_brushes_close_popup (priv->callback);
      else if (resource_type == CLAY_TYPE_PALETTE)
        clay_palettes_close_popup (priv->callback);
      else if (resource_type == CLAY_TYPE_PATTERN)
        clay_patterns_close_popup (priv->callback);
      else
        g_warning ("%s: unhandled resource type", G_STRFUNC);

      clay_plug_in_remove_temp_procedure (clay_get_plug_in (), priv->callback);
      g_clear_pointer (&priv->callback, g_free);
    }

  G_OBJECT_CLASS (clay_resource_chooser_parent_class)->dispose (self);
}

static void
clay_resource_chooser_finalize (GObject *object)
{
  ClayResourceChooser *self = CLAY_RESOURCE_CHOOSER (object);
  ClayResourceChooserPrivate *priv = clay_resource_chooser_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->label, g_free);

  G_OBJECT_CLASS (clay_resource_chooser_parent_class)->finalize (object);
}

/**
 * _clay_resource_chooser_set_drag_target:
 * @chooser:            A [class@ResourceChooser]
 * @drag_region_widget: An interior widget to be a droppable region
 *                      and emit "drag-data-received" signal
 * @drag_target:        The drag target to accept
 *
 * Called by a subclass init to specialize the instance.
 *
 * Subclass knows its interior widget whose region is a drop zone.
 * Subclass knows what things can be dropped (target.)
 * Self (super) handles the drop.
 *
 * Since: 3.0
 **/
void
_clay_resource_chooser_set_drag_target (ClayResourceChooser  *chooser,
                                        GtkWidget            *drag_region_widget,
                                        const GtkTargetEntry *drag_target)
{
#ifdef PLATFORM_OSX
  GtkTargetEntry  quartz_target;
  gchar          *quartz_name;
#endif

  g_return_if_fail (CLAY_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (drag_target != NULL);
  g_return_if_fail (drag_region_widget != NULL);

#ifdef PLATFORM_OSX
  /* on macOS, GDK turns any custom target containing a "/" to a UTI-like
     format, but not the other way around. So, rewrite "/" to "-" to work
     (see app/widgets/claydnd.c) */
  quartz_target = *drag_target;
  quartz_name   = g_strdup (drag_target->target);

  g_strdelimit (quartz_name, "/", '-');
  quartz_target.target = quartz_name;
#endif

  gtk_drag_dest_set (drag_region_widget,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
#ifndef PLATFORM_OSX
                     drag_target, 1,  /* Pass array of size 1 */
#else
                     &quartz_target, 1,
#endif
                     GDK_ACTION_COPY);

#ifdef PLATFORM_OSX
  g_free (quartz_name);
#endif

  /* connect drag_region_widget's drag_received signal to chooser's callback. */
  g_signal_connect_swapped (drag_region_widget, "drag-data-received",
                            G_CALLBACK (clay_resource_select_drag_data_received),
                            chooser);
}

/**
 * _clay_resource_chooser_set_clickable:
 * @chooser: A [class@ResourceChooser]
 * @widget:  An interior widget that emits "clicked" signal
 *
 * Called by a subclass init to specialize the instance.
 *
 * Subclass knows its interior widget whose region when clicked
 * should popup remote chooser.
 * Self handles the click event.
 *
 * Since: 3.0
 **/
void
_clay_resource_chooser_set_clickable (ClayResourceChooser *chooser,
                                      GtkWidget           *widget)
{
  ClayResourceChooserPrivate *priv;

  g_return_if_fail (CLAY_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  priv = clay_resource_chooser_get_instance_private (chooser);
  /* Require the widget have a signal "clicked", usually a button. */
  g_signal_connect_swapped (widget, "clicked",
                            G_CALLBACK (clay_resource_chooser_clicked),
                            chooser);

  /* To make this activatable with mnemonics, we need to attach to
   * a widget that can get focus */
  if (priv->label_widget != NULL)
    gtk_label_set_mnemonic_widget (GTK_LABEL (priv->label_widget), widget);
}

/**
 * clay_resource_chooser_get_resource:
 * @chooser: A #ClayResourceChooser
 *
 * Gets the currently selected resource.
 *
 * Returns: (transfer none): an internal copy of the resource which must not be freed.
 *
 * Since: 3.0
 */
ClayResource *
clay_resource_chooser_get_resource (ClayResourceChooser *chooser)
{
  ClayResourceChooserPrivate *priv;

  g_return_val_if_fail (CLAY_IS_RESOURCE_CHOOSER (chooser), NULL);

  priv = clay_resource_chooser_get_instance_private (chooser);

  return priv->resource;
}

/**
 * clay_resource_chooser_set_resource:
 * @chooser: A #ClayResourceChooser
 * @resource: Resource to set.
 *
 * Sets the currently selected resource.
 * This will select the resource in both the button and any chooser popup.
 *
 * Since: 3.0
 */
void
clay_resource_chooser_set_resource (ClayResourceChooser *chooser,
                                    ClayResource        *resource)
{
  ClayResourceChooserPrivate *priv;

  g_return_if_fail (CLAY_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (resource != NULL);

  priv = clay_resource_chooser_get_instance_private (chooser);

  if (priv->callback)
    {
      /* A popup chooser dialog is already shown.
       * Call its setter to change the selection there
       * (since all views of the resource must be consistent.)
       * That will call back, which will change our own view of the resource.
       */
      clay_resource_chooser_set_remote_dialog (chooser, resource);
    }
  else
    {
      /* Call our own setter. */
      clay_resource_chooser_callback (resource, FALSE, chooser);
    }
}

/**
 * clay_resource_chooser_get_label:
 * @widget: A [class@ResourceChooser].
 *
 * Returns the label widget.
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 * Since: 3.0
 */
GtkWidget *
clay_resource_chooser_get_label (ClayResourceChooser *widget)
{
  ClayResourceChooserPrivate *priv;

  g_return_val_if_fail (CLAY_IS_RESOURCE_CHOOSER (widget), NULL);

  priv = clay_resource_chooser_get_instance_private (widget);
  return priv->label_widget;
}


/*  private functions  */

static void
clay_resource_chooser_set_remote_dialog (ClayResourceChooser *self,
                                         ClayResource        *resource)
{
  ClayResourceChooserPrivate *priv;
  ClayResourceChooserClass   *klass;

  g_return_if_fail (CLAY_IS_RESOURCE_CHOOSER (self));
  g_return_if_fail (resource != NULL);

  priv  = clay_resource_chooser_get_instance_private (self);
  klass = CLAY_RESOURCE_CHOOSER_GET_CLASS (self);

  g_return_if_fail (klass->resource_type != G_TYPE_INVALID);
  g_return_if_fail (klass->resource_type == G_TYPE_FROM_INSTANCE (resource));

  clay_resource_select_set (priv->callback, resource);
}

static void
clay_resource_chooser_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *gvalue,
                                    GParamSpec   *pspec)
{
  ClayResourceChooser        *self = CLAY_RESOURCE_CHOOSER (object);
  ClayResourceChooserPrivate *priv = clay_resource_chooser_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (gvalue);
      break;

    case PROP_LABEL:
      priv->label = g_value_dup_string (gvalue);
      break;

    case PROP_RESOURCE:
      clay_resource_chooser_set_resource (self, g_value_get_object (gvalue));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_resource_chooser_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayResourceChooser        *self = CLAY_RESOURCE_CHOOSER (object);
  ClayResourceChooserPrivate *priv = clay_resource_chooser_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;

    case PROP_RESOURCE:
      g_value_set_object (value, priv->resource);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* A callback from the remote resource select popup.
 * When user chooses a resource.
 * Via a temporary PDB procedure.
 *
 * Set self's model (priv->resource)
 * Notify any parent widget subscribed on the "resource" property
 * typically a prop widget.
 * Update the view, since model changed.
 */
static void
clay_resource_chooser_callback (ClayResource *resource,
                                gboolean      dialog_closing,
                                gpointer      user_data)
{
  ClayResourceChooser        *self = CLAY_RESOURCE_CHOOSER (user_data);
  ClayResourceChooserPrivate *priv = clay_resource_chooser_get_instance_private (self);

  priv->resource = resource;

  CLAY_RESOURCE_CHOOSER_GET_CLASS (self)->draw_interior (self);

  if (dialog_closing)
    g_clear_pointer (&priv->callback, g_free);

  g_signal_emit (self, resource_button_signals[RESOURCE_SET], 0, resource, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (self), resource_button_props[PROP_RESOURCE]);
}

static void
clay_resource_chooser_clicked (ClayResourceChooser *self)
{
  ClayResourceChooserPrivate *priv  = clay_resource_chooser_get_instance_private (self);
  ClayResourceChooserClass   *klass = CLAY_RESOURCE_CHOOSER_GET_CLASS (self);

  if (priv->callback)
    {
      /* Popup already created.  Calling setter raises the popup. */
      clay_resource_chooser_set_remote_dialog (self, priv->resource);
    }
  else
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
      GBytes    *handle   = NULL;
#ifdef PLATFORM_OSX
      NSRunningApplication *clay_app;
#endif

      if (CLAY_IS_DIALOG (toplevel))
        handle = clay_dialog_get_native_handle (CLAY_DIALOG (toplevel));

      priv->callback = g_strdup (clay_resource_select_new (priv->title,
                                                           handle,
                                                           priv->resource,
                                                           klass->resource_type,
                                                           clay_resource_chooser_callback,
                                                           self,
                                                           NULL));
      clay_resource_chooser_set_remote_dialog (self, priv->resource);

#ifdef PLATFORM_OSX
      /* activate the parent Clay process so the resource dialog appears on top */
      clay_app = [NSRunningApplication runningApplicationWithProcessIdentifier:getppid()];
      if (clay_app)
        {
          if (@available(macOS 14.0, *))
            {
              [[NSApplication sharedApplication] yieldActivationToApplication:clay_app];
              [clay_app activateFromApplication:[NSRunningApplication currentApplication] options:0];
            }
          else
            {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
              [clay_app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
#pragma clang diagnostic pop
            }
        }
#endif
    }
}


/* Drag methods. */

static void
clay_resource_select_drag_data_received (ClayResourceChooser *self,
                                         GdkDragContext      *context,
                                         gint                 x,
                                         gint                 y,
                                         GtkSelectionData    *selection,
                                         guint                info,
                                         guint                time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  ClayResourceChooserClass *klass;

  klass = CLAY_RESOURCE_CHOOSER_GET_CLASS (self);
  /* Require class resource_type was initialized. */
  g_assert (klass->resource_type != 0);

  /* Drag data is a string that is the ID of the resource. */

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid resource data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

#ifndef _UCRT
      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == clay_getpid () && name_offset > 0)
#else
      if (sscanf_s (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == clay_getpid () && name_offset > 0)
#endif
        {
          gchar        *name = str + name_offset;
          ClayResource *resource;

          resource = clay_resource_get_by_name (klass->resource_type, name);
          clay_resource_chooser_set_resource (self, resource);
        }
    }

  g_free (str);
}
