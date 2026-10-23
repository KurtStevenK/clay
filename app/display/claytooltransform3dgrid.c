/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytool3dtransformgrid.c
 * Copyright (C) 2019 Ell
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

#include "core/clay-transform-3d-utils.h"
#include "core/clay-utils.h"

#include "claydisplayshell.h"
#include "claydisplayshell-transform.h"
#include "claytooltransform3dgrid.h"

#include "clay-intl.h"


#define CONSTRAINT_MIN_DIST   8.0
#define PIXELS_PER_REVOLUTION 1000


enum
{
  PROP_0,
  PROP_MODE,
  PROP_UNIFIED,
  PROP_CONSTRAIN_AXIS,
  PROP_Z_AXIS,
  PROP_LOCAL_FRAME,
  PROP_CAMERA_X,
  PROP_CAMERA_Y,
  PROP_CAMERA_Z,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_OFFSET_Z,
  PROP_ROTATION_ORDER,
  PROP_ANGLE_X,
  PROP_ANGLE_Y,
  PROP_ANGLE_Z,
  PROP_PIVOT_3D_X,
  PROP_PIVOT_3D_Y,
  PROP_PIVOT_3D_Z
};

typedef enum
{
  AXIS_NONE,
  AXIS_X,
  AXIS_Y
} Axis;

struct _ClayToolTransform3DGridPrivate
{
  ClayTransform3DMode mode;
  gboolean            unified;

  gboolean            constrain_axis;
  gboolean            z_axis;
  gboolean            local_frame;

  gdouble             camera_x;
  gdouble             camera_y;
  gdouble             camera_z;

  gdouble             offset_x;
  gdouble             offset_y;
  gdouble             offset_z;

  gint                rotation_order;
  gdouble             angle_x;
  gdouble             angle_y;
  gdouble             angle_z;

  gdouble             pivot_x;
  gdouble             pivot_y;
  gdouble             pivot_z;

  ClayTransformHandle handle;

  gdouble             orig_x;
  gdouble             orig_y;
  gdouble             orig_offset_x;
  gdouble             orig_offset_y;
  gdouble             orig_offset_z;
  ClayMatrix3         orig_transform;

  gdouble             last_x;
  gdouble             last_y;

  Axis                constrained_axis;
};


/*  local function prototypes  */

static void       clay_tool_transform_3d_grid_constructed            (GObject                 *object);
static void       clay_tool_transform_3d_grid_set_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      const GValue            *value,
                                                                      GParamSpec              *pspec);
static void       clay_tool_transform_3d_grid_get_property           (GObject                 *object,
                                                                      guint                    property_id,
                                                                      GValue                  *value,
                                                                      GParamSpec              *pspec);

static gint       clay_tool_transform_3d_grid_button_press           (ClayToolWidget          *widget,
                                                                      const ClayCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state,
                                                                      ClayButtonPressType      press_type);
static void       clay_tool_transform_3d_grid_motion                 (ClayToolWidget          *widget,
                                                                      const ClayCoords        *coords,
                                                                      guint32                  time,
                                                                      GdkModifierType          state);
static void       clay_tool_transform_3d_grid_hover                  (ClayToolWidget          *widget,
                                                                      const ClayCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      gboolean                 proximity);
static void       clay_tool_transform_3d_grid_hover_modifier         (ClayToolWidget          *widget,
                                                                      GdkModifierType          key,
                                                                      gboolean                 press,
                                                                      GdkModifierType          state);
static gboolean   clay_tool_transform_3d_grid_get_cursor              (ClayToolWidget          *widget,
                                                                      const ClayCoords        *coords,
                                                                      GdkModifierType          state,
                                                                      ClayCursorType          *cursor,
                                                                      ClayToolCursorType      *tool_cursor,
                                                                      ClayCursorModifier      *modifier);

static void       clay_tool_transform_3d_grid_update_mode            (ClayToolTransform3DGrid *grid);
static void       clay_tool_transform_3d_grid_reset_motion           (ClayToolTransform3DGrid *grid);
static gboolean   clay_tool_transform_3d_grid_constrain              (ClayToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y,
                                                                      gdouble                  ox,
                                                                      gdouble                  oy,
                                                                      gdouble                 *tx,
                                                                      gdouble                 *ty);

