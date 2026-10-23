/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybatchprocedure.h
 * Copyright (C) 2022 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_BATCH_PROCEDURE_H__
#define __CLAY_BATCH_PROCEDURE_H__

#include <libclay/clayprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayBatchFunc:
 * @procedure:   the #ClayProcedure that runs.
 * @run_mode:    the #ClayRunMode.
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in clay_batch_procedure_new().
 *
 * The batch function is run during the lifetime of the Clay session,
 * each time a plug-in batch procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef ClayValueArray * (* ClayBatchFunc) (ClayProcedure        *procedure,
                                            ClayRunMode           run_mode,
                                            const gchar          *command,
                                            ClayProcedureConfig  *config,
                                            gpointer              run_data);


#define CLAY_TYPE_BATCH_PROCEDURE (clay_batch_procedure_get_type ())
G_DECLARE_FINAL_TYPE (ClayBatchProcedure, clay_batch_procedure, CLAY, BATCH_PROCEDURE, ClayProcedure)


ClayProcedure * clay_batch_procedure_new                  (ClayPlugIn      *plug_in,
                                                           const gchar     *name,
                                                           const gchar     *interpreter_name,
                                                           ClayPDBProcType  proc_type,
                                                           ClayBatchFunc    run_func,
                                                           gpointer         run_data,
                                                           GDestroyNotify   run_data_destroy);

void            clay_batch_procedure_set_interpreter_name (ClayBatchProcedure *procedure,
                                                           const gchar        *interpreter_name);
const gchar *   clay_batch_procedure_get_interpreter_name (ClayBatchProcedure *procedure);

G_END_DECLS

#endif  /*  __CLAY_BATCH_PROCEDURE_H__  */
