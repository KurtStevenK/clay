/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolwidget.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include <stdarg.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"

#include "display-types.h"

#include "core/claymarshal.h"

#include "claycanvasarc.h"
#include "claycanvascorner.h"
#include "claycanvasgroup.h"
#include "claycanvashandle.h"
#include "claycanvaslimit.h"
#include "claycanvasline.h"
#include "claycanvaspath.h"
#include "claycanvaspolygon.h"
#include "claycanvasrectangle.h"
#include "claycanvasrectangleguides.h"
#include "claycanvastransformguides.h"
#include "claydisplayshell.h"
#include "claytoolwidget.h"


enum
{
  PROP_0,
  PROP_SHELL,
  PROP_ITEM
};

enum
{
  CHANGED,
  RESPONSE,
  SNAP_OFFSETS,
  STATUS,
  STATUS_COORDS,
  MESSAGE,
  FOCUS_CHANGED,
  LAST_SIGNAL
};

struct _ClayToolWidgetPrivate
{
  ClayDisplayShell *shell;
  ClayCanvasItem   *item;
  GList            *group_stack;

  gint              snap_offset_x;
  gint              snap_offset_y;
  gint              snap_width;
  gint              snap_height;

  gboolean          visible;
  gboolean          focus;
};


/*  local function prototypes  */

