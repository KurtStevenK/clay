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

#include <errno.h>

#include <glib/gstdio.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaycolor/claycolor-private.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "operations/claycurvesconfig.h"
#include "operations/clayoperationcurves.h"

#include "core/clay.h"
#include "core/claycurve.h"
#include "core/claycurve-map.h"
#include "core/claydrawable.h"
#include "core/claydrawable-histogram.h"
#include "core/clayerror.h"
#include "core/clayhistogram.h"
#include "core/clayimage.h"

#include "widgets/claycolorbar.h"
#include "widgets/claycurveview.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claysettingsbox.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"

#include "claycurvestool.h"
#include "clayhistogramoptions.h"

#include "clay-intl.h"


#define GRAPH_SIZE 256
#define BAR_SIZE    12
#define RADIUS       4


/*  local function prototypes  */

static gboolean    clay_curves_tool_initialize      (ClayTool             *tool,
                                                     ClayDisplay          *display,
                                                     GError              **error);
static void        clay_curves_tool_button_release  (ClayTool             *tool,
                                                     const ClayCoords     *coords,
                                                     guint32               time,
                                                     GdkModifierType       state,
                                                     ClayButtonReleaseType release_type,
                                                     ClayDisplay          *display);
static gboolean    clay_curves_tool_key_press       (ClayTool             *tool,
                                                     GdkEventKey          *kevent,
                                                     ClayDisplay          *display);
static void        clay_curves_tool_oper_update     (ClayTool             *tool,
                                                     const ClayCoords     *coords,
                                                     GdkModifierType       state,
                                                     gboolean              proximity,
                                                     ClayDisplay          *display);

static gchar     * clay_curves_tool_get_operation   (ClayFilterTool       *filter_tool,
                                                    gchar               **description);
static void        clay_curves_tool_dialog          (ClayFilterTool       *filter_tool);
static void        clay_curves_tool_reset           (ClayFilterTool       *filter_tool);
static void        clay_curves_tool_config_notify   (ClayFilterTool       *filter_tool,
                                                     ClayConfig           *config,
                                                     const GParamSpec     *pspec);
static gboolean    clay_curves_tool_settings_import (ClayFilterTool       *filter_tool,
                                                     GInputStream         *input,
                                                     GError              **error);
static gboolean    clay_curves_tool_settings_export (ClayFilterTool       *filter_tool,
                                                     GOutputStream        *output,
                                                     GError              **error);
static void        clay_curves_tool_color_picked    (ClayFilterTool       *filter_tool,
                                                     gpointer              identifier,
                                                     gdouble               x,
                                                     gdouble               y,
                                                     const Babl           *sample_format,
                                                     GeglColor            *color);

static void        clay_curves_tool_update_channel  (ClayCurvesTool       *tool);
static void        clay_curves_tool_update_point    (ClayCurvesTool       *tool);

static void        curves_curve_dirty_callback      (ClayCurve            *curve,
                                                     ClayCurvesTool       *tool);

static void        curves_channel_callback          (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);
static void        curves_channel_reset_callback    (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);

static gboolean    curves_menu_sensitivity          (gint                  value,
                                                     gpointer              data);

static void        curves_graph_selection_callback  (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);

static void        curves_point_coords_callback     (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);
static void        curves_point_type_callback       (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);

static void        curves_curve_type_callback       (GtkWidget            *widget,
                                                     ClayCurvesTool       *tool);

static GeglColor * curves_get_channel_color        (GtkWidget            *widget,
                                                    ClayHistogramChannel  channel);


