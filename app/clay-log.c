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

#include "glib-object.h"

#include "clay-debug.h"
#include "clay-log.h"


static const GDebugKey log_keys[] =
{
  { "tool-events",        CLAY_LOG_TOOL_EVENTS        },
  { "tool-focus",         CLAY_LOG_TOOL_FOCUS         },
  { "dnd",                CLAY_LOG_DND                },
  { "help",               CLAY_LOG_HELP               },
  { "dialog-factory",     CLAY_LOG_DIALOG_FACTORY     },
  { "menus",              CLAY_LOG_MENUS              },
  { "save-dialog",        CLAY_LOG_SAVE_DIALOG        },
  { "image-scale",        CLAY_LOG_IMAGE_SCALE        },
  { "shadow-tiles",       CLAY_LOG_SHADOW_TILES       },
  { "scale",              CLAY_LOG_SCALE              },
  { "wm",                 CLAY_LOG_WM                 },
  { "floating-selection", CLAY_LOG_FLOATING_SELECTION },
  { "shm",                CLAY_LOG_SHM                },
  { "text-editing",       CLAY_LOG_TEXT_EDITING       },
  { "key-events",         CLAY_LOG_KEY_EVENTS         },
  { "auto-tab-style",     CLAY_LOG_AUTO_TAB_STYLE     },
  { "instances",          CLAY_LOG_INSTANCES          },
  { "rectangle-tool",     CLAY_LOG_RECTANGLE_TOOL     },
  { "brush-cache",        CLAY_LOG_BRUSH_CACHE        },
  { "projection",         CLAY_LOG_PROJECTION         },
  { "xcf",                CLAY_LOG_XCF                }
};

static const gchar * const log_domains[] =
{
  "Clay",
  "Clay-Actions",
  "Clay-Base",
  "Clay-Composite",
  "Clay-Config",
  "Clay-Core",
  "Clay-Dialogs",
  "Clay-Display",
  "Clay-File",
  "Clay-GEGL",
  "Clay-GUI",
  "Clay-Menus",
  "Clay-Operations",
  "Clay-PDB",
  "Clay-Paint",
  "Clay-Paint-Funcs",
  "Clay-Plug-In",
  "Clay-Text",
  "Clay-Tools",
  "Clay-Vectors",
  "Clay-Widgets",
  "Clay-XCF",
  "LibClayBase",
  "LibClayColor",
  "LibClayConfig",
  "LibClayMath",
  "LibClayModule",
  "LibClayThumb",
  "LibClayWidgets",
  "GEGL",
  NULL
};


ClayLogFlags clay_log_flags = 0;


void
clay_log_init (void)
{
  const gchar *env_log_val = g_getenv ("CLAY_LOG");

  if (! env_log_val)
    env_log_val = g_getenv ("CLAY_DEBUG");

  if (env_log_val)
    g_setenv ("G_MESSAGES_DEBUG", env_log_val, TRUE);

  if (env_log_val)
    {
      /*  g_parse_debug_string() has special treatment of the string 'help',
       *  but we want to use it for the CLAY_LOG_HELP domain. "list-all"
       *  is a replacement for "help" in Clay.
       */
      if (g_ascii_strcasecmp (env_log_val, "list-all") == 0)
        clay_log_flags = g_parse_debug_string ("help",
                                               log_keys,
                                               G_N_ELEMENTS (log_keys));
      else if (g_ascii_strcasecmp (env_log_val, "help") == 0)
        clay_log_flags = CLAY_LOG_HELP;
      else
        clay_log_flags = g_parse_debug_string (env_log_val,
                                               log_keys,
                                               G_N_ELEMENTS (log_keys));

      if (clay_log_flags & CLAY_LOG_INSTANCES)
        {
          clay_debug_enable_instances ();
        }
      else if (! clay_log_flags)
        {
          /* If the environment variable was set but no log flags are
           * set as a result, let's assume one is not sure how to use
           * the log flags and output the list of keys as a helper.
           */
          clay_log_flags = g_parse_debug_string ("help",
                                                 log_keys,
                                                 G_N_ELEMENTS (log_keys));
        }
    }
}

void
clay_log (ClayLogFlags  flags,
          const gchar  *function,
          gint          line,
          const gchar  *format,
          ...)
{
  va_list args;

  va_start (args, format);
  clay_logv (flags, function, line, format, args);
  va_end (args);
}

void
clay_logv (ClayLogFlags  flags,
           const gchar  *function,
           gint          line,
           const gchar  *format,
           va_list       args)
{
  const gchar *domain = "unknown";
  gchar       *message;
  gint         i;

  for (i = 0; i < G_N_ELEMENTS (log_keys); i++)
    if (log_keys[i].value == flags)
      {
        domain = log_keys[i].key;
        break;
      }

  if (format)
    message = g_strdup_vprintf (format, args);
  else
    message = g_strdup ("called");

  g_log (domain, G_LOG_LEVEL_DEBUG,
         "%s(%d): %s", function, line, message);

  g_free (message);
}

ClayLogHandler
clay_log_set_handler (gboolean       global,
                      GLogLevelFlags log_levels,
                      GLogFunc       log_func,
                      gpointer       user_data)
{
  ClayLogHandler handler;
  gint           n;
  gint           i;

  g_return_val_if_fail (log_func != NULL, NULL);

  n = G_N_ELEMENTS (log_domains) - (global ? 1 : 0);

  handler = g_new (guint, n + 1);

  handler[0] = n;

  for (i = 0; i < n; i++)
    {
      handler[i + 1] = g_log_set_handler (log_domains[i], log_levels,
                                          log_func, user_data);
    }

  return handler;
}

void
clay_log_remove_handler (ClayLogHandler handler)
{
  gint n;
  gint i;

  g_return_if_fail (handler != NULL);

  n = handler[0];

  for (i = 0; i < n; i++)
    g_log_remove_handler (log_domains[i], handler[i + 1]);

  g_free (handler);
}
