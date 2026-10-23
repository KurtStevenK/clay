/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "claytoolinfo.h"
#include "claytooloptions.h"
#include "claytoolpreset.h"
#include "claytoolpreset-load.h"
#include "claytoolpreset-save.h"

#include "clay-intl.h"


/*  The defaults are "everything except color", which is problematic
 *  with gradients, which is why we special case the gradient tool in
 *  clay_tool_preset_set_options().
 */
#define DEFAULT_USE_FG_BG              FALSE
#define DEFAULT_USE_OPACITY_PAINT_MODE TRUE
#define DEFAULT_USE_BRUSH              TRUE
#define DEFAULT_USE_DYNAMICS           TRUE
#define DEFAULT_USE_MYBRUSH            TRUE
#define DEFAULT_USE_GRADIENT           FALSE
#define DEFAULT_USE_PATTERN            TRUE
#define DEFAULT_USE_PALETTE            FALSE
#define DEFAULT_USE_FONT               TRUE

enum
{
  PROP_0,
  PROP_NAME,
  PROP_CLAY,
  PROP_TOOL_OPTIONS,
  PROP_USE_FG_BG,
  PROP_USE_OPACITY_PAINT_MODE,
  PROP_USE_BRUSH,
  PROP_USE_DYNAMICS,
  PROP_USE_MYBRUSH,
  PROP_USE_GRADIENT,
  PROP_USE_PATTERN,
  PROP_USE_PALETTE,
  PROP_USE_FONT
};


static void          clay_tool_preset_config_iface_init    (ClayConfigInterface *iface);

static void          clay_tool_preset_constructed          (GObject          *object);
static void          clay_tool_preset_finalize             (GObject          *object);
static void          clay_tool_preset_set_property         (GObject          *object,
                                                            guint             property_id,
                                                            const GValue     *value,
                                                            GParamSpec       *pspec);
static void          clay_tool_preset_get_property         (GObject          *object,
                                                            guint             property_id,
                                                            GValue           *value,
                                                            GParamSpec       *pspec);
static void
             clay_tool_preset_dispatch_properties_changed  (GObject          *object,
                                                            guint             n_pspecs,
                                                            GParamSpec      **pspecs);

static const gchar * clay_tool_preset_get_extension        (ClayData         *data);

static gboolean      clay_tool_preset_deserialize_property (ClayConfig       *config,
                                                            guint             property_id,
                                                            GValue           *value,
                                                            GParamSpec       *pspec,
                                                            GScanner         *scanner,
                                                            GTokenType       *expected);

static void          clay_tool_preset_set_options          (ClayToolPreset   *preset,
                                                            ClayToolOptions  *options);
static void          clay_tool_preset_options_notify       (GObject          *tool_options,
                                                            const GParamSpec *pspec,
                                                            ClayToolPreset   *preset);
static void     clay_tool_preset_options_prop_name_changed (ClayContext         *tool_options,
                                                            ClayContextPropType  prop,
                                                            ClayToolPreset      *preset);


G_DEFINE_TYPE_WITH_CODE (ClayToolPreset, clay_tool_preset, CLAY_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_tool_preset_config_iface_init))

#define parent_class clay_tool_preset_parent_class


