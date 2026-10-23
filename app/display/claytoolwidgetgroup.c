/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolwidgetgroup.c
 * Copyright (C) 2018 Ell
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

#include "display-types.h"

#include "core/claycontainer.h"
#include "core/claylist.h"

#include "claycanvasgroup.h"
#include "claydisplayshell.h"
#include "claytoolwidgetgroup.h"


struct _ClayToolWidgetGroupPrivate
{
  ClayContainer  *children;

  ClayToolWidget *focus_widget;
  ClayToolWidget *hover_widget;

  gboolean        auto_raise;
};


/*  local function prototypes  */

static void             clay_tool_widget_group_finalize            (GObject               *object);

static void             clay_tool_widget_group_focus_changed       (ClayToolWidget        *widget);
static gint             clay_tool_widget_group_button_press        (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    ClayButtonPressType    press_type);
static void             clay_tool_widget_group_button_release      (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    ClayButtonReleaseType  release_type);
static void             clay_tool_widget_group_motion              (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state);
static ClayHit          clay_tool_widget_group_hit                 (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             clay_tool_widget_group_hover               (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity);
static void             clay_tool_widget_group_leave_notify        (ClayToolWidget        *widget);
static gboolean         clay_tool_widget_group_key_press           (ClayToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static gboolean         clay_tool_widget_group_key_release         (ClayToolWidget        *widget,
                                                                    GdkEventKey           *kevent);
static void             clay_tool_widget_group_motion_modifier     (ClayToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static void             clay_tool_widget_group_hover_modifier      (ClayToolWidget        *widget,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state);
static gboolean         clay_tool_widget_group_get_cursor          (ClayToolWidget        *widget,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    ClayCursorType        *cursor,
                                                                    ClayToolCursorType    *tool_cursor,
                                                                    ClayCursorModifier    *modifier);

static void             clay_tool_widget_group_children_add        (ClayContainer         *container,
                                                                    ClayToolWidget        *child,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_children_remove     (ClayContainer         *container,
                                                                    ClayToolWidget        *child,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_children_reorder    (ClayContainer         *container,
                                                                    ClayToolWidget        *child,
                                                                    gint                   old_index,
                                                                    gint                   new_index,
                                                                    ClayToolWidgetGroup   *group);

static void             clay_tool_widget_group_child_changed       (ClayToolWidget        *child,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_response      (ClayToolWidget        *child,
                                                                    gint                   response_id,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_snap_offsets  (ClayToolWidget        *child,
                                                                    gint                   offset_x,
                                                                    gint                   offset_y,
                                                                    gint                   width,
                                                                    gint                   height,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_status        (ClayToolWidget        *child,
                                                                    const gchar           *status,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_status_coords (ClayToolWidget        *child,
                                                                    const gchar           *title,
                                                                    gdouble                x,
                                                                    const gchar           *separator,
                                                                    gdouble                y,
                                                                    const gchar           *help,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_message       (ClayToolWidget        *child,
                                                                    const gchar           *message,
                                                                    ClayToolWidgetGroup   *group);
static void             clay_tool_widget_group_child_focus_changed (ClayToolWidget        *child,
                                                                    ClayToolWidgetGroup   *group);

static ClayToolWidget * clay_tool_widget_group_get_hover_widget    (ClayToolWidgetGroup   *group,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    ClayHit               *hit);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolWidgetGroup, clay_tool_widget_group,
                            CLAY_TYPE_TOOL_WIDGET)

#define parent_class clay_tool_widget_group_parent_class


/*  priv functions  */


static void
clay_tool_widget_group_class_init (ClayToolWidgetGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->finalize        = clay_tool_widget_group_finalize;

  widget_class->focus_changed   = clay_tool_widget_group_focus_changed;
  widget_class->button_press    = clay_tool_widget_group_button_press;
  widget_class->button_release  = clay_tool_widget_group_button_release;
  widget_class->motion          = clay_tool_widget_group_motion;
  widget_class->hit             = clay_tool_widget_group_hit;
  widget_class->hover           = clay_tool_widget_group_hover;
  widget_class->leave_notify    = clay_tool_widget_group_leave_notify;
  widget_class->key_press       = clay_tool_widget_group_key_press;
  widget_class->key_release     = clay_tool_widget_group_key_release;
  widget_class->motion_modifier = clay_tool_widget_group_motion_modifier;
  widget_class->hover_modifier  = clay_tool_widget_group_hover_modifier;
  widget_class->get_cursor      = clay_tool_widget_group_get_cursor;
}

static void
clay_tool_widget_group_init (ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv;

  priv = group->priv = clay_tool_widget_group_get_instance_private (group);

  priv->children = g_object_new (CLAY_TYPE_LIST,
                                 "child-type", CLAY_TYPE_TOOL_WIDGET,
                                 "append",     TRUE,
                                 NULL);

  g_signal_connect (priv->children, "add",
                    G_CALLBACK (clay_tool_widget_group_children_add),
                    group);
  g_signal_connect (priv->children, "remove",
                    G_CALLBACK (clay_tool_widget_group_children_remove),
                    group);
  g_signal_connect (priv->children, "reorder",
                    G_CALLBACK (clay_tool_widget_group_children_reorder),
                    group);

  clay_container_add_handler (priv->children, "changed",
                              G_CALLBACK (clay_tool_widget_group_child_changed),
                              group);
  clay_container_add_handler (priv->children, "response",
                              G_CALLBACK (clay_tool_widget_group_child_response),
                              group);
  clay_container_add_handler (priv->children, "snap-offsets",
                              G_CALLBACK (clay_tool_widget_group_child_snap_offsets),
                              group);
  clay_container_add_handler (priv->children, "status",
                              G_CALLBACK (clay_tool_widget_group_child_status),
                              group);
  clay_container_add_handler (priv->children, "status-coords",
                              G_CALLBACK (clay_tool_widget_group_child_status_coords),
                              group);
  clay_container_add_handler (priv->children, "message",
                              G_CALLBACK (clay_tool_widget_group_child_message),
                              group);
  clay_container_add_handler (priv->children, "focus-changed",
                              G_CALLBACK (clay_tool_widget_group_child_focus_changed),
                              group);
}

static void
clay_tool_widget_group_finalize (GObject *object)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (object);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  g_clear_object (&priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
clay_tool_widget_group_button_press (ClayToolWidget      *widget,
                                     const ClayCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     ClayButtonPressType  press_type)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  clay_tool_widget_group_hover (widget, coords, state, TRUE);

  if (priv->focus_widget != priv->hover_widget)
    {
      if (priv->hover_widget)
        clay_tool_widget_set_focus (priv->hover_widget, TRUE);
      else if (priv->focus_widget)
        clay_tool_widget_set_focus (priv->focus_widget, FALSE);
    }

  if (priv->hover_widget)
    {
      if (priv->auto_raise)
        {
          clay_container_reorder (priv->children,
                                  CLAY_OBJECT (priv->hover_widget), -1);
        }

      return clay_tool_widget_button_press (priv->hover_widget,
                                            coords, time, state, press_type);
    }

  return FALSE;
}

static void
clay_tool_widget_group_focus_changed (ClayToolWidget *widget)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    {
      ClayToolWidget *focus_widget = priv->focus_widget;

      clay_tool_widget_set_focus (priv->focus_widget,
                                  clay_tool_widget_get_focus (widget));

      priv->focus_widget = focus_widget;
    }
}

static void
clay_tool_widget_group_button_release (ClayToolWidget        *widget,
                                       const ClayCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       ClayButtonReleaseType  release_type)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      clay_tool_widget_button_release (priv->hover_widget,
                                       coords, time, state, release_type);
    }
}

static void
clay_tool_widget_group_motion (ClayToolWidget   *widget,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv = group->priv;

  if (priv->hover_widget)
    clay_tool_widget_motion (priv->hover_widget, coords, time, state);
}

static ClayHit
clay_tool_widget_group_hit (ClayToolWidget   *widget,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity)
{
  ClayToolWidgetGroup *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayHit              hit;

  clay_tool_widget_group_get_hover_widget (group,
                                           coords, state, proximity,
                                           &hit);

  return hit;
}

static void
clay_tool_widget_group_hover (ClayToolWidget   *widget,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;
  ClayToolWidget             *hover_widget;

  hover_widget =
    clay_tool_widget_group_get_hover_widget (group,
                                             coords, state, proximity,
                                             NULL);

  if (priv->hover_widget && priv->hover_widget != hover_widget)
    clay_tool_widget_leave_notify (priv->hover_widget);

  priv->hover_widget = hover_widget;

  if (priv->hover_widget)
    clay_tool_widget_hover (priv->hover_widget, coords, state, proximity);
}

static void
clay_tool_widget_group_leave_notify (ClayToolWidget *widget)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      clay_tool_widget_leave_notify (priv->hover_widget);

      priv->hover_widget = NULL;
    }

  CLAY_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
clay_tool_widget_group_key_press (ClayToolWidget *widget,
                                  GdkEventKey    *kevent)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return clay_tool_widget_key_press (priv->focus_widget, kevent);

  return CLAY_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
}

static gboolean
clay_tool_widget_group_key_release (ClayToolWidget *widget,
                                    GdkEventKey    *kevent)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->focus_widget)
    return clay_tool_widget_key_release (priv->focus_widget, kevent);

  return FALSE;
}

static void
clay_tool_widget_group_motion_modifier (ClayToolWidget  *widget,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    clay_tool_widget_motion_modifier (priv->hover_widget, key, press, state);
}

static void
clay_tool_widget_group_hover_modifier (ClayToolWidget  *widget,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;
  GList                      *iter;

  for (iter = g_queue_peek_head_link (CLAY_LIST (priv->children)->queue);
       iter;
       iter = g_list_next (iter))
    {
      clay_tool_widget_hover_modifier (iter->data, key, press, state);
    }
}

static gboolean
clay_tool_widget_group_get_cursor (ClayToolWidget     *widget,
                                   const ClayCoords   *coords,
                                   GdkModifierType     state,
                                   ClayCursorType     *cursor,
                                   ClayToolCursorType *tool_cursor,
                                   ClayCursorModifier *modifier)
{
  ClayToolWidgetGroup        *group = CLAY_TOOL_WIDGET_GROUP (widget);
  ClayToolWidgetGroupPrivate *priv  = group->priv;

  if (priv->hover_widget)
    {
      return clay_tool_widget_get_cursor (priv->hover_widget,
                                          coords, state,
                                          cursor, tool_cursor, modifier);
    }

  return FALSE;
}

static void
clay_tool_widget_group_children_add (ClayContainer       *container,
                                     ClayToolWidget      *child,
                                     ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);
  ClayCanvasGroup            *canvas_group;

  canvas_group = CLAY_CANVAS_GROUP (clay_tool_widget_get_item (widget));

  clay_canvas_group_add_item (canvas_group,
                              clay_tool_widget_get_item (child));

  if (clay_tool_widget_get_focus (child) && priv->focus_widget)
    {
      clay_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget = child;

      clay_tool_widget_set_focus (child, clay_tool_widget_get_focus (widget));
    }

  clay_tool_widget_changed (widget);
}

static void
clay_tool_widget_group_children_remove (ClayContainer       *container,
                                        ClayToolWidget      *child,
                                        ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);
  ClayCanvasGroup            *canvas_group;

  canvas_group = CLAY_CANVAS_GROUP (clay_tool_widget_get_item (widget));

  if (priv->focus_widget == child)
    {
      clay_tool_widget_set_focus (child, FALSE);

      priv->focus_widget = NULL;
    }

  if (priv->hover_widget == child)
    {
      clay_tool_widget_leave_notify (child);

      priv->hover_widget = NULL;
    }

  if (! priv->focus_widget)
    {
      priv->focus_widget =
        CLAY_TOOL_WIDGET (clay_container_get_last_child (container));

      if (priv->focus_widget)
        clay_tool_widget_set_focus (priv->focus_widget, TRUE);
    }

  clay_canvas_group_remove_item (canvas_group,
                                 clay_tool_widget_get_item (child));

  clay_tool_widget_changed (widget);
}

