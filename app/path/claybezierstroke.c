/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybezierstroke.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include <gio/gio.h>
#include <glib-object.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "path-types.h"

#include "core/clay-transform-utils.h"
#include "core/claybezierdesc.h"
#include "core/claycoords.h"
#include "core/claycoords-interpolate.h"
#include "core/claysavable.h"
#include "core/claysavable-load.h"

#include "clayanchor.h"
#include "claybezierstroke.h"
#include "claypath.h"


/*  local prototypes  */

static void
    clay_bezier_stroke_savable_iface_init  (ClaySavableInterface  *iface);

static void
    clay_bezier_stroke_save                (ClaySavable           *savable,
                                            ClaySaveState         *state);
static gboolean
    clay_bezier_stroke_load_enter          (ClayLoadState         *state,
                                            const gchar          **attribute_names,
                                            const gchar          **attribute_values,
                                            gpointer               user_data,
                                            GError               **error);
static gboolean
    clay_bezier_stroke_load_exit           (ClayLoadState         *state,
                                            const gchar           *text,
                                            gsize                  len,
                                            gpointer               user_data,
                                            GError               **error);

static gdouble
    clay_bezier_stroke_nearest_point_get   (ClayStroke            *stroke,
                                            const ClayCoords      *coord,
                                            gdouble                precision,
                                            ClayCoords            *ret_point,
                                            ClayAnchor           **ret_segment_start,
                                            ClayAnchor           **ret_segment_end,
                                            gdouble               *ret_pos);
static gdouble
    clay_bezier_stroke_segment_nearest_point_get
                                           (const ClayCoords      *beziercoords,
                                            const ClayCoords      *coord,
                                            gdouble                precision,
                                            ClayCoords            *ret_point,
                                            gdouble               *ret_pos,
                                            gint                   depth);
static gdouble
    clay_bezier_stroke_nearest_tangent_get (ClayStroke            *stroke,
                                            const ClayCoords      *coord1,
                                            const ClayCoords      *coord2,
                                            gdouble                precision,
                                            ClayCoords            *nearest,
                                            ClayAnchor           **ret_segment_start,
                                            ClayAnchor           **ret_segment_end,
                                            gdouble               *ret_pos);
static gdouble
    clay_bezier_stroke_segment_nearest_tangent_get
                                           (const ClayCoords      *beziercoords,
                                            const ClayCoords      *coord1,
                                            const ClayCoords      *coord2,
                                            gdouble                precision,
                                            ClayCoords            *ret_point,
                                            gdouble               *ret_pos);
static void
    clay_bezier_stroke_anchor_move_relative
                                           (ClayStroke            *stroke,
                                            ClayAnchor            *anchor,
                                            const ClayCoords      *deltacoord,
                                            ClayAnchorFeatureType  feature);
static void
    clay_bezier_stroke_anchor_move_absolute
                                           (ClayStroke            *stroke,
                                            ClayAnchor            *anchor,
                                            const ClayCoords      *coord,
                                            ClayAnchorFeatureType  feature);
static void
    clay_bezier_stroke_anchor_convert      (ClayStroke            *stroke,
                                            ClayAnchor            *anchor,
                                            ClayAnchorFeatureType  feature);
static void
    clay_bezier_stroke_anchor_delete       (ClayStroke            *stroke,
                                            ClayAnchor            *anchor);
static gboolean
    clay_bezier_stroke_point_is_movable    (ClayStroke            *stroke,
                                            ClayAnchor            *predec,
                                            gdouble                position);
static void
    clay_bezier_stroke_point_move_relative (ClayStroke            *stroke,
                                            ClayAnchor            *predec,
                                            gdouble                position,
                                            const ClayCoords      *deltacoord,
                                            ClayAnchorFeatureType  feature);
static void
    clay_bezier_stroke_point_move_absolute (ClayStroke            *stroke,
                                            ClayAnchor            *predec,
                                            gdouble                position,
                                            const ClayCoords      *coord,
                                            ClayAnchorFeatureType  feature);

static void clay_bezier_stroke_close       (ClayStroke            *stroke);

static ClayStroke *
    clay_bezier_stroke_open                (ClayStroke            *stroke,
                                            ClayAnchor            *end_anchor);
static gboolean
    clay_bezier_stroke_anchor_is_insertable
                                           (ClayStroke            *stroke,
                                            ClayAnchor            *predec,
                                            gdouble                position);
static ClayAnchor *
    clay_bezier_stroke_anchor_insert       (ClayStroke            *stroke,
                                            ClayAnchor            *predec,
                                            gdouble                position);
static gboolean
    clay_bezier_stroke_is_extendable       (ClayStroke            *stroke,
                                            ClayAnchor            *neighbor);
static gboolean
    clay_bezier_stroke_connect_stroke      (ClayStroke            *stroke,
                                            ClayAnchor            *anchor,
                                            ClayStroke            *extension,
                                            ClayAnchor            *neighbor);
static gboolean
    clay_bezier_stroke_reverse             (ClayStroke            *stroke);
static gboolean
    clay_bezier_stroke_shift_start         (ClayStroke            *stroke,
                                            ClayAnchor            *anchor);
static GArray *
    clay_bezier_stroke_interpolate         (ClayStroke            *stroke,
                                            gdouble                precision,
                                            gboolean              *closed);
static ClayBezierDesc *
    clay_bezier_stroke_make_bezier         (ClayStroke            *stroke);
static void clay_bezier_stroke_transform   (ClayStroke            *stroke,
                                            const ClayMatrix3     *matrix,
                                            GQueue                *ret_strokes);

static void clay_bezier_stroke_finalize    (GObject               *object);


static GList * clay_bezier_stroke_get_anchor_listitem
                                           (GList                 *list);

static gboolean clay_bezier_stroke_exit_cp (ClayLoadState         *state,
                                            const gchar           *text,
                                            gsize                  len,
                                            gpointer               user_data,
                                            GError               **error);


G_DEFINE_TYPE_WITH_CODE (ClayBezierStroke, clay_bezier_stroke, CLAY_TYPE_STROKE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_bezier_stroke_savable_iface_init))

#define parent_class clay_bezier_stroke_parent_class


static void
clay_bezier_stroke_class_init (ClayBezierStrokeClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  ClayStrokeClass *stroke_class = CLAY_STROKE_CLASS (klass);

  object_class->finalize             = clay_bezier_stroke_finalize;

  stroke_class->nearest_point_get    = clay_bezier_stroke_nearest_point_get;
  stroke_class->nearest_tangent_get  = clay_bezier_stroke_nearest_tangent_get;
  stroke_class->nearest_intersection_get = NULL;
  stroke_class->anchor_move_relative = clay_bezier_stroke_anchor_move_relative;
  stroke_class->anchor_move_absolute = clay_bezier_stroke_anchor_move_absolute;
  stroke_class->anchor_convert       = clay_bezier_stroke_anchor_convert;
  stroke_class->anchor_delete        = clay_bezier_stroke_anchor_delete;
  stroke_class->point_is_movable     = clay_bezier_stroke_point_is_movable;
  stroke_class->point_move_relative  = clay_bezier_stroke_point_move_relative;
  stroke_class->point_move_absolute  = clay_bezier_stroke_point_move_absolute;
  stroke_class->close                = clay_bezier_stroke_close;
  stroke_class->open                 = clay_bezier_stroke_open;
  stroke_class->anchor_is_insertable = clay_bezier_stroke_anchor_is_insertable;
  stroke_class->anchor_insert        = clay_bezier_stroke_anchor_insert;
  stroke_class->is_extendable        = clay_bezier_stroke_is_extendable;
  stroke_class->extend               = clay_bezier_stroke_extend;
  stroke_class->connect_stroke       = clay_bezier_stroke_connect_stroke;
  stroke_class->reverse              = clay_bezier_stroke_reverse;
  stroke_class->shift_start          = clay_bezier_stroke_shift_start;
  stroke_class->interpolate          = clay_bezier_stroke_interpolate;
  stroke_class->make_bezier          = clay_bezier_stroke_make_bezier;
  stroke_class->transform            = clay_bezier_stroke_transform;
}

static void
clay_bezier_stroke_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "bezier-stroke";
  iface->save       = clay_bezier_stroke_save;
  iface->load_enter = clay_bezier_stroke_load_enter;
  iface->load_exit  = clay_bezier_stroke_load_exit;
}

static void
clay_bezier_stroke_init (ClayBezierStroke *stroke)
{
}

static void
clay_bezier_stroke_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_bezier_stroke_save (ClaySavable   *savable,
                         ClaySaveState *state)
{
  ClayBezierStroke *stroke = CLAY_BEZIER_STROKE (savable);
  guint32           closed;
  GArray           *control_points;

  control_points = clay_stroke_control_points_get (CLAY_STROKE (stroke),
                                                   (gint32 *) &closed);

  clay_savable_print_element_start (state, "bezier-stroke",
                                    "closed", "%b", closed,
                                    NULL);

  for (gint i = 0; i < control_points->len; i++)
    {
      ClayAnchor *anchor;

      anchor = &(g_array_index (control_points, ClayAnchor, i));

      clay_savable_print_element (state, "control-point", NULL, NULL,
                                  "type", "%s", anchor->type == CLAY_ANCHOR_ANCHOR ? "anchor" : "bezier-control-point",
                                  "x",    "%f", anchor->position.x,
                                  "y",    "%f", anchor->position.y,
                                  NULL);
    }

  clay_savable_print_element_end (state, "bezier-stroke");

  g_array_free (control_points, TRUE);
}

