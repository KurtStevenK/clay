/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfgbgeditor.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-colormap.h"
#include "core/claymarshal.h"
#include "core/claypalette.h"

#include "claydnd.h"
#include "clayfgbgeditor.h"
#include "claywidgets-utils.h"

#define CHANNEL_EPSILON 1e-3

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_ACTIVE_COLOR
};

enum
{
  COLOR_CLICKED,
  COLOR_DROPPED,
  COLORS_SWAPPED,
  COLORS_DEFAULT,
  TOOLTIP,
  LAST_SIGNAL
};


static void     clay_fg_bg_editor_dispose           (GObject          *object);
static void     clay_fg_bg_editor_set_property      (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void     clay_fg_bg_editor_get_property      (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static GtkSizeRequestMode
                clay_fg_bg_editor_get_request_mode  (GtkWidget        *widget);
static void
   clay_fg_bg_editor_get_preferred_width_for_height (GtkWidget        *widget,
                                                     gint              height,
                                                     gint             *minimum_width,
                                                     gint             *natural_width);

static void     clay_fg_bg_editor_size_allocate     (GtkWidget        *editor,
                                                     GtkAllocation    *allocation);
static void     clay_fg_bg_editor_style_updated     (GtkWidget        *widget);
static gboolean clay_fg_bg_editor_draw              (GtkWidget        *widget,
                                                     cairo_t          *cr);
static gboolean clay_fg_bg_editor_button_press      (GtkWidget        *widget,
                                                     GdkEventButton   *bevent);
static gboolean clay_fg_bg_editor_button_release    (GtkWidget        *widget,
                                                     GdkEventButton   *bevent);
static gboolean clay_fg_bg_editor_drag_motion       (GtkWidget        *widget,
                                                     GdkDragContext   *context,
                                                     gint              x,
                                                     gint              y,
                                                     guint             time);
static gboolean clay_fg_bg_editor_query_tooltip     (GtkWidget        *widget,
                                                     gint              x,
                                                     gint              y,
                                                     gboolean          keyboard_mode,
                                                     GtkTooltip       *tooltip);

static void     clay_fg_bg_editor_drag_color        (GtkWidget        *widget,
                                                     GeglColor       **color,
                                                     gpointer          data);
static void     clay_fg_bg_editor_drop_color        (GtkWidget        *widget,
                                                     gint              x,
                                                     gint              y,
                                                     GeglColor       *color,
                                                     gpointer          data);

static void     clay_fg_bg_editor_image_changed     (ClayFgBgEditor   *editor,
                                                     ClayImage        *image);

static void     clay_fg_bg_editor_draw_color_frame  (ClayFgBgEditor   *editor,
                                                     cairo_t          *cr,
                                                     GeglColor        *color,
                                                     gint              x,
                                                     gint              y,
                                                     gint              width,
                                                     gint              height,
                                                     gint              corner_dx,
                                                     gint              corner_dy);

G_DEFINE_TYPE (ClayFgBgEditor, clay_fg_bg_editor, GTK_TYPE_EVENT_BOX)

#define parent_class clay_fg_bg_editor_parent_class

static guint  editor_signals[LAST_SIGNAL] = { 0 };


static void
clay_fg_bg_editor_class_init (ClayFgBgEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  editor_signals[COLOR_CLICKED] =
    g_signal_new ("color-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFgBgEditorClass, color_clicked),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_ACTIVE_COLOR);

  editor_signals[COLOR_DROPPED] =
    g_signal_new ("color-dropped",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFgBgEditorClass, color_dropped),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_ACTIVE_COLOR);

  editor_signals[COLORS_SWAPPED] =
    g_signal_new ("colors-swapped",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFgBgEditorClass, colors_swapped),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  editor_signals[COLORS_DEFAULT] =
    g_signal_new ("colors-default",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFgBgEditorClass, colors_default),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  editor_signals[TOOLTIP] =
    g_signal_new ("tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFgBgEditorClass, tooltip),
                  NULL, NULL,
                  clay_marshal_VOID__INT_OBJECT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  GTK_TYPE_TOOLTIP);

  object_class->dispose              = clay_fg_bg_editor_dispose;
  object_class->set_property         = clay_fg_bg_editor_set_property;
  object_class->get_property         = clay_fg_bg_editor_get_property;

  widget_class->get_request_mode               = clay_fg_bg_editor_get_request_mode;
  widget_class->get_preferred_width_for_height = clay_fg_bg_editor_get_preferred_width_for_height;
  widget_class->size_allocate                  = clay_fg_bg_editor_size_allocate;
  widget_class->style_updated                  = clay_fg_bg_editor_style_updated;
  widget_class->draw                           = clay_fg_bg_editor_draw;
  widget_class->button_press_event             = clay_fg_bg_editor_button_press;
  widget_class->button_release_event           = clay_fg_bg_editor_button_release;
  widget_class->drag_motion                    = clay_fg_bg_editor_drag_motion;
  widget_class->query_tooltip                  = clay_fg_bg_editor_query_tooltip;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ACTIVE_COLOR,
                                   g_param_spec_enum ("active-color",
                                                      NULL, NULL,
                                                      CLAY_TYPE_ACTIVE_COLOR,
                                                      CLAY_ACTIVE_COLOR_FOREGROUND,
                                                      CLAY_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              GTK_ICON_SIZE_SMALL_TOOLBAR,
                                                              CLAY_PARAM_READABLE));

  gtk_widget_class_set_css_name (widget_class, "ClayFgBgEditor");
}

static void
clay_fg_bg_editor_init (ClayFgBgEditor *editor)
{
  editor->active_color = CLAY_ACTIVE_COLOR_FOREGROUND;

  gtk_widget_set_can_focus (GTK_WIDGET (editor), FALSE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (editor), FALSE);

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  clay_dnd_color_source_add (GTK_WIDGET (editor),
                             clay_fg_bg_editor_drag_color, NULL);
  clay_dnd_color_dest_add (GTK_WIDGET (editor),
                           clay_fg_bg_editor_drop_color, NULL);

  gtk_widget_set_size_request (GTK_WIDGET (editor), 24, 24);
}

static void
clay_fg_bg_editor_dispose (GObject *object)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (object);

  if (editor->context)
    clay_fg_bg_editor_set_context (editor, NULL);

  g_clear_weak_pointer (&editor->active_image);
  g_clear_object (&editor->default_icon);
  g_clear_object (&editor->swap_icon);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_fg_bg_editor_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      clay_fg_bg_editor_set_context (editor, g_value_get_object (value));
      break;
    case PROP_ACTIVE_COLOR:
      clay_fg_bg_editor_set_active (editor, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_fg_bg_editor_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    case PROP_ACTIVE_COLOR:
      g_value_set_enum (value, editor->active_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkSizeRequestMode
clay_fg_bg_editor_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
clay_fg_bg_editor_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
  *minimum_width = height;
  *natural_width = height;
}

static void
clay_fg_bg_editor_size_allocate (GtkWidget      *widget,
                                 GtkAllocation  *allocation)
{
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  clay_fg_bg_editor_style_updated (widget);
}

static void
clay_fg_bg_editor_style_updated (GtkWidget *widget)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&editor->default_icon);
  g_clear_object (&editor->swap_icon);
}

