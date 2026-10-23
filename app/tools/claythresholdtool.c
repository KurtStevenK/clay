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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay-gui.h"
#include "core/clayasync.h"
#include "core/claycancelable.h"
#include "core/claydrawable.h"
#include "core/claydrawable-histogram.h"
#include "core/clayhistogram.h"
#include "core/clayimage.h"
#include "core/claytoolinfo.h"
#include "core/claytriviallycancelablewaitable.h"
#include "core/claywaitable.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayhistogrambox.h"
#include "widgets/clayhistogramview.h"

#include "display/claydisplay.h"

#include "clayhistogramoptions.h"
#include "claythresholdtool.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void       clay_threshold_tool_finalize        (GObject           *object);

static gboolean   clay_threshold_tool_initialize      (ClayTool          *tool,
                                                       ClayDisplay       *display,
                                                       GError           **error);

static gchar    * clay_threshold_tool_get_operation   (ClayFilterTool    *filter_tool,
                                                       gchar            **description);
static void       clay_threshold_tool_dialog          (ClayFilterTool    *filter_tool);
static void       clay_threshold_tool_config_notify   (ClayFilterTool    *filter_tool,
                                                       ClayConfig        *config,
                                                       const GParamSpec  *pspec);

static gboolean   clay_threshold_tool_channel_sensitive
                                                      (gint               value,
                                                       gpointer           data);
static void       clay_threshold_tool_histogram_range (ClayHistogramView *view,
                                                       gint               start,
                                                       gint               end,
                                                       ClayThresholdTool *t_tool);
static void       clay_threshold_tool_auto_clicked    (GtkWidget         *button,
                                                       ClayThresholdTool *t_tool);


G_DEFINE_TYPE (ClayThresholdTool, clay_threshold_tool,
               CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_threshold_tool_parent_class


void
clay_threshold_tool_register (ClayToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (CLAY_TYPE_THRESHOLD_TOOL,
                CLAY_TYPE_HISTOGRAM_OPTIONS,
                NULL,
                0,
                "clay-threshold-tool",
                _("Threshold"),
                _("Reduce image to two colors using a threshold"),
                N_("_Threshold..."), NULL,
                NULL, CLAY_HELP_TOOL_THRESHOLD,
                CLAY_ICON_TOOL_THRESHOLD,
                data);
}

static void
clay_threshold_tool_class_init (ClayThresholdToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  object_class->finalize           = clay_threshold_tool_finalize;

  tool_class->initialize           = clay_threshold_tool_initialize;

  filter_tool_class->get_operation = clay_threshold_tool_get_operation;
  filter_tool_class->dialog        = clay_threshold_tool_dialog;
  filter_tool_class->config_notify = clay_threshold_tool_config_notify;
}

static void
clay_threshold_tool_init (ClayThresholdTool *t_tool)
{
  t_tool->histogram = clay_histogram_new (FALSE);
}

static void
clay_threshold_tool_finalize (GObject *object)
{
  ClayThresholdTool *t_tool = CLAY_THRESHOLD_TOOL (object);

  g_clear_object (&t_tool->histogram);
  g_clear_object (&t_tool->histogram_async);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_threshold_tool_initialize (ClayTool     *tool,
                                ClayDisplay  *display,
                                GError      **error)
{
  ClayThresholdTool *t_tool      = CLAY_THRESHOLD_TOOL (tool);
  ClayFilterTool    *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayImage         *image       = clay_display_get_image (display);
  GList             *drawables;
  ClayDrawable      *drawable;
  gdouble            low;
  gdouble            high;
  gint               n_bins;

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

  g_clear_object (&t_tool->histogram_async);

  g_object_get (filter_tool->config,
                "low",  &low,
                "high", &high,
                NULL);

  /* this is a hack to make sure that
   * 'clay_histogram_n_bins (t_tool->histogram)' returns the correct value for
   * 'drawable' before the asynchronous calculation of its histogram is
   * finished.
   */
  {
    GeglBuffer *temp;

    temp = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 1, 1),
                            clay_drawable_get_format (drawable));

    clay_histogram_calculate (t_tool->histogram,
                              temp, GEGL_RECTANGLE (0, 0, 1, 1),
                              NULL, NULL);

    g_object_unref (temp);
  }

  n_bins = clay_histogram_n_bins (t_tool->histogram);

  t_tool->histogram_async = clay_drawable_calculate_histogram_async (
    drawable, t_tool->histogram, FALSE);
  clay_histogram_view_set_histogram (t_tool->histogram_box->view,
                                     t_tool->histogram);

  clay_histogram_view_set_range (t_tool->histogram_box->view,
                                 low  * (n_bins - 0.0001),
                                 high * (n_bins - 0.0001));

  return TRUE;
}

static gchar *
clay_threshold_tool_get_operation (ClayFilterTool  *filter_tool,
                                   gchar          **description)
{
  *description = g_strdup (_("Apply Threshold"));

  return g_strdup ("clay:threshold");
}


/**********************/
/*  Threshold dialog  */
/**********************/