static gboolean
clay_bezier_stroke_load_enter (ClayLoadState  *state,
                               const gchar   **attribute_names,
                               const gchar   **attribute_values,
                               gpointer        user_data,
                               GError        **error)
{
  ClayValueArray *control_points;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "closed") == 0)
        clay_savable_load_store_from_string (state, error,
                                             "closed", G_TYPE_BOOLEAN, *attribute_values,
                                             NULL);
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  control_points = clay_value_array_new (6);
  clay_savable_load_store_value (state, "control-points", control_points, NULL);
  clay_savable_load_add_simple_handler (state, "control-point", G_TYPE_NONE,
                                        (ClayExitElementhandler) clay_bezier_stroke_exit_cp,
                                        control_points, NULL,
                                        TRUE, FALSE,
                                        "type", G_TYPE_STRING,
                                        "x",    G_TYPE_DOUBLE,
                                        "y",    G_TYPE_DOUBLE,
                                        NULL);
  return TRUE;
}

static gboolean
clay_bezier_stroke_load_exit (ClayLoadState  *state,
                              const gchar    *text,
                              gsize           len,
                              gpointer        user_data,
                              GError        **error)
{
  ClayBezierStroke *stroke;
  ClayPath         *path;
  ClayValueArray   *control_points = NULL;
  gboolean          closed         = FALSE;

  g_return_val_if_fail (CLAY_IS_PATH (clay_savable_load_peek_active_object (state)), FALSE);

  path = CLAY_PATH (clay_savable_load_peek_active_object (state));

  /* We know that "control-points" exists. As for "closed", it may not
   * exist, which will mean it's FALSE.
   */
  clay_savable_load_get_values (state,
                                "control-points", &control_points,
                                "closed",         &closed,
                                NULL);
  stroke = g_object_new (CLAY_TYPE_BEZIER_STROKE,
                         "closed",         closed,
                         "control-points", control_points,
                         NULL);
  clay_path_stroke_add (path, CLAY_STROKE (stroke));

  clay_value_array_unref (control_points);
  g_object_unref (stroke);

  return TRUE;
}


/* Bezier specific functions */

ClayStroke *
clay_bezier_stroke_new (void)
{
  return g_object_new (CLAY_TYPE_BEZIER_STROKE, NULL);
}


ClayStroke *
clay_bezier_stroke_new_from_coords (const ClayCoords *coords,
                                    gint              n_coords,
                                    gboolean          closed)
{
  ClayStroke *stroke;
  ClayAnchor *last_anchor;
  gint        count;

  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (n_coords >= 3, NULL);
  g_return_val_if_fail ((n_coords % 3) == 0, NULL);

  stroke = clay_bezier_stroke_new ();

  last_anchor = NULL;

  for (count = 0; count < n_coords; count++)
    last_anchor = clay_bezier_stroke_extend (stroke,
                                             &coords[count],
                                             last_anchor,
                                             CLAY_STROKE_EXTEND_SIMPLE);

  if (closed)
    clay_stroke_close (stroke);

  return stroke;
}

/* Returns the EndPoint (ANCHOR) that a given Handle (CONTROL) controls.
 *
 * ClayAnchors are in a list.
 * ClayAnchors of type ANCHOR are surrounded by two ClayAnchors
 * of type CONTROL (Handle) in the pattern HEH HEH....
 * E denotes a ClayAnchor of type ANCHOR,  aka EndPoint.
 * H denotes a ClayAnchor of type CONTROL, aka Handle.
 */
static ClayAnchor*
clay_bezier_stroke_find_endpoint_for_handle (ClayStroke *stroke,
                                             ClayAnchor *anchor)
{
  GList *anchor_element;
  GList *prior_anchor_element;
  GList *result_element;

  /* Require given anchor is CONTROL i.e. Handle. */
  g_return_val_if_fail (anchor->type == CLAY_ANCHOR_CONTROL, NULL);

  anchor_element = g_queue_find (stroke->anchors, anchor);
  prior_anchor_element  = g_list_previous (anchor_element);

  if (prior_anchor_element == NULL)
    {
      /* No prior element. Given handle is very first ClayAnchor in the list: thisH E...
       * The desired ANCHOR endpoint trails it.
       */
      result_element = g_list_next (anchor_element);
    }
  else if (((ClayAnchor *)prior_anchor_element->data)->type == CLAY_ANCHOR_ANCHOR)
    {
      /* Prior element is-a ANCHOR i.e. EndPoint.
       * Given handle is the trailing handle for the desired endpoint:   ...E thisH ...
       */
      result_element = prior_anchor_element;
    }
  else
    {
      /* Prior element is another CONTROL i.e. handle.
       * Given handle is leading the desired endpoint  ...H thisH E...
       */
      result_element = g_list_next (anchor_element);
    }

  /* Ensure the list of anchors is not corrupt by missing next anchors. */
  g_return_val_if_fail (result_element != NULL, NULL);
  /* Ensure result_element type is ANCHOR i.e. EndPoint. */
  g_return_val_if_fail (((ClayAnchor*)result_element->data)->type == CLAY_ANCHOR_ANCHOR, NULL);

  return result_element->data;
}

static void
clay_bezier_stroke_anchor_delete (ClayStroke *stroke,
                                  ClayAnchor *anchor)
{
  GList *list;
  GList *list2;
  gint   i;

  /* Anchors always are surrounded by two handles that have to
   * be deleted too
   */

  list2 = g_queue_find (stroke->anchors, anchor);
  list = g_list_previous (list2);

  for (i = 0; i < 3; i++)
    {
      g_return_if_fail (list != NULL);

      list2 = g_list_next (list);
      clay_anchor_free (list->data);
      g_queue_delete_link (stroke->anchors, list);
      list = list2;
    }
}

static ClayStroke *
clay_bezier_stroke_open (ClayStroke *stroke,
                         ClayAnchor *end_anchor)
{
  GList      *list;
  GList      *list2;
  ClayStroke *new_stroke = NULL;

  list = g_queue_find (stroke->anchors, end_anchor);

  g_return_val_if_fail (list != NULL && list->next != NULL, NULL);

  list = g_list_next (list);  /* protect the handle... */

  list2 = list->next;
  list->next = NULL;

  if (list2 != NULL)
    {
      GList *tail = stroke->anchors->tail;

      stroke->anchors->tail = list;
      stroke->anchors->length -= g_list_length (list2);

      list2->prev = NULL;

      if (stroke->closed)
        {
          GList *l;

          for (l = tail; l; l = g_list_previous (l))
            g_queue_push_head (stroke->anchors, l->data);

          g_list_free (list2);
        }
      else
        {
          new_stroke = clay_bezier_stroke_new ();
          new_stroke->anchors->head = list2;
          new_stroke->anchors->tail = g_list_last (list2);
          new_stroke->anchors->length = g_list_length (list2);
        }
    }

  stroke->closed = FALSE;
  g_object_notify (G_OBJECT (stroke), "closed");

  return new_stroke;
}

static gboolean
clay_bezier_stroke_anchor_is_insertable (ClayStroke *stroke,
                                         ClayAnchor *predec,
                                         gdouble     position)
{
  return (g_queue_find (stroke->anchors, predec) != NULL);
}


