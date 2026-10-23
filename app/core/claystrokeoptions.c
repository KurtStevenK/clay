/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claystrokeoptions.c
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "paint/claypaintoptions.h"

#include "path/clayvectorlayer.h"
#include "path/clayvectorlayeroptions.h"

#include "clay.h"
#include "claycontext.h"
#include "claydashpattern.h"
#include "claypaintinfo.h"
#include "clayparamspecs.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "claystrokeoptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,

  PROP_METHOD,

  PROP_STYLE,
  PROP_WIDTH,
  PROP_UNIT,
  PROP_CAP_STYLE,
  PROP_JOIN_STYLE,
  PROP_MITER_LIMIT,
  PROP_ANTIALIAS,
  PROP_DASH_UNIT,
  PROP_DASH_OFFSET,
  PROP_DASH_INFO,

  PROP_PAINT_OPTIONS,
  PROP_EMULATE_DYNAMICS
};

enum
{
  DASH_INFO_CHANGED,
  LAST_SIGNAL
};


typedef struct _ClayStrokeOptionsPrivate ClayStrokeOptionsPrivate;

struct _ClayStrokeOptionsPrivate
{
  ClayStrokeMethod  method;

  /*  options for method == LIBART  */
  gdouble           width;
  ClayUnit         *unit;

  ClayCapStyle      cap_style;
  ClayJoinStyle     join_style;

  gdouble           miter_limit;

  gdouble           dash_offset;
  GArray           *dash_info;

  /*  options for method == PAINT_TOOL  */
  ClayPaintOptions *paint_options;
  gboolean          emulate_dynamics;
};

#define GET_PRIVATE(options) \
        ((ClayStrokeOptionsPrivate *) clay_stroke_options_get_instance_private ((ClayStrokeOptions *) (options)))


static void         clay_stroke_options_config_iface_init  (gpointer              iface,
                                                            gpointer              iface_data);
static void         clay_stroke_options_savable_iface_init (ClaySavableInterface *iface);

static void         clay_stroke_options_finalize           (GObject              *object);
static void         clay_stroke_options_set_property       (GObject              *object,
                                                            guint                 property_id,
                                                            const GValue         *value,
                                                            GParamSpec           *pspec);
static void         clay_stroke_options_get_property       (GObject              *object,
                                                            guint                 property_id,
                                                            GValue               *value,
                                                            GParamSpec           *pspec);

static ClayConfig * clay_stroke_options_duplicate          (ClayConfig           *config);

static void         clay_stroke_options_save               (ClaySavable          *savable,
                                                            ClaySaveState        *state);

static gboolean     clay_stroke_options_load_enter         (ClayLoadState        *state,
                                                            const gchar         **attribute_names,
                                                            const gchar         **attribute_values,
                                                            gpointer              user_data,
                                                            GError              **error);
static gboolean     clay_stroke_options_load_exit          (ClayLoadState        *state,
                                                            const gchar          *text,
                                                            gsize                 len,
                                                            gpointer              user_data,
                                                            GError              **error);
static gboolean     clay_stroke_options_enter_dash_pattern (ClayLoadState        *state,
                                                            const gchar         **attribute_names,
                                                            const gchar         **attribute_values,
                                                            gpointer              user_data,
                                                            GError              **error);
static gboolean     clay_stroke_options_exit_dash_pattern  (ClayLoadState        *state,
                                                            const gchar          *text,
                                                            gsize                 len,
                                                            gpointer              user_data,
                                                            GError              **error);


G_DEFINE_TYPE_WITH_CODE (ClayStrokeOptions, clay_stroke_options,
                         CLAY_TYPE_FILL_OPTIONS,
                         G_ADD_PRIVATE (ClayStrokeOptions)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_stroke_options_config_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_stroke_options_savable_iface_init))

#define parent_class clay_stroke_options_parent_class

static ClayConfigInterface  *parent_config_iface  = NULL;
static ClaySavableInterface *parent_savable_iface = NULL;

static guint stroke_options_signals[LAST_SIGNAL] = { 0 };


