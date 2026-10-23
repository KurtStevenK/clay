/* The Clay -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayfilloptions.c
 * Copyright (C) 2003 Simon Budig
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "path/clayvectorlayer.h"
#include "path/clayvectorlayeroptions.h"

#include "clay.h"
#include "clay-utils.h"
#include "clay-palettes.h"
#include "claydrawable.h"
#include "claydrawable-fill.h"
#include "clayerror.h"
#include "clayfilloptions.h"
#include "clayimage.h"
#include "claypattern.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_CUSTOM_STYLE,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE,
  PROP_UPDATE_COLOR_HISTORY
};


typedef struct _ClayFillOptionsPrivate ClayFillOptionsPrivate;

struct _ClayFillOptionsPrivate
{
  ClayFillStyle   style;
  ClayCustomStyle custom_style;
  gboolean        antialias;
  gboolean        feather;
  gdouble         feather_radius;

  ClayViewType    pattern_view_type;
  ClayViewSize    pattern_view_size;

  gboolean        update_color_history;

  const gchar    *undo_desc;
};

#define GET_PRIVATE(options) \
        ((ClayFillOptionsPrivate *) clay_fill_options_get_instance_private ((ClayFillOptions *) (options)))


static void     clay_fill_options_config_init        (ClayConfigInterface  *iface);
static void     clay_fill_options_savable_iface_init (ClaySavableInterface *iface);

static void     clay_fill_options_set_property       (GObject              *object,
                                                      guint                 property_id,
                                                      const GValue         *value,
                                                      GParamSpec           *pspec);
static void     clay_fill_options_get_property       (GObject              *object,
                                                      guint                 property_id,
                                                      GValue               *value,
                                                      GParamSpec           *pspec);

static gboolean clay_fill_options_serialize          (ClayConfig           *config,
                                                      ClayConfigWriter     *writer,
                                                      gpointer              data);

static void     clay_fill_options_savable_save       (ClaySavable          *savable,
                                                      ClaySaveState        *state);

static gboolean clay_fill_options_load_enter         (ClayLoadState  *state,
                                                      const gchar   **attribute_names,
                                                      const gchar   **attribute_values,
                                                      gpointer        user_data,
                                                      GError        **error);
static gboolean clay_fill_options_load_exit          (ClayLoadState  *state,
                                                      const gchar    *text,
                                                      gsize           len,
                                                      gpointer        user_data,
                                                      GError        **error);


G_DEFINE_TYPE_WITH_CODE (ClayFillOptions, clay_fill_options, CLAY_TYPE_CONTEXT,
                         G_ADD_PRIVATE (ClayFillOptions)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_fill_options_config_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_fill_options_savable_iface_init))


static void
clay_fill_options_class_init (ClayFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_fill_options_set_property;
  object_class->get_property = clay_fill_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Style"),
                         NULL,
                         CLAY_TYPE_FILL_STYLE,
                         CLAY_FILL_STYLE_FG_COLOR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CUSTOM_STYLE,
                         "custom-style",
                         _("Custom style"),
                         NULL,
                         CLAY_TYPE_CUSTOM_STYLE,
                         CLAY_CUSTOM_STYLE_SOLID_COLOR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            NULL,
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

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_TYPE,
                                   g_param_spec_enum ("pattern-view-type",
                                                      NULL, NULL,
                                                      CLAY_TYPE_VIEW_TYPE,
                                                      CLAY_VIEW_TYPE_GRID,
                                                      G_PARAM_CONSTRUCT |
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_SIZE,
                                   g_param_spec_int ("pattern-view-size",
                                                     NULL, NULL,
                                                     CLAY_VIEW_SIZE_TINY,
                                                     CLAY_VIEWABLE_MAX_BUTTON_SIZE,
                                                     CLAY_VIEW_SIZE_SMALL,
                                                     G_PARAM_CONSTRUCT |
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_UPDATE_COLOR_HISTORY,
                                   g_param_spec_boolean ("update-color-history",
                                                         NULL,
                                                         NULL,
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT |
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_fill_options_config_init (ClayConfigInterface *iface)
{
  iface->serialize = clay_fill_options_serialize;
}

static void
clay_fill_options_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "fill-options";
  iface->save       = clay_fill_options_savable_save;
  iface->load_enter = clay_fill_options_load_enter;
  iface->load_exit  = clay_fill_options_load_exit;
}

static void
clay_fill_options_init (ClayFillOptions *options)
{
}

static void
clay_fill_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      private->style = g_value_get_enum (value);
      private->undo_desc = NULL;
      break;
    case PROP_CUSTOM_STYLE:
      private->custom_style = g_value_get_enum (value);
      private->undo_desc = NULL;
      break;
    case PROP_ANTIALIAS:
      private->antialias = g_value_get_boolean (value);
      break;
    case PROP_FEATHER:
      private->feather = g_value_get_boolean (value);
      break;
    case PROP_FEATHER_RADIUS:
      private->feather_radius = g_value_get_double (value);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      private->pattern_view_type = g_value_get_enum (value);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      private->pattern_view_size = g_value_get_int (value);
      break;

    case PROP_UPDATE_COLOR_HISTORY:
      private->update_color_history = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_fill_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, private->style);
      break;
    case PROP_CUSTOM_STYLE:
      g_value_set_enum (value, private->custom_style);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, private->antialias);
      break;
    case PROP_FEATHER:
      g_value_set_boolean (value, private->feather);
      break;
    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, private->feather_radius);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      g_value_set_enum (value, private->pattern_view_type);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      g_value_set_int (value, private->pattern_view_size);
      break;

    case PROP_UPDATE_COLOR_HISTORY:
      g_value_set_boolean (value, private->update_color_history);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_fill_options_serialize (ClayConfig       *config,
                             ClayConfigWriter *writer,
                             gpointer          data)
{
  return clay_config_serialize_properties (config, writer);
}

static void
clay_fill_options_savable_save (ClaySavable   *savable,
                                ClaySaveState *state)
{
  ClayFillOptions        *options = CLAY_FILL_OPTIONS (savable);
  ClayFillOptionsPrivate *private = GET_PRIVATE (options);
  GeglColor              *color;
  ClayPattern            *pattern;

  if (G_TYPE_FROM_INSTANCE (savable) == CLAY_TYPE_FILL_OPTIONS)
    clay_savable_print_element_start (state, "fill-options", NULL);

  clay_savable_print_element (state, "style",
                              "%[ClayCustomStyle]", private->custom_style, NULL);
  clay_savable_print_element (state, "antialias", "%b", private->antialias, NULL);

  color = clay_context_get_foreground (CLAY_CONTEXT (options));
  if (color)
    clay_savable_color_save (color, NULL, NULL, state);

  pattern = clay_context_get_pattern (CLAY_CONTEXT (options));
  if (pattern)
    clay_savable_save (CLAY_SAVABLE (pattern), state);

  if (G_TYPE_FROM_INSTANCE (savable) == CLAY_TYPE_FILL_OPTIONS)
    clay_savable_print_element_end (state, "fill-options");
}

static gboolean
clay_fill_options_load_enter (ClayLoadState  *state,
                              const gchar   **attribute_names,
                              const gchar   **attribute_values,
                              gpointer        user_data,
                              GError        **error)
{
  while (*attribute_names)
    {
      CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_FORMAT, ;,
                               "%s: unexpected attribute: '%s'",
                               G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  clay_savable_load_add_simple_handler (state, "style", CLAY_TYPE_CUSTOM_STYLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "antialias", G_TYPE_BOOLEAN,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_handlers (state, "color",
                                  clay_savable_enter_color,
                                  clay_savable_exit_color,
                                  NULL, NULL);

  clay_savable_load (CLAY_TYPE_PATTERN, state);

  return TRUE;
}

static gboolean
clay_fill_options_load_exit (ClayLoadState  *state,
                             const gchar    *text,
                             gsize           len,
                             gpointer        user_data,
                             GError        **error)
{
  ClayVectorLayer *layer;
  ClayFillOptions *options;
  GeglColor       *color;
  ClayPattern     *pattern;
  ClayCustomStyle  style;
  gboolean         antialias;
  GType            real_type;

  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  layer     = CLAY_VECTOR_LAYER (clay_savable_load_peek_active_object (state));
  real_type = user_data ? CLAYPOINTER_TO_TYPE (user_data) : CLAY_TYPE_FILL_OPTIONS;

  if (real_type == CLAY_TYPE_FILL_OPTIONS)
    options = layer->options->fill_options;
  else
    options = CLAY_FILL_OPTIONS (layer->options->stroke_options);

  if (clay_savable_load_get_values (state, "style", &style, NULL))
    clay_fill_options_set_custom_style (options, style);

  if (clay_savable_load_get_values (state, "antialias", &antialias, NULL))
    clay_fill_options_set_antialias (options, antialias);

  if (clay_savable_load_get_values (state, "color", &color, NULL))
    clay_context_set_foreground (CLAY_CONTEXT (options), color);

  if (clay_savable_load_get_values (state, "pattern", &pattern, NULL))
    clay_context_set_pattern (CLAY_CONTEXT (options), pattern);

  return TRUE;
}


/*  public functions  */