static ClayAnchor *
clay_bezier_stroke_anchor_insert (ClayStroke *stroke,
                                  ClayAnchor *predec,
                                  gdouble     position)
{
  GList      *segment_start;
  GList      *list;
  GList      *list2;
  ClayCoords  subdivided[8];
  ClayCoords  beziercoords[4];
  gint        i;

  segment_start = g_queue_find (stroke->anchors, predec);

  if (! segment_start)
    return NULL;

  list = segment_start;

  for (i = 0; i <= 3; i++)
    {
      beziercoords[i] = CLAY_ANCHOR (list->data)->position;
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  clay_coords_mix (1-position, &(beziercoords[0]),
                   position,   &(beziercoords[1]),
                   &(subdivided[1]));

  clay_coords_mix (1-position, &(beziercoords[1]),
                   position,   &(beziercoords[2]),
                   &(subdivided[7]));

  clay_coords_mix (1-position, &(beziercoords[2]),
                   position,   &(beziercoords[3]),
                   &(subdivided[5]));

  clay_coords_mix (1-position, &(subdivided[1]),
                   position,   &(subdivided[7]),
                   &(subdivided[2]));

  clay_coords_mix (1-position, &(subdivided[7]),
                   position,   &(subdivided[5]),
                   &(subdivided[4]));

  clay_coords_mix (1-position, &(subdivided[2]),
                   position,   &(subdivided[4]),
                   &(subdivided[3]));

  /* subdivided 0-6 contains the bezier segment subdivided at <position> */

  list = segment_start;

  for (i = 0; i <= 6; i++)
    {
      if (i >= 2 && i <= 4)
        {
          list2 = g_list_append (NULL,
                                 clay_anchor_new ((i == 3 ?
                                                    CLAY_ANCHOR_ANCHOR:
                                                    CLAY_ANCHOR_CONTROL),
                                                  &(subdivided[i])));
          /* insert it *before* list manually. */
          list2->next = list;
          list2->prev = list->prev;
          if (list->prev)
            list->prev->next = list2;
          list->prev = list2;

          list = list2;

          if (i == 3)
            segment_start = list;
        }
      else
        {
          CLAY_ANCHOR (list->data)->position = subdivided[i];
        }

      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  stroke->anchors->head = g_list_first (list);
  stroke->anchors->tail = g_list_last (list);
  stroke->anchors->length += 3;

  return CLAY_ANCHOR (segment_start->data);
}


static gboolean
clay_bezier_stroke_point_is_movable (ClayStroke *stroke,
                                     ClayAnchor *predec,
                                     gdouble     position)
{
  return (g_queue_find (stroke->anchors, predec) != NULL);
}


static void
clay_bezier_stroke_point_move_relative (ClayStroke            *stroke,
                                        ClayAnchor            *predec,
                                        gdouble                position,
                                        const ClayCoords      *deltacoord,
                                        ClayAnchorFeatureType  feature)
{
  ClayCoords  offsetcoords[2];
  GList      *segment_start;
  GList      *list;
  gint        i;
  gdouble     feel_good;

  segment_start = g_queue_find (stroke->anchors, predec);

  g_return_if_fail (segment_start != NULL);

  /* dragging close to endpoints just moves the handle related to
   * the endpoint. Just make sure that feel_good is in the range from
   * 0 to 1. The 1.0 / 6.0 and 5.0 / 6.0 are duplicated in
   * tools/claypathtool.c.
   */
  if (position <= 1.0 / 6.0)
    feel_good = 0;
  else if (position <= 0.5)
    feel_good = (pow((6 * position - 1) / 2.0, 3)) / 2;
  else if (position <= 5.0 / 6.0)
    feel_good = (1 - pow((6 * (1-position) - 1) / 2.0, 3)) / 2 + 0.5;
  else
    feel_good = 1;

  clay_coords_scale ((1-feel_good)/(3*position*
                                    (1-position)*(1-position)),
                     deltacoord,
                     &(offsetcoords[0]));
  clay_coords_scale (feel_good/(3*position*position*(1-position)),
                     deltacoord,
                     &(offsetcoords[1]));

  list = segment_start;
  list = g_list_next (list);
  if (! list)
    list = stroke->anchors->head;

  for (i = 0; i <= 1; i++)
    {
      clay_stroke_anchor_move_relative (stroke, CLAY_ANCHOR (list->data),
                                        &(offsetcoords[i]), feature);
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }
}


static void
clay_bezier_stroke_point_move_absolute (ClayStroke            *stroke,
                                        ClayAnchor            *predec,
                                        gdouble                position,
                                        const ClayCoords      *coord,
                                        ClayAnchorFeatureType  feature)
{
  ClayCoords  deltacoord;
  ClayCoords  tmp1, tmp2, abs_pos;
  ClayCoords  beziercoords[4];
  GList      *segment_start;
  GList      *list;
  gint        i;

  segment_start = g_queue_find (stroke->anchors, predec);

  g_return_if_fail (segment_start != NULL);

  list = segment_start;

  for (i = 0; i <= 3; i++)
    {
      beziercoords[i] = CLAY_ANCHOR (list->data)->position;
      list = g_list_next (list);
      if (! list)
        list = stroke->anchors->head;
    }

  clay_coords_mix ((1-position)*(1-position)*(1-position), &(beziercoords[0]),
                   3*(1-position)*(1-position)*position, &(beziercoords[1]),
                   &tmp1);
  clay_coords_mix (3*(1-position)*position*position, &(beziercoords[2]),
                   position*position*position, &(beziercoords[3]),
                   &tmp2);
  clay_coords_add (&tmp1, &tmp2, &abs_pos);

  clay_coords_difference (coord, &abs_pos, &deltacoord);

  clay_bezier_stroke_point_move_relative (stroke, predec, position,
                                          &deltacoord, feature);
}

static void
clay_bezier_stroke_close (ClayStroke *stroke)
{
  GList      *start;
  GList      *end;
  ClayAnchor *anchor;

  start = stroke->anchors->head;
  end   = stroke->anchors->tail;

  g_return_if_fail (start->next != NULL && end->prev != NULL);

  if (start->next != end->prev)
    {
      if (clay_coords_equal (&(CLAY_ANCHOR (start->next->data)->position),
                             &(CLAY_ANCHOR (start->data)->position)) &&
          clay_coords_equal (&(CLAY_ANCHOR (start->data)->position),
                             &(CLAY_ANCHOR (end->data)->position)) &&
          clay_coords_equal (&(CLAY_ANCHOR (end->data)->position),
                             &(CLAY_ANCHOR (end->prev->data)->position)))
        {
          /* redundant segment */

          clay_anchor_free (stroke->anchors->tail->data);
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          clay_anchor_free (stroke->anchors->tail->data);
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          anchor = stroke->anchors->tail->data;
          g_queue_delete_link (stroke->anchors, stroke->anchors->tail);

          clay_anchor_free (stroke->anchors->head->data);
          stroke->anchors->head->data = anchor;
        }
    }

  CLAY_STROKE_CLASS (parent_class)->close (stroke);
}

static gdouble
clay_bezier_stroke_nearest_point_get (ClayStroke        *stroke,
                                      const ClayCoords  *coord,
                                      gdouble            precision,
                                      ClayCoords        *ret_point,
                                      ClayAnchor       **ret_segment_start,
                                      ClayAnchor       **ret_segment_end,
                                      gdouble           *ret_pos)
{
  gdouble     min_dist, dist, pos;
  ClayCoords  point = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  ClayCoords  segmentcoords[4];
  GList      *anchorlist;
  ClayAnchor *segment_start;
  ClayAnchor *segment_end = NULL;
  ClayAnchor *anchor;
  gint        count;

  if (g_queue_is_empty (stroke->anchors))
    return -1.0;

  count = 0;
  min_dist = -1;
  pos = 0;

  for (anchorlist = stroke->anchors->head;
       CLAY_ANCHOR (anchorlist->data)->type != CLAY_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  segment_start = anchorlist->data;

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          segment_end = anchorlist->data;
          dist = clay_bezier_stroke_segment_nearest_point_get (segmentcoords,
                                                               coord, precision,
                                                               &point, &pos,
                                                               10);

          if (dist < min_dist || min_dist < 0)
            {
              min_dist = dist;

              if (ret_pos)
                *ret_pos = pos;
              if (ret_point)
                *ret_point = point;
              if (ret_segment_start)
                *ret_segment_start = segment_start;
              if (ret_segment_end)
                *ret_segment_end = segment_end;
            }

          segment_start = anchorlist->data;
          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  if (stroke->closed && stroke->anchors->head)
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = CLAY_ANCHOR (anchorlist->data)->position;
          count++;
        }

      anchorlist = g_list_next (anchorlist);

      if (anchorlist)
        {
          segment_end = CLAY_ANCHOR (anchorlist->data);
          segmentcoords[3] = segment_end->position;
        }

      dist = clay_bezier_stroke_segment_nearest_point_get (segmentcoords,
                                                           coord, precision,
                                                           &point, &pos,
                                                           10);

      if (dist < min_dist || min_dist < 0)
        {
          min_dist = dist;

          if (ret_pos)
            *ret_pos = pos;
          if (ret_point)
            *ret_point = point;
          if (ret_segment_start)
            *ret_segment_start = segment_start;
          if (ret_segment_end)
            *ret_segment_end = segment_end;
        }
    }

  return min_dist;
}


static gdouble
clay_bezier_stroke_segment_nearest_point_get (const ClayCoords *beziercoords,
                                              const ClayCoords *coord,
                                              gdouble           precision,
                                              ClayCoords       *ret_point,
                                              gdouble          *ret_pos,
                                              gint              depth)
{
  /*
   * beziercoords has to contain four ClayCoords with the four control points
   * of the bezier segment. We subdivide it at the parameter 0.5.
   */

  ClayCoords subdivided[8];
  gdouble    dist1, dist2;
  ClayCoords point1, point2;
  gdouble    pos1, pos2;

  clay_coords_difference (&beziercoords[1], &beziercoords[0], &point1);
  clay_coords_difference (&beziercoords[3], &beziercoords[2], &point2);

  if (! depth || (clay_coords_bezier_is_straight (beziercoords, precision) &&
                  clay_coords_length_squared (&point1) < precision         &&
                  clay_coords_length_squared (&point2) < precision))
    {
      ClayCoords line, dcoord;
      gdouble    length2, scalar;
      gint       i;

      clay_coords_difference (&(beziercoords[3]),
                              &(beziercoords[0]),
                              &line);

      clay_coords_difference (coord,
                              &(beziercoords[0]),
                              &dcoord);

      length2 = clay_coords_scalarprod (&line, &line);
      scalar = clay_coords_scalarprod (&line, &dcoord) / length2;

      scalar = CLAMP (scalar, 0.0, 1.0);

      /* lines look the same as bezier curves where the handles
       * sit on the anchors, however, they are parametrized
       * differently. Hence we have to do some weird approximation.  */

      pos1 = pos2 = 0.5;

      for (i = 0; i <= 15; i++)
        {
          pos2 *= 0.5;

          if (3 * pos1 * pos1 * (1-pos1) + pos1 * pos1 * pos1 < scalar)
            pos1 += pos2;
          else
            pos1 -= pos2;
        }

      *ret_pos = pos1;

      clay_coords_mix (1.0, &(beziercoords[0]),
                       scalar, &line,
                       ret_point);

      clay_coords_difference (coord, ret_point, &dcoord);

      return clay_coords_length (&dcoord);
    }

  /* ok, we have to subdivide */

  subdivided[0] = beziercoords[0];
  subdivided[6] = beziercoords[3];

  /* if (!depth) g_printerr ("Hit recursion depth limit!\n"); */

  clay_coords_average (&(beziercoords[0]), &(beziercoords[1]),
                       &(subdivided[1]));

  clay_coords_average (&(beziercoords[1]), &(beziercoords[2]),
                       &(subdivided[7]));

  clay_coords_average (&(beziercoords[2]), &(beziercoords[3]),
                       &(subdivided[5]));

  clay_coords_average (&(subdivided[1]), &(subdivided[7]),
                       &(subdivided[2]));

  clay_coords_average (&(subdivided[7]), &(subdivided[5]),
                       &(subdivided[4]));

  clay_coords_average (&(subdivided[2]), &(subdivided[4]),
                       &(subdivided[3]));

  /*
   * We now have the coordinates of the two bezier segments in
   * subdivided [0-3] and subdivided [3-6]
   */

  dist1 = clay_bezier_stroke_segment_nearest_point_get (&(subdivided[0]),
                                                        coord, precision,
                                                        &point1, &pos1,
                                                        depth - 1);

  dist2 = clay_bezier_stroke_segment_nearest_point_get (&(subdivided[3]),
                                                        coord, precision,
                                                        &point2, &pos2,
                                                        depth - 1);

  if (dist1 <= dist2)
    {
      *ret_point = point1;
      *ret_pos = 0.5 * pos1;
      return dist1;
    }
  else
    {
      *ret_point = point2;
      *ret_pos = 0.5 + 0.5 * pos2;
      return dist2;
    }
}


static gdouble
clay_bezier_stroke_nearest_tangent_get (ClayStroke        *stroke,
                                        const ClayCoords  *coord1,
                                        const ClayCoords  *coord2,
                                        gdouble            precision,
                                        ClayCoords        *nearest,
                                        ClayAnchor       **ret_segment_start,
                                        ClayAnchor       **ret_segment_end,
                                        gdouble           *ret_pos)
{
  gdouble     min_dist, dist, pos;
  ClayCoords  point;
  ClayCoords  segmentcoords[4];
  GList      *anchorlist;
  ClayAnchor *segment_start;
  ClayAnchor *segment_end = NULL;
  ClayAnchor *anchor;
  gint        count;

  if (g_queue_is_empty (stroke->anchors))
    return -1.0;

  count = 0;
  min_dist = -1;

  for (anchorlist = stroke->anchors->head;
       CLAY_ANCHOR (anchorlist->data)->type != CLAY_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  segment_start = anchorlist->data;

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          segment_end = anchorlist->data;
          dist = clay_bezier_stroke_segment_nearest_tangent_get (segmentcoords,
                                                                 coord1, coord2,
                                                                 precision,
                                                                 &point, &pos);

          if (dist >= 0 && (dist < min_dist || min_dist < 0))
            {
              min_dist = dist;

              if (ret_pos)
                *ret_pos = pos;
              if (nearest)
                *nearest = point;
              if (ret_segment_start)
                *ret_segment_start = segment_start;
              if (ret_segment_end)
                *ret_segment_end = segment_end;
            }

          segment_start = anchorlist->data;
          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  if (stroke->closed && ! g_queue_is_empty (stroke->anchors))
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = CLAY_ANCHOR (anchorlist->data)->position;
          count++;
        }

      anchorlist = g_list_next (anchorlist);

      if (anchorlist)
        {
          segment_end = CLAY_ANCHOR (anchorlist->data);
          segmentcoords[3] = segment_end->position;
        }

      dist = clay_bezier_stroke_segment_nearest_tangent_get (segmentcoords,
                                                             coord1, coord2,
                                                             precision,
                                                             &point, &pos);

      if (dist >= 0 && (dist < min_dist || min_dist < 0))
        {
          min_dist = dist;

          if (ret_pos)
            *ret_pos = pos;
          if (nearest)
            *nearest = point;
          if (ret_segment_start)
            *ret_segment_start = segment_start;
          if (ret_segment_end)
            *ret_segment_end = segment_end;
        }
    }

  return min_dist;
}

