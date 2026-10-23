/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolfocus.c
 * Copyright (C) 2020 Ell
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
#include "libclaymath/claymath.h"

#include "display-types.h"

#include "widgets/claywidgets-utils.h"

#include "claycanvasgroup.h"
#include "claycanvashandle.h"
#include "claycanvaslimit.h"
#include "claydisplayshell.h"
#include "claydisplayshell-transform.h"
#include "claydisplayshell-utils.h"
#include "claytoolfocus.h"

#include "clay-intl.h"


#define HANDLE_SIZE   12.0
#define SNAP_DISTANCE 12.0

#define EPSILON       1e-6


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_X,
  PROP_Y,
  PROP_RADIUS,
  PROP_ASPECT_RATIO,
  PROP_ANGLE,
  PROP_INNER_LIMIT,
  PROP_MIDPOINT
};

enum
{
  LIMIT_OUTER,
  LIMIT_INNER,
  LIMIT_MIDDLE,

  N_LIMITS
};


typedef enum
{
  HOVER_NONE,
  HOVER_LIMIT,
  HOVER_HANDLE,
  HOVER_MOVE,
  HOVER_ROTATE
} Hover;


typedef struct
{
  ClayCanvasItem *item;

  GtkOrientation  orientation;
  ClayVector2     dir;
} ClayToolFocusHandle;

typedef struct
{
  ClayCanvasGroup     *group;
  ClayCanvasItem      *item;

  gint                 n_handles;
  ClayToolFocusHandle  handles[4];
} ClayToolFocusLimit;

struct _ClayToolFocusPrivate
{
  ClayLimitType       type;

  gdouble             x;
  gdouble             y;
  gdouble             radius;
  gdouble             aspect_ratio;
  gdouble             angle;

  gdouble             inner_limit;
  gdouble             midpoint;

  ClayToolFocusLimit  limits[N_LIMITS];

  Hover               hover;
  gint                hover_limit;
  gint                hover_handle;
  ClayCanvasItem     *hover_item;

  ClayCanvasItem     *last_hover_item;

  gdouble             saved_x;
  gdouble             saved_y;
  gdouble             saved_radius;
  gdouble             saved_aspect_ratio;
  gdouble             saved_angle;

  gdouble             saved_inner_limit;
  gdouble             saved_midpoint;

  gdouble             orig_x;
  gdouble             orig_y;
};


/*  local function prototypes  */

static void       clay_tool_focus_constructed      (GObject               *object);
static void       clay_tool_focus_set_property     (GObject               *object,
                                                    guint                  property_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void       clay_tool_focus_get_property     (GObject               *object,
                                                    guint                  property_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);

static void       clay_tool_focus_changed          (ClayToolWidget        *widget);
static gint       clay_tool_focus_button_press     (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state,
                                                    ClayButtonPressType    press_type);
static void       clay_tool_focus_button_release   (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state,
                                                    ClayButtonReleaseType  release_type);
static void       clay_tool_focus_motion           (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    guint32                time,
                                                    GdkModifierType        state);
static ClayHit    clay_tool_focus_hit              (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    GdkModifierType        state,
                                                    gboolean               proximity);
static void       clay_tool_focus_hover            (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    GdkModifierType        state,
                                                    gboolean               proximity);
static void       clay_tool_focus_leave_notify     (ClayToolWidget        *widget);
static void       clay_tool_focus_motion_modifier  (ClayToolWidget        *widget,
                                                    GdkModifierType        key,
                                                    gboolean               press,
                                                    GdkModifierType        state);
static void       clay_tool_focus_hover_modifier   (ClayToolWidget        *widget,
                                                    GdkModifierType        key,
                                                    gboolean               press,
                                                    GdkModifierType        state);
static gboolean   clay_tool_focus_get_cursor       (ClayToolWidget        *widget,
                                                    const ClayCoords      *coords,
                                                    GdkModifierType        state,
                                                    ClayCursorType        *cursor,
                                                    ClayToolCursorType    *tool_cursor,
                                                    ClayCursorModifier    *modifier);

static void       clay_tool_focus_update_hover     (ClayToolFocus         *focus,
                                                    const ClayCoords      *coords,
                                                    gboolean               proximity);

static void       clay_tool_focus_update_highlight (ClayToolFocus         *focus);
static void       clay_tool_focus_update_status    (ClayToolFocus         *focus,
                                                    GdkModifierType        state);

static void       clay_tool_focus_save             (ClayToolFocus         *focus);
static void       clay_tool_focus_restore          (ClayToolFocus         *focus);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolFocus, clay_tool_focus,
                            CLAY_TYPE_TOOL_WIDGET)

