/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerview.c
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claymarshal.h"
#include "core/claytreehandler.h"
#include "core/clayviewable.h"

#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claycontainerview-private.h"
#include "claydnd.h"
#include "clayviewrenderer.h"


enum
{
  SELECTION_CHANGED,
  ITEM_ACTIVATED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   clay_container_view_real_selection_changed
                                                     (ClayContainerView *view);
static void   clay_container_view_real_item_activated
                                                     (ClayContainerView *view,
                                                      ClayViewable      *item);
static void   clay_container_view_real_set_container (ClayContainerView *view,
                                                      ClayContainer     *container);
static void   clay_container_view_real_set_context   (ClayContainerView *view,
                                                      ClayContext       *context);
static void   clay_container_view_real_set_selection_mode
                                                     (ClayContainerView *view,
                                                      GtkSelectionMode   mode);
static gboolean
              clay_container_view_real_set_selected  (ClayContainerView  *view,
                                                      GList              *list);
static gint   clay_container_view_real_get_selected  (ClayContainerView  *view,
                                                      GList             **list);

static void   clay_container_view_container_freeze   (ClayContainerView  *view,
                                                      ClayContainer      *container);
static void   clay_container_view_container_thaw     (ClayContainerView  *view,
                                                      ClayContainer      *container);

static void   clay_container_view_connect_context    (ClayContainerView *view);
static void   clay_container_view_disconnect_context (ClayContainerView *view);
static void   clay_container_view_context_changed    (ClayContext        *context,
                                                      ClayViewable       *viewable,
                                                      ClayContainerView  *view);

static void   clay_container_view_viewable_dropped   (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      ClayViewable       *viewable,
                                                      gpointer            data);
static void   clay_container_view_button_viewables_dropped
                                                     (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      GList              *viewables,
                                                      gpointer            data);
static void   clay_container_view_button_viewable_dropped
                                                     (GtkWidget          *widget,
                                                      gint                x,
                                                      gint                y,
                                                      ClayViewable       *viewable,
                                                      gpointer            data);


G_DEFINE_INTERFACE (ClayContainerView, clay_container_view, GTK_TYPE_WIDGET)


static guint view_signals[LAST_SIGNAL] = { 0 };


static void
clay_container_view_default_init (ClayContainerViewInterface *iface)
{
  view_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContainerViewInterface, selection_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  view_signals[ITEM_ACTIVATED] =
    g_signal_new ("item-activated",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContainerViewInterface, item_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_OBJECT);

  iface->selection_changed  = clay_container_view_real_selection_changed;
  iface->item_activated     = clay_container_view_real_item_activated;

  iface->set_container      = clay_container_view_real_set_container;
  iface->set_context        = clay_container_view_real_set_context;
  iface->set_selection_mode = clay_container_view_real_set_selection_mode;
  iface->set_view_size      = NULL;
  iface->set_selected       = clay_container_view_real_set_selected;
  iface->get_selected       = clay_container_view_real_get_selected;

  iface->insert_item        = NULL;
  iface->insert_items_after = NULL;
  iface->remove_item        = NULL;
  iface->reorder_item       = NULL;
  iface->rename_item        = NULL;
  iface->expand_item        = NULL;
  iface->clear_items        = _clay_container_view_real_clear_items;

  iface->insert_data_free   = NULL;
  iface->model_is_tree      = FALSE;
  iface->use_list_model     = FALSE;

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("container",
                                                            NULL, NULL,
                                                            CLAY_TYPE_CONTAINER,
                                                            CLAY_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            CLAY_TYPE_CONTEXT,
                                                            CLAY_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("selection-mode",
                                                          NULL, NULL,
                                                          GTK_TYPE_SELECTION_MODE,
                                                          GTK_SELECTION_SINGLE,
                                                          CLAY_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("reorderable",
                                                             NULL, NULL,
                                                             FALSE,
                                                             CLAY_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-size",
                                                         NULL, NULL,
                                                         1, CLAY_VIEWABLE_MAX_PREVIEW_SIZE,
                                                         CLAY_VIEW_SIZE_MEDIUM,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_int ("view-border-width",
                                                         NULL, NULL,
                                                         0,
                                                         CLAY_VIEW_MAX_BORDER_WIDTH,
                                                         1,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}


/*  public functions  */

ClayContainer *
clay_container_view_get_container (ClayContainerView *view)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), NULL);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->container;
}

void
clay_container_view_set_container (ClayContainerView *view,
                                   ClayContainer     *container)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (container == NULL || CLAY_IS_CONTAINER (container));
  g_return_if_fail (container == NULL ||
                    g_type_is_a (clay_container_get_child_type (container),
                                 CLAY_TYPE_VIEWABLE));

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (container != private->container)
    {
      if (private->context && private->container)
        {
          clay_container_view_disconnect_context (view);
        }

      CLAY_CONTAINER_VIEW_GET_IFACE (view)->set_container (view, container);

      if (private->context && private->container)
        {
          clay_container_view_connect_context (view);
        }

      g_object_notify (G_OBJECT (view), "container");
    }
}

ClayContext *
clay_container_view_get_context (ClayContainerView *view)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), NULL);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->context;
}

