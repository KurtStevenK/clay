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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"
#include "libclaywidgets/claywidgets-private.h"

#include "tools-types.h"

#include "config/clayconfig-utils.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydashpattern.h"
#include "core/claydatafactory.h"
#include "core/claypattern.h"
#include "core/claystrokeoptions.h"
#include "core/claytoolinfo.h"
#include "core/clayviewable.h"

#include "text/claytext.h"

#include "text/clayfont.h"

#include "widgets/claycolorpanel.h"
#include "widgets/claymenufactory.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claystrokeeditor.h"
#include "widgets/claytextbuffer.h"
#include "widgets/claytexteditor.h"
#include "widgets/clayviewablebox.h"
#include "widgets/claywidgets-utils.h"

#include "claytextoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_ANTIALIAS,
  PROP_HINT_STYLE,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,
  PROP_BOX_MODE,

  PROP_OUTLINE,
  PROP_OUTLINE_STYLE,       /* fill-options */
  PROP_OUTLINE_FOREGROUND,  /* context */
  PROP_OUTLINE_PATTERN,     /* context */
  PROP_OUTLINE_WIDTH,       /* stroke-options */
  PROP_OUTLINE_UNIT,
  PROP_OUTLINE_DIRECTION,
  PROP_OUTLINE_CAP_STYLE,
  PROP_OUTLINE_JOIN_STYLE,
  PROP_OUTLINE_MITER_LIMIT,
  PROP_OUTLINE_ANTIALIAS,   /* fill-options */
  PROP_OUTLINE_DASH_OFFSET,
  PROP_OUTLINE_DASH_INFO,
  PROP_USE_EDITOR,
  PROP_SHOW_ON_CANVAS_EDITOR,

  PROP_FONT_VIEW_TYPE,
  PROP_FONT_VIEW_SIZE
};


static void
             clay_text_options_config_iface_init    (ClayConfigInterface *config_iface);
static gboolean
             clay_text_options_serialize_property   (ClayConfig          *config,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec,
                                                     ClayConfigWriter    *writer);
static gboolean
             clay_text_options_deserialize_property (ClayConfig          *config,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec,
                                                     GScanner            *scanner,
                                                     GTokenType          *expected);

