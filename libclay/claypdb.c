/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdb.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/clayprotocol-private.h"
#include "libclaybase/claywire-private.h"

#include "claygpparams.h"
#include "claypdb-private.h"
#include "claypdb_pdb.h"
#include "claypdbprocedure.h"
#include "clayplugin-private.h"

#include "libclay-intl.h"


/**
 * ClayPDB:
 *
 * Provides access to the Procedural DataBase (PDB).
 */


struct _ClayPDB
{
  GObject             parent_instance;

  ClayPlugIn         *plug_in;

  GHashTable         *procedures;

  ClayPDBStatusType   error_status;
  gchar              *error_message;
};


static void   clay_pdb_dispose   (GObject        *object);
static void   clay_pdb_finalize  (GObject        *object);

static void   clay_pdb_set_error (ClayPDB        *pdb,
                                  ClayValueArray *return_values);


G_DEFINE_TYPE (ClayPDB, clay_pdb, G_TYPE_OBJECT)

#define parent_class clay_pdb_parent_class


static void
clay_pdb_class_init (ClayPDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = clay_pdb_dispose;
  object_class->finalize = clay_pdb_finalize;
}

static void
clay_pdb_init (ClayPDB *pdb)
{
  pdb->procedures = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, g_object_unref);

  pdb->error_status = CLAY_PDB_SUCCESS;
}

static void
clay_pdb_dispose (GObject *object)
{
  ClayPDB *pdb = CLAY_PDB (object);

  g_clear_pointer (&pdb->procedures, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_pdb_finalize (GObject *object)
{
  ClayPDB *pdb = CLAY_PDB (object);

  g_clear_object (&pdb->plug_in);
  g_clear_pointer (&pdb->error_message, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

ClayPDB *
_clay_pdb_new (ClayPlugIn *plug_in)
{
  ClayPDB *pdb;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  pdb = g_object_new (CLAY_TYPE_PDB, NULL);

  pdb->plug_in = g_object_ref (plug_in);

  return pdb;
}

ClayPlugIn *
_clay_pdb_get_plug_in (ClayPDB *pdb)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);

  return pdb->plug_in;
}

/**
 * clay_pdb_procedure_exists:
 * @pdb:            A PDB instance.
 * @procedure_name: A procedure name
 *
 * This function checks if a procedure exists in the procedural
 * database.
 *
 * Return: %TRUE if the procedure exists, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_pdb_procedure_exists (ClayPDB     *pdb,
                           const gchar *procedure_name)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (clay_is_canonical_identifier (procedure_name), FALSE);

  return _clay_pdb_proc_exists (procedure_name);
}

/**
 * clay_pdb_lookup_procedure:
 * @pdb:            A #ClayPDB instance.
 * @procedure_name: A procedure name
 *
 * This function returns the [class@Procedure] which is registered
 * with @procedure_name if it exists, or returns %NULL otherwise.
 *
 * The returned [class@Procedure] is owned by @pdb and must not be modified.
 *
 * Return: (nullable) (transfer none): A [class@Procedure], or %NULL.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_pdb_lookup_procedure (ClayPDB     *pdb,
                           const gchar *procedure_name)
{
  ClayProcedure *procedure;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (procedure_name), NULL);

  procedure = g_hash_table_lookup (pdb->procedures, procedure_name);

  if (! procedure && clay_is_canonical_identifier (procedure_name) &&
      _clay_pdb_proc_exists (procedure_name))
    {
      procedure = _clay_pdb_procedure_new (pdb, procedure_name);

      if (procedure)
        g_hash_table_insert (pdb->procedures,
                             g_strdup (procedure_name), procedure);
    }

  return procedure;
}

/**
 * clay_pdb_temp_procedure_name:
 * @pdb: the #ClayPDB object.
 *
 * Generates a unique temporary PDB name.
 *
 * This function generates a temporary PDB entry name that is
 * guaranteed to be unique.
 *
 * Returns: (transfer full): A unique temporary name for a temporary
 *          PDB entry. The returned value must be freed with
 *          g_free().
 *
 * Since: 3.0
 **/
gchar *
clay_pdb_temp_procedure_name (ClayPDB *pdb)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);

  return _clay_pdb_temp_name ();
}