void
clay_container_view_set_context (ClayContainerView *view,
                                 ClayContext       *context)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (context != private->context)
    {
      if (private->context && private->container)
        {
          clay_container_view_disconnect_context (view);
        }

      CLAY_CONTAINER_VIEW_GET_IFACE (view)->set_context (view, context);

      if (private->context && private->container)
        {
          clay_container_view_connect_context (view);
        }

      g_object_notify (G_OBJECT (view), "context");
    }
}

GtkSelectionMode
clay_container_view_get_selection_mode (ClayContainerView *view)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->selection_mode;
}

void
clay_container_view_set_selection_mode (ClayContainerView *view,
                                        GtkSelectionMode   mode)
{
  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (mode == GTK_SELECTION_SINGLE ||
                    mode == GTK_SELECTION_MULTIPLE);

  CLAY_CONTAINER_VIEW_GET_IFACE (view)->set_selection_mode (view, mode);
}

gint
clay_container_view_get_view_size (ClayContainerView *view,
                                   gint              *view_border_width)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), 0);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (view_border_width)
    *view_border_width = private->view_border_width;

  return private->view_size;
}

void
clay_container_view_set_view_size (ClayContainerView *view,
                                   gint               view_size,
                                   gint               view_border_width)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (view_size >  0 &&
                    view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->view_size         != view_size ||
      private->view_border_width != view_border_width)
    {
      private->view_size         = view_size;
      private->view_border_width = view_border_width;

      CLAY_CONTAINER_VIEW_GET_IFACE (view)->set_view_size (view);

      g_object_freeze_notify (G_OBJECT (view));
      g_object_notify (G_OBJECT (view), "view-size");
      g_object_notify (G_OBJECT (view), "view-border-width");
      g_object_thaw_notify (G_OBJECT (view));
    }
}

gboolean
clay_container_view_get_reorderable (ClayContainerView *view)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), FALSE);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->reorderable;
}

void
clay_container_view_set_reorderable (ClayContainerView *view,
                                     gboolean           reorderable)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  private->reorderable = reorderable ? TRUE : FALSE;
  g_object_notify (G_OBJECT (view), "reorderable");
}

GtkWidget *
clay_container_view_get_dnd_widget (ClayContainerView *view)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), NULL);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  return private->dnd_widget;
}

void
clay_container_view_set_dnd_widget (ClayContainerView *view,
                                    GtkWidget         *dnd_widget)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (dnd_widget == NULL || GTK_IS_WIDGET (dnd_widget));

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  private->dnd_widget = dnd_widget;
}

void
clay_container_view_enable_dnd (ClayContainerView *view,
                                GtkButton         *button,
                                GType              child_type)
{
  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (GTK_IS_BUTTON (button));

  clay_dnd_viewable_list_dest_add (GTK_WIDGET (button),
                                   child_type,
                                   clay_container_view_button_viewables_dropped,
                                   view);
  clay_dnd_viewable_dest_add (GTK_WIDGET (button),
                              child_type,
                              clay_container_view_button_viewable_dropped,
                              view);
}

gboolean
clay_container_view_set_selected (ClayContainerView *view,
                                  GList             *viewables)
{
  ClayContainerViewPrivate *private;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), FALSE);

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (clay_container_frozen (private->container))
    return FALSE;

  return CLAY_CONTAINER_VIEW_GET_IFACE (view)->set_selected (view, viewables);
}

gboolean
clay_container_view_set_1_selected (ClayContainerView *view,
                                    ClayViewable      *viewable)
{
  GList    *viewables = NULL;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), FALSE);

  if (viewable)
    viewables = g_list_prepend (viewables, viewable);

  success = clay_container_view_set_selected (view, viewables);

  g_list_free (viewables);

  return success;
}

/**
 * clay_container_view_get_selected:
 * @view:
 * @items:
 *
 * Get the selected items in @view.
 *
 * If @items is not %NULL, fills it with a newly allocated #GList of the
 * selected items.
 *
 * Note that by default, the interface only implements some basic single
 * selection. Override select_items() signal to get more complete
 * selection support.
 *
 * Returns: the number of selected items.
 */
gint
clay_container_view_get_selected (ClayContainerView  *view,
                                  GList             **items)
{
  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), 0);

  if (items)
    *items = NULL;

  return CLAY_CONTAINER_VIEW_GET_IFACE (view)->get_selected (view, items);
}