static gdouble
clay_bezier_stroke_segment_nearest_tangent_get (const ClayCoords *beziercoords,
                                                const ClayCoords *coord1,
                                                const ClayCoords *coord2,
                                                gdouble           precision,
                                                ClayCoords       *ret_point,
                                                gdouble          *ret_pos)
{
  GArray     *ret_coords;
  GArray     *ret_params;
  ClayCoords  dir, line, dcoord, min_point;
  gdouble     min_dist = -1;
  gdouble     dist, length2, scalar, ori, ori2;
  gint        i;

  clay_coords_difference (coord2, coord1, &line);

  ret_coords = g_array_new (FALSE, FALSE, sizeof (ClayCoords));
  ret_params = g_array_new (FALSE, FALSE, sizeof (gdouble));

  clay_coords_interpolate_bezier (beziercoords, precision,
                                  ret_coords, ret_params);

  g_return_val_if_fail (ret_coords->len == ret_params->len, -1.0);

  if (ret_coords->len < 2)
    return -1;

  clay_coords_difference (&g_array_index (ret_coords, ClayCoords, 1),
                          &g_array_index (ret_coords, ClayCoords, 0),
                          &dir);
  ori = dir.x * line.y - dir.y * line.x;

  for (i = 2; i < ret_coords->len; i++)
    {
      clay_coords_difference (&g_array_index (ret_coords, ClayCoords, i),
                              &g_array_index (ret_coords, ClayCoords, i-1),
                              &dir);
      ori2 = dir.x * line.y - dir.y * line.x;

      if (ori * ori2 <= 0)
        {
          clay_coords_difference (&g_array_index (ret_coords, ClayCoords, i),
                                  coord1,
                                  &dcoord);

          length2 = clay_coords_scalarprod (&line, &line);
          scalar = clay_coords_scalarprod (&line, &dcoord) / length2;

          if (scalar >= 0 && scalar <= 1)
            {
              clay_coords_mix (1.0, coord1,
                               scalar, &line,
                               &min_point);
              clay_coords_difference (&min_point,
                                      &g_array_index (ret_coords, ClayCoords, i),
                                      &dcoord);
              dist = clay_coords_length (&dcoord);

              if (dist < min_dist || min_dist < 0)
                {
                  min_dist   = dist;
                  *ret_point = g_array_index (ret_coords, ClayCoords, i);
                  *ret_pos   = g_array_index (ret_params, gdouble, i);
                }
            }
        }
      ori = ori2;
    }

  g_array_free (ret_coords, TRUE);
  g_array_free (ret_params, TRUE);

  return min_dist;
}

static gboolean
clay_bezier_stroke_is_extendable (ClayStroke *stroke,
                                  ClayAnchor *neighbor)
{
  GList *listneighbor;
  gint   loose_end;

  if (stroke->closed)
    return FALSE;

  if (g_queue_is_empty (stroke->anchors))
    return TRUE;

  /* assure that there is a neighbor specified */
  g_return_val_if_fail (neighbor != NULL, FALSE);

  loose_end = 0;
  listneighbor = stroke->anchors->tail;

  /* Check if the neighbor is at an end of the control points */
  if (listneighbor->data == neighbor)
    {
      loose_end = 1;
    }
  else
    {
      listneighbor = g_list_first (stroke->anchors->head);

      if (listneighbor->data == neighbor)
        {
          loose_end = -1;
        }
      else
        {
          /*
           * It isn't. If we are on a handle go to the nearest
           * anchor and see if we can find an end from it.
           * Yes, this is tedious.
           */

          listneighbor = g_queue_find (stroke->anchors, neighbor);

          if (listneighbor && neighbor->type == CLAY_ANCHOR_CONTROL)
            {
              if (listneighbor->prev &&
                  CLAY_ANCHOR (listneighbor->prev->data)->type == CLAY_ANCHOR_ANCHOR)
                {
                  listneighbor = listneighbor->prev;
                }
              else if (listneighbor->next &&
                       CLAY_ANCHOR (listneighbor->next->data)->type == CLAY_ANCHOR_ANCHOR)
                {
                  listneighbor = listneighbor->next;
                }
              else
                {
                  loose_end = 0;
                  listneighbor = NULL;
                }
            }

          if (listneighbor)
            /* we found a suitable ANCHOR_ANCHOR now, lets
             * search for its loose end.
             */
            {
              if (listneighbor->prev &&
                  listneighbor->prev->prev == NULL)
                {
                  loose_end = -1;
                }
              else if (listneighbor->next &&
                       listneighbor->next->next == NULL)
                {
                  loose_end = 1;
                }
            }
        }
    }

  return (loose_end != 0);
}