static void
clay_tool_widget_group_children_reorder (ClayContainer       *container,
                                         ClayToolWidget      *child,
                                         gint                 old_index,
                                         gint                 new_index,
                                         ClayToolWidgetGroup *group)
{
  ClayToolWidget  *widget = CLAY_TOOL_WIDGET (group);
  ClayCanvasGroup *canvas_group;
  GList           *iter;

  canvas_group = CLAY_CANVAS_GROUP (clay_tool_widget_get_item (widget));

  for (iter = g_queue_peek_head_link (CLAY_LIST (container)->queue);
       iter;
       iter = g_list_next (iter))
    {
      ClayCanvasItem *item = clay_tool_widget_get_item (iter->data);

      clay_canvas_group_remove_item (canvas_group, item);
      clay_canvas_group_add_item    (canvas_group, item);
    }

  clay_tool_widget_changed (widget);
}

static void
clay_tool_widget_group_child_changed (ClayToolWidget      *child,
                                      ClayToolWidgetGroup *group)
{
  ClayToolWidget *widget = CLAY_TOOL_WIDGET (group);

  clay_tool_widget_changed (widget);
}

static void
clay_tool_widget_group_child_response (ClayToolWidget      *child,
                                       gint                 response_id,
                                       ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    clay_tool_widget_response (widget, response_id);
}

