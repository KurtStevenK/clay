/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrow.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claydnd.h"
#include "clayeditor.h"
#include "clayrow.h"
#include "clayview.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


enum
{
  EDIT_NAME,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_VIEWABLE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


typedef struct _ClayRowPrivate ClayRowPrivate;

struct _ClayRowPrivate
{
  ClayContext  *context;
  ClayViewable *viewable;

  gint          view_size;
  gint          view_border_width;

  gint          n_toggles;

  GtkWidget    *box;

  GtkWidget    *icon;
  GtkWidget    *view;
  GtkWidget    *label;

  GtkWidget    *rename_popover;
  GtkGesture   *rename_long_press;
};

#define GET_PRIVATE(obj) \
  ((ClayRowPrivate *) clay_row_get_instance_private ((ClayRow *) obj))


static void       clay_row_constructed           (GObject          *object);
static void       clay_row_dispose               (GObject          *object);
static void       clay_row_set_property          (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       clay_row_get_property          (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static gboolean   clay_row_button_press_event    (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static void       clay_row_style_updated         (GtkWidget        *widget);
static gboolean   clay_row_query_tooltip         (GtkWidget        *widget,
                                                  gint              x,
                                                  gint              y,
                                                  gboolean          keyboard_tooltip,
                                                  GtkTooltip       *tooltip);

static void       clay_row_real_set_context      (ClayRow          *row,
                                                  ClayContext      *context);
static void       clay_row_real_set_viewable     (ClayRow          *row,
                                                  ClayViewable     *viewable);
static void       clay_row_real_set_view_size    (ClayRow          *row);
static void       clay_row_real_icon_changed     (ClayRow          *row);
static void       clay_row_real_name_changed     (ClayRow          *row);
static void       clay_row_real_monitor_changed  (ClayRow          *row);
static void       clay_row_real_edit_name        (ClayRow          *row);
static gboolean   clay_row_real_name_edited      (ClayRow          *row,
                                                  const gchar      *new_name);

static void       clay_row_icon_changed          (ClayViewable     *viewable,
                                                  const GParamSpec *pspec,
                                                  ClayRow          *row);
static void       clay_row_name_changed          (ClayViewable     *viewable,
                                                  ClayRow          *row);

static void       clay_row_label_long_pressed    (GtkGesture       *gesture,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  ClayRow          *row);
static void       clay_row_rename_entry_activate (GtkEntry         *entry,
                                                  ClayRow          *row);

static ClayViewable *
                  clay_row_drag_viewable         (GtkWidget        *widget,
                                                  ClayContext     **context,
                                                  gpointer          data);
static GdkPixbuf *clay_row_drag_pixbuf           (GtkWidget        *widget,
                                                  gpointer          data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayRow, clay_row, GTK_TYPE_LIST_BOX_ROW)

#define parent_class clay_row_parent_class

static guint row_signals[LAST_SIGNAL] = { 0 };


static void
clay_row_class_init (ClayRowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  object_class->constructed        = clay_row_constructed;
  object_class->dispose            = clay_row_dispose;
  object_class->set_property       = clay_row_set_property;
  object_class->get_property       = clay_row_get_property;

  widget_class->button_press_event = clay_row_button_press_event;
  widget_class->style_updated      = clay_row_style_updated;
  widget_class->query_tooltip      = clay_row_query_tooltip;

  klass->set_context               = clay_row_real_set_context;
  klass->set_viewable              = clay_row_real_set_viewable;
  klass->set_view_size             = clay_row_real_set_view_size;
  klass->icon_changed              = clay_row_real_icon_changed;
  klass->name_changed              = clay_row_real_name_changed;
  klass->monitor_changed           = clay_row_real_monitor_changed;
  klass->edit_name                 = clay_row_real_edit_name;
  klass->name_edited               = clay_row_real_name_edited;

  row_signals[EDIT_NAME] =
    g_signal_new ("edit-name",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ClayRowClass, edit_name),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_F2, 0,
                                "edit-name", 0);

  obj_props[PROP_CONTEXT] =
    g_param_spec_object ("context",
                         NULL, NULL,
                         CLAY_TYPE_CONTEXT,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEWABLE] =
    g_param_spec_object ("viewable",
                         NULL, NULL,
                         CLAY_TYPE_VIEWABLE,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEW_SIZE] =
    g_param_spec_int ("view-size",
                      NULL, NULL,
                      1, CLAY_VIEWABLE_MAX_PREVIEW_SIZE,
                      CLAY_VIEW_SIZE_MEDIUM,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEW_BORDER_WIDTH] =
    g_param_spec_int ("view-border-width",
                      NULL, NULL,
                      0, CLAY_VIEW_MAX_BORDER_WIDTH,
                      1,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
clay_row_init (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);
  GtkWidget      *ebox;

  gtk_widget_set_has_tooltip (GTK_WIDGET (row), TRUE);

  priv->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (row), priv->box);
  gtk_widget_set_visible (priv->box, TRUE);

  priv->icon = gtk_image_new_from_icon_name (NULL, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->icon, FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->icon, TRUE);

  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (priv->box), ebox, TRUE, TRUE, 0);
  gtk_widget_set_visible (ebox, TRUE);

  priv->label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);
  gtk_container_add (GTK_CONTAINER (ebox), priv->label);
  gtk_widget_set_visible (priv->label, TRUE);

  priv->rename_long_press = gtk_gesture_long_press_new (ebox);
  gtk_event_controller_set_propagation_phase
    (GTK_EVENT_CONTROLLER (priv->rename_long_press),
     GTK_PHASE_TARGET);

  g_signal_connect (priv->rename_long_press, "pressed",
                    G_CALLBACK (clay_row_label_long_pressed),
                    row);
}

static void
clay_row_constructed (GObject *object)
{
  ClayRowPrivate *priv    = GET_PRIVATE (object);
  gboolean        preview = FALSE;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->view = clay_view_new (priv->context,
                              priv->viewable,
                              priv->view_size,
                              priv->view_border_width,
                              FALSE);
  CLAY_VIEW (priv->view)->eat_button_events = FALSE;
  CLAY_VIEW (priv->view)->show_popup        = TRUE;
  gtk_box_pack_start (GTK_BOX (priv->box), priv->view, FALSE, FALSE, 0);

  if (priv->viewable)
    preview = clay_viewable_has_preview (priv->viewable);

  gtk_widget_set_visible (priv->icon, ! preview);
  gtk_widget_set_visible (priv->view, preview);
}

static void
clay_row_dispose (GObject *object)
{
  ClayRowPrivate *priv = GET_PRIVATE (object);

  clay_row_set_viewable (CLAY_ROW (object), NULL);

  g_clear_pointer (&priv->rename_popover, gtk_widget_destroy);
  g_clear_object  (&priv->rename_long_press);

  clay_row_set_context  (CLAY_ROW (object), NULL);
  clay_row_set_viewable (CLAY_ROW (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_row_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  ClayRow *row = CLAY_ROW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      clay_row_set_context (row, g_value_get_object (value));
      break;
    case PROP_VIEWABLE:
      clay_row_set_viewable (row, g_value_get_object (value));
      break;
    case PROP_VIEW_SIZE:
    case PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = clay_row_get_view_size (row, &border);

        if (property_id == PROP_VIEW_SIZE)
          size = g_value_get_int (value);
        else
          border = g_value_get_int (value);

        clay_row_set_view_size (row, size, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_row_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  ClayRow *row = CLAY_ROW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, clay_row_get_context (row));
      break;
    case PROP_VIEWABLE:
      g_value_set_object (value, clay_row_get_viewable (row));
      break;
    case PROP_VIEW_SIZE:
    case PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = clay_row_get_view_size (row, &border);

        if (property_id == PROP_VIEW_SIZE)
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

static gboolean
clay_row_button_press_event (GtkWidget      *widget,
                             GdkEventButton *bevent)
{
  GdkEvent *event = (GdkEvent *) bevent;

  GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  if (gdk_event_triggers_context_menu (event))
    {
      GtkWidget *list_box = gtk_widget_get_parent (widget);

      if (GTK_IS_LIST_BOX (list_box))
        {
          GtkWidget *editor = gtk_widget_get_ancestor (list_box,
                                                       CLAY_TYPE_EDITOR);

          if (editor)
            {
              if (! gtk_list_box_row_is_selected (GTK_LIST_BOX_ROW (widget)))
                {
                  gtk_list_box_select_row (GTK_LIST_BOX (list_box),
                                           GTK_LIST_BOX_ROW (widget));
                 }

              return clay_editor_popup_menu_at_pointer (CLAY_EDITOR (editor),
                                                        event);
            }
        }
    }

  return FALSE;
}

static void
clay_row_style_updated (GtkWidget *widget)
{
  ClayRowPrivate *priv = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (priv->view)
    clay_view_renderer_invalidate (CLAY_VIEW (priv->view)->renderer);
}

static gboolean
clay_row_query_tooltip (GtkWidget  *widget,
                        gint        x,
                        gint        y,
                        gboolean    keyboard_tooltip,
                        GtkTooltip *tooltip)
{
  ClayRowPrivate *priv     = GET_PRIVATE (widget);
  gboolean        show_tip = FALSE;

  if (priv->viewable)
    {
      gchar *desc;
      gchar *tip;

      desc = clay_viewable_get_description (priv->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);

          show_tip = TRUE;

          g_free (tip);
        }

      g_free (desc);
    }

  return show_tip;
}

static void
clay_row_real_set_context (ClayRow     *row,
                           ClayContext *context)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_set_object (&priv->context, context);

  if (priv->view)
    clay_view_renderer_set_context (CLAY_VIEW (priv->view)->renderer,
                                    priv->context);
}

static void
clay_row_real_set_viewable (ClayRow      *row,
                            ClayViewable *viewable)
{
  ClayRowPrivate *priv          = GET_PRIVATE (row);
  GType           viewable_type = G_TYPE_NONE;
  gboolean        preview       = FALSE;

  if (viewable)
    viewable_type = G_TYPE_FROM_INSTANCE (viewable);

  if (priv->viewable)
    {
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            clay_row_icon_changed,
                                            row);
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            clay_row_name_changed,
                                            row);

      if (! viewable)
        {
          if (clay_dnd_viewable_source_remove (GTK_WIDGET (row),
                                               G_TYPE_FROM_INSTANCE (priv->viewable)))
            {
              if (clay_viewable_get_size (priv->viewable, NULL, NULL))
                clay_dnd_pixbuf_source_remove (GTK_WIDGET (row));

              gtk_drag_source_unset (GTK_WIDGET (row));
            }
        }
    }
  else if (viewable)
    {
      if (clay_dnd_drag_source_set_by_type (GTK_WIDGET (row),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            viewable_type,
                                            GDK_ACTION_COPY))
        {
          clay_dnd_viewable_source_add (GTK_WIDGET (row),
                                        viewable_type,
                                        clay_row_drag_viewable,
                                        NULL);

          if (clay_viewable_get_size (viewable, NULL, NULL))
            clay_dnd_pixbuf_source_add (GTK_WIDGET (row),
                                        clay_row_drag_pixbuf,
                                        NULL);
        }
    }

