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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "gegl/clay-babl.h"

#include "operations/claylevelsconfig.h"
#include "operations/clayoperationlevels.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-gui.h"
#include "core/clayasync.h"
#include "core/claycancelable.h"
#include "core/claydata.h"
#include "core/claydrawable.h"
#include "core/claydrawable-histogram.h"
#include "core/clayerror.h"
#include "core/clayhistogram.h"
#include "core/clayimage.h"
#include "core/claytoolinfo.h"
#include "core/claytriviallycancelablewaitable.h"
#include "core/claywaitable.h"

#include "widgets/claycolorbar.h"
#include "widgets/clayhandlebar.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayhistogramview.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claysettingsbox.h"
#include "widgets/claywidgets-constructors.h"

#include "display/claydisplay.h"

#include "clayhistogramoptions.h"
#include "claylevelstool.h"

#include "clay-intl.h"


#define PICK_LOW_INPUT    (1 << 0)
#define PICK_GAMMA        (1 << 1)
#define PICK_HIGH_INPUT   (1 << 2)
#define PICK_ALL_CHANNELS (1 << 8)

#define HISTOGRAM_WIDTH    256
#define GRADIENT_HEIGHT     18
#define CONTROL_HEIGHT      16


/*  local function prototypes  */

static void       clay_levels_tool_finalize       (GObject          *object);

static gboolean   clay_levels_tool_initialize     (ClayTool         *tool,
                                                   ClayDisplay      *display,
                                                   GError          **error);

static gchar    * clay_levels_tool_get_operation  (ClayFilterTool   *filter_tool,
                                                   gchar           **description);
static void       clay_levels_tool_dialog         (ClayFilterTool   *filter_tool);
static void       clay_levels_tool_reset          (ClayFilterTool   *filter_tool);
static void       clay_levels_tool_config_notify  (ClayFilterTool   *filter_tool,
                                                   ClayConfig       *config,
                                                   const GParamSpec *pspec);
static void       clay_levels_tool_style_updated  (ClayGuiConfig    *config,
                                                   GParamSpec       *pspec,
                                                   ClayLevelsTool   *tool);
static gboolean   clay_levels_tool_settings_import(ClayFilterTool   *filter_tool,
                                                   GInputStream     *input,
                                                   GError          **error);
static gboolean   clay_levels_tool_settings_export(ClayFilterTool   *filter_tool,
                                                   GOutputStream    *output,
                                                   GError          **error);
static void       clay_levels_tool_color_picked   (ClayFilterTool   *filter_tool,
                                                   gpointer          identifier,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   const Babl       *sample_format,
                                                   GeglColor        *color);

static void       levels_update_input_bar         (ClayLevelsTool   *tool);

static void       levels_channel_callback         (GtkWidget        *widget,
                                                   ClayFilterTool   *filter_tool);
static void       levels_channel_reset_callback   (GtkWidget        *widget,
                                                   ClayFilterTool   *filter_tool);

static gboolean   levels_menu_sensitivity         (gint              value,
                                                   gpointer          data);

static void       levels_stretch_callback         (GtkWidget        *widget,
                                                   ClayLevelsTool   *tool);
static void       levels_linear_gamma_changed     (GtkAdjustment    *adjustment,
                                                   ClayLevelsTool   *tool);

static void       levels_to_curves_callback       (GtkWidget        *widget,
                                                   ClayFilterTool   *filter_tool);