static void
clay_tool_widget_group_child_snap_offsets (ClayToolWidget      *child,
                                           gint                 offset_x,
                                           gint                 offset_y,
                                           gint                 width,
                                           gint                 height,
                                           ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    {
      clay_tool_widget_set_snap_offsets (widget,
                                         offset_x, offset_y, width, height);
    }
}

static void
clay_tool_widget_group_child_status (ClayToolWidget      *child,
                                     const gchar         *status,
                                     ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    clay_tool_widget_set_status (widget, status);
}

static void
clay_tool_widget_group_child_status_coords (ClayToolWidget      *child,
                                            const gchar         *title,
                                            gdouble              x,
                                            const gchar         *separator,
                                            gdouble              y,
                                            const gchar         *help,
                                            ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (priv->hover_widget == child)
    clay_tool_widget_set_status_coords (widget, title, x, separator, y, help);
}

static void
clay_tool_widget_group_child_message (ClayToolWidget      *child,
                                      const gchar         *message,
                                      ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (priv->focus_widget == child)
    clay_tool_widget_message_literal (widget, message);
}

static void
clay_tool_widget_group_child_focus_changed (ClayToolWidget      *child,
                                            ClayToolWidgetGroup *group)
{
  ClayToolWidgetGroupPrivate *priv   = group->priv;
  ClayToolWidget             *widget = CLAY_TOOL_WIDGET (group);

  if (clay_tool_widget_get_focus (child))
    {
      if (priv->focus_widget && priv->focus_widget != child)
        clay_tool_widget_set_focus (priv->focus_widget, FALSE);

      priv->focus_widget = child;

      clay_tool_widget_set_focus (widget, TRUE);
    }
  else
    {
      if (priv->focus_widget == child)
        priv->focus_widget = NULL;
    }
}