static void
clay_stroke_options_class_init (ClayStrokeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  object_class->finalize     = clay_stroke_options_finalize;
  object_class->set_property = clay_stroke_options_set_property;
  object_class->get_property = clay_stroke_options_get_property;

  klass->dash_info_changed = NULL;

  stroke_options_signals[DASH_INFO_CHANGED] =
    g_signal_new ("dash-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayStrokeOptionsClass, dash_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DASH_PRESET);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_METHOD,
                         "method",
                         _("Method"),
                         NULL,
                         CLAY_TYPE_STROKE_METHOD,
                         CLAY_STROKE_LINE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_WIDTH,
                           "width",
                           _("Line width"),
                           NULL,
                           0.0, 2000.0, 6.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "unit",
                         _("Unit"),
                         NULL,
                         TRUE, FALSE, clay_unit_pixel (),
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CAP_STYLE,
                         "cap-style",
                         _("Cap style"),
                         NULL,
                         CLAY_TYPE_CAP_STYLE, CLAY_CAP_BUTT,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_JOIN_STYLE,
                         "join-style",
                         _("Join style"),
                         NULL,
                         CLAY_TYPE_JOIN_STYLE, CLAY_JOIN_MITER,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_MITER_LIMIT,
                           "miter-limit",
                           _("Miter limit"),
                           _("Convert a mitered join to a bevelled "
                             "join if the miter would extend to a "
                             "distance of more than miter-limit * "
                             "line-width from the actual join point."),
                           0.0, 100.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_DASH_OFFSET,
                           "dash-offset",
                           _("Dash offset"),
                           NULL,
                           0.0, 2000.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS);

  array_spec = g_param_spec_double ("dash-length", NULL, NULL,
                                    0.0, 2000.0, 1.0, CLAY_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DASH_INFO,
                                   clay_param_spec_value_array ("dash-info",
                                                                NULL, NULL,
                                                                array_spec,
                                                                CLAY_PARAM_STATIC_STRINGS |
                                                                CLAY_CONFIG_PARAM_FLAGS));

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_PAINT_OPTIONS,
                           "paint-options",
                           NULL, NULL,
                           CLAY_TYPE_PAINT_OPTIONS,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_EMULATE_DYNAMICS,
                            "emulate-brush-dynamics",
                            _("Emulate brush dynamics"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_stroke_options_config_iface_init (gpointer  iface,
                                       gpointer  iface_data)
{
  ClayConfigInterface *config_iface = (ClayConfigInterface *) iface;

  parent_config_iface = g_type_interface_peek_parent (config_iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (CLAY_TYPE_CONFIG);

  config_iface->duplicate = clay_stroke_options_duplicate;
}

static void
clay_stroke_options_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "stroke-options";
  iface->save       = clay_stroke_options_save;
  iface->load_enter = clay_stroke_options_load_enter;
  iface->load_exit  = clay_stroke_options_load_exit;
}

static void
clay_stroke_options_init (ClayStrokeOptions *options)
{
}

static void
clay_stroke_options_finalize (GObject *object)
{
  ClayStrokeOptionsPrivate *private = GET_PRIVATE (object);

  if (private->dash_info)
    {
      clay_dash_pattern_free (private->dash_info);
      private->dash_info = NULL;
    }

  g_clear_object (&private->paint_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_stroke_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayStrokeOptions        *options = CLAY_STROKE_OPTIONS (object);
  ClayStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      private->method = g_value_get_enum (value);
      break;

    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_UNIT:
      private->unit = g_value_get_object (value);
      break;
    case PROP_CAP_STYLE:
      private->cap_style = g_value_get_enum (value);
      break;
    case PROP_JOIN_STYLE:
      private->join_style = g_value_get_enum (value);
      break;
    case PROP_MITER_LIMIT:
      private->miter_limit = g_value_get_double (value);
      break;
    case PROP_DASH_OFFSET:
      private->dash_offset = g_value_get_double (value);
      break;
    case PROP_DASH_INFO:
      {
        ClayValueArray *value_array = g_value_get_boxed (value);
        GArray         *pattern;

        pattern = clay_dash_pattern_from_value_array (value_array);
        clay_stroke_options_take_dash_pattern (options, CLAY_DASH_CUSTOM,
                                               pattern);
      }
      break;

    case PROP_PAINT_OPTIONS:
      g_set_object (&private->paint_options, g_value_get_object (value));
      break;
    case PROP_EMULATE_DYNAMICS:
      private->emulate_dynamics = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_stroke_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayStrokeOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_METHOD:
      g_value_set_enum (value, private->method);
      break;

    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_UNIT:
      g_value_set_object (value, private->unit);
      break;
    case PROP_CAP_STYLE:
      g_value_set_enum (value, private->cap_style);
      break;
    case PROP_JOIN_STYLE:
      g_value_set_enum (value, private->join_style);
      break;
    case PROP_MITER_LIMIT:
      g_value_set_double (value, private->miter_limit);
      break;
    case PROP_DASH_OFFSET:
      g_value_set_double (value, private->dash_offset);
      break;
    case PROP_DASH_INFO:
      {
        ClayValueArray *value_array;

        value_array = clay_dash_pattern_to_value_array (private->dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;

    case PROP_PAINT_OPTIONS:
      g_value_set_object (value, private->paint_options);
      break;
    case PROP_EMULATE_DYNAMICS:
      g_value_set_boolean (value, private->emulate_dynamics);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static ClayConfig *
clay_stroke_options_duplicate (ClayConfig *config)
{
  ClayStrokeOptions        *options = CLAY_STROKE_OPTIONS (config);
  ClayStrokeOptionsPrivate *private = GET_PRIVATE (options);
  ClayStrokeOptions        *new_options;

  new_options = CLAY_STROKE_OPTIONS (parent_config_iface->duplicate (config));

  if (private->paint_options)
    {
      GObject *paint_options;

      paint_options = clay_config_duplicate (CLAY_CONFIG (private->paint_options));
      g_object_set (new_options, "paint-options", paint_options, NULL);
      g_object_unref (paint_options);
    }

  return CLAY_CONFIG (new_options);
}

static void
clay_stroke_options_save (ClaySavable   *savable,
                          ClaySaveState *state)
{
  ClayStrokeOptions        *options = CLAY_STROKE_OPTIONS (savable);
  ClayStrokeOptionsPrivate *private = GET_PRIVATE (options);
  GArray                   *dash_info;
  ClayValueArray           *dash_value_array;

  clay_savable_print_element_start (state, "stroke-options", NULL);

  parent_savable_iface->save (savable, state);

  clay_savable_print_element (state, "line-width", "%f", private->width, NULL);
  clay_savable_print_element (state, "cap-style", "%[ClayCapStyle]", private->cap_style, NULL);
  clay_savable_print_element (state, "join-style", "%[ClayJoinStyle]", private->join_style, NULL);
  clay_savable_print_element (state, "miter-limit", "%f", private->miter_limit, NULL);

  dash_info        = clay_stroke_options_get_dash_info (options);
  dash_value_array = clay_dash_pattern_to_value_array (dash_info);
  if (dash_value_array)
    {
      clay_savable_print_element_start (state, "dash-pattern", NULL, NULL, NULL);
      clay_savable_value_array_save (dash_value_array, state);
      clay_savable_print_element_end (state, "dash-pattern");
    }

  clay_savable_print_element_end (state, "stroke-options");

  g_clear_pointer (&dash_value_array, clay_value_array_unref);
}

static gboolean
clay_stroke_options_load_enter (ClayLoadState  *state,
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

  parent_savable_iface->load_enter (state, attribute_names, attribute_values,
                                    user_data, error);

  clay_savable_load_add_simple_handler (state, "line-width", G_TYPE_DOUBLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "cap-style", CLAY_TYPE_CAP_STYLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "join-style", CLAY_TYPE_JOIN_STYLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "miter-limit", G_TYPE_DOUBLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_handlers (state, "dash-pattern",
                                  clay_stroke_options_enter_dash_pattern,
                                  clay_stroke_options_exit_dash_pattern,
                                  NULL, NULL);

  return TRUE;
}

static gboolean
clay_stroke_options_load_exit (ClayLoadState  *state,
                               const gchar    *text,
                               gsize           len,
                               gpointer        user_data,
                               GError        **error)
{
  ClayVectorLayer          *layer;
  ClayStrokeOptions        *options;
  ClayStrokeOptionsPrivate *private;

  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  parent_savable_iface->load_exit (state, text, len, user_data, error);

  layer   = CLAY_VECTOR_LAYER (clay_savable_load_peek_active_object (state));
  options = layer->options->stroke_options;
  private = GET_PRIVATE (options);

  clay_savable_load_get_values (state,
                                "line-width",  &private->width,
                                "cap-style",   &private->cap_style,
                                "join-style",  &private->join_style,
                                "miter-limit", &private->miter_limit,
                                NULL);

  return TRUE;
}

static gboolean
clay_stroke_options_enter_dash_pattern (ClayLoadState  *state,
                                        const gchar   **attribute_names,
                                        const gchar   **attribute_values,
                                        gpointer        user_data,
                                        GError        **error)
{
  clay_savable_load_add_handlers (state, "value-array",
                                  clay_savable_enter_value_array,
                                  clay_savable_exit_value_array,
                                  NULL, NULL);
  return TRUE;
}

static gboolean
clay_stroke_options_exit_dash_pattern (ClayLoadState  *state,
                                       const gchar    *text,
                                       gsize           len,
                                       gpointer        user_data,
                                       GError        **error)
{
  ClayValueArray    *array;
  ClayVectorLayer   *layer;
  ClayStrokeOptions *options;

  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  if (clay_savable_load_get_values (state, "value-array", &array, NULL))
    {
      GArray *pattern;

      clay_value_array_ref (array);

      layer   = CLAY_VECTOR_LAYER (clay_savable_load_peek_active_object (state));
      options = layer->options->stroke_options;

      pattern = clay_dash_pattern_from_value_array (array);
      clay_stroke_options_take_dash_pattern (options, CLAY_DASH_CUSTOM, pattern);
    }
  return TRUE;
}


/*  public functions  */

ClayStrokeOptions *
clay_stroke_options_new (Clay        *clay,
                         ClayContext *context,
                         gboolean     use_context_color)
{
  ClayPaintInfo     *paint_info = NULL;
  ClayStrokeOptions *options;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  if (context)
    paint_info = clay_context_get_paint_info (context);

  if (! paint_info)
    paint_info = clay_paint_info_get_standard (clay);

  options = g_object_new (CLAY_TYPE_STROKE_OPTIONS,
                          "clay",       clay,
                          "paint-info", paint_info,
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

ClayStrokeMethod
clay_stroke_options_get_method (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options),
                        CLAY_STROKE_LINE);

  return GET_PRIVATE (options)->method;
}

gdouble
clay_stroke_options_get_width (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->width;
}

ClayUnit *
clay_stroke_options_get_unit (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), clay_unit_pixel ());

  return GET_PRIVATE (options)->unit;
}

ClayCapStyle
clay_stroke_options_get_cap_style (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), CLAY_CAP_BUTT);

  return GET_PRIVATE (options)->cap_style;
}

ClayJoinStyle
clay_stroke_options_get_join_style (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), CLAY_JOIN_MITER);

  return GET_PRIVATE (options)->join_style;
}

gdouble
clay_stroke_options_get_miter_limit (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), 1.0);

  return GET_PRIVATE (options)->miter_limit;
}