G_DEFINE_TYPE (ClayCurvesTool, clay_curves_tool, CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_curves_tool_parent_class


/*  public functions  */

void
clay_curves_tool_register (ClayToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (CLAY_TYPE_CURVES_TOOL,
                CLAY_TYPE_HISTOGRAM_OPTIONS,
                clay_color_options_gui,
                0,
                "clay-curves-tool",
                _("Curves"),
                _("Adjust color curves"),
                N_("_Curves..."), NULL,
                NULL, CLAY_HELP_TOOL_CURVES,
                CLAY_ICON_TOOL_CURVES,
                data);
}


/*  private functions  */

static void
clay_curves_tool_class_init (ClayCurvesToolClass *klass)
{
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  tool_class->initialize             = clay_curves_tool_initialize;
  tool_class->button_release         = clay_curves_tool_button_release;
  tool_class->key_press              = clay_curves_tool_key_press;
  tool_class->oper_update            = clay_curves_tool_oper_update;

  filter_tool_class->get_operation   = clay_curves_tool_get_operation;
  filter_tool_class->dialog          = clay_curves_tool_dialog;
  filter_tool_class->reset           = clay_curves_tool_reset;
  filter_tool_class->config_notify   = clay_curves_tool_config_notify;
  filter_tool_class->settings_import = clay_curves_tool_settings_import;
  filter_tool_class->settings_export = clay_curves_tool_settings_export;
  filter_tool_class->color_picked    = clay_curves_tool_color_picked;
}

static void
clay_curves_tool_init (ClayCurvesTool *tool)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (tool->picked_color); i++)
    tool->picked_color[i] = -1.0;
}

static gboolean
clay_curves_tool_initialize (ClayTool     *tool,
                             ClayDisplay  *display,
                             GError      **error)
{
  ClayFilterTool       *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesTool       *c_tool      = CLAY_CURVES_TOOL (tool);
  ClayImage            *image       = clay_display_get_image (display);
  GList                *drawables;
  ClayDrawable         *drawable;
  ClayCurvesConfig     *config;
  ClayHistogram        *histogram;
  ClayHistogramChannel  channel;

  if (! CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = clay_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        clay_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        clay_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }
  drawable = drawables->data;
  g_list_free (drawables);

  config = CLAY_CURVES_CONFIG (filter_tool->config);

  histogram = clay_histogram_new (config->trc);
  g_object_unref (clay_drawable_calculate_histogram_async (drawable, histogram,
                                                           FALSE));
  clay_histogram_view_set_background (CLAY_HISTOGRAM_VIEW (c_tool->graph),
                                      histogram);
  g_object_unref (histogram);

  if (clay_drawable_get_component_type (drawable) == CLAY_COMPONENT_TYPE_U8)
    {
      c_tool->scale = 255.0;

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_input),  0);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_output), 0);

      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_input),  3);
      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_output), 3);
    }
  else
    {
      c_tool->scale = 100.0;

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_input),  2);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c_tool->point_output), 2);

      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_input),  6);
      gtk_entry_set_width_chars (GTK_ENTRY (c_tool->point_output), 6);
    }

  clay_curve_view_set_range_x (CLAY_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);
  clay_curve_view_set_range_y (CLAY_CURVE_VIEW (c_tool->graph),
                               0, c_tool->scale);

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (c_tool->point_output),
                             0, c_tool->scale);

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      g_signal_connect (config->curve[channel], "dirty",
                        G_CALLBACK (curves_curve_dirty_callback),
                        tool);
    }

  clay_curves_tool_update_point (c_tool);

  /*  always pick colors  */
  clay_filter_tool_enable_color_picking (filter_tool, NULL, FALSE);

  return TRUE;
}

static void
clay_curves_tool_button_release (ClayTool              *tool,
                                 const ClayCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 ClayButtonReleaseType  release_type,
                                 ClayDisplay           *display)
{
  ClayCurvesTool   *c_tool      = CLAY_CURVES_TOOL (tool);
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);

  if (state & clay_get_extend_selection_mask ())
    {
      ClayCurve *curve = config->curve[config->channel];
      gdouble    value = c_tool->picked_color[config->channel];
      gint       point;

      point = clay_curve_get_point_at (curve, value);

      if (point < 0)
        {
          ClayCurvePointType type = CLAY_CURVE_POINT_SMOOTH;

          point = clay_curve_view_get_selected (
            CLAY_CURVE_VIEW (c_tool->graph));

          if (point >= 0)
            type = clay_curve_get_point_type (curve, point);

          point = clay_curve_add_point (
            curve,
            value, clay_curve_map_value (curve, value));

          clay_curve_set_point_type (curve, point, type);
        }

      clay_curve_view_set_selected (CLAY_CURVE_VIEW (c_tool->graph), point);
    }
  else if (state & clay_get_toggle_behavior_mask ())
    {
      ClayHistogramChannel channel;
      ClayCurvePointType   type = CLAY_CURVE_POINT_SMOOTH;
      gint                 point;

      point = clay_curve_view_get_selected (CLAY_CURVE_VIEW (c_tool->graph));

      if (point >= 0)
        {
          type = clay_curve_get_point_type (config->curve[config->channel],
                                            point);
        }

      for (channel = CLAY_HISTOGRAM_VALUE;
           channel <= CLAY_HISTOGRAM_ALPHA;
           channel++)
        {
          ClayCurve *curve = config->curve[channel];
          gdouble    value = c_tool->picked_color[channel];

          if (value != -1)
            {
              point = clay_curve_get_point_at (curve, value);

              if (point < 0)
                {
                  point = clay_curve_add_point (
                    curve,
                    value, clay_curve_map_value (curve, value));

                  clay_curve_set_point_type (curve, point, type);
                }

              if (channel == config->channel)
                {
                  clay_curve_view_set_selected (CLAY_CURVE_VIEW (c_tool->graph),
                                                point);
                }
            }
        }
    }

  /*  chain up to halt the tool */
  CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static gboolean