ClayAnchor *
clay_bezier_stroke_extend (ClayStroke           *stroke,
                           const ClayCoords     *coords,
                           ClayAnchor           *neighbor,
                           ClayStrokeExtendMode  extend_mode)
{
  ClayAnchor *anchor = NULL;
  GList      *listneighbor;
  gint        loose_end, control_count;

  if (g_queue_is_empty (stroke->anchors))
    {
      /* assure that there is no neighbor specified */
      g_return_val_if_fail (neighbor == NULL, NULL);

      anchor = clay_anchor_new (CLAY_ANCHOR_CONTROL, coords);

      g_queue_push_tail (stroke->anchors, anchor);

      switch (extend_mode)
        {
        case CLAY_STROKE_EXTEND_SIMPLE:
          break;

        case CLAY_STROKE_EXTEND_EDITABLE:
          anchor = clay_bezier_stroke_extend (stroke,
                                              coords, anchor,
                                              CLAY_STROKE_EXTEND_SIMPLE);

          /* we return the CLAY_ANCHOR_ANCHOR */
          clay_bezier_stroke_extend (stroke,
                                     coords, anchor,
                                     CLAY_STROKE_EXTEND_SIMPLE);

          break;

        default:
          anchor = NULL;
        }

      return anchor;
    }
  else
    {
      /* assure that there is a neighbor specified */
      g_return_val_if_fail (neighbor != NULL, NULL);

      loose_end = 0;
      listneighbor = stroke->anchors->tail;

      /* Check if the neighbor is at an end of the control points */
      if (listneighbor->data == neighbor)
        {
          loose_end = 1;
        }
      else
        {
          listneighbor = stroke->anchors->head;

          if (listneighbor->data == neighbor)
            {
              loose_end = -1;
            }
          else
            {
              /*
               * It isn't. If we are on a handle go to the nearest
               * anchor and see if we can find an end from it.
               * Yes, this is tedious.
               */

              listneighbor = g_queue_find (stroke->anchors, neighbor);

              if (listneighbor && neighbor->type == CLAY_ANCHOR_CONTROL)
                {
                  if (listneighbor->prev &&
                      CLAY_ANCHOR (listneighbor->prev->data)->type == CLAY_ANCHOR_ANCHOR)
                    {
                      listneighbor = listneighbor->prev;
                    }
                  else if (listneighbor->next &&
                           CLAY_ANCHOR (listneighbor->next->data)->type == CLAY_ANCHOR_ANCHOR)
                    {
                      listneighbor = listneighbor->next;
                    }
                  else
                    {
                      loose_end = 0;
                      listneighbor = NULL;
                    }
                }

              if (listneighbor)
                /* we found a suitable ANCHOR_ANCHOR now, lets
                 * search for its loose end.
                 */
                {
                  if (listneighbor->next &&
                      listneighbor->next->next == NULL)
                    {
                      loose_end = 1;
                      listneighbor = listneighbor->next;
                    }
                  else if (listneighbor->prev &&
                      listneighbor->prev->prev == NULL)
                    {
                      loose_end = -1;
                      listneighbor = listneighbor->prev;
                    }
                }
            }
        }

      if (loose_end)
        {
          ClayAnchorType  type;

          /* We have to detect the type of the point to add... */

          control_count = 0;

          if (loose_end == 1)
            {
              while (listneighbor &&
                     CLAY_ANCHOR (listneighbor->data)->type == CLAY_ANCHOR_CONTROL)
                {
                  control_count++;
                  listneighbor = listneighbor->prev;
                }
            }
          else
            {
              while (listneighbor &&
                     CLAY_ANCHOR (listneighbor->data)->type == CLAY_ANCHOR_CONTROL)
                {
                  control_count++;
                  listneighbor = listneighbor->next;
                }
            }

          switch (extend_mode)
            {
            case CLAY_STROKE_EXTEND_SIMPLE:
              switch (control_count)
                {
                case 0:
                  type = CLAY_ANCHOR_CONTROL;
                  break;
                case 1:
                  if (listneighbor)  /* only one handle in the path? */
                    type = CLAY_ANCHOR_CONTROL;
                  else
                    type = CLAY_ANCHOR_ANCHOR;
                  break;
                case 2:
                  type = CLAY_ANCHOR_ANCHOR;
                  break;
                default:
                  g_warning ("inconsistent bezier curve: "
                             "%d successive control handles", control_count);
                  type = CLAY_ANCHOR_ANCHOR;
                }

              anchor = clay_anchor_new (type, coords);

              if (loose_end == 1)
                g_queue_push_tail (stroke->anchors, anchor);

              if (loose_end == -1)
                g_queue_push_head (stroke->anchors, anchor);
              break;

            case CLAY_STROKE_EXTEND_EDITABLE:
              switch (control_count)
                {
                case 0:
                  neighbor = clay_bezier_stroke_extend (stroke,
                                                        &(neighbor->position),
                                                        neighbor,
                                                        CLAY_STROKE_EXTEND_SIMPLE);
                case 1:
                  neighbor = clay_bezier_stroke_extend (stroke,
                                                        coords,
                                                        neighbor,
                                                        CLAY_STROKE_EXTEND_SIMPLE);
                case 2:
                  anchor = clay_bezier_stroke_extend (stroke,
                                                      coords,
                                                      neighbor,
                                                      CLAY_STROKE_EXTEND_SIMPLE);

                  neighbor = clay_bezier_stroke_extend (stroke,
                                                        coords,
                                                        anchor,
                                                        CLAY_STROKE_EXTEND_SIMPLE);
                  break;
                default:
                  g_warning ("inconsistent bezier curve: "
                             "%d successive control handles", control_count);
                }
            }

          return anchor;
        }

      return NULL;
    }
}

static gboolean
clay_bezier_stroke_connect_stroke (ClayStroke *stroke,
                                   ClayAnchor *anchor,
                                   ClayStroke *extension,
                                   ClayAnchor *neighbor)
{
  GList *list1;
  GList *list2;

  list1 = g_queue_find (stroke->anchors, anchor);
  list1 = clay_bezier_stroke_get_anchor_listitem (list1);
  list2 = g_queue_find (extension->anchors, neighbor);
  list2 = clay_bezier_stroke_get_anchor_listitem (list2);

  g_return_val_if_fail (list1 != NULL && list2 != NULL, FALSE);

  if (stroke == extension)
    {
      g_return_val_if_fail ((list1->prev && list1->prev->prev == NULL &&
                             list2->next && list2->next->next == NULL) ||
                            (list1->next && list1->next->next == NULL &&
                             list2->prev && list2->prev->prev == NULL), FALSE);
      clay_stroke_close (stroke);
      return TRUE;
    }

  if (list1->prev && list1->prev->prev == NULL)
    {
      g_queue_reverse (stroke->anchors);
    }

  g_return_val_if_fail (list1->next && list1->next->next == NULL, FALSE);

  if (list2->next && list2->next->next == NULL)
    {
      g_queue_reverse (extension->anchors);
    }

  g_return_val_if_fail (list2->prev && list2->prev->prev == NULL, FALSE);

  for (list1 = extension->anchors->head; list1; list1 = g_list_next (list1))
    g_queue_push_tail (stroke->anchors, list1->data);

  g_queue_clear (extension->anchors);

  return TRUE;
}


static gboolean
clay_bezier_stroke_reverse (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_BEZIER_STROKE (stroke), FALSE);

  g_queue_reverse (stroke->anchors);

  /* keep the first nodegroup the same for closed strokes */
  if (stroke->closed && stroke->anchors->length >= 3)
    {
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
      g_queue_push_head_link (stroke->anchors,
                              g_queue_pop_tail_link (stroke->anchors));
    }

  return TRUE;
}


static gboolean
clay_bezier_stroke_shift_start (ClayStroke *stroke,
                                ClayAnchor *new_start)
{
  GList *link;
  ClayAnchor *real_new_start = NULL;

  g_return_val_if_fail (CLAY_IS_BEZIER_STROKE (stroke), FALSE);
  g_return_val_if_fail (new_start != NULL, FALSE);

  /* Action "shift-start" is enabled for either Handle (CONTROL) or EndPoint (ANCHOR)
   * but operates only on an EndPoint.
   * When user chose a Handle, get the Handle's EndPoint to operate on.
   */
  if (new_start->type == CLAY_ANCHOR_CONTROL)
    {
      real_new_start = clay_bezier_stroke_find_endpoint_for_handle (stroke, new_start);
    }
  else if (new_start->type == CLAY_ANCHOR_ANCHOR)
    {
      real_new_start = new_start;
    }
  /* Else type is corrupt and real_new_start is NULL. */

  g_return_val_if_fail (real_new_start != NULL, FALSE);
  g_return_val_if_fail (real_new_start->type == CLAY_ANCHOR_ANCHOR, FALSE);

  link = g_queue_find (stroke->anchors, real_new_start);
  if (!link)
    return FALSE;

  /* the preceding control anchor will be the new head */

  link = g_list_previous (link);
  if (!link)
    return FALSE;

  if (link == stroke->anchors->head)
    return TRUE;

  stroke->anchors->tail->next = stroke->anchors->head;
  stroke->anchors->head->prev = stroke->anchors->tail;
  stroke->anchors->tail = link->prev;
  stroke->anchors->head = link;
  stroke->anchors->tail->next = NULL;
  stroke->anchors->head->prev = NULL;

  return TRUE;

}


static void
clay_bezier_stroke_anchor_move_relative (ClayStroke            *stroke,
                                         ClayAnchor            *anchor,
                                         const ClayCoords      *deltacoord,
                                         ClayAnchorFeatureType  feature)
{
  ClayCoords  delta, coord1, coord2;
  GList      *anchor_list;

  delta = *deltacoord;
  delta.pressure = 0;
  delta.xtilt    = 0;
  delta.ytilt    = 0;
  delta.wheel    = 0;

  clay_coords_add (&(anchor->position), &delta, &coord1);
  anchor->position = coord1;

  anchor_list = g_queue_find (stroke->anchors, anchor);
  g_return_if_fail (anchor_list != NULL);

  if (anchor->type == CLAY_ANCHOR_ANCHOR)
    {
      if (g_list_previous (anchor_list))
        {
          coord2 = CLAY_ANCHOR (g_list_previous (anchor_list)->data)->position;
          clay_coords_add (&coord2, &delta, &coord1);
          CLAY_ANCHOR (g_list_previous (anchor_list)->data)->position = coord1;
        }

      if (g_list_next (anchor_list))
        {
          coord2 = CLAY_ANCHOR (g_list_next (anchor_list)->data)->position;
          clay_coords_add (&coord2, &delta, &coord1);
          CLAY_ANCHOR (g_list_next (anchor_list)->data)->position = coord1;
        }
    }
  else
    {
      if (feature == CLAY_ANCHOR_FEATURE_SYMMETRIC)
        {
          GList *neighbour = NULL, *opposite = NULL;

          /* search for opposite control point. Sigh. */
          neighbour = g_list_previous (anchor_list);
          if (neighbour &&
              CLAY_ANCHOR (neighbour->data)->type == CLAY_ANCHOR_ANCHOR)
            {
              opposite = g_list_previous (neighbour);
            }
          else
            {
              neighbour = g_list_next (anchor_list);
              if (neighbour &&
                  CLAY_ANCHOR (neighbour->data)->type == CLAY_ANCHOR_ANCHOR)
                {
                  opposite = g_list_next (neighbour);
                }
            }
          if (opposite &&
              CLAY_ANCHOR (opposite->data)->type == CLAY_ANCHOR_CONTROL)
            {
              clay_coords_difference (&(CLAY_ANCHOR (neighbour->data)->position),
                                      &(anchor->position), &delta);
              clay_coords_add (&(CLAY_ANCHOR (neighbour->data)->position),
                               &delta, &coord1);
              CLAY_ANCHOR (opposite->data)->position = coord1;
            }
        }
    }
}


