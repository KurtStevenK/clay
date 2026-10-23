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
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"
#include "libclaywidgets/claywidgets-private.h"

#include "tools-types.h"

#include "config/claycoreconfig.h"
#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydatafactory.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/claypaintinfo.h"
#include "core/claystrokeoptions.h"
#include "core/claytoolinfo.h"

#include "display/claydisplay.h"

#include "paint/claysourceoptions.h"

#include "widgets/claycontainercombobox.h"
#include "widgets/claycontainertreestore.h"
#include "widgets/claycontainerview.h"
#include "widgets/claypropwidgets.h"
#include "widgets/clayviewablebox.h"
#include "widgets/clayviewrenderer.h"
#include "widgets/claywidgets-utils.h"

#include "claybucketfilloptions.h"
#include "claypaintoptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_FILL_MODE,
  PROP_FILL_AREA,
  PROP_FILL_TRANSPARENT,
  PROP_SAMPLE_MERGED,
  PROP_DIAGONAL_NEIGHBORS,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_THRESHOLD,
  PROP_LINE_ART_SOURCE,
  PROP_LINE_ART_THRESHOLD,
  PROP_LINE_ART_MAX_GROW,
  PROP_LINE_ART_STROKE,
  PROP_LINE_ART_STROKE_TOOL,
  PROP_LINE_ART_AUTOMATIC_CLOSURE,
  PROP_LINE_ART_MAX_GAP_LENGTH,
  PROP_FILL_CRITERION,
  PROP_FILL_COLOR_AS_LINE_ART,
  PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD,
};

struct _ClayBucketFillOptionsPrivate
{
  GtkWidget *diagonal_neighbors_checkbox;
  GtkWidget *threshold_scale;

  GtkWidget *similar_color_frame;
  GtkWidget *line_art_settings;
  GtkWidget *fill_as_line_art_frame;
  GtkWidget *line_art_detect_opacity;
};

static void   clay_bucket_fill_options_config_iface_init (ClayConfigInterface *config_iface);

static void   clay_bucket_fill_options_finalize          (GObject               *object);
static void   clay_bucket_fill_options_set_property      (GObject               *object,
                                                          guint                  property_id,
                                                          const GValue          *value,
                                                          GParamSpec            *pspec);
static void   clay_bucket_fill_options_get_property      (GObject               *object,
                                                          guint                  property_id,
                                                          GValue                *value,
                                                          GParamSpec            *pspec);
static void  clay_bucket_fill_options_select_stroke_tool (ClayContainerView     *view,
                                                          ClayBucketFillOptions *options);
static void  clay_bucket_fill_options_tool_cell_renderer (GtkCellLayout         *layout,
                                                          GtkCellRenderer       *cell,
                                                          GtkTreeModel          *model,
                                                          GtkTreeIter           *iter,
                                                          gpointer               data);
static void clay_bucket_fill_options_image_changed       (ClayContext           *context,
                                                          ClayImage             *image,
                                                          ClayBucketFillOptions *options);


static void   clay_bucket_fill_options_reset             (ClayConfig            *config);
static void   clay_bucket_fill_options_update_area       (ClayBucketFillOptions *options);


G_DEFINE_TYPE_WITH_CODE (ClayBucketFillOptions, clay_bucket_fill_options,
                         CLAY_TYPE_PAINT_OPTIONS,
                         G_ADD_PRIVATE (ClayBucketFillOptions)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_bucket_fill_options_config_iface_init))

#define parent_class clay_bucket_fill_options_parent_class

static ClayConfigInterface *parent_config_iface = NULL;