static void
clay_tool_preset_class_init (ClayToolPresetClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayDataClass *data_class   = CLAY_DATA_CLASS (klass);

  object_class->constructed                 = clay_tool_preset_constructed;
  object_class->finalize                    = clay_tool_preset_finalize;
  object_class->set_property                = clay_tool_preset_set_property;
  object_class->get_property                = clay_tool_preset_get_property;
  object_class->dispatch_properties_changed = clay_tool_preset_dispatch_properties_changed;

  data_class->save                          = clay_tool_preset_save;
  data_class->get_extension                 = clay_tool_preset_get_extension;

  CLAY_CONFIG_PROP_STRING (object_class, PROP_NAME,
                           "name",
                           NULL, NULL,
                           "Unnamed",
                           CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_TOOL_OPTIONS,
                           "tool-options",
                           NULL, NULL,
                           CLAY_TYPE_TOOL_OPTIONS,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_FG_BG,
                            "use-fg-bg",
                            _("Apply stored FG/BG"),
                            NULL,
                            DEFAULT_USE_FG_BG,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_OPACITY_PAINT_MODE,
                            "use-opacity-paint-mode",
                            _("Apply stored opacity/paint mode"),
                            NULL,
                            DEFAULT_USE_OPACITY_PAINT_MODE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_BRUSH,
                            "use-brush",
                            _("Apply stored brush"),
                            NULL,
                            DEFAULT_USE_BRUSH,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_DYNAMICS,
                            "use-dynamics",
                            _("Apply stored dynamics"),
                            NULL,
                            DEFAULT_USE_DYNAMICS,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_MYBRUSH,
                            "use-mypaint-brush",
                            _("Apply stored MyPaint brush"),
                            NULL,
                            DEFAULT_USE_MYBRUSH,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_PATTERN,
                            "use-pattern",
                            _("Apply stored pattern"),
                            NULL,
                            DEFAULT_USE_PATTERN,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_PALETTE,
                            "use-palette",
                            _("Apply stored palette"),
                            NULL,
                            DEFAULT_USE_PALETTE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_GRADIENT,
                            "use-gradient",
                            _("Apply stored gradient"),
                            NULL,
                            DEFAULT_USE_GRADIENT,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_FONT,
                            "use-font",
                            _("Apply stored font"),
                            NULL,
                            DEFAULT_USE_FONT,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_tool_preset_config_iface_init (ClayConfigInterface *iface)
{
  iface->deserialize_property = clay_tool_preset_deserialize_property;
}

static void
clay_tool_preset_init (ClayToolPreset *tool_preset)
{
}

static void
clay_tool_preset_constructed (GObject *object)
{
  ClayToolPreset *preset = CLAY_TOOL_PRESET (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_return_if_fail (CLAY_IS_CLAY (preset->clay));
}

static void
clay_tool_preset_finalize (GObject *object)
{
  ClayToolPreset *tool_preset = CLAY_TOOL_PRESET (object);

  clay_tool_preset_set_options (tool_preset, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_tool_preset_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayToolPreset *tool_preset = CLAY_TOOL_PRESET (object);

  switch (property_id)
    {
    case PROP_NAME:
      clay_object_set_name (CLAY_OBJECT (tool_preset),
                            g_value_get_string (value));
      break;

    case PROP_CLAY:
      tool_preset->clay = g_value_get_object (value); /* don't ref */
      break;

    case PROP_TOOL_OPTIONS:
      clay_tool_preset_set_options (tool_preset,
                                    CLAY_TOOL_OPTIONS (g_value_get_object (value)));
      break;

    case PROP_USE_FG_BG:
      tool_preset->use_fg_bg = g_value_get_boolean (value);
      break;
    case PROP_USE_OPACITY_PAINT_MODE:
      tool_preset->use_opacity_paint_mode = g_value_get_boolean (value);
      break;
    case PROP_USE_BRUSH:
      tool_preset->use_brush = g_value_get_boolean (value);
      break;
    case PROP_USE_DYNAMICS:
      tool_preset->use_dynamics = g_value_get_boolean (value);
      break;
    case PROP_USE_MYBRUSH:
      tool_preset->use_mybrush = g_value_get_boolean (value);
      break;
    case PROP_USE_PATTERN:
      tool_preset->use_pattern = g_value_get_boolean (value);
      break;
    case PROP_USE_PALETTE:
      tool_preset->use_palette = g_value_get_boolean (value);
      break;
    case PROP_USE_GRADIENT:
      tool_preset->use_gradient = g_value_get_boolean (value);
      break;
    case PROP_USE_FONT:
      tool_preset->use_font = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_preset_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayToolPreset *tool_preset = CLAY_TOOL_PRESET (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, clay_object_get_name (tool_preset));
      break;

    case PROP_CLAY:
      g_value_set_object (value, tool_preset->clay);
      break;

    case PROP_TOOL_OPTIONS:
      g_value_set_object (value, tool_preset->tool_options);
      break;

    case PROP_USE_FG_BG:
      g_value_set_boolean (value, tool_preset->use_fg_bg);
      break;
    case PROP_USE_OPACITY_PAINT_MODE:
      g_value_set_boolean (value, tool_preset->use_opacity_paint_mode);
      break;
    case PROP_USE_BRUSH:
      g_value_set_boolean (value, tool_preset->use_brush);
      break;
    case PROP_USE_MYBRUSH:
      g_value_set_boolean (value, tool_preset->use_mybrush);
      break;
    case PROP_USE_DYNAMICS:
      g_value_set_boolean (value, tool_preset->use_dynamics);
      break;
    case PROP_USE_PATTERN:
      g_value_set_boolean (value, tool_preset->use_pattern);
      break;
    case PROP_USE_PALETTE:
      g_value_set_boolean (value, tool_preset->use_palette);
      break;
    case PROP_USE_GRADIENT:
      g_value_set_boolean (value, tool_preset->use_gradient);
      break;
    case PROP_USE_FONT:
      g_value_set_boolean (value, tool_preset->use_font);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_preset_dispatch_properties_changed (GObject     *object,
                                              guint        n_pspecs,
                                              GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & CLAY_CONFIG_PARAM_SERIALIZE)
        {
          clay_data_dirty (CLAY_DATA (object));
          break;
        }
    }
}

static const gchar *
clay_tool_preset_get_extension (ClayData *data)
{
  return CLAY_TOOL_PRESET_FILE_EXTENSION;
}

static gboolean
clay_tool_preset_deserialize_property (ClayConfig *config,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec,
                                       GScanner   *scanner,
                                       GTokenType *expected)
{
  ClayToolPreset *tool_preset = CLAY_TOOL_PRESET (config);

  switch (property_id)
    {
    case PROP_TOOL_OPTIONS:
      {
        GObject             *options;
        gchar               *type_name;
        GType                type;
        ClayContextPropMask  serialize_props;

        if (! clay_scanner_parse_string (scanner, &type_name))
          {
            *expected = G_TOKEN_STRING;
            break;
          }

        if (! (type_name && *type_name))
          {
            g_scanner_error (scanner, "ClayToolOptions type name is empty");
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        if (! strcmp (type_name, "ClayTransformOptions"))
          {
            g_printerr ("Correcting tool options type ClayTransformOptions "
                        "to ClayTransformGridOptions\n");
            g_free (type_name);
            type_name = g_strdup ("ClayTransformGridOptions");
          }

        type = g_type_from_name (type_name);

        if (! type)
          {
            g_scanner_error (scanner,
                             "unable to determine type of '%s'",
                             type_name);
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        if (! g_type_is_a (type, CLAY_TYPE_TOOL_OPTIONS))
          {
            g_scanner_error (scanner,
                             "'%s' is not a subclass of ClayToolOptions",
                             type_name);
            *expected = G_TOKEN_NONE;
            g_free (type_name);
            break;
          }

        g_free (type_name);

        options = g_object_new (type,
                                "clay", tool_preset->clay,
                                NULL);

        /*  Initialize all ClayContext object properties that can be
         *  used by presets with some non-NULL object, so loading a
         *  broken preset won't leave us with NULL objects that have
         *  bad effects. See bug #742159.
         */
        clay_context_copy_properties (clay_get_user_context (tool_preset->clay),
                                      CLAY_CONTEXT (options),
                                      CLAY_CONTEXT_PROP_MASK_BRUSH    |
                                      CLAY_CONTEXT_PROP_MASK_DYNAMICS |
                                      CLAY_CONTEXT_PROP_MASK_MYBRUSH  |
                                      CLAY_CONTEXT_PROP_MASK_PATTERN  |
                                      CLAY_CONTEXT_PROP_MASK_GRADIENT |
                                      CLAY_CONTEXT_PROP_MASK_PALETTE  |
                                      CLAY_CONTEXT_PROP_MASK_FONT);

        if (! CLAY_CONFIG_GET_IFACE (options)->deserialize (CLAY_CONFIG (options),
                                                            scanner, 1, NULL))
          {
            *expected = G_TOKEN_NONE;
            g_object_unref (options);
            break;
          }

        /* we need both tool and tool-info on the options */
        if (clay_context_get_tool (CLAY_CONTEXT (options)))
          {
            g_object_set (options,
                          "tool-info",
                          clay_context_get_tool (CLAY_CONTEXT (options)),
                          NULL);
          }
        else if (CLAY_TOOL_OPTIONS (options)->tool_info)
          {
            g_object_set (options,
                          "tool", CLAY_TOOL_OPTIONS (options)->tool_info,
                          NULL);
          }
        else
          {
            /* if we have none, the options set_property() logic will
             * replace the NULL with its best guess
             */
            g_object_set (options,
                          "tool",      NULL,
                          "tool-info", NULL,
                          NULL);
          }

        serialize_props =
          clay_context_get_serialize_properties (CLAY_CONTEXT (options));

        clay_context_set_serialize_properties (CLAY_CONTEXT (options),
                                               serialize_props |
                                               CLAY_CONTEXT_PROP_MASK_TOOL);

        g_value_take_object (value, options);
      }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
clay_tool_preset_set_options (ClayToolPreset  *preset,
                              ClayToolOptions *options)
{
  if (preset->tool_options)
    {
      g_signal_handlers_disconnect_by_func (preset->tool_options,
                                            clay_tool_preset_options_notify,
                                            preset);

      g_signal_handlers_disconnect_by_func (preset->tool_options,
                                            clay_tool_preset_options_prop_name_changed,
                                            preset);

      g_clear_object (&preset->tool_options);
    }

  if (options)
    {
      ClayContextPropMask serialize_props;

      preset->tool_options =
        CLAY_TOOL_OPTIONS (clay_config_duplicate (CLAY_CONFIG (options)));

      serialize_props =
        clay_context_get_serialize_properties (CLAY_CONTEXT (preset->tool_options));

      clay_context_set_serialize_properties (CLAY_CONTEXT (preset->tool_options),
                                             serialize_props |
                                             CLAY_CONTEXT_PROP_MASK_TOOL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_FOREGROUND) &&
          ! (serialize_props & CLAY_CONTEXT_PROP_MASK_BACKGROUND))
        g_object_set (preset, "use-fg-bg", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_OPACITY) &&
          ! (serialize_props & CLAY_CONTEXT_PROP_MASK_PAINT_MODE))
        g_object_set (preset, "use-opacity-paint-mode", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_BRUSH))
        g_object_set (preset, "use-brush", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_DYNAMICS))
        g_object_set (preset, "use-dynamics", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_MYBRUSH))
        g_object_set (preset, "use-mypaint-brush", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_GRADIENT))
        g_object_set (preset, "use-gradient", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_PATTERN))
        g_object_set (preset, "use-pattern", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_PALETTE))
        g_object_set (preset, "use-palette", FALSE, NULL);

      if (! (serialize_props & CLAY_CONTEXT_PROP_MASK_FONT))
        g_object_set (preset, "use-font", FALSE, NULL);

      /*  see comment above the DEFAULT defines at the top of the file  */
      if (! g_strcmp0 ("clay-gradient-tool",
                       clay_object_get_name (preset->tool_options->tool_info)))
        g_object_set (preset, "use-gradient", TRUE, NULL);

      g_signal_connect (preset->tool_options, "notify",
                        G_CALLBACK (clay_tool_preset_options_notify),
                        preset);

      g_signal_connect (preset->tool_options, "prop-name-changed",
                        G_CALLBACK (clay_tool_preset_options_prop_name_changed),
                        preset);
    }

  g_object_notify (G_OBJECT (preset), "tool-options");
}