  g_set_weak_pointer (&priv->viewable, viewable);

  if (priv->viewable)
    {
      g_signal_connect (priv->viewable,
                        CLAY_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                        G_CALLBACK (clay_row_name_changed),
                        row);
      g_signal_connect (priv->viewable, "notify::icon-name",
                        G_CALLBACK (clay_row_icon_changed),
                        row);

      preview = clay_viewable_has_preview (priv->viewable);
    }

  clay_row_name_changed (priv->viewable, row);

  gtk_widget_set_visible (priv->icon, ! preview);
  clay_row_icon_changed (priv->viewable, NULL, row);

  if (priv->view)
    {
      gtk_widget_set_visible (priv->view, preview);
      clay_view_set_viewable (CLAY_VIEW (priv->view), priv->viewable);
    }
}

static void
clay_row_real_set_view_size (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  gtk_image_set_pixel_size (GTK_IMAGE (priv->icon),
                            priv->view_size);

  if (priv->view)
    clay_view_renderer_set_size (CLAY_VIEW (priv->view)->renderer,
                                 priv->view_size,
                                 priv->view_border_width);
}

static void
clay_row_real_icon_changed (ClayRow *row)
{
  ClayRowPrivate *priv      = GET_PRIVATE (row);
  const gchar    *icon_name = NULL;

  if (priv->viewable)
    icon_name = clay_viewable_get_icon_name (priv->viewable);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), icon_name,
                                GTK_ICON_SIZE_BUTTON);
}