clay_curves_tool_key_press (ClayTool    *tool,
                            GdkEventKey *kevent,
                            ClayDisplay *display)
{
  ClayCurvesTool *c_tool = CLAY_CURVES_TOOL (tool);

  if (tool->display && c_tool->graph)
    {
      if (gtk_widget_event (c_tool->graph, (GdkEvent *) kevent))
        return TRUE;
    }

  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
clay_curves_tool_oper_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              ClayDisplay      *display)
{
  if (clay_filter_tool_on_guide (CLAY_FILTER_TOOL (tool),
                                 coords, display))
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                                   display);
    }
  else
    {
      ClayColorPickTarget  target;
      gchar               *status      = NULL;
      GdkModifierType      extend_mask = clay_get_extend_selection_mask ();
      GdkModifierType      toggle_mask = clay_get_toggle_behavior_mask ();

      clay_tool_pop_status (tool, display);

      if (state & extend_mask)
        {
          target = CLAY_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add a control point"));
        }
      else if (state & toggle_mask)
        {
          target = CLAY_COLOR_PICK_TARGET_PALETTE;
          status = g_strdup (_("Click to add control points to all channels"));
        }
      else
        {
          target = CLAY_COLOR_PICK_TARGET_NONE;
          status = clay_suggest_modifiers (_("Click to locate on curve"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: add control point"),
                                           _("%s: add control points to all channels"),
                                           NULL);
        }

      CLAY_COLOR_TOOL (tool)->pick_target = target;

      if (proximity)
        clay_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
}

static gchar *
clay_curves_tool_get_operation (ClayFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Curves"));

  return g_strdup ("clay:curves");
}


/*******************/
/*  Curves dialog  */
/*******************/

static void
clay_curves_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayCurvesTool   *tool         = CLAY_CURVES_TOOL (filter_tool);
  ClayToolOptions  *tool_options = CLAY_TOOL_GET_OPTIONS (filter_tool);
  ClayCurvesConfig *config       = CLAY_CURVES_CONFIG (filter_tool->config);
  GtkListStore     *store;
  GtkWidget        *main_vbox;
  GtkWidget        *frame_vbox;
  GtkWidget        *vbox;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *main_frame;
  GtkWidget        *frame;
  GtkWidget        *grid;
  GtkWidget        *button;
  GtkWidget        *bar;
  GtkWidget        *combo;

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

  clay_settings_box_show_legacy (CLAY_SETTINGS_BOX (filter_tool->settings_box));

  /*  The combo box for selecting channels  */
  main_frame = clay_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_frame, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_frame_set_label_widget (GTK_FRAME (main_frame), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  store = clay_enum_store_new_with_range (CLAY_TYPE_HISTOGRAM_CHANNEL,
                                          CLAY_HISTOGRAM_VALUE,
                                          CLAY_HISTOGRAM_ALPHA);
  g_set_weak_pointer (&tool->channel_menu,
                      clay_enum_combo_box_new_with_model (CLAY_ENUM_STORE (store)));
  g_object_unref (store);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (tool->channel_menu),
                                       "clay-channel");
  clay_int_combo_box_set_sensitivity (CLAY_INT_COMBO_BOX (tool->channel_menu),
                                      curves_menu_sensitivity, filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tool->channel_menu, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->channel_menu, TRUE);

  g_signal_connect (tool->channel_menu, "changed",
                    G_CALLBACK (curves_channel_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->channel_menu);

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (curves_channel_reset_callback),
                    tool);

  /*  The histogram scale radio buttons  */
  hbox2 = clay_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "clay-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  /*  The linear/perceptual radio buttons  */
  hbox2 = clay_prop_enum_icon_box_new (G_OBJECT (config), "trc",
                                       "clay-color-space",
                                       -1, -1);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_set_visible (frame_vbox, TRUE);

  /*  The grid for the color bars and the graph  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (frame_vbox), grid, TRUE, TRUE, 0);

  /*  The left color bar  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_grid_attach (GTK_GRID (grid), vbox, 0, 0, 1, 1);
  gtk_widget_set_visible (vbox, TRUE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, RADIUS);
  gtk_widget_set_visible (frame, TRUE);

  tool->yrange = clay_color_bar_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request (tool->yrange, BAR_SIZE, -1);
  gtk_container_add (GTK_CONTAINER (frame), tool->yrange);
  gtk_widget_set_visible (tool->yrange, TRUE);

  /*  The curves graph  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_widget_set_vexpand (frame, TRUE);
  gtk_grid_attach (GTK_GRID (grid), frame, 1, 0, 1, 1);
  gtk_widget_set_visible (frame, TRUE);

  g_set_weak_pointer (&tool->graph, clay_curve_view_new ());

  clay_curve_view_set_range_x (CLAY_CURVE_VIEW (tool->graph), 0, 255);
  clay_curve_view_set_range_y (CLAY_CURVE_VIEW (tool->graph), 0, 255);
  gtk_widget_set_size_request (tool->graph,
                               GRAPH_SIZE + RADIUS * 2,
                               GRAPH_SIZE + RADIUS * 2);
  g_object_set (tool->graph,
                "border-width", RADIUS,
                "subdivisions", 1,
                NULL);
  gtk_container_add (GTK_CONTAINER (frame), tool->graph);
  gtk_widget_set_visible (tool->graph, TRUE);

  g_object_bind_property (G_OBJECT (tool_options), "histogram-scale",
                          G_OBJECT (tool->graph),  "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_signal_connect (tool->graph, "selection-changed",
                    G_CALLBACK (curves_graph_selection_callback),
                    tool);

  /*  The bottom color bar  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox2, 1, 1, 1, 1);
  gtk_widget_set_visible (hbox2, TRUE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, TRUE, TRUE, RADIUS);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  tool->xrange = clay_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (tool->xrange, -1, BAR_SIZE / 2);
  gtk_box_pack_start (GTK_BOX (vbox), tool->xrange, TRUE, TRUE, 0);
  gtk_widget_set_visible (tool->xrange, TRUE);

  bar = clay_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), bar, TRUE, TRUE, 0);
  gtk_widget_set_visible (bar, TRUE);

  gtk_widget_set_visible (grid, TRUE);

  /*  The point properties box  */
  tool->point_box = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->point_box, TRUE);

  label = gtk_label_new_with_mnemonic (_("_Input:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  tool->point_input = clay_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_input, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->point_input, TRUE);

  g_signal_connect (tool->point_input, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_input);

  label = gtk_label_new_with_mnemonic (_("O_utput:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  tool->point_output = clay_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), tool->point_output, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->point_output, TRUE);

  g_signal_connect (tool->point_output, "value-changed",
                    G_CALLBACK (curves_point_coords_callback),
                    tool);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_output);

  label = gtk_label_new_with_mnemonic (_("T_ype:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  hbox2 = clay_enum_icon_box_new (CLAY_TYPE_CURVE_POINT_TYPE,
                                  "clay-curve-point",
                                  GTK_ICON_SIZE_MENU,
                                  G_CALLBACK (curves_point_type_callback),
                                  tool, NULL,
                                  &tool->point_type);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox2, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->point_type);

  label = gtk_label_new_with_mnemonic (_("Curve _type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  /*  The curve-type combo  */
  tool->curve_type = combo = clay_enum_combo_box_new (CLAY_TYPE_CURVE_TYPE);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (combo),
                                       "clay-curve");
  clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo), 0,
                              G_CALLBACK (curves_curve_type_callback),
                              tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_set_visible (combo, TRUE);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  clay_curves_tool_update_channel (tool);
}

static void
clay_curves_tool_reset (ClayFilterTool *filter_tool)
{
  ClayHistogramChannel channel;

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  CLAY_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  g_object_set (filter_tool->config,
                "channel", channel,
                NULL);
}

static void
clay_curves_tool_config_notify (ClayFilterTool   *filter_tool,
                                ClayConfig       *config,
                                const GParamSpec *pspec)
{
  ClayCurvesTool   *curves_tool   = CLAY_CURVES_TOOL (filter_tool);
  ClayCurvesConfig *curves_config = CLAY_CURVES_CONFIG (config);
  ClayCurve        *curve         = curves_config->curve[curves_config->channel];

  CLAY_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! curves_tool->channel_menu ||
      ! curves_tool->graph)
    return;

  if (! strcmp (pspec->name, "trc"))
    {
      ClayHistogram *histogram;

      histogram = clay_histogram_new (curves_config->trc);
      g_object_unref (clay_drawable_calculate_histogram_async
                      (CLAY_TOOL (filter_tool)->drawables->data, histogram, FALSE));
      clay_histogram_view_set_background (CLAY_HISTOGRAM_VIEW (curves_tool->graph),
                                          histogram);
      g_object_unref (histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      clay_curves_tool_update_channel (CLAY_CURVES_TOOL (filter_tool));
    }
  else if (! strcmp (pspec->name, "curve"))
    {
      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (curves_tool->curve_type),
                                     curve->curve_type);
    }
}

static gboolean
clay_curves_tool_settings_import (ClayFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  ClayCurvesConfig *config = CLAY_CURVES_CONFIG (filter_tool->config);
  gchar             header[64];
  gsize             bytes_read;

  if (! g_input_stream_read_all (input, header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      /* A Photoshop .acv curves preset file might be smaller than
       * 64 bytes, so we'll check if that's the case */
      if (bytes_read > 2 && header[0] == 0 &&
          (header[1] == 1 || header[1] == 4))
        {
          g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

          return clay_curves_config_load_acv (config, input, error);
        }

      if (error && *error)
        g_prefix_error (error, _("Could not read header: "));
      else
        g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                     _("Could not read header: "));
      return FALSE;
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  if (g_str_has_prefix (header, "# Clay Curves File\n"))
    return clay_curves_config_load_cruft (config, input, error);

  if (bytes_read > 2 && header[0] == 0 &&
      (header[1] == 1 || header[1] == 4))
    return clay_curves_config_load_acv (config, input, error);

  return CLAY_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
clay_curves_tool_settings_export (ClayFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  ClayCurvesConfig *config   = CLAY_CURVES_CONFIG (filter_tool->config);
  ClaySettingsBox  *settings = CLAY_SETTINGS_BOX (filter_tool->settings_box);

  if (clay_settings_box_get_legacy (settings))
    return clay_curves_config_save_cruft (config, output, error);

  return CLAY_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
}

static void
clay_curves_tool_color_picked (ClayFilterTool *filter_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               GeglColor      *color)
{
  ClayCurvesTool   *tool     = CLAY_CURVES_TOOL (filter_tool);
  ClayCurvesConfig *config   = CLAY_CURVES_CONFIG (filter_tool->config);
  ClayDrawable     *drawable = CLAY_TOOL (tool)->drawables->data;
  gdouble           rgb[4];

  /* TODO: double-check this code. What RGB values do we want exactly? Right
   * now, we use sRGB space values.
   */
  if (config->trc == CLAY_TRC_LINEAR)
    gegl_color_get_pixel (color, babl_format ("RGBA double"), rgb);
  else
    gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), rgb);

  tool->picked_color[CLAY_HISTOGRAM_RED]   = rgb[0];
  tool->picked_color[CLAY_HISTOGRAM_GREEN] = rgb[1];
  tool->picked_color[CLAY_HISTOGRAM_BLUE]  = rgb[2];

  if (clay_drawable_has_alpha (drawable))
    tool->picked_color[CLAY_HISTOGRAM_ALPHA] = rgb[3];
  else
    tool->picked_color[CLAY_HISTOGRAM_ALPHA] = -1;

  tool->picked_color[CLAY_HISTOGRAM_VALUE] = MAX (MAX (rgb[0], rgb[1]), rgb[2]);

  clay_curve_view_set_xpos (CLAY_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);
}

static void
clay_curves_tool_update_channel (ClayCurvesTool *tool)
{
  ClayFilterTool       *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig     *config      = CLAY_CURVES_CONFIG (filter_tool->config);
  ClayCurve            *curve       = config->curve[config->channel];
  ClayHistogramChannel  channel;

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);

  switch (config->channel)
    {
      guchar r[256];
      guchar g[256];
      guchar b[256];

    case CLAY_HISTOGRAM_VALUE:
    case CLAY_HISTOGRAM_ALPHA:
    case CLAY_HISTOGRAM_RGB:
    case CLAY_HISTOGRAM_LUMINANCE:
      clay_curve_get_uchar (curve, sizeof (r), r);

      clay_color_bar_set_buffers (CLAY_COLOR_BAR (tool->xrange),
                                  r, r, r);
      break;

    case CLAY_HISTOGRAM_RED:
    case CLAY_HISTOGRAM_GREEN:
    case CLAY_HISTOGRAM_BLUE:
      clay_curve_get_uchar (config->curve[CLAY_HISTOGRAM_RED],
                            sizeof (r), r);
      clay_curve_get_uchar (config->curve[CLAY_HISTOGRAM_GREEN],
                            sizeof (g), g);
      clay_curve_get_uchar (config->curve[CLAY_HISTOGRAM_BLUE],
                            sizeof (b), b);

      clay_color_bar_set_buffers (CLAY_COLOR_BAR (tool->xrange),
                                  r, g, b);
      break;
    }

  clay_histogram_view_set_channel (CLAY_HISTOGRAM_VIEW (tool->graph),
                                   config->channel);
  clay_curve_view_set_xpos (CLAY_CURVE_VIEW (tool->graph),
                            tool->picked_color[config->channel]);

  clay_color_bar_set_channel (CLAY_COLOR_BAR (tool->yrange),
                              config->channel);

  clay_curve_view_remove_all_backgrounds (CLAY_CURVE_VIEW (tool->graph));

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      GeglColor *curve_color;

      curve_color = curves_get_channel_color (tool->graph, channel);

      if (channel == config->channel)
        clay_curve_view_set_curve (CLAY_CURVE_VIEW (tool->graph), curve, curve_color);
      else
        clay_curve_view_add_background (CLAY_CURVE_VIEW (tool->graph),
                                        config->curve[channel], curve_color);

      g_clear_object (&curve_color);
    }

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (tool->curve_type),
                                 curve->curve_type);

  clay_curves_tool_update_point (tool);
}