static void     clay_tool_widget_finalize           (GObject         *object);
static void     clay_tool_widget_constructed        (GObject         *object);
static void     clay_tool_widget_set_property       (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void     clay_tool_widget_get_property       (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void     clay_tool_widget_properties_changed (GObject         *object,
                                                     guint            n_pspecs,
                                                     GParamSpec     **pspecs);

static void     clay_tool_widget_real_leave_notify  (ClayToolWidget  *widget);
static gboolean clay_tool_widget_real_key_press     (ClayToolWidget  *widget,
                                                     GdkEventKey     *kevent);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolWidget, clay_tool_widget, CLAY_TYPE_OBJECT)

#define parent_class clay_tool_widget_parent_class

static guint widget_signals[LAST_SIGNAL] = { 0 };


static void
clay_tool_widget_class_init (ClayToolWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize                    = clay_tool_widget_finalize;
  object_class->constructed                 = clay_tool_widget_constructed;
  object_class->set_property                = clay_tool_widget_set_property;
  object_class->get_property                = clay_tool_widget_get_property;
  object_class->dispatch_properties_changed = clay_tool_widget_properties_changed;

  klass->leave_notify                       = clay_tool_widget_real_leave_notify;
  klass->key_press                          = clay_tool_widget_real_key_press;

  widget_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  widget_signals[RESPONSE] =
    g_signal_new ("response",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  widget_signals[SNAP_OFFSETS] =
    g_signal_new ("snap-offsets",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, snap_offsets),
                  NULL, NULL,
                  clay_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  widget_signals[STATUS] =
    g_signal_new ("status",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, status),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[STATUS_COORDS] =
    g_signal_new ("status-coords",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, status_coords),
                  NULL, NULL,
                  clay_marshal_VOID__STRING_DOUBLE_STRING_DOUBLE_STRING,
                  G_TYPE_NONE, 5,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_STRING);

  widget_signals[MESSAGE] =
    g_signal_new ("message",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, message),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  widget_signals[FOCUS_CHANGED] =
    g_signal_new ("focus-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolWidgetClass, focus_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DISPLAY_SHELL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM,
                                   g_param_spec_object ("item",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CANVAS_ITEM,
                                                        CLAY_PARAM_READABLE));
}

static void
clay_tool_widget_init (ClayToolWidget *widget)
{
  widget->private = clay_tool_widget_get_instance_private (widget);

  widget->private->visible = TRUE;
}

static void
clay_tool_widget_constructed (GObject *object)
{
  ClayToolWidget        *widget  = CLAY_TOOL_WIDGET (object);
  ClayToolWidgetPrivate *private = widget->private;
  ClayToolWidgetClass   *klass   = CLAY_TOOL_WIDGET_GET_CLASS (widget);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DISPLAY_SHELL (private->shell));

  private->item = clay_canvas_group_new (private->shell);

  clay_canvas_item_set_visible (private->item, private->visible);

  if (klass->changed)
    {
      if (klass->update_on_scale)
        {
          g_signal_connect_object (private->shell, "scaled",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }

      if (klass->update_on_scroll)
        {
          g_signal_connect_object (private->shell, "scrolled",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }

      if (klass->update_on_rotate)
        {
          g_signal_connect_object (private->shell, "rotated",
                                   G_CALLBACK (klass->changed),
                                   widget,
                                   G_CONNECT_SWAPPED);
        }
    }
}

static void
clay_tool_widget_finalize (GObject *object)
{
  ClayToolWidget        *widget  = CLAY_TOOL_WIDGET (object);
  ClayToolWidgetPrivate *private = widget->private;

  g_clear_object (&private->item);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_tool_widget_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayToolWidget        *widget  = CLAY_TOOL_WIDGET (object);
  ClayToolWidgetPrivate *private = widget->private;

  switch (property_id)
    {
    case PROP_SHELL:
      private->shell = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_widget_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayToolWidget        *widget  = CLAY_TOOL_WIDGET (object);
  ClayToolWidgetPrivate *private = widget->private;

  switch (property_id)
    {
    case PROP_SHELL:
      g_value_set_object (value, private->shell);
      break;

    case PROP_ITEM:
      g_value_set_object (value, private->item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_widget_properties_changed (GObject     *object,
                                     guint        n_pspecs,
                                     GParamSpec **pspecs)
{
  ClayToolWidget *widget = CLAY_TOOL_WIDGET (object);

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs,
                                                              pspecs);

  clay_tool_widget_changed (widget);
}

static void
clay_tool_widget_real_leave_notify (ClayToolWidget *widget)
{
  clay_tool_widget_set_status (widget, NULL);
}

static gboolean
clay_tool_widget_real_key_press (ClayToolWidget *widget,
                                 GdkEventKey    *kevent)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      clay_tool_widget_response (widget, CLAY_TOOL_WIDGET_RESPONSE_CONFIRM);
      return TRUE;

    case GDK_KEY_Escape:
      clay_tool_widget_response (widget, CLAY_TOOL_WIDGET_RESPONSE_CANCEL);
      return TRUE;

    case GDK_KEY_BackSpace:
      clay_tool_widget_response (widget, CLAY_TOOL_WIDGET_RESPONSE_RESET);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}


/*  public functions  */

ClayDisplayShell *
clay_tool_widget_get_shell (ClayToolWidget *widget)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->shell;
}

ClayCanvasItem *
clay_tool_widget_get_item (ClayToolWidget *widget)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  return widget->private->item;
}

void
clay_tool_widget_set_visible (ClayToolWidget *widget,
                              gboolean        visible)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  if (visible != widget->private->visible)
    {
      widget->private->visible = visible;

      if (widget->private->item)
        clay_canvas_item_set_visible (widget->private->item, visible);

      if (! visible)
        clay_tool_widget_set_status (widget, NULL);
    }
}

gboolean
clay_tool_widget_get_visible (ClayToolWidget *widget)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->visible;
}

void
clay_tool_widget_set_focus (ClayToolWidget *widget,
                            gboolean        focus)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  if (focus != widget->private->focus)
    {
      widget->private->focus = focus;

      g_signal_emit (widget, widget_signals[FOCUS_CHANGED], 0);
    }
}

gboolean
clay_tool_widget_get_focus (ClayToolWidget *widget)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), FALSE);

  return widget->private->focus;
}

void
clay_tool_widget_changed (ClayToolWidget *widget)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[CHANGED], 0);
}

void
clay_tool_widget_response (ClayToolWidget *widget,
                           gint            response_id)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[RESPONSE], 0,
                 response_id);
}