gdouble
clay_stroke_options_get_dash_offset (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), 0.0);

  return GET_PRIVATE (options)->dash_offset;
}

GArray *
clay_stroke_options_get_dash_info (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->dash_info;
}

ClayPaintOptions *
clay_stroke_options_get_paint_options (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), NULL);

  return GET_PRIVATE (options)->paint_options;
}

gboolean
clay_stroke_options_get_emulate_dynamics (ClayStrokeOptions *options)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->emulate_dynamics;
}

/**
 * clay_stroke_options_take_dash_pattern:
 * @options: a #ClayStrokeOptions object
 * @preset: a value out of the #ClayDashPreset enum
 * @pattern: a #GArray or %NULL if @preset is not %CLAY_DASH_CUSTOM
 *
 * Sets the dash pattern. Either a @preset is passed and @pattern is
 * %NULL or @preset is %CLAY_DASH_CUSTOM and @pattern is the #GArray
 * to use as the dash pattern. Note that this function takes ownership
 * of the passed pattern.
 */
void
clay_stroke_options_take_dash_pattern (ClayStrokeOptions *options,
                                       ClayDashPreset     preset,
                                       GArray            *pattern)
{
  ClayStrokeOptionsPrivate *private;

  g_return_if_fail (CLAY_IS_STROKE_OPTIONS (options));
  g_return_if_fail (preset == CLAY_DASH_CUSTOM || pattern == NULL);

  private = GET_PRIVATE (options);

  if (preset != CLAY_DASH_CUSTOM)
    pattern = clay_dash_pattern_new_from_preset (preset);

  if (private->dash_info)
    clay_dash_pattern_free (private->dash_info);

  private->dash_info = pattern;

  g_object_notify (G_OBJECT (options), "dash-info");

  g_signal_emit (options, stroke_options_signals [DASH_INFO_CHANGED], 0,
                 preset);
}

