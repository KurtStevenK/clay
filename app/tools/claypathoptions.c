/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypathoptions.c
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydatafactory.h"
#include "core/claypattern.h"
#include "core/claystrokeoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claystrokeeditor.h"
#include "widgets/claywidgets-utils.h"

#include "claypathoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_PATH_EDIT_MODE,
  PROP_PATH_POLYGONAL,
  PROP_PATH_AUTO_CONNECT,
  PROP_ENABLE_FILL,
  PROP_ENABLE_STROKE,
  PROP_FILL_STYLE,
  PROP_FILL_FOREGROUND,
  PROP_FILL_PATTERN,
  PROP_FILL_ANTIALIAS,
  PROP_STROKE_STYLE,
  PROP_STROKE_FOREGROUND,
  PROP_STROKE_PATTERN,
  PROP_STROKE_ANTIALIAS,
  PROP_STROKE_WIDTH,
  PROP_STROKE_UNIT,
  PROP_STROKE_CAP_STYLE,
  PROP_STROKE_JOIN_STYLE,
  PROP_STROKE_MITER_LIMIT,
  PROP_STROKE_DASH_OFFSET
};

static void      clay_path_options_config_iface_init    (ClayConfigInterface *config_iface);
static gboolean  clay_path_options_serialize_property   (ClayConfig          *config,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec,
                                                         ClayConfigWriter    *writer);
static gboolean  clay_path_options_deserialize_property (ClayConfig          *config,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec,
                                                         GScanner            *scanner,
                                                         GTokenType          *expected);

static void      clay_path_options_finalize             (GObject             *object);
static void      clay_path_options_set_property         (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void      clay_path_options_get_property         (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);


G_DEFINE_TYPE_WITH_CODE (ClayPathOptions, clay_path_options,
                         CLAY_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_path_options_config_iface_init))


#define parent_class clay_path_options_parent_class

static ClayConfigInterface *parent_config_iface = NULL;


