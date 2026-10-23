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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "core/clay.h"
#include "core/claybrushgenerated.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/clayimage.h"
#include "core/claylist.h"
#include "core/claypaintinfo.h"
#include "core/claytoolinfo.h"

#include "paint/claypaintoptions.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claypaletteeditor.h"
#include "widgets/claycolormapeditor.h"

#include "actions.h"
#include "context-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     context_select_object    (ClayActionSelectType  select_type,
                                          ClayContext          *context,
                                          ClayContainer        *container);
static gint     context_paint_mode_index (ClayLayerMode         paint_mode,
                                          const ClayLayerMode  *modes,
                                          gint                  n_modes);

static void     context_select_color     (ClayActionSelectType  select_type,
                                          GeglColor            *color,
                                          gboolean              use_colormap,
                                          gboolean              use_palette);

static gint     context_get_color_index  (gboolean              use_colormap,
                                          gboolean              use_palette,
                                          GeglColor            *color);
static gint     context_max_color_index  (gboolean              use_colormap,
                                          gboolean              use_palette);
static gboolean context_set_color_index  (gint                  index,
                                          gboolean              use_colormap,
                                          gboolean              use_palette,
                                          GeglColor            *color);

static ClayPaletteEditor  * context_get_palette_editor  (void);
static ClayColormapEditor * context_get_colormap_editor (void);


/*  public functions  */

void
context_colors_default_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext *context;
  return_if_no_context (context, data);

  clay_context_set_default_colors (context);
}

void
context_colors_swap_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContext *context;
  return_if_no_context (context, data);

  clay_context_swap_colors (context);
}