static gboolean
clay_fg_bg_editor_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  ClayFgBgEditor  *editor    = CLAY_FG_BG_EDITOR (widget);
  GtkStyleContext *style     = gtk_widget_get_style_context (widget);
  GtkIconSize      tool_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint             pixel_size;
  GtkBorder        border;
  GtkBorder        padding;
  GdkRectangle     rect;
  gint             scale_factor;
  gint             width, height;
  gint             default_w, default_h;
  gint             swap_w, swap_h;

  gtk_style_context_save (style);

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_style_context_get_border (style, gtk_style_context_get_state (style),
                                &border);
  gtk_style_context_get_padding (style, gtk_style_context_get_state (style),
                                 &padding);

  gtk_widget_style_get (GTK_WIDGET (widget),
                        "tool-icon-size", &tool_size,
                        NULL);
  /* Setting the upper bounds of the swap and default colors icons
   * based on the user's current icon scale */
  switch (tool_size)
    {
    case GTK_ICON_SIZE_LARGE_TOOLBAR:
      pixel_size = 9;
      break;

    case GTK_ICON_SIZE_DND:
      pixel_size = 15;
      break;

    case GTK_ICON_SIZE_DIALOG:
      pixel_size = 20;
      break;

    case GTK_ICON_SIZE_SMALL_TOOLBAR:
    default:
      pixel_size = 6;
    }

  border.left   += padding.left;
  border.right  += padding.right;
  border.top    += padding.top;
  border.bottom += padding.bottom;

  scale_factor = gtk_widget_get_scale_factor (widget);

  /*  draw the default colors pixbuf  */
  if (! editor->default_icon)
    editor->default_icon = clay_widget_load_icon (widget,
                                                  CLAY_ICON_COLORS_DEFAULT,
                                                  MAX (MIN (width * 0.3,
                                                            2 * pixel_size),
                                                    pixel_size));

  default_w = gdk_pixbuf_get_width  (editor->default_icon) / scale_factor;
  default_h = gdk_pixbuf_get_height (editor->default_icon) / scale_factor;

  if (default_w < width / 2 && default_h < height / 2)
    {
      cairo_surface_t *surface;

      surface = gdk_cairo_surface_create_from_pixbuf (editor->default_icon,
                                                      scale_factor, NULL);
      cairo_set_source_surface (cr, surface,
                                border.left,
                                height - border.bottom - default_h);
      cairo_surface_destroy (surface);

      cairo_paint (cr);
    }
  else
    {
      default_w = default_h = 0;
    }

  /*  draw the swap colors pixbuf  */
  if (! editor->swap_icon)
    editor->swap_icon = clay_widget_load_icon (widget,
                                               CLAY_ICON_COLORS_SWAP,
                                               MAX (MIN (width * 0.3,
                                                         2 * pixel_size),
                                                    pixel_size));

  swap_w = gdk_pixbuf_get_width  (editor->swap_icon) / scale_factor;
  swap_h = gdk_pixbuf_get_height (editor->swap_icon) / scale_factor;

  if (swap_w < width / 2 && swap_h < height / 2)
    {
      cairo_surface_t *surface;

      surface = gdk_cairo_surface_create_from_pixbuf (editor->swap_icon,
                                                      scale_factor, NULL);
      cairo_set_source_surface (cr, surface,
                                width - border.right - swap_w,
                                border.top);
      cairo_surface_destroy (surface);

      cairo_paint (cr);
    }
  else
    {
      swap_w = swap_h = 0;
    }

  rect.width  = width  - MAX (default_w, swap_w) - 4 - border.top  - border.bottom;
  rect.height = height - MAX (default_h, swap_h) - 2 - border.left - border.right;

  if (rect.height > (height * 3 / 4))
    rect.width = MAX (rect.width - (rect.height - ((height * 3 / 4))),
                      width * 2 / 3);

  editor->rect_width  = rect.width;
  editor->rect_height = rect.height;

  if (editor->context)
    {
      GeglColor *color;

      /*  draw the background frame  */
      color = clay_context_get_background (editor->context);
      rect.x = width  - rect.width  - border.right;
      rect.y = height - rect.height - border.bottom;
      clay_fg_bg_editor_draw_color_frame (editor, cr, color,
                                          rect.x,     rect.y,
                                          rect.width, rect.height,
                                          -1,         -1);

      /*  draw the foreground frame  */
      color = clay_context_get_foreground (editor->context);
      rect.x = border.left;
      rect.y = border.top;
      clay_fg_bg_editor_draw_color_frame (editor, cr, color,
                                          rect.x,     rect.y,
                                          rect.width, rect.height,
                                          +1,         +1);
    }

  gtk_style_context_restore (style);

  return TRUE;
}