static void
clay_bezier_stroke_anchor_move_absolute (ClayStroke            *stroke,
                                         ClayAnchor            *anchor,
                                         const ClayCoords      *coord,
                                         ClayAnchorFeatureType  feature)
{
  ClayCoords deltacoord;

  clay_coords_difference (coord, &anchor->position, &deltacoord);
  clay_bezier_stroke_anchor_move_relative (stroke, anchor,
                                           &deltacoord, feature);
}

static void
clay_bezier_stroke_anchor_convert (ClayStroke            *stroke,
                                   ClayAnchor            *anchor,
                                   ClayAnchorFeatureType  feature)
{
  GList *anchor_list;

  anchor_list = g_queue_find (stroke->anchors, anchor);

  g_return_if_fail (anchor_list != NULL);

  switch (feature)
    {
    case CLAY_ANCHOR_FEATURE_EDGE:
      if (anchor->type == CLAY_ANCHOR_ANCHOR)
        {
          if (g_list_previous (anchor_list))
            CLAY_ANCHOR (g_list_previous (anchor_list)->data)->position =
              anchor->position;

          if (g_list_next (anchor_list))
            CLAY_ANCHOR (g_list_next (anchor_list)->data)->position =
              anchor->position;
        }
      else
        {
          if (g_list_previous (anchor_list) &&
              CLAY_ANCHOR (g_list_previous (anchor_list)->data)->type == CLAY_ANCHOR_ANCHOR)
            anchor->position = CLAY_ANCHOR (g_list_previous (anchor_list)->data)->position;
          if (g_list_next (anchor_list) &&
              CLAY_ANCHOR (g_list_next (anchor_list)->data)->type == CLAY_ANCHOR_ANCHOR)
            anchor->position = CLAY_ANCHOR (g_list_next (anchor_list)->data)->position;
        }

      break;

    default:
      g_warning ("clay_bezier_stroke_anchor_convert: "
                 "unimplemented anchor conversion %d\n", feature);
    }
}


static ClayBezierDesc *
clay_bezier_stroke_make_bezier (ClayStroke *stroke)
{
  GArray            *points;
  GArray            *cmd_array;
  ClayBezierDesc    *bezdesc;
  cairo_path_data_t  pathdata;
  gint               num_cmds, i;

  points = clay_stroke_control_points_get (stroke, NULL);

  g_return_val_if_fail (points && points->len % 3 == 0, NULL);
  if (points->len < 3)
    return NULL;

  /* Moveto + (n-1) * curveto + (if closed) curveto + closepath */
  num_cmds = 2 + (points->len / 3 - 1) * 4;
  if (stroke->closed)
    num_cmds += 1 + 4;

  cmd_array = g_array_sized_new (FALSE, FALSE,
                                 sizeof (cairo_path_data_t),
                                 num_cmds);

  pathdata.header.type = CAIRO_PATH_MOVE_TO;
  pathdata.header.length = 2;
  g_array_append_val (cmd_array, pathdata);
  pathdata.point.x = g_array_index (points, ClayAnchor, 1).position.x;
  pathdata.point.y = g_array_index (points, ClayAnchor, 1).position.y;
  g_array_append_val (cmd_array, pathdata);

  for (i = 2; i+2 < points->len; i += 3)
    {
      pathdata.header.type = CAIRO_PATH_CURVE_TO;
      pathdata.header.length = 4;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, i).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, i).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, i+1).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, i+1).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, i+2).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, i+2).position.y;
      g_array_append_val (cmd_array, pathdata);
    }

  if (stroke->closed)
    {
      pathdata.header.type = CAIRO_PATH_CURVE_TO;
      pathdata.header.length = 4;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, i).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, i).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, 0).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, 0).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.point.x = g_array_index (points, ClayAnchor, 1).position.x;
      pathdata.point.y = g_array_index (points, ClayAnchor, 1).position.y;
      g_array_append_val (cmd_array, pathdata);

      pathdata.header.type = CAIRO_PATH_CLOSE_PATH;
      pathdata.header.length = 1;
      g_array_append_val (cmd_array, pathdata);
    }

  if (cmd_array->len != num_cmds)
    g_printerr ("miscalculated path cmd length! (%d vs. %d)\n",
                cmd_array->len, num_cmds);

  bezdesc = clay_bezier_desc_new ((cairo_path_data_t *) cmd_array->data,
                                  cmd_array->len);
  g_array_free (points, TRUE);
  g_array_free (cmd_array, FALSE);

  return bezdesc;
}


static GArray *
clay_bezier_stroke_interpolate (ClayStroke *stroke,
                                gdouble     precision,
                                gboolean   *ret_closed)
{
  GArray     *ret_coords;
  ClayAnchor *anchor;
  GList      *anchorlist;
  ClayCoords  segmentcoords[4];
  gint        count;
  gboolean    need_endpoint = FALSE;

  if (g_queue_is_empty (stroke->anchors))
    {
      if (ret_closed)
        *ret_closed = FALSE;
      return NULL;
    }

  ret_coords = g_array_new (FALSE, FALSE, sizeof (ClayCoords));

  count = 0;

  for (anchorlist = stroke->anchors->head;
       anchorlist && CLAY_ANCHOR (anchorlist->data)->type != CLAY_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  for ( ; anchorlist; anchorlist = g_list_next (anchorlist))
    {
      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          clay_coords_interpolate_bezier (segmentcoords, precision,
                                          ret_coords, NULL);
          segmentcoords[0] = segmentcoords[3];
          count = 1;
          need_endpoint = TRUE;
        }
    }

  if (stroke->closed && ! g_queue_is_empty (stroke->anchors))
    {
      anchorlist = stroke->anchors->head;

      while (count < 3)
        {
          segmentcoords[count] = CLAY_ANCHOR (anchorlist->data)->position;
          count++;
        }
      anchorlist = g_list_next (anchorlist);
      if (anchorlist)
        segmentcoords[3] = CLAY_ANCHOR (anchorlist->data)->position;

      clay_coords_interpolate_bezier (segmentcoords, precision,
                                      ret_coords, NULL);
      need_endpoint = TRUE;

    }

  if (need_endpoint)
    ret_coords = g_array_append_val (ret_coords, segmentcoords[3]);

  if (ret_closed)
    *ret_closed = stroke->closed;

  if (ret_coords->len == 0)
    {
      g_array_free (ret_coords, TRUE);
      ret_coords = NULL;
    }

  return ret_coords;
}


static void
clay_bezier_stroke_transform (ClayStroke        *stroke,
                              const ClayMatrix3 *matrix,
                              GQueue            *ret_strokes)
{
  ClayStroke *first_stroke = NULL;
  ClayStroke *last_stroke  = NULL;
  GList      *anchorlist;
  ClayAnchor *anchor;
  ClayCoords  segmentcoords[4];
  GQueue     *transformed[2];
  gint        n_transformed;
  gint        count;
  gboolean    first;
  gboolean    last;

  /* if there's no need for clipping, use the default implementation */
  if (! ret_strokes                   ||
      clay_matrix3_is_affine (matrix) ||
      g_queue_is_empty (stroke->anchors))
    {
      CLAY_STROKE_CLASS (parent_class)->transform (stroke, matrix, ret_strokes);

      return;
    }

  /* transform the individual segments */
  count = 0;
  first = TRUE;
  last  = FALSE;

  /* find the first non-control anchor */
  for (anchorlist = stroke->anchors->head;
       anchorlist && CLAY_ANCHOR (anchorlist->data)->type != CLAY_ANCHOR_ANCHOR;
       anchorlist = g_list_next (anchorlist));

  for ( ; anchorlist || stroke->closed; anchorlist = g_list_next (anchorlist))
    {
      /* wrap around if 'stroke' is closed, so that we transform the final
       * segment
       */
      if (! anchorlist)
        {
          anchorlist = stroke->anchors->head;
          last       = TRUE;
        }

      anchor = anchorlist->data;

      segmentcoords[count] = anchor->position;
      count++;

      if (count == 4)
        {
          gboolean start_in;
          gboolean end_in;
          gint     i;

          clay_transform_bezier_coords (matrix, segmentcoords,
                                        transformed, &n_transformed,
                                        &start_in, &end_in);

          for (i = 0; i < n_transformed; i++)
            {
              ClayStroke *s = NULL;
              GList      *list;
              gint        j;

              if (i == 0 && start_in)
                {
                  /* current stroke is connected to last stroke */
                  s = last_stroke;
                }
              else if (last_stroke)
                {
                  /* current stroke is not connected to last stroke.  finalize
                   * last stroke.
                   */
                  anchor = g_queue_peek_tail (last_stroke->anchors);

                  g_queue_push_tail (last_stroke->anchors,
                                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                                      &anchor->position));
                }

              for (list = transformed[i]->head; list; list = g_list_next (list))
                {
                  ClayCoords *transformedcoords = list->data;

                  if (! s)
                    {
                      /* start a new stroke */
                      s = clay_bezier_stroke_new ();

                      g_queue_push_tail (s->anchors,
                                         clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                                          &transformedcoords[0]));

                      g_queue_push_tail (ret_strokes, s);

                      j = 0;
                    }
                  else
                    {
                      /* continue an existing stroke, skipping the first anchor,
                       * which is the same as the last anchor of the last stroke
                       */
                      j = 1;
                    }

                  for (; j < 4; j++)
                    {
                      ClayAnchorType type;

                      if (j == 0 || j == 3)
                        type = CLAY_ANCHOR_ANCHOR;
                      else
                        type = CLAY_ANCHOR_CONTROL;

                      g_queue_push_tail (s->anchors,
                                         clay_anchor_new (type,
                                                          &transformedcoords[j]));
                    }

                  g_free (transformedcoords);
                }

              g_queue_free (transformed[i]);

              /* if the current stroke is an initial segment of 'stroke',
               * remember it, so that we can possibly connect it to the last
               * stroke later.
               */
              if (i == 0 && start_in && first)
                first_stroke = s;

              last_stroke = s;
              first       = FALSE;
            }

          if (! end_in && last_stroke)
            {
              /* the next stroke is not connected to the last stroke.  finalize
               * the last stroke.
               */
              anchor = g_queue_peek_tail (last_stroke->anchors);

              g_queue_push_tail (last_stroke->anchors,
                                 clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                                  &anchor->position));

              last_stroke = NULL;
            }

          if (last)
            break;

          segmentcoords[0] = segmentcoords[3];
          count = 1;
        }
    }

  /* if the last stroke is a final segment of 'stroke'... */
  if (last_stroke)
    {
      /* ... and the first stroke is an initial segment of 'stroke', and
       * 'stroke' is closed ...
       */
      if (first_stroke && stroke->closed)
        {
          /* connect the first and last strokes */

          /* remove the first anchor, which is a synthetic control point */
          clay_anchor_free (g_queue_pop_head (first_stroke->anchors));
          /* remove the last anchor, which is the same anchor point as the
           * first anchor
           */
          clay_anchor_free (g_queue_pop_tail (last_stroke->anchors));

          if (first_stroke == last_stroke)
            {
              /* the result is a single stroke.  move the last anchor, which is
               * an orphan control point, to the front, to fill in the removed
               * control point of the first anchor, and close the stroke.
               */
              g_queue_push_head (first_stroke->anchors,
                                 g_queue_pop_tail (first_stroke->anchors));

              first_stroke->closed = TRUE;
            }
          else
            {
              /* the result is multiple strokes.  prepend the last stroke to
               * the first stroke, and discard it.
               */
              while ((anchor = g_queue_pop_tail (last_stroke->anchors)))
                g_queue_push_head (first_stroke->anchors, anchor);

              g_object_unref (g_queue_pop_tail (ret_strokes));
            }
        }
      else
        {
          /* otherwise, the first and last strokes are not connected.  finalize
           * the last stroke.
           */
          anchor = g_queue_peek_tail (last_stroke->anchors);

          g_queue_push_tail (last_stroke->anchors,
                             clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                              &anchor->position));
        }
    }
}