#define SELECT_COLOR_CMD_CALLBACK(name, fgbg, use_colormap, use_palette) \
void \
context_##name##_##fgbg##ground_cmd_callback (ClayAction *action, \
                                              GVariant   *value, \
                                              gpointer    data) \
{ \
  ClayContext          *context; \
  GeglColor            *color; \
  ClayActionSelectType  select_type; \
  return_if_no_context (context, data); \
  \
  select_type = (ClayActionSelectType) g_variant_get_int32 (value); \
  \
  color = gegl_color_duplicate (clay_context_get_##fgbg##ground (context)); \
  context_select_color (select_type, color, \
                        use_colormap, use_palette); \
  clay_context_set_##fgbg##ground (context, color); \
  g_object_unref (color); \
}

SELECT_COLOR_CMD_CALLBACK (palette,  fore, FALSE, TRUE)
SELECT_COLOR_CMD_CALLBACK (palette,  back, FALSE, TRUE)
SELECT_COLOR_CMD_CALLBACK (colormap, fore, TRUE,  FALSE)
SELECT_COLOR_CMD_CALLBACK (colormap, back, TRUE,  FALSE)
SELECT_COLOR_CMD_CALLBACK (swatch,   fore, TRUE,  TRUE)
SELECT_COLOR_CMD_CALLBACK (swatch,   back, TRUE,  TRUE)

void
context_foreground_red_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_foreground (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  /* TODO: if value was already out-of-gamut, say we want to decrease it
   * progressively. Should the algorithm allow it to be decreased while still
   * staying out-of-gamut? Currently the function always clamps the result to
   * min/max.
   */
  pixel[0] = action_select_value (select_type,
                                  pixel[0],
                                  0.0, 1.0, 1.0,
                                  1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_foreground_green_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_foreground (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  pixel[1] = action_select_value (select_type,
                                  pixel[1],
                                  0.0, 1.0, 1.0,
                                  1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_foreground_blue_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_foreground (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  pixel[2] = action_select_value (select_type,
                                  pixel[2],
                                  0.0, 1.0, 1.0,
                                  1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_background_red_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_background (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  pixel[0] = action_select_value (select_type,
                                  pixel[0],
                                  0.0, 1.0, 1.0,
                                  1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_background_green_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_background (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  pixel[1] = action_select_value (select_type,
                                  pixel[1],
                                  0.0, 1.0, 1.0,
                                  1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_background_blue_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  const Babl           *format;
  gdouble               pixel[4];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color  = gegl_color_duplicate (clay_context_get_background (context));
  format = clay_context_get_rgba_format (context, color, "double", NULL);

  gegl_color_get_pixel (color, format, pixel);
  pixel[2] = action_select_value (select_type,
                              pixel[2],
                              0.0, 1.0, 1.0,
                              1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, format, pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_foreground_hue_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_foreground (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[0] = (gfloat) action_select_value (select_type,
                                           pixel[0],
                                           0.0, 1.0, 1.0,
                                           1.0 / 360.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_foreground_saturation_cmd_callback (ClayAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_foreground (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[1] = (gfloat) action_select_value (select_type,
                                           pixel[1],
                                           0.0, 1.0, 1.0,
                                           0.01, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_foreground_value_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_foreground (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[2] = (gfloat) action_select_value (select_type,
                                           pixel[2],
                                           0.0, 1.0, 1.0,
                                           0.01, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_foreground (context, color);
  g_object_unref (color);
}

void
context_background_hue_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_background (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[0] = (gfloat) action_select_value (select_type,
                                           pixel[0],
                                           0.0, 1.0, 1.0,
                                           1.0 / 360.0, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_background_saturation_cmd_callback (ClayAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_background (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[1] = (gfloat) action_select_value (select_type,
                                           pixel[1],
                                           0.0, 1.0, 1.0,
                                           0.01, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_background_value_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayContext          *context;
  GeglColor            *color;
  gfloat                pixel[3];
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  color = gegl_color_duplicate (clay_context_get_background (context));
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  pixel[2] = (gfloat) action_select_value (select_type,
                                           pixel[2],
                                           0.0, 1.0, 1.0,
                                           0.01, 0.01, 0.1, 0.0, FALSE);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV float", NULL), pixel);
  clay_context_set_background (context, color);
  g_object_unref (color);
}

void
context_opacity_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayContext          *context;
  ClayToolInfo         *tool_info;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  tool_info = clay_context_get_tool (context);

  if (tool_info && CLAY_IS_TOOL_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "opacity",
                              1.0 / 255.0, 0.01, 0.1, 0.1, FALSE);
    }
}

void
context_paint_mode_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayContext          *context;
  ClayToolInfo         *tool_info;
  ClayLayerMode        *modes;
  gint                  n_modes;
  ClayLayerMode         paint_mode;
  gint                  index;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  paint_mode = clay_context_get_paint_mode (context);

  modes = clay_layer_mode_get_context_array (paint_mode,
                                             CLAY_LAYER_MODE_CONTEXT_PAINT,
                                             &n_modes);
  index = context_paint_mode_index (paint_mode, modes, n_modes);
  index = action_select_value (select_type,
                               index, 0, n_modes - 1, 0,
                               0.0, 1.0, 1.0, 0.0, FALSE);
  paint_mode = modes[index];
  g_free (modes);

  clay_context_set_paint_mode (context, paint_mode);

  tool_info = clay_context_get_tool (context);

  if (tool_info && CLAY_IS_TOOL_OPTIONS (tool_info->tool_options))
    {
      ClayDisplay *display;
      const char  *value_desc;

      clay_enum_get_value (CLAY_TYPE_LAYER_MODE, paint_mode,
                           NULL, NULL, &value_desc, NULL);

      display = action_data_get_display (data);

      if (value_desc && display)
        {
          action_message (display, G_OBJECT (tool_info->tool_options),
                          _("Paint Mode: %s"), value_desc);
        }
    }
}

void
context_tool_select_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context, context->clay->tool_info_list);
}

void
context_brush_select_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         clay_data_factory_get_container (context->clay->brush_factory));
}

void
context_pattern_select_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         clay_data_factory_get_container (context->clay->pattern_factory));
}

void
context_palette_select_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         clay_data_factory_get_container (context->clay->palette_factory));
}

void
context_gradient_select_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         clay_data_factory_get_container (context->clay->gradient_factory));
}

void
context_font_select_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContext          *context;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         clay_data_factory_get_container (context->clay->font_factory));
}

void
context_brush_spacing_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH (brush) && clay_data_is_writable (CLAY_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "spacing",
                              1.0, 5.0, 20.0, 0.1, FALSE);
    }
}

void
context_brush_shape_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContext             *context;
  ClayBrush               *brush;
  ClayBrushGeneratedShape  shape;
  return_if_no_context (context, data);

  shape = (ClayBrushGeneratedShape) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      ClayBrushGenerated *generated = CLAY_BRUSH_GENERATED (brush);
      ClayDisplay        *display;
      const char         *value_desc;

      clay_brush_generated_set_shape (generated, shape);

      clay_enum_get_value (CLAY_TYPE_BRUSH_GENERATED_SHAPE, shape,
                           NULL, NULL, &value_desc, NULL);
      display = action_data_get_display (data);

      if (value_desc && display)
        {
          action_message (display, G_OBJECT (brush),
                          _("Brush Shape: %s"), value_desc);
        }
    }
}

void
context_brush_radius_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      ClayBrushGenerated *generated = CLAY_BRUSH_GENERATED (brush);
      ClayDisplay        *display;
      gdouble             radius;
      gdouble             min_radius;

      radius = clay_brush_generated_get_radius (generated);

      /* If the user uses a high precision radius adjustment command
       * then we allow a minimum radius of 0.1 px, otherwise we set the
       * minimum radius to 1.0 px and adjust the radius to 1.0 px if it
       * is less than 1.0 px. This prevents irritating 0.1, 1.1, 2.1 etc
       * radius sequences when 1.0 px steps are used.
       */
      switch (select_type)
        {
        case CLAY_ACTION_SELECT_SMALL_PREVIOUS:
        case CLAY_ACTION_SELECT_SMALL_NEXT:
        case CLAY_ACTION_SELECT_PERCENT_PREVIOUS:
        case CLAY_ACTION_SELECT_PERCENT_NEXT:
          min_radius = 0.1;
          break;

        default:
          min_radius = 1.0;

          if (radius < 1.0)
            radius = 1.0;
          break;
        }

      radius = action_select_value (select_type,
                                    radius,
                                    min_radius, CLAY_BRUSH_GENERATED_MAX_RADIUS, min_radius,
                                    0.1, 1.0, 10.0, 0.05, FALSE);
      clay_brush_generated_set_radius (generated, radius);

      display = action_data_get_display (data);

      if (display)
        {
          action_message (action_data_get_display (data), G_OBJECT (brush),
                          _("Brush Radius: %2.2f"), radius);
        }
    }
}

void
context_brush_spikes_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "spikes",
                              0.0, 1.0, 4.0, 0.1, FALSE);
    }
}