ClayViewable *
clay_container_view_get_1_selected (ClayContainerView *view)
{
  ClayViewable *item = NULL;
  GList        *items;

  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (view), NULL);

  if (clay_container_view_get_selected (view, &items) == 1)
    item = items->data;

  g_list_free (items);

  return item;
}

gboolean
clay_container_view_is_item_selected (ClayContainerView *view,
                                      ClayViewable      *viewable)
{
  GList    *items;
  gboolean  found;

  clay_container_view_get_selected (view, &items);
  found = (g_list_find (items, viewable) != NULL);

  g_list_free (items);

  return found;
}


/*  protected functions, only to be used by implementors  */

void
_clay_container_view_selection_changed (ClayContainerView *view)
{
  ClayContainerViewPrivate *private;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));

  private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (! clay_container_frozen (private->container))
    g_signal_emit (view, view_signals[SELECTION_CHANGED], 0);
}

void
_clay_container_view_item_activated (ClayContainerView *view,
                                     ClayViewable      *viewable)
{
  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));
  g_return_if_fail (CLAY_IS_VIEWABLE (viewable));

  g_signal_emit (view, view_signals[ITEM_ACTIVATED], 0,
                 viewable);
}

/**
 * clay_container_view_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #ClayContainerView. A #ClayContainerViewProp property is installed
 * for each property, using the values from the #ClayContainerViewProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %CLAY_CONTAINER_VIEW_PROP_LAST is good for).
 **/
void
_clay_container_view_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_CONTAINER,
                                    "container");
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_CONTEXT,
                                    "context");
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_SELECTION_MODE,
                                    "selection-mode");
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_REORDERABLE,
                                    "reorderable");
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE,
                                    "view-size");
  g_object_class_override_property (klass,
                                    CLAY_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
                                    "view-border-width");
}

void
_clay_container_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case CLAY_CONTAINER_VIEW_PROP_CONTAINER:
      clay_container_view_set_container (view, g_value_get_object (value));
      break;
    case CLAY_CONTAINER_VIEW_PROP_CONTEXT:
      clay_container_view_set_context (view, g_value_get_object (value));
      break;
    case CLAY_CONTAINER_VIEW_PROP_SELECTION_MODE:
      clay_container_view_set_selection_mode (view, g_value_get_enum (value));
      break;
    case CLAY_CONTAINER_VIEW_PROP_REORDERABLE:
      clay_container_view_set_reorderable (view, g_value_get_boolean (value));
      break;
    case CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case CLAY_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = clay_container_view_get_view_size (view, &border);

        if (property_id == CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE)
          size = g_value_get_int (value);
        else
          border = g_value_get_int (value);

        clay_container_view_set_view_size (view, size, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
_clay_container_view_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (object);

  switch (property_id)
    {
    case CLAY_CONTAINER_VIEW_PROP_CONTAINER:
      g_value_set_object (value, clay_container_view_get_container (view));
      break;
    case CLAY_CONTAINER_VIEW_PROP_CONTEXT:
      g_value_set_object (value, clay_container_view_get_context (view));
      break;
    case CLAY_CONTAINER_VIEW_PROP_SELECTION_MODE:
      g_value_set_enum (value, clay_container_view_get_selection_mode (view));
      break;
    case CLAY_CONTAINER_VIEW_PROP_REORDERABLE:
      g_value_set_boolean (value, clay_container_view_get_reorderable (view));
      break;
    case CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE:
    case CLAY_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = clay_container_view_get_view_size (view, &border);

        if (property_id == CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE)
          g_value_set_int (value, size);
        else
          g_value_set_int (value, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  Private functions  */

static void
clay_container_view_real_selection_changed (ClayContainerView *view)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container && private->context)
    {
      GType child_type = clay_container_get_child_type (private->container);

      if (clay_context_type_to_signal_name (child_type))
        {
          GList *items = NULL;
          gint   n_items;

          n_items = clay_container_view_get_selected (view, &items);

          if (n_items == 1 &&
              clay_context_get_by_type (private->context, child_type) !=
              CLAY_OBJECT (items->data))
            {
              ClayContext *context;

              /*  ref and remember the context because
               *  private->context may become NULL by calling
               *  clay_context_set_by_type()
               */
              context = g_object_ref (private->context);

              g_signal_handlers_block_by_func (context,
                                               clay_container_view_context_changed,
                                               view);

              clay_context_set_by_type (context, child_type, items->data);

              g_signal_handlers_unblock_by_func (context,
                                                 clay_container_view_context_changed,
                                                 view);

              g_object_unref (context);
            }

          g_list_free (items);
        }
    }
}

static void
clay_container_view_real_item_activated (ClayContainerView *view,
                                         ClayViewable      *item)
{
}

static void
clay_container_view_real_set_container (ClayContainerView *view,
                                        ClayContainer     *container)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->container)
    {
      g_signal_handlers_disconnect_by_func (private->container,
                                            clay_container_view_container_freeze,
                                            view);
      g_signal_handlers_disconnect_by_func (private->container,
                                            clay_container_view_container_thaw,
                                            view);

      if (! CLAY_CONTAINER_VIEW_GET_IFACE (view)->use_list_model)
        _clay_container_view_disconnect_cruft (view);
    }

  private->container = container;

  if (private->container)
    {
      if (! CLAY_CONTAINER_VIEW_GET_IFACE (view)->use_list_model)
        _clay_container_view_connect_cruft (view);

      g_signal_connect_object (private->container, "freeze",
                               G_CALLBACK (clay_container_view_container_freeze),
                               view,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (private->container, "thaw",
                               G_CALLBACK (clay_container_view_container_thaw),
                               view,
                               G_CONNECT_SWAPPED);
    }
}

