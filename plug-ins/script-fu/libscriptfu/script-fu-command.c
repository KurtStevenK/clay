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

#include <libclay/clay.h>

#include "script-fu-types.h"    /* SFScript */
#include "script-fu-lib.h"
#include "script-fu-script.h"

#include "script-fu-command.h"


/* Methods for interpreting commands.
 *
 * Usually there is a stack of calls similar to:
 *       script_fu_run_image_procedure (outer run func)
 * calls script_fu_interpret_image_proc
 * calls script_fu_run_command
 * calls ts_interpret_string
 * calls the inner run func in Scheme
 *
 * but script_fu_run_command is also called directly for loading scripts.
 *
 * FUTURE: see also similar code in script-fu-interface.c
 * which could be migrated here.
 */


/* Interpret a command.
 * A command is a string for a Scheme call to a script plugin's run function.
 *
 * When errors during interpretation:
 * 1) set the error message from tinyscheme into GError at given handle.
 * 2) return FALSE
 * Otherwise, return TRUE and discard any result of interpretation.
 * ScriptFu return values only have a ClayPDBStatus,
 * since ScriptFu plugin scripts can only be declared returning void.
 *
 * In v2, we captured output from a script (calls to Scheme:display)
 * and they were a prefix of any error message.
 * In v3, output from a script is shown in any stdout/terminal in which Clay was started.
 * And any error msg is retrieved from the inner interpreter.
 *
 * While interpreting, any errors from further calls to the PDB
 * can show error dialogs in any Clay gui,
 * unless the caller has taken responsibility with a prior call to
 * clay_plug_in_set_pdb_error_handler
 *
 * FIXME: see script_fu_run_procedure.
 * It does not call clay_plug_in_set_pdb_error_handler for NON-INTERACTIVE mode.
 */
gboolean
script_fu_run_command (const gchar  *command,
                       GError      **error)
{
  g_debug ("script_fu_run_command: %s", command);

  if (script_fu_interpret_string (command))
    {
      *error = script_fu_get_gerror ();
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

/* Wrap a run_command in error handler.
 * Returns the result of running the command.
 * The result is a PDB call result whose only value is the success.
 * Since no SF command returns more values i.e. scripts return type void.
 */
static ClayValueArray *
sf_wrap_run_command (ClayProcedure *procedure,
                     SFScript      *script,
                     gchar         *command)
{
  ClayValueArray *result = NULL;
  gboolean        interpretation_result;
  GError         *error = NULL;

  /* Take responsibility for handling errors from the scripts further calls to PDB.
   * ScriptFu does not show an error dialog, but forwards errors back to Clay.
   * This only tells Clay that ScriptFu itself will forward ClayPDBStatus errors from
   * this scripts calls to the PDB.
   * The onus is on this script's called PDB procedures to return errors in the ClayPDBStatus.
   * Any that do not, but for example only call clay-message, are breaching contract.
   */
  clay_plug_in_set_pdb_error_handler (clay_get_plug_in (),
                                      CLAY_PDB_ERROR_HANDLER_PLUGIN);

  interpretation_result = script_fu_run_command (command, &error);

  if (! interpretation_result)
    {
      /* This is to the console.
       * script->name not localized.
       * error->message expected to be localized.
       * Clay will later display "PDB procedure failed: <message>" localized.
       */
      g_warning ("While executing %s: %s",
                 script->name,
                 error->message);
      /* A GError was allocated and this will take ownership. */
      result = clay_procedure_new_return_values (procedure,
                                                 CLAY_PDB_EXECUTION_ERROR,
                                                 error);
    }
  else
    {
      result = clay_procedure_new_return_values (procedure,
                                                 CLAY_PDB_SUCCESS,
                                                 NULL);
    }

  clay_plug_in_set_pdb_error_handler (clay_get_plug_in (),
                                      CLAY_PDB_ERROR_HANDLER_INTERNAL);

  return result;
}

/* Interpret a script that defines a ClayImageProcedure.
 *
 * Similar to v2 code in script-fu-interface.c, except:
 * 1) builds a command from a GValueArray from a ClayConfig,
 *    instead of from local array of SFArg.
 * 2) adds actual args image, drawable, etc. for ClayImageProcedure
 */
ClayValueArray *
script_fu_interpret_image_proc (ClayProcedure        *procedure,
                                SFScript             *script,
                                ClayImage            *image,
                                ClayDrawable        **drawables,
                                ClayProcedureConfig  *config)
{
  gchar          *command;
  ClayValueArray *result = NULL;

  command = script_fu_script_get_command_for_image_proc (script, image, drawables, config);
  result = sf_wrap_run_command (procedure, script, command);
  g_free (command);
  return result;
}

/* Interpret a script that defines a ClayProcedure. */
ClayValueArray *
script_fu_interpret_regular_proc (ClayProcedure        *procedure,
                                  SFScript             *script,
                                  ClayProcedureConfig  *config)
{
  gchar          *command;
  ClayValueArray *result = NULL;

  command = script_fu_script_get_command_for_regular_proc (script, config);
  result = sf_wrap_run_command (procedure, script, command);
  g_free (command);
  return result;
}
