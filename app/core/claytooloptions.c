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
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clayerror.h"
#include "claytoolinfo.h"
#include "claytooloptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_TOOL,
  PROP_TOOL_INFO
};


static void   clay_tool_options_config_iface_init (ClayConfigInterface *iface);

static void   clay_tool_options_dispose           (GObject         *object);
static void   clay_tool_options_set_property      (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   clay_tool_options_get_property      (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);

static void   clay_tool_options_config_reset      (ClayConfig      *config);

static void   clay_tool_options_tool_notify       (ClayToolOptions *options,
                                                   GParamSpec      *pspec);


G_DEFINE_TYPE_WITH_CODE (ClayToolOptions, clay_tool_options, CLAY_TYPE_CONTEXT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_tool_options_config_iface_init))

#define parent_class clay_tool_options_parent_class


static void
clay_tool_options_class_init (ClayToolOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_tool_options_dispose;
  object_class->set_property = clay_tool_options_set_property;
  object_class->get_property = clay_tool_options_get_property;

  g_object_class_override_property (object_class, PROP_TOOL, "tool");

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        CLAY_TYPE_TOOL_INFO,
                                                        CLAY_PARAM_READWRITE));

}

static void
clay_tool_options_init (ClayToolOptions *options)
{
  options->tool_info = NULL;

  g_signal_connect (options, "notify::tool",
                    G_CALLBACK (clay_tool_options_tool_notify),
                    NULL);
}

static void
clay_tool_options_config_iface_init (ClayConfigInterface *iface)
{
  iface->reset = clay_tool_options_config_reset;
}

static void
clay_tool_options_dispose (GObject *object)
{
  ClayToolOptions *options = CLAY_TOOL_OPTIONS (object);

  g_clear_object (&options->tool_info);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*  This is such a horrible hack, but necessary because we
 *  a) load an option's tool-info from disk in many cases
 *  b) screwed up in the past and saved the wrong tool-info in some cases
 */
static ClayToolInfo *
clay_tool_options_check_tool_info (ClayToolOptions *options,
                                   ClayToolInfo    *tool_info,
                                   gboolean         warn)
{
  if (tool_info && G_OBJECT_TYPE (options) == tool_info->tool_options_type)
    {
      return tool_info;
    }
  else
    {
      GList *list;

      for (list = clay_get_tool_info_iter (CLAY_CONTEXT (options)->clay);
           list;
           list = g_list_next (list))
        {
          ClayToolInfo *new_info = list->data;

          if (G_OBJECT_TYPE (options) == new_info->tool_options_type)
            {
              if (warn)
                g_printerr ("%s: correcting bogus deserialized tool "
                            "type '%s' with right type '%s'\n",
                            g_type_name (G_OBJECT_TYPE (options)),
                            tool_info ? clay_object_get_name (tool_info) : "NULL",
                            clay_object_get_name (new_info));

              return new_info;
            }
        }

      g_return_val_if_reached (NULL);
    }
}

static void
clay_tool_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayToolOptions *options = CLAY_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TOOL:
      {
        ClayToolInfo *tool_info = g_value_get_object (value);
        ClayToolInfo *context_tool;

        context_tool = clay_context_get_tool (CLAY_CONTEXT (options));

        g_return_if_fail (context_tool == NULL ||
                          context_tool == tool_info);

        tool_info = clay_tool_options_check_tool_info (options, tool_info, TRUE);

        if (! context_tool)
          clay_context_set_tool (CLAY_CONTEXT (options), tool_info);
      }
      break;

    case PROP_TOOL_INFO:
      {
        ClayToolInfo *tool_info = g_value_get_object (value);

        g_return_if_fail (options->tool_info == NULL ||
                          options->tool_info == tool_info);

        tool_info = clay_tool_options_check_tool_info (options, tool_info, TRUE);

        if (! options->tool_info)
          {
            options->tool_info = g_object_ref (tool_info);

            clay_context_set_serialize_properties (CLAY_CONTEXT (options),
                                                   tool_info->context_props);
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayToolOptions *options = CLAY_TOOL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TOOL:
      g_value_set_object (value, clay_context_get_tool (CLAY_CONTEXT (options)));
      break;

    case PROP_TOOL_INFO:
      g_value_set_object (value, options->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_options_config_reset (ClayConfig *config)
{
  gchar *name = g_strdup (clay_object_get_name (config));

  clay_config_reset_properties (G_OBJECT (config));

  clay_object_take_name (CLAY_OBJECT (config), name);
}

static void
clay_tool_options_tool_notify (ClayToolOptions *options,
                               GParamSpec      *pspec)
{
  ClayToolInfo *tool_info = clay_context_get_tool (CLAY_CONTEXT (options));
  ClayToolInfo *new_info;

  new_info = clay_tool_options_check_tool_info (options, tool_info, FALSE);

  if (tool_info && new_info != tool_info)
    g_warning ("%s: 'tool' property on %s was set to bogus value "
               "'%s', it MUST be '%s'.",
               G_STRFUNC,
               g_type_name (G_OBJECT_TYPE (options)),
               clay_object_get_name (tool_info),
               clay_object_get_name (new_info));
}


/*  public functions  */

void
clay_tool_options_set_gui_mode (ClayToolOptions *tool_options,
                                gboolean         gui_mode)
{
  g_return_if_fail (CLAY_IS_TOOL_OPTIONS (tool_options));

  tool_options->gui_mode = gui_mode ? TRUE : FALSE;
}

gboolean
clay_tool_options_get_gui_mode (ClayToolOptions *tool_options)
{
  g_return_val_if_fail (CLAY_IS_TOOL_OPTIONS (tool_options), FALSE);

  return tool_options->gui_mode;
}

gboolean
clay_tool_options_serialize (ClayToolOptions  *tool_options,
                             GError          **error)
{
  GFile    *file;
  gchar    *header;
  gchar    *footer;
  gboolean  retval;

  g_return_val_if_fail (CLAY_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_tool_info_get_options_file (tool_options->tool_info, NULL);

  if (tool_options->tool_info->clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  header = g_strdup_printf ("Clay %s options",
                            clay_object_get_name (tool_options->tool_info));
  footer = g_strdup_printf ("end of %s options",
                            clay_object_get_name (tool_options->tool_info));

  retval = clay_config_serialize_to_file (CLAY_CONFIG (tool_options),
                                          file,
                                          header, footer,
                                          NULL,
                                          error);

  g_free (header);
  g_free (footer);

  g_object_unref (file);

  return retval;
}

gboolean
clay_tool_options_deserialize (ClayToolOptions  *tool_options,
                               GError          **error)
{
  GFile    *file;
  gboolean  retval;

  g_return_val_if_fail (CLAY_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_tool_info_get_options_file (tool_options->tool_info, NULL);

  if (tool_options->tool_info->clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  retval = clay_config_deserialize_file (CLAY_CONFIG (tool_options),
                                         file,
                                         NULL,
                                         error);

  g_object_unref (file);

  return retval;
}

gboolean
clay_tool_options_delete (ClayToolOptions  *tool_options,
                          GError          **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (CLAY_IS_TOOL_OPTIONS (tool_options), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_tool_info_get_options_file (tool_options->tool_info, NULL);

  if (tool_options->tool_info->clay->be_verbose)
    g_print ("Deleting '%s'\n", clay_file_get_utf8_name (file));

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

void
clay_tool_options_create_folder (void)
{
  GFile *file = clay_directory_file ("tool-options", NULL);

  g_file_make_directory (file, NULL, NULL);
  g_object_unref (file);
}