static ClayFgBgTarget
clay_fg_bg_editor_target (ClayFgBgEditor *editor,
                          gint            x,
                          gint            y)
{
  GtkWidget       *widget = GTK_WIDGET (editor);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkBorder        border;
  GtkBorder        padding;
  gint             width;
  gint             height;
  gint             rect_w = editor->rect_width;
  gint             rect_h = editor->rect_height;
  gint             button_width;
  gint             button_height;

  width  = gtk_widget_get_allocated_width  (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_style_context_get_border (style, gtk_style_context_get_state (style),
                                &border);
  gtk_style_context_get_padding (style, gtk_style_context_get_state (style),
                                 &padding);

  border.left   += padding.left;
  border.right  += padding.right;
  border.top    += padding.top;
  border.bottom += padding.bottom;

  button_width  = width  - border.left - border.right  - rect_w;
  button_height = height - border.top  - border.bottom - rect_h;

  if (x > border.left          &&
      x < border.left + rect_w &&
      y > border.top           &&
      y < border.top + rect_h)
    {
      return CLAY_FG_BG_TARGET_FOREGROUND;
    }
  else if (x > width  - border.right - rect_w  &&
           x < width  - border.right           &&
           y > height - border.bottom - rect_h &&
           y < height - border.bottom)
    {
      return CLAY_FG_BG_TARGET_BACKGROUND;
    }
  else if (x > border.left                &&
           x < border.left + button_width &&
           y > border.top + rect_h        &&
           y < height - border.bottom)
    {
      return CLAY_FG_BG_TARGET_DEFAULT;
    }
  else if (x > border.left + rect_w &&
           x < width - border.right &&
           y > border.top           &&
           y < border.top + button_height)
    {
      return CLAY_FG_BG_TARGET_SWAP;
    }

  return CLAY_FG_BG_TARGET_INVALID;
}

static gboolean
clay_fg_bg_editor_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);

  if (bevent->button == 1 && gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS)
    {
      ClayFgBgTarget target = clay_fg_bg_editor_target (editor,
                                                        bevent->x, bevent->y);

      editor->click_target = CLAY_FG_BG_TARGET_INVALID;

      switch (target)
        {
        case CLAY_FG_BG_TARGET_FOREGROUND:
          if (editor->active_color != CLAY_ACTIVE_COLOR_FOREGROUND)
            clay_fg_bg_editor_set_active (editor,
                                          CLAY_ACTIVE_COLOR_FOREGROUND);
          editor->click_target = CLAY_FG_BG_TARGET_FOREGROUND;
          break;

        case CLAY_FG_BG_TARGET_BACKGROUND:
          if (editor->active_color != CLAY_ACTIVE_COLOR_BACKGROUND)
            clay_fg_bg_editor_set_active (editor,
                                          CLAY_ACTIVE_COLOR_BACKGROUND);
          editor->click_target = CLAY_FG_BG_TARGET_BACKGROUND;
          break;

        case CLAY_FG_BG_TARGET_SWAP:
          if (editor->context)
            {
              clay_context_swap_colors (editor->context);
              g_signal_emit (editor, editor_signals[COLORS_SWAPPED], 0);
            }
          break;

        case CLAY_FG_BG_TARGET_DEFAULT:
          if (editor->context)
            {
              clay_context_set_default_colors (editor->context);
              g_signal_emit (editor, editor_signals[COLORS_DEFAULT], 0);
            }
          break;

        default:
          break;
        }
    }

  return GDK_EVENT_STOP;
}