void
clay_tool_widget_set_snap_offsets (ClayToolWidget *widget,
                                   gint            offset_x,
                                   gint            offset_y,
                                   gint            width,
                                   gint            height)
{
  ClayToolWidgetPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  private = widget->private;

  if (offset_x != private->snap_offset_x ||
      offset_y != private->snap_offset_y ||
      width    != private->snap_width    ||
      height   != private->snap_height)
    {
      private->snap_offset_x = offset_x;
      private->snap_offset_y = offset_y;
      private->snap_width    = width;
      private->snap_height   = height;

      g_signal_emit (widget, widget_signals[SNAP_OFFSETS], 0,
                 offset_x, offset_y, width, height);
    }
}

void
clay_tool_widget_get_snap_offsets (ClayToolWidget *widget,
                                   gint           *offset_x,
                                   gint           *offset_y,
                                   gint           *width,
                                   gint           *height)
{
  ClayToolWidgetPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  private = widget->private;

  if (offset_x) *offset_x = private->snap_offset_x;
  if (offset_y) *offset_y = private->snap_offset_y;
  if (width)    *width    = private->snap_width;
  if (height)   *height   = private->snap_height;
}

void
clay_tool_widget_set_status (ClayToolWidget *widget,
                             const gchar    *status)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS], 0,
                 status);
}

void
clay_tool_widget_set_status_coords (ClayToolWidget *widget,
                                    const gchar    *title,
                                    gdouble         x,
                                    const gchar    *separator,
                                    gdouble         y,
                                    const gchar    *help)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  g_signal_emit (widget, widget_signals[STATUS_COORDS], 0,
                 title, x, separator, y, help);
}

void
clay_tool_widget_message (ClayToolWidget *widget,
                          const gchar    *format,
                          ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  message = g_strdup_vprintf (format, args);

  va_end (args);

  clay_tool_widget_message_literal (widget, message);

  g_free (message);
}

void
clay_tool_widget_message_literal (ClayToolWidget *widget,
                                  const gchar    *message)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (message != NULL);

  g_signal_emit (widget, widget_signals[MESSAGE], 0,
                 message);
}

void
clay_tool_widget_add_item (ClayToolWidget *widget,
                           ClayCanvasItem *item)
{
  ClayCanvasGroup *group;

  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  group = CLAY_CANVAS_GROUP (widget->private->item);

  if (widget->private->group_stack)
    group = widget->private->group_stack->data;

  clay_canvas_group_add_item (group, item);
}

void
clay_tool_widget_remove_item (ClayToolWidget *widget,
                              ClayCanvasItem *item)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (widget->private->item),
                                 item);
}

ClayCanvasGroup *
clay_tool_widget_add_group (ClayToolWidget *widget)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_group_new (widget->private->shell);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return CLAY_CANVAS_GROUP (item);
}

ClayCanvasGroup *
clay_tool_widget_add_stroke_group (ClayToolWidget *widget)
{
  ClayCanvasGroup *group;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  group = clay_tool_widget_add_group (widget);
  clay_canvas_group_set_group_stroking (group, TRUE);

  return group;
}

ClayCanvasGroup *
clay_tool_widget_add_fill_group (ClayToolWidget *widget)
{
  ClayCanvasGroup *group;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  group = clay_tool_widget_add_group (widget);
  clay_canvas_group_set_group_filling (group, TRUE);

  return group;
}

void
clay_tool_widget_push_group (ClayToolWidget  *widget,
                             ClayCanvasGroup *group)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));

  widget->private->group_stack = g_list_prepend (widget->private->group_stack,
                                                 group);
}

void
clay_tool_widget_pop_group (ClayToolWidget *widget)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (widget->private->group_stack != NULL);

  widget->private->group_stack = g_list_remove (widget->private->group_stack,
                                                widget->private->group_stack->data);
}

/**
 * clay_tool_widget_add_line:
 * @widget: the #ClayToolWidget
 * @x1:     start point X in image coordinates
 * @y1:     start point Y in image coordinates
 * @x2:     end point X in image coordinates
 * @y2:     end point Y in image coordinates
 *
 * This function adds a #ClayCanvasLine to @widget.
 **/