static void
clay_bucket_fill_options_class_init (ClayBucketFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_bucket_fill_options_finalize;
  object_class->set_property = clay_bucket_fill_options_set_property;
  object_class->get_property = clay_bucket_fill_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FILL_MODE,
                         "fill-mode",
                         _("Fill type"),
                         NULL,
                         CLAY_TYPE_BUCKET_FILL_MODE,
                         CLAY_BUCKET_FILL_FG,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FILL_AREA,
                         "fill-area",
                         _("Fill selection"),
                         _("Which area will be filled"),
                         CLAY_TYPE_BUCKET_FILL_AREA,
                         CLAY_BUCKET_FILL_SIMILAR_COLORS,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_TRANSPARENT,
                            "fill-transparent",
                            _("Fill transparent areas"),
                            _("Allow completely transparent regions "
                              "to be filled"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            _("Base filled area on all visible layers"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_DIAGONAL_NEIGHBORS,
                            "diagonal-neighbors",
                            _("Diagonal neighbors"),
                            _("Treat diagonally neighboring pixels as "
                              "connected"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Base fill opacity on color difference from "
                              "the clicked pixel (see threshold) or on line "
                              " art borders. Disable antialiasing to fill "
                              "the entire area uniformly."),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of fill edges"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                           "threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_LINE_ART_SOURCE,
                         "line-art-source",
                         _("Source"),
                         _("Source image for line art computation"),
                         CLAY_TYPE_LINE_ART_SOURCE,
                         CLAY_LINE_ART_SOURCE_SAMPLE_MERGED,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_COLOR_AS_LINE_ART,
                            "fill-color-as-line-art",
                            _("Manual closure in fill layer"),
                            _("Consider pixels of selected layer and filled with the fill color as line art closure"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD,
                           "fill-color-as-line-art-threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_ART_THRESHOLD,
                           "line-art-threshold",
                           _("Line art detection threshold"),
                           _("Threshold to detect contour (higher values will include more pixels)"),
                           0.0, 1.0, 0.92,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GROW,
                        "line-art-max-grow",
                        _("Maximum growing size"),
                        _("Maximum number of pixels grown under the line art"),
                        1, 100, 3,
                        CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_STROKE,
                            "line-art-stroke-border",
                            _("Stroke borders"),
                            _("Stroke fill borders with last stroke options"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_STRING (object_class, PROP_LINE_ART_STROKE_TOOL,
                           "line-art-stroke-tool",
                           _("Stroke tool"),
                           _("The tool to stroke the fill borders with"),
                           "clay-pencil", CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_LINE_ART_AUTOMATIC_CLOSURE,
                            "line-art-automatic-closure",
                            _("Automatic closure"),
                            _("Geometric analysis of the stroke contours to close line arts by splines/segments"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_INT (object_class, PROP_LINE_ART_MAX_GAP_LENGTH,
                        "line-art-max-gap-length",
                        _("Maximum gap length"),
                        _("Maximum gap (in pixels) in line art which can be closed"),
                        0, 1000, 100,
                        CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FILL_CRITERION,
                         "fill-criterion",
                         _("Fill by"),
                         _("Criterion used for determining color similarity"),
                         CLAY_TYPE_SELECT_CRITERION,
                         CLAY_SELECT_CRITERION_COMPOSITE,
                         CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_bucket_fill_options_config_iface_init (ClayConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = clay_bucket_fill_options_reset;
}

static void
clay_bucket_fill_options_init (ClayBucketFillOptions *options)
{
  options->priv = clay_bucket_fill_options_get_instance_private (options);

  options->line_art_stroke_tool = NULL;
}

static void
clay_bucket_fill_options_finalize (GObject *object)
{
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_OPTIONS (object);

  g_clear_object (&options->stroke_options);
  g_clear_pointer (&options->line_art_stroke_tool, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_bucket_fill_options_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayBucketFillOptions *options      = CLAY_BUCKET_FILL_OPTIONS (object);
  ClayToolOptions       *tool_options = CLAY_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      options->fill_mode = g_value_get_enum (value);
      clay_bucket_fill_options_update_area (options);
      break;
    case PROP_FILL_AREA:
      options->fill_area = g_value_get_enum (value);
      clay_bucket_fill_options_update_area (options);
      break;
    case PROP_FILL_TRANSPARENT:
      options->fill_transparent = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_DIAGONAL_NEIGHBORS:
      options->diagonal_neighbors = g_value_get_boolean (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_FEATHER:
      options->feather = g_value_get_boolean (value);
      break;
    case PROP_FEATHER_RADIUS:
      options->feather_radius = g_value_get_double (value);
      break;
    case PROP_THRESHOLD:
      options->threshold = g_value_get_double (value);
      break;
    case PROP_LINE_ART_SOURCE:
      options->line_art_source = g_value_get_enum (value);
      clay_bucket_fill_options_update_area (options);
      break;
    case PROP_LINE_ART_THRESHOLD:
      options->line_art_threshold = g_value_get_double (value);
      break;
    case PROP_LINE_ART_MAX_GROW:
      options->line_art_max_grow = g_value_get_int (value);
      break;
    case PROP_LINE_ART_STROKE:
      options->line_art_stroke = g_value_get_boolean (value);
      break;
    case PROP_LINE_ART_STROKE_TOOL:
      g_set_str (&options->line_art_stroke_tool, g_value_get_string (value));

      if (options->stroke_options)
        {
          ClayPaintInfo *paint_info = NULL;
          Clay          *clay       = tool_options->tool_info->clay;

          if (! options->line_art_stroke_tool)
            options->line_art_stroke_tool = g_strdup ("clay-pencil");

          paint_info = CLAY_PAINT_INFO (clay_container_get_child_by_name (clay->paint_info_list,
                                                                          options->line_art_stroke_tool));
          if (! paint_info && ! clay_container_is_empty (clay->paint_info_list))
            paint_info = CLAY_PAINT_INFO (clay_container_get_child_by_index (clay->paint_info_list, 0));

          g_object_set (options->stroke_options,
                        "paint-info", paint_info,
                        NULL);
        }
      break;
    case PROP_LINE_ART_AUTOMATIC_CLOSURE:
      options->line_art_automatic_closure = g_value_get_boolean (value);
      break;
    case PROP_LINE_ART_MAX_GAP_LENGTH:
      options->line_art_max_gap_length = g_value_get_int (value);
      break;
    case PROP_FILL_CRITERION:
      options->fill_criterion = g_value_get_enum (value);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART:
      options->fill_as_line_art = g_value_get_boolean (value);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD:
      options->fill_as_line_art_threshold = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_bucket_fill_options_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayBucketFillOptions *options = CLAY_BUCKET_FILL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      g_value_set_enum (value, options->fill_mode);
      break;
    case PROP_FILL_AREA:
      g_value_set_enum (value, options->fill_area);
      break;
    case PROP_FILL_TRANSPARENT:
      g_value_set_boolean (value, options->fill_transparent);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_DIAGONAL_NEIGHBORS:
      g_value_set_boolean (value, options->diagonal_neighbors);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_FEATHER:
      g_value_set_boolean (value, options->feather);
      break;
    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, options->feather_radius);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, options->threshold);
      break;
    case PROP_LINE_ART_SOURCE:
      g_value_set_enum (value, options->line_art_source);
      break;
    case PROP_LINE_ART_THRESHOLD:
      g_value_set_double (value, options->line_art_threshold);
      break;
    case PROP_LINE_ART_MAX_GROW:
      g_value_set_int (value, options->line_art_max_grow);
      break;
    case PROP_LINE_ART_STROKE:
      g_value_set_boolean (value, options->line_art_stroke);
      break;
    case PROP_LINE_ART_STROKE_TOOL:
      g_value_set_string (value, options->line_art_stroke_tool);
      break;
    case PROP_LINE_ART_AUTOMATIC_CLOSURE:
      g_value_set_boolean (value, options->line_art_automatic_closure);
      break;
    case PROP_LINE_ART_MAX_GAP_LENGTH:
      g_value_set_int (value, options->line_art_max_gap_length);
      break;
    case PROP_FILL_CRITERION:
      g_value_set_enum (value, options->fill_criterion);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART:
      g_value_set_boolean (value, options->fill_as_line_art);
      break;
    case PROP_FILL_COLOR_AS_LINE_ART_THRESHOLD:
      g_value_set_double (value, options->fill_as_line_art_threshold);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_bucket_fill_options_select_stroke_tool (ClayContainerView     *view,
                                             ClayBucketFillOptions *options)
{
  ClayViewable *item = clay_container_view_get_1_selected (view);

  g_object_set (options,
                "line-art-stroke-tool", item,
                NULL);
}

static void
clay_bucket_fill_options_tool_cell_renderer (GtkCellLayout   *layout,
                                             GtkCellRenderer *cell,
                                             GtkTreeModel    *model,
                                             GtkTreeIter     *iter,
                                             gpointer         data)
{
  ClayViewRenderer *renderer;

  gtk_tree_model_get (model, iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer->viewable)
    {
      ClayPaintInfo *info = CLAY_PAINT_INFO (renderer->viewable);

      if (CLAY_IS_SOURCE_OPTIONS (info->paint_options))
        gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                            CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE, FALSE,
                            -1);
    }
  g_object_unref (renderer);
}

static void
clay_bucket_fill_options_image_changed (ClayContext           *context,
                                        ClayImage             *image,
                                        ClayBucketFillOptions *options)
{
  ClayImage *prev_image;

  prev_image = g_object_get_data (G_OBJECT (options), "clay-bucket-fill-options-image");

  if (image != prev_image)
    {
      if (prev_image)
        g_signal_handlers_disconnect_by_func (prev_image,
                                              G_CALLBACK (clay_bucket_fill_options_update_area),
                                              options);
      if (image)
        {
          g_signal_connect_object (image, "selected-channels-changed",
                                   G_CALLBACK (clay_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "selected-layers-changed",
                                   G_CALLBACK (clay_bucket_fill_options_update_area),
                                   options, G_CONNECT_SWAPPED);
        }

      g_object_set_data (G_OBJECT (options), "clay-bucket-fill-options-image", image);
      clay_bucket_fill_options_update_area (options);
    }
}

static void
clay_bucket_fill_options_reset (ClayConfig *config)
{
  ClayToolOptions *tool_options = CLAY_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      tool_options->tool_info->clay->config->default_threshold;

  parent_config_iface->reset (config);
}

static void
clay_bucket_fill_options_update_area (ClayBucketFillOptions *options)
{
  ClayImage   *image;
  GList       *drawables = NULL;
  const gchar *tooltip   = _("Opaque pixels will be considered as line art "
                             "instead of low luminance pixels");

  image = clay_context_get_image (clay_get_user_context (CLAY_CONTEXT (options)->clay));

  /* GUI not created yet. */
  if (! options->priv->threshold_scale)
    return;

  if (image)
    drawables = clay_image_get_selected_drawables (image);

  switch (options->fill_area)
    {
    case CLAY_BUCKET_FILL_LINE_ART:
      gtk_widget_set_visible (options->priv->similar_color_frame, FALSE);
      gtk_widget_set_visible (options->priv->line_art_settings, TRUE);
      if ((options->fill_mode == CLAY_BUCKET_FILL_FG ||
           options->fill_mode == CLAY_BUCKET_FILL_BG) &&
          (options->line_art_source == CLAY_LINE_ART_SOURCE_LOWER_LAYER ||
           options->line_art_source == CLAY_LINE_ART_SOURCE_UPPER_LAYER))
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, TRUE);
      else
        gtk_widget_set_sensitive (options->priv->fill_as_line_art_frame, FALSE);

      if (image != NULL                                                  &&
          options->line_art_source != CLAY_LINE_ART_SOURCE_SAMPLE_MERGED &&
          g_list_length (drawables) == 1)
        {
          ClayDrawable  *source = NULL;
          ClayItem      *parent;
          ClayContainer *container;
          gint           index;

          parent = clay_item_get_parent (CLAY_ITEM (drawables->data));
          if (parent)
            container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
          else
            container = clay_image_get_layers (image);

          index = clay_item_get_index (CLAY_ITEM (drawables->data));
          if (options->line_art_source == CLAY_LINE_ART_SOURCE_ACTIVE_LAYER)
            source = drawables->data;
          else if (options->line_art_source == CLAY_LINE_ART_SOURCE_LOWER_LAYER)
            source = CLAY_DRAWABLE (clay_container_get_child_by_index (container, index + 1));
          else if (options->line_art_source == CLAY_LINE_ART_SOURCE_UPPER_LAYER)
            source = CLAY_DRAWABLE (clay_container_get_child_by_index (container, index - 1));

          gtk_widget_set_sensitive (options->priv->line_art_detect_opacity,
                                    source != NULL &&
                                    clay_drawable_has_alpha (source));
          if (source == NULL)
            tooltip = _("No valid source drawable selected");
          else if (! clay_drawable_has_alpha (source))
            tooltip = _("The source drawable has no alpha channel");
        }
      else
        {
          gtk_widget_set_sensitive (options->priv->line_art_detect_opacity,
                                    TRUE);
        }
      gtk_widget_set_tooltip_text (options->priv->line_art_detect_opacity,
                                   tooltip);
      break;
    case CLAY_BUCKET_FILL_SIMILAR_COLORS:
      gtk_widget_set_visible (options->priv->similar_color_frame, TRUE);
      gtk_widget_set_visible (options->priv->line_art_settings, FALSE);
      break;
    default:
      gtk_widget_set_visible (options->priv->similar_color_frame, FALSE);
      gtk_widget_set_visible (options->priv->line_art_settings, FALSE);
      break;
    }

  g_list_free (drawables);
}

GtkWidget *
clay_bucket_fill_options_gui (ClayToolOptions *tool_options)
{
  ClayBucketFillOptions *options    = CLAY_BUCKET_FILL_OPTIONS (tool_options);
  GObject               *config     = G_OBJECT (tool_options);
  Clay                  *clay       = tool_options->tool_info->clay;
  GtkWidget             *vbox       = clay_paint_options_gui (tool_options);
  GtkWidget             *box2;
  GtkWidget             *frame;
  GtkWidget             *hbox;
  GtkWidget             *widget;
  GtkWidget             *scale;
  GtkWidget             *combo;
  gchar                 *str;
  gboolean               bold;
  GdkModifierType        extend_mask = clay_get_extend_selection_mask ();
  GdkModifierType        toggle_mask = GDK_MOD1_MASK;

  /*  fill type  */
  str = g_strdup_printf (_("Fill Type  (%s)"),
                         clay_get_mod_string (toggle_mask)),
  frame = clay_prop_enum_radio_frame_new (config, "fill-mode", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  hbox = clay_prop_pattern_box_new (NULL, CLAY_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  clay_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             CLAY_BUCKET_FILL_PATTERN, TRUE);

  /*  fill selection  */
  str = g_strdup_printf (_("Affected Area  (%s)"),
                         clay_get_mod_string (extend_mask));
  frame = clay_prop_enum_radio_frame_new (config, "fill-area", str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  /* Similar color frame */
  frame = clay_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  options->priv->similar_color_frame = frame;
  gtk_widget_set_visible (frame, TRUE);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_set_visible (box2, TRUE);

  /*  the fill transparent areas toggle  */
  widget = clay_prop_check_button_new (config, "fill-transparent", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the sample merged toggle  */
  widget = clay_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the diagonal neighbors toggle  */
  widget = clay_prop_check_button_new (config, "diagonal-neighbors", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
  options->priv->diagonal_neighbors_checkbox = widget;

  /*  the antialias toggle  */
  widget = clay_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /*  the threshold scale  */
  scale = clay_prop_spin_scale_new (config, "threshold",
                                    1.0, 16.0, 1);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);
  options->priv->threshold_scale = scale;

  /*  the fill criterion combo  */
  combo = clay_prop_enum_combo_box_new (config, "fill-criterion", 0, 0);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Fill by"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /* Line art settings */
  options->priv->line_art_settings = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), options->priv->line_art_settings, FALSE, FALSE, 0);
  clay_widget_set_identifier (options->priv->line_art_settings, "line-art-settings");

  frame = clay_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /* Line art: label widget */
  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_frame_set_label_widget (GTK_FRAME (frame), box2);
  gtk_widget_set_visible (box2, TRUE);

  widget = gtk_label_new (_("Line Art Detection"));
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);
  gtk_widget_style_get (GTK_WIDGET (frame),
                        "label-bold", &bold,
                        NULL);
  clay_label_set_attributes (GTK_LABEL (widget),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_widget_set_visible (widget, TRUE);

  options->line_art_busy_box = clay_busy_box_new (_("(computing...)"));
  gtk_box_pack_start (GTK_BOX (box2), options->line_art_busy_box,
                      FALSE, FALSE, 0);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_set_visible (box2, TRUE);

  /*  Line Art: stroke threshold */
  scale = clay_prop_spin_scale_new (config, "line-art-threshold",
                                    0.05, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /*  Line Art: source combo (replace sample merged!) */
  combo = clay_prop_enum_combo_box_new (config, "line-art-source", 0, 0);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Source"));
  gtk_box_pack_start (GTK_BOX (box2), combo, FALSE, FALSE, 0);

  /*  the fill transparent areas toggle  */
  widget = clay_prop_check_button_new (config, "fill-transparent",
                                       _("Detect opacity rather than grayscale"));
  options->priv->line_art_detect_opacity = widget;
  gtk_box_pack_start (GTK_BOX (box2), widget, FALSE, FALSE, 0);

  /* Line Art Closure frame */
  frame = clay_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /* Line Art Closure: frame label */
  widget = gtk_label_new (_("Line Art Closure"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), widget);
  gtk_widget_set_visible (widget, TRUE);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_set_visible (box2, TRUE);

  /*  Line Art Closure: max gap length */
  scale = clay_prop_spin_scale_new (config, "line-art-max-gap-length",
                                    1, 5, 0);
  frame = clay_prop_expanding_frame_new (config, "line-art-automatic-closure", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Closure: manual line art closure */
  scale = clay_prop_spin_scale_new (config, "fill-color-as-line-art-threshold",
                                    1.0, 16.0, 1);

  frame = clay_prop_expanding_frame_new (config, "fill-color-as-line-art", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  options->priv->fill_as_line_art_frame = frame;

  /* Line Art Borders frame */

  frame = clay_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (options->priv->line_art_settings), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /* Line Art Borders: frame label */
  widget = gtk_label_new (_("Fill borders"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), widget);
  gtk_widget_set_visible (widget, TRUE);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box2);
  gtk_widget_set_visible (box2, TRUE);

  /*  Line Art Borders: max growing size */
  scale = clay_prop_spin_scale_new (config, "line-art-max-grow",
                                    1, 5, 0);
  gtk_box_pack_start (GTK_BOX (box2), scale, FALSE, FALSE, 0);

  /*  Line Art Borders: feather radius scale  */
  scale = clay_prop_spin_scale_new (config, "feather-radius",
                                    1.0, 10.0, 1);

  frame = clay_prop_expanding_frame_new (config, "feather", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);

  /*  Line Art Borders: stroke border with paint brush */

  options->stroke_options = clay_stroke_options_new (clay,
                                                     clay_get_user_context (clay),
                                                     TRUE);
  clay_config_sync (G_OBJECT (CLAY_DIALOG_CONFIG (clay->config)->stroke_options),
                    G_OBJECT (options->stroke_options), 0);

  widget = clay_container_combo_box_new (clay->paint_info_list,
                                         CLAY_CONTEXT (options->stroke_options),
                                         16, 0);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (widget),
                                      CLAY_CONTAINER_COMBO_BOX (widget)->viewable_renderer,
                                      clay_bucket_fill_options_tool_cell_renderer,
                                      options, NULL);
  g_signal_connect (widget, "selection-changed",
                    G_CALLBACK (clay_bucket_fill_options_select_stroke_tool),
                    options);

  frame = clay_prop_expanding_frame_new (config, "line-art-stroke-border", NULL,
                                         widget, NULL);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (widget, TRUE);

  clay_bucket_fill_options_update_area (options);

  g_signal_connect (clay_get_user_context (CLAY_CONTEXT (tool_options)->clay),
                    "image-changed",
                    G_CALLBACK (clay_bucket_fill_options_image_changed),
                    tool_options);

  return vbox;
}