static void
clay_tool_preset_options_notify (GObject          *tool_options,
                                 const GParamSpec *pspec,
                                 ClayToolPreset   *preset)
{
  if (pspec->owner_type == CLAY_TYPE_CONTEXT)
    {
      ClayContextPropMask serialize_props;

      serialize_props =
        clay_context_get_serialize_properties (CLAY_CONTEXT (tool_options));

      if ((1 << pspec->param_id) & serialize_props)
        {
          g_object_notify (G_OBJECT (preset), "tool-options");
        }
    }
  else if (pspec->flags & CLAY_CONFIG_PARAM_SERIALIZE)
    {
      g_object_notify (G_OBJECT (preset), "tool-options");
    }
}

static void
clay_tool_preset_options_prop_name_changed (ClayContext         *tool_options,
                                            ClayContextPropType  prop,
                                            ClayToolPreset      *preset)
{
  ClayContextPropMask serialize_props;

  serialize_props =
    clay_context_get_serialize_properties (CLAY_CONTEXT (preset->tool_options));

  if ((1 << prop) & serialize_props)
    {
      g_object_notify (G_OBJECT (preset), "tool-options");
    }
}


/*  public functions  */

ClayData *
clay_tool_preset_new (ClayContext *context,
                      const gchar *unused)
{
  ClayToolInfo *tool_info;
  const gchar  *icon_name;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  tool_info = clay_context_get_tool (context);

  g_return_val_if_fail (tool_info != NULL, NULL);

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));

  return g_object_new (CLAY_TYPE_TOOL_PRESET,
                       "name",         tool_info->label,
                       "icon-name",    icon_name,
                       "clay",         context->clay,
                       "tool-options", tool_info->tool_options,
                       NULL);
}