static gboolean
clay_fg_bg_editor_button_release (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);

  if (bevent->button == 1)
    {
      ClayFgBgTarget target = clay_fg_bg_editor_target (editor,
                                                        bevent->x, bevent->y);

      if (target == editor->click_target)
        {
          switch (target)
            {
            case CLAY_FG_BG_TARGET_FOREGROUND:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             CLAY_ACTIVE_COLOR_FOREGROUND);
              break;

            case CLAY_FG_BG_TARGET_BACKGROUND:
              g_signal_emit (editor, editor_signals[COLOR_CLICKED], 0,
                             CLAY_ACTIVE_COLOR_BACKGROUND);
              break;

            default:
              break;
            }
        }

      editor->click_target = CLAY_FG_BG_TARGET_INVALID;
    }

  return GDK_EVENT_STOP;
}

static gboolean
clay_fg_bg_editor_drag_motion (GtkWidget      *widget,
                               GdkDragContext *context,
                               gint            x,
                               gint            y,
                               guint           time)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);
  ClayFgBgTarget  target;

#ifdef PLATFORM_OSX
  /* on macOS, the (x, y) that GTK passes to the drag-n-drop handlers are
     offset (see #12211) so query the pointer position relative to the widget */
  gdk_window_get_device_position (gtk_widget_get_window (widget),
                                  gdk_drag_context_get_device (context),
                                  &x, &y, NULL);
  if (! gtk_widget_get_has_window (widget))
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);
      x -= allocation.x;
      y -= allocation.y;
    }