static void
clay_curves_tool_update_point (ClayCurvesTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);
  ClayCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = clay_curve_view_get_selected (CLAY_CURVE_VIEW (tool->graph));

  gtk_widget_set_sensitive (tool->point_box, point >= 0);

  if (point >= 0)
    {
      gdouble min = 0.0;
      gdouble max = 1.0;
      gdouble x;
      gdouble y;

      if (point > 0)
        clay_curve_get_point (curve, point - 1, &min, NULL);

      if (point < clay_curve_get_n_points (curve) - 1)
        clay_curve_get_point (curve, point + 1, &max, NULL);

      clay_curve_get_point (curve, point, &x, &y);

      x   *= tool->scale;
      y   *= tool->scale;
      min *= tool->scale;
      max *= tool->scale;

      g_signal_handlers_block_by_func (tool->point_input,
                                       curves_point_coords_callback,
                                       tool);
      g_signal_handlers_block_by_func (tool->point_output,
                                       curves_point_coords_callback,
                                       tool);

      gtk_spin_button_set_range (GTK_SPIN_BUTTON (tool->point_input), min, max);

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->point_input),  x);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool->point_output), y);


      g_signal_handlers_unblock_by_func (tool->point_input,
                                         curves_point_coords_callback,
                                         tool);
      g_signal_handlers_unblock_by_func (tool->point_output,
                                         curves_point_coords_callback,
                                         tool);

      g_signal_handlers_block_by_func (tool->point_type,
                                       curves_point_type_callback,
                                       tool);

      clay_int_radio_group_set_active (
        GTK_RADIO_BUTTON (tool->point_type),
        clay_curve_get_point_type (curve, point));

      g_signal_handlers_unblock_by_func (tool->point_type,
                                         curves_point_type_callback,
                                         tool);
    }
}