ClayCanvasItem *
clay_tool_widget_add_line (ClayToolWidget *widget,
                           gdouble         x1,
                           gdouble         y1,
                           gdouble         x2,
                           gdouble         y2)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_line_new (widget->private->shell,
                               x1, y1, x2, y2);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_rectangle (ClayToolWidget *widget,
                                gdouble         x,
                                gdouble         y,
                                gdouble         width,
                                gdouble         height,
                                gboolean        filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_rectangle_new (widget->private->shell,
                                    x, y, width, height, filled);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_arc (ClayToolWidget *widget,
                          gdouble         center_x,
                          gdouble         center_y,
                          gdouble         radius_x,
                          gdouble         radius_y,
                          gdouble         start_angle,
                          gdouble         slice_angle,
                          gboolean        filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_arc_new (widget->private->shell,
                              center_x, center_y,
                              radius_x, radius_y,
                              start_angle, slice_angle,
                              filled);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_limit (ClayToolWidget *widget,
                            ClayLimitType   type,
                            gdouble         x,
                            gdouble         y,
                            gdouble         radius,
                            gdouble         aspect_ratio,
                            gdouble         angle,
                            gboolean        dashed)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_limit_new (widget->private->shell,
                                type,
                                x, y,
                                radius,
                                aspect_ratio,
                                angle,
                                dashed);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_polygon (ClayToolWidget    *widget,
                              ClayMatrix3       *transform,
                              const ClayVector2 *points,
                              gint               n_points,
                              gboolean           filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = clay_canvas_polygon_new (widget->private->shell,
                                  points, n_points,
                                  transform, filled);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_polygon_from_coords (ClayToolWidget    *widget,
                                          ClayMatrix3       *transform,
                                          const ClayCoords  *points,
                                          gint               n_points,
                                          gboolean           filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (points == NULL || n_points > 0, NULL);

  item = clay_canvas_polygon_new_from_coords (widget->private->shell,
                                              points, n_points,
                                              transform, filled);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_path (ClayToolWidget       *widget,
                           const ClayBezierDesc *desc)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_path_new (widget->private->shell,
                               desc, 0, 0, FALSE, CLAY_PATH_STYLE_DEFAULT);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_handle (ClayToolWidget   *widget,
                             ClayHandleType    type,
                             gdouble           x,
                             gdouble           y,
                             gint              width,
                             gint              height,
                             ClayHandleAnchor  anchor)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_handle_new (widget->private->shell,
                                 type, anchor, x, y, width, height);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_corner (ClayToolWidget   *widget,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             ClayHandleAnchor  anchor,
                             gint              corner_width,
                             gint              corner_height,
                             gboolean          outside)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_corner_new (widget->private->shell,
                                 x, y, width, height,
                                 anchor, corner_width, corner_height,
                                 outside);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_rectangle_guides (ClayToolWidget *widget,
                                       gdouble         x,
                                       gdouble         y,
                                       gdouble         width,
                                       gdouble         height,
                                       ClayGuidesType  type)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_rectangle_guides_new (widget->private->shell,
                                           x, y, width, height, type, 4);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_tool_widget_add_transform_guides (ClayToolWidget    *widget,
                                       const ClayMatrix3 *transform,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       ClayGuidesType     type,
                                       gint               n_guides,
                                       gboolean           clip)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);

  item = clay_canvas_transform_guides_new (widget->private->shell,
                                           transform, x1, y1, x2, y2,
                                           type, n_guides, clip);

  clay_tool_widget_add_item (widget, item);
  g_object_unref (item);

  return item;
}

gint
clay_tool_widget_button_press (ClayToolWidget      *widget,
                               const ClayCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               ClayButtonPressType  press_type)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), 0);
  g_return_val_if_fail (coords != NULL, 0);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->button_press)
    {
      return CLAY_TOOL_WIDGET_GET_CLASS (widget)->button_press (widget,
                                                                coords, time,
                                                                state,
                                                                press_type);
    }

  return 0;
}