G_DEFINE_TYPE (ClayLevelsTool, clay_levels_tool, CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_levels_tool_parent_class


void
clay_levels_tool_register (ClayToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (CLAY_TYPE_LEVELS_TOOL,
                CLAY_TYPE_HISTOGRAM_OPTIONS,
                clay_color_options_gui,
                0,
                "clay-levels-tool",
                _("Levels"),
                _("Adjust color levels"),
                N_("_Levels..."), NULL,
                NULL, CLAY_HELP_TOOL_LEVELS,
                CLAY_ICON_TOOL_LEVELS,
                data);
}

static void
clay_levels_tool_class_init (ClayLevelsToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  object_class->finalize             = clay_levels_tool_finalize;

  tool_class->initialize             = clay_levels_tool_initialize;

  filter_tool_class->get_operation   = clay_levels_tool_get_operation;
  filter_tool_class->dialog          = clay_levels_tool_dialog;
  filter_tool_class->reset           = clay_levels_tool_reset;
  filter_tool_class->config_notify   = clay_levels_tool_config_notify;
  filter_tool_class->settings_import = clay_levels_tool_settings_import;
  filter_tool_class->settings_export = clay_levels_tool_settings_export;
  filter_tool_class->color_picked    = clay_levels_tool_color_picked;
}

static void
clay_levels_tool_init (ClayLevelsTool *tool)
{
}

static void
clay_levels_tool_finalize (GObject *object)
{
  ClayLevelsTool *tool = CLAY_LEVELS_TOOL (object);

  g_clear_object (&tool->histogram);
  g_clear_object (&tool->histogram_async);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_levels_tool_initialize (ClayTool     *tool,
                             ClayDisplay  *display,
                             GError      **error)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayLevelsTool   *l_tool      = CLAY_LEVELS_TOOL (tool);
  ClayImage        *image       = clay_display_get_image (display);
  GList            *drawables;
  ClayDrawable     *drawable;
  ClayLevelsConfig *config;
  gdouble           scale_factor;
  gdouble           step_increment;
  gdouble           page_increment;
  gint              digits;

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

  config = CLAY_LEVELS_CONFIG (filter_tool->config);

  g_clear_object (&l_tool->histogram);
  g_clear_object (&l_tool->histogram_async);
  l_tool->histogram = clay_histogram_new (config->trc);

  l_tool->histogram_async = clay_drawable_calculate_histogram_async
    (drawable, l_tool->histogram, FALSE);
  clay_histogram_view_set_histogram (CLAY_HISTOGRAM_VIEW (l_tool->histogram_view),
                                     l_tool->histogram);

  if (clay_drawable_get_component_type (drawable) == CLAY_COMPONENT_TYPE_U8)
    {
      scale_factor   = 255.0;
      step_increment = 1.0;
      page_increment = 8.0;
      digits         = 0;
    }
  else
    {
      scale_factor   = 100;
      step_increment = 0.01;
      page_increment = 1.0;
      digits         = 2;
    }

  clay_prop_widget_set_factor (l_tool->low_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  clay_prop_widget_set_factor (l_tool->high_input_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  clay_prop_widget_set_factor (l_tool->low_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);
  clay_prop_widget_set_factor (l_tool->high_output_spinbutton,
                               scale_factor, step_increment, page_increment,
                               digits);

  gtk_adjustment_configure (l_tool->gamma_linear,
                            scale_factor / 2.0,
                            0, scale_factor, 0.1, 1.0, 0);

  return TRUE;
}

static gchar *
clay_levels_tool_get_operation (ClayFilterTool  *filter_tool,
                                gchar          **description)
{
  *description = g_strdup (_("Adjust Color Levels"));

  return g_strdup ("clay:levels");
}


/*******************/
/*  Levels dialog  */
/*******************/

static GtkWidget *
clay_levels_tool_color_picker_new (ClayLevelsTool *tool,
                                   guint           value)
{
  const gchar *icon_name;
  const gchar *help;
  gboolean     all_channels = (value & PICK_ALL_CHANNELS) != 0;

  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      icon_name = CLAY_ICON_COLOR_PICKER_BLACK;

      if (all_channels)
        help = _("Pick black point for all channels");
      else
        help = _("Pick black point for the selected channel");
      break;

    case PICK_GAMMA:
      icon_name = CLAY_ICON_COLOR_PICKER_GRAY;

      if (all_channels)
        help = _("Pick gray point for all channels");
      else
        help = _("Pick gray point for the selected channel");
      break;

    case PICK_HIGH_INPUT:
      icon_name = CLAY_ICON_COLOR_PICKER_WHITE;

      if (all_channels)
        help = _("Pick white point for all channels");
      else
        help = _("Pick white point for the selected channel");
      break;

    default:
      return NULL;
    }

  return clay_filter_tool_add_color_picker (CLAY_FILTER_TOOL (tool),
                                            GUINT_TO_POINTER (value),
                                            icon_name,
                                            help,
                                            /* pick_abyss = */ FALSE,
                                            NULL, NULL);
}

static void
clay_levels_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayLevelsTool   *tool         = CLAY_LEVELS_TOOL (filter_tool);
  ClayToolOptions  *tool_options = CLAY_TOOL_GET_OPTIONS (filter_tool);
  ClayContext      *context      = CLAY_CONTEXT (tool_options);
  ClayLevelsConfig *config       = CLAY_LEVELS_CONFIG (filter_tool->config);
  ClayGuiConfig    *gui_config   = CLAY_GUI_CONFIG (context->clay->config);
  GtkListStore     *store;
  GtkWidget        *main_vbox;
  GtkWidget        *frame_vbox;
  GtkWidget        *vbox;
  GtkWidget        *vbox2;
  GtkWidget        *vbox3;
  GtkWidget        *hbox;
  GtkWidget        *hbox2;
  GtkWidget        *label;
  GtkWidget        *main_frame;
  GtkWidget        *frame;
  GtkWidget        *button;
  GtkWidget        *spinbutton;
  GtkAdjustment    *adjustment;
  GtkWidget        *bar;
  gint              border;

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

  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (tool->channel_menu),
                                       "clay-channel");
  clay_int_combo_box_set_sensitivity (CLAY_INT_COMBO_BOX (tool->channel_menu),
                                      levels_menu_sensitivity, filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tool->channel_menu, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->channel_menu, TRUE);

  g_signal_connect_object (tool->channel_menu, "changed",
                           G_CALLBACK (levels_channel_callback),
                           tool, 0);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), tool->channel_menu);

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (levels_channel_reset_callback),
                           tool, 0);

  /*  The histogram scale radio buttons  */
  hbox2 = clay_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "clay-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  /*  The linear/perceptual radio buttons  */
  /* XXX: override the default to Perceptual for the time being, until
   * we are able to change "clay:levels" default for "trc" directly.
   * See #15962.
   */
  g_object_set (config,
                "trc", CLAY_TRC_PERCEPTUAL,
                NULL);
  hbox2 = clay_prop_enum_icon_box_new (G_OBJECT (config), "trc",
                                       "clay-color-space",
                                       -1, -1);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_set_visible (frame_vbox, TRUE);

  /*  Input levels frame  */
  frame = clay_frame_new (_("Input Levels"));
  gtk_box_pack_start (GTK_BOX (frame_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  g_set_weak_pointer (&tool->histogram_view,
                      clay_histogram_view_new (FALSE));

  gtk_box_pack_start (GTK_BOX (vbox2), tool->histogram_view, TRUE, TRUE, 0);
  gtk_widget_set_visible (GTK_WIDGET (tool->histogram_view), TRUE);

  g_object_bind_property (G_OBJECT (tool_options),         "histogram-scale",
                          G_OBJECT (tool->histogram_view), "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_object_get (tool->histogram_view, "border-width", &border, NULL);

  vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), border);
  gtk_box_pack_start (GTK_BOX (vbox2), vbox3, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox3, TRUE);

  tool->input_bar = g_object_new (CLAY_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->input_bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), tool->input_bar, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->input_bar, TRUE);

  bar = g_object_new (CLAY_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (bar, -1, GRADIENT_HEIGHT / 2);
  gtk_box_pack_start (GTK_BOX (vbox3), bar, FALSE, FALSE, 0);
  gtk_widget_set_visible (bar, TRUE);

  tool->input_handle_bar = g_object_new (CLAY_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (tool->input_handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox3), tool->input_handle_bar, FALSE, FALSE,
                      0);
  gtk_widget_set_visible (tool->input_handle_bar, TRUE);

  clay_handle_bar_connect_events (CLAY_HANDLE_BAR (tool->input_handle_bar),
                                  tool->input_bar);
  clay_handle_bar_connect_events (CLAY_HANDLE_BAR (tool->input_handle_bar),
                                  bar);

  /*  Horizontal box for input levels spinbuttons  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  low input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox2, TRUE);

  button = clay_levels_tool_color_picker_new (tool, PICK_LOW_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  tool->low_input_spinbutton = spinbutton =
    clay_prop_spin_button_new (filter_tool->config, "low-input",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);

  tool->low_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  clay_handle_bar_set_adjustment (CLAY_HANDLE_BAR (tool->input_handle_bar), 0,
                                  tool->low_input);

  /*  clamp input toggle  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);
  gtk_widget_set_visible (hbox2, TRUE);

  button = clay_prop_check_button_new (filter_tool->config, "clamp-input",
                                       _("Clamp _input"));
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

  /*  input gamma spin  */
  spinbutton = clay_prop_spin_button_new (filter_tool->config, "gamma",
                                          0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  clay_help_set_help_data (spinbutton, _("Gamma"), NULL);

  tool->gamma = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

  tool->gamma_linear = gtk_adjustment_new (127, 0, 255, 0.1, 1.0, 0.0);
  g_signal_connect_object (tool->gamma_linear, "value-changed",
                           G_CALLBACK (levels_linear_gamma_changed),
                           tool, 0);

  clay_handle_bar_set_adjustment (CLAY_HANDLE_BAR (tool->input_handle_bar), 1,
                                  tool->gamma_linear);
  g_object_unref (tool->gamma_linear);

  /*  high input spin  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox2, TRUE);

  button = clay_levels_tool_color_picker_new (tool, PICK_HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  spinbutton = clay_prop_spin_button_new (filter_tool->config, "high-input",
                                          0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  tool->high_input_spinbutton = spinbutton;

  tool->high_input = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  clay_handle_bar_set_adjustment (CLAY_HANDLE_BAR (tool->input_handle_bar), 2,
                                  tool->high_input);

  /*  Output levels frame  */
  frame = clay_frame_new (_("Output Levels"));
  gtk_box_pack_start (GTK_BOX (frame_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), border);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_set_visible (vbox2, TRUE);

  tool->output_bar = g_object_new (CLAY_TYPE_COLOR_BAR, NULL);
  gtk_widget_set_size_request (tool->output_bar, -1, GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->output_bar, FALSE, FALSE, 0);
  gtk_widget_set_visible (tool->output_bar, TRUE);

  tool->output_handle_bar = g_object_new (CLAY_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (tool->output_handle_bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), tool->output_handle_bar, FALSE, FALSE,
                      0);
  gtk_widget_set_visible (tool->output_handle_bar, TRUE);

  clay_handle_bar_connect_events (CLAY_HANDLE_BAR (tool->output_handle_bar),
                                  tool->output_bar);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  low output spin  */
  tool->low_output_spinbutton = spinbutton =
    clay_prop_spin_button_new (filter_tool->config, "low-output",
                               0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  clay_handle_bar_set_adjustment (CLAY_HANDLE_BAR (tool->output_handle_bar), 0,
                                  adjustment);

  /*  clamp output toggle  */
  button = clay_prop_check_button_new (filter_tool->config, "clamp-output",
                                       _("Clamp outpu_t"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);

  /*  high output spin  */
  tool->high_output_spinbutton = spinbutton =
    clay_prop_spin_button_new (filter_tool->config, "high-output",
                               0.01, 0.1, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));
  clay_handle_bar_set_adjustment (CLAY_HANDLE_BAR (tool->output_handle_bar), 2,
                                  adjustment);

  /*  all channels frame  */
  main_frame = clay_frame_new (_("All Channels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (main_frame, TRUE);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_set_visible (frame_vbox, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Auto Input Levels"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  clay_help_set_help_data (button,
                           _("Adjust levels for all channels automatically"),
                           NULL);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (levels_stretch_callback),
                           tool, 0);

  button = clay_levels_tool_color_picker_new (tool,
                                              PICK_HIGH_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  button = clay_levels_tool_color_picker_new (tool,
                                              PICK_GAMMA |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  button = clay_levels_tool_color_picker_new (tool,
                                              PICK_LOW_INPUT |
                                              PICK_ALL_CHANNELS);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  button = clay_icon_button_new (CLAY_ICON_TOOL_CURVES,
                                 _("Edit these Settings as Curves"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (levels_to_curves_callback),
                           tool, 0);

  g_signal_connect_object (gui_config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (clay_levels_tool_style_updated),
                           tool, G_CONNECT_AFTER);
  g_signal_connect_object (gui_config,
                           "notify::custom-icon-size",
                           G_CALLBACK (clay_levels_tool_style_updated),
                           tool, G_CONNECT_AFTER);
  clay_levels_tool_style_updated (gui_config, NULL, tool);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (tool->channel_menu),
                                 config->channel);
}

static void
clay_levels_tool_reset (ClayFilterTool *filter_tool)
{
  ClayHistogramChannel channel;

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  CLAY_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  g_object_set (filter_tool->config,
                "channel", channel,
                NULL);
  /* XXX: the tool defaults to Perceptual, unlike the op.
   * See #15962.
   */
  g_object_set (filter_tool->config,
                "trc", CLAY_TRC_PERCEPTUAL,
                NULL);
}

static void
clay_levels_tool_config_notify (ClayFilterTool   *filter_tool,
                                ClayConfig       *config,
                                const GParamSpec *pspec)
{
  ClayLevelsTool   *levels_tool   = CLAY_LEVELS_TOOL (filter_tool);
  ClayLevelsConfig *levels_config = CLAY_LEVELS_CONFIG (config);

  CLAY_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! levels_tool->channel_menu ||
      ! levels_tool->histogram_view)
    return;

  if (! strcmp (pspec->name, "trc"))
    {
      g_clear_object (&levels_tool->histogram);
      g_clear_object (&levels_tool->histogram_async);
      levels_tool->histogram = clay_histogram_new (levels_config->trc);

      levels_tool->histogram_async = clay_drawable_calculate_histogram_async
        (CLAY_TOOL (filter_tool)->drawables->data, levels_tool->histogram, FALSE);
      clay_histogram_view_set_histogram (CLAY_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                         levels_tool->histogram);
    }
  else if (! strcmp (pspec->name, "channel"))
    {
      clay_histogram_view_set_channel (CLAY_HISTOGRAM_VIEW (levels_tool->histogram_view),
                                       levels_config->channel);
      clay_color_bar_set_channel (CLAY_COLOR_BAR (levels_tool->output_bar),
                                  levels_config->channel);
      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (levels_tool->channel_menu),
                                     levels_config->channel);
    }
  else if (! strcmp (pspec->name, "gamma")     ||
           ! strcmp (pspec->name, "low-input") ||
           ! strcmp (pspec->name, "high-input"))
    {
      gdouble low  = gtk_adjustment_get_value (levels_tool->low_input);
      gdouble high = gtk_adjustment_get_value (levels_tool->high_input);
      gdouble delta, mid, tmp, value;

      gtk_adjustment_set_lower (levels_tool->high_input,   low);
      gtk_adjustment_set_lower (levels_tool->gamma_linear, low);

      gtk_adjustment_set_upper (levels_tool->low_input,    high);
      gtk_adjustment_set_upper (levels_tool->gamma_linear, high);

      levels_update_input_bar (levels_tool);

      delta = (high - low) / 2.0;
      mid   = low + delta;
      tmp   = log10 (1.0 / levels_config->gamma[levels_config->channel]);
      value = mid + delta * tmp;

      gtk_adjustment_set_value (levels_tool->gamma_linear, value);
    }
}

static void
clay_levels_tool_style_updated (ClayGuiConfig  *config,
                                GParamSpec     *pspec,
                                ClayLevelsTool *tool)
{
  gint handle_bar_size = CONTROL_HEIGHT;

  if (config->override_icon_size)
    {
      switch (config->custom_icon_size)
        {
        case CLAY_ICON_SIZE_LARGE:
          handle_bar_size += 4;
          break;

        case CLAY_ICON_SIZE_HUGE:
          handle_bar_size += 8;
          break;

        case CLAY_ICON_SIZE_MEDIUM:
        case CLAY_ICON_SIZE_SMALL:
        default:
          handle_bar_size = CONTROL_HEIGHT;
        }
    }

  gtk_widget_set_size_request (tool->input_handle_bar, -1, handle_bar_size);
  gtk_widget_set_size_request (tool->output_handle_bar, -1, handle_bar_size);
}

static gboolean
clay_levels_tool_settings_import (ClayFilterTool  *filter_tool,
                                  GInputStream    *input,
                                  GError         **error)
{
  ClayLevelsConfig *config = CLAY_LEVELS_CONFIG (filter_tool->config);
  gchar             header[64];
  gsize             bytes_read;

  if (! g_input_stream_read_all (input, header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
      if (error && *error)
        g_prefix_error (error, _("Could not read header: "));
      else
        g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                     _("Could not read header: "));
      return FALSE;
    }

  g_seekable_seek (G_SEEKABLE (input), 0, G_SEEK_SET, NULL, NULL);

  if (g_str_has_prefix (header, "# Clay Levels File\n"))
    return clay_levels_config_load_cruft (config, input, error);

  /* Check if we're loading a Photoshop Levels preset .alv file */
  if (bytes_read > 2 && header[0] == 0 && (header[1] == 2))
    return clay_levels_config_load_alv (config, input, error);

  return CLAY_FILTER_TOOL_CLASS (parent_class)->settings_import (filter_tool,
                                                                 input,
                                                                 error);
}

static gboolean
clay_levels_tool_settings_export (ClayFilterTool  *filter_tool,
                                  GOutputStream   *output,
                                  GError         **error)
{
  ClayLevelsConfig *config   = CLAY_LEVELS_CONFIG (filter_tool->config);
  ClaySettingsBox  *settings = CLAY_SETTINGS_BOX (filter_tool->settings_box);

  if (clay_settings_box_get_legacy (settings))
    return clay_levels_config_save_cruft (config, output, error);

  return CLAY_FILTER_TOOL_CLASS (parent_class)->settings_export (filter_tool,
                                                                 output,
                                                                 error);
}

static void
levels_input_adjust_by_color (ClayLevelsConfig     *config,
                              guint                 value,
                              ClayHistogramChannel  channel,
                              const Babl           *target_space,
                              GeglColor            *color)
{
  switch (value & 0xF)
    {
    case PICK_LOW_INPUT:
      clay_levels_config_adjust_by_colors (config, channel, target_space, color, NULL, NULL);
      break;
    case PICK_GAMMA:
      clay_levels_config_adjust_by_colors (config, channel, target_space, NULL, color, NULL);
      break;
    case PICK_HIGH_INPUT:
      clay_levels_config_adjust_by_colors (config, channel, target_space, NULL, NULL, color);
      break;
    default:
      break;
    }
}

static void
clay_levels_tool_color_picked (ClayFilterTool *color_tool,
                               gpointer        identifier,
                               gdouble         x,
                               gdouble         y,
                               const Babl     *sample_format,
                               GeglColor      *color)
{
  ClayFilterTool   *filter_tool  = CLAY_FILTER_TOOL (color_tool);
  ClayLevelsConfig *config       = CLAY_LEVELS_CONFIG (filter_tool->config);
  guint             value        = GPOINTER_TO_UINT (identifier);
  const Babl       *target_space = NULL;

  target_space = clay_drawable_get_space (CLAY_TOOL (filter_tool)->drawables->data);

  if (value & PICK_ALL_CHANNELS &&
      clay_babl_format_get_base_type (sample_format) == CLAY_RGB)
    {
      ClayHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
        {
        case PICK_LOW_INPUT:
          config->low_input[CLAY_HISTOGRAM_VALUE] = 0.0;
          break;
        case PICK_GAMMA:
          config->gamma[CLAY_HISTOGRAM_VALUE] = 1.0;
          break;
        case PICK_HIGH_INPUT:
          config->high_input[CLAY_HISTOGRAM_VALUE] = 1.0;
          break;
        default:
          break;
        }

      /*  then adjust all color channels  */
      for (channel = CLAY_HISTOGRAM_RED;
           channel <= CLAY_HISTOGRAM_BLUE;
           channel++)
        {
          levels_input_adjust_by_color (config, value, channel, target_space, color);
        }
    }
  else
    {
      levels_input_adjust_by_color (config, value, config->channel, target_space, color);
    }
}

static void
levels_update_input_bar (ClayLevelsTool *tool)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayLevelsConfig *config      = CLAY_LEVELS_CONFIG (filter_tool->config);

  switch (config->channel)
    {
      gdouble value;

    case CLAY_HISTOGRAM_VALUE:
    case CLAY_HISTOGRAM_ALPHA:
    case CLAY_HISTOGRAM_RGB:
    case CLAY_HISTOGRAM_LUMINANCE:
      {
        guchar v[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            value = clay_operation_levels_map_input (config,
                                                     config->channel,
                                                     i / 255.0);
            v[i] = CLAMP (value, 0.0, 1.0) * 255.999;
          }

        clay_color_bar_set_buffers (CLAY_COLOR_BAR (tool->input_bar),
                                    v, v, v);
      }
      break;

    case CLAY_HISTOGRAM_RED:
    case CLAY_HISTOGRAM_GREEN:
    case CLAY_HISTOGRAM_BLUE:
      {
        guchar r[256];
        guchar g[256];
        guchar b[256];
        gint   i;

        for (i = 0; i < 256; i++)
          {
            value = clay_operation_levels_map_input (config,
                                                     CLAY_HISTOGRAM_RED,
                                                     i / 255.0);
            r[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = clay_operation_levels_map_input (config,
                                                     CLAY_HISTOGRAM_GREEN,
                                                     i / 255.0);
            g[i] = CLAMP (value, 0.0, 1.0) * 255.999;

            value = clay_operation_levels_map_input (config,
                                                     CLAY_HISTOGRAM_BLUE,
                                                     i / 255.0);
            b[i] = CLAMP (value, 0.0, 1.0) * 255.999;
          }

        clay_color_bar_set_buffers (CLAY_COLOR_BAR (tool->input_bar),
                                    r, g, b);
      }
      break;
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
                         ClayFilterTool *filter_tool)
{
  ClayLevelsConfig *config = CLAY_LEVELS_CONFIG (filter_tool->config);
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
levels_channel_reset_callback (GtkWidget      *widget,
                               ClayFilterTool *filter_tool)
{
  clay_levels_config_reset_channel (CLAY_LEVELS_CONFIG (filter_tool->config));
}

static gboolean
levels_menu_sensitivity (gint      value,
                         gpointer  data)
{
  ClayDrawable         *drawable;
  ClayHistogramChannel  channel  = value;

  if (! CLAY_TOOL (data)->drawables)
    return FALSE;

  drawable = CLAY_TOOL (data)->drawables->data;

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
levels_stretch_callback (GtkWidget      *widget,
                         ClayLevelsTool *levels_tool)
{
  ClayTool       *tool        = CLAY_TOOL (levels_tool);
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (levels_tool);
  ClayWaitable   *waitable;

  waitable = clay_trivially_cancelable_waitable_new (
    CLAY_WAITABLE (levels_tool->histogram_async));

  clay_wait (tool->tool_info->clay, waitable, _("Calculating histogram..."));

  g_object_unref (waitable);

  if (clay_async_is_synced   (levels_tool->histogram_async) &&
      clay_async_is_finished (levels_tool->histogram_async))
    {
      clay_levels_config_stretch (CLAY_LEVELS_CONFIG (filter_tool->config),
                                  levels_tool->histogram,
                                  clay_drawable_is_rgb (tool->drawables->data));
    }
}

static void
levels_linear_gamma_changed (GtkAdjustment  *adjustment,
                             ClayLevelsTool *tool)
{
  gdouble low_input  = gtk_adjustment_get_value (tool->low_input);
  gdouble high_input = gtk_adjustment_get_value (tool->high_input);
  gdouble delta, mid, tmp, value;

  delta = (high_input - low_input) / 2.0;

  if (delta >= 0.5)
    {
      mid   = low_input + delta;
      tmp   = (gtk_adjustment_get_value (adjustment) - mid) / delta;
      value = 1.0 / pow (10, tmp);

      /*  round the gamma value to the nearest 1/100th  */
      value = floor (value * 100 + 0.5) / 100.0;

      gtk_adjustment_set_value (tool->gamma, value);
    }
}

static void
levels_to_curves_callback (GtkWidget      *widget,
                           ClayFilterTool *filter_tool)
{
  ClayLevelsConfig *config = CLAY_LEVELS_CONFIG (filter_tool->config);
  ClayCurvesConfig *curves;

  curves = clay_levels_config_to_curves_config (config);

  clay_filter_tool_edit_as (filter_tool,
                            "clay-curves-tool",
                            CLAY_CONFIG (curves));

  g_object_unref (curves);
}