static void
clay_row_real_name_changed (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);
  gchar          *desc = NULL;

  if (priv->viewable)
    desc = clay_viewable_get_description (priv->viewable, NULL);

  gtk_label_set_text (GTK_LABEL (priv->label), desc);

  if (desc)
    g_free (desc);
}

static void
clay_row_real_monitor_changed (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  if (priv->view)
    clay_view_renderer_free_color_transform (CLAY_VIEW (priv->view)->renderer);
}

static void
clay_row_real_edit_name (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);
  GtkWidget      *box;
  GtkWidget      *label;
  GtkWidget      *entry;
  const gchar    *default_name;
  gchar          *title;

  if (! priv->viewable ||
      ! clay_viewable_is_name_editable (priv->viewable))
    {
      gtk_widget_error_bell (GTK_WIDGET (row));
      return;
    }

  priv->rename_popover = gtk_popover_new (priv->label);
  gtk_popover_set_modal (GTK_POPOVER (priv->rename_popover), TRUE);

  g_object_add_weak_pointer (G_OBJECT (priv->rename_popover),
                             (gpointer) &priv->rename_popover);

  g_signal_connect (priv->rename_popover, "closed",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (priv->rename_popover), box);
  gtk_widget_set_visible (box, TRUE);

  default_name = CLAY_VIEWABLE_GET_CLASS (priv->viewable)->default_name;

  title = g_strdup_printf (_("Rename %s"), default_name);
  label = gtk_label_new (title);
  g_free (title);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), clay_object_get_name (priv->viewable));
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry, TRUE);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (clay_row_rename_entry_activate),
                    row);

  gtk_popover_popup (GTK_POPOVER (priv->rename_popover));
}