static void
curves_curve_dirty_callback (ClayCurve      *curve,
                             ClayCurvesTool *tool)
{
  if (tool->graph &&
      clay_curve_view_get_curve (CLAY_CURVE_VIEW (tool->graph)) == curve)
    {
      clay_curves_tool_update_point (tool);
    }
}

static void
curves_channel_callback (GtkWidget      *widget,
                         ClayCurvesTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);
  gint              value;

  if (clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (widget), &value) &&
      config->channel != value)
    {
      g_object_set (config,
                    "channel", value,
                    NULL);
    }
}

static void
curves_channel_reset_callback (GtkWidget      *widget,
                               ClayCurvesTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);

  clay_curve_reset (config->curve[config->channel], FALSE);
}

static gboolean
curves_menu_sensitivity (gint      value,
                         gpointer  data)
{
  ClayDrawable         *drawable = CLAY_TOOL (data)->drawables->data;
  ClayHistogramChannel  channel  = value;

  if (!drawable)
    return FALSE;

  switch (channel)
    {
    case CLAY_HISTOGRAM_VALUE:
      return TRUE;

    case CLAY_HISTOGRAM_RED:
    case CLAY_HISTOGRAM_GREEN:
    case CLAY_HISTOGRAM_BLUE:
      return clay_drawable_is_rgb (drawable);

    case CLAY_HISTOGRAM_ALPHA:
      return clay_drawable_has_alpha (drawable);

    case CLAY_HISTOGRAM_RGB:
      return FALSE;

    case CLAY_HISTOGRAM_LUMINANCE:
      return FALSE;
    }

  return FALSE;
}