void
context_brush_hardness_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "hardness",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
context_brush_aspect_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "aspect-ratio",
                              0.1, 1.0, 4.0, 0.1, FALSE);
    }
}

void
context_brush_angle_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContext          *context;
  ClayBrush            *brush;
  ClayActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  brush = clay_context_get_brush (context);

  if (CLAY_IS_BRUSH_GENERATED (brush) &&
      clay_data_is_writable (CLAY_DATA (brush)))
    {
      ClayBrushGenerated *generated = CLAY_BRUSH_GENERATED (brush);
      ClayDisplay        *display;
      gdouble             angle;

      angle = clay_brush_generated_get_angle (generated);

      if (select_type == CLAY_ACTION_SELECT_FIRST)
        angle = 0.0;
      else if (select_type == CLAY_ACTION_SELECT_LAST)
        angle = 90.0;
      else
        angle = action_select_value (select_type,
                                     angle,
                                     0.0, 180.0, 0.0,
                                     0.1, 1.0, 15.0, 0.1, TRUE);

      clay_brush_generated_set_angle (generated, angle);

      display = action_data_get_display (data);

      if (display)
        {
          action_message (action_data_get_display (data), G_OBJECT (brush),
                          _("Brush Angle: %2.2f"), angle);
        }
    }
}

