/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolpath.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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
#include "libclaymath/claymath.h"

#include "display-types.h"

#include "core/claycontext.h"

#include "path/clayanchor.h"
#include "path/claybezierstroke.h"
#include "path/claypath.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydockcontainer.h"
#include "widgets/claymenufactory.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "tools/claytools-utils.h"

#include "menus/menus.h"

#include "claycanvashandle.h"
#include "claycanvasitem-utils.h"
#include "claycanvasline.h"
#include "claycanvaspath.h"
#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claytoolpath.h"

#include "clay-intl.h"


#define TOGGLE_MASK  clay_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  clay_get_toggle_behavior_mask ()


/*  possible path functions  */
typedef enum
{
  VECTORS_SELECT_PATH,
  VECTORS_CREATE_PATH,
  VECTORS_CREATE_STROKE,
  VECTORS_ADD_ANCHOR,
  VECTORS_MOVE_ANCHOR,
  VECTORS_MOVE_ANCHORSET,
  VECTORS_MOVE_HANDLE,
  VECTORS_MOVE_CURVE,
  VECTORS_MOVE_STROKE,
  VECTORS_MOVE_PATH,
  VECTORS_INSERT_ANCHOR,
  VECTORS_DELETE_ANCHOR,
  VECTORS_CONNECT_STROKES,
  VECTORS_DELETE_SEGMENT,
  VECTORS_CONVERT_EDGE,
  VECTORS_FINISHED
} ClayPathFunction;

enum
{
  PROP_0,
  PROP_PATH,
  PROP_EDIT_MODE,
  PROP_POLYGONAL,
  PROP_AUTO_CONNECT
};

enum
{
  BEGIN_CHANGE,
  END_CHANGE,
  ACTIVATE,
  LAST_SIGNAL
};


struct _ClayToolPathPrivate
{
  ClayPath             *path;           /* the current Path data           */
  ClayPathMode          edit_mode;
  gboolean              polygonal;
  gboolean              auto_connect;

  ClayPathFunction      function;       /* function we're performing         */
  ClayAnchorFeatureType restriction;    /* movement restriction              */
  gboolean              modifier_lock;  /* can we toggle the Shift key?      */
  GdkModifierType       saved_state;    /* modifier state at button_press    */
  gdouble               last_x;         /* last x coordinate                 */
  gdouble               last_y;         /* last y coordinate                 */
  gboolean              undo_motion;    /* we need a motion to have an undo  */
  gboolean              have_undo;      /* did we push an undo at            */
                                        /* ..._button_press?                 */

  ClayAnchor           *cur_anchor;     /* the current Anchor                */
  ClayAnchor           *cur_anchor2;    /* secondary Anchor (end on_curve)   */
  ClayStroke           *cur_stroke;     /* the current Stroke                */
  gdouble               cur_position;   /* the current Position on a segment */

  gint                  sel_count;      /* number of selected anchors        */
  ClayAnchor           *sel_anchor;     /* currently selected anchor, NULL   */
                                        /* if multiple anchors are selected  */
  ClayStroke           *sel_stroke;     /* selected stroke                   */

  ClayPathMode          saved_mode;     /* used by modifier_key()            */

  ClayCanvasItem       *canvas_path;
  GList                *items;
};


/*  local function prototypes  */

static void     clay_tool_path_constructed     (GObject               *object);
static void     clay_tool_path_dispose         (GObject               *object);
static void     clay_tool_path_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     clay_tool_path_get_property    (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);