static void
curves_graph_selection_callback (GtkWidget      *widget,
                                 ClayCurvesTool *tool)
{
  clay_curves_tool_update_point (tool);
}

static void
curves_point_coords_callback (GtkWidget      *widget,
                              ClayCurvesTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);
  ClayCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = clay_curve_view_get_selected (CLAY_CURVE_VIEW (tool->graph));

  if (point >= 0)
    {
      gdouble x;
      gdouble y;

      x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_input));
      y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (tool->point_output));

      x /= tool->scale;
      y /= tool->scale;

      clay_curve_set_point (curve, point, x, y);
    }
}

static void
curves_point_type_callback (GtkWidget      *widget,
                            ClayCurvesTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);
  ClayCurve        *curve       = config->curve[config->channel];
  gint              point;

  point = clay_curve_view_get_selected (CLAY_CURVE_VIEW (tool->graph));

  if (point >= 0 && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      ClayCurvePointType type;

      type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                 "clay-item-data"));

      clay_curve_set_point_type (curve, point, type);
    }
}

static void
curves_curve_type_callback (GtkWidget      *widget,
                            ClayCurvesTool *tool)
{
  gint value;

  if (clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (widget), &value))
    {
      ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
      ClayCurvesConfig *config      = CLAY_CURVES_CONFIG (filter_tool->config);
      ClayCurveType     curve_type  = value;

      if (config->curve[config->channel]->curve_type != curve_type)
        clay_curve_set_curve_type (config->curve[config->channel], curve_type);
    }
}

static GeglColor *
curves_get_channel_color (GtkWidget            *widget,
                          ClayHistogramChannel  channel)
{
  static const gdouble channel_colors[CLAY_HISTOGRAM_RGB][4] =
  {
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.5, 0.5, 0.5, 1.0 }
  };

  GeglColor *color;
  GdkRGBA    rgba;

  if (channel == CLAY_HISTOGRAM_VALUE)
    return NULL;

  color = gegl_color_new (NULL);
  if (channel == CLAY_HISTOGRAM_ALPHA)
    {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      gdouble          lum;
      gdouble          rgb[4];

      gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                                   &rgba);

      lum = CLAY_RGB_LUMINANCE (rgba.red, rgba.green, rgba.blue);

      rgb[3] = 1.0;
      if (lum > 0.5)
        rgb[0] = rgb[1] = rgb[2] = lum - 0.3;
      else
        rgb[0] = rgb[1] = rgb[2] = lum + 0.3;

      gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), rgb);

      return color;
    }

  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), channel_colors[channel]);

  return color;
}