void
context_toggle_dynamics_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContext   *context;
  ClayPaintInfo *paint_info;
  gboolean       enabled;

  return_if_no_context (context, data);

  paint_info = clay_context_get_paint_info (context);
  if (paint_info)
    {
      ClayPaintOptions *paint_options;
      ClayDisplay      *display;

      paint_options = paint_info->paint_options;
      enabled = clay_paint_options_are_dynamics_enabled (paint_options);

      clay_paint_options_enable_dynamics (paint_options, ! enabled);

      display = action_data_get_display (data);

      if (enabled)
        action_message (display, G_OBJECT (paint_options),
                        _("Dynamics disabled"));
      else
        action_message (display, G_OBJECT (paint_options),
                        _("Dynamics enabled"));
    }
}


/*  private functions  */

static void
context_select_object (ClayActionSelectType  select_type,
                       ClayContext          *context,
                       ClayContainer        *container)
{
  ClayObject *current;
  GType       child_type;

  child_type = clay_container_get_child_type (container);

  current = clay_context_get_by_type (context, child_type);

  current = action_select_object (select_type, container, current);

  if (current)
    clay_context_set_by_type (context, child_type, current);
}

static gint
context_paint_mode_index (ClayLayerMode        paint_mode,
                          const ClayLayerMode *modes,
                          gint                 n_modes)
{
  gint i = 0;

  while (i < (n_modes - 1) && modes[i] != paint_mode)
    i++;

  return i;
}

static void
context_select_color (ClayActionSelectType  select_type,
                      GeglColor            *color,
                      gboolean              use_colormap,
                      gboolean              use_palette)
{
  gint index;
  gint max;

  index = context_get_color_index (use_colormap, use_palette, color);
  max   = context_max_color_index (use_colormap, use_palette);

  index = action_select_value (select_type,
                               index,
                               0, max, 0,
                               0, 1, 4, 0, FALSE);

  context_set_color_index (index, use_colormap, use_palette, color);
}

static gint
context_get_color_index (gboolean   use_colormap,
                         gboolean   use_palette,
                         GeglColor *color)
{
  if (use_colormap)
    {
      ClayColormapEditor *editor = context_get_colormap_editor ();

      if (editor)
        {
          gint index = clay_colormap_editor_get_index (editor, color);

          if (index != -1)
            return index;
        }
    }

  if (use_palette)
    {
      ClayPaletteEditor *editor = context_get_palette_editor ();

      if (editor)
        {
          gint index = clay_palette_editor_get_index (editor, color);

          if (index != -1)
            return index;
        }
    }

  return 0;
}

static gint
context_max_color_index (gboolean use_colormap,
                         gboolean use_palette)
{
  if (use_colormap)
    {
      ClayColormapEditor *editor = context_get_colormap_editor ();

      if (editor)
        {
          gint index = clay_colormap_editor_max_index (editor);

          if (index != -1)
            return index;
        }
    }

  if (use_palette)
    {
      ClayPaletteEditor *editor = context_get_palette_editor ();

      if (editor)
        {
          gint index = clay_palette_editor_max_index (editor);

          if (index != -1)
            return index;
        }
    }

  return 0;
}

static gboolean
context_set_color_index (gint       index,
                         gboolean   use_colormap,
                         gboolean   use_palette,
                         GeglColor *color)
{
  if (use_colormap)
    {
      ClayColormapEditor *editor = context_get_colormap_editor ();

      if (editor && clay_colormap_editor_set_index (editor, index, color))
        return TRUE;
    }

  if (use_palette)
    {
      ClayPaletteEditor *editor = context_get_palette_editor ();

      if (editor && clay_palette_editor_set_index (editor, index, color))
        return TRUE;
    }

  return FALSE;
}

static ClayPaletteEditor *
context_get_palette_editor (void)
{
  GtkWidget *widget;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (clay_dialog_factory_get_singleton ()), NULL);

  widget = clay_dialog_factory_find_widget (clay_dialog_factory_get_singleton (),
                                            "clay-palette-editor");
  if (widget)
    return CLAY_PALETTE_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

  return NULL;
}

static ClayColormapEditor *
context_get_colormap_editor (void)
{
  GtkWidget *widget;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (clay_dialog_factory_get_singleton ()), NULL);

  widget = clay_dialog_factory_find_widget (clay_dialog_factory_get_singleton (),
                                            "clay-indexed-palette");
  if (widget)
    return CLAY_COLORMAP_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

  return NULL;
}