/**
 * clay_pdb_dump_to_file:
 * @pdb:  A #ClayPDB.
 * @file: The dump file.
 *
 * Dumps the current contents of the procedural database
 *
 * This procedure dumps the contents of the procedural database to the
 * specified @file. The file will contain all of the information
 * provided for each registered procedure.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
clay_pdb_dump_to_file (ClayPDB *pdb,
                       GFile   *file)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  return _clay_pdb_dump (file);
}

/**
 * clay_pdb_query_procedures:
 * @pdb:         A #ClayPDB.
 * @name:        The regex for procedure name.
 * @blurb:       The regex for procedure blurb.
 * @help:        The regex for procedure help.
 * @help_id:     The regex for procedure help-id.
 * @authors:     The regex for procedure authors.
 * @copyright:   The regex for procedure copyright.
 * @date:        The regex for procedure date.
 * @proc_type:   The regex for procedure type: { 'Internal Clay procedure', 'Clay Plug-in', 'Clay Extension', 'Temporary Procedure' }.
 *
 * Queries the procedural database for its contents using regular
 * expression matching.
 *
 * This function queries the contents of the procedural database. It
 * is supplied with eight arguments matching procedures on
 *
 * { name, blurb, help, help-id, authors, copyright, date, procedure type}.
 *
 * This is accomplished using regular expression matching. For
 * instance, to find all procedures with "jpeg" listed in the blurb,
 * all seven arguments can be supplied as ".*", except for the second,
 * which can be supplied as ".*jpeg.*". There are two return arguments
 * for this procedure. The first is the number of procedures matching
 * the query. The second is a concatenated list of procedure names
 * corresponding to those matching the query. If no matching entries
 * are found, then the returned string is NULL and the number of
 * entries is 0.
 *
 * Returns: (array zero-terminated=1) (transfer full): The list
 *          of procedure names. Free with g_strfreev().
 *
 * Since: 3.0
 **/
gchar **
clay_pdb_query_procedures (ClayPDB     *pdb,
                           const gchar *name,
                           const gchar *blurb,
                           const gchar *help,
                           const gchar *help_id,
                           const gchar *authors,
                           const gchar *copyright,
                           const gchar *date,
                           const gchar *proc_type)
{
  gchar **matches;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);

  _clay_pdb_query (name,
                   blurb, help, /* FIXME help_id */
                   authors, copyright, date,
                   proc_type,
                   &matches);

  return matches;
}

GQuark
_clay_pdb_error_quark (void)
{
  return g_quark_from_static_string ("clay-pdb-error-quark");
}


/**
 * clay_pdb_get_last_error:
 * @pdb: a #ClayPDB.
 *
 * Retrieves the error message from the last procedure call.
 *
 * If a procedure call fails, then it might pass an error message with
 * the return values. Plug-ins that are using the libclay C wrappers
 * don't access the procedure return values directly. Thus #ClayPDB
 * stores the error message and makes it available with this
 * function. The next procedure call unsets the error message again.
 *
 * The returned string is owned by @pdb and must not be freed or
 * modified.
 *
 * Returns: the error message
 *
 * Since: 3.0
 **/
const gchar *
clay_pdb_get_last_error (ClayPDB *pdb)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);

  if (pdb->error_message && strlen (pdb->error_message))
    return pdb->error_message;

  switch (pdb->error_status)
    {
    case CLAY_PDB_SUCCESS:
      /*  procedure executed successfully  */
      return _("success");

    case CLAY_PDB_EXECUTION_ERROR:
      /*  procedure execution failed       */
      return _("execution error");

    case CLAY_PDB_CALLING_ERROR:
      /*  procedure called incorrectly     */
      return _("calling error");

    case CLAY_PDB_CANCEL:
      /*  procedure execution cancelled    */
      return _("cancelled");

    default:
      return "invalid return status";
    }
}

/**
 * clay_pdb_get_last_status:
 * @pdb: a #ClayPDB.
 *
 * Retrieves the status from the last procedure call.
 *
 * Returns: the #ClayPDBStatusType.
 *
 * Since: 3.0
 **/
ClayPDBStatusType
clay_pdb_get_last_status (ClayPDB *pdb)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), CLAY_PDB_SUCCESS);

  return pdb->error_status;
}

