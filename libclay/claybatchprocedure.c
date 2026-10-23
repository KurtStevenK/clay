/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybatchprocedure.c
 * Copyright (C) 2022 Jehan
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

#include "clay.h"
#include "claybatchprocedure.h"
#include "clayprocedureconfig-private.h"
#include "claypdb_pdb.h"


/**
 * ClayBatchProcedure:
 *
 * Batch procedures implement an interpreter able to run commands as input.
 *
 * In particular, batch procedures will be available on the command line
 * through the `--batch-interpreter` option to switch the chosen interpreter.
 * Then any command given through the `--batch` option will have to be in the
 * chosen language.
 *
 * It makes Clay usable on the command line, but also to process small scripts
 * (without making full-featured plug-ins), fully in command line without
 * graphical interface.
 **/

struct _ClayBatchProcedure
{
  ClayProcedure    parent_instance;

  gchar           *interpreter_name;

  ClayBatchFunc    run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;
};


static void   clay_batch_procedure_constructed (GObject              *object);
static void   clay_batch_procedure_finalize    (GObject              *object);

static void   clay_batch_procedure_install     (ClayProcedure        *procedure);
static ClayValueArray *
              clay_batch_procedure_run         (ClayProcedure        *procedure,
                                                const ClayValueArray *args);
static ClayProcedureConfig *
            clay_batch_procedure_create_config (ClayProcedure        *procedure,
                                                GParamSpec          **args,
                                                gint                  n_args);

G_DEFINE_TYPE (ClayBatchProcedure, clay_batch_procedure, CLAY_TYPE_PROCEDURE)

#define parent_class clay_batch_procedure_parent_class


static void
clay_batch_procedure_class_init (ClayBatchProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed      = clay_batch_procedure_constructed;
  object_class->finalize         = clay_batch_procedure_finalize;

  procedure_class->install       = clay_batch_procedure_install;
  procedure_class->run           = clay_batch_procedure_run;
  procedure_class->create_config = clay_batch_procedure_create_config;
}

static void
clay_batch_procedure_init (ClayBatchProcedure *procedure)
{
}

static void
clay_batch_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    CLAY_TYPE_RUN_MODE,
                                    CLAY_RUN_NONINTERACTIVE,
                                    G_PARAM_READWRITE);

  clay_procedure_add_string_argument (procedure, "script",
                                      "Batch commands in the target language",
                                      "Batch commands in the target language, which will be run by the interpreter",
                                      "",
                                      G_PARAM_READWRITE);
}

