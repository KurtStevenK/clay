/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/claychannel.h"
#include "core/claydrawable.h"
#include "core/claydrawablefilter.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"

#include "display/claydisplay.h"
#include "display/claytoolgui.h"

#include "clayoffsettool.h"
#include "clayfilteroptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static gboolean   clay_offset_tool_initialize            (ClayTool              *tool,
                                                          ClayDisplay           *display,
                                                          GError               **error);
static void       clay_offset_tool_control               (ClayTool              *tool,
                                                          ClayToolAction         action,
                                                          ClayDisplay           *display);
static void       clay_offset_tool_button_press          (ClayTool              *tool,
                                                          const ClayCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          ClayButtonPressType    press_type,
                                                          ClayDisplay           *display);
static void       clay_offset_tool_button_release        (ClayTool              *tool,
                                                          const ClayCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          ClayButtonReleaseType  release_type,
                                                          ClayDisplay           *display);
static void       clay_offset_tool_motion                (ClayTool              *tool,
                                                          const ClayCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          ClayDisplay           *display);
static void       clay_offset_tool_oper_update           (ClayTool              *tool,
                                                          const ClayCoords      *coords,
                                                          GdkModifierType        state,
                                                          gboolean               proximity,
                                                          ClayDisplay           *display);
static void       clay_offset_tool_cursor_update         (ClayTool              *tool,
                                                          const ClayCoords      *coords,
                                                          GdkModifierType        state,
                                                          ClayDisplay           *display);

static gchar    * clay_offset_tool_get_operation         (ClayFilterTool        *filter_tool,
                                                          gchar                **description);
static void       clay_offset_tool_dialog                (ClayFilterTool        *filter_tool);
static void       clay_offset_tool_config_notify         (ClayFilterTool        *filter_tool,
                                                          ClayConfig            *config,
                                                          const GParamSpec      *pspec);
static void       clay_offset_tool_region_changed        (ClayFilterTool        *filter_tool);

static void       clay_offset_tool_offset_changed        (ClaySizeEntry         *se,
                                                          ClayOffsetTool        *offset_tool);

static void       clay_offset_tool_half_xy_clicked       (GtkButton             *button,
                                                          ClayOffsetTool        *offset_tool);
static void       clay_offset_tool_half_x_clicked        (GtkButton             *button,
                                                          ClayOffsetTool        *offset_tool);
static void       clay_offset_tool_half_y_clicked        (GtkButton             *button,
                                                          ClayOffsetTool        *offset_tool);

static void       clay_offset_tool_edge_behavior_toggled (GtkToggleButton       *toggle,
                                                          ClayOffsetTool        *offset_tool);

static void       clay_offset_tool_color_changed         (ClayColorButton       *button,
                                                          ClayOffsetTool        *offset_tool);

static gint       clay_offset_tool_get_width             (ClayOffsetTool        *offset_tool);
static gint       clay_offset_tool_get_height            (ClayOffsetTool        *offset_tool);

static void       clay_offset_tool_update                (ClayOffsetTool        *offset_tool);

static void       clay_offset_tool_halt                  (ClayOffsetTool        *offset_tool);