static void     clay_tool_path_changed         (ClayToolWidget        *widget);
static gint     clay_tool_path_button_press    (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonPressType    press_type);
static void     clay_tool_path_button_release  (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonReleaseType  release_type);
static void     clay_tool_path_motion          (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state);
static ClayHit  clay_tool_path_hit             (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     clay_tool_path_hover           (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static gboolean clay_tool_path_key_press       (ClayToolWidget        *widget,
                                                GdkEventKey           *kevent);
static gboolean clay_tool_path_get_cursor      (ClayToolWidget        *widget,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                ClayCursorType        *cursor,
                                                ClayToolCursorType    *tool_cursor,
                                                ClayCursorModifier    *modifier);
static ClayUIManager * clay_tool_path_get_popup (ClayToolWidget       *widget,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                const gchar          **ui_path);

static ClayPathFunction
                   clay_tool_path_get_function (ClayToolPath          *tool_path,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state);

static void     clay_tool_path_update_status   (ClayToolPath          *tool_path,
                                                GdkModifierType        state,
                                                gboolean               proximity);

static void     clay_tool_path_begin_change    (ClayToolPath          *tool_path,
                                                const gchar           *desc);
static void     clay_tool_path_end_change      (ClayToolPath          *tool_path,
                                                gboolean               success);

static void     clay_tool_path_path_visible    (ClayPath              *path,
                                                ClayToolPath          *tool_path);
static void     clay_tool_path_path_freeze     (ClayPath              *path,
                                                ClayToolPath          *tool_path);
static void     clay_tool_path_path_thaw       (ClayPath              *path,
                                                ClayToolPath          *tool_path);
static void     clay_tool_path_verify_state    (ClayToolPath          *tool_path);

static void     clay_tool_path_move_selected_anchors
                                               (ClayToolPath          *tool_path,
                                                gdouble                x,
                                                gdouble                y);
static void     clay_tool_path_delete_selected_anchors
                                               (ClayToolPath          *tool_path);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolPath, clay_tool_path, CLAY_TYPE_TOOL_WIDGET)

#define parent_class clay_tool_path_parent_class

static guint path_signals[LAST_SIGNAL] = { 0 };


static void
clay_tool_path_class_init (ClayToolPathClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = clay_tool_path_constructed;
  object_class->dispose         = clay_tool_path_dispose;
  object_class->set_property    = clay_tool_path_set_property;
  object_class->get_property    = clay_tool_path_get_property;

  widget_class->changed         = clay_tool_path_changed;
  widget_class->focus_changed   = clay_tool_path_changed;
  widget_class->button_press    = clay_tool_path_button_press;
  widget_class->button_release  = clay_tool_path_button_release;
  widget_class->motion          = clay_tool_path_motion;
  widget_class->hit             = clay_tool_path_hit;
  widget_class->hover           = clay_tool_path_hover;
  widget_class->key_press       = clay_tool_path_key_press;
  widget_class->get_cursor      = clay_tool_path_get_cursor;
  widget_class->get_popup       = clay_tool_path_get_popup;

  path_signals[BEGIN_CHANGE] =
    g_signal_new ("begin-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolPathClass, begin_change),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  path_signals[END_CHANGE] =
    g_signal_new ("end-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolPathClass, end_change),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  path_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolPathClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  g_object_class_install_property (object_class, PROP_PATH,
                                   g_param_spec_object ("path", NULL, NULL,
                                                        CLAY_TYPE_PATH,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EDIT_MODE,
                                   g_param_spec_enum ("edit-mode",
                                                      _("Edit Mode"),
                                                      NULL,
                                                      CLAY_TYPE_PATH_MODE,
                                                      CLAY_PATH_MODE_DESIGN,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_POLYGONAL,
                                   g_param_spec_boolean ("polygonal",
                                                         _("Polygonal"),
                                                         _("Restrict editing to polygons"),
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_AUTO_CONNECT,
                                   g_param_spec_boolean ("auto-connect",
                                                         _("Auto-connect on click"),
                                                         _("If checked, clicking on the end node in "
                                                           "Design mode will close the path"),
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
clay_tool_path_init (ClayToolPath *tool_path)
{
  tool_path->private = clay_tool_path_get_instance_private (tool_path);
}

static void
clay_tool_path_constructed (GObject *object)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (object);
  ClayToolWidget      *widget    = CLAY_TOOL_WIDGET (object);
  ClayToolPathPrivate *private   = tool_path->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->canvas_path = clay_tool_widget_add_path (widget, NULL);

  clay_tool_path_changed (widget);
}

static void
clay_tool_path_dispose (GObject *object)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (object);

  clay_tool_path_set_path (tool_path, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_tool_path_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (object);
  ClayToolPathPrivate *private   = tool_path->private;

  switch (property_id)
    {
    case PROP_PATH:
      clay_tool_path_set_path (tool_path, g_value_get_object (value));
      break;
    case PROP_EDIT_MODE:
      private->edit_mode = g_value_get_enum (value);
      break;
    case PROP_POLYGONAL:
      private->polygonal = g_value_get_boolean (value);
      break;
    case PROP_AUTO_CONNECT:
      private->auto_connect = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_path_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (object);
  ClayToolPathPrivate *private   = tool_path->private;

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_object (value, private->path);
      break;
    case PROP_EDIT_MODE:
      g_value_set_enum (value, private->edit_mode);
      break;
    case PROP_POLYGONAL:
      g_value_set_boolean (value, private->polygonal);
      break;
    case PROP_AUTO_CONNECT:
      g_value_set_boolean (value, private->auto_connect);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_remove_func (ClayCanvasItem *item,
                  ClayToolWidget *widget)
{
  clay_tool_widget_remove_item (widget, item);
}

static void
clay_tool_path_changed (ClayToolWidget *widget)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;
  ClayPath            *path      = private->path;

  if (private->items)
    {
      g_list_foreach (private->items, (GFunc) item_remove_func, widget);
      g_list_free (private->items);
      private->items = NULL;
    }

  if (path && clay_path_get_bezier (path))
    {
      ClayStroke *cur_stroke;

      clay_canvas_path_set (private->canvas_path,
                            clay_path_get_bezier (path));
      clay_canvas_item_set_visible (private->canvas_path,
                                    ! clay_item_get_visible (CLAY_ITEM (path)));

      for (cur_stroke = clay_path_stroke_get_next (path, NULL);
           cur_stroke;
           cur_stroke = clay_path_stroke_get_next (path, cur_stroke))
        {
          ClayCanvasItem *item;
          GArray         *coords;
          GList          *draw_anchors;
          GList          *list;
          gboolean        first = TRUE;

          /* anchor handles */
          draw_anchors = clay_stroke_get_draw_anchors (cur_stroke);

          for (list = draw_anchors; list; list = g_list_next (list))
            {
              ClayAnchor *cur_anchor = CLAY_ANCHOR (list->data);

              if (cur_anchor->type == CLAY_ANCHOR_ANCHOR)
                {
                  item =
                    clay_tool_widget_add_handle (widget,
                                                 cur_anchor->selected ?
                                                 CLAY_HANDLE_CIRCLE :
                                                 CLAY_HANDLE_FILLED_CIRCLE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 CLAY_HANDLE_ANCHOR_CENTER);

                  if (first)
                    {
                      gdouble angle = 0.0;
                      ClayAnchor *next;

                      for (next = clay_stroke_anchor_get_next (cur_stroke,
                                                               cur_anchor);
                           next;
                           next = clay_stroke_anchor_get_next (cur_stroke, next))
                        {
                          if (((next->position.x - cur_anchor->position.x) *
                               (next->position.x - cur_anchor->position.x) +
                               (next->position.y - cur_anchor->position.y) *
                               (next->position.y - cur_anchor->position.y)) >= 0.1)
                            break;
                        }

                      if (next)
                        {
                          angle = atan2 (next->position.y - cur_anchor->position.y,
                                         next->position.x - cur_anchor->position.x);
                          g_object_set (item,
                                        "type", (cur_anchor->selected ?
                                                 CLAY_HANDLE_DROP :
                                                 CLAY_HANDLE_FILLED_DROP),
                                        "start-angle", angle,
                                        NULL);
                        }
                    }

                  private->items = g_list_prepend (private->items, item);

                  first = FALSE;
                }
            }

          g_list_free (draw_anchors);

          if (private->sel_count <= 2)
            {
              /* the lines to the control handles */
              coords = clay_stroke_get_draw_lines (cur_stroke);

              if (coords)
                {
                  if (coords->len % 2 == 0)
                    {
                      gint i;

                      for (i = 0; i < coords->len; i += 2)
                        {
                          item = clay_tool_widget_add_line
                            (widget,
                             g_array_index (coords, ClayCoords, i).x,
                             g_array_index (coords, ClayCoords, i).y,
                             g_array_index (coords, ClayCoords, i + 1).x,
                             g_array_index (coords, ClayCoords, i + 1).y);

                          if (clay_tool_widget_get_focus (widget))
                            clay_canvas_item_set_highlight (item, TRUE);

                          private->items = g_list_prepend (private->items, item);
                        }
                    }

                  g_array_free (coords, TRUE);
                }

              /* control handles */
              draw_anchors = clay_stroke_get_draw_controls (cur_stroke);

              for (list = draw_anchors; list; list = g_list_next (list))
                {
                  ClayAnchor *cur_anchor = CLAY_ANCHOR (list->data);

                  item =
                    clay_tool_widget_add_handle (widget,
                                                 CLAY_HANDLE_SQUARE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 CLAY_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 CLAY_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 CLAY_HANDLE_ANCHOR_CENTER);

                  private->items = g_list_prepend (private->items, item);
                }

              g_list_free (draw_anchors);
            }
        }
    }
  else
    {
      clay_canvas_path_set (private->canvas_path, NULL);
    }
}

static gboolean
clay_tool_path_check_writable (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private     = tool_path->private;
  ClayToolWidget      *widget      = CLAY_TOOL_WIDGET (tool_path);
  ClayDisplayShell    *shell       = clay_tool_widget_get_shell (widget);
  ClayItem            *locked_item = NULL;

  if (clay_item_is_content_locked (CLAY_ITEM (private->path), &locked_item) ||
      clay_item_is_position_locked (CLAY_ITEM (private->path), &locked_item))
    {
      clay_tool_widget_message_literal (CLAY_TOOL_WIDGET (tool_path),
                                        _("The selected path is locked."));

      if (locked_item == NULL)
        locked_item = CLAY_ITEM (private->path);

      /* FIXME: this should really be done by the tool */
      clay_tools_blink_lock_box (shell->display->clay, locked_item);

      private->function = VECTORS_FINISHED;

      return FALSE;
    }

  return TRUE;
}

gboolean
clay_tool_path_button_press (ClayToolWidget      *widget,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;

  /* do nothing if we are in a FINISHED state */
  if (private->function == VECTORS_FINISHED)
    return 0;

  g_return_val_if_fail (private->path  != NULL                   ||
                        private->function == VECTORS_SELECT_PATH ||
                        private->function == VECTORS_CREATE_PATH, 0);

  private->undo_motion = FALSE;

  /* save the current modifier state */

  private->saved_state = state;


  /* select a path object */

  if (private->function == VECTORS_SELECT_PATH)
    {
      ClayPath *path;

      if (clay_canvas_item_on_path (private->canvas_path,
                                    coords,
                                    CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                    CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                    NULL, NULL, NULL, NULL, NULL, &path))
        {
          clay_tool_path_set_path (tool_path, path);
        }

      private->function = VECTORS_FINISHED;
    }


  /* create a new vector from scratch */

  if (private->function == VECTORS_CREATE_PATH)
    {
      ClayDisplayShell *shell = clay_tool_widget_get_shell (widget);
      ClayImage        *image = clay_display_get_image (shell->display);
      ClayPath         *path;

      path = clay_path_new (image, _("Unnamed"));
      g_object_ref_sink (path);

      /* Undo step gets added implicitly */
      private->have_undo = TRUE;

      private->undo_motion = TRUE;

      clay_tool_path_set_path (tool_path, path);
      g_object_unref (path);

      private->function = VECTORS_CREATE_STROKE;
    }


  clay_path_freeze (private->path);

  /* create a new stroke */

  if (private->function == VECTORS_CREATE_STROKE &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Add Stroke"));
      private->undo_motion = TRUE;

      private->cur_stroke = clay_bezier_stroke_new ();
      clay_path_stroke_add (private->path, private->cur_stroke);
      g_object_unref (private->cur_stroke);

      private->sel_stroke = private->cur_stroke;
      private->cur_anchor = NULL;
      private->sel_anchor = NULL;
      private->function   = VECTORS_ADD_ANCHOR;
    }


  /* add an anchor to an existing stroke */

  if (private->function == VECTORS_ADD_ANCHOR &&
      clay_tool_path_check_writable (tool_path))
    {
      ClayCoords position = CLAY_COORDS_DEFAULT_VALUES;

      position.x = coords->x;
      position.y = coords->y;

      clay_tool_path_begin_change (tool_path, _("Add Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = clay_bezier_stroke_extend (private->sel_stroke,
                                                       &position,
                                                       private->sel_anchor,
                                                       CLAY_STROKE_EXTEND_EDITABLE);

      private->restriction = CLAY_ANCHOR_FEATURE_SYMMETRIC;

      if (! private->polygonal)
        private->function = VECTORS_MOVE_HANDLE;
      else
        private->function = VECTORS_MOVE_ANCHOR;

      private->cur_stroke = private->sel_stroke;
    }


  /* insertion of an anchor in a curve segment */

  if (private->function == VECTORS_INSERT_ANCHOR &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Insert Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = clay_stroke_anchor_insert (private->cur_stroke,
                                                       private->cur_anchor,
                                                       private->cur_position);
      if (private->cur_anchor)
        {
          if (private->polygonal)
            {
              clay_stroke_anchor_convert (private->cur_stroke,
                                          private->cur_anchor,
                                          CLAY_ANCHOR_FEATURE_EDGE);
            }

          private->function = VECTORS_MOVE_ANCHOR;
        }
      else
        {
          private->function = VECTORS_FINISHED;
        }
    }


  /* move a handle */

  if (private->function == VECTORS_MOVE_HANDLE &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Drag Handle"));

      if (private->cur_anchor->type == CLAY_ANCHOR_ANCHOR)
        {
          if (! private->cur_anchor->selected)
            {
              clay_path_anchor_select (private->path,
                                       private->cur_stroke,
                                       private->cur_anchor,
                                       TRUE, TRUE);
              private->undo_motion = TRUE;
            }

          clay_canvas_item_on_path_handle (private->canvas_path,
                                           private->path, coords,
                                           CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                           CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                           CLAY_ANCHOR_CONTROL, TRUE,
                                           &private->cur_anchor,
                                           &private->cur_stroke);
          if (! private->cur_anchor)
            private->function = VECTORS_FINISHED;
        }
    }


  /* move an anchor */

  if (private->function == VECTORS_MOVE_ANCHOR &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Drag Anchor"));

      if (! private->cur_anchor->selected)
        {
          clay_path_anchor_select (private->path,
                                   private->cur_stroke,
                                   private->cur_anchor,
                                   TRUE, TRUE);
          private->undo_motion = TRUE;
        }
    }


  /* move multiple anchors */

  if (private->function == VECTORS_MOVE_ANCHORSET &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Drag Anchors"));

      if (state & TOGGLE_MASK)
        {
          clay_path_anchor_select (private->path,
                                   private->cur_stroke,
                                   private->cur_anchor,
                                   !private->cur_anchor->selected,
                                   FALSE);
          private->undo_motion = TRUE;

          if (private->cur_anchor->selected == FALSE)
            private->function = VECTORS_FINISHED;
        }
    }


  /* move a curve segment directly */

  if (private->function == VECTORS_MOVE_CURVE &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Drag Curve"));

      /* the magic numbers are taken from the "feel good" parameter
       * from clay_bezier_stroke_point_move_relative in claybezierstroke.c. */
      if (private->cur_position < 5.0 / 6.0)
        {
          clay_path_anchor_select (private->path,
                                   private->cur_stroke,
                                   private->cur_anchor, TRUE, TRUE);
          private->undo_motion = TRUE;
        }

      if (private->cur_position > 1.0 / 6.0)
        {
          clay_path_anchor_select (private->path,
                                   private->cur_stroke,
                                   private->cur_anchor2, TRUE,
                                   (private->cur_position >= 5.0 / 6.0));
          private->undo_motion = TRUE;
        }

    }


  /* connect two strokes */

  if (private->function == VECTORS_CONNECT_STROKES &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Connect Strokes"));
      private->undo_motion = TRUE;

      clay_stroke_connect_stroke (private->sel_stroke,
                                  private->sel_anchor,
                                  private->cur_stroke,
                                  private->cur_anchor);

      if (private->cur_stroke != private->sel_stroke &&
          clay_stroke_is_empty (private->cur_stroke))
        {
          clay_path_stroke_remove (private->path,
                                   private->cur_stroke);
        }

      private->sel_anchor = private->cur_anchor;
      private->cur_stroke = private->sel_stroke;

      clay_path_anchor_select (private->path,
                               private->sel_stroke,
                               private->sel_anchor, TRUE, TRUE);

      private->function = VECTORS_FINISHED;
    }


  /* move a stroke or all strokes of a path object */

  if ((private->function == VECTORS_MOVE_STROKE ||
       private->function == VECTORS_MOVE_PATH) &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Drag Path"));

      /* Work is being done in clay_tool_path_motion()... */
    }


  /* convert an anchor to something that looks like an edge */

  if (private->function == VECTORS_CONVERT_EDGE &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Convert Edge"));
      private->undo_motion = TRUE;

      clay_stroke_anchor_convert (private->cur_stroke,
                                  private->cur_anchor,
                                  CLAY_ANCHOR_FEATURE_EDGE);

      if (private->cur_anchor->type == CLAY_ANCHOR_ANCHOR)
        {
          clay_path_anchor_select (private->path,
                                   private->cur_stroke,
                                   private->cur_anchor, TRUE, TRUE);

          private->function = VECTORS_MOVE_ANCHOR;
        }
      else
        {
          private->cur_stroke = NULL;
          private->cur_anchor = NULL;

          /* avoid doing anything stupid */
          private->function = VECTORS_FINISHED;
        }
    }


  /* removal of a node in a stroke */

  if (private->function == VECTORS_DELETE_ANCHOR &&
      clay_tool_path_check_writable (tool_path))
    {
      clay_tool_path_begin_change (tool_path, _("Delete Anchor"));
      private->undo_motion = TRUE;

      clay_stroke_anchor_delete (private->cur_stroke,
                                 private->cur_anchor);

      if (clay_stroke_is_empty (private->cur_stroke))
        clay_path_stroke_remove (private->path,
                                 private->cur_stroke);

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function = VECTORS_FINISHED;
    }


  /* deleting a segment (opening up a stroke) */

  if (private->function == VECTORS_DELETE_SEGMENT &&
      clay_tool_path_check_writable (tool_path))
    {
      ClayStroke *new_stroke;

      clay_tool_path_begin_change (tool_path, _("Delete Segment"));
      private->undo_motion = TRUE;

      new_stroke = clay_stroke_open (private->cur_stroke,
                                     private->cur_anchor);
      if (new_stroke)
        {
          clay_path_stroke_add (private->path, new_stroke);
          g_object_unref (new_stroke);
        }

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function   = VECTORS_FINISHED;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  clay_path_thaw (private->path);

  return 1;
}

void
clay_tool_path_button_release (ClayToolWidget        *widget,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;

  private->function = VECTORS_FINISHED;

  if (private->have_undo)
    {
      if (! private->undo_motion ||
          release_type == CLAY_BUTTON_RELEASE_CANCEL)
        {
          clay_tool_path_end_change (tool_path, FALSE);
        }
      else
        {
          clay_tool_path_end_change (tool_path, TRUE);
        }
    }
}

void
clay_tool_path_motion (ClayToolWidget   *widget,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;
  ClayCoords           position  = CLAY_COORDS_DEFAULT_VALUES;
  ClayAnchor          *anchor;

  if (private->function == VECTORS_FINISHED)
    return;

  position.x = coords->x;
  position.y = coords->y;

  clay_path_freeze (private->path);

  if ((private->saved_state & TOGGLE_MASK) != (state & TOGGLE_MASK))
    private->modifier_lock = FALSE;

  if (! private->modifier_lock)
    {
      if (state & TOGGLE_MASK)
        {
          private->restriction = CLAY_ANCHOR_FEATURE_SYMMETRIC;
        }
      else
        {
          private->restriction = CLAY_ANCHOR_FEATURE_NONE;
        }
    }

  switch (private->function)
    {
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_HANDLE:
      anchor = private->cur_anchor;

      if (anchor)
        {
          clay_stroke_anchor_move_absolute (private->cur_stroke,
                                            private->cur_anchor,
                                            &position,
                                            private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        {
          clay_tool_path_move_selected_anchors (tool_path,
                                                coords->x - private->last_x,
                                                coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else
        {
          clay_stroke_point_move_absolute (private->cur_stroke,
                                           private->cur_anchor,
                                           private->cur_position,
                                           &position,
                                           private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_ANCHORSET:
      clay_tool_path_move_selected_anchors (tool_path,
                                            coords->x - private->last_x,
                                            coords->y - private->last_y);
      private->undo_motion = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      if (private->cur_stroke)
        {
          clay_stroke_translate (private->cur_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else if (private->sel_stroke)
        {
          clay_stroke_translate (private->sel_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_PATH:
      clay_item_translate (CLAY_ITEM (private->path),
                           coords->x - private->last_x,
                           coords->y - private->last_y, FALSE);
      private->undo_motion = TRUE;
      break;

    default:
      break;
    }

  clay_path_thaw (private->path);

  private->last_x = coords->x;
  private->last_y = coords->y;
}

ClayHit
clay_tool_path_hit (ClayToolWidget   *widget,
                    const ClayCoords *coords,
                    GdkModifierType   state,
                    gboolean          proximity)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (widget);

  switch (clay_tool_path_get_function (tool_path, coords, state))
    {
    case VECTORS_SELECT_PATH:
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_ANCHORSET:
    case VECTORS_MOVE_HANDLE:
    case VECTORS_MOVE_CURVE:
    case VECTORS_MOVE_STROKE:
    case VECTORS_DELETE_ANCHOR:
    case VECTORS_DELETE_SEGMENT:
    case VECTORS_INSERT_ANCHOR:
    case VECTORS_CONNECT_STROKES:
    case VECTORS_CONVERT_EDGE:
      return CLAY_HIT_DIRECT;

    case VECTORS_CREATE_PATH:
    case VECTORS_CREATE_STROKE:
    case VECTORS_ADD_ANCHOR:
    case VECTORS_MOVE_PATH:
      return CLAY_HIT_INDIRECT;

    case VECTORS_FINISHED:
      return CLAY_HIT_NONE;
    }

  return CLAY_HIT_NONE;
}

void
clay_tool_path_hover (ClayToolWidget   *widget,
                      const ClayCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;

  private->function = clay_tool_path_get_function (tool_path, coords, state);

  clay_tool_path_update_status (tool_path, state, proximity);
}

static gboolean
clay_tool_path_key_press (ClayToolWidget *widget,
                          GdkEventKey    *kevent)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;
  ClayDisplayShell    *shell;
  gdouble              xdist, ydist;
  gdouble              pixels = 1.0;

  if (! private->path)
    return FALSE;

  shell = clay_tool_widget_get_shell (widget);

  if (kevent->state & clay_get_extend_selection_mask ())
    pixels = 10.0;

  if (kevent->state & clay_get_toggle_behavior_mask ())
    pixels = 50.0;

  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      g_signal_emit (tool_path, path_signals[ACTIVATE], 0,
                     kevent->state);
      break;

    case GDK_KEY_BackSpace:
    case GDK_KEY_Delete:
      clay_tool_path_delete_selected_anchors (tool_path);
      break;

    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      xdist = FUNSCALEX (shell, pixels);
      ydist = FUNSCALEY (shell, pixels);

      clay_tool_path_begin_change (tool_path, _("Move Anchors"));
      clay_path_freeze (private->path);

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          clay_tool_path_move_selected_anchors (tool_path, -xdist, 0);
          break;

        case GDK_KEY_Right:
          clay_tool_path_move_selected_anchors (tool_path, xdist, 0);
          break;

        case GDK_KEY_Up:
          clay_tool_path_move_selected_anchors (tool_path, 0, -ydist);
          break;

        case GDK_KEY_Down:
          clay_tool_path_move_selected_anchors (tool_path, 0, ydist);
          break;

        default:
          break;
        }

      clay_path_thaw (private->path);
      clay_tool_path_end_change (tool_path, TRUE);
      break;

    case GDK_KEY_Escape:
      if (private->edit_mode != CLAY_PATH_MODE_DESIGN)
        g_object_set (private,
                      "path-edit-mode", CLAY_PATH_MODE_DESIGN,
                      NULL);
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
clay_tool_path_get_cursor (ClayToolWidget     *widget,
                           const ClayCoords   *coords,
                           GdkModifierType     state,
                           ClayCursorType     *cursor,
                           ClayToolCursorType *tool_cursor,
                           ClayCursorModifier *modifier)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;

  *tool_cursor = CLAY_TOOL_CURSOR_PATHS;
  *modifier    = CLAY_CURSOR_MODIFIER_NONE;

  switch (private->function)
    {
    case VECTORS_SELECT_PATH:
      *tool_cursor = CLAY_TOOL_CURSOR_HAND;
      break;

    case VECTORS_CREATE_PATH:
    case VECTORS_CREATE_STROKE:
      *modifier = CLAY_CURSOR_MODIFIER_CONTROL;
      break;

    case VECTORS_ADD_ANCHOR:
    case VECTORS_INSERT_ANCHOR:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = CLAY_CURSOR_MODIFIER_PLUS;
      break;

    case VECTORS_DELETE_ANCHOR:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = CLAY_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_DELETE_SEGMENT:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = CLAY_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_HANDLE:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONVERT_EDGE:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = CLAY_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_ANCHOR:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_CURVE:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_STROKE:
    case VECTORS_MOVE_PATH:
      *modifier = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_ANCHORSET:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONNECT_STROKES:
      *tool_cursor = CLAY_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = CLAY_CURSOR_MODIFIER_JOIN;
      break;

    default:
      *modifier = CLAY_CURSOR_MODIFIER_BAD;
      break;
    }

  return TRUE;
}

static ClayUIManager *
clay_tool_path_get_popup (ClayToolWidget    *widget,
                          const ClayCoords  *coords,
                          GdkModifierType    state,
                          const gchar      **ui_path)
{
  ClayToolPath        *tool_path = CLAY_TOOL_PATH (widget);
  ClayToolPathPrivate *private   = tool_path->private;
  ClayDisplayShell    *shell     = clay_tool_widget_get_shell (widget);
  ClayImageWindow     *image_window;
  ClayDialogFactory   *dialog_factory;
  ClayMenuFactory     *menu_factory;
  ClayUIManager       *ui_manager;

  image_window   = clay_display_shell_get_window (shell);
  dialog_factory = clay_dock_container_get_dialog_factory (CLAY_DOCK_CONTAINER (image_window));

  menu_factory   = menus_get_global_menu_factory (clay_dialog_factory_get_context (dialog_factory)->clay);
  ui_manager     = clay_menu_factory_get_manager (menu_factory,
                                                  "<ToolPath>", widget);

  /* we're using a side effects of clay_tool_path_get_function
   * that update the private->cur_* variables. */
  clay_tool_path_get_function (tool_path, coords, state);

  if (private->cur_stroke)
    {
      clay_ui_manager_update (ui_manager, widget);

      *ui_path = "/tool-path-popup";
      return ui_manager;
    }

  return NULL;
}

static ClayPathFunction
clay_tool_path_get_function (ClayToolPath     *tool_path,
                             const ClayCoords *coords,
                             GdkModifierType   state)
{
  ClayToolPathPrivate *private    = tool_path->private;
  ClayAnchor          *anchor     = NULL;
  ClayAnchor          *anchor2    = NULL;
  ClayStroke          *stroke     = NULL;
  gdouble              position   = -1;
  gboolean             on_handle  = FALSE;
  gboolean             on_curve   = FALSE;
  gboolean             on_path    = FALSE;
  ClayPathFunction     function   = VECTORS_FINISHED;

  private->modifier_lock = FALSE;

  /* are we hovering the current path on the current display? */
  if (private->path)
    {
      on_handle = clay_canvas_item_on_path_handle (private->canvas_path,
                                                   private->path,
                                                   coords,
                                                   CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   CLAY_ANCHOR_ANCHOR,
                                                   private->sel_count > 2,
                                                   &anchor, &stroke);

      if (! on_handle)
        on_curve = clay_canvas_item_on_path_curve (private->canvas_path,
                                                   private->path,
                                                   coords,
                                                   CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                                   NULL,
                                                   &position, &anchor,
                                                   &anchor2, &stroke);
    }

  if (! on_handle && ! on_curve)
    {
      on_path = clay_canvas_item_on_path (private->canvas_path,
                                          coords,
                                          CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                          CLAY_CANVAS_HANDLE_SIZE_CIRCLE,
                                          NULL, NULL, NULL, NULL, NULL,
                                          NULL);
    }

  private->cur_position = position;
  private->cur_anchor   = anchor;
  private->cur_anchor2  = anchor2;
  private->cur_stroke   = stroke;

  switch (private->edit_mode)
    {
    case CLAY_PATH_MODE_DESIGN:
      if (! private->path)
        {
          if (on_path)
            {
              function = VECTORS_SELECT_PATH;
            }
          else
            {
              function               = VECTORS_CREATE_PATH;
              private->restriction   = CLAY_ANCHOR_FEATURE_SYMMETRIC;
              private->modifier_lock = TRUE;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == CLAY_ANCHOR_ANCHOR)
            {
              if (! (state & TOGGLE_MASK)                         &&
                  private->auto_connect                           &&
                  private->sel_anchor                             &&
                  private->sel_anchor != anchor                   &&
                  clay_stroke_is_extendable (private->sel_stroke,
                                             private->sel_anchor) &&
                  clay_stroke_is_extendable (stroke, anchor))
                {
                  function = VECTORS_CONNECT_STROKES;
                }
              else
                {
                  if (state & TOGGLE_MASK)
                    {
                      function = VECTORS_MOVE_ANCHORSET;
                    }
                  else
                    {
                      if (private->sel_count >= 2 && anchor->selected)
                        function = VECTORS_MOVE_ANCHORSET;
                      else
                        function = VECTORS_MOVE_ANCHOR;
                    }
                }
            }
          else
            {
              function = VECTORS_MOVE_HANDLE;

              if (state & TOGGLE_MASK)
                private->restriction = CLAY_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = CLAY_ANCHOR_FEATURE_NONE;
            }
        }
      else if (on_curve)
        {
          if (clay_stroke_point_is_movable (stroke, anchor, position))
            {
              function = VECTORS_MOVE_CURVE;

              if (state & TOGGLE_MASK)
                private->restriction = CLAY_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = CLAY_ANCHOR_FEATURE_NONE;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else
        {
          if (private->sel_stroke &&
              private->sel_anchor &&
              clay_stroke_is_extendable (private->sel_stroke,
                                         private->sel_anchor) &&
              ! (state & TOGGLE_MASK))
            function = VECTORS_ADD_ANCHOR;
          else
            function = VECTORS_CREATE_STROKE;

          private->restriction   = CLAY_ANCHOR_FEATURE_SYMMETRIC;
          private->modifier_lock = TRUE;
        }

      break;

    case CLAY_PATH_MODE_EDIT:
      if (! private->path)
        {
          if (on_path)
            {
              function = VECTORS_SELECT_PATH;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == CLAY_ANCHOR_ANCHOR)
            {
              if (! (state & TOGGLE_MASK) &&
                  private->sel_anchor &&
                  private->sel_anchor != anchor &&
                  clay_stroke_is_extendable (private->sel_stroke,
                                             private->sel_anchor) &&
                  clay_stroke_is_extendable (stroke, anchor))
                {
                  function = VECTORS_CONNECT_STROKES;
                }
              else
                {
                  if (state & TOGGLE_MASK)
                    {
                      function = VECTORS_DELETE_ANCHOR;
                    }
                  else
                    {
                      if (private->polygonal)
                        function = VECTORS_MOVE_ANCHOR;
                      else
                        function = VECTORS_MOVE_HANDLE;
                    }
                }
            }
          else
            {
              if (state & TOGGLE_MASK)
                function = VECTORS_CONVERT_EDGE;
              else
                function = VECTORS_MOVE_HANDLE;
            }
        }
      else if (on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              function = VECTORS_DELETE_SEGMENT;
            }
          else if (clay_stroke_anchor_is_insertable (stroke, anchor, position))
            {
              function = VECTORS_INSERT_ANCHOR;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else
        {
          function = VECTORS_FINISHED;
        }

      break;

    case CLAY_PATH_MODE_MOVE:
      if (! private->path)
        {
          if (on_path)
            {
              function = VECTORS_SELECT_PATH;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else if (on_handle || on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              function = VECTORS_MOVE_PATH;
            }
          else
            {
              function = VECTORS_MOVE_STROKE;
            }
        }
      else
        {
          if (on_path)
            {
              function = VECTORS_SELECT_PATH;
            }
          else
            {
              function = VECTORS_MOVE_PATH;
            }
        }
      break;
    }

  return function;
}

static void
clay_tool_path_update_status (ClayToolPath    *tool_path,
                              GdkModifierType  state,
                              gboolean         proximity)
{
  ClayToolPathPrivate *private     = tool_path->private;
  GdkModifierType      extend_mask = clay_get_extend_selection_mask ();
  GdkModifierType      toggle_mask = clay_get_toggle_behavior_mask ();
  const gchar         *status      = NULL;
  gboolean             free_status = FALSE;

  if (! proximity)
    {
      clay_tool_widget_set_status (CLAY_TOOL_WIDGET (tool_path), NULL);
      return;
    }

  switch (private->function)
    {
    case VECTORS_SELECT_PATH:
      status = _("Click to pick path to edit");
      break;

    case VECTORS_CREATE_PATH:
      status = _("Click to create a new path");
      break;

    case VECTORS_CREATE_STROKE:
      status = _("Click to create a new component of the path");
      break;

    case VECTORS_ADD_ANCHOR:
      status = clay_suggest_modifiers (_("Click or Click-Drag to create "
                                         "a new anchor"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_ANCHOR:
      if (private->edit_mode != CLAY_PATH_MODE_EDIT)
        {
          status = clay_suggest_modifiers (_("Click-Drag to move the "
                                             "anchor around"),
                                           toggle_mask & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
        }
      else
        status = _("Click-Drag to move the anchor around");
      break;

    case VECTORS_MOVE_ANCHORSET:
      status = _("Click-Drag to move the anchors around");
      break;

    case VECTORS_MOVE_HANDLE:
      if (private->restriction != CLAY_ANCHOR_FEATURE_SYMMETRIC)
        {
          status = clay_suggest_modifiers (_("Click-Drag to move the "
                                             "handle around"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      else
        {
          status = clay_suggest_modifiers (_("Click-Drag to move the "
                                             "handles around symmetrically"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      free_status = TRUE;
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        status = clay_suggest_modifiers (_("Click-Drag to move the "
                                           "anchors around"),
                                         extend_mask & ~state,
                                         NULL, NULL, NULL);
      else
        status = clay_suggest_modifiers (_("Click-Drag to change the "
                                           "shape of the curve"),
                                         extend_mask & ~state,
                                         _("%s: symmetrical"), NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      status = clay_suggest_modifiers (_("Click-Drag to move the "
                                         "component around"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_PATH:
      status = _("Click-Drag to move the path around");
      break;

    case VECTORS_INSERT_ANCHOR:
      status = clay_suggest_modifiers (_("Click-Drag to insert an anchor "
                                         "on the path"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_DELETE_ANCHOR:
      status = _("Click to delete this anchor");
      break;

    case VECTORS_CONNECT_STROKES:
      status = _("Click to connect this anchor "
                 "with the selected endpoint");
      break;

    case VECTORS_DELETE_SEGMENT:
      status = _("Click to open up the path");
      break;

    case VECTORS_CONVERT_EDGE:
      status = _("Click to make this node angular");
      break;

    case VECTORS_FINISHED:
      status = _("Clicking here does nothing, try clicking on path elements.");
      break;
    }

  clay_tool_widget_set_status (CLAY_TOOL_WIDGET (tool_path), status);

  if (free_status)
    g_free ((gchar *) status);
}

static void
clay_tool_path_begin_change (ClayToolPath *tool_path,
                             const gchar  *desc)
{
  ClayToolPathPrivate *private = tool_path->private;

  g_return_if_fail (private->path != NULL);

  /* don't push two undos */
  if (private->have_undo)
    return;

  g_signal_emit (tool_path, path_signals[BEGIN_CHANGE], 0,
                 desc);

  private->have_undo = TRUE;
}

static void
clay_tool_path_end_change (ClayToolPath *tool_path,
                           gboolean      success)
{
  ClayToolPathPrivate *private = tool_path->private;

  private->have_undo   = FALSE;
  private->undo_motion = FALSE;

  g_signal_emit (tool_path, path_signals[END_CHANGE], 0,
                 success);
}

static void
clay_tool_path_path_visible (ClayPath     *path,
                             ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;

  clay_canvas_item_set_visible (private->canvas_path,
                                ! clay_item_get_visible (CLAY_ITEM (path)));
}

static void
clay_tool_path_path_freeze (ClayPath     *path,
                            ClayToolPath *tool_path)
{
}

static void
clay_tool_path_path_thaw (ClayPath     *path,
                          ClayToolPath *tool_path)
{
  /*  Ok, the vector might have changed externally (e.g. Undo) we need
   *  to validate our internal state.
   */
  clay_tool_path_verify_state (tool_path);
  clay_tool_path_changed (CLAY_TOOL_WIDGET (tool_path));
}

static void
clay_tool_path_verify_state (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private          = tool_path->private;
  ClayStroke          *cur_stroke       = NULL;
  gboolean             cur_anchor_valid = FALSE;
  gboolean             cur_stroke_valid = FALSE;

  private->sel_count  = 0;
  private->sel_anchor = NULL;
  private->sel_stroke = NULL;

  if (! private->path)
    {
      private->cur_position = -1;
      private->cur_anchor   = NULL;
      private->cur_stroke   = NULL;
      return;
    }

  while ((cur_stroke = clay_path_stroke_get_next (private->path,
                                                  cur_stroke)))
    {
      GList *anchors;
      GList *list;

      /* anchor handles */
      anchors = clay_stroke_get_draw_anchors (cur_stroke);

      if (cur_stroke == private->cur_stroke)
        cur_stroke_valid = TRUE;

      for (list = anchors; list; list = g_list_next (list))
        {
          ClayAnchor *cur_anchor = list->data;

          if (cur_anchor == private->cur_anchor)
            cur_anchor_valid = TRUE;

          if (cur_anchor->type == CLAY_ANCHOR_ANCHOR &&
              cur_anchor->selected)
            {
              private->sel_count++;
              if (private->sel_count == 1)
                {
                  private->sel_anchor = cur_anchor;
                  private->sel_stroke = cur_stroke;
                }
              else
                {
                  private->sel_anchor = NULL;
                  private->sel_stroke = NULL;
                }
            }
        }

      g_list_free (anchors);

      anchors = clay_stroke_get_draw_controls (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          ClayAnchor *cur_anchor = list->data;

          if (cur_anchor == private->cur_anchor)
            cur_anchor_valid = TRUE;
        }

      g_list_free (anchors);
    }

  if (! cur_stroke_valid)
    private->cur_stroke = NULL;

  if (! cur_anchor_valid)
    private->cur_anchor = NULL;
}

static void
clay_tool_path_move_selected_anchors (ClayToolPath *tool_path,
                                      gdouble       x,
                                      gdouble       y)
{
  ClayToolPathPrivate *private = tool_path->private;
  ClayAnchor          *cur_anchor;
  ClayStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  ClayCoords           offset = { 0.0, };

  offset.x = x;
  offset.y = y;

  while ((cur_stroke = clay_path_stroke_get_next (private->path,
                                                  cur_stroke)))
    {
      /* anchors */
      anchors = clay_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = CLAY_ANCHOR (list->data);

          if (cur_anchor->selected)
            clay_stroke_anchor_move_relative (cur_stroke,
                                              cur_anchor,
                                              &offset,
                                              CLAY_ANCHOR_FEATURE_NONE);
        }

      g_list_free (anchors);
    }
}

static void
clay_tool_path_delete_selected_anchors (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;
  ClayAnchor          *cur_anchor;
  ClayStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  gboolean             have_undo = FALSE;

  clay_path_freeze (private->path);

  while ((cur_stroke = clay_path_stroke_get_next (private->path,
                                                  cur_stroke)))
    {
      /* anchors */
      anchors = clay_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = CLAY_ANCHOR (list->data);

          if (cur_anchor->selected)
            {
              if (! have_undo)
                {
                  clay_tool_path_begin_change (tool_path, _("Delete Anchors"));
                  have_undo = TRUE;
                }

              clay_stroke_anchor_delete (cur_stroke, cur_anchor);

              if (clay_stroke_is_empty (cur_stroke))
                {
                  clay_path_stroke_remove (private->path, cur_stroke);
                  cur_stroke = NULL;
                }
            }
        }

      g_list_free (anchors);
    }

  if (have_undo)
    clay_tool_path_end_change (tool_path, TRUE);

  clay_path_thaw (private->path);
}


/*  public functions  */

ClayToolWidget *
clay_tool_path_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_TOOL_PATH,
                       "shell", shell,
                       NULL);
}

void
clay_tool_path_set_path (ClayToolPath *tool_path,
                         ClayPath     *path)
{
  ClayToolPathPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_PATH (tool_path));
  g_return_if_fail (path == NULL || CLAY_IS_PATH (path));

  private = tool_path->private;

  if (path == private->path)
    return;

  if (private->path)
    {
      g_signal_handlers_disconnect_by_func (private->path,
                                            clay_tool_path_path_visible,
                                            tool_path);
      g_signal_handlers_disconnect_by_func (private->path,
                                            clay_tool_path_path_freeze,
                                            tool_path);
      g_signal_handlers_disconnect_by_func (private->path,
                                            clay_tool_path_path_thaw,
                                            tool_path);

      g_object_unref (private->path);
    }

  private->path     = path;
  private->function = VECTORS_FINISHED;
  clay_tool_path_verify_state (tool_path);

  if (private->path)
    {
      g_object_ref (private->path);

      g_signal_connect_object (private->path, "visibility-changed",
                               G_CALLBACK (clay_tool_path_path_visible),
                               tool_path, 0);
      g_signal_connect_object (private->path, "freeze",
                               G_CALLBACK (clay_tool_path_path_freeze),
                               tool_path, 0);
      g_signal_connect_object (private->path, "thaw",
                               G_CALLBACK (clay_tool_path_path_thaw),
                               tool_path, 0);
    }

  g_object_notify (G_OBJECT (tool_path), "path");
}

void
clay_tool_path_get_popup_state (ClayToolPath *tool_path,
                                gboolean     *on_handle,
                                gboolean     *on_curve)
{
  ClayToolPathPrivate *private = tool_path->private;

  if (on_handle)
    *on_handle = private->cur_anchor2 == NULL;

  if (on_curve)
    *on_curve = private->cur_stroke != NULL;

}

void
clay_tool_path_delete_anchor (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;

  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  clay_path_freeze (private->path);
  clay_tool_path_begin_change (tool_path, _("Delete Anchors"));

  if (private->cur_anchor->type == CLAY_ANCHOR_ANCHOR)
    {
      clay_stroke_anchor_delete (private->cur_stroke, private->cur_anchor);
      if (clay_stroke_is_empty (private->cur_stroke))
        clay_path_stroke_remove (private->path,
                                 private->cur_stroke);
    }
  else
    {
      clay_stroke_anchor_convert (private->cur_stroke,
                                  private->cur_anchor,
                                  CLAY_ANCHOR_FEATURE_EDGE);
    }

  clay_tool_path_end_change (tool_path, TRUE);
  clay_path_thaw (private->path);
}

void
clay_tool_path_shift_start (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;

  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  clay_path_freeze (private->path);
  clay_tool_path_begin_change (tool_path, _("Shift start"));

  clay_stroke_shift_start (private->cur_stroke, private->cur_anchor);

  clay_tool_path_end_change (tool_path, TRUE);
  clay_path_thaw (private->path);
}

void
clay_tool_path_insert_anchor (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;

  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);
  g_return_if_fail (private->cur_position >= 0.0);

  clay_path_freeze (private->path);
  clay_tool_path_begin_change (tool_path, _("Insert Anchor"));

  private->cur_anchor = clay_stroke_anchor_insert (private->cur_stroke,
                                                   private->cur_anchor,
                                                   private->cur_position);

  clay_tool_path_end_change (tool_path, TRUE);
  clay_path_thaw (private->path);
}

void
clay_tool_path_delete_segment (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;
  ClayStroke          *new_stroke;

  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  clay_path_freeze (private->path);
  clay_tool_path_begin_change (tool_path, _("Delete Segment"));

  new_stroke = clay_stroke_open (private->cur_stroke,
                                 private->cur_anchor);
  if (new_stroke)
    {
      clay_path_stroke_add (private->path, new_stroke);
      g_object_unref (new_stroke);
    }

  clay_tool_path_end_change (tool_path, TRUE);
  clay_path_thaw (private->path);
}

void
clay_tool_path_reverse_stroke (ClayToolPath *tool_path)
{
  ClayToolPathPrivate *private = tool_path->private;

  g_return_if_fail (private->cur_stroke != NULL);

  clay_path_freeze (private->path);
  clay_tool_path_begin_change (tool_path, _("Insert Anchor"));

  clay_stroke_reverse (private->cur_stroke);

  clay_tool_path_end_change (tool_path, TRUE);
  clay_path_thaw (private->path);
}
