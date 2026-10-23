/* xyz.kainzmayer.ClayDesign
 *
 * claynpointdeformationtool.c
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <npd/npd_common.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h" /* playground */

#include "gegl/clay-gegl-utils.h"
#include "gegl/clay-gegl-apply-operation.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claycanvasbufferpreview.h"

#include "claynpointdeformationtool.h"
#include "claynpointdeformationoptions.h"
#include "claytoolcontrol.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


//#define CLAY_NPD_DEBUG
#define CLAY_NPD_MAXIMUM_DEFORMATION_DELAY 100000 /* 100000 microseconds == 10 FPS */
#define CLAY_NPD_DRAW_INTERVAL                 50 /*     50 milliseconds == 20 FPS */


static void     clay_n_point_deformation_tool_start                   (ClayNPointDeformationTool *npd_tool,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_halt                    (ClayNPointDeformationTool *npd_tool);
static void     clay_n_point_deformation_tool_commit                  (ClayNPointDeformationTool *npd_tool);
static void     clay_n_point_deformation_tool_set_options             (ClayNPointDeformationTool *npd_tool,
                                                                       ClayNPointDeformationOptions *npd_options);
static void     clay_n_point_deformation_tool_options_notify          (ClayTool                  *tool,
                                                                       ClayToolOptions           *options,
                                                                       const GParamSpec          *pspec);
static void     clay_n_point_deformation_tool_button_press            (ClayTool                  *tool,
                                                                       const ClayCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       ClayButtonPressType        press_type,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_button_release          (ClayTool                  *tool,
                                                                       const ClayCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       ClayButtonReleaseType      release_type,
                                                                       ClayDisplay               *display);
static gboolean clay_n_point_deformation_tool_key_press               (ClayTool                  *tool,
                                                                       GdkEventKey               *kevent,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_modifier_key            (ClayTool                  *tool,
                                                                       GdkModifierType            key,
                                                                       gboolean                   press,
                                                                       GdkModifierType            state,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_cursor_update           (ClayTool                  *tool,
                                                                       const ClayCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_draw                    (ClayDrawTool              *draw_tool);
static void     clay_n_point_deformation_tool_control                 (ClayTool                  *tool,
                                                                       ClayToolAction             action,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_oper_update             (ClayTool                  *tool,
                                                                       const ClayCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       gboolean                   proximity,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_motion                  (ClayTool                  *tool,
                                                                       const ClayCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       ClayDisplay               *display);
static void     clay_n_point_deformation_tool_clear_selected_points_list
                                                                      (ClayNPointDeformationTool *npd_tool);
static gboolean clay_n_point_deformation_tool_add_cp_to_selection     (ClayNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
static gboolean clay_n_point_deformation_tool_is_cp_in_area           (NPDControlPoint           *cp,
                                                                       gfloat                     x0,
                                                                       gfloat                     y0,
                                                                       gfloat                     x1,
                                                                       gfloat                     y1,
                                                                       gfloat                     offset_x,
                                                                       gfloat                     offset_y,
                                                                       gfloat                     cp_radius);
static void     clay_n_point_deformation_tool_remove_cp_from_selection
                                                                      (ClayNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
static gpointer clay_n_point_deformation_tool_deform_thread_func      (gpointer                   data);
static gboolean clay_n_point_deformation_tool_canvas_update_timeout   (ClayNPointDeformationTool *npd_tool);
static void     clay_n_point_deformation_tool_perform_deformation     (ClayNPointDeformationTool *npd_tool);
static void     clay_n_point_deformation_tool_halt_threads            (ClayNPointDeformationTool *npd_tool);
static void     clay_n_point_deformation_tool_apply_deformation       (ClayNPointDeformationTool *npd_tool);

#ifdef CLAY_NPD_DEBUG
#define clay_npd_debug(x) g_printf x
#else
#define clay_npd_debug(x)
#endif

G_DEFINE_TYPE (ClayNPointDeformationTool, clay_n_point_deformation_tool,
               CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_n_point_deformation_tool_parent_class


void
clay_n_point_deformation_tool_register (ClayToolRegisterCallback  callback,
                                        gpointer                  data)
{
  /* we should not know that "data" is a Clay*, but what the heck this
   * is experimental playground stuff
   */
  if (CLAY_GUI_CONFIG (Clay (data)->config)->playground_npd_tool)
    (* callback) (CLAY_TYPE_N_POINT_DEFORMATION_TOOL,
                  CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS,
                  clay_n_point_deformation_options_gui,
                  0,
                  "clay-n-point-deformation-tool",
                  _("N-Point Deformation"),
                  _("N-Point Deformation Tool: Rubber-like deformation of "
                    "image using points"),
                  N_("_N-Point Deformation"), "<shift>N",
                  NULL, CLAY_HELP_TOOL_N_POINT_DEFORMATION,
                  CLAY_ICON_TOOL_N_POINT_DEFORMATION,
                  data);
}

static void
clay_n_point_deformation_tool_class_init (ClayNPointDeformationToolClass *klass)
{
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  tool_class->options_notify = clay_n_point_deformation_tool_options_notify;
  tool_class->button_press   = clay_n_point_deformation_tool_button_press;
  tool_class->button_release = clay_n_point_deformation_tool_button_release;
  tool_class->key_press      = clay_n_point_deformation_tool_key_press;
  tool_class->modifier_key   = clay_n_point_deformation_tool_modifier_key;
  tool_class->control        = clay_n_point_deformation_tool_control;
  tool_class->motion         = clay_n_point_deformation_tool_motion;
  tool_class->oper_update    = clay_n_point_deformation_tool_oper_update;
  tool_class->cursor_update  = clay_n_point_deformation_tool_cursor_update;

  draw_tool_class->draw      = clay_n_point_deformation_tool_draw;
}

static void
clay_n_point_deformation_tool_init (ClayNPointDeformationTool *npd_tool)
{
  ClayTool *tool = CLAY_TOOL (npd_tool);

  clay_tool_control_set_precision            (tool->control,
                                              CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor          (tool->control,
                                              CLAY_TOOL_CURSOR_PERSPECTIVE);
  clay_tool_control_set_preserve             (tool->control, FALSE);
  clay_tool_control_set_wants_click          (tool->control, TRUE);
  clay_tool_control_set_wants_all_key_events (tool->control, TRUE);
  clay_tool_control_set_handle_empty_image   (tool->control, FALSE);
  clay_tool_control_set_dirty_mask           (tool->control,
                                              CLAY_DIRTY_IMAGE           |
                                              CLAY_DIRTY_IMAGE_STRUCTURE |
                                              CLAY_DIRTY_DRAWABLE        |
                                              CLAY_DIRTY_SELECTION       |
                                              CLAY_DIRTY_ACTIVE_DRAWABLE);
}

static void
clay_n_point_deformation_tool_control (ClayTool       *tool,
                                       ClayToolAction  action,
                                       ClayDisplay    *display)
{
  ClayNPointDeformationTool *npd_tool = CLAY_N_POINT_DEFORMATION_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_n_point_deformation_tool_halt (npd_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_n_point_deformation_tool_commit (npd_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_n_point_deformation_tool_start (ClayNPointDeformationTool *npd_tool,
                                     ClayDisplay               *display)
{
  ClayTool                     *tool = CLAY_TOOL (npd_tool);
  ClayNPointDeformationOptions *npd_options;
  ClayImage                    *image;
  GeglBuffer                   *source_buffer;
  GeglBuffer                   *preview_buffer;
  NPDModel                     *model;

  npd_options = CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

  image = clay_display_get_image (display);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = clay_image_get_selected_drawables (image);

  npd_tool->active = TRUE;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  /* create GEGL graph */
  source_buffer  = clay_drawable_get_buffer (tool->drawables->data);

  preview_buffer = gegl_buffer_new (gegl_buffer_get_extent (source_buffer),
                                    babl_format ("cairo-ARGB32"));

  npd_tool->graph    = gegl_node_new ();

  npd_tool->source   = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:buffer-source",
                                            "buffer",    source_buffer,
                                            NULL);
  npd_tool->npd_node = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:npd",
                                            NULL);
  npd_tool->sink     = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:write-buffer",
                                            "buffer",    preview_buffer,
                                            NULL);

  gegl_node_link_many (npd_tool->source,
                       npd_tool->npd_node,
                       npd_tool->sink,
                       NULL);

  /* initialize some options */
  g_object_set (G_OBJECT (npd_options), "mesh-visible", TRUE, NULL);
  clay_n_point_deformation_options_set_sensitivity (npd_options, TRUE);

  /* compute and get model */
  gegl_node_process (npd_tool->npd_node);
  gegl_node_get (npd_tool->npd_node, "model", &model, NULL);

  npd_tool->model          = model;
  npd_tool->preview_buffer = preview_buffer;
  npd_tool->selected_cp    = NULL;
  npd_tool->hovering_cp    = NULL;
  npd_tool->selected_cps   = NULL;
  npd_tool->rubber_band    = FALSE;
  npd_tool->lattice_points = g_new (ClayVector2,
                                    5 * model->hidden_model->num_of_bones);

  clay_item_get_offset (CLAY_ITEM (tool->drawables->data),
                        &npd_tool->offset_x, &npd_tool->offset_y);
  clay_npd_debug (("offset: %f %f\n", npd_tool->offset_x, npd_tool->offset_y));

  clay_draw_tool_start (CLAY_DRAW_TOOL (npd_tool), display);

  clay_n_point_deformation_tool_perform_deformation (npd_tool);

  /* hide original image */
  clay_item_set_visible (CLAY_ITEM (tool->drawables->data), FALSE, FALSE);
  clay_image_flush (image);

  /* create and start a deformation thread */
  npd_tool->deform_thread =
    g_thread_new ("deform thread",
                  (GThreadFunc) clay_n_point_deformation_tool_deform_thread_func,
                  npd_tool);

  /* create and start canvas update timeout */
  npd_tool->draw_timeout_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                  CLAY_NPD_DRAW_INTERVAL,
                                  (GSourceFunc) clay_n_point_deformation_tool_canvas_update_timeout,
                                  npd_tool,
                                  NULL);
}

static void
clay_n_point_deformation_tool_commit (ClayNPointDeformationTool *npd_tool)
{
  ClayTool *tool = CLAY_TOOL (npd_tool);

  if (npd_tool->active)
    {
      clay_n_point_deformation_tool_halt_threads (npd_tool);

      clay_tool_control_push_preserve (tool->control, TRUE);
      clay_n_point_deformation_tool_apply_deformation (npd_tool);
      clay_tool_control_pop_preserve (tool->control);

      /* show original/deformed image */
      clay_item_set_visible (CLAY_ITEM (tool->drawables->data), TRUE, FALSE);
      clay_image_flush (clay_display_get_image (tool->display));

      npd_tool->active = FALSE;
    }
}

static void
clay_n_point_deformation_tool_halt (ClayNPointDeformationTool *npd_tool)
{
  ClayTool                     *tool      = CLAY_TOOL (npd_tool);
  ClayDrawTool                 *draw_tool = CLAY_DRAW_TOOL (npd_tool);
  ClayNPointDeformationOptions *npd_options;

  npd_options = CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  if (npd_tool->active)
    {
      clay_n_point_deformation_tool_halt_threads (npd_tool);

      /* show original/deformed image */
      clay_item_set_visible (CLAY_ITEM (tool->drawables->data), TRUE, FALSE);
      clay_image_flush (clay_display_get_image (tool->display));

      /* disable some options */
      clay_n_point_deformation_options_set_sensitivity (npd_options, FALSE);

      npd_tool->active = FALSE;
    }

  if (clay_draw_tool_is_active (draw_tool))
    clay_draw_tool_stop (draw_tool);

  clay_n_point_deformation_tool_clear_selected_points_list (npd_tool);

  g_clear_object (&npd_tool->graph);
  npd_tool->source   = NULL;
  npd_tool->npd_node = NULL;
  npd_tool->sink     = NULL;

  g_clear_object (&npd_tool->preview_buffer);
  g_clear_pointer (&npd_tool->lattice_points, g_free);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
clay_n_point_deformation_tool_set_options (ClayNPointDeformationTool    *npd_tool,
                                           ClayNPointDeformationOptions *npd_options)
{
  gegl_node_set (npd_tool->npd_node,
                 "square-size",       (gint) npd_options->square_size,
                 "rigidity",          (gint) npd_options->rigidity,
                 "asap-deformation",  npd_options->asap_deformation,
                 "mls-weights",       npd_options->mls_weights,
                 "mls-weights-alpha", npd_options->mls_weights_alpha,
                 NULL);
}

static void
clay_n_point_deformation_tool_options_notify (ClayTool         *tool,
                                              ClayToolOptions  *options,
                                              const GParamSpec *pspec)
{
  ClayNPointDeformationTool    *npd_tool    = CLAY_N_POINT_DEFORMATION_TOOL (tool);
  ClayNPointDeformationOptions *npd_options = CLAY_N_POINT_DEFORMATION_OPTIONS (options);
  ClayDrawTool                 *draw_tool   = CLAY_DRAW_TOOL (tool);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! npd_tool->active)
    return;

  clay_draw_tool_pause (draw_tool);

  clay_npd_debug (("npd options notify\n"));
  clay_n_point_deformation_tool_set_options (npd_tool, npd_options);

  clay_draw_tool_resume (draw_tool);
}

static gboolean
clay_n_point_deformation_tool_key_press (ClayTool    *tool,
                                         GdkEventKey *kevent,
                                         ClayDisplay *display)
{
  ClayNPointDeformationTool *npd_tool = CLAY_N_POINT_DEFORMATION_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      /* if there is at least one control point, remove last added
       *  control point
       */
      if (npd_tool->model                 &&
          npd_tool->model->control_points &&
          npd_tool->model->control_points->len > 0)
        {
          GArray          *cps = npd_tool->model->control_points;
          NPDControlPoint *cp  = &g_array_index (cps, NPDControlPoint,
                                                 cps->len - 1);

          clay_npd_debug (("removing last cp %p\n", cp));
          clay_n_point_deformation_tool_remove_cp_from_selection (npd_tool, cp);
          npd_remove_control_point (npd_tool->model, cp);
        }
      break;

    case GDK_KEY_Delete:
      if (npd_tool->model &&
          npd_tool->selected_cps)
        {
          /* if there is at least one selected control point, remove it */
          npd_remove_control_points (npd_tool->model, npd_tool->selected_cps);
          clay_n_point_deformation_tool_clear_selected_points_list (npd_tool);
        }
      break;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);
      break;

    case GDK_KEY_Escape:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
      break;

    default:
      return FALSE;
  }

  return TRUE;
}

static void
clay_n_point_deformation_tool_modifier_key (ClayTool        *tool,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state,
                                            ClayDisplay     *display)
{
}

static void
clay_n_point_deformation_tool_cursor_update (ClayTool         *tool,
                                             const ClayCoords *coords,
                                             GdkModifierType   state,
                                             ClayDisplay      *display)
{
  ClayNPointDeformationTool *npd_tool = CLAY_N_POINT_DEFORMATION_TOOL (tool);
  ClayCursorModifier         modifier = CLAY_CURSOR_MODIFIER_PLUS;

  if (! npd_tool->active)
    {
      modifier = CLAY_CURSOR_MODIFIER_NONE;
    }
  else if (npd_tool->hovering_cp)
    {
      modifier = CLAY_CURSOR_MODIFIER_MOVE;
    }

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_n_point_deformation_tool_clear_selected_points_list (ClayNPointDeformationTool *npd_tool)
{
  if (npd_tool->selected_cps)
    {
      g_list_free (npd_tool->selected_cps);
      npd_tool->selected_cps = NULL;
    }
}

static gboolean
clay_n_point_deformation_tool_add_cp_to_selection (ClayNPointDeformationTool *npd_tool,
                                                   NPDControlPoint           *cp)
{
  if (! g_list_find (npd_tool->selected_cps, cp))
    {
      npd_tool->selected_cps = g_list_append (npd_tool->selected_cps, cp);

      return TRUE;
    }

  return FALSE;
}

static void
clay_n_point_deformation_tool_remove_cp_from_selection (ClayNPointDeformationTool *npd_tool,
                                                        NPDControlPoint           *cp)
{
  npd_tool->selected_cps = g_list_remove (npd_tool->selected_cps, cp);
}

static void
clay_n_point_deformation_tool_button_press (ClayTool            *tool,
                                            const ClayCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            ClayButtonPressType  press_type,
                                            ClayDisplay         *display)
{
  ClayNPointDeformationTool *npd_tool = CLAY_N_POINT_DEFORMATION_TOOL (tool);

  if (display != tool->display)
    {
      /* this is the first click on the drawable - just start the tool */
      clay_n_point_deformation_tool_start (npd_tool, display);
    }

  npd_tool->selected_cp = NULL;

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    {
      NPDControlPoint *cp = npd_tool->hovering_cp;

      if (cp)
        {
          /* there is a control point at cursor's position */
          npd_tool->selected_cp = cp;

          if (! g_list_find (npd_tool->selected_cps, cp))
            {
              /* control point isn't selected, so we can add it to the
               * list of selected control points
               */

              if (! (state & clay_get_extend_selection_mask ()))
                {
                  /* <SHIFT> isn't pressed, so this isn't a
                   * multiselection - clear the list of selected
                   * control points
                   */
                  clay_n_point_deformation_tool_clear_selected_points_list (npd_tool);
                }

              clay_n_point_deformation_tool_add_cp_to_selection (npd_tool, cp);
            }
          else if (state & clay_get_extend_selection_mask ())
            {
              /* control point is selected and <SHIFT> is pressed -
               * remove control point from selected points
               */
              clay_n_point_deformation_tool_remove_cp_from_selection (npd_tool,
                                                                      cp);
            }
        }

      npd_tool->start_x = coords->x;
      npd_tool->start_y = coords->y;

      npd_tool->last_x = coords->x;
      npd_tool->last_y = coords->y;
    }

  clay_tool_control_activate (tool->control);
}

static gboolean
clay_n_point_deformation_tool_is_cp_in_area (NPDControlPoint *cp,
                                             gfloat           x0,
                                             gfloat           y0,
                                             gfloat           x1,
                                             gfloat           y1,
                                             gfloat           offset_x,
                                             gfloat           offset_y,
                                             gfloat           cp_radius)
{
  NPDPoint p = cp->point;

  p.x += offset_x;
  p.y += offset_y;

  return p.x >= x0 - cp_radius && p.x <= x1 + cp_radius &&
         p.y >= y0 - cp_radius && p.y <= y1 + cp_radius;
}

static void
clay_n_point_deformation_tool_button_release (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonReleaseType  release_type,
                                              ClayDisplay           *display)
{
  ClayNPointDeformationTool *npd_tool = CLAY_N_POINT_DEFORMATION_TOOL (tool);

  clay_tool_control_halt (tool->control);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (npd_tool));

  if (release_type == CLAY_BUTTON_RELEASE_CLICK)
    {
      if (! npd_tool->hovering_cp)
        {
          NPDPoint p;

          clay_npd_debug (("cp doesn't exist, adding\n"));
          p.x = coords->x - npd_tool->offset_x;
          p.y = coords->y - npd_tool->offset_y;

          npd_add_control_point (npd_tool->model, &p);
        }
    }
  else if (release_type == CLAY_BUTTON_RELEASE_NORMAL)
    {
      if (npd_tool->rubber_band)
        {
          GArray *cps = npd_tool->model->control_points;
          gint    x0  = MIN (npd_tool->start_x, npd_tool->cursor_x);
          gint    y0  = MIN (npd_tool->start_y, npd_tool->cursor_y);
          gint    x1  = MAX (npd_tool->start_x, npd_tool->cursor_x);
          gint    y1  = MAX (npd_tool->start_y, npd_tool->cursor_y);
          gint    i;

          if (! (state & clay_get_extend_selection_mask ()))
            {
              /* <SHIFT> isn't pressed, so we want a clear selection */
              clay_n_point_deformation_tool_clear_selected_points_list (npd_tool);
            }

          for (i = 0; i < cps->len; i++)
            {
              NPDControlPoint *cp = &g_array_index (cps, NPDControlPoint, i);

              if (clay_n_point_deformation_tool_is_cp_in_area (cp,
                                                               x0, y0,
                                                               x1, y1,
                                                               npd_tool->offset_x,
                                                               npd_tool->offset_y,
                                                               npd_tool->cp_scaled_radius))
                {
                  /* control point is situated in an area defined by
                   * rubber band
                  */
                  clay_n_point_deformation_tool_add_cp_to_selection (npd_tool,
                                                                     cp);
                  clay_npd_debug (("%p selected\n", cp));
                }
            }
        }
    }
  else if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      clay_npd_debug (("clay_button_release_cancel\n"));
    }

  npd_tool->rubber_band = FALSE;

  clay_draw_tool_resume (CLAY_DRAW_TOOL (npd_tool));
}

static void
clay_n_point_deformation_tool_oper_update (ClayTool         *tool,
                                           const ClayCoords *coords,
                                           GdkModifierType   state,
                                           gboolean          proximity,
                                           ClayDisplay      *display)
{
  ClayNPointDeformationTool *npd_tool  = CLAY_N_POINT_DEFORMATION_TOOL (tool);
  ClayDrawTool              *draw_tool = CLAY_DRAW_TOOL (tool);

  clay_draw_tool_pause (draw_tool);

  if (npd_tool->active)
    {
      NPDModel         *model = npd_tool->model;
      ClayDisplayShell *shell = clay_display_get_shell (display);
      NPDPoint          p;

      npd_tool->cp_scaled_radius = model->control_point_radius / shell->scale_x;

      p.x = coords->x - npd_tool->offset_x;
      p.y = coords->y - npd_tool->offset_y;

      npd_tool->hovering_cp =
        npd_get_control_point_with_radius_at (model,
                                              &p,
                                              npd_tool->cp_scaled_radius);
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  clay_draw_tool_resume (draw_tool);
}

static void
clay_n_point_deformation_tool_prepare_lattice (ClayNPointDeformationTool *npd_tool)
{
  NPDHiddenModel *hm     = npd_tool->model->hidden_model;
  ClayVector2    *points = npd_tool->lattice_points;
  gint            i, j;

  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone *bone = &hm->current_bones[i];

      for (j = 0; j < 4; j++)
        clay_vector2_set (&points[5 * i + j], bone->points[j].x, bone->points[j].y);
      clay_vector2_set (&points[5 * i + j], bone->points[0].x, bone->points[0].y);
    }
}

static void
clay_n_point_deformation_tool_draw_lattice (ClayNPointDeformationTool *npd_tool)
{
  ClayVector2 *points    = npd_tool->lattice_points;
  gint         n_squares = npd_tool->model->hidden_model->num_of_bones;
  gint         i;

  for (i = 0; i < n_squares; i++)
    clay_draw_tool_add_lines (CLAY_DRAW_TOOL (npd_tool),
                              &points[5 * i], 5, NULL, FALSE);
}

static void
clay_n_point_deformation_tool_draw (ClayDrawTool *draw_tool)
{
  ClayNPointDeformationTool    *npd_tool;
  ClayNPointDeformationOptions *npd_options;
  NPDModel                     *model;
  gint                          x0, y0, x1, y1;
  gint                          i;

  npd_tool    = CLAY_N_POINT_DEFORMATION_TOOL (draw_tool);
  npd_options = CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  model = npd_tool->model;

  g_return_if_fail (model != NULL);

  /* draw lattice */
  if (npd_options->mesh_visible)
    clay_n_point_deformation_tool_draw_lattice (npd_tool);

  x0 = MIN (npd_tool->start_x, npd_tool->cursor_x);
  y0 = MIN (npd_tool->start_y, npd_tool->cursor_y);
  x1 = MAX (npd_tool->start_x, npd_tool->cursor_x);
  y1 = MAX (npd_tool->start_y, npd_tool->cursor_y);

  for (i = 0; i < model->control_points->len; i++)
    {
      NPDControlPoint *cp = &g_array_index (model->control_points,
                                            NPDControlPoint, i);
      NPDPoint         p  = cp->point;
      ClayHandleType   handle_type;

      p.x += npd_tool->offset_x;
      p.y += npd_tool->offset_y;

      handle_type = CLAY_HANDLE_CIRCLE;

      /* check if cursor is hovering over a control point or if a
       * control point is situated in an area defined by rubber band
       */
      if (cp == npd_tool->hovering_cp ||
          (npd_tool->rubber_band &&
           clay_n_point_deformation_tool_is_cp_in_area (cp,
                                                        x0, y0,
                                                        x1, y1,
                                                        npd_tool->offset_x,
                                                        npd_tool->offset_y,
                                                        npd_tool->cp_scaled_radius)))
        {
          handle_type = CLAY_HANDLE_FILLED_CIRCLE;
        }

      clay_draw_tool_add_handle (draw_tool,
                                 handle_type,
                                 p.x, p.y,
                                 CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                 CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                 CLAY_HANDLE_ANCHOR_CENTER);

      if (g_list_find (npd_tool->selected_cps, cp))
        {
          clay_draw_tool_add_handle (draw_tool,
                                     CLAY_HANDLE_SQUARE,
                                     p.x, p.y,
                                     CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                     CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                     CLAY_HANDLE_ANCHOR_CENTER);
        }
    }

  if (npd_tool->rubber_band)
    {
      /* draw a rubber band */
      clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                    x0, y0, x1 - x0, y1 - y0);
    }

  if (npd_tool->preview_buffer)
    {
      ClayCanvasItem *item;

      item = clay_canvas_buffer_preview_new (clay_display_get_shell (draw_tool->display),
                                             npd_tool->preview_buffer);

      clay_draw_tool_add_preview (draw_tool, item);
      g_object_unref (item);
    }

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
clay_n_point_deformation_tool_motion (ClayTool         *tool,
                                      const ClayCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      ClayDisplay      *display)
{
  ClayNPointDeformationTool *npd_tool  = CLAY_N_POINT_DEFORMATION_TOOL (tool);
  ClayDrawTool              *draw_tool = CLAY_DRAW_TOOL (tool);

  clay_draw_tool_pause (draw_tool);

  if (npd_tool->selected_cp)
    {
      GList   *list;
      gdouble  shift_x = coords->x - npd_tool->last_x;
      gdouble  shift_y = coords->y - npd_tool->last_y;

      for (list = npd_tool->selected_cps; list; list = g_list_next (list))
        {
          NPDControlPoint *cp = list->data;

          cp->point.x += shift_x;
          cp->point.y += shift_y;
        }
    }
  else
    {
      /* activate a rubber band selection */
      npd_tool->rubber_band = TRUE;
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  npd_tool->last_x = coords->x;
  npd_tool->last_y = coords->y;

  clay_draw_tool_resume (draw_tool);
}

static gboolean
clay_n_point_deformation_tool_canvas_update_timeout (ClayNPointDeformationTool *npd_tool)
{
  if (! CLAY_TOOL (npd_tool)->drawables->data)
    return FALSE;

  clay_npd_debug (("canvas update thread\n"));

  clay_draw_tool_pause (CLAY_DRAW_TOOL(npd_tool));
  clay_draw_tool_resume (CLAY_DRAW_TOOL(npd_tool));

  clay_npd_debug (("canvas update thread stop\n"));

  return TRUE;
}

static gpointer
clay_n_point_deformation_tool_deform_thread_func (gpointer data)
{
  ClayNPointDeformationTool    *npd_tool = data;
  ClayNPointDeformationOptions *npd_options;
  guint64                       start, duration;

  npd_options = CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  npd_tool->deformation_active = TRUE;

  while (npd_tool->deformation_active)
    {
      start = g_get_monotonic_time ();

      clay_n_point_deformation_tool_perform_deformation (npd_tool);

      if (npd_options->mesh_visible)
        clay_n_point_deformation_tool_prepare_lattice (npd_tool);

      duration = g_get_monotonic_time () - start;
      if (duration < CLAY_NPD_MAXIMUM_DEFORMATION_DELAY)
        {
          g_usleep (CLAY_NPD_MAXIMUM_DEFORMATION_DELAY - duration);
        }
    }

  clay_npd_debug (("deform thread exit\n"));

  return NULL;
}

static void
clay_n_point_deformation_tool_perform_deformation (ClayNPointDeformationTool *npd_tool)
{
  GObject *operation;

  g_object_get (npd_tool->npd_node,
                "gegl-operation", &operation,
                NULL);
  clay_npd_debug (("gegl_operation_invalidate\n"));
  gegl_operation_invalidate (GEGL_OPERATION (operation), NULL, FALSE);
  g_object_unref (operation);

  clay_npd_debug (("gegl_node_process\n"));
  gegl_node_process (npd_tool->sink);
}

static void
clay_n_point_deformation_tool_halt_threads (ClayNPointDeformationTool *npd_tool)
{
  if (! npd_tool->deformation_active)
    return;

  clay_npd_debug (("waiting for deform thread to finish\n"));
  npd_tool->deformation_active = FALSE;

  /* wait for deformation thread to finish its work */
  if (npd_tool->deform_thread)
    {
      g_thread_join (npd_tool->deform_thread);
      npd_tool->deform_thread = NULL;
    }

  /* remove canvas update timeout */
  if (npd_tool->draw_timeout_id)
    {
      g_source_remove (npd_tool->draw_timeout_id);
      npd_tool->draw_timeout_id = 0;
    }

  clay_npd_debug (("finished\n"));
}

static void
clay_n_point_deformation_tool_apply_deformation (ClayNPointDeformationTool *npd_tool)
{
  ClayTool                     *tool = CLAY_TOOL (npd_tool);
  ClayNPointDeformationOptions *npd_options;
  GeglBuffer                   *buffer;
  ClayImage                    *image;
  gint                          width, height, prev;

  npd_options = CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  image  = clay_display_get_image (tool->display);
  buffer = clay_drawable_get_buffer (tool->drawables->data);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  prev = npd_options->rigidity;
  npd_options->rigidity = 0;
  clay_n_point_deformation_tool_set_options (npd_tool, npd_options);
  npd_options->rigidity = prev;

  clay_drawable_push_undo (tool->drawables->data, _("N-Point Deformation"), NULL,
                           0, 0, width, height);


  clay_gegl_apply_operation (NULL, NULL, _("N-Point Deformation"),
                             npd_tool->npd_node,
                             clay_drawable_get_buffer (tool->drawables->data),
                             NULL, FALSE);

  clay_drawable_update (tool->drawables->data,
                        0, 0, width, height);

  clay_projection_flush (clay_image_get_projection (image));
}

#undef clay_npd_debug
#ifdef CLAY_NPD_DEBUG
#undef CLAY_NPD_DEBUG
#endif