static void
clay_threshold_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayThresholdTool    *t_tool       = CLAY_THRESHOLD_TOOL (filter_tool);
  ClayToolOptions      *tool_options = CLAY_TOOL_GET_OPTIONS (filter_tool);
  GtkWidget            *main_vbox;
  GtkWidget            *main_frame;
  GtkWidget            *frame_vbox;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *hbox2;
  GtkWidget            *box;
  GtkWidget            *button;
  ClayHistogramChannel  channel;

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

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

  t_tool->channel_menu = clay_prop_enum_combo_box_new (filter_tool->config,
                                                       "channel", -1, -1);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (t_tool->channel_menu),
                                       "clay-channel");
  clay_int_combo_box_set_sensitivity (CLAY_INT_COMBO_BOX (t_tool->channel_menu),
                                      clay_threshold_tool_channel_sensitive,
                                      filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), t_tool->channel_menu, FALSE, FALSE, 0);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), t_tool->channel_menu);

  hbox2 = clay_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "clay-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_set_visible (frame_vbox, TRUE);

  box = clay_histogram_box_new ();
  gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, TRUE, 0);
  gtk_widget_set_visible (box, TRUE);

  t_tool->histogram_box = CLAY_HISTOGRAM_BOX (box);

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  clay_histogram_view_set_channel (t_tool->histogram_box->view, channel);

  g_signal_connect (t_tool->histogram_box->view, "range-changed",
                    G_CALLBACK (clay_threshold_tool_histogram_range),
                    t_tool);

  g_object_bind_property (G_OBJECT (tool_options),                "histogram-scale",
                          G_OBJECT (t_tool->histogram_box->view), "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Auto"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  clay_help_set_help_data (button, _("Automatically adjust to optimal "
                                     "binarization threshold"), NULL);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_threshold_tool_auto_clicked),
                    t_tool);
}

static void
clay_threshold_tool_config_notify (ClayFilterTool   *filter_tool,
                                   ClayConfig       *config,
                                   const GParamSpec *pspec)
{
  ClayThresholdTool *t_tool = CLAY_THRESHOLD_TOOL (filter_tool);

  CLAY_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! t_tool->histogram_box)
    return;

  if (! strcmp (pspec->name, "channel"))
    {
      ClayHistogramChannel channel;

      g_object_get (config,
                    "channel", &channel,
                    NULL);

      clay_histogram_view_set_channel (t_tool->histogram_box->view,
                                       channel);
    }
  else if (! strcmp (pspec->name, "low") ||
           ! strcmp (pspec->name, "high"))
    {
      gdouble low;
      gdouble high;
      gint    n_bins;

      g_object_get (config,
                    "low",  &low,
                    "high", &high,
                    NULL);

      n_bins = clay_histogram_n_bins (t_tool->histogram);

      clay_histogram_view_set_range (t_tool->histogram_box->view,
                                     low  * (n_bins - 0.0001),
                                     high * (n_bins - 0.0001));
    }
}

static gboolean
clay_threshold_tool_channel_sensitive (gint     value,
                                       gpointer data)
{
  GList                *drawables = CLAY_TOOL (data)->drawables;
  ClayDrawable         *drawable;
  ClayHistogramChannel  channel   = value;

  if (!drawables)
    return FALSE;

  g_return_val_if_fail (g_list_length (drawables) == 1, FALSE);
  drawable = drawables->data;

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
      return clay_drawable_is_rgb (drawable);

    case CLAY_HISTOGRAM_LUMINANCE:
      return clay_drawable_is_rgb (drawable);
    }

  return FALSE;
}

static void
clay_threshold_tool_histogram_range (ClayHistogramView *widget,
                                     gint               start,
                                     gint               end,
                                     ClayThresholdTool *t_tool)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (t_tool);
  gint            n_bins      = clay_histogram_n_bins (t_tool->histogram);
  gdouble         low         = (gdouble) start / (n_bins - 1);
  gdouble         high        = (gdouble) end   / (n_bins - 1);
  gdouble         config_low;
  gdouble         config_high;

  g_object_get (filter_tool->config,
                "low",  &config_low,
                "high", &config_high,
                NULL);

  if (low  != config_low ||
      high != config_high)
    {
      g_object_set (filter_tool->config,
                    "low",  low,
                    "high", high,
                    NULL);
    }
}

static void
clay_threshold_tool_auto_clicked (GtkWidget         *button,
                                  ClayThresholdTool *t_tool)
{
  ClayTool     *tool = CLAY_TOOL (t_tool);
  ClayWaitable *waitable;

  waitable = clay_trivially_cancelable_waitable_new (
    CLAY_WAITABLE (t_tool->histogram_async));

  clay_wait (tool->tool_info->clay, waitable, _("Calculating histogram..."));

  g_object_unref (waitable);

  if (clay_async_is_synced   (t_tool->histogram_async) &&
      clay_async_is_finished (t_tool->histogram_async))
    {
      ClayHistogramChannel channel;
      gint                 n_bins;
      gdouble              low;

      g_object_get (CLAY_FILTER_TOOL (t_tool)->config,
                    "channel", &channel,
                    NULL);

      n_bins = clay_histogram_n_bins (t_tool->histogram);

      low = clay_histogram_get_threshold (t_tool->histogram,
                                          channel,
                                          0, n_bins - 1);

      clay_histogram_view_set_range (t_tool->histogram_box->view,
                                     low, n_bins - 1);
    }
}