ClayStroke *
clay_bezier_stroke_new_moveto (const ClayCoords *start)
{
  ClayStroke *stroke = clay_bezier_stroke_new ();

  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      start));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_ANCHOR,
                                      start));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      start));
  return stroke;
}

void
clay_bezier_stroke_lineto (ClayStroke       *stroke,
                           const ClayCoords *end)
{
  g_return_if_fail (CLAY_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      end));
}

void
clay_bezier_stroke_conicto (ClayStroke       *stroke,
                            const ClayCoords *control,
                            const ClayCoords *end)
{
  ClayCoords start, coords;

  g_return_if_fail (CLAY_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_get_length (stroke->anchors) > 1);

  start = CLAY_ANCHOR (stroke->anchors->tail->prev->data)->position;

  clay_coords_mix (2.0 / 3.0, control, 1.0 / 3.0, &start, &coords);

  CLAY_ANCHOR (stroke->anchors->tail->data)->position = coords;

  clay_coords_mix (2.0 / 3.0, control, 1.0 / 3.0, end, &coords);

  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      &coords));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      end));
}

void
clay_bezier_stroke_cubicto (ClayStroke       *stroke,
                            const ClayCoords *control1,
                            const ClayCoords *control2,
                            const ClayCoords *end)
{
  g_return_if_fail (CLAY_IS_BEZIER_STROKE (stroke));
  g_return_if_fail (stroke->closed == FALSE);
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  CLAY_ANCHOR (stroke->anchors->tail->data)->position = *control1;

  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      control2));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_ANCHOR,
                                      end));
  g_queue_push_tail (stroke->anchors,
                     clay_anchor_new (CLAY_ANCHOR_CONTROL,
                                      end));
}

static gdouble
arcto_circleparam (gdouble  h,
                   gdouble *y)
{
  gdouble t0 = 0.5;
  gdouble dt = 0.25;
  gdouble pt0;
  gdouble y01, y12, y23, y012, y123, y0123;   /* subdividing y[] */

  while (dt >= 0.00001)
    {
      pt0 = (    y[0] * (1-t0) * (1-t0) * (1-t0) +
             3 * y[1] * (1-t0) * (1-t0) * t0     +
             3 * y[2] * (1-t0) * t0     * t0     +
                 y[3] * t0     * t0     * t0 );

      if (pt0 > h)
        t0 = t0 - dt;
      else if (pt0 < h)
        t0 = t0 + dt;
      else
        break;
      dt = dt/2;
    }

  y01   = y[0] * (1-t0) + y[1] * t0;
  y12   = y[1] * (1-t0) + y[2] * t0;
  y23   = y[2] * (1-t0) + y[3] * t0;
  y012  = y01  * (1-t0) + y12  * t0;
  y123  = y12  * (1-t0) + y23  * t0;
  y0123 = y012 * (1-t0) + y123 * t0;

  y[0] = y0123; y[1] = y123; y[2] = y23; /* y[3] unchanged */

  return t0;
}

static void
arcto_subdivide (gdouble     t,
                 gint        part,
                 ClayCoords *p)
{
  ClayCoords p01, p12, p23, p012, p123, p0123;

  clay_coords_mix (1-t, &(p[0]), t, &(p[1]), &p01  );
  clay_coords_mix (1-t, &(p[1]), t, &(p[2]), &p12  );
  clay_coords_mix (1-t, &(p[2]), t, &(p[3]), &p23  );
  clay_coords_mix (1-t,  &p01  , t,  &p12  , &p012 );
  clay_coords_mix (1-t,  &p12  , t,  &p23  , &p123 );
  clay_coords_mix (1-t,  &p012 , t,  &p123 , &p0123);

  if (part == 0)
    {
      /* p[0] unchanged */
      p[1] = p01;
      p[2] = p012;
      p[3] = p0123;
    }
  else
    {
      p[0] = p0123;
      p[1] = p123;
      p[2] = p23;
      /* p[3] unchanged */
    }
}

static void
arcto_ellipsesegment (gdouble     radius_x,
                      gdouble     radius_y,
                      gdouble     phi0,
                      gdouble     phi1,
                      ClayCoords *ellips)
{
  const ClayCoords  template    = CLAY_COORDS_DEFAULT_VALUES;
  const gdouble     circlemagic = 4.0 * (G_SQRT2 - 1.0) / 3.0;

  gdouble  phi_s, phi_e;
  gdouble  y[4];
  gdouble  h0, h1;
  gdouble  t0, t1;

  g_return_if_fail (ellips != NULL);

  y[0] = 0.0;
  y[1] = circlemagic;
  y[2] = 1.0;
  y[3] = 1.0;

  ellips[0] = template;
  ellips[1] = template;
  ellips[2] = template;
  ellips[3] = template;

  if (phi0 < phi1)
    {
      phi_s = floor (phi0 / G_PI_2) * G_PI_2;
      while (phi_s < 0) phi_s += 2 * G_PI;
      phi_e = phi_s + G_PI_2;
    }
  else
    {
      phi_e = floor (phi1 / G_PI_2) * G_PI_2;
      while (phi_e < 0) phi_e += 2 * G_PI;
      phi_s = phi_e + G_PI_2;
    }

  h0 = sin (fabs (phi0-phi_s));
  h1 = sin (fabs (phi1-phi_s));

  ellips[0].x = cos (phi_s); ellips[0].y = sin (phi_s);
  ellips[3].x = cos (phi_e); ellips[3].y = sin (phi_e);

  clay_coords_mix (1, &(ellips[0]), circlemagic, &(ellips[3]), &(ellips[1]));
  clay_coords_mix (circlemagic, &(ellips[0]), 1, &(ellips[3]), &(ellips[2]));

  if (h0 > y[0])
    {
      t0 = arcto_circleparam (h0, y); /* also subdivides y[] at t0 */
      arcto_subdivide (t0, 1, ellips);
    }

  if (h1 < y[3])
    {
      t1 = arcto_circleparam (h1, y);
      arcto_subdivide (t1, 0, ellips);
    }

  ellips[0].x *= radius_x ; ellips[0].y *= radius_y;
  ellips[1].x *= radius_x ; ellips[1].y *= radius_y;
  ellips[2].x *= radius_x ; ellips[2].y *= radius_y;
  ellips[3].x *= radius_x ; ellips[3].y *= radius_y;
}