static void
clay_container_view_real_set_context (ClayContainerView *view,
                                      ClayContext       *context)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  g_set_object (&private->context, context);
}

static void
clay_container_view_real_set_selection_mode (ClayContainerView *view,
                                             GtkSelectionMode   mode)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  private->selection_mode = mode;
}

static gboolean
clay_container_view_real_set_selected (ClayContainerView *view,
                                       GList             *items)
{
  return FALSE;
}

static gint
clay_container_view_real_get_selected (ClayContainerView  *view,
                                       GList             **items)
{
  if (*items)
    *items = NULL;

  return 0;
}

static void
clay_container_view_container_freeze (ClayContainerView *view,
                                      ClayContainer     *container)
{
}

static void
clay_container_view_container_thaw (ClayContainerView *view,
                                    ClayContainer     *container)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (private->context)
    {
      GType        child_type;
      const gchar *signal_name;

      child_type  = clay_container_get_child_type (private->container);
      signal_name = clay_context_type_to_signal_name (child_type);

      if (signal_name)
        {
          ClayObject *object;

          object = clay_context_get_by_type (private->context, child_type);

          clay_container_view_set_1_selected (view, CLAY_VIEWABLE (object));
        }
    }
}

static void
clay_container_view_connect_context (ClayContainerView *view)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     child_type;
  const gchar              *signal_name;

  child_type  = clay_container_get_child_type (private->container);
  signal_name = clay_context_type_to_signal_name (child_type);

  if (signal_name)
    {
      g_signal_connect_object (private->context, signal_name,
                               G_CALLBACK (clay_container_view_context_changed),
                               view,
                               0);

      if (private->dnd_widget)
        clay_dnd_viewable_dest_add (private->dnd_widget,
                                    child_type,
                                    clay_container_view_viewable_dropped,
                                    view);

      if (! clay_container_frozen (private->container))
        {
          ClayObject *object = clay_context_get_by_type (private->context,
                                                         child_type);

          clay_container_view_set_1_selected (view, CLAY_VIEWABLE (object));
        }
    }
}

static void
clay_container_view_disconnect_context (ClayContainerView *view)
{
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);
  GType                     child_type;
  const gchar              *signal_name;

  child_type  = clay_container_get_child_type (private->container);
  signal_name = clay_context_type_to_signal_name (child_type);

  if (signal_name)
    {
      g_signal_handlers_disconnect_by_func (private->context,
                                            clay_container_view_context_changed,
                                            view);

      if (private->dnd_widget)
        {
          gtk_drag_dest_unset (private->dnd_widget);
          clay_dnd_viewable_dest_remove (private->dnd_widget,
                                         child_type);
        }
    }
}

static void
clay_container_view_context_changed (ClayContext       *context,
                                     ClayViewable      *viewable,
                                     ClayContainerView *view)
{
  clay_container_view_set_1_selected (view, viewable);
}

static void
clay_container_view_viewable_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      ClayViewable *viewable,
                                      gpointer      data)
{
  ClayContainerView        *view    = CLAY_CONTAINER_VIEW (data);
  ClayContainerViewPrivate *private = CLAY_CONTAINER_VIEW_GET_PRIVATE (view);

  if (viewable && private->container &&
      clay_container_have (private->container, CLAY_OBJECT (viewable)))
    {
      clay_container_view_set_1_selected (view, viewable);
    }
}

static void
clay_container_view_button_viewables_dropped (GtkWidget *widget,
                                              gint        x,
                                              gint        y,
                                              GList      *viewables,
                                              gpointer    data)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (data);

  if (viewables && clay_container_view_set_selected (view, viewables))
    {
      gtk_button_clicked (GTK_BUTTON (widget));
    }
}

static void
clay_container_view_button_viewable_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             ClayViewable *viewable,
                                             gpointer      data)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (data);

  if (viewable && clay_container_view_set_1_selected (view, viewable))
    {
      gtk_button_clicked (GTK_BUTTON (widget));
    }
}