/*  Cruft API  */

/**
 * clay_pdb_get_data:
 * @identifier: The identifier associated with data.
 * @data: A byte array containing data.
 *
 * Returns data associated with the specified identifier.
 *
 * This procedure returns any data which may have been associated with
 * the specified identifier. The data is copied into the given memory
 * location.
 *
 * WARNING: this function is exported in the library so that it can be used by
 * libclaywidgets. Nevertheless it is considered internal, and is not declared
 * in any public header on purpose. It should not be considered part of the API
 * and therefore should not be used in plug-ins. It may disappear at any time.
 *
 * Returns: TRUE on success, FALSE if no data has been associated with
 * the identifier
 */
gboolean
clay_pdb_get_data (const gchar  *identifier,
                   GBytes      **data)
{
  return _clay_pdb_get_data (identifier, data);
}

/**
 * clay_pdb_set_data:
 * @identifier: The identifier associated with data.
 * @data: (array length=data_len): A byte array containing data.
 * @data_len: The number of bytes in the data
 *
 * Associates the specified identifier with the supplied data.
 *
 * This procedure associates the supplied data with the provided
 * identifier. The data may be subsequently retrieved by a call to
 * 'procedural-db-get-data'. This storage is global within the session, even
 * shareable between plug-ins, though it won't survive a restart of Clay.
 *
 * WARNING: this function is exported in the library so that it can be used by
 * libclaywidgets. Nevertheless it is considered internal, and is not declared
 * in any public header on purpose. It should not be considered part of the API
 * and therefore should not be used in plug-ins. It may disappear at any time.

 * Returns: TRUE on success.
 */
gboolean
clay_pdb_set_data (const gchar *identifier,
                   GBytes      *data)
{
  return _clay_pdb_set_data (identifier, data);
}

/**
 * _clay_pdb_run_procedure_array:
 * @pdb:            the #ClayPDB object.
 * @procedure_name: the procedure registered name.
 * @arguments:      the call arguments.
 *
 * Runs the procedure named @procedure_name with @arguments.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
ClayValueArray *
_clay_pdb_run_procedure_array (ClayPDB              *pdb,
                               const gchar          *procedure_name,
                               const ClayValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  ClayWireMessage  msg;
  ClayValueArray  *return_values;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (procedure_name), NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  proc_run.name     = (gchar *) procedure_name;
  proc_run.n_params = clay_value_array_length (arguments);
  proc_run.params   = _clay_value_array_to_gp_params (arguments, FALSE);

  if (! gp_proc_run_write (_clay_plug_in_get_write_channel (pdb->plug_in),
                           &proc_run, pdb->plug_in))
    _clay_quit ();

  _clay_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

  _clay_plug_in_read_expect_msg (pdb->plug_in, &msg, GP_PROC_RETURN);

  proc_return = msg.data;

  return_values = _clay_gp_params_to_value_array (NULL,
                                                  NULL, 0,
                                                  proc_return->params,
                                                  proc_return->n_params,
                                                  TRUE);

  clay_wire_destroy (&msg);

  clay_pdb_set_error (pdb, return_values);

  return return_values;
}


/*  private functions  */

static void
clay_pdb_set_error (ClayPDB        *pdb,
                    ClayValueArray *return_values)
{
  g_clear_pointer (&pdb->error_message, g_free);
  pdb->error_status = CLAY_PDB_SUCCESS;

  if (clay_value_array_length (return_values) > 0)
    {
      pdb->error_status = CLAY_VALUES_GET_ENUM (return_values, 0);

      switch (pdb->error_status)
        {
        case CLAY_PDB_SUCCESS:
        case CLAY_PDB_PASS_THROUGH:
          break;

        case CLAY_PDB_EXECUTION_ERROR:
        case CLAY_PDB_CALLING_ERROR:
        case CLAY_PDB_CANCEL:
          if (clay_value_array_length (return_values) > 1)
            {
              GValue *value = clay_value_array_index (return_values, 1);

              if (G_VALUE_HOLDS_STRING (value))
                pdb->error_message = g_value_dup_string (value);
            }
          break;
        }
    }
}