void
clay_bezier_stroke_arcto (ClayStroke       *bez_stroke,
                          gdouble           radius_x,
                          gdouble           radius_y,
                          gdouble           angle_rad,
                          gboolean          large_arc,
                          gboolean          sweep,
                          const ClayCoords *end)
{
  ClayCoords  start;
  ClayCoords  middle;   /* between start and end */
  ClayCoords  trans_delta;
  ClayCoords  trans_center;
  ClayCoords  tmp_center;
  ClayCoords  center;
  ClayCoords  ellips[4]; /* control points of untransformed ellipse segment */
  ClayCoords  ctrl[4];   /* control points of next bezier segment           */

  ClayMatrix3 anglerot;

  gdouble     lambda;
  gdouble     phi0, phi1, phi2;
  gdouble     tmpx, tmpy;

  g_return_if_fail (CLAY_IS_BEZIER_STROKE (bez_stroke));
  g_return_if_fail (bez_stroke->closed == FALSE);
  g_return_if_fail (g_queue_get_length (bez_stroke->anchors) > 1);

  if (radius_x == 0 || radius_y == 0)
    {
      clay_bezier_stroke_lineto (bez_stroke, end);
      return;
    }

  start = CLAY_ANCHOR (bez_stroke->anchors->tail->prev->data)->position;

  clay_matrix3_identity (&anglerot);
  clay_matrix3_rotate (&anglerot, -angle_rad);

  clay_coords_mix (0.5, &start, -0.5, end, &trans_delta);
  clay_matrix3_transform_point (&anglerot,
                                trans_delta.x, trans_delta.y,
                                &tmpx, &tmpy);
  trans_delta.x = tmpx;
  trans_delta.y = tmpy;

  lambda = (SQR (trans_delta.x) / SQR (radius_x) +
            SQR (trans_delta.y) / SQR (radius_y));

  if (lambda < 0.00001)
    {
      /* don't bother with it - endpoint is too close to startpoint */
      return;
    }

  trans_center = trans_delta;

  if (lambda > 1.0)
    {
      /* The radii are too small for a matching ellipse. We expand them
       * so that they fit exactly (center of the ellipse between the
       * start- and endpoint
       */
      radius_x *= sqrt (lambda);
      radius_y *= sqrt (lambda);
      trans_center.x = 0.0;
      trans_center.y = 0.0;
    }
  else
    {
      gdouble factor = sqrt ((1.0 - lambda) / lambda);

      trans_center.x =   trans_delta.y * radius_x / radius_y * factor;
      trans_center.y = - trans_delta.x * radius_y / radius_x * factor;
    }

  if ((large_arc && sweep) || (!large_arc && !sweep))
    {
      trans_center.x *= -1;
      trans_center.y *= -1;
    }

  clay_matrix3_identity (&anglerot);
  clay_matrix3_rotate (&anglerot, angle_rad);

  tmp_center = trans_center;
  clay_matrix3_transform_point (&anglerot,
                                tmp_center.x, tmp_center.y,
                                &tmpx, &tmpy);
  tmp_center.x = tmpx;
  tmp_center.y = tmpy;

  clay_coords_mix (0.5, &start, 0.5, end, &middle);
  clay_coords_add (&tmp_center, &middle, &center);

  phi1 = atan2 ((trans_delta.y - trans_center.y) / radius_y,
                (trans_delta.x - trans_center.x) / radius_x);

  phi2 = atan2 ((- trans_delta.y - trans_center.y) / radius_y,
                (- trans_delta.x - trans_center.x) / radius_x);

  if (phi1 < 0)
    phi1 += 2 * G_PI;

  if (phi2 < 0)
    phi2 += 2 * G_PI;

  if (sweep)
    {
      while (phi2 < phi1)
        phi2 += 2 * G_PI;

      phi0 = floor (phi1 / G_PI_2) * G_PI_2;

      while (phi0 < phi2)
        {
          arcto_ellipsesegment (radius_x, radius_y,
                                MAX (phi0, phi1), MIN (phi0 + G_PI_2, phi2),
                                ellips);

          clay_matrix3_transform_point (&anglerot, ellips[0].x, ellips[0].y,
                                        &tmpx, &tmpy);
          ellips[0].x = tmpx; ellips[0].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[1].x, ellips[1].y,
                                        &tmpx, &tmpy);
          ellips[1].x = tmpx; ellips[1].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[2].x, ellips[2].y,
                                        &tmpx, &tmpy);
          ellips[2].x = tmpx; ellips[2].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[3].x, ellips[3].y,
                                        &tmpx, &tmpy);
          ellips[3].x = tmpx; ellips[3].y = tmpy;

          clay_coords_add (&center, &(ellips[1]), &(ctrl[1]));
          clay_coords_add (&center, &(ellips[2]), &(ctrl[2]));
          clay_coords_add (&center, &(ellips[3]), &(ctrl[3]));

          clay_bezier_stroke_cubicto (bez_stroke,
                                      &(ctrl[1]), &(ctrl[2]), &(ctrl[3]));
          phi0 += G_PI_2;
        }
    }
  else
    {
      while (phi1 < phi2)
        phi1 += 2 * G_PI;

      phi0 = ceil (phi1 / G_PI_2) * G_PI_2;

      while (phi0 > phi2)
        {
          arcto_ellipsesegment (radius_x, radius_y,
                                MIN (phi0, phi1), MAX (phi0 - G_PI_2, phi2),
                                ellips);

          clay_matrix3_transform_point (&anglerot, ellips[0].x, ellips[0].y,
                                        &tmpx, &tmpy);
          ellips[0].x = tmpx; ellips[0].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[1].x, ellips[1].y,
                                        &tmpx, &tmpy);
          ellips[1].x = tmpx; ellips[1].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[2].x, ellips[2].y,
                                        &tmpx, &tmpy);
          ellips[2].x = tmpx; ellips[2].y = tmpy;
          clay_matrix3_transform_point (&anglerot, ellips[3].x, ellips[3].y,
                                        &tmpx, &tmpy);
          ellips[3].x = tmpx; ellips[3].y = tmpy;

          clay_coords_add (&center, &(ellips[1]), &(ctrl[1]));
          clay_coords_add (&center, &(ellips[2]), &(ctrl[2]));
          clay_coords_add (&center, &(ellips[3]), &(ctrl[3]));

          clay_bezier_stroke_cubicto (bez_stroke,
                                      &(ctrl[1]), &(ctrl[2]), &(ctrl[3]));
          phi0 -= G_PI_2;
        }
    }
}

ClayStroke *
clay_bezier_stroke_new_ellipse (const ClayCoords *center,
                                gdouble           radius_x,
                                gdouble           radius_y,
                                gdouble           angle)
{
  ClayStroke    *stroke;
  ClayCoords     p1 = *center;
  ClayCoords     p2 = *center;
  ClayCoords     p3 = *center;
  ClayCoords     dx = { 0, };
  ClayCoords     dy = { 0, };
  const gdouble  circlemagic = 4.0 * (G_SQRT2 - 1.0) / 3.0;
  ClayAnchor    *handle;

  dx.x =   radius_x * cos (angle);
  dx.y = - radius_x * sin (angle);
  dy.x =   radius_y * sin (angle);
  dy.y =   radius_y * cos (angle);

  clay_coords_mix (1.0, center, 1.0, &dx, &p1);
  stroke = clay_bezier_stroke_new_moveto (&p1);

  handle = g_queue_peek_head (stroke->anchors);
  clay_coords_mix (1.0,    &p1, -circlemagic, &dy, &handle->position);

  clay_coords_mix (1.0,    &p1,  circlemagic, &dy, &p1);
  clay_coords_mix (1.0, center,          1.0, &dy, &p3);
  clay_coords_mix (1.0,    &p3,  circlemagic, &dx, &p2);
  clay_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  clay_coords_mix (1.0,    &p3, -circlemagic, &dx, &p1);
  clay_coords_mix (1.0, center,         -1.0, &dx, &p3);
  clay_coords_mix (1.0,    &p3,  circlemagic, &dy, &p2);
  clay_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  clay_coords_mix (1.0,    &p3, -circlemagic, &dy, &p1);
  clay_coords_mix (1.0, center,         -1.0, &dy, &p3);
  clay_coords_mix (1.0,    &p3, -circlemagic, &dx, &p2);
  clay_bezier_stroke_cubicto (stroke, &p1, &p2, &p3);

  handle = g_queue_peek_tail (stroke->anchors);
  clay_coords_mix (1.0,    &p3,  circlemagic, &dx, &handle->position);

  clay_stroke_close (stroke);

  return stroke;
}


/* helper function to get the associated anchor of a listitem */

static GList *
clay_bezier_stroke_get_anchor_listitem (GList *list)
{
  if (!list)
    return NULL;

  if (CLAY_ANCHOR (list->data)->type == CLAY_ANCHOR_ANCHOR)
    return list;

  if (list->prev && CLAY_ANCHOR (list->prev->data)->type == CLAY_ANCHOR_ANCHOR)
    return list->prev;

  if (list->next && CLAY_ANCHOR (list->next->data)->type == CLAY_ANCHOR_ANCHOR)
    return list->next;

  g_return_val_if_fail (/* bezier stroke inconsistent! */ FALSE, NULL);

  return NULL;
}

static gboolean
clay_bezier_stroke_exit_cp (ClayLoadState  *state,
                            const gchar    *text,
                            gsize           len,
                            gpointer        user_data,
                            GError        **error)
{
  ClayValueArray *control_points;
  const gchar    *type;
  gdouble         x;
  gdouble         y;

  clay_savable_load_get_parent_values (state,
                                       "control-points", &control_points,
                                       NULL);

  if (clay_savable_load_get_values (state,
                                    "control-point:type", &type,
                                    "control-point:x",    &x,
                                    "control-point:y",    &y,
                                    NULL))
    {
      ClayAnchor     anchor = { { 0, } };
      ClayAnchorType anchor_type;
      GValue         value  = G_VALUE_INIT;

      anchor_type = (g_strcmp0 (type, "anchor") == 0) ? CLAY_ANCHOR_ANCHOR : CLAY_ANCHOR_CONTROL;
      anchor.selected   = FALSE;
      anchor.type       = anchor_type;
      anchor.position.x = x;
      anchor.position.y = y;

      g_value_init (&value, CLAY_TYPE_ANCHOR);
      g_value_set_boxed (&value, &anchor);
      clay_value_array_append (control_points, &value);
      g_value_unset (&value);
    }

  return TRUE;
}
