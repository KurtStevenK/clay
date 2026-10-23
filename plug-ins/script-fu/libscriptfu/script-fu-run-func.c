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
#include <glib.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "scheme-wrapper.h"       /* type "pointer" */

#include "script-fu-lib.h"
#include "script-fu-types.h"
#include "script-fu-interface.h"  /* ScriptFu's GUI implementation. */
#include "script-fu-dialog.h"     /* Clay's GUI implementation. */
#include "script-fu-script.h"
#include "script-fu-scripts.h"    /* script_fu_find_script */
#include "script-fu-command.h"
#include "script-fu-version.h"
#include "script-fu-progress.h"

#include "script-fu-run-func.h"

/* Outer run_funcs
 * One each for ClayProcedure and ClayImageProcedure.
 * These are called from Clay, with two different signatures.
 * These form and interpret "commands" which are calls to inner run_funcs
 * defined in Scheme by a script.

 * These return the result of interpretation,
 * in a ClayValueArray whose only element is a status.
 * !!! ScriptFu does not let authors define procedures that return values.
 *
 * A prior script may have called (script-fu-use-v3) to opt in to interpret v3 dialect.
 * When this is long-running extension-script-fu process,
 * ensure initial dialect is v2, the default.
 * FUTURE: default is v3 and script must opt in to v2 dialect.
 */

/* run_func for a ClayImageProcedure
 *
 * Type is ClayRunImageFunc.
 *
 * Uses Clay's config and gui.
 *
 * Since 3.0
 */
ClayValueArray *
script_fu_run_image_procedure (ClayProcedure        *procedure, /* ClayImageProcedure */
                               ClayRunMode           run_mode,
                               ClayImage            *image,
                               ClayDrawable        **drawables,
                               ClayProcedureConfig  *config,
                               gpointer              data)
{

  ClayValueArray *result = NULL;
  SFScript       *script;

  g_debug ("script_fu_run_image_procedure");
  script = script_fu_find_script (clay_procedure_get_name (procedure));

  if (! script)
    return clay_procedure_new_return_values (procedure, CLAY_PDB_CALLING_ERROR, NULL);

  ts_set_run_mode (run_mode);

  begin_interpret_default_dialect ();

  script_fu_progress_init (clay_procedure_get_menu_label (procedure));

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      {
        guint n_specs;

        clay_ui_init ("script-fu");

        g_free (g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_specs));
        if (n_specs > 1)
          {
            /* Let user choose "other" args in a dialog, then interpret. Maintain a config. */
            result = script_fu_dialog_run_image_proc (procedure, script, image, drawables, config);
          }
        else
          {
            /* No "other" args for user to choose. No config to maintain. */
            result = script_fu_interpret_image_proc (procedure, script, image, drawables, config);
          }
        break;
      }
    case CLAY_RUN_NONINTERACTIVE:
      {
        /* A call from another PDB procedure.
         * Use the given config, without interacting with user.
         * Since no user interaction, no config to maintain.
         */
        result = script_fu_interpret_image_proc (procedure, script, image, drawables, config);
        break;
      }
    case CLAY_RUN_WITH_LAST_VALS:
      {
        /* User invoked from a menu "Filter>Run with last values".
         * Do not show dialog. config are already last values.
         */
        result = script_fu_interpret_image_proc (procedure, script, image, drawables, config);
        break;
      }
    default:
      {
        result = clay_procedure_new_return_values (procedure, CLAY_PDB_CALLING_ERROR, NULL);
      }
    }

  return result;
}

/* The number of pspecs in a config's pspecs to skip, for a regular ClayProcedure.
 * Skip procedure-name and run-mode.
 */
#define SF_ARGS_SKIPPED_REGULAR 2

/* run_func for a ClayProcedure
 *
 * Type is ClayRunFunc.
 *
 * Uses Clay's config and gui.
 *
 * Since 3.0
 */