#define parent_class clay_tool_focus_parent_class


/*  private functions  */

static void
clay_tool_focus_class_init (ClayToolFocusClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = clay_tool_focus_constructed;
  object_class->set_property    = clay_tool_focus_set_property;
  object_class->get_property    = clay_tool_focus_get_property;

  widget_class->changed         = clay_tool_focus_changed;
  widget_class->button_press    = clay_tool_focus_button_press;
  widget_class->button_release  = clay_tool_focus_button_release;
  widget_class->motion          = clay_tool_focus_motion;
  widget_class->hit             = clay_tool_focus_hit;
  widget_class->hover           = clay_tool_focus_hover;
  widget_class->leave_notify    = clay_tool_focus_leave_notify;
  widget_class->motion_modifier = clay_tool_focus_motion_modifier;
  widget_class->hover_modifier  = clay_tool_focus_hover_modifier;
  widget_class->get_cursor      = clay_tool_focus_get_cursor;
  widget_class->update_on_scale = TRUE;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      CLAY_TYPE_LIMIT_TYPE,
                                                      CLAY_LIMIT_CIRCLE,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RADIUS,
                                   g_param_spec_double ("radius", NULL, NULL,
                                                        0.0,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ASPECT_RATIO,
                                   g_param_spec_double ("aspect-ratio", NULL, NULL,
                                                        -1.0,
                                                        +1.0,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("angle", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INNER_LIMIT,
                                   g_param_spec_double ("inner-limit", NULL, NULL,
                                                        0.0,
                                                        1.0,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MIDPOINT,
                                   g_param_spec_double ("midpoint", NULL, NULL,
                                                        0.0,
                                                        1.0,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_tool_focus_init (ClayToolFocus *focus)
{
  ClayToolFocusPrivate *priv;

  priv = clay_tool_focus_get_instance_private (focus);

  focus->priv = priv;

  priv->hover = HOVER_NONE;
}

static void
clay_tool_focus_constructed (GObject *object)
{
  ClayToolFocus        *focus  = CLAY_TOOL_FOCUS (object);
  ClayToolWidget       *widget = CLAY_TOOL_WIDGET (object);
  ClayToolFocusPrivate *priv   = focus->priv;
  gint                  i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  for (i = N_LIMITS - 1; i >= 0; i--)
    {
      priv->limits[i].group = clay_tool_widget_add_group (widget);

      clay_tool_widget_push_group (widget, priv->limits[i].group);

      priv->limits[i].item = clay_tool_widget_add_limit (
        widget,
        CLAY_LIMIT_CIRCLE,
        0.0, 0.0,
        0.0,
        1.0,
        0.0,
        /* dashed = */ i == LIMIT_MIDDLE);

      if (i == LIMIT_OUTER || i == LIMIT_INNER)
        {
          gint j;

          priv->limits[i].n_handles = 4;

          for (j = priv->limits[i].n_handles - 1; j >= 0; j--)
            {
              priv->limits[i].handles[j].item = clay_tool_widget_add_handle (
                widget,
                CLAY_HANDLE_FILLED_CIRCLE,
                0.0, 0.0, HANDLE_SIZE, HANDLE_SIZE,
                CLAY_HANDLE_ANCHOR_CENTER);

              priv->limits[i].handles[j].orientation =
                j % 2 == 0 ? GTK_ORIENTATION_HORIZONTAL :
                             GTK_ORIENTATION_VERTICAL;

              priv->limits[i].handles[j].dir.x = cos (j * G_PI / 2.0);
              priv->limits[i].handles[j].dir.y = sin (j * G_PI / 2.0);
            }
        }

      clay_tool_widget_pop_group (widget);
    }

  clay_tool_focus_changed (widget);
}

static void
clay_tool_focus_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (object);
  ClayToolFocusPrivate *priv  = focus->priv;

  switch (property_id)
    {
    case PROP_TYPE:
      priv->type = g_value_get_enum (value);
      break;

    case PROP_X:
      priv->x = g_value_get_double (value);
      break;

    case PROP_Y:
      priv->y = g_value_get_double (value);
      break;

    case PROP_RADIUS:
      priv->radius = g_value_get_double (value);
      break;

    case PROP_ASPECT_RATIO:
      priv->aspect_ratio = g_value_get_double (value);
      break;

    case PROP_ANGLE:
      priv->angle = g_value_get_double (value);
      break;

    case PROP_INNER_LIMIT:
      priv->inner_limit = g_value_get_double (value);
      break;

    case PROP_MIDPOINT:
      priv->midpoint = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_focus_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (object);
  ClayToolFocusPrivate *priv  = focus->priv;

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, priv->type);
      break;

    case PROP_X:
      g_value_set_double (value, priv->x);
      break;

    case PROP_Y:
      g_value_set_double (value, priv->y);
      break;

    case PROP_RADIUS:
      g_value_set_double (value, priv->radius);
      break;

    case PROP_ASPECT_RATIO:
      g_value_set_double (value, priv->aspect_ratio);
      break;

    case PROP_ANGLE:
      g_value_set_double (value, priv->angle);
      break;

    case PROP_INNER_LIMIT:
      g_value_set_double (value, priv->inner_limit);
      break;

    case PROP_MIDPOINT:
      g_value_set_double (value, priv->midpoint);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_focus_changed (ClayToolWidget *widget)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (widget);
  ClayToolFocusPrivate *priv  = focus->priv;
  gint                  i;

  for (i = 0; i < N_LIMITS; i++)
    {
      clay_canvas_item_begin_change (priv->limits[i].item);

      g_object_set (priv->limits[i].item,
                    "type",         priv->type,
                    "x",            priv->x,
                    "y",            priv->y,
                    "aspect-ratio", priv->aspect_ratio,
                    "angle",        priv->angle,
                    NULL);
    }

  g_object_set (priv->limits[LIMIT_OUTER].item,
                "radius", priv->radius,
                NULL);

  g_object_set (priv->limits[LIMIT_INNER].item,
                "radius", priv->radius * priv->inner_limit,
                NULL);

  g_object_set (priv->limits[LIMIT_MIDDLE].item,
                "radius", priv->radius * (priv->inner_limit         +
                                          (1.0 - priv->inner_limit) *
                                          priv->midpoint),
                NULL);

  for (i = 0; i < N_LIMITS; i++)
    {
      gdouble rx, ry;
      gdouble max_r = 0.0;
      gint    j;

      clay_canvas_limit_get_radii (CLAY_CANVAS_LIMIT (priv->limits[i].item),
                                   &rx, &ry);

      for (j = 0; j < priv->limits[i].n_handles; j++)
        {
          ClayVector2 p = priv->limits[i].handles[j].dir;
          gdouble     r;

          p.x *= rx;
          p.y *= ry;

          clay_vector2_rotate (&p, -priv->angle);

          p.x += priv->x;
          p.y += priv->y;

          clay_canvas_handle_set_position (priv->limits[i].handles[j].item,
                                           p.x, p.y);

          r = clay_canvas_item_transform_distance (
            priv->limits[i].handles[j].item,
            priv->x, priv->y,
            p.x,     p.y);

          max_r = MAX (max_r, r);
        }

      for (j = 0; j < priv->limits[i].n_handles; j++)
        {
          clay_canvas_item_set_visible (priv->limits[i].handles[j].item,
                                        priv->type != CLAY_LIMIT_HORIZONTAL &&
                                        priv->type != CLAY_LIMIT_VERTICAL   &&
                                        max_r >= 1.5 * HANDLE_SIZE);
        }

      clay_canvas_item_end_change (priv->limits[i].item);
    }
}

static gint
clay_tool_focus_button_press (ClayToolWidget      *widget,
                              const ClayCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              ClayButtonPressType  press_type)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (widget);
  ClayToolFocusPrivate *priv  = focus->priv;

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    {
      clay_tool_focus_save (focus);

      priv->orig_x = coords->x;
      priv->orig_y = coords->y;

      return TRUE;
    }

  return FALSE;
}

static void
clay_tool_focus_button_release (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type)
{
  ClayToolFocus *focus = CLAY_TOOL_FOCUS (widget);

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    clay_tool_focus_restore (focus);
}

static void
clay_tool_focus_motion (ClayToolWidget   *widget,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (widget);
  ClayToolFocusPrivate *priv  = focus->priv;
  ClayDisplayShell     *shell = clay_tool_widget_get_shell (widget);
  gboolean              extend;
  gboolean              constrain;

  extend    = state & clay_get_extend_selection_mask ();
  constrain = state & clay_get_constrain_behavior_mask ();

  switch (priv->hover)
    {
    case HOVER_NONE:
      break;

    case HOVER_LIMIT:
      {
        ClayCanvasItem *limit = priv->limits[priv->hover_limit].item;
        gdouble         radius;
        gdouble         outer_radius;
        gdouble         inner_radius;
        gdouble         x,  y;
        gdouble         cx, cy;

        x = coords->x;
        y = coords->y;

        clay_canvas_limit_center_point (CLAY_CANVAS_LIMIT (limit),
                                        x,   y,
                                        &cx, &cy);

        if (clay_canvas_item_transform_distance (limit,
                                                 x,  y,
                                                 cx, cy) <= SNAP_DISTANCE)
          {
            x = cx;
            y = cy;
          }

        if (fabs (fabs (priv->aspect_ratio) - 1.0) <= EPSILON)
          {
            if (priv->radius <= EPSILON)
              {
                g_object_set (focus,
                              "aspect-ratio", 0.0,
                              NULL);
              }
            else
              {
                break;
              }
          }

        radius = clay_canvas_limit_boundary_radius (CLAY_CANVAS_LIMIT (limit),
                                                    x, y);

        outer_radius = priv->radius;
        inner_radius = priv->radius * priv->inner_limit;

        switch (priv->hover_limit)
          {
          case LIMIT_OUTER:
            {
              outer_radius = radius;

              if (extend)
                inner_radius = priv->inner_limit * radius;
              else
                outer_radius = MAX (outer_radius, inner_radius);
            }
            break;

          case LIMIT_INNER:
            {
              inner_radius = radius;

              if (extend)
                {
                  if (priv->inner_limit > EPSILON)
                    outer_radius = inner_radius / priv->inner_limit;
                  else
                    inner_radius = 0.0;
                }
              else
                {
                  inner_radius = MIN (inner_radius, outer_radius);
                }
            }
            break;

          case LIMIT_MIDDLE:
            {
              if (extend)
                {
                  if (priv->inner_limit > EPSILON || priv->midpoint > EPSILON)
                    {
                      outer_radius = radius / (priv->inner_limit         +
                                               (1.0 - priv->inner_limit) *
                                               priv->midpoint);
                      inner_radius = priv->inner_limit * outer_radius;
                    }
                  else
                    {
                      radius = 0.0;
                    }
                }
              else
                {
                  radius = CLAMP (radius, inner_radius, outer_radius);
                }

              if (fabs (outer_radius - inner_radius) > EPSILON)
                {
                  g_object_set (focus,
                                "midpoint", MAX ((radius       - inner_radius) /
                                                 (outer_radius - inner_radius),
                                                 0.0),
                                NULL);
                }
            }
            break;
          }

        g_object_set (focus,
                      "radius", outer_radius,
                      NULL);

        if (outer_radius > EPSILON)
          {
            g_object_set (focus,
                          "inner-limit", inner_radius / outer_radius,
                          NULL);
          }
      }
      break;

    case HOVER_HANDLE:
      {
        ClayToolFocusHandle *handle;
        ClayVector2          e;
        ClayVector2          s;
        ClayVector2          p;
        gdouble              rx, ry;
        gdouble              r;

        handle = &priv->limits[priv->hover_limit].handles[priv->hover_handle];

        e = handle->dir;

        clay_vector2_rotate (&e, -priv->angle);

        s = e;

        clay_canvas_limit_get_radii (
          CLAY_CANVAS_LIMIT (priv->limits[priv->hover_limit].item),
          &rx, &ry);

        if (handle->orientation == GTK_ORIENTATION_HORIZONTAL)
          clay_vector2_mul (&s, ry);
        else
          clay_vector2_mul (&s, rx);

        p.x = coords->x - priv->x;
        p.y = coords->y - priv->y;

        r = clay_vector2_inner_product (&p, &e);
        r = MAX (r, 0.0);

        p = e;

        clay_vector2_mul (&p, r);

        if (extend)
          {
            if (handle->orientation == GTK_ORIENTATION_HORIZONTAL)
              {
                if (rx <= EPSILON && ry > EPSILON)
                  break;

                ry = r * (1.0 - priv->aspect_ratio);
              }
            else
              {
                if (ry <= EPSILON && rx > EPSILON)
                  break;

                rx = r * (1.0 + priv->aspect_ratio);
              }
          }
        else
          {
            if (clay_canvas_item_transform_distance (
                  priv->limits[priv->hover_limit].item,
                  s.x, s.y,
                  p.x, p.y) <= SNAP_DISTANCE * 0.75)
              {
                if (handle->orientation == GTK_ORIENTATION_HORIZONTAL)
                  r = ry;
                else
                  r = rx;
              }
          }

        if (handle->orientation == GTK_ORIENTATION_HORIZONTAL)
          rx = r;
        else
          ry = r;

        r = MAX (rx, ry);

        if (priv->hover_limit == LIMIT_INNER)
          r /= priv->inner_limit;

        g_object_set (focus,
                      "radius", r,
                      NULL);

        if (! extend)
          {
            gdouble aspect_ratio;

            if (fabs (rx - ry) <= EPSILON)
              aspect_ratio = 0.0;
            else if (rx > ry)
              aspect_ratio = 1.0 - ry / rx;
            else
              aspect_ratio = rx / ry - 1.0;

            g_object_set (focus,
                          "aspect-ratio", aspect_ratio,
                          NULL);
          }
      }
      break;

    case HOVER_MOVE:
      g_object_set (focus,
                    "x", priv->saved_x + (coords->x - priv->orig_x),
                    "y", priv->saved_y + (coords->y - priv->orig_y),
                    NULL);
      break;

    case HOVER_ROTATE:
      {
        gdouble angle;
        gdouble orig_angle;

        angle      = atan2 (coords->y    - priv->y, coords->x    - priv->x);
        orig_angle = atan2 (priv->orig_y - priv->y, priv->orig_x - priv->x);

        angle = priv->saved_angle + (angle - orig_angle);

        if (constrain)
          angle = clay_display_shell_constrain_angle (shell, angle, 12);

        g_object_set (focus,
                      "angle", angle,
                      NULL);
      }
      break;
    }
}

static ClayHit
clay_tool_focus_hit (ClayToolWidget   *widget,
                     const ClayCoords *coords,
                     GdkModifierType   state,
                     gboolean          proximity)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (widget);
  ClayToolFocusPrivate *priv  = focus->priv;

  clay_tool_focus_update_hover (focus, coords, proximity);

  switch (priv->hover)
    {
    case HOVER_NONE:
      return CLAY_HIT_NONE;

    case HOVER_LIMIT:
    case HOVER_HANDLE:
      return CLAY_HIT_DIRECT;

    case HOVER_MOVE:
    case HOVER_ROTATE:
      return CLAY_HIT_INDIRECT;
    }

  g_return_val_if_reached (CLAY_HIT_NONE);
}

static void
clay_tool_focus_hover (ClayToolWidget   *widget,
                       const ClayCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity)
{
  ClayToolFocus *focus = CLAY_TOOL_FOCUS (widget);

  clay_tool_focus_update_hover (focus, coords, proximity);

  clay_tool_focus_update_highlight (focus);
  clay_tool_focus_update_status    (focus, state);
}

static void
clay_tool_focus_leave_notify (ClayToolWidget *widget)
{
  ClayToolFocus *focus = CLAY_TOOL_FOCUS (widget);

  clay_tool_focus_update_hover (focus, NULL, FALSE);

  clay_tool_focus_update_highlight (focus);

  CLAY_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static void
clay_tool_focus_motion_modifier (ClayToolWidget  *widget,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state)
{
  ClayToolFocus *focus = CLAY_TOOL_FOCUS (widget);

  clay_tool_focus_update_status (focus, state);
}

static void
clay_tool_focus_hover_modifier (ClayToolWidget  *widget,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state)
{
  ClayToolFocus *focus = CLAY_TOOL_FOCUS (widget);

  clay_tool_focus_update_status (focus, state);
}

static gboolean
clay_tool_focus_get_cursor (ClayToolWidget     *widget,
                            const ClayCoords   *coords,
                            GdkModifierType     state,
                            ClayCursorType     *cursor,
                            ClayToolCursorType *tool_cursor,
                            ClayCursorModifier *modifier)
{
  ClayToolFocus        *focus = CLAY_TOOL_FOCUS (widget);
  ClayToolFocusPrivate *priv  = focus->priv;

  switch (priv->hover)
    {
    case HOVER_NONE:
      return FALSE;

    case HOVER_LIMIT:
    case HOVER_HANDLE:
      *modifier = CLAY_CURSOR_MODIFIER_RESIZE;
      return TRUE;

    case HOVER_MOVE:
      *modifier = CLAY_CURSOR_MODIFIER_MOVE;
      return TRUE;

    case HOVER_ROTATE:
      *modifier = CLAY_CURSOR_MODIFIER_ROTATE;
      return TRUE;
    }

  g_return_val_if_reached (FALSE);
}

static void
clay_tool_focus_update_hover (ClayToolFocus    *focus,
                              const ClayCoords *coords,
                              gboolean          proximity)
{
  ClayToolFocusPrivate *priv            = focus->priv;
  gdouble               min_handle_dist = HANDLE_SIZE;
  gint                  i;

  priv->hover      = HOVER_NONE;
  priv->hover_item = NULL;

  if (! proximity)
    return;

  for (i = 0; i < N_LIMITS; i++)
    {
      gint j;

      for (j = 0; j < priv->limits[i].n_handles; j++)
        {
          ClayCanvasItem *handle = priv->limits[i].handles[j].item;

          if (clay_canvas_item_get_visible (handle))
            {
              gdouble x, y;
              gdouble dist;

              g_object_get (handle,
                            "x", &x,
                            "y", &y,
                            NULL);

              dist = clay_canvas_item_transform_distance (handle,
                                                          x,         y,
                                                          coords->x, coords->y);

              if (dist < min_handle_dist)
                {
                  min_handle_dist = dist;

                  priv->hover        = HOVER_HANDLE;
                  priv->hover_limit  = i;
                  priv->hover_handle = j;
                  priv->hover_item   = handle;
                }
            }
        }
    }

  if (priv->hover != HOVER_NONE)
    return;

  if (  clay_canvas_limit_is_inside (
          CLAY_CANVAS_LIMIT (priv->limits[LIMIT_OUTER].item),
          coords->x, coords->y) &&
      ! clay_canvas_limit_is_inside (
          CLAY_CANVAS_LIMIT (priv->limits[LIMIT_INNER].item),
          coords->x, coords->y))
    {
      if (clay_canvas_item_hit (priv->limits[LIMIT_MIDDLE].item,
                                coords->x, coords->y))
        {
          priv->hover       = HOVER_LIMIT;
          priv->hover_limit = LIMIT_MIDDLE;
          priv->hover_item  = priv->limits[LIMIT_MIDDLE].item;

          return;
        }
    }

  if (! clay_canvas_limit_is_inside (
          CLAY_CANVAS_LIMIT (priv->limits[LIMIT_INNER].item),
          coords->x, coords->y))
    {
      if (clay_canvas_item_hit (priv->limits[LIMIT_OUTER].item,
                                coords->x, coords->y))
        {
          priv->hover       = HOVER_LIMIT;
          priv->hover_limit = LIMIT_OUTER;
          priv->hover_item  = priv->limits[LIMIT_OUTER].item;

          return;
        }
    }

  if (clay_canvas_item_hit (priv->limits[LIMIT_INNER].item,
                            coords->x, coords->y))
    {
      priv->hover       = HOVER_LIMIT;
      priv->hover_limit = LIMIT_INNER;
      priv->hover_item  = priv->limits[LIMIT_INNER].item;

      return;
    }

  if (clay_canvas_limit_is_inside (
        CLAY_CANVAS_LIMIT (priv->limits[LIMIT_OUTER].item),
        coords->x, coords->y))
    {
      priv->hover = HOVER_MOVE;
    }
  else
    {
      priv->hover = HOVER_ROTATE;
    }
}

static void
clay_tool_focus_update_highlight (ClayToolFocus *focus)
{
  ClayToolWidget       *widget = CLAY_TOOL_WIDGET (focus);
  ClayToolFocusPrivate *priv   = focus->priv;
  gint                  i;

  if (priv->hover_item == priv->last_hover_item)
    return;

  if (priv->last_hover_item)
    clay_canvas_item_set_highlight (priv->last_hover_item, FALSE);

  #define RAISE_ITEM(item)                           \
    G_STMT_START                                     \
      {                                              \
        g_object_ref (item);                         \
                                                     \
        clay_tool_widget_remove_item (widget, item); \
        clay_tool_widget_add_item    (widget, item); \
                                                     \
        g_object_unref (item);                       \
      }                                              \
    G_STMT_END

  for (i = N_LIMITS - 1; i >= 0; i--)
    RAISE_ITEM (CLAY_CANVAS_ITEM (priv->limits[i].group));

  if (priv->hover_item)
    {
      clay_canvas_item_set_highlight (priv->hover_item, TRUE);

      RAISE_ITEM (CLAY_CANVAS_ITEM (priv->limits[priv->hover_limit].group));
    }

  #undef RAISE_ITEM

  priv->last_hover_item = priv->hover_item;
}

static void
clay_tool_focus_update_status (ClayToolFocus   *focus,
                               GdkModifierType  state)
{
  ClayToolFocusPrivate *priv                    = focus->priv;
  GdkModifierType       state_mask              = 0;
  const gchar          *message                 = NULL;
  const gchar          *extend_selection_format = NULL;
  const gchar          *toggle_behavior_format  = NULL;
  gchar                *status;

  switch (priv->hover)
    {
    case HOVER_NONE:
      break;

    case HOVER_LIMIT:
      if (! (state & clay_get_extend_selection_mask ()))
        {
          if (priv->hover_limit == LIMIT_MIDDLE)
            message                = _("Click-Drag to change the midpoint");
          else
            message                = _("Click-Drag to resize the limit");

          extend_selection_format  = _("%s to resize the focus");
          state_mask              |= clay_get_extend_selection_mask ();
        }
      else
        {
          message                  = _("Click-Drag to resize the focus");
        }
      break;

    case HOVER_HANDLE:
      if (! (state & clay_get_extend_selection_mask ()))
        {
          message                  = _("Click-Drag to change the aspect ratio");
          extend_selection_format  = _("%s to resize the focus");
          state_mask              |= clay_get_extend_selection_mask ();
        }
      else
        {
          message                  = _("Click-Drag to resize the focus");
        }
      break;

    case HOVER_MOVE:
      message                      = _("Click-Drag to move the focus");
      break;

    case HOVER_ROTATE:
      message                      = _("Click-Drag to rotate the focus");
      toggle_behavior_format       = _("%s for constrained angles");
      state_mask                  |= clay_get_constrain_behavior_mask ();
      break;
    }

  status = clay_suggest_modifiers (message,
                                   ~state & state_mask,
                                   extend_selection_format,
                                   toggle_behavior_format,
                                   NULL);

  clay_tool_widget_set_status (CLAY_TOOL_WIDGET (focus), status);

  g_free (status);
}

static void
clay_tool_focus_save (ClayToolFocus *focus)
{
  ClayToolFocusPrivate *priv = focus->priv;

  priv->saved_x            = priv->x;
  priv->saved_y            = priv->y;
  priv->saved_radius       = priv->radius;
  priv->saved_aspect_ratio = priv->aspect_ratio;
  priv->saved_angle        = priv->angle;

  priv->saved_inner_limit  = priv->inner_limit;
  priv->saved_midpoint     = priv->midpoint;
}

static void
clay_tool_focus_restore (ClayToolFocus *focus)
{
  ClayToolFocusPrivate *priv = focus->priv;

  g_object_set (focus,
                "x",            priv->saved_x,
                "y",            priv->saved_y,
                "radius",       priv->saved_radius,
                "aspect-ratio", priv->saved_aspect_ratio,
                "angle",        priv->saved_angle,

                "inner_limit",  priv->saved_inner_limit,
                "midpoint",     priv->saved_midpoint,

                NULL);
}


/*  public functions  */

ClayToolWidget *
clay_tool_focus_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_TOOL_FOCUS,
                       "shell", shell,
                       NULL);
}