#endif

  target = clay_fg_bg_editor_target (editor, x, y);

  if (target == CLAY_FG_BG_TARGET_FOREGROUND ||
      target == CLAY_FG_BG_TARGET_BACKGROUND)
    {
      gdk_drag_status (context, GDK_ACTION_COPY, time);

      return TRUE;
    }

  gdk_drag_status (context, 0, time);

  return FALSE;
}

static gboolean
clay_fg_bg_editor_query_tooltip (GtkWidget  *widget,
                                 gint        x,
                                 gint        y,
                                 gboolean    keyboard_mode,
                                 GtkTooltip *tooltip)
{
  if (! keyboard_mode)
    {
      ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);
      ClayFgBgTarget  target = clay_fg_bg_editor_target (editor, x, y);

      if (target != CLAY_FG_BG_TARGET_INVALID)
        {
          g_signal_emit (widget, editor_signals[TOOLTIP], 0,
                         target, tooltip);

          return TRUE;
        }
    }

  return FALSE;
}


/*  public functions  */

GtkWidget *
clay_fg_bg_editor_new (ClayContext *context)
{
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_FG_BG_EDITOR,
                       "context", context,
                       NULL);
}

void
clay_fg_bg_editor_set_context (ClayFgBgEditor *editor,
                               ClayContext    *context)
{
  g_return_if_fail (CLAY_IS_FG_BG_EDITOR (editor));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (context != editor->context)
    {
      if (editor->context)
        {
          g_signal_handlers_disconnect_by_func (editor->context,
                                                gtk_widget_queue_draw,
                                                editor);
          g_signal_handlers_disconnect_by_func (editor->context,
                                                G_CALLBACK (clay_fg_bg_editor_image_changed),
                                                editor);
          g_object_unref (editor->context);
          g_clear_object (&editor->color_config);
        }

      editor->context = context;

      if (context)
        {
          g_object_ref (context);

          g_signal_connect_swapped (context, "foreground-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);
          g_signal_connect_swapped (context, "background-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);
          g_signal_connect_swapped (context, "image-changed",
                                    G_CALLBACK (clay_fg_bg_editor_image_changed),
                                    editor);

          editor->color_config = g_object_ref (context->clay->config->color_management);
        }

      g_object_notify (G_OBJECT (editor), "context");
    }
}

void
clay_fg_bg_editor_set_active (ClayFgBgEditor  *editor,
                              ClayActiveColor  active)
{
  g_return_if_fail (CLAY_IS_FG_BG_EDITOR (editor));

  editor->active_color = active;
  gtk_widget_queue_draw (GTK_WIDGET (editor));
  g_object_notify (G_OBJECT (editor), "active-color");
}


/*  private functions  */

static void
clay_fg_bg_editor_drag_color (GtkWidget  *widget,
                              GeglColor **color,
                              gpointer   data)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);

  if (editor->context)
    {
      switch (editor->active_color)
        {
        case CLAY_ACTIVE_COLOR_FOREGROUND:
          *color = gegl_color_duplicate (clay_context_get_foreground (editor->context));
          break;

        case CLAY_ACTIVE_COLOR_BACKGROUND:
          *color = gegl_color_duplicate (clay_context_get_background (editor->context));
          break;
        }
    }
}

static void
clay_fg_bg_editor_drop_color (GtkWidget *widget,
                              gint       x,
                              gint       y,
                              GeglColor *color,
                              gpointer   data)
{
  ClayFgBgEditor *editor = CLAY_FG_BG_EDITOR (widget);

#ifdef PLATFORM_OSX
  /* on macOS, the (x, y) that GTK passes to the drag-n-drop handlers are
     offset (see #12211) so query the pointer position relative to the widget */
  gdk_window_get_device_position (gtk_widget_get_window (widget),
                                  gdk_seat_get_pointer (gdk_display_get_default_seat (gtk_widget_get_display (widget))),
                                  &x, &y, NULL);
  if (! gtk_widget_get_has_window (widget))
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);
      x -= allocation.x;
      y -= allocation.y;
    }
#endif

  if (editor->context)
    {
      switch (clay_fg_bg_editor_target (editor, x, y))
        {
        case CLAY_FG_BG_TARGET_FOREGROUND:
          clay_context_set_foreground (editor->context, color);
          g_signal_emit (editor, editor_signals[COLOR_DROPPED], 0,
                         CLAY_ACTIVE_COLOR_FOREGROUND);
          break;

        case CLAY_FG_BG_TARGET_BACKGROUND:
          clay_context_set_background (editor->context, color);
          g_signal_emit (editor, editor_signals[COLOR_DROPPED], 0,
                         CLAY_ACTIVE_COLOR_BACKGROUND);
          break;

        default:
          break;
        }
    }
}