ClayValueArray *
script_fu_run_regular_procedure (ClayProcedure        *procedure,
                                 ClayProcedureConfig  *config,
                                 gpointer              data)
{

  ClayValueArray *result = NULL;
  SFScript       *script;
  ClayRunMode     run_mode;

  g_debug ("%s", G_STRFUNC);

  script = script_fu_find_script (clay_procedure_get_name (procedure));

  if (! script)
    return clay_procedure_new_return_values (procedure, CLAY_PDB_CALLING_ERROR, NULL);

  /* Unlike ImageProcedure, run-mode is a prop in the config. */
  g_object_get (config, "run-mode", &run_mode, NULL);
  ts_set_run_mode (run_mode);

  begin_interpret_default_dialect ();

  script_fu_progress_init (clay_procedure_get_menu_label (procedure));

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      {
        guint n_specs;

        clay_ui_init ("script-fu");

        g_free (g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_specs));
        if (n_specs > SF_ARGS_SKIPPED_REGULAR)
          {
            /* Let user choose non-skipped args in a dialog, then interpret. Maintain a config. */
            result = script_fu_dialog_run_regular_proc (procedure, script, config);
          }
        else
          {
            /* No args for user to choose. No config to maintain. */
            result = script_fu_interpret_regular_proc (procedure, script, config);
          }
        break;
      }
    case CLAY_RUN_NONINTERACTIVE:
      {
        /* A call from another PDB procedure.
         * Use the given config, without interacting with user.
         * Since no user interaction, no config to maintain.
         */
        result = script_fu_interpret_regular_proc (procedure, script, config);
        break;
      }
    case CLAY_RUN_WITH_LAST_VALS:
      {
        /* User invoked from a menu "Filter>Run with last values".
         * Do not show dialog. config are already last values.
         */
        result = script_fu_interpret_regular_proc (procedure, script, config);
        break;
      }
    default:
      {
        result = clay_procedure_new_return_values (procedure, CLAY_PDB_CALLING_ERROR, NULL);
      }
    }

  return result;
}

/* run_func for a ClayProcedure registered using Scheme "script-fu-register"
 *
 * Type is ClayRunFunc
 *
 * Uses ScriptFu's own GUI implementation, and retains settings locally.
 *
 * Since prior to 3.0 but formerly named script_fu_script_proc
 */
ClayValueArray *
script_fu_run_procedure (ClayProcedure       *procedure,
                         ClayProcedureConfig *config,
                         gpointer             data)
{
  ClayPDBStatusType   status = CLAY_PDB_SUCCESS;
  SFScript           *script;
  GParamSpec        **pspecs;
  guint               n_pspecs;
  gint                n_aux_args;
  ClayRunMode         run_mode;
  GError             *error = NULL;

  script = script_fu_find_script (clay_procedure_get_name (procedure));

  if (! script)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_CALLING_ERROR,
                                             NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config), &n_pspecs);
  clay_procedure_get_aux_arguments (procedure, &n_aux_args);

  g_object_get (config, "run-mode", &run_mode, NULL);

  ts_set_run_mode (run_mode);

  begin_interpret_default_dialect ();

  script_fu_progress_init (clay_procedure_get_menu_label (procedure));

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      {
        gint min_args = 0;

        clay_ui_init ("script-fu");

        /*  First, try to collect the standard script arguments...  */
        min_args = script_fu_script_collect_standard_args (script, pspecs, n_pspecs, config);

        /*  If plugin has more than the standard args. */
        if (script->n_args > min_args)
          {
            /* Get the rest of arguments with a dialog, and run the command.  */
            status = script_fu_interface_dialog (script, min_args);

            if (status == CLAY_PDB_EXECUTION_ERROR)
              return clay_procedure_new_return_values (procedure, status,
                                                       script_fu_get_gerror ());

            /* Else no error, or CLAY_PDB_CANCEL.
             * CLAY_PDB_CALLING_ERROR is emitted prior to this.
             * Break and return without an error message.
             */
            break;
          }
        /*  Else fallthrough to next case and run the script without dialog. */
      }

    case CLAY_RUN_NONINTERACTIVE:
      /* Verify actual args count equals declared arg count. */
      if (n_pspecs != script->n_args + n_aux_args + SF_ARG_TO_CONFIG_OFFSET)
        status = CLAY_PDB_CALLING_ERROR;

      if (status == CLAY_PDB_SUCCESS)
        {
          gchar *command;

          command = script_fu_script_get_command_from_params (script, pspecs, n_pspecs, config);

          /*  run the command through the interpreter  */
          if (! script_fu_run_command (command, &error))
            {
              return clay_procedure_new_return_values (procedure,
                                                       CLAY_PDB_EXECUTION_ERROR,
                                                       error);
            }

          g_free (command);
        }
      break;

    case CLAY_RUN_WITH_LAST_VALS:
      {
        gchar *command;

        /*  First, try to collect the standard script arguments  */
        script_fu_script_collect_standard_args (script, pspecs, n_pspecs, config);

        command = script_fu_script_get_command (script);

        /*  run the command through the interpreter  */
        if (! script_fu_run_command (command, &error))
          {
            return clay_procedure_new_return_values (procedure,
                                                     CLAY_PDB_EXECUTION_ERROR,
                                                     error);
          }

        g_free (command);
      }
      break;

    default:
      break;
    }

  return clay_procedure_new_return_values (procedure, status, NULL);
}