void
clay_stroke_options_prepare (ClayStrokeOptions *options,
                             ClayContext       *context,
                             ClayPaintOptions  *paint_options)
{
  ClayStrokeOptionsPrivate *private;

  g_return_if_fail (CLAY_IS_STROKE_OPTIONS (options));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (paint_options == NULL ||
                    CLAY_IS_PAINT_OPTIONS (paint_options));

  private = GET_PRIVATE (options);

  switch (private->method)
    {
    case CLAY_STROKE_LINE:
      break;

    case CLAY_STROKE_PAINT_METHOD:
      {
        ClayPaintInfo *paint_info = CLAY_CONTEXT (options)->paint_info;

        if (paint_options)
          {
            g_return_if_fail (paint_info == paint_options->paint_info);

            /*  undefine the paint-relevant context properties and get them
             *  from the passed context
             */
            clay_context_define_properties (CLAY_CONTEXT (paint_options),
                                            CLAY_CONTEXT_PROP_MASK_PAINT,
                                            FALSE);
            clay_context_set_parent (CLAY_CONTEXT (paint_options), context);

            g_object_ref (paint_options);
          }
        else
          {
            ClayCoreConfig      *config       = context->clay->config;
            ClayContextPropMask  global_props = 0;

            paint_options =
              clay_config_duplicate (CLAY_CONFIG (paint_info->paint_options));

            /*  FG and BG are always shared between all tools  */
            global_props |= CLAY_CONTEXT_PROP_MASK_FOREGROUND;
            global_props |= CLAY_CONTEXT_PROP_MASK_BACKGROUND;

            if (config->global_brush)
              global_props |= CLAY_CONTEXT_PROP_MASK_BRUSH;
            if (config->global_dynamics)
              global_props |= CLAY_CONTEXT_PROP_MASK_DYNAMICS;
            if (config->global_pattern)
              global_props |= CLAY_CONTEXT_PROP_MASK_PATTERN;
            if (config->global_palette)
              global_props |= CLAY_CONTEXT_PROP_MASK_PALETTE;
            if (config->global_gradient)
              global_props |= CLAY_CONTEXT_PROP_MASK_GRADIENT;
            if (config->global_font)
              global_props |= CLAY_CONTEXT_PROP_MASK_FONT;

            clay_context_copy_properties (context,
                                          CLAY_CONTEXT (paint_options),
                                          global_props);
          }

        g_object_set (options, "paint-options", paint_options, NULL);
        g_object_unref (paint_options);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

void
clay_stroke_options_finish (ClayStrokeOptions *options)
{
  g_return_if_fail (CLAY_IS_STROKE_OPTIONS (options));

  g_object_set (options, "paint-options", NULL, NULL);
}