static void  clay_text_options_finalize             (GObject             *object);
static void  clay_text_options_set_property         (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void  clay_text_options_get_property         (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static void  clay_text_options_reset                (ClayConfig          *config);

static void  clay_text_options_notify_font          (ClayContext         *context,
                                                     GParamSpec          *pspec,
                                                     ClayText            *text);
static void  clay_text_options_notify_text_font     (ClayText            *text,
                                                     GParamSpec          *pspec,
                                                     ClayContext         *context);
static void  clay_text_options_notify_color         (ClayContext         *context,
                                                     GParamSpec          *pspec,
                                                     ClayText            *text);
static void  clay_text_options_notify_text_color    (ClayText            *text,
                                                     GParamSpec          *pspec,
                                                     ClayContext         *context);
static void  clay_text_options_outline_changed      (GtkWidget           *combo,
                                                     GtkWidget           *vbox);



G_DEFINE_TYPE_WITH_CODE (ClayTextOptions, clay_text_options,
                         CLAY_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_text_options_config_iface_init))

#define parent_class clay_text_options_parent_class

static ClayConfigInterface *parent_config_iface = NULL;


static void
clay_text_options_class_init (ClayTextOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *gray         = gegl_color_new ("gray");
  GParamSpec   *array_spec;

  object_class->finalize     = clay_text_options_finalize;
  object_class->set_property = clay_text_options_set_property;
  object_class->get_property = clay_text_options_get_property;

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "font-size-unit",
                         _("Unit"),
                         _("Font size unit"),
                         TRUE, FALSE, clay_unit_pixel (),
                         CLAY_PARAM_STATIC_STRINGS);
  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                           "font-size",
                           _("Font size"),
                           _("Font size"),
                           0.0, 8192.0, 62.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_HINT_STYLE,
                         "hint-style",
                         _("Hinting"),
                         _("Hinting alters the font outline to "
                           "produce a crisp bitmap at small "
                           "sizes"),
                         CLAY_TYPE_TEXT_HINT_STYLE,
                         CLAY_TEXT_HINT_STYLE_MEDIUM,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language",
                           _("Language"),
                           _("The text language may have an effect "
                             "on the way the text is rendered."),
                           (const gchar *) gtk_get_default_language (),
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_BASE_DIR,
                         "base-direction",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_DIRECTION,
                         CLAY_TEXT_DIRECTION_LTR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                         "justify",
                         _("Justify"),
                         _("Text alignment"),
                         CLAY_TYPE_TEXT_JUSTIFICATION,
                         CLAY_TEXT_JUSTIFY_LEFT,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_INDENTATION,
                           "indent",
                           _("Indentation"),
                           _("Indentation of the first line"),
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                           "line-spacing",
                           _("Line spacing"),
                           _("Adjust line spacing"),
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                           "letter-spacing",
                           _("Letter spacing"),
                           _("Adjust letter spacing"),
                           -8192.0, 8192.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_BOX_MODE,
                         "box-mode",
                         _("Box"),
                         _("Whether text flows into rectangular shape or "
                           "moves into a new line when you press Enter"),
                         CLAY_TYPE_TEXT_BOX_MODE,
                         CLAY_TEXT_BOX_DYNAMIC,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_EDITOR,
                            "use-editor",
                            _("Use editor window"),
                            _("Use an external editor window for text entry"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_ON_CANVAS_EDITOR,
                            "show-on-canvas",
                            _("Show on-canvas editor"),
                            _("Show on-canvas text editor"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FONT_VIEW_TYPE,
                         "font-view-type",
                         NULL, NULL,
                         CLAY_TYPE_VIEW_TYPE,
                         CLAY_VIEW_TYPE_LIST,
                         CLAY_PARAM_STATIC_STRINGS);
  CLAY_CONFIG_PROP_INT (object_class, PROP_FONT_VIEW_SIZE,
                        "font-view-size",
                        NULL, NULL,
                        CLAY_VIEW_SIZE_TINY,
                        CLAY_VIEWABLE_MAX_BUTTON_SIZE,
                        CLAY_VIEW_SIZE_SMALL,
                        CLAY_PARAM_STATIC_STRINGS);
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE,
                         "outline",
                         NULL, NULL,
                         CLAY_TYPE_TEXT_OUTLINE,
                         CLAY_TEXT_OUTLINE_NONE,
                         CLAY_PARAM_STATIC_STRINGS |
                         CLAY_CONFIG_PARAM_DEFAULTS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_STYLE,
                          "outline-custom-style",
                          NULL, NULL,
                          CLAY_TYPE_CUSTOM_STYLE,
                          CLAY_CUSTOM_STYLE_SOLID_COLOR,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_COLOR (object_class, PROP_OUTLINE_FOREGROUND,
                           "outline-foreground",
                           NULL, NULL,
                           FALSE, gray,
                           CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_OBJECT (object_class, PROP_OUTLINE_PATTERN,
                            "outline-pattern",
                            NULL, NULL,
                            CLAY_TYPE_PATTERN,
                            CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_WIDTH,
                            "outline-width",
                            _("Outline width"),
                            _("Adjust outline width"),
                            0, 8192.0, 2.0,
                            CLAY_PARAM_STATIC_STRINGS |
                            CLAY_CONFIG_PARAM_DEFAULTS);
   CLAY_CONFIG_PROP_UNIT (object_class, PROP_OUTLINE_UNIT,
                          "outline-unit",
                          _("Unit"),
                          _("Outline width unit"),
                          TRUE, FALSE, clay_unit_pixel (),
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_DIRECTION,
                          "outline-direction", NULL, NULL,
                          CLAY_TYPE_TEXT_OUTLINE_DIRECTION,
                          CLAY_TEXT_OUTLINE_DIRECTION_OUTER,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_CAP_STYLE,
                          "outline-cap-style",
                          NULL, NULL,
                          CLAY_TYPE_CAP_STYLE, CLAY_CAP_BUTT,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_JOIN_STYLE,
                          "outline-join-style",
                          NULL, NULL,
                          CLAY_TYPE_JOIN_STYLE, CLAY_JOIN_MITER,
                          CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_MITER_LIMIT,
                            "outline-miter-limit",
                            _("Outline miter limit"),
                            _("Convert a mitered join to a bevelled "
                              "join if the miter would extend to a "
                              "distance of more than miter-limit * "
                              "line-width from the actual join point."),
                            0.0, 100.0, 2.0,
                            CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_OUTLINE_ANTIALIAS,
                             "outline-antialias",
                             NULL, NULL,
                             TRUE,
                             CLAY_PARAM_STATIC_STRINGS);
   CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_DASH_OFFSET,
                            "outline-dash-offset",
                            NULL, NULL,
                            0.0, 2000.0, 0.0,
                            CLAY_PARAM_STATIC_STRINGS);

   array_spec = g_param_spec_double ("outline-dash-length",
                                     NULL, NULL,
                                     0.0, 2000.0, 1.0,
                                     CLAY_PARAM_READWRITE);
   g_object_class_install_property (object_class, PROP_OUTLINE_DASH_INFO,
                                    clay_param_spec_value_array ("outline-dash-info",
                                                                 NULL, NULL,
                                                                 array_spec,
                                                                 CLAY_PARAM_STATIC_STRINGS |
                                                                 CLAY_CONFIG_PARAM_FLAGS));

  g_object_unref (gray);
}

static void
clay_text_options_config_iface_init (ClayConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = clay_text_options_reset;

  config_iface->serialize_property   = clay_text_options_serialize_property;
  config_iface->deserialize_property = clay_text_options_deserialize_property;
}

static void
clay_text_options_init (ClayTextOptions *options)
{
  GeglColor *gray = gegl_color_new ("gray");

  options->size_entry         = NULL;
  options->outline_foreground = gray;
}

static void
clay_text_options_finalize (GObject *object)
{
  ClayTextOptions *options = CLAY_TEXT_OPTIONS (object);

  g_clear_pointer (&options->language, g_free);
  g_clear_object (&options->outline_foreground);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_text_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayTextOptions *options = CLAY_TEXT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      g_value_set_double (value, options->font_size);
      break;
    case PROP_UNIT:
      g_value_set_object (value, options->unit);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_HINT_STYLE:
      g_value_set_enum (value, options->hint_style);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, options->language);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, options->base_dir);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, options->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, options->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, options->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, options->letter_spacing);
      break;
    case PROP_BOX_MODE:
      g_value_set_enum (value, options->box_mode);
      break;

    case PROP_OUTLINE:
      g_value_set_enum (value, options->outline);
      break;
    case PROP_OUTLINE_STYLE:
      g_value_set_enum (value, options->outline_style);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_value_set_object (value, options->outline_foreground);
      break;
    case PROP_OUTLINE_PATTERN:
      g_value_set_object (value, options->outline_pattern);
      break;
    case PROP_OUTLINE_WIDTH:
      g_value_set_double (value, options->outline_width);
      break;
    case PROP_OUTLINE_UNIT:
      g_value_set_object (value, options->outline_unit);
      break;
    case PROP_OUTLINE_DIRECTION:
      g_value_set_enum (value, options->outline_direction);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      g_value_set_enum (value, options->outline_cap_style);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      g_value_set_enum (value, options->outline_join_style);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      g_value_set_double (value, options->outline_miter_limit);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      g_value_set_boolean (value, options->outline_antialias);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      g_value_set_double (value, options->outline_dash_offset);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        ClayValueArray *value_array;

        value_array = clay_dash_pattern_to_value_array (options->outline_dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;

    case PROP_USE_EDITOR:
      g_value_set_boolean (value, options->use_editor);
      break;
    case PROP_SHOW_ON_CANVAS_EDITOR:
      g_value_set_boolean (value, options->show_on_canvas);
      break;

    case PROP_FONT_VIEW_TYPE:
      g_value_set_enum (value, options->font_view_type);
      break;
    case PROP_FONT_VIEW_SIZE:
      g_value_set_int (value, options->font_view_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayTextOptions *options = CLAY_TEXT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      options->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_object (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_HINT_STYLE:
      options->hint_style = g_value_get_enum (value);
      break;
    case PROP_BASE_DIR:
      options->base_dir = g_value_get_enum (value);
      break;
    case PROP_LANGUAGE:
      g_set_str (&options->language, g_value_get_string (value));
      break;
    case PROP_JUSTIFICATION:
      options->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      options->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      options->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      options->letter_spacing = g_value_get_double (value);
      break;
    case PROP_BOX_MODE:
      options->box_mode = g_value_get_enum (value);
      break;

    case PROP_OUTLINE:
      options->outline = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_STYLE:
      options->outline_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_set_object (&options->outline_foreground, g_value_get_object (value));;
      break;
    case PROP_OUTLINE_PATTERN:
      {
        ClayPattern *pattern = g_value_get_object (value);

        if (options->outline_pattern != pattern)
          {
            if (options->outline_pattern)
              g_object_unref (options->outline_pattern);

            options->outline_pattern = pattern ? g_object_ref (pattern) : pattern;
          }
        break;
      }
    case PROP_OUTLINE_WIDTH:
      options->outline_width = g_value_get_double (value);
      break;
    case PROP_OUTLINE_UNIT:
      options->outline_unit = g_value_get_object (value);
      break;
    case PROP_OUTLINE_DIRECTION:
      options->outline_direction = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      options->outline_cap_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      options->outline_join_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      options->outline_miter_limit = g_value_get_double (value);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      options->outline_antialias = g_value_get_boolean (value);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      options->outline_dash_offset = g_value_get_double (value);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        ClayValueArray *value_array = g_value_get_boxed (value);
        options->outline_dash_info = clay_dash_pattern_from_value_array (value_array);
      }
      break;

    case PROP_USE_EDITOR:
      options->use_editor = g_value_get_boolean (value);
      break;
    case PROP_SHOW_ON_CANVAS_EDITOR:
      options->show_on_canvas = g_value_get_boolean (value);
      break;

    case PROP_FONT_VIEW_TYPE:
      options->font_view_type = g_value_get_enum (value);
      break;
    case PROP_FONT_VIEW_SIZE:
      options->font_view_size = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_options_reset (ClayConfig *config)
{
  GObject *object = G_OBJECT (config);

  /*  implement reset() ourselves because the default impl would
   *  reset *all* properties, including all rectangle properties
   *  of the text box
   */

  /* context */
  clay_config_reset_property (object, "font");
  clay_config_reset_property (object, "foreground");

  /* text options */
  clay_config_reset_property (object, "font-size");
  clay_config_reset_property (object, "font-size-unit");
  clay_config_reset_property (object, "antialias");
  clay_config_reset_property (object, "hint-style");
  clay_config_reset_property (object, "language");
  clay_config_reset_property (object, "base-direction");
  clay_config_reset_property (object, "justify");
  clay_config_reset_property (object, "indent");
  clay_config_reset_property (object, "line-spacing");
  clay_config_reset_property (object, "letter-spacing");
  clay_config_reset_property (object, "box-mode");

  clay_config_reset_property (object, "outline");
  clay_config_reset_property (object, "outline-custom-style");
  clay_config_reset_property (object, "outline-foreground");
  clay_config_reset_property (object, "outline-pattern");
  clay_config_reset_property (object, "outline-width");
  clay_config_reset_property (object, "outline-unit");
  clay_config_reset_property (object, "outline-direction");
  clay_config_reset_property (object, "outline-cap-style");
  clay_config_reset_property (object, "outline-join-style");
  clay_config_reset_property (object, "outline-miter-limit");
  clay_config_reset_property (object, "outline-antialias");
  clay_config_reset_property (object, "outline-dash-offset");
  clay_config_reset_property (object, "outline-dash-info");

  clay_config_reset_property (object, "use-editor");
  clay_config_reset_property (object, "use-on-canvas");
}

static void
clay_text_options_notify_font (ClayContext *context,
                               GParamSpec  *pspec,
                               ClayText    *text)
{
  if (clay_context_get_font (context) == text->font || clay_context_get_font (context) == NULL)
    return;

  g_signal_handlers_block_by_func (text,
                                   clay_text_options_notify_text_font,
                                   context);

  g_object_set (text, "font", clay_context_get_font (context), NULL);

  g_signal_handlers_unblock_by_func (text,
                                     clay_text_options_notify_text_font,
                                     context);
}

static void
clay_text_options_notify_text_font (ClayText    *text,
                                    GParamSpec  *pspec,
                                    ClayContext *context)
{
  if (context->font == NULL)
    return;

  g_signal_handlers_block_by_func (context,
                                   clay_text_options_notify_font, text);

  clay_context_set_font (context, text->font);

  g_signal_handlers_unblock_by_func (context,
                                     clay_text_options_notify_font, text);
}

static void
clay_text_options_notify_color (ClayContext *context,
                                GParamSpec  *pspec,
                                ClayText    *text)
{
  g_signal_handlers_block_by_func (text,
                                   clay_text_options_notify_text_color,
                                   context);

  g_object_set (text, "color", clay_context_get_foreground (context), NULL);

  g_signal_handlers_unblock_by_func (text,
                                     clay_text_options_notify_text_color,
                                     context);
}

static void
clay_text_options_notify_text_color (ClayText    *text,
                                     GParamSpec  *pspec,
                                     ClayContext *context)
{
  g_signal_handlers_block_by_func (context,
                                   clay_text_options_notify_color, text);

  clay_context_set_foreground (context, text->color);

  g_signal_handlers_unblock_by_func (context,
                                     clay_text_options_notify_color, text);
}

/*  This function could live in claytexttool.c also.
 *  But it takes some bloat out of that file...
 */
void
clay_text_options_connect_text (ClayTextOptions *options,
                                ClayText        *text)
{
  ClayContext *context;

  g_return_if_fail (CLAY_IS_TEXT_OPTIONS (options));
  g_return_if_fail (CLAY_IS_TEXT (text));

  context = CLAY_CONTEXT (options);

  clay_config_sync (G_OBJECT (options), G_OBJECT (text), 0);

  g_object_set (text,
                "color", clay_context_get_foreground (context),
                "font",  clay_context_get_font (context),
                NULL);

  clay_config_connect (G_OBJECT (options), G_OBJECT (text), NULL);

  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (clay_text_options_notify_font),
                           text, 0);
  g_signal_connect_object (text, "notify::font",
                           G_CALLBACK (clay_text_options_notify_text_font),
                           options, 0);

  g_signal_connect_object (options, "notify::foreground",
                           G_CALLBACK (clay_text_options_notify_color),
                           text, 0);
  g_signal_connect_object (text, "notify::color",
                           G_CALLBACK (clay_text_options_notify_text_color),
                           options, 0);
}

GtkWidget *
clay_text_options_gui (ClayToolOptions *tool_options)
{
  GObject           *config    = G_OBJECT (tool_options);
  ClayTextOptions   *options   = CLAY_TEXT_OPTIONS (tool_options);
  GtkWidget         *main_vbox = clay_tool_options_gui (tool_options);
  ClayAsyncSet      *async_set;
  GtkWidget         *options_vbox;
  GtkWidget         *outline_grid;
  GtkWidget         *grid;
  GtkWidget         *vbox;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;
  GtkWidget         *box;
  GtkWidget         *spinbutton;
  GtkWidget         *combo;
  GtkWidget         *editor;
  ClayStrokeOptions *stroke_options;
  GtkWidget         *outline_frame;
  GtkSizeGroup      *size_group;
  gint               row = 0;

  async_set =
    clay_data_factory_get_async_set (tool_options->tool_info->clay->font_factory);

  box = clay_busy_box_new (_("Loading fonts (this may take a while...)"));
  gtk_container_set_border_width (GTK_CONTAINER (box), 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), box, FALSE, FALSE, 0);

  g_object_bind_property (async_set, "empty",
                          box,       "visible",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL,
                              gtk_box_get_spacing (GTK_BOX (main_vbox)));
  gtk_box_pack_start (GTK_BOX (main_vbox), options_vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (options_vbox, TRUE);

  g_object_bind_property (async_set,    "empty",
                          options_vbox, "sensitive",
                          G_BINDING_SYNC_CREATE);

  hbox = clay_prop_font_box_new (NULL, CLAY_CONTEXT (tool_options),
                                 _("Font"), 2,
                                 "font-view-type", "font-view-size");
  gtk_box_pack_start (GTK_BOX (options_vbox), hbox, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  entry = clay_prop_size_entry_new (config,
                                    "font-size", FALSE, "font-size-unit", "%n",
                                    CLAY_SIZE_ENTRY_UPDATE_SIZE, 72.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Size:"), 0.0, 0.5,
                            entry, 2);

  options->size_entry = entry;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  button = clay_prop_check_button_new (config, "use-editor", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = clay_prop_check_button_new (config, "show-on-canvas", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = clay_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  row = 0;

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  button = clay_prop_enum_combo_box_new (config, "hint-style", -1, -1);
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Hinting:"), 0.0, 0.5,
                            button, 1);
  gtk_size_group_add_widget (size_group, button);

  button = clay_prop_color_button_new (config, "foreground", _("Text Color"),
                                       TRUE, 40, 24, CLAY_COLOR_AREA_FLAT);
  clay_color_button_set_update (CLAY_COLOR_BUTTON (button), TRUE);
  clay_color_panel_set_context (CLAY_COLOR_PANEL (button),
                                CLAY_CONTEXT (options));
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Color:"), 0.0, 0.5,
                            button, 1);
  gtk_size_group_add_widget (size_group, button);

  button = clay_prop_enum_combo_box_new (config, "outline", -1, -1);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                             _("Style:"), 0.0, 0.5,
                             button, 1);
  gtk_size_group_add_widget (size_group, button);

  outline_frame = clay_frame_new (_("Outline Options"));
  clay_widget_set_identifier (outline_frame, "text-outline-settings");
  gtk_box_pack_start (GTK_BOX (options_vbox), outline_frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (outline_frame, TRUE);

  g_signal_connect (button, "changed",
                    G_CALLBACK (clay_text_options_outline_changed),
                    outline_frame);
  clay_text_options_outline_changed (button, outline_frame);

  outline_grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (outline_grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (outline_grid), 2);
  gtk_container_add (GTK_CONTAINER (outline_frame), outline_grid);
  gtk_widget_set_visible (outline_grid, TRUE);

  button = clay_prop_enum_combo_box_new (config, "outline-direction", -1, -1);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (button), _("Outline Direction:"));
  clay_grid_attach_aligned (GTK_GRID (outline_grid), 0, 0,
                            NULL, 0.0, 0.5,
                            button, 1);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  row = 0;

  box = clay_prop_enum_icon_box_new (config, "justify", "format-justify", 0, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_START);
  gtk_box_set_homogeneous (GTK_BOX (box), FALSE);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Justify:"), 0.0, 0.5,
                            box, 2);
  gtk_size_group_add_widget (size_group, box);
  g_object_unref (size_group);

  spinbutton = clay_prop_spin_button_new (config, "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  clay_grid_attach_icon (GTK_GRID (grid), row++,
                         CLAY_ICON_FORMAT_INDENT_MORE,
                         spinbutton, 1);

  spinbutton = clay_prop_spin_button_new (config, "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  clay_grid_attach_icon (GTK_GRID (grid), row++,
                         CLAY_ICON_FORMAT_TEXT_SPACING_LINE,
                         spinbutton, 1);

  spinbutton = clay_prop_spin_button_new (config,
                                          "letter-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  clay_grid_attach_icon (GTK_GRID (grid), row++,
                         CLAY_ICON_FORMAT_TEXT_SPACING_LETTER,
                         spinbutton, 1);

  combo = clay_prop_enum_combo_box_new (config, "box-mode", 0, 0);
  gtk_widget_set_halign (combo, GTK_ALIGN_START);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Box:"), 0.0, 0.5,
                            combo, 1);
  stroke_options = clay_stroke_options_new (CLAY_CONTEXT (options)->clay,
                                            NULL, FALSE);