G_DEFINE_TYPE (ClayOffsetTool, clay_offset_tool,
               CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_offset_tool_parent_class


void
clay_offset_tool_register (ClayToolRegisterCallback callback,
                           gpointer                 data)
{
  (* callback) (CLAY_TYPE_OFFSET_TOOL,
                CLAY_TYPE_FILTER_OPTIONS, NULL,
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-offset-tool",
                _("Offset"),
                _("Shift the pixels, optionally wrapping them at the borders"),
                N_("_Offset..."), NULL,
                NULL, CLAY_HELP_TOOL_OFFSET,
                CLAY_ICON_TOOL_OFFSET,
                data);
}

static void
clay_offset_tool_class_init (ClayOffsetToolClass *klass)
{
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  tool_class->initialize            = clay_offset_tool_initialize;
  tool_class->control               = clay_offset_tool_control;
  tool_class->button_press          = clay_offset_tool_button_press;
  tool_class->button_release        = clay_offset_tool_button_release;
  tool_class->motion                = clay_offset_tool_motion;
  tool_class->oper_update           = clay_offset_tool_oper_update;
  tool_class->cursor_update         = clay_offset_tool_cursor_update;

  filter_tool_class->get_operation  = clay_offset_tool_get_operation;
  filter_tool_class->dialog         = clay_offset_tool_dialog;
  filter_tool_class->config_notify  = clay_offset_tool_config_notify;
  filter_tool_class->region_changed = clay_offset_tool_region_changed;
}

static void
clay_offset_tool_init (ClayOffsetTool *offset_tool)
{
  ClayTool *tool = CLAY_TOOL (offset_tool);

  clay_tool_control_set_scroll_lock (tool->control, TRUE);
  clay_tool_control_set_precision   (tool->control,
                                     CLAY_CURSOR_PRECISION_PIXEL_CENTER);
}

static gboolean
clay_offset_tool_initialize (ClayTool     *tool,
                             ClayDisplay  *display,
                             GError      **error)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (tool);
  ClayContext    *context     = CLAY_CONTEXT (CLAY_TOOL_GET_OPTIONS (tool));
  ClayImage      *image;
  ClayDrawable   *drawable;
  GeglColor      *color;
  gdouble         xres;
  gdouble         yres;

  if (! CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    return FALSE;

  if (g_list_length (tool->drawables) != 1)
    {
      if (g_list_length (tool->drawables) > 1)
        clay_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        clay_tool_message_literal (tool, display, _("No selected drawables."));

      return FALSE;
    }

  drawable = tool->drawables->data;

  image = clay_item_get_image (CLAY_ITEM (drawable));

  clay_image_get_resolution (image, &xres, &yres);

  g_signal_handlers_block_by_func (offset_tool->offset_se,
                                   clay_offset_tool_offset_changed,
                                   offset_tool);

  clay_size_entry_set_resolution (
    CLAY_SIZE_ENTRY (offset_tool->offset_se), 0,
    xres, FALSE);
  clay_size_entry_set_resolution (
    CLAY_SIZE_ENTRY (offset_tool->offset_se), 1,
    yres, FALSE);

  if (CLAY_IS_LAYER (drawable))
    clay_tool_gui_set_description (filter_tool->gui, _("Offset Layer"));
  else if (CLAY_IS_LAYER_MASK (drawable))
    clay_tool_gui_set_description (filter_tool->gui, _("Offset Layer Mask"));
  else if (CLAY_IS_CHANNEL (drawable))
    clay_tool_gui_set_description (filter_tool->gui, _("Offset Channel"));
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  gtk_widget_set_sensitive (offset_tool->transparent_radio,
                            clay_drawable_has_alpha (drawable));

  g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                     clay_offset_tool_offset_changed,
                                     offset_tool);

  color = gegl_color_duplicate (clay_context_get_background (context));
  gegl_node_set (filter_tool->operation, "color", color, NULL);
  g_object_unref (color);

  clay_offset_tool_update (offset_tool);

  return TRUE;
}