ClayContextPropMask
clay_tool_preset_get_prop_mask (ClayToolPreset *preset)
{
  ClayContextPropMask serialize_props;
  ClayContextPropMask use_props = 0;

  g_return_val_if_fail (CLAY_IS_TOOL_PRESET (preset), 0);

  serialize_props =
    clay_context_get_serialize_properties (CLAY_CONTEXT (preset->tool_options));

  if (preset->use_fg_bg)
    {
      use_props |= (CLAY_CONTEXT_PROP_MASK_FOREGROUND & serialize_props);
      use_props |= (CLAY_CONTEXT_PROP_MASK_BACKGROUND & serialize_props);
    }

  if (preset->use_opacity_paint_mode)
    {
      use_props |= (CLAY_CONTEXT_PROP_MASK_OPACITY    & serialize_props);
      use_props |= (CLAY_CONTEXT_PROP_MASK_PAINT_MODE & serialize_props);
    }

  if (preset->use_brush)
    use_props |= (CLAY_CONTEXT_PROP_MASK_BRUSH & serialize_props);

  if (preset->use_dynamics)
    use_props |= (CLAY_CONTEXT_PROP_MASK_DYNAMICS & serialize_props);

  if (preset->use_mybrush)
    use_props |= (CLAY_CONTEXT_PROP_MASK_MYBRUSH & serialize_props);

  if (preset->use_pattern)
    use_props |= (CLAY_CONTEXT_PROP_MASK_PATTERN & serialize_props);

  if (preset->use_palette)
    use_props |= (CLAY_CONTEXT_PROP_MASK_PALETTE & serialize_props);

  if (preset->use_gradient)
    use_props |= (CLAY_CONTEXT_PROP_MASK_GRADIENT & serialize_props);

  if (preset->use_font)
    use_props |= (CLAY_CONTEXT_PROP_MASK_FONT & serialize_props);

  return use_props;
}