static void
clay_fg_bg_editor_image_changed (ClayFgBgEditor *editor,
                                 ClayImage      *image)
{
  gtk_widget_queue_draw (GTK_WIDGET (editor));

  if (editor->active_image)
    {
      g_signal_handlers_disconnect_by_func (editor->active_image,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            editor);
      if (clay_image_get_base_type (editor->active_image) == CLAY_INDEXED)
        {
          ClayPalette *palette;

          palette = clay_image_get_colormap_palette (editor->active_image);
          g_signal_handlers_disconnect_by_func (palette,
                                                G_CALLBACK (gtk_widget_queue_draw),
                                                editor);
        }
    }
  g_set_weak_pointer (&editor->active_image, image);
  if (image)
    {
      g_signal_connect_swapped (image, "notify::base-type",
                                G_CALLBACK (gtk_widget_queue_draw),
                                editor);
      g_signal_connect_swapped (image, "colormap-changed",
                                G_CALLBACK (gtk_widget_queue_draw),
                                editor);

      if (clay_image_get_base_type (image) == CLAY_INDEXED)
        {
          ClayPalette *palette;

          palette = clay_image_get_colormap_palette (editor->active_image);
          g_signal_connect_swapped (palette, "dirty",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    editor);
        }
    }
}

static void
clay_fg_bg_editor_draw_color_frame (ClayFgBgEditor *editor,
                                    cairo_t        *cr,
                                    GeglColor      *color,
                                    gint            x,
                                    gint            y,
                                    gint            width,
                                    gint            height,
                                    gint            corner_dx,
                                    gint            corner_dy)
{
  ClayPalette       *colormap_palette = NULL;
  ClayImageBaseType  base_type        = CLAY_RGB;
  gboolean           is_out_of_gamut;

  if (editor->active_image)
    {
      base_type = clay_image_get_base_type (editor->active_image);

      if (base_type == CLAY_INDEXED)
        {
          colormap_palette = clay_image_get_colormap_palette (
            editor->active_image);
        }
    }

  cairo_save (cr);
  clay_cairo_set_source_color (cr, color, editor->color_config, FALSE, GTK_WIDGET (editor));

  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);

  if (editor->active_image)
    {
      const Babl *target_space = clay_image_get_layer_space (editor->active_image);

      is_out_of_gamut = clay_color_is_out_of_gamut (color, target_space);
    }
  else
    {
      /* Without active image, we are never out of gamut. */
      is_out_of_gamut = FALSE;
    }

  if (editor->color_config &&
      /* Common out-of-gamut case */
      (is_out_of_gamut ||
       /* Indexed images */
       (colormap_palette &&
        ! clay_palette_find_entry (colormap_palette, color, NULL))))
    {
      GeglColor *out_of_gamut_color;
      gint       corner_x = x + 0.5 * (1.0 - corner_dx) * width;
      gint       corner_y = y + 0.5 * (1.0 - corner_dy) * height;
      gint       side     = MIN (width, height) * 2 / 3;

      cairo_move_to (cr, corner_x, corner_y);
      cairo_line_to (cr, corner_x + side * corner_dx, corner_y);
      cairo_line_to (cr, corner_x, corner_y + side * corner_dy);
      cairo_close_path (cr);

      out_of_gamut_color = clay_color_config_get_out_of_gamut_color (editor->color_config);
      /* Out-of-gamut color is color-managed to FG/BG editor's monitor profile
       * but not soft-proofed.
       */
      clay_cairo_set_source_color (cr, out_of_gamut_color, editor->color_config, FALSE, GTK_WIDGET (editor));
      cairo_fill (cr);

      g_object_unref (out_of_gamut_color);
    }

  cairo_set_line_width (cr, 1.0);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, x + 0.5, y + 0.5, width - 1.0, height - 1.0);
  cairo_stroke (cr);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, x + 1.5, y + 1.5, width - 3.0, height - 3.0);
  cairo_stroke (cr);

  cairo_restore (cr);
}