static void
clay_offset_tool_control (ClayTool       *tool,
                          ClayToolAction  action,
                          ClayDisplay    *display)
{
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_offset_tool_halt (offset_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
clay_offset_tool_get_operation (ClayFilterTool  *filter_tool,
                                gchar          **description)
{
  return g_strdup ("clay:offset");
}

static void
clay_offset_tool_button_press (ClayTool            *tool,
                               const ClayCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               ClayButtonPressType  press_type,
                               ClayDisplay         *display)
{
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (tool);

  offset_tool->dragging = ! clay_filter_tool_on_guide (CLAY_FILTER_TOOL (tool),
                                                       coords, display);

  if (! offset_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      offset_tool->x = coords->x;
      offset_tool->y = coords->y;

      g_object_get (CLAY_FILTER_TOOL (tool)->config,
                    "x", &offset_tool->offset_x,
                    "y", &offset_tool->offset_y,
                    NULL);

      tool->display = display;

      clay_tool_control_activate (tool->control);

      clay_tool_pop_status (tool, display);

      clay_tool_push_status_coords (tool, display,
                                    CLAY_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    0,
                                    ", ",
                                    0,
                                    NULL);
    }
}

static void
clay_offset_tool_button_release (ClayTool              *tool,
                                 const ClayCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 ClayButtonReleaseType  release_type,
                                 ClayDisplay           *display)
{
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      clay_tool_control_halt (tool->control);

      offset_tool->dragging = FALSE;

      if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
        {
          g_object_set (CLAY_FILTER_TOOL (tool)->config,
                        "x", offset_tool->offset_x,
                        "y", offset_tool->offset_y,
                        NULL);
        }
    }
}

static void
clay_offset_tool_motion (ClayTool         *tool,
                         const ClayCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         ClayDisplay      *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (tool);

  if (! offset_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      ClayOffsetType type;
      gint           offset_x;
      gint           offset_y;
      gint           x;
      gint           y;
      gint           width;
      gint           height;

      g_object_get (filter_tool->config,
                    "type", &type,
                    NULL);

      offset_x = RINT (coords->x - offset_tool->x);
      offset_y = RINT (coords->y - offset_tool->y);

      x = offset_tool->offset_x + offset_x;
      y = offset_tool->offset_y + offset_y;

      width  = clay_offset_tool_get_width  (offset_tool);
      height = clay_offset_tool_get_height (offset_tool);

      if (type == CLAY_OFFSET_WRAP_AROUND)
        {
          x %= MAX (width,  1);
          y %= MAX (height, 1);
        }
      else
        {
          x = CLAMP (x, -width,  +width);
          y = CLAMP (y, -height, +height);
        }

      g_object_set (filter_tool->config,
                    "x", x,
                    "y", y,
                    NULL);

      clay_tool_pop_status (tool, display);

      clay_tool_push_status_coords (tool, display,
                                    CLAY_CURSOR_PRECISION_PIXEL_CENTER,
                                    _("Offset: "),
                                    offset_x,
                                    ", ",
                                    offset_y,
                                    NULL);
    }
}

static void
clay_offset_tool_oper_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              ClayDisplay      *display)
{
  if (! tool->drawables ||
      clay_filter_tool_on_guide (CLAY_FILTER_TOOL (tool),
                                 coords, display))
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
  else
    {
      clay_tool_pop_status (tool, display);

      clay_tool_push_status (tool, display, "%s",
                             _("Click-Drag to offset drawable"));
    }
}

