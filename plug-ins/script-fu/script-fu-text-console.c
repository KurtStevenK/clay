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

#include <stdio.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libclay/clay.h"

#include "script-fu-text-console.h"

#include "script-fu-intl.h"
#include "script-fu-lib.h"

ClayValueArray *
script_fu_text_console_run (ClayProcedure        *procedure,
                            ClayProcedureConfig  *config)
{
  script_fu_redirect_output_to_stdout ();

  script_fu_print_welcome ();

  clay_plug_in_set_pdb_error_handler (clay_procedure_get_plug_in (procedure),
                                      CLAY_PDB_ERROR_HANDLER_PLUGIN);

  script_fu_run_read_eval_print_loop ();

  clay_plug_in_set_pdb_error_handler (clay_procedure_get_plug_in (procedure),
                                      CLAY_PDB_ERROR_HANDLER_INTERNAL);

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}
