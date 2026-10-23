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

#include <libclay/clay.h>

#include "console/script-fu-console.h"
#include "script-fu-eval.h"
#include "script-fu-text-console.h"

#include "libscriptfu/script-fu-lib.h"
#include "libscriptfu/script-fu-intl.h"


#define SCRIPT_FU_TYPE (script_fu_get_type ())
G_DECLARE_FINAL_TYPE (ScriptFu, script_fu, SCRIPT, FU, ClayPlugIn)

struct _ScriptFu
{
  ClayPlugIn      parent_instance;
};

static GList          * script_fu_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * script_fu_create_procedure (ClayPlugIn           *plug_in,
                                                    const gchar          *name);

static ClayValueArray * script_fu_run              (ClayProcedure        *procedure,
                                                    ClayProcedureConfig  *config,
                                                    gpointer              run_data);
static ClayValueArray * script_fu_batch_run        (ClayProcedure        *procedure,
                                                    ClayRunMode           run_mode,
                                                    const gchar          *code,
                                                    ClayProcedureConfig  *config,
                                                    gpointer              run_data);
static void             script_fu_run_init         (ClayProcedure        *procedure,
                                                    ClayRunMode           run_mode);
static void             script_fu_extension_init   (ClayPlugIn           *plug_in);


G_DEFINE_TYPE (ScriptFu, script_fu, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (SCRIPT_FU_TYPE)
DEFINE_STD_SET_I18N


static void
script_fu_class_init (ScriptFuClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = script_fu_query_procedures;
  plug_in_class->create_procedure = script_fu_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
script_fu_init (ScriptFu *script_fu)
{
}

static GList *
script_fu_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup ("extension-script-fu"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-console"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-text-console"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-eval"));

  return list;
}

static ClayProcedure *
script_fu_create_procedure (ClayPlugIn  *plug_in,
                            const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, "extension-script-fu"))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PERSISTENT,
                                      script_fu_run, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "A scheme interpreter for scripting "
                                        "Clay operations",
                                        NULL, NULL);
      clay_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");
    }
  else if (! strcmp (name, "plug-in-script-fu-console"))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Script-Fu _Console"));
      clay_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Development/Script-Fu");

      clay_procedure_set_documentation (procedure,
                                        _("Interactive console for Script-Fu "
                                          "development"),
                                        "Provides an interface which allows "
                                        "interactive scheme development.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      clay_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        CLAY_TYPE_RUN_MODE,
                                        CLAY_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
      clay_procedure_add_string_array_aux_argument (procedure, "history",
                                                    "Command history",
                                                    "History",
                                                    G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-text-console"))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Provides a text console mode for "
                                        "script-fu development",
                                        "Provides an interface which allows "
                                        "interactive scheme development.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      clay_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        CLAY_TYPE_RUN_MODE,
                                        CLAY_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-eval"))
    {
      procedure = clay_batch_procedure_new (plug_in, name, "Script-fu (scheme)",
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            script_fu_batch_run, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Evaluate scheme code",
                                        "Evaluate the code under the scheme "
                                        "interpreter (primarily for batch mode)",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Manish Singh",
                                      "Manish Singh",
                                      "1998");
    }

  return procedure;
}

static ClayValueArray *
script_fu_run (ClayProcedure        *procedure,
               ClayProcedureConfig  *config,
               gpointer              run_data)
{
  ClayPlugIn     *plug_in     = clay_procedure_get_plug_in (procedure);
  const gchar    *name        = clay_procedure_get_name (procedure);
  ClayValueArray *return_vals = NULL;
  ClayRunMode     run_mode    = CLAY_RUN_NONINTERACTIVE;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (config), "run-mode") != NULL)
    g_object_get (config, "run-mode", &run_mode, NULL);
  script_fu_run_init (procedure, run_mode);

  if (strcmp (name, "extension-script-fu") == 0)
    {
      /* Simultaneously poll both reads of commands from clay app, and GUI events.
       * extension-script-fu periodically has GUI: dialogs for sequential commands.
       */

      /*  Acknowledge that the extension is properly initialized  */
      clay_procedure_persistent_ready (procedure);

      /* Hang async read from clay app. */
      clay_plug_in_persistent_enable (plug_in);

      /* Process both async reads of commands and GUI events. */
      g_main_loop_run (g_main_loop_new (NULL, FALSE));

      /* The loop does not return because no command or event calls clay_quit.
       * Clay app does not orderly shutdown extension-script-fu.
       * When orderly, need to free the GMainLoop.
       */
      g_assert_not_reached ();
    }
  else if (strcmp (name, "plug-in-script-fu-text-console") == 0)
    {
      /*
       *  The script-fu text console for interactive Scheme development
       */

      return_vals = script_fu_text_console_run (procedure, config);
    }
  else if (strcmp (name, "plug-in-script-fu-console") == 0)
    {
      /*
       *  The script-fu console for interactive Scheme development
       */

      return_vals = script_fu_console_run (procedure, config);
    }

  if (! return_vals)
    return_vals = clay_procedure_new_return_values (procedure,
                                                    CLAY_PDB_SUCCESS,
                                                    NULL);

  return return_vals;
}

static ClayValueArray *
script_fu_batch_run (ClayProcedure        *procedure,
                     ClayRunMode           run_mode,
                     const gchar          *code,
                     ClayProcedureConfig  *config,
                     gpointer              run_data)
{
  const gchar    *name        = clay_procedure_get_name (procedure);
  ClayValueArray *return_vals = NULL;

  script_fu_run_init (procedure, run_mode);

  if (strcmp (name, "plug-in-script-fu-eval") == 0)
    {
      /*
       *  A non-interactive "console" (for batch mode)
       */

      if (g_strcmp0 (code, "-") == 0)
        /* Redirecting to script-fu text console, for backward compatibility  */
        return_vals = script_fu_text_console_run (procedure, config);
      else
        return_vals = script_fu_eval_run (procedure, run_mode, code, config);
    }

  if (! return_vals)
    return_vals = clay_procedure_new_return_values (procedure,
                                                    CLAY_PDB_SUCCESS,
                                                    NULL);

  return return_vals;
}

static void
script_fu_run_init (ClayProcedure *procedure,
                    ClayRunMode    run_mode)
{
  ClayPlugIn     *plug_in     = clay_procedure_get_plug_in (procedure);
  const gchar    *name        = clay_procedure_get_name (procedure);
  GList          *path;

  path = script_fu_search_path ();

  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*  Setup auxiliary temporary procedures for the base extension  */
      script_fu_extension_init (plug_in);

      /*  Init the interpreter, allow register scripts */
      script_fu_init_embedded_interpreter (path, TRUE, run_mode, TRUE);
    }
  else
    {
      /*  Init the interpreter, not allow register scripts */
      script_fu_init_embedded_interpreter (path, FALSE, run_mode, FALSE);
    }

  script_fu_find_and_register_scripts (plug_in, path);

  g_list_free_full (path, (GDestroyNotify) g_object_unref);
}

static void
script_fu_extension_init (ClayPlugIn *plug_in)
{
  /* Commented out until fixed or replaced.
   * script_fu_register_refresh_procedure (plug_in);
   */
}