static void
clay_offset_tool_cursor_update (ClayTool         *tool,
                                const ClayCoords *coords,
                                GdkModifierType   state,
                                ClayDisplay      *display)
{
  if (! tool->drawables ||
      clay_filter_tool_on_guide (CLAY_FILTER_TOOL (tool),
                                 coords, display))
    {
      CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
  else
    {
      clay_tool_set_cursor (tool, display,
                            CLAY_CURSOR_MOUSE,
                            CLAY_TOOL_CURSOR_MOVE,
                            CLAY_CURSOR_MODIFIER_NONE);
    }
}

static void
clay_offset_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayOffsetTool *offset_tool = CLAY_OFFSET_TOOL (filter_tool);
  GtkWidget      *main_vbox;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *spinbutton;
  GtkWidget      *frame;
  GtkAdjustment  *adjustment;
  GeglColor      *color = NULL;
  gint            type  = 0;

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

  /*  The offset frame  */
  frame = clay_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = clay_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  offset_tool->offset_se = clay_size_entry_new (1, clay_unit_pixel (), "%a",
                                                TRUE, TRUE, FALSE, 10,
                                                CLAY_SIZE_ENTRY_UPDATE_SIZE);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (offset_tool->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (offset_tool->offset_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_set_visible (spinbutton, TRUE);

  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (offset_tool->offset_se),
                                _("_X:"), 0, 0, 0.0);
  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (offset_tool->offset_se),
                                _("_Y:"), 1, 0, 0.0);

  gtk_box_pack_start (GTK_BOX (vbox), offset_tool->offset_se, FALSE, FALSE, 0);
  gtk_widget_set_visible (offset_tool->offset_se, TRUE);

  clay_size_entry_set_unit (CLAY_SIZE_ENTRY (offset_tool->offset_se),
                            clay_unit_pixel ());

  g_signal_connect (offset_tool->offset_se, "refval-changed",
                    G_CALLBACK (clay_offset_tool_offset_changed),
                    offset_tool);
  g_signal_connect (offset_tool->offset_se, "value-changed",
                    G_CALLBACK (clay_offset_tool_offset_changed),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By width/_2, height/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_offset_tool_half_xy_clicked),
                    offset_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("By _width/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_offset_tool_half_x_clicked),
                    offset_tool);

  button = gtk_button_new_with_mnemonic (_("By _height/2"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_offset_tool_half_y_clicked),
                    offset_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The edge behavior frame  */
  frame = clay_int_radio_group_new (TRUE, _("Edge Behavior"),

                                    G_CALLBACK (clay_offset_tool_edge_behavior_toggled),
                                    offset_tool, NULL,

                                    CLAY_OFFSET_WRAP_AROUND,

                                    _("W_rap around"),
                                    CLAY_OFFSET_WRAP_AROUND, NULL,

                                    _("Fill with plain color"),
                                    CLAY_OFFSET_COLOR, NULL,

                                    _("Make _transparent"),
                                    CLAY_OFFSET_TRANSPARENT,
                                    &offset_tool->transparent_radio,
                                    NULL);

  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /* Color area */
  gegl_node_get (filter_tool->operation,
                 "color", &color,
                 "type",  &type,
                 NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_valign (vbox, GTK_ALIGN_END);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  offset_tool->color_button = clay_color_button_new (_("Fill Color"),
                                                     40, 24, color,
                                                     CLAY_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (vbox), offset_tool->color_button, TRUE, FALSE, 0);
  if (type == CLAY_OFFSET_COLOR)
    gtk_widget_set_visible (offset_tool->color_button, TRUE);

  g_signal_connect (CLAY_COLOR_BUTTON (offset_tool->color_button),
                    "color-changed",
                    G_CALLBACK (clay_offset_tool_color_changed),
                    offset_tool);
  g_object_unref (color);
}

static void
clay_offset_tool_config_notify (ClayFilterTool   *filter_tool,
                                ClayConfig       *config,
                                const GParamSpec *pspec)
{
  clay_offset_tool_update (CLAY_OFFSET_TOOL (filter_tool));

  CLAY_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);
}

static void
clay_offset_tool_region_changed (ClayFilterTool *filter_tool)
{
  clay_offset_tool_update (CLAY_OFFSET_TOOL (filter_tool));
}

static void
clay_offset_tool_offset_changed (ClaySizeEntry  *se,
                                 ClayOffsetTool *offset_tool)
{
  g_object_set (CLAY_FILTER_TOOL (offset_tool)->config,
                "x", (gint) clay_size_entry_get_refval (se, 0),
                "y", (gint) clay_size_entry_get_refval (se, 1),
                NULL);
}

static void
clay_offset_tool_half_xy_clicked (GtkButton      *button,
                                  ClayOffsetTool *offset_tool)
{
  g_object_set (CLAY_FILTER_TOOL (offset_tool)->config,
                "x", clay_offset_tool_get_width  (offset_tool) / 2,
                "y", clay_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
clay_offset_tool_half_x_clicked (GtkButton      *button,
                                 ClayOffsetTool *offset_tool)
{
  g_object_set (CLAY_FILTER_TOOL (offset_tool)->config,
                "x", clay_offset_tool_get_width (offset_tool) / 2,
                NULL);
}

static void
clay_offset_tool_half_y_clicked (GtkButton      *button,
                                 ClayOffsetTool *offset_tool)
{
  g_object_set (CLAY_FILTER_TOOL (offset_tool)->config,
                "y", clay_offset_tool_get_height (offset_tool) / 2,
                NULL);
}

static void
clay_offset_tool_edge_behavior_toggled (GtkToggleButton *toggle,
                                        ClayOffsetTool  *offset_tool)
{
  if (gtk_toggle_button_get_active (toggle))
    {
      ClayOffsetType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (toggle),
                                                 "clay-item-data"));

      g_object_set (CLAY_FILTER_TOOL (offset_tool)->config,
                    "type", type,
                    NULL);

      if (type == CLAY_OFFSET_COLOR)
        gtk_widget_set_visible (offset_tool->color_button, TRUE);
      else
        gtk_widget_set_visible (offset_tool->color_button, FALSE);
    }
}

static void
clay_offset_tool_color_changed (ClayColorButton *button,
                                ClayOffsetTool  *offset_tool)
{
  GeglColor      *color       = clay_color_button_get_color (button);
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (offset_tool);

  gegl_node_set (filter_tool->operation,
                 "color", color,
                 NULL);

  clay_drawable_filter_apply (filter_tool->filter, NULL);
  g_object_unref (color);
}

static gint
clay_offset_tool_get_width (ClayOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (clay_filter_tool_get_drawable_area (CLAY_FILTER_TOOL (offset_tool),
                                          &drawable_offset_x,
                                          &drawable_offset_y,
                                          &drawable_area) &&
      ! gegl_rectangle_is_empty (&drawable_area))
    {
      return drawable_area.width;
    }

  return 0;
}

static gint
clay_offset_tool_get_height (ClayOffsetTool *offset_tool)
{
  GeglRectangle drawable_area;
  gint          drawable_offset_x;
  gint          drawable_offset_y;

  if (clay_filter_tool_get_drawable_area (CLAY_FILTER_TOOL (offset_tool),
                                          &drawable_offset_x,
                                          &drawable_offset_y,
                                          &drawable_area) &&
      ! gegl_rectangle_is_empty (&drawable_area))
    {
      return drawable_area.height;
    }

  return 0;
}

static void
clay_offset_tool_update (ClayOffsetTool *offset_tool)
{
  ClayTool       *tool        = CLAY_TOOL (offset_tool);
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (offset_tool);
  ClayOffsetType  orig_type;
  gint            orig_x;
  gint            orig_y;
  ClayOffsetType  type;
  gint            x;
  gint            y;
  gint            width;
  gint            height;

  g_object_get (filter_tool->config,
                "type", &orig_type,
                "x",    &orig_x,
                "y",    &orig_y,
                NULL);

  width  = clay_offset_tool_get_width  (offset_tool);
  height = clay_offset_tool_get_height (offset_tool);

  x = CLAMP (orig_x, -width,  +width);
  y = CLAMP (orig_y, -height, +height);

  type = orig_type;

  if (tool->drawables                                   &&
      ! clay_drawable_has_alpha (tool->drawables->data) &&
      type == CLAY_OFFSET_TRANSPARENT)
    {
      type = CLAY_OFFSET_COLOR;
    }

  if (x    != orig_x ||
      y    != orig_y ||
      type != orig_type)
    {
      g_object_set (filter_tool->config,
                    "type", type,
                    "x",    x,
                    "y",    y,
                    NULL);
    }

  if (offset_tool->offset_se)
    {
      gint width  = clay_offset_tool_get_width  (offset_tool);
      gint height = clay_offset_tool_get_height (offset_tool);

      g_signal_handlers_block_by_func (offset_tool->offset_se,
                                       clay_offset_tool_offset_changed,
                                       offset_tool);

      clay_size_entry_set_refval_boundaries (
        CLAY_SIZE_ENTRY (offset_tool->offset_se), 0,
        -width, +width);
      clay_size_entry_set_refval_boundaries (
        CLAY_SIZE_ENTRY (offset_tool->offset_se), 1,
        -height, +height);

      clay_size_entry_set_size (
        CLAY_SIZE_ENTRY (offset_tool->offset_se), 0,
        0, width);
      clay_size_entry_set_size (
        CLAY_SIZE_ENTRY (offset_tool->offset_se), 1,
        0, height);

      clay_size_entry_set_refval (CLAY_SIZE_ENTRY (offset_tool->offset_se), 0,
                                  x);
      clay_size_entry_set_refval (CLAY_SIZE_ENTRY (offset_tool->offset_se), 1,
                                  y);

      g_signal_handlers_unblock_by_func (offset_tool->offset_se,
                                         clay_offset_tool_offset_changed,
                                         offset_tool);
    }

  if (offset_tool->transparent_radio)
    {
      clay_int_radio_group_set_active (
        GTK_RADIO_BUTTON (offset_tool->transparent_radio),
        type);
    }

  if (offset_tool->color_button)
    {
      GeglColor *color;

      gegl_node_get (filter_tool->operation, "color", &color, NULL);

      g_signal_handlers_block_by_func (offset_tool->color_button,
                                       clay_offset_tool_color_changed,
                                       offset_tool);
      clay_color_button_set_color (CLAY_COLOR_BUTTON (offset_tool->color_button),
                                   color);
      g_signal_handlers_unblock_by_func (offset_tool->color_button,
                                         clay_offset_tool_color_changed,
                                         offset_tool);

      g_object_unref (color);
    }

}

static void
clay_offset_tool_halt (ClayOffsetTool *offset_tool)
{
  offset_tool->offset_se         = NULL;
  offset_tool->transparent_radio = NULL;
  offset_tool->color_button      = NULL;
}