ClayFillOptions *
clay_fill_options_new (Clay        *clay,
                       ClayContext *context,
                       gboolean     use_context_color)
{
  ClayFillOptions *options;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  options = g_object_new (CLAY_TYPE_FILL_OPTIONS,
                          "clay", clay,
                          NULL);

  if (use_context_color)
    {
      clay_context_define_properties (CLAY_CONTEXT (options),
                                      CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                                      CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                                      CLAY_CONTEXT_PROP_MASK_PATTERN,
                                      FALSE);

      clay_context_set_parent (CLAY_CONTEXT (options), context);
    }

  return options;
}

ClayFillStyle
clay_fill_options_get_style (ClayFillOptions *options)
{
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), CLAY_FILL_STYLE_FG_COLOR);

  return GET_PRIVATE (options)->style;
}

void
clay_fill_options_set_style (ClayFillOptions *options,
                             ClayFillStyle    style)
{
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  g_object_set (options, "style", style, NULL);
}

ClayCustomStyle
clay_fill_options_get_custom_style (ClayFillOptions *options)
{
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options),
                        CLAY_CUSTOM_STYLE_SOLID_COLOR);

  return GET_PRIVATE (options)->custom_style;
}

void
clay_fill_options_set_custom_style (ClayFillOptions *options,
                                    ClayCustomStyle  custom_style)
{
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  g_object_set (options, "custom-style", custom_style, NULL);
}

