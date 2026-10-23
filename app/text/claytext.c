/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pango.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "text-types.h"

#include "clayfont.h"

#include "core/clay.h"
#include "core/clay-memsize.h"
#include "core/clay-utils.h"
#include "core/claycontainer.h"
#include "core/claydashpattern.h"
#include "core/claydatafactory.h"
#include "core/claystrokeoptions.h"
#include "core/claypattern.h"

#include "claytext.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_TEXT,
  PROP_MARKUP,
  PROP_FONT,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_ANTIALIAS,
  PROP_HINT_STYLE,
  PROP_KERNING,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_COLOR,
  PROP_OUTLINE,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,
  PROP_BOX_MODE,
  PROP_BOX_WIDTH,
  PROP_BOX_HEIGHT,
  PROP_OUTLINE_DIRECTION,
  PROP_BOX_UNIT,
  PROP_TRANSFORMATION,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_BORDER,

  PROP_OUTLINE_STYLE,       /* fill-options */
  PROP_OUTLINE_FOREGROUND,  /* context */
  PROP_OUTLINE_PATTERN,     /* context */
  PROP_OUTLINE_WIDTH,       /* stroke-options */
  PROP_OUTLINE_UNIT,
  PROP_OUTLINE_CAP_STYLE,
  PROP_OUTLINE_JOIN_STYLE,
  PROP_OUTLINE_MITER_LIMIT,
  PROP_OUTLINE_ANTIALIAS,   /* fill-options */
  PROP_OUTLINE_DASH_OFFSET,
  PROP_OUTLINE_DASH_INFO,
  /* for backward compatibility */
  PROP_HINTING
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

static void     clay_text_config_iface_init           (ClayConfigInterface *iface);
static gboolean clay_text_serialize_property          (ClayConfig       *config,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec,
                                                       ClayConfigWriter *writer);
static gboolean clay_text_deserialize_property        (ClayConfig       *config,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec,
                                                       GScanner         *scanner,
                                                       GTokenType       *expected);