static gboolean
clay_row_real_name_edited (ClayRow     *row,
                           const gchar *new_name)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  clay_object_set_name (CLAY_OBJECT (priv->viewable), new_name);

  return TRUE;
}


/*  public functions  */

GtkWidget *
clay_row_new (GType         row_type,
              ClayContext  *context,
              ClayViewable *viewable,
              gint          view_size,
              gint          view_border_width)
{
  g_return_val_if_fail (g_type_is_a (row_type, CLAY_TYPE_ROW), NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (CLAY_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (row_type,
                       "context",           context,
                       "viewable",          viewable,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

void
clay_row_set_context (ClayRow     *row,
                      ClayContext *context)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (CLAY_IS_ROW (row));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (context != priv->context)
    {
      CLAY_ROW_GET_CLASS (row)->set_context (row, context);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_CONTEXT]);
    }
}

ClayContext *
clay_row_get_context (ClayRow *row)
{
  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return GET_PRIVATE (row)->context;
}

void
clay_row_set_viewable (ClayRow      *row,
                       ClayViewable *viewable)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (CLAY_IS_ROW (row));
  g_return_if_fail (viewable == NULL || CLAY_IS_VIEWABLE (viewable));

  if (viewable != priv->viewable)
    {
      CLAY_ROW_GET_CLASS (row)->set_viewable (row, viewable);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEWABLE]);
    }
}

ClayViewable *
clay_row_get_viewable (ClayRow *row)
{
  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return GET_PRIVATE (row)->viewable;
}