#define BIND(a)                                    \
  g_object_bind_property (options, "outline-" #a,  \
                          stroke_options, #a,                           \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
  BIND (custom-style);
  BIND (foreground);
  BIND (pattern);
  BIND (width);
  BIND (unit);
  BIND (cap-style);
  BIND (join-style);
  BIND (miter-limit);
  BIND (antialias);
  BIND (dash-offset);
  BIND (dash-info);

  editor = clay_stroke_editor_new (stroke_options, 72.0, TRUE, TRUE);
  clay_grid_attach_aligned (GTK_GRID (outline_grid), 0, 1,
                            NULL, 0.0, 0.5,
                            editor, 1);
  gtk_widget_set_visible (editor, TRUE);

  g_object_unref (stroke_options);

  /*  Only add the language entry if the iso-codes package is available.  */

#ifdef HAVE_ISO_CODES
  {
    GtkWidget *label;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start (GTK_BOX (options_vbox), vbox, FALSE, FALSE, 0);
    gtk_widget_set_visible (vbox, TRUE);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_set_visible (hbox, TRUE);

    label = gtk_label_new (_("Language:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_set_visible (label, TRUE);

    entry = clay_prop_language_entry_new (config, "language");
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  }
#endif

  return main_vbox;
}

static void
clay_text_options_editor_dir_changed (ClayTextEditor  *editor,
                                      ClayTextOptions *options)
{
  g_object_set (options,
                "base-direction", editor->base_dir,
                NULL);
}

static void
clay_text_options_editor_notify_dir (ClayTextOptions *options,
                                     GParamSpec      *pspec,
                                     ClayTextEditor  *editor)
{
  ClayTextDirection  dir;

  g_object_get (options,
                "base-direction", &dir,
                NULL);

  clay_text_editor_set_direction (editor, dir);
}

static void
clay_text_options_editor_notify_font (ClayTextOptions *options,
                                      GParamSpec      *pspec,
                                      ClayTextEditor  *editor)
{
  const gchar *font_name;

  font_name = clay_context_get_font_name (CLAY_CONTEXT (options));

  clay_text_editor_set_font_name (editor, font_name);
}

static void
clay_text_options_outline_changed (GtkWidget *combo,
                                   GtkWidget *vbox)
{
  ClayTextOutline active;

  active = (ClayTextOutline) gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (active == CLAY_TEXT_OUTLINE_NONE)
    gtk_widget_set_visible (vbox, FALSE);
  else
    gtk_widget_set_visible (vbox, TRUE);
}

GtkWidget *
clay_text_options_editor_new (GtkWindow       *parent,
                              Clay            *clay,
                              ClayTextOptions *options,
                              ClayMenuFactory *menu_factory,
                              const gchar     *title,
                              ClayText        *text,
                              ClayTextBuffer  *text_buffer,
                              gdouble          xres,
                              gdouble          yres)
{
  GtkWidget   *editor;
  const gchar *font_name;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_OPTIONS (options), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_TEXT (text), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_BUFFER (text_buffer), NULL);

  editor = clay_text_editor_new (title, parent, clay, menu_factory,
                                 text, text_buffer, xres, yres);

  font_name = clay_context_get_font_name (CLAY_CONTEXT (options));

  clay_text_editor_set_direction (CLAY_TEXT_EDITOR (editor),
                                  options->base_dir);
  clay_text_editor_set_font_name (CLAY_TEXT_EDITOR (editor),
                                  font_name);

  g_signal_connect_object (editor, "dir-changed",
                           G_CALLBACK (clay_text_options_editor_dir_changed),
                           options, 0);
  g_signal_connect_object (options, "notify::base-direction",
                           G_CALLBACK (clay_text_options_editor_notify_dir),
                           editor, 0);
  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (clay_text_options_editor_notify_font),
                           editor, 0);

  return editor;
}

static gboolean
clay_text_options_serialize_property (ClayConfig       *config,
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

  return FALSE;
}

static gboolean
clay_text_options_deserialize_property (ClayConfig *object,
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
          ClayContext   *context = CLAY_CONTEXT (object);
          ClayContainer *container;
          ClayObject    *deserialize_obj;

          if (! object_name)
            object_name = g_strdup ("");

          container = clay_data_factory_get_container (context->clay->pattern_factory);

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

  return FALSE;
}