void
clay_tool_widget_button_release (ClayToolWidget        *widget,
                                 const ClayCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 ClayButtonReleaseType  release_type)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->button_release)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->button_release (widget,
                                                           coords, time, state,
                                                           release_type);
    }
}

void
clay_tool_widget_motion (ClayToolWidget   *widget,
                         const ClayCoords *coords,
                         guint32           time,
                         GdkModifierType   state)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->motion)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->motion (widget,
                                                   coords, time, state);
    }
}

ClayHit
clay_tool_widget_hit (ClayToolWidget   *widget,
                      const ClayCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), CLAY_HIT_NONE);
  g_return_val_if_fail (coords != NULL, CLAY_HIT_NONE);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->hit)
    {
      return CLAY_TOOL_WIDGET_GET_CLASS (widget)->hit (widget,
                                                       coords, state,
                                                       proximity);
    }

  return CLAY_HIT_NONE;
}

void
clay_tool_widget_hover (ClayToolWidget   *widget,
                        const ClayCoords *coords,
                        GdkModifierType   state,
                        gboolean          proximity)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (coords != NULL);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->hover)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->hover (widget,
                                                  coords, state, proximity);
    }
}

void
clay_tool_widget_leave_notify (ClayToolWidget *widget)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->leave_notify)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->leave_notify (widget);
    }
}

gboolean
clay_tool_widget_key_press (ClayToolWidget *widget,
                            GdkEventKey    *kevent)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->key_press)
    {
      return CLAY_TOOL_WIDGET_GET_CLASS (widget)->key_press (widget, kevent);
    }

  return FALSE;
}

gboolean
clay_tool_widget_key_release (ClayToolWidget *widget,
                              GdkEventKey    *kevent)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->key_release)
    {
      return CLAY_TOOL_WIDGET_GET_CLASS (widget)->key_release (widget, kevent);
    }

  return FALSE;
}

void
clay_tool_widget_motion_modifier (ClayToolWidget  *widget,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->motion_modifier (widget,
                                                            key, press, state);
    }
}

void
clay_tool_widget_hover_modifier (ClayToolWidget  *widget,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier)
    {
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->hover_modifier (widget,
                                                           key, press, state);
    }
}

gboolean
clay_tool_widget_get_cursor (ClayToolWidget      *widget,
                             const ClayCoords    *coords,
                             GdkModifierType      state,
                             ClayCursorType      *cursor,
                             ClayToolCursorType  *tool_cursor,
                             ClayCursorModifier  *modifier)

{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->get_cursor)
    {
      ClayCursorType     my_cursor;
      ClayToolCursorType my_tool_cursor;
      ClayCursorModifier my_modifier;

      if (cursor)      my_cursor      = *cursor;
      if (tool_cursor) my_tool_cursor = *tool_cursor;
      if (modifier)    my_modifier    = *modifier;

      if (CLAY_TOOL_WIDGET_GET_CLASS (widget)->get_cursor (widget, coords,
                                                           state,
                                                           &my_cursor,
                                                           &my_tool_cursor,
                                                           &my_modifier))
        {
          if (cursor)      *cursor      = my_cursor;
          if (tool_cursor) *tool_cursor = my_tool_cursor;
          if (modifier)    *modifier    = my_modifier;

          return TRUE;
        }
    }

  return FALSE;
}

ClayUIManager *
clay_tool_widget_get_popup (ClayToolWidget        *widget,
                            const ClayCoords      *coords,
                            GdkModifierType        state,
                            const gchar          **ui_path)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET (widget), NULL);
  g_return_val_if_fail (coords != NULL, NULL);

  if (widget->private->visible &&
      CLAY_TOOL_WIDGET_GET_CLASS (widget)->get_popup)
    {
      return CLAY_TOOL_WIDGET_GET_CLASS (widget)->get_popup (widget, coords,
                                                             state, ui_path);
    }

  return NULL;
}