static void
clay_path_options_class_init (ClayPathOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *black        = gegl_color_new ("black");
  GeglColor    *white        = gegl_color_new ("white");

  object_class->finalize     = clay_path_options_finalize;
  object_class->set_property = clay_path_options_set_property;
  object_class->get_property = clay_path_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_PATH_EDIT_MODE,
                         "path-edit-mode",
                         _("Edit Mode"),
                         NULL,
                         CLAY_TYPE_PATH_MODE,
                         CLAY_PATH_MODE_DESIGN,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_PATH_POLYGONAL,
                            "path-polygonal",
                            _("Polygonal"),
                            _("Restrict editing to polygons"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_PATH_POLYGONAL,
                            "path-auto-connect",
                            _("Auto-connect on click"),
                            _("If checked, clicking on the end node in "
                              "Design mode will close the path"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  /* Vector layer specific properties */
  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_FILL,
                            "enable-fill",
                            _("Enable Fill"), NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_STROKE,
                            "enable-stroke",
                            _("Enable Stroke"), NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FILL_STYLE,
                         "fill-custom-style",
                         NULL, NULL,
                         CLAY_TYPE_CUSTOM_STYLE,
                         CLAY_CUSTOM_STYLE_SOLID_COLOR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_FILL_FOREGROUND,
                          "fill-foreground",
                          NULL, NULL,
                          TRUE, white,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_FILL_PATTERN,
                           "fill-pattern",
                           NULL, NULL,
                           CLAY_TYPE_PATTERN,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_ANTIALIAS,
                            "fill-antialias",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STROKE_STYLE,
                         "stroke-custom-style",
                         NULL, NULL,
                         CLAY_TYPE_CUSTOM_STYLE,
                         CLAY_CUSTOM_STYLE_SOLID_COLOR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_STROKE_FOREGROUND,
                          "stroke-foreground",
                          NULL, NULL,
                          TRUE, black,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_PATTERN,
                           "stroke-pattern",
                           NULL, NULL,
                           CLAY_TYPE_PATTERN,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_ANTIALIAS,
                            "stroke-antialias",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_WIDTH,
                           "stroke-width",
                           NULL, NULL,
                           0.0, 2000.0, 6.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_STROKE_UNIT,
                         "stroke-unit",
                         NULL, NULL,
                         TRUE, FALSE, clay_unit_pixel (),
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STROKE_CAP_STYLE,
                         "stroke-cap-style",
                         NULL, NULL,
                         CLAY_TYPE_CAP_STYLE, CLAY_CAP_BUTT,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STROKE_JOIN_STYLE,
                         "stroke-join-style",
                         NULL, NULL,
                         CLAY_TYPE_JOIN_STYLE, CLAY_JOIN_MITER,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_MITER_LIMIT,
                           "stroke-miter-limit",
                           NULL, NULL,
                           0.0, 100.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_DASH_OFFSET,
                           "stroke-dash-offset",
                           NULL, NULL,
                           0.0, 2000.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS);

  g_clear_object (&white);
  g_clear_object (&black);
}

static void
clay_path_options_config_iface_init (ClayConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->serialize_property   = clay_path_options_serialize_property;
  config_iface->deserialize_property = clay_path_options_deserialize_property;
}

static void
clay_path_options_init (ClayPathOptions *options)
{
  GeglColor *black = gegl_color_new ("black");
  GeglColor *white = gegl_color_new ("white");

  options->fill_foreground   = white;
  options->stroke_foreground = black;
}

static void
clay_path_options_finalize (GObject *object)
{
  ClayPathOptions *options = CLAY_PATH_OPTIONS (object);

  g_clear_object (&options->fill_options);
  g_clear_object (&options->stroke_options);
  g_clear_object (&options->fill_foreground);
  g_clear_object (&options->stroke_foreground);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_path_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayPathOptions *options = CLAY_PATH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PATH_EDIT_MODE:
      options->edit_mode = g_value_get_enum (value);
      break;
    case PROP_PATH_POLYGONAL:
      options->polygonal = g_value_get_boolean (value);
      break;
    case PROP_PATH_AUTO_CONNECT:
      options->auto_connect = g_value_get_boolean (value);
      break;

    case PROP_ENABLE_FILL:
      options->enable_fill = g_value_get_boolean (value);
      break;
    case PROP_ENABLE_STROKE:
      options->enable_stroke = g_value_get_boolean (value);
      break;

    case PROP_FILL_STYLE:
      options->fill_style = g_value_get_enum (value);
      break;
    case PROP_FILL_FOREGROUND:
      g_set_object (&options->fill_foreground, g_value_get_object (value));
      break;
    case PROP_FILL_PATTERN:
      g_set_object (&options->fill_pattern, g_value_get_object (value));
      break;
    case PROP_FILL_ANTIALIAS:
      options->fill_antialias = g_value_get_boolean (value);
      break;

    case PROP_STROKE_STYLE:
      options->stroke_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_FOREGROUND:
      g_set_object (&options->stroke_foreground, g_value_get_object (value));
      break;
    case PROP_STROKE_PATTERN:
      g_set_object (&options->stroke_pattern, g_value_get_object (value));
      break;
    case PROP_STROKE_ANTIALIAS:
      options->stroke_antialias = g_value_get_boolean (value);
      break;

    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_double (value);
      break;
    case PROP_STROKE_UNIT:
      options->stroke_unit = g_value_get_object (value);
      break;
    case PROP_STROKE_CAP_STYLE:
      options->stroke_cap_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_JOIN_STYLE:
      options->stroke_join_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_MITER_LIMIT:
      options->stroke_miter_limit = g_value_get_double (value);
      break;
    case PROP_STROKE_DASH_OFFSET:
      options->stroke_dash_offset = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_path_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayPathOptions *options = CLAY_PATH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PATH_EDIT_MODE:
      g_value_set_enum (value, options->edit_mode);
      break;
    case PROP_PATH_POLYGONAL:
      g_value_set_boolean (value, options->polygonal);
      break;
    case PROP_PATH_AUTO_CONNECT:
      g_value_set_boolean (value, options->auto_connect);
      break;

    case PROP_ENABLE_FILL:
      g_value_set_boolean (value, options->enable_fill);
      break;
    case PROP_ENABLE_STROKE:
      g_value_set_boolean (value, options->enable_stroke);
      break;

    case PROP_FILL_STYLE:
      g_value_set_enum (value, options->fill_style);
      break;
    case PROP_FILL_FOREGROUND:
      g_value_set_object (value, options->fill_foreground);
      break;
    case PROP_FILL_PATTERN:
      g_value_set_object (value, options->fill_pattern);
      break;
    case PROP_FILL_ANTIALIAS:
      g_value_set_boolean (value, options->fill_antialias);
      break;

    case PROP_STROKE_STYLE:
      g_value_set_enum (value, options->stroke_style);
      break;
    case PROP_STROKE_FOREGROUND:
      g_value_set_object (value, options->stroke_foreground);
      break;
    case PROP_STROKE_PATTERN:
      g_value_set_object (value, options->stroke_pattern);
      break;
    case PROP_STROKE_ANTIALIAS:
      g_value_set_boolean (value, options->stroke_antialias);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_double (value, options->stroke_width);
      break;
    case PROP_STROKE_UNIT:
      g_value_set_object (value, options->stroke_unit);
      break;
    case PROP_STROKE_CAP_STYLE:
      g_value_set_enum (value, options->stroke_cap_style);
      break;
    case PROP_STROKE_JOIN_STYLE:
      g_value_set_enum (value, options->stroke_join_style);
      break;
    case PROP_STROKE_MITER_LIMIT:
      g_value_set_double (value, options->stroke_miter_limit);
      break;
    case PROP_STROKE_DASH_OFFSET:
      g_value_set_double (value, options->stroke_dash_offset);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_path_options_serialize_property (ClayConfig       *config,
                                      guint             property_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec,
                                      ClayConfigWriter *writer)
{
  if (property_id == PROP_FILL_PATTERN ||
      property_id == PROP_STROKE_PATTERN)
    {
      ClayObject *serialize_obj = g_value_get_object (value);

      clay_config_writer_open (writer, pspec->name);

      if (serialize_obj)
        clay_config_writer_string (writer,
                                   clay_object_get_name (serialize_obj));
      else
        clay_config_writer_print (writer, "NULL", 4);

      clay_config_writer_close (writer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
clay_path_options_deserialize_property (ClayConfig *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec,
                                        GScanner   *scanner,
                                        GTokenType *expected)
{
  if (property_id == PROP_FILL_PATTERN ||
      property_id == PROP_STROKE_PATTERN)
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

          container =
            clay_data_factory_get_container (context->clay->pattern_factory);

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

static void
button_append_modifier (GtkWidget       *button,
                        GdkModifierType  modifiers)
{
  gchar *str = g_strdup_printf ("%s (%s)",
                                gtk_button_get_label (GTK_BUTTON (button)),
                                clay_get_mod_string (modifiers));

  gtk_button_set_label (GTK_BUTTON (button), str);
  g_free (str);
}

GtkWidget *
clay_path_options_gui (ClayToolOptions *tool_options)
{
  GObject           *config  = G_OBJECT (tool_options);
  ClayPathOptions   *options = CLAY_PATH_OPTIONS (tool_options);
  GtkWidget         *vbox    = clay_tool_options_gui (tool_options);
  GtkWidget         *frame;
  GtkWidget         *button;
  gchar             *str;

  /*  tool toggle  */
  frame = clay_prop_enum_radio_frame_new (config, "path-edit-mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  button = g_object_get_data (G_OBJECT (frame), "radio-button");

  if (GTK_IS_RADIO_BUTTON (button))
    {
      GSList *list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

      /* CLAY_PATH_MODE_MOVE  */
      button_append_modifier (list->data, GDK_MOD1_MASK);

      if (list->next)   /* CLAY_PATH_MODE_EDIT  */
        button_append_modifier (list->next->data,
                                clay_get_toggle_behavior_mask ());
    }

  button = clay_prop_check_button_new (config, "path-polygonal", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = clay_prop_check_button_new (config, "path-auto-connect", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  str = g_strdup_printf (_("Path to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s  Intersect"),
                         clay_get_mod_string (clay_get_extend_selection_mask ()),
                         clay_get_mod_string (clay_get_modify_selection_mask ()),
                         clay_get_mod_string (clay_get_extend_selection_mask () |
                                              clay_get_modify_selection_mask ()));

  button = clay_button_new ();
  /*  Create a selection from the current path  */
  gtk_button_set_label (GTK_BUTTON (button), _("Selection from Path"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  clay_help_set_help_data (button, str, CLAY_HELP_PATH_SELECTION_REPLACE);
  gtk_widget_set_visible (button, TRUE);

  g_free (str);

  options->to_selection_button = button;

  button = gtk_button_new_with_label (_("Create New Vector Layer"));
  clay_widget_set_identifier (button, "create-new-vector-layer-button");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  clay_help_set_help_data (button, NULL, NULL);
  gtk_widget_set_visible (button, TRUE);

  options->vector_layer_button = button;

  /* The fill editor */
  {
    GtkWidget *frame;
    GtkWidget *fill_editor;

    options->fill_options =
      clay_fill_options_new (CLAY_CONTEXT (options)->clay,
                             NULL, FALSE);

#define FILL_BIND(a)                                 \
  g_object_bind_property (options, "fill-" #a,       \
                          options->fill_options, #a, \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
    FILL_BIND (custom-style);
    FILL_BIND (foreground);
    FILL_BIND (pattern);

    fill_editor = clay_fill_editor_new (options->fill_options,
                                        TRUE, TRUE);
    gtk_widget_set_visible (fill_editor, TRUE);

    frame = clay_prop_expanding_frame_new (config, "enable-fill", NULL,
                                           fill_editor, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  }

  /* The stroke editor */
  {
    GtkWidget *frame;
    GtkWidget *stroke_editor;

    options->stroke_options =
      clay_stroke_options_new (CLAY_CONTEXT (options)->clay,
                               NULL, FALSE);

#define STROKE_BIND(a)                                 \
  g_object_bind_property (options, "stroke-" #a,       \
                          options->stroke_options, #a, \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
    STROKE_BIND (custom-style);
    STROKE_BIND (foreground);
    STROKE_BIND (pattern);
    STROKE_BIND (width);
    STROKE_BIND (unit);
    STROKE_BIND (cap-style);
    STROKE_BIND (join-style);
    STROKE_BIND (miter-limit);
    STROKE_BIND (antialias);
    STROKE_BIND (dash-offset);

    stroke_editor = clay_stroke_editor_new (options->stroke_options, 72.0,
                                            TRUE, TRUE);
    gtk_widget_set_visible (stroke_editor, TRUE);

    frame = clay_prop_expanding_frame_new (config, "enable-stroke", NULL,
                                           stroke_editor, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  }

  return vbox;
}