void
clay_row_set_view_size (ClayRow *row,
                        gint     view_size,
                        gint     view_border_width)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (CLAY_IS_ROW (row));
  g_return_if_fail (view_size >  0 &&
                    view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH);

  if (priv->view_size         != view_size ||
      priv->view_border_width != view_border_width)
    {
      priv->view_size         = view_size;
      priv->view_border_width = view_border_width;

      CLAY_ROW_GET_CLASS (row)->set_view_size (row);

      g_object_freeze_notify (G_OBJECT (row));
      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEW_SIZE]);
      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEW_BORDER_WIDTH]);
      g_object_thaw_notify (G_OBJECT (row));
    }
}

gint
clay_row_get_view_size (ClayRow *row,
                        gint    *view_border_width)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW (row), 1);

  if (view_border_width)
    *view_border_width = priv->view_border_width;

  return priv->view_size;
}

void
clay_row_monitor_changed (ClayRow *row)
{
  g_return_if_fail (CLAY_IS_ROW (row));

  CLAY_ROW_GET_CLASS (row)->monitor_changed (row);
}

GtkWidget *
_clay_row_get_box (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return priv->box;
}

GtkWidget *
_clay_row_get_icon (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return priv->icon;
}

GtkWidget *
_clay_row_get_view (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return priv->view;
}

GtkWidget *
_clay_row_get_label (ClayRow *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);

  return priv->label;
}

GtkWidget *
_clay_row_add_toggle (ClayRow      *row,
                      const gchar  *icon_name,
                      GtkWidget   **icon)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);
  GtkWidget      *toggle;
  GtkWidget      *image;

  g_return_val_if_fail (CLAY_IS_ROW (row), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (icon != NULL, NULL);

  toggle = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (priv->box), toggle, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (priv->box), toggle, priv->n_toggles);
  gtk_widget_set_visible (toggle, TRUE);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_set_visible (image, TRUE);

  priv->n_toggles++;

  *icon = image;

  return toggle;
}


/*  private functions  */

static void
clay_row_icon_changed (ClayViewable     *viewable,
                       const GParamSpec *pspec,
                       ClayRow          *row)
{
  CLAY_ROW_GET_CLASS (row)->icon_changed (row);
}

static void
clay_row_name_changed (ClayViewable *viewable,
                       ClayRow      *row)
{
  CLAY_ROW_GET_CLASS (row)->name_changed (row);
}

static void
clay_row_label_long_pressed (GtkGesture *gesture,
                             gdouble     x,
                             gdouble     y,
                             ClayRow    *row)
{
  g_signal_emit (row, row_signals[EDIT_NAME], 0);
}

static void
clay_row_rename_entry_activate (GtkEntry *entry,
                                ClayRow  *row)
{
  ClayRowPrivate *priv = GET_PRIVATE (row);
  const gchar    *old_name;
  gchar          *new_name;

  old_name = clay_object_get_name (priv->viewable);

  /*  Dup the new name, and popdown before renaming, because it can
   *  destroy the row and the popup.
   */
  new_name = g_strdup (gtk_entry_get_text (entry));
  gtk_popover_popdown (GTK_POPOVER (priv->rename_popover));

  if (g_strcmp0 (old_name, new_name))
    {
      if (! CLAY_ROW_GET_CLASS (row)->name_edited (row, new_name))
        {
          gtk_widget_error_bell (GTK_WIDGET (row));
        }
    }

  g_free (new_name);
}

static ClayViewable *
clay_row_drag_viewable (GtkWidget    *widget,
                        ClayContext **context,
                        gpointer      data)
{
  ClayRowPrivate *priv = GET_PRIVATE (widget);

  if (context)
    *context = priv->context;

  return priv->viewable;
}

static GdkPixbuf *
clay_row_drag_pixbuf (GtkWidget *widget,
                      gpointer   data)
{
  ClayRowPrivate *priv = GET_PRIVATE (widget);
  gint            width;
  gint            height;

  g_printerr ("drag pixbuf\n");

  if (priv->viewable &&
      clay_viewable_get_size (priv->viewable, &width, &height))
    {
      return clay_viewable_get_new_pixbuf (priv->viewable,
                                           priv->context,
                                           width, height, 1, NULL);
    }

  g_printerr ("failed\n");

  return NULL;
}