gboolean
clay_fill_options_get_antialias (ClayFillOptions *options)
{
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->antialias;
}

void
clay_fill_options_set_antialias (ClayFillOptions *options,
                                 gboolean         antialias)
{
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  g_object_set (options, "antialias", antialias, NULL);
}

gboolean
clay_fill_options_get_feather (ClayFillOptions *options,
                               gdouble         *radius)
{
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), FALSE);

  if (radius)
    *radius = GET_PRIVATE (options)->feather_radius;

  return GET_PRIVATE (options)->feather;
}

void
clay_fill_options_set_feather (ClayFillOptions *options,
                               gboolean         feather,
                               gdouble          radius)
{
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  g_object_set (options, "feather", feather, NULL);
  g_object_set (options, "feather-radius", radius, NULL);
}

gboolean
clay_fill_options_set_by_fill_type (ClayFillOptions  *options,
                                    ClayContext      *context,
                                    ClayFillType      fill_type,
                                    GError          **error)
{
  ClayFillOptionsPrivate *private;
  GeglColor              *color = NULL;
  const gchar            *undo_desc;

  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GET_PRIVATE (options);

  private->undo_desc = NULL;

  switch (fill_type)
    {
    case CLAY_FILL_FOREGROUND:
      color = gegl_color_duplicate (clay_context_get_foreground (context));
      undo_desc = C_("undo-type", "Fill with Foreground Color");
      break;

    case CLAY_FILL_BACKGROUND:
      color = gegl_color_duplicate (clay_context_get_background (context));
      undo_desc = C_("undo-type", "Fill with Background Color");
      break;

    case CLAY_FILL_CIELAB_MIDDLE_GRAY:
      {
        const float cielab_pixel[3] = {50, 0, 0};

        color = gegl_color_new (NULL);
        gegl_color_set_pixel (color, babl_format ("CIE Lab float"), cielab_pixel);

        undo_desc = C_("undo-type", "Fill with Middle Gray (CIELAB) Color");
      }
      break;

    case CLAY_FILL_WHITE:
      color = gegl_color_new ("white");
      undo_desc = C_("undo-type", "Fill with White");
      break;

    case CLAY_FILL_TRANSPARENT:
      color = gegl_color_duplicate (clay_context_get_background (context));
      clay_context_set_paint_mode (CLAY_CONTEXT (options),
                                   CLAY_LAYER_MODE_ERASE);
      undo_desc = C_("undo-type", "Fill with Transparency");
      break;

    case CLAY_FILL_PATTERN:
      {
        ClayPattern *pattern = clay_context_get_pattern (context);

        if (! pattern)
          {
            g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                 _("No patterns available for this operation."));
            return FALSE;
          }

        clay_fill_options_set_style (options, CLAY_FILL_STYLE_PATTERN);
        clay_context_set_pattern (CLAY_CONTEXT (options), pattern);
        private->undo_desc = C_("undo-type", "Fill with Pattern");

        return TRUE;
      }
      break;

    default:
      g_warning ("%s: invalid fill_type %d", G_STRFUNC, fill_type);
      return FALSE;
    }

  g_return_val_if_fail (color != NULL, FALSE);

  clay_fill_options_set_style (options, CLAY_FILL_STYLE_FG_COLOR);
  clay_context_set_foreground (CLAY_CONTEXT (options), color);
  private->undo_desc = undo_desc;

  g_object_unref (color);

  return TRUE;
}