static void
clay_batch_procedure_finalize (GObject *object)
{
  ClayBatchProcedure *procedure = CLAY_BATCH_PROCEDURE (object);

  g_clear_pointer (&procedure->interpreter_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_batch_procedure_install (ClayProcedure *procedure)
{
  ClayBatchProcedure *proc = CLAY_BATCH_PROCEDURE (procedure);

  g_return_if_fail (proc->interpreter_name != NULL);

  CLAY_PROCEDURE_CLASS (parent_class)->install (procedure);

  _clay_pdb_set_batch_interpreter (clay_procedure_get_name (procedure),
                                   proc->interpreter_name);
}

#define ARG_OFFSET 2

static ClayValueArray *
clay_batch_procedure_run (ClayProcedure        *procedure,
                          const ClayValueArray *args)
{
  ClayBatchProcedure  *batch_proc = CLAY_BATCH_PROCEDURE (procedure);
  ClayValueArray      *remaining;
  ClayValueArray      *return_values;
  ClayRunMode          run_mode;
  const gchar         *cmd;
  ClayProcedureConfig *config;
  ClayPDBStatusType    status = CLAY_PDB_EXECUTION_ERROR;
  gint                 i;

  run_mode = CLAY_VALUES_GET_ENUM   (args, 0);
  cmd      = CLAY_VALUES_GET_STRING (args, 1);

  remaining = clay_value_array_new (clay_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < clay_value_array_length (args); i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_array_append (remaining, value);
    }

  config = _clay_procedure_create_run_config (procedure);
  _clay_procedure_config_begin_run (config, NULL, run_mode, remaining, NULL);
  clay_value_array_unref (remaining);

  return_values = batch_proc->run_func (procedure, run_mode,
                                        cmd, config,
                                        batch_proc->run_data);
  if (return_values != NULL                       &&
      clay_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (return_values, 0)))
    status = CLAY_VALUES_GET_ENUM (return_values, 0);

  _clay_procedure_config_end_run (config, status);
  g_object_unref (config);

  return return_values;
}

static ClayProcedureConfig *
clay_batch_procedure_create_config (ClayProcedure  *procedure,
                                    GParamSpec    **args,
                                    gint            n_args)
{
  if (n_args > ARG_OFFSET)
    {
      args   += ARG_OFFSET;
      n_args -= ARG_OFFSET;
    }
  else
    {
      args   = NULL;
      n_args = 0;
    }

  return CLAY_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}

/*  public functions  */

/**
 * clay_batch_procedure_new:
 * @plug_in:          a #ClayPlugIn.
 * @name:             the new procedure's name.
 * @interpreter_name: the public-facing name, e.g. "Python 3".
 * @proc_type:        the new procedure's #ClayPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new batch interpreter procedure named @name which will call
 * @run_func when invoked.
 *
 * See clay_procedure_new() for information about @proc_type.
 *
 * #ClayBatchProcedure is a #ClayProcedure subclass that makes it easier
 * to write batch interpreter procedures.
 *
 * It automatically adds the standard
 *
 * (#ClayRunMode, #gchar)
 *
 * arguments of a batch procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via clay_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #ClayBatchFunc. The "args"
 * #ClayValueArray of #ClayRunSaveFunc only contains additionally added
 * arguments.
 *
 * Returns: a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure  *
clay_batch_procedure_new (ClayPlugIn       *plug_in,
                          const gchar      *name,
                          const gchar      *interpreter_name,
                          ClayPDBProcType   proc_type,
                          ClayBatchFunc     run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  ClayBatchProcedure *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_BATCH_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  clay_batch_procedure_set_interpreter_name (procedure, interpreter_name);

  return CLAY_PROCEDURE (procedure);
}

/**
 * clay_batch_procedure_set_interpreter_name:
 * @procedure:        A batch procedure.
 * @interpreter_name: A public-facing name for the interpreter, e.g. "Python 3".
 *
 * Associates an interpreter name with a batch procedure.
 *
 * This name can be used for any public-facing strings, such as
 * graphical interface labels or command line usage. E.g. the command
 * line interface could list all available interface, displaying both a
 * procedure name and a "pretty printing" title.
 *
 * Note that since the format name is public-facing, it is recommended
 * to localize it at runtime, for instance through gettext, like:
 *
 * ```c
 * clay_batch_procedure_set_interpreter_name (procedure, _("Python 3"));
 * ```
 *
 * Some language would indeed localize even some technical terms or
 * acronyms, even if sometimes just to rewrite them with the local
 * writing system.
 *
 * Since: 3.0
 **/
void
clay_batch_procedure_set_interpreter_name (ClayBatchProcedure *procedure,
                                           const gchar        *interpreter_name)
{
  g_return_if_fail (CLAY_IS_BATCH_PROCEDURE (procedure));
  g_return_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL));

  g_free (procedure->interpreter_name);
  procedure->interpreter_name = g_strdup (interpreter_name);
}

/**
 * clay_batch_procedure_get_interpreter_name:
 * @procedure: A batch procedure object.
 *
 * Returns the procedure's interpreter name, as set with
 * [method@BatchProcedure.set_interpreter_name].
 *
 * Returns: The procedure's interpreter name.
 *
 * Since: 3.0
 **/
const gchar *
clay_batch_procedure_get_interpreter_name (ClayBatchProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_BATCH_PROCEDURE (procedure), NULL);

  return procedure->interpreter_name;
}