static gboolean   clay_tool_transform_3d_grid_motion_vanishing_point (ClayToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   clay_tool_transform_3d_grid_motion_move            (ClayToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);
static gboolean   clay_tool_transform_3d_grid_motion_rotate          (ClayToolTransform3DGrid *grid,
                                                                      gdouble                  x,
                                                                      gdouble                  y);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolTransform3DGrid, clay_tool_transform_3d_grid,
                            CLAY_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class clay_tool_transform_3d_grid_parent_class


static void
clay_tool_transform_3d_grid_class_init (ClayToolTransform3DGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = clay_tool_transform_3d_grid_constructed;
  object_class->set_property    = clay_tool_transform_3d_grid_set_property;
  object_class->get_property    = clay_tool_transform_3d_grid_get_property;

  widget_class->button_press    = clay_tool_transform_3d_grid_button_press;
  widget_class->motion          = clay_tool_transform_3d_grid_motion;
  widget_class->hover           = clay_tool_transform_3d_grid_hover;
  widget_class->hover_modifier  = clay_tool_transform_3d_grid_hover_modifier;
  widget_class->get_cursor      = clay_tool_transform_3d_grid_get_cursor;

  g_object_class_install_property (object_class, PROP_MODE,
                                   g_param_spec_enum ("mode",
                                                      NULL, NULL,
                                                      CLAY_TYPE_TRANSFORM_3D_MODE,
                                                      CLAY_TRANSFORM_3D_MODE_CAMERA,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_UNIFIED,
                                   g_param_spec_boolean ("unified",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAIN_AXIS,
                                   g_param_spec_boolean ("constrain-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Z_AXIS,
                                   g_param_spec_boolean ("z-axis",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LOCAL_FRAME,
                                   g_param_spec_boolean ("local-frame",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_X,
                                   g_param_spec_double ("camera-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Y,
                                   g_param_spec_double ("camera-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CAMERA_Z,
                                   g_param_spec_double ("camera-z",
                                                        NULL, NULL,
                                                        -(1.0 / 0.0),
                                                        1.0 / 0.0,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_X,
                                   g_param_spec_double ("offset-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Y,
                                   g_param_spec_double ("offset-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OFFSET_Z,
                                   g_param_spec_double ("offset-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROTATION_ORDER,
                                   g_param_spec_int ("rotation-order",
                                                     NULL, NULL,
                                                     0, 6, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_X,
                                   g_param_spec_double ("angle-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Y,
                                   g_param_spec_double ("angle-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ANGLE_Z,
                                   g_param_spec_double ("angle-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_X,
                                   g_param_spec_double ("pivot-3d-x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Y,
                                   g_param_spec_double ("pivot-3d-y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PIVOT_3D_Z,
                                   g_param_spec_double ("pivot-3d-z",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_tool_transform_3d_grid_init (ClayToolTransform3DGrid *grid)
{
  grid->priv = clay_tool_transform_3d_grid_get_instance_private (grid);
}

static void
clay_tool_transform_3d_grid_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "clip-guides",         TRUE,
                "dynamic-handle-size", FALSE,
                NULL);
}

static void
clay_tool_transform_3d_grid_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  ClayToolTransform3DGrid        *grid = CLAY_TOOL_TRANSFORM_3D_GRID (object);
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  switch (property_id)
    {
    case PROP_MODE:
      priv->mode = g_value_get_enum (value);
      clay_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_UNIFIED:
      priv->unified = g_value_get_boolean (value);
      clay_tool_transform_3d_grid_update_mode (grid);
      break;

    case PROP_CONSTRAIN_AXIS:
      priv->constrain_axis = g_value_get_boolean (value);
      clay_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_Z_AXIS:
      priv->z_axis = g_value_get_boolean (value);
      clay_tool_transform_3d_grid_reset_motion (grid);
      break;
    case PROP_LOCAL_FRAME:
      priv->local_frame = g_value_get_boolean (value);
      clay_tool_transform_3d_grid_reset_motion (grid);
      break;

    case PROP_CAMERA_X:
      priv->camera_x = g_value_get_double (value);
      g_object_set (grid,
                    "pivot-x", priv->camera_x,
                    NULL);
      break;
    case PROP_CAMERA_Y:
      priv->camera_y = g_value_get_double (value);
      g_object_set (grid,
                    "pivot-y", priv->camera_y,
                    NULL);
      break;
    case PROP_CAMERA_Z:
      priv->camera_z = g_value_get_double (value);
      break;

    case PROP_OFFSET_X:
      priv->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      priv->offset_y = g_value_get_double (value);
      break;
    case PROP_OFFSET_Z:
      priv->offset_z = g_value_get_double (value);
      break;

    case PROP_ROTATION_ORDER:
      priv->rotation_order = g_value_get_int (value);
      break;
    case PROP_ANGLE_X:
      priv->angle_x = g_value_get_double (value);
      break;
    case PROP_ANGLE_Y:
      priv->angle_y = g_value_get_double (value);
      break;
    case PROP_ANGLE_Z:
      priv->angle_z = g_value_get_double (value);
      break;

    case PROP_PIVOT_3D_X:
      priv->pivot_x = g_value_get_double (value);
      break;
    case PROP_PIVOT_3D_Y:
      priv->pivot_y = g_value_get_double (value);
      break;
    case PROP_PIVOT_3D_Z:
      priv->pivot_z = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_transform_3d_grid_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ClayToolTransform3DGrid        *grid = CLAY_TOOL_TRANSFORM_3D_GRID (object);
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_UNIFIED:
      g_value_set_boolean (value, priv->unified);
      break;

    case PROP_CONSTRAIN_AXIS:
      g_value_set_boolean (value, priv->constrain_axis);
      break;
    case PROP_Z_AXIS:
      g_value_set_boolean (value, priv->z_axis);
      break;
    case PROP_LOCAL_FRAME:
      g_value_set_boolean (value, priv->local_frame);
      break;

    case PROP_CAMERA_X:
      g_value_set_double (value, priv->camera_x);
      break;
    case PROP_CAMERA_Y:
      g_value_set_double (value, priv->camera_y);
      break;
    case PROP_CAMERA_Z:
      g_value_set_double (value, priv->camera_z);
      break;

    case PROP_OFFSET_X:
      g_value_set_double (value, priv->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, priv->offset_y);
      break;
    case PROP_OFFSET_Z:
      g_value_set_double (value, priv->offset_z);
      break;

    case PROP_ROTATION_ORDER:
      g_value_set_int (value, priv->rotation_order);
      break;
    case PROP_ANGLE_X:
      g_value_set_double (value, priv->angle_x);
      break;
    case PROP_ANGLE_Y:
      g_value_set_double (value, priv->angle_y);
      break;
    case PROP_ANGLE_Z:
      g_value_set_double (value, priv->angle_z);
      break;

    case PROP_PIVOT_3D_X:
      g_value_set_double (value, priv->pivot_x);
      break;
    case PROP_PIVOT_3D_Y:
      g_value_set_double (value, priv->pivot_y);
      break;
    case PROP_PIVOT_3D_Z:
      g_value_set_double (value, priv->pivot_z);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
clay_tool_transform_3d_grid_button_press (ClayToolWidget      *widget,
                                          const ClayCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          ClayButtonPressType  press_type)
{
  ClayToolTransform3DGrid        *grid = CLAY_TOOL_TRANSFORM_3D_GRID (widget);
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  priv->handle = CLAY_TOOL_WIDGET_CLASS (parent_class)->button_press (
    widget, coords, time, state, press_type);

  priv->orig_x           = coords->x;
  priv->orig_y           = coords->y;
  priv->orig_offset_x    = priv->offset_x;
  priv->orig_offset_y    = priv->offset_y;
  priv->orig_offset_z    = priv->offset_z;
  priv->last_x           = coords->x;
  priv->last_y           = coords->y;

  clay_tool_transform_3d_grid_reset_motion (grid);

  return priv->handle;
}

void
clay_tool_transform_3d_grid_motion (ClayToolWidget   *widget,
                                    const ClayCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state)
{
  ClayToolTransform3DGrid        *grid = CLAY_TOOL_TRANSFORM_3D_GRID (widget);
  ClayToolTransform3DGridPrivate *priv = grid->priv;
  ClayMatrix3                     transform;
  gboolean                        update = TRUE;

  switch (priv->handle)
    {
    case CLAY_TRANSFORM_HANDLE_PIVOT:
      update = clay_tool_transform_3d_grid_motion_vanishing_point (
        grid, coords->x, coords->y);
      break;

    case CLAY_TRANSFORM_HANDLE_CENTER:
      update = clay_tool_transform_3d_grid_motion_move (
        grid, coords->x, coords->y);
      break;

    case CLAY_TRANSFORM_HANDLE_ROTATION:
      update = clay_tool_transform_3d_grid_motion_rotate (
        grid, coords->x, coords->y);
    break;

    default:
      g_return_if_reached ();
    }

  if (update)
    {
      clay_transform_3d_matrix (&transform,

                                priv->camera_x,
                                priv->camera_y,
                                priv->camera_z,

                                priv->offset_x,
                                priv->offset_y,
                                priv->offset_z,

                                priv->rotation_order,
                                priv->angle_x,
                                priv->angle_y,
                                priv->angle_z,

                                priv->pivot_x,
                                priv->pivot_y,
                                priv->pivot_z);

      g_object_set (widget,
                    "transform", &transform,
                    NULL);
    }
}

static void
clay_tool_transform_3d_grid_hover (ClayToolWidget   *widget,
                                   const ClayCoords *coords,
                                   GdkModifierType   state,
                                   gboolean          proximity)
{
  CLAY_TOOL_WIDGET_CLASS (parent_class)->hover (widget,
                                                coords, state, proximity);

  if (proximity &&
      clay_tool_transform_grid_get_handle (CLAY_TOOL_TRANSFORM_GRID (widget)) ==
      CLAY_TRANSFORM_HANDLE_PIVOT)
    {
      clay_tool_widget_set_status (widget,
                                   _("Click-Drag to move the vanishing point"));
    }
}

static void
clay_tool_transform_3d_grid_hover_modifier (ClayToolWidget  *widget,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state)
{
  ClayToolTransform3DGrid        *grid = CLAY_TOOL_TRANSFORM_3D_GRID (widget);
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  CLAY_TOOL_WIDGET_CLASS (parent_class)->hover_modifier (widget,
                                                         key, press, state);

  priv->local_frame = (state & clay_get_extend_selection_mask ()) != 0;
}

static gboolean
clay_tool_transform_3d_grid_get_cursor (ClayToolWidget     *widget,
                                        const ClayCoords   *coords,
                                        GdkModifierType     state,
                                        ClayCursorType     *cursor,
                                        ClayToolCursorType *tool_cursor,
                                        ClayCursorModifier *modifier)
{
  if (! CLAY_TOOL_WIDGET_CLASS (parent_class)->get_cursor (widget,
                                                           coords,
                                                           state,
                                                           cursor,
                                                           tool_cursor,
                                                           modifier))
    {
      return FALSE;
    }

  if (clay_tool_transform_grid_get_handle (CLAY_TOOL_TRANSFORM_GRID (widget)) ==
      CLAY_TRANSFORM_HANDLE_PIVOT)
    {
      *tool_cursor = CLAY_TOOL_CURSOR_TRANSFORM_3D_CAMERA;
    }

  return TRUE;
}

static void
clay_tool_transform_3d_grid_update_mode (ClayToolTransform3DGrid *grid)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  if (priv->unified)
    {
      g_object_set (grid,
                    "inside-function",  CLAY_TRANSFORM_FUNCTION_MOVE,
                    "outside-function", CLAY_TRANSFORM_FUNCTION_ROTATE,
                    "use-pivot-handle", TRUE,
                    NULL);
    }
  else
    {
      switch (priv->mode)
        {
        case CLAY_TRANSFORM_3D_MODE_CAMERA:
          g_object_set (grid,
                        "inside-function",  CLAY_TRANSFORM_FUNCTION_NONE,
                        "outside-function", CLAY_TRANSFORM_FUNCTION_NONE,
                        "use-pivot-handle", TRUE,
                        NULL);
          break;

        case CLAY_TRANSFORM_3D_MODE_MOVE:
          g_object_set (grid,
                        "inside-function",  CLAY_TRANSFORM_FUNCTION_MOVE,
                        "outside-function", CLAY_TRANSFORM_FUNCTION_MOVE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;

        case CLAY_TRANSFORM_3D_MODE_ROTATE:
          g_object_set (grid,
                        "inside-function",  CLAY_TRANSFORM_FUNCTION_ROTATE,
                        "outside-function", CLAY_TRANSFORM_FUNCTION_ROTATE,
                        "use-pivot-handle", FALSE,
                        NULL);
          break;
        }
    }
}

static void
clay_tool_transform_3d_grid_reset_motion (ClayToolTransform3DGrid *grid)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;
  ClayMatrix3                    *transform;

  priv->constrained_axis = AXIS_NONE;

  g_object_get (grid,
                "transform", &transform,
                NULL);

  priv->orig_transform = *transform;

  g_free (transform);
}

static gboolean
clay_tool_transform_3d_grid_constrain (ClayToolTransform3DGrid *grid,
                                       gdouble                  x,
                                       gdouble                  y,
                                       gdouble                  ox,
                                       gdouble                  oy,
                                       gdouble                 *tx,
                                       gdouble                 *ty)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;

  if (! priv->constrain_axis)
    return TRUE;

  if (priv->constrained_axis == AXIS_NONE)
    {
      ClayDisplayShell *shell;
      gdouble           x1, y1;
      gdouble           x2, y2;

      shell = clay_tool_widget_get_shell (CLAY_TOOL_WIDGET (grid));

      clay_display_shell_transform_xy_f (shell,
                                         priv->last_x, priv->last_y,
                                         &x1,          &y1);
      clay_display_shell_transform_xy_f (shell,
                                         x,            y,
                                         &x2,          &y2);

      if (hypot (x2 - x1, y2 - y1) < CONSTRAINT_MIN_DIST)
        return FALSE;

      if (fabs (*tx - ox) >= fabs (*ty - oy))
        priv->constrained_axis = AXIS_X;
      else
        priv->constrained_axis = AXIS_Y;
    }

  if (priv->constrained_axis == AXIS_X)
    *ty = oy;
  else
    *tx = ox;

  return TRUE;
}

static gboolean
clay_tool_transform_3d_grid_motion_vanishing_point (ClayToolTransform3DGrid *grid,
                                                    gdouble                  x,
                                                    gdouble                  y)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;
  ClayCoords                      c    = {};
  gdouble                         pivot_x;
  gdouble                         pivot_y;

  if (! clay_tool_transform_3d_grid_constrain (grid,
                                               x,            y,
                                               priv->last_x, priv->last_y,
                                               &x,           &y))
    {
      return FALSE;
    }

  c.x = x;
  c.y = y;

  CLAY_TOOL_WIDGET_CLASS (parent_class)->motion (CLAY_TOOL_WIDGET (grid),
                                                 &c, 0, 0);

  g_object_get (grid,
                "pivot-x", &pivot_x,
                "pivot-y", &pivot_y,
                NULL);

  g_object_set (grid,
                "camera-x", pivot_x,
                "camera-y", pivot_y,
                NULL);

  priv->last_x = c.x;
  priv->last_y = c.y;

  return TRUE;
}

static gboolean
clay_tool_transform_3d_grid_motion_move (ClayToolTransform3DGrid *grid,
                                         gdouble                  x,
                                         gdouble                  y)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;
  ClayMatrix4                     matrix;

  if (! priv->z_axis)
    {
      gdouble x1, y1, z1, w1;
      gdouble x2, y2, z2, w2;

      if (! priv->local_frame)
        {
          clay_matrix4_identity (&matrix);
        }
      else
        {
          ClayMatrix3 transform_inv = priv->orig_transform;;

          clay_matrix3_invert (&transform_inv);

          clay_transform_3d_matrix3_to_matrix4 (&transform_inv, &matrix, 2);
        }

      w1 = clay_matrix4_transform_point (&matrix,
                                         priv->last_x, priv->last_y, 0.0,
                                         &x1,          &y1,          &z1);
      w2 = clay_matrix4_transform_point (&matrix,
                                         x,            y,            0.0,
                                         &x2,          &y2,          &z2);

      if (w1 <= 0.0)
        return FALSE;

      if (! clay_tool_transform_3d_grid_constrain (grid,
                                                   x,   y,
                                                   x1,  y1,
                                                   &x2, &y2))
        {
          return FALSE;
        }

      if (priv->local_frame)
        {
          clay_matrix4_identity (&matrix);

          clay_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          clay_matrix4_transform_point (&matrix,
                                        x1,  y1,  z1,
                                        &x1, &y1, &z1);
          clay_matrix4_transform_point (&matrix,
                                        x2,  y2,  z2,
                                        &x2, &y2, &z2);
        }

      if (w2 > 0.0)
        {
          g_object_set (grid,
                        "offset-x", priv->offset_x + (x2 - x1),
                        "offset-y", priv->offset_y + (y2 - y1),
                        "offset-z", priv->offset_z + (z2 - z1),
                        NULL);

          priv->last_x = x;
          priv->last_y = y;
        }
      else
        {
          g_object_set (grid,
                        "offset-x", priv->orig_offset_x,
                        "offset-y", priv->orig_offset_y,
                        "offset-z", priv->orig_offset_z,
                        NULL);

          priv->last_x = priv->orig_x;
          priv->last_y = priv->orig_y;
        }
    }
  else
    {
      ClayVector3 axis;
      gdouble     amount;

      if (! priv->local_frame)
        {
          axis.x = 0.0;
          axis.y = 0.0;
          axis.z = 1.0;
        }
      else
        {
          clay_matrix4_identity (&matrix);

          clay_transform_3d_matrix4_rotate_euler (&matrix,
                                                  priv->rotation_order,
                                                  priv->angle_x,
                                                  priv->angle_y,
                                                  priv->angle_z,
                                                  0.0, 0.0, 0.0);

          axis.x = matrix.coeff[0][2];
          axis.y = matrix.coeff[1][2];
          axis.z = matrix.coeff[2][2];

          if (axis.x < 0.0)
            clay_vector3_neg (&axis);
        }

      amount = x - priv->last_x;

      g_object_set (grid,
                    "offset-x", priv->offset_x + axis.x * amount,
                    "offset-y", priv->offset_y + axis.y * amount,
                    "offset-z", priv->offset_z + axis.z * amount,
                    NULL);

      priv->last_x = x;
      priv->last_y = y;
    }

  return TRUE;
}

static gboolean
clay_tool_transform_3d_grid_motion_rotate (ClayToolTransform3DGrid *grid,
                                           gdouble                  x,
                                           gdouble                  y)
{
  ClayToolTransform3DGridPrivate *priv = grid->priv;
  ClayDisplayShell               *shell;
  ClayMatrix4                     matrix;
  ClayMatrix2                     basis_inv;
  ClayVector3                     omega;
  gdouble                         z_sign;
  gboolean                        local_frame;

  shell = clay_tool_widget_get_shell (CLAY_TOOL_WIDGET (grid));

  local_frame = priv->local_frame && (priv->constrain_axis || priv->z_axis);

  if (! local_frame)
    {
      clay_matrix2_identity (&basis_inv);
      z_sign = 1.0;
    }
  else
    {
      {
        ClayVector3 o, n, c;

        clay_matrix4_identity (&matrix);

        clay_transform_3d_matrix4_rotate_euler (&matrix,
                                                priv->rotation_order,
                                                priv->angle_x,
                                                priv->angle_y,
                                                priv->angle_z,
                                                priv->pivot_x,
                                                priv->pivot_y,
                                                priv->pivot_z);

        clay_transform_3d_matrix4_translate (&matrix,
                                             priv->offset_x,
                                             priv->offset_y,
                                             priv->offset_z);

        clay_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  0.0,
                                      &o.x, &o.y, &o.z);
        clay_matrix4_transform_point (&matrix,
                                      0.0,  0.0,  1.0,
                                      &n.x, &n.y, &n.z);

        c.x = priv->camera_x;
        c.y = priv->camera_y;
        c.z = priv->camera_z;

        clay_vector3_sub (&n, &n, &o);
        clay_vector3_sub (&c, &c, &o);

        z_sign = clay_vector3_inner_product (&c, &n) <= 0.0 ? +1.0 : -1.0;
      }

      {
        ClayVector2 o, u, v;

        clay_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y,
                                      &o.x, &o.y);
        clay_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x + 1.0, priv->pivot_y,
                                      &u.x, &u.y);
        clay_matrix3_transform_point (&priv->orig_transform,
                                      priv->pivot_x, priv->pivot_y + 1.0,
                                      &v.x, &v.y);

        clay_vector2_sub (&u, &u, &o);
        clay_vector2_sub (&v, &v, &o);

        clay_vector2_normalize (&u);
        clay_vector2_normalize (&v);

        basis_inv.coeff[0][0] = u.x;
        basis_inv.coeff[1][0] = u.y;
        basis_inv.coeff[0][1] = v.x;
        basis_inv.coeff[1][1] = v.y;

        clay_matrix2_invert (&basis_inv);
      }
    }

  if (! priv->z_axis)
    {
      ClayVector2 scale;
      gdouble     norm;

      clay_matrix2_transform_point (&basis_inv,
                                    -(y - priv->last_y),
                                    x - priv->last_x,
                                    &omega.x, &omega.y);

      omega.z = 0.0;

      if (! clay_tool_transform_3d_grid_constrain (grid,
                                                   x,        y,
                                                   0.0,      0.0,
                                                   &omega.x, &omega.y))
        {
          return FALSE;
        }

      norm = clay_vector3_length (&omega);

      if (norm > 0.0)
        {
          scale.x = shell->scale_x * omega.y / norm;
          scale.y = shell->scale_y * omega.x / norm;

          clay_vector3_mul (&omega, clay_vector2_length (&scale));
          clay_vector3_mul (&omega, 2.0 * G_PI / PIXELS_PER_REVOLUTION);
        }
    }
  else
    {
      ClayVector2 o;
      ClayVector2 v1 = {priv->last_x,  priv->last_y};
      ClayVector2 v2 = {x,             y};

      g_warn_if_fail (priv->pivot_z == 0.0);

      clay_matrix3_transform_point (&priv->orig_transform,
                                    priv->pivot_x, priv->pivot_y,
                                    &o.x,          &o.y);

      clay_vector2_sub (&v1, &v1, &o);
      clay_vector2_sub (&v2, &v2, &o);

      clay_vector2_normalize (&v1);
      clay_vector2_normalize (&v2);

      omega.x = 0.0;
      omega.y = 0.0;
      omega.z = atan2 (clay_vector2_cross_product (&v1, &v2).y,
                       clay_vector2_inner_product (&v1, &v2));

      omega.z *= z_sign;
    }

  clay_matrix4_identity (&matrix);

  if (local_frame)
    clay_transform_3d_matrix4_rotate (&matrix, &omega);

  clay_transform_3d_matrix4_rotate_euler (&matrix,
                                          priv->rotation_order,
                                          priv->angle_x,
                                          priv->angle_y,
                                          priv->angle_z,
                                          0.0, 0.0, 0.0);

  if (! local_frame)
    clay_transform_3d_matrix4_rotate (&matrix, &omega);

  clay_transform_3d_matrix4_rotate_euler_decompose (&matrix,
                                                    priv->rotation_order,
                                                    &priv->angle_x,
                                                    &priv->angle_y,
                                                    &priv->angle_z);

  priv->last_x = x;
  priv->last_y = y;

  g_object_set (grid,
                "angle-x", priv->angle_x,
                "angle-y", priv->angle_y,
                "angle-z", priv->angle_z,
                NULL);

  return TRUE;
}


/*  public functions  */

ClayToolWidget *
clay_tool_transform_3d_grid_new (ClayDisplayShell *shell,
                                 gdouble           x1,
                                 gdouble           y1,
                                 gdouble           x2,
                                 gdouble           y2,
                                 gdouble           camera_x,
                                 gdouble           camera_y,
                                 gdouble           camera_z)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_TOOL_TRANSFORM_3D_GRID,
                       "shell",      shell,
                       "x1",         x1,
                       "y1",         y1,
                       "x2",         x2,
                       "y2",         y2,
                       "camera-x",   camera_x,
                       "camera-y",   camera_y,
                       "camera-z",   camera_z,
                       "pivot-3d-x", (x1 + x2) / 2.0,
                       "pivot-3d-y", (y1 + y2) / 2.0,
                       "pivot-3d-z", 0.0,
                       NULL);
}