gboolean
clay_fill_options_set_by_fill_mode (ClayFillOptions     *options,
                                    ClayContext         *context,
                                    ClayBucketFillMode   fill_mode,
                                    GError             **error)
{
  ClayFillType fill_type;

  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  switch (fill_mode)
    {
    default:
    case CLAY_BUCKET_FILL_FG:
      fill_type = CLAY_FILL_FOREGROUND;
      break;

    case CLAY_BUCKET_FILL_BG:
      fill_type = CLAY_FILL_BACKGROUND;
      break;

    case CLAY_BUCKET_FILL_PATTERN:
      fill_type = CLAY_FILL_PATTERN;
      break;
    }

  return clay_fill_options_set_by_fill_type (options, context,
                                             fill_type, error);
}

const gchar *
clay_fill_options_get_undo_desc (ClayFillOptions *options)
{
  ClayFillOptionsPrivate *private;

  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);

  private = GET_PRIVATE (options);

  if (private->undo_desc)
    return private->undo_desc;

  switch (private->style)
    {
    case CLAY_FILL_STYLE_FG_COLOR:
      return C_("undo-type", "Fill with Foreground Color");

    case CLAY_FILL_STYLE_BG_COLOR:
      return C_("undo-type", "Fill with Background Color");

    case CLAY_FILL_STYLE_PATTERN:
      return C_("undo-type", "Fill with Pattern");
    }

  g_return_val_if_reached (NULL);
}

const Babl *
clay_fill_options_get_format (ClayFillOptions *options,
                              ClayDrawable    *drawable)
{
  ClayContext *context;

  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  context = CLAY_CONTEXT (options);

  return clay_layer_mode_get_format (clay_context_get_paint_mode (context),
                                     CLAY_LAYER_COLOR_SPACE_AUTO,
                                     CLAY_LAYER_COLOR_SPACE_AUTO,
                                     clay_layer_mode_get_paint_composite_mode (
                                       clay_context_get_paint_mode (context)),
                                     clay_drawable_get_format (drawable));
}

GeglBuffer *
clay_fill_options_create_buffer (ClayFillOptions     *options,
                                 ClayDrawable        *drawable,
                                 const GeglRectangle *rect,
                                 gint                 pattern_offset_x,
                                 gint                 pattern_offset_y)
{
  GeglBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (clay_fill_options_get_style (options) !=
                        CLAY_FILL_STYLE_PATTERN ||
                        clay_context_get_pattern (CLAY_CONTEXT (options)) != NULL,
                        NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = gegl_buffer_new (rect,
                            clay_fill_options_get_format (options, drawable));

  clay_fill_options_fill_buffer (options, drawable, buffer,
                                 pattern_offset_x, pattern_offset_y);

  return buffer;
}

void
clay_fill_options_fill_buffer (ClayFillOptions *options,
                               ClayDrawable    *drawable,
                               GeglBuffer      *buffer,
                               gint             pattern_offset_x,
                               gint             pattern_offset_y)
{
  ClayFillOptionsPrivate *priv;

  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));
  g_return_if_fail (clay_fill_options_get_style (options) !=
                    CLAY_FILL_STYLE_PATTERN ||
                    clay_context_get_pattern (CLAY_CONTEXT (options)) != NULL);
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  priv = GET_PRIVATE (options);

  switch (clay_fill_options_get_style (options))
    {
    case CLAY_FILL_STYLE_FG_COLOR:
      {
        GeglColor *color;

        color = clay_context_get_foreground (CLAY_CONTEXT (options));
        if (priv->update_color_history)
          clay_palettes_add_color_history (CLAY_CONTEXT (options)->clay, color);

        clay_drawable_fill_buffer (drawable, buffer, color, NULL, 0, 0);
      }
      break;

    case CLAY_FILL_STYLE_BG_COLOR:
      {
        GeglColor *color;

        color = clay_context_get_background (CLAY_CONTEXT (options));
        if (priv->update_color_history)
          clay_palettes_add_color_history (CLAY_CONTEXT (options)->clay, color);

        clay_drawable_fill_buffer (drawable, buffer, color, NULL, 0, 0);
      }
      break;

    case CLAY_FILL_STYLE_PATTERN:
      {
        ClayPattern *pattern;

        pattern = clay_context_get_pattern (CLAY_CONTEXT (options));

        clay_drawable_fill_buffer (drawable, buffer,
                                   NULL, pattern,
                                   pattern_offset_x,
                                   pattern_offset_y);
      }
      break;
    }
}

void
clay_fill_options_enable_color_history (ClayFillOptions *options,
                                        gboolean         enable)
{
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  g_object_set (options,
                "update-color-history", enable,
                NULL);
}