static void     clay_text_finalize                    (GObject      *object);
static void     clay_text_get_property                (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void     clay_text_set_property                (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void     clay_text_dispatch_properties_changed (GObject      *object,
                                                       guint         n_pspecs,
                                                       GParamSpec  **pspecs);
static gint64   clay_text_get_memsize                 (ClayObject   *object,
                                                       gint64       *gui_size);


G_DEFINE_TYPE_WITH_CODE (ClayText, clay_text, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_text_config_iface_init))

#define parent_class clay_text_parent_class

static guint text_signals[LAST_SIGNAL] = { 0 };


static void
clay_text_class_init (ClayTextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  GeglColor       *black             = gegl_color_new ("black");
  GeglColor       *gray              = gegl_color_new ("gray");
  ClayMatrix2      identity;
  gchar           *language;
  GParamSpec      *array_spec;

  text_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayTextClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize                    = clay_text_finalize;
  object_class->get_property                = clay_text_get_property;
  object_class->set_property                = clay_text_set_property;
  object_class->dispatch_properties_changed = clay_text_dispatch_properties_changed;

  clay_object_class->get_memsize            = clay_text_get_memsize;

  clay_matrix2_identity (&identity);

  CLAY_CONFIG_PROP_STRING (object_class, PROP_TEXT,
                           "text",
                           NULL, NULL,
                           NULL,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_STRING (object_class, PROP_MARKUP,
                           "markup",
                           NULL, NULL,
                           NULL,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_FONT (object_class, PROP_FONT,
                         "font", NULL, NULL,
                         CLAY_CONFIG_PARAM_FLAGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                           "font-size",
                           NULL, NULL,
                           0.0, 8192.0, 24.0,
                           CLAY_PARAM_STATIC_STRINGS);

  /*  We use the name "font-size-unit" for backward compatibility.
   *  The unit is also used for other sizes in the text object.
   */
  CLAY_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "font-size-unit",
                         NULL, NULL,
                         TRUE, FALSE, clay_unit_pixel (),
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_HINT_STYLE,
                         "hint-style",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_HINT_STYLE,
                         CLAY_TEXT_HINT_STYLE_MEDIUM,
                         CLAY_PARAM_STATIC_STRINGS |
                         CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_KERNING,
                            "kerning",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS |
                            CLAY_CONFIG_PARAM_DEFAULTS);

  language = clay_get_default_language (NULL);

  CLAY_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language",
                           NULL, NULL,
                           language,
                           CLAY_PARAM_STATIC_STRINGS);

  g_free (language);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_BASE_DIR,
                         "base-direction",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_DIRECTION,
                         CLAY_TEXT_DIRECTION_LTR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_COLOR,
                          "color",
                          NULL, NULL,
                          FALSE, black,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE,
                         "outline",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_OUTLINE,
                         CLAY_TEXT_OUTLINE_NONE,
                         CLAY_PARAM_STATIC_STRINGS |
                         CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                         "justify",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_JUSTIFICATION,
                         CLAY_TEXT_JUSTIFY_LEFT,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_INDENTATION,
                           "indent",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                           "line-spacing",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                           "letter-spacing",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_BOX_MODE,
                         "box-mode",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_BOX_MODE,
                         CLAY_TEXT_BOX_DYNAMIC,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_BOX_WIDTH,
                           "box-width",
                           NULL, NULL,
                           0.0, CLAY_MAX_IMAGE_SIZE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_BOX_HEIGHT,
                           "box-height",
                           NULL, NULL,
                           0.0, CLAY_MAX_IMAGE_SIZE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_BOX_UNIT,
                         "box-unit",
                         NULL, NULL,
                         TRUE, FALSE, clay_unit_pixel (),
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_MATRIX2 (object_class, PROP_TRANSFORMATION,
                            "transformation",
                            NULL, NULL,
                            &identity,
                            CLAY_PARAM_STATIC_STRINGS |
                            CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OFFSET_X,
                           "offset-x",
                           NULL, NULL,
                           -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OFFSET_Y,
                           "offset-y",
                           NULL, NULL,
                           -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  /*  border does only exist to implement the old text API  */
  g_object_class_install_property (object_class, PROP_BORDER,
                                   g_param_spec_int ("border", NULL, NULL,
                                                     0, CLAY_MAX_IMAGE_SIZE, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     CLAY_PARAM_WRITABLE));

   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_STYLE,
                          "outline-custom-style", NULL, NULL,
                          CLAY_TYPE_CUSTOM_STYLE,
                          CLAY_CUSTOM_STYLE_SOLID_COLOR,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_OBJECT (object_class, PROP_OUTLINE_PATTERN,
                            "outline-pattern", NULL, NULL,
                            CLAY_TYPE_PATTERN,
                            CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_COLOR (object_class, PROP_OUTLINE_FOREGROUND,
                           "outline-foreground", NULL, NULL,
                           FALSE, gray,
                           CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_WIDTH,
                            "outline-width", NULL, NULL,
                            0.0, 8192.0, 4.0,
                            CLAY_PARAM_STATIC_STRINGS |
                            CLAY_CONFIG_PARAM_DEFAULTS);
   CLAY_CONFIG_PROP_UNIT (object_class, PROP_OUTLINE_UNIT,
                          "outline-unit",
                          NULL, NULL,
                          TRUE, FALSE, clay_unit_pixel (),
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_DIRECTION,
                          "outline-direction", NULL, NULL,
                          CLAY_TYPE_TEXT_OUTLINE_DIRECTION,
                          CLAY_TEXT_OUTLINE_DIRECTION_OUTER,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_CAP_STYLE,
                          "outline-cap-style", NULL, NULL,
                          CLAY_TYPE_CAP_STYLE, CLAY_CAP_BUTT,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_JOIN_STYLE,
                          "outline-join-style", NULL, NULL,
                          CLAY_TYPE_JOIN_STYLE, CLAY_JOIN_MITER,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_MITER_LIMIT,
                            "outline-miter-limit",
                            NULL, NULL,
                            0.0, 100.0, 2.0,
                            CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_OUTLINE_ANTIALIAS,
                             "outline-antialias", NULL, NULL,
                             TRUE,
                             CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_DASH_OFFSET,
                            "outline-dash-offset", NULL, NULL,
                            0.0, 2000.0, 0.0,
                            CLAY_PARAM_STATIC_STRINGS);

   array_spec = g_param_spec_double ("outline-dash-length", NULL, NULL,
                                     0.0, 2000.0, 1.0, CLAY_PARAM_READWRITE);
   g_object_class_install_property (object_class, PROP_OUTLINE_DASH_INFO,
                                    clay_param_spec_value_array ("outline-dash-info",
                                                                 NULL, NULL,
                                                                 array_spec,
                                                                 CLAY_PARAM_STATIC_STRINGS |
                                                                 CLAY_CONFIG_PARAM_FLAGS));

  /*  the old hinting options have been replaced by 'hint-style'  */
  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_HINTING,
                            "hinting",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_unref (black);
  g_object_unref (gray);
}

static void
clay_text_init (ClayText *text)
{
  text->color              = gegl_color_new ("black");
  text->outline_foreground = gegl_color_new ("gray");
}

static void
clay_text_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize_property   = clay_text_serialize_property;
  iface->deserialize_property = clay_text_deserialize_property;
}

static void
clay_text_finalize (GObject *object)
{
  ClayText *text = CLAY_TEXT (object);

  g_clear_pointer (&text->text,     g_free);
  g_clear_pointer (&text->markup,   g_free);
  g_clear_pointer (&text->language, g_free);
  g_clear_object (&text->font);
  g_clear_object (&text->color);
  g_clear_object (&text->outline_foreground);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_text_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  ClayText *text = CLAY_TEXT (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, text->text);
      break;
    case PROP_MARKUP:
      g_value_set_string (value, text->markup);
      break;
    case PROP_FONT:
      g_value_set_object (value, text->font);
      break;
    case PROP_FONT_SIZE:
      g_value_set_double (value, text->font_size);
      break;
    case PROP_UNIT:
      g_value_set_object (value, text->unit);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, text->antialias);
      break;
    case PROP_HINT_STYLE:
      g_value_set_enum (value, text->hint_style);
      break;
    case PROP_KERNING:
      g_value_set_boolean (value, text->kerning);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, text->base_dir);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, text->language);
      break;
    case PROP_COLOR:
      g_value_set_object (value, text->color);
      break;
    case PROP_OUTLINE:
      g_value_set_enum (value, text->outline);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, text->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, text->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, text->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, text->letter_spacing);
      break;
    case PROP_BOX_MODE:
      g_value_set_enum (value, text->box_mode);
      break;
    case PROP_BOX_WIDTH:
      g_value_set_double (value, text->box_width);
      break;
    case PROP_BOX_HEIGHT:
      g_value_set_double (value, text->box_height);
      break;
    case PROP_BOX_UNIT:
      g_value_set_object (value, text->box_unit);
      break;
    case PROP_TRANSFORMATION:
      g_value_set_boxed (value, &text->transformation);
      break;
    case PROP_OFFSET_X:
      g_value_set_double (value, text->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, text->offset_y);
      break;
    case PROP_OUTLINE_STYLE:
      g_value_set_enum (value, text->outline_style);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_value_set_object (value, text->outline_foreground);
      break;
    case PROP_OUTLINE_PATTERN:
      g_value_set_object (value, text->outline_pattern);
      break;
    case PROP_OUTLINE_WIDTH:
      g_value_set_double (value, text->outline_width);
      break;
    case PROP_OUTLINE_UNIT:
      g_value_set_object (value, text->outline_unit);
      break;
    case PROP_OUTLINE_DIRECTION:
      g_value_set_enum (value, text->outline_direction);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      g_value_set_enum (value, text->outline_cap_style);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      g_value_set_enum (value, text->outline_join_style);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      g_value_set_double (value, text->outline_miter_limit);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      g_value_set_boolean (value, text->outline_antialias);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      g_value_set_double (value, text->outline_dash_offset);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        ClayValueArray *value_array;

        value_array = clay_dash_pattern_to_value_array (text->outline_dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;
    case PROP_HINTING:
      g_value_set_boolean (value,
                           text->hint_style != CLAY_TEXT_HINT_STYLE_NONE);
      break;
    case PROP_CLAY:
      g_value_set_object (value, text->clay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayText    *text = CLAY_TEXT (object);
  ClayMatrix2 *matrix;

  switch (property_id)
    {
    case PROP_TEXT:
      g_set_str (&text->text, g_value_get_string (value));
      if (text->text && text->markup)
        {
          g_clear_pointer (&text->markup, g_free);
          g_object_notify (object, "markup");
        }
      break;
    case PROP_MARKUP:
      g_set_str (&text->markup, g_value_get_string (value));
      if (text->markup && text->text)
        {
          g_clear_pointer (&text->text, g_free);
          g_object_notify (object, "text");
        }
      break;
    case PROP_FONT:
      {
        ClayFont *font = g_value_get_object (value);

        if (font != text->font && font != NULL)
          g_set_object (&text->font, font);
        /* this is defensive to avoid some crashes */
        else if (font == NULL)
          g_set_object (&text->font, CLAY_FONT (clay_font_get_standard ()));
      }
      break;
    case PROP_FONT_SIZE:
      text->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      text->unit = g_value_get_object (value);
      break;
    case PROP_ANTIALIAS:
      text->antialias = g_value_get_boolean (value);
      break;
    case PROP_HINT_STYLE:
      text->hint_style = g_value_get_enum (value);
      break;
    case PROP_KERNING:
      text->kerning = g_value_get_boolean (value);
      break;
    case PROP_LANGUAGE:
      g_set_str (&text->language, g_value_get_string (value));
      break;
    case PROP_BASE_DIR:
      text->base_dir = g_value_get_enum (value);
      break;
    case PROP_COLOR:
      g_set_object (&text->color, g_value_get_object (value));;
      break;
    case PROP_OUTLINE:
      text->outline = g_value_get_enum (value);
      break;
    case PROP_JUSTIFICATION:
      text->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      text->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      text->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      text->letter_spacing = g_value_get_double (value);
      break;
    case PROP_BOX_MODE:
      text->box_mode = g_value_get_enum (value);
      break;
    case PROP_BOX_WIDTH:
      text->box_width = g_value_get_double (value);
      break;
    case PROP_BOX_HEIGHT:
      text->box_height = g_value_get_double (value);
      break;
    case PROP_BOX_UNIT:
      text->box_unit = g_value_get_object (value);
      break;
    case PROP_TRANSFORMATION:
      matrix = g_value_get_boxed (value);
      text->transformation = *matrix;
      break;
    case PROP_OFFSET_X:
      text->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      text->offset_y = g_value_get_double (value);
      break;
    case PROP_OUTLINE_STYLE:
      text->outline_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_set_object (&text->outline_foreground, g_value_get_object (value));;
      break;
    case PROP_OUTLINE_PATTERN:
      {
        ClayPattern *pattern = g_value_get_object (value);

        if (text->outline_pattern != pattern)
          {
            if (text->outline_pattern)
              g_object_unref (text->outline_pattern);

            text->outline_pattern = pattern ? g_object_ref (pattern) : pattern;
          }
        break;
      }
    case PROP_OUTLINE_WIDTH:
      text->outline_width = g_value_get_double (value);
      break;
    case PROP_OUTLINE_UNIT:
      text->outline_unit = g_value_get_object (value);
      break;
    case PROP_OUTLINE_DIRECTION:
      text->outline_direction = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      text->outline_cap_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      text->outline_join_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      text->outline_miter_limit = g_value_get_double (value);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      text->outline_antialias = g_value_get_boolean (value);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      text->outline_dash_offset = g_value_get_double (value);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        ClayValueArray *value_array = g_value_get_boxed (value);
        text->outline_dash_info = clay_dash_pattern_from_value_array (value_array);
      }
      break;
    case PROP_BORDER:
      text->border = g_value_get_int (value);
      break;
    case PROP_HINTING:
      /* interpret "hinting" only if "hint-style" has its default
       * value, so we don't overwrite a serialized new hint-style with
       * a compat "hinting" that is only there for old Clay versions
       */
      if (text->hint_style == CLAY_TEXT_HINT_STYLE_MEDIUM)
        text->hint_style = (g_value_get_boolean (value) ?
                            CLAY_TEXT_HINT_STYLE_MEDIUM :
                            CLAY_TEXT_HINT_STYLE_NONE);
      break;
    case PROP_CLAY:
      text->clay = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_dispatch_properties_changed (GObject     *object,
                                       guint        n_pspecs,
                                       GParamSpec **pspecs)
{
  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  g_signal_emit (object, text_signals[CHANGED], 0);
}

static gint64
clay_text_get_memsize (ClayObject *object,
                       gint64     *gui_size)
{
  ClayText *text    = CLAY_TEXT (object);
  gint64    memsize = 0;

  memsize += clay_string_get_memsize (text->text);
  memsize += clay_string_get_memsize (text->markup);
  memsize += clay_string_get_memsize (text->language);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
clay_text_get_transformation (ClayText    *text,
                              ClayMatrix3 *matrix)
{
  g_return_if_fail (CLAY_IS_TEXT (text));
  g_return_if_fail (matrix != NULL);

  matrix->coeff[0][0] = text->transformation.coeff[0][0];
  matrix->coeff[0][1] = text->transformation.coeff[0][1];
  matrix->coeff[0][2] = text->offset_x;

  matrix->coeff[1][0] = text->transformation.coeff[1][0];
  matrix->coeff[1][1] = text->transformation.coeff[1][1];
  matrix->coeff[1][2] = text->offset_y;

  matrix->coeff[2][0] = 0.0;
  matrix->coeff[2][1] = 0.0;
  matrix->coeff[2][2] = 1.0;
}

static gboolean
clay_text_serialize_property (ClayConfig       *config,
                              guint             property_id,
                              const GValue     *value,
                              GParamSpec       *pspec,
                              ClayConfigWriter *writer)
{
  if (property_id == PROP_OUTLINE_PATTERN)
    {
      ClayObject *serialize_obj = g_value_get_object (value);

      clay_config_writer_open (writer, pspec->name);

      if (serialize_obj)
        clay_config_writer_string (writer, clay_object_get_name (serialize_obj));
      else
        clay_config_writer_print (writer, "NULL", 4);

      clay_config_writer_close (writer);

      return TRUE;
    }
  else if (property_id == PROP_MARKUP)
    {
      gchar         *markup = (gchar*)g_value_get_string (value);
      GRegex        *regex;
      ClayText      *text;
      ClayContainer *container;
      PangoAttrList *attr_list;
      ClayFont      *font;
      guint          length;
      GSList        *list   = NULL;
      GSList        *fonts  = NULL;

      g_return_val_if_fail (CLAY_IS_TEXT (config), FALSE);

      if (markup == NULL)
        return FALSE;

      text      = CLAY_TEXT (config);
      container = clay_data_factory_get_container (text->clay->font_factory);

      /*lookupname format is "clayfont%d" we keep only the "font%d" part
       * this is to avoid problems when deserializing
       * e.g. if  there are 2 fonts with lookupname clayfont17 and clayfont23
       * we might replace the first with a font whose lookupname is clayfont23,
       * and we might replace the original clayfont23 with say clayfont29
       * this means that all occurences of clayfont23 turned into clayfont29
       */
      regex  = g_regex_new ("\"clayfont(\\d+)\"", 0, 0, NULL);
      markup = g_regex_replace (regex, markup, -1, 0, "\"font\\1\"", 0, NULL);

      clay_config_writer_open   (writer, "markup");
      clay_config_writer_string (writer, markup);

      pango_parse_markup (markup, -1, 0, &attr_list, NULL, NULL, NULL);

      list   = pango_attr_list_get_attributes (attr_list);
      length = g_slist_length (list);

      for (guint i = 0; i < length; ++i)
        {
          PangoAttrFontDesc *attr_font_desc = pango_attribute_as_font_desc ((PangoAttribute*)g_slist_nth_data (list, i));

          if (attr_font_desc != NULL)
            {
              gchar *altered_font_name = pango_font_description_to_string (attr_font_desc->desc);
              gchar *font_name         = g_strdup_printf ("clay%s", altered_font_name);

              if (g_slist_find_custom (fonts, (gconstpointer) font_name, (GCompareFunc) g_strcmp0) == NULL)
                {
                  fonts = g_slist_prepend (fonts, (gpointer) font_name);

                  if (g_str_has_prefix (altered_font_name, "font"))
                    font = CLAY_FONT (clay_container_search (container,
                                                             (ClayContainerSearchFunc) clay_font_match_by_lookup_name,
                                                             (gpointer) font_name));
                  /* in case the font is an alias its lookupname is that alias and doesn't conform to the format "clayfont%d"*/
                  else
                    font = CLAY_FONT (clay_container_search (container,
                                                             (ClayContainerSearchFunc) clay_font_match_by_lookup_name,
                                                             (gpointer) altered_font_name));
                  /* in case pango returns a non existant font name */
                  if (font == NULL)
                    {
                      font = CLAY_FONT (clay_font_get_standard ());
                      font_name = "clayfont";
                    }

                  clay_config_writer_open   (writer, "markupfont");
                  /*lookupname format is "clayfont%d" we keep only the "font%d",
                   * and in case the font is an alias, we keep its name (see the above comments)*/
                  clay_config_writer_string (writer, font_name+4);

                  clay_config_writer_open   (writer, "font");
                  CLAY_CONFIG_GET_IFACE     (CLAY_CONFIG (font))->serialize (CLAY_CONFIG (font),
                                             writer,
                                             NULL);
                  clay_config_writer_close  (writer);
                  clay_config_writer_close  (writer);
                }

              else
                {
                  g_free (font_name);
                }
              g_free (altered_font_name);
            }
        }

      clay_config_writer_close  (writer);

      g_slist_free_full (fonts, (GDestroyNotify) g_free);
      g_slist_free_full (list,  (GDestroyNotify) pango_attribute_destroy);
      pango_attr_list_unref (attr_list);
      g_free (markup);
      g_regex_unref (regex);

      return TRUE;
    }

  return FALSE;
}

static gboolean
clay_text_deserialize_property (ClayConfig *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec,
                                GScanner   *scanner,
                                GTokenType *expected)
{
  if (property_id == PROP_OUTLINE_PATTERN)
    {
      gchar *object_name;

      if (clay_scanner_parse_identifier (scanner, "NULL"))
        {
          g_value_set_object (value, NULL);
        }
      else if (clay_scanner_parse_string (scanner, &object_name))
        {
          ClayText      *text = CLAY_TEXT (object);
          ClayContainer *container;
          ClayObject    *deserialize_obj;

          if (! object_name)
            object_name = g_strdup ("");

          container = clay_data_factory_get_container (text->clay->pattern_factory);

          deserialize_obj = clay_container_get_child_by_name (container,
                                                              object_name);

          g_value_set_object (value, deserialize_obj);

          g_free (object_name);
        }
      else
        {
          *expected = G_TOKEN_STRING;
        }

      return TRUE;
    }
  else if (property_id == PROP_OUTLINE_DASH_INFO)
    {
      if (clay_scanner_parse_identifier (scanner, "NULL"))
        {
          g_value_take_boxed (value, NULL);
          return TRUE;
        }
    }
  else if (property_id == PROP_MARKUP)
    {
      gchar    *markup       = NULL;
      GString  *markup_str;
      ClayFont *dummy_object = g_object_new (CLAY_TYPE_FONT, NULL);

      clay_scanner_parse_string (scanner, &markup);

      markup_str = g_string_new (markup);

      /* This is for backward compatibility with older xcf files.*/
      if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
        {
          while (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
            {
              gchar    *markup_fontname = NULL;
              gchar    *replaced_markup;
              gchar    *new_markup;
              ClayFont *font;

              clay_scanner_parse_string (scanner, &markup_fontname);

              font = CLAY_FONT (CLAY_CONFIG_GET_IFACE (dummy_object)->deserialize_create (CLAY_TYPE_FONT,
                                                                                          scanner,
                                                                                          -1,
                                                                                          NULL));
              replaced_markup = g_markup_printf_escaped (" font=\"%s\"", markup_fontname);
              new_markup      = g_strdup_printf (" clayfont=\"%s\"", clay_font_get_lookup_name (font));
              g_string_replace (markup_str, replaced_markup, new_markup, 0);

              g_free (markup_fontname);
              g_free (replaced_markup);
              g_free (new_markup);
              g_object_unref (font);
            }
          /* We avoid the edge case when actual fonts are called "clayfont%d"
           * and their replacement name chains to each other by marking already
           * processed font as "clayfont=". Then we clean up this fake attribute
           * name in the end.
           * This is not a problem with the new format as font are stored as
           * "font%d" (see comment in clay_text_serialize_property()).
           */
          g_string_replace (markup_str, " clayfont=\"", " font=\"", 0);
        }
      else
        {
          while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
            {
              gchar    *lookupname = NULL;
              gchar    *replaced_markup;
              gchar    *new_markup;
              ClayFont *font;

              g_scanner_get_next_token  (scanner); /* ( */
              g_scanner_get_next_token  (scanner); /* "lookupname" */
              clay_scanner_parse_string (scanner, &lookupname);

              g_scanner_get_next_token  (scanner); /* ) */
              g_scanner_get_next_token  (scanner); /* font */

              font = CLAY_FONT (CLAY_CONFIG_GET_IFACE (dummy_object)->deserialize_create (CLAY_TYPE_FONT,
                                                                                          scanner,
                                                                                          -1,
                                                                                          NULL));
              g_scanner_get_next_token  (scanner); /* ) */
              g_scanner_get_next_token  (scanner); /* ) */

              replaced_markup = g_markup_printf_escaped (" font=\"%s\"", lookupname);
              new_markup      = g_strdup_printf (" font=\"%s\"", clay_font_get_lookup_name (font));
              g_string_replace (markup_str, replaced_markup, new_markup, 0);

              g_free (lookupname);
              g_free (replaced_markup);
              g_free (new_markup);
              g_object_unref (font);
            }
        }

      g_value_set_string (value, markup_str->str);

      g_object_unref (dummy_object);
      g_string_free  (markup_str, TRUE);
      g_free (markup);

      return TRUE;
    }

  return FALSE;
}