static ClayToolWidget *
clay_tool_widget_group_get_hover_widget (ClayToolWidgetGroup *group,
                                         const ClayCoords    *coords,
                                         GdkModifierType      state,
                                         gboolean             proximity,
                                         ClayHit             *hit)
{
  ClayToolWidgetGroupPrivate *priv           = group->priv;
  ClayToolWidget             *indirect_child = NULL;
  gboolean                    indirect       = FALSE;
  GList                      *iter;

  for (iter = g_queue_peek_tail_link (CLAY_LIST (priv->children)->queue);
       iter;
       iter = g_list_previous (iter))
    {
      ClayToolWidget *child = iter->data;

      switch (clay_tool_widget_hit (child, coords, state, proximity))
        {
        case CLAY_HIT_DIRECT:
          if (hit) *hit = CLAY_HIT_DIRECT;

          return child;

        case CLAY_HIT_INDIRECT:
          if (! indirect || child == priv->focus_widget)
            indirect_child = child;
          else if (indirect_child != priv->focus_widget)
            indirect_child = NULL;

          indirect = TRUE;

          break;

        case CLAY_HIT_NONE:
          break;
        }
    }

  if (hit) *hit = indirect_child ? CLAY_HIT_INDIRECT : CLAY_HIT_NONE;

  return indirect_child;
}


/*  public functions  */


ClayToolWidget *
clay_tool_widget_group_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_TOOL_WIDGET_GROUP,
                       "shell", shell,
                       NULL);
}

ClayContainer *
clay_tool_widget_group_get_children (ClayToolWidgetGroup *group)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->children;
}

ClayToolWidget *
clay_tool_widget_group_get_focus_widget (ClayToolWidgetGroup *group)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET_GROUP (group), NULL);

  return group->priv->focus_widget;
}

void
clay_tool_widget_group_set_auto_raise (ClayToolWidgetGroup *group,
                                       gboolean             auto_raise)
{
  g_return_if_fail (CLAY_IS_TOOL_WIDGET_GROUP (group));

  group->priv->auto_raise = auto_raise;
}

gboolean
clay_tool_widget_group_get_auto_raise (ClayToolWidgetGroup *group)
{
  g_return_val_if_fail (CLAY_IS_TOOL_WIDGET_GROUP (group), FALSE);

  return group->priv->auto_raise;
}

