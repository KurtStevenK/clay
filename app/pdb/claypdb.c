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

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include <gobject/gvaluecollector.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "pdb-types.h"

#include "core/clay.h"
#include "core/clayitem.h"
#include "core/clay-memsize.h"
#include "core/claycontext.h"
#include "core/clayprogress.h"

#include "claypdb.h"
#include "claypdberror.h"
#include "clayprocedure.h"

#include "clay-intl.h"


enum
{
  REGISTER_PROCEDURE,
  UNREGISTER_PROCEDURE,
  LAST_SIGNAL
};


static void     clay_pdb_finalize                  (GObject       *object);
static gint64   clay_pdb_get_memsize               (ClayObject    *object,
                                                    gint64        *gui_size);
static void     clay_pdb_real_register_procedure   (ClayPDB       *pdb,
                                                    ClayProcedure *procedure);
static void     clay_pdb_real_unregister_procedure (ClayPDB       *pdb,
                                                    ClayProcedure *procedure);
static void     clay_pdb_entry_free                (gpointer       key,
                                                    gpointer       value,
                                                    gpointer       user_data);
static gint64   clay_pdb_entry_get_memsize         (GList         *procedures,
                                                    gint64        *gui_size);


G_DEFINE_TYPE (ClayPDB, clay_pdb, CLAY_TYPE_OBJECT)

#define parent_class clay_pdb_parent_class

static guint clay_pdb_signals[LAST_SIGNAL] = { 0 };


static void
clay_pdb_class_init (ClayPDBClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  clay_pdb_signals[REGISTER_PROCEDURE] =
    g_signal_new ("register-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPDBClass, register_procedure),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PROCEDURE);

  clay_pdb_signals[UNREGISTER_PROCEDURE] =
    g_signal_new ("unregister-procedure",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPDBClass, unregister_procedure),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PROCEDURE);

  object_class->finalize         = clay_pdb_finalize;

  clay_object_class->get_memsize = clay_pdb_get_memsize;

  klass->register_procedure      = clay_pdb_real_register_procedure;
  klass->unregister_procedure    = clay_pdb_real_unregister_procedure;
}

static void
clay_pdb_init (ClayPDB *pdb)
{
  pdb->procedures        = g_hash_table_new (g_str_hash, g_str_equal);
  pdb->compat_proc_names = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
clay_pdb_finalize (GObject *object)
{
  ClayPDB *pdb = CLAY_PDB (object);

  if (pdb->procedures)
    {
      g_hash_table_foreach (pdb->procedures, clay_pdb_entry_free, NULL);
      g_hash_table_destroy (pdb->procedures);
      pdb->procedures = NULL;
    }

  if (pdb->compat_proc_names)
    {
      g_hash_table_destroy (pdb->compat_proc_names);
      pdb->compat_proc_names = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_pdb_get_memsize (ClayObject *object,
                      gint64     *gui_size)
{
  ClayPDB *pdb     = CLAY_PDB (object);
  gint64   memsize = 0;

  memsize += clay_g_hash_table_get_memsize_foreach (pdb->procedures,
                                                    (ClayMemsizeFunc)
                                                    clay_pdb_entry_get_memsize,
                                                    gui_size);
  memsize += clay_g_hash_table_get_memsize (pdb->compat_proc_names, 0);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_pdb_real_register_procedure (ClayPDB       *pdb,
                                  ClayProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = clay_object_get_name (procedure);

  list = g_hash_table_lookup (pdb->procedures, name);

  g_hash_table_replace (pdb->procedures, (gpointer) name,
                        g_list_prepend (list, g_object_ref (procedure)));
}

static void
clay_pdb_real_unregister_procedure (ClayPDB       *pdb,
                                    ClayProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  name = clay_object_get_name (procedure);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    {
      list = g_list_remove (list, procedure);

      if (list)
        {
          name = clay_object_get_name (list->data);
          g_hash_table_replace (pdb->procedures, (gpointer) name, list);
        }
      else
        {
          g_hash_table_remove (pdb->procedures, name);
        }

      g_object_unref (procedure);
    }
}


/*  public functions  */

ClayPDB *
clay_pdb_new (Clay *clay)
{
  ClayPDB *pdb;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  pdb = g_object_new (CLAY_TYPE_PDB,
                      "name", "pdb",
                      NULL);

  pdb->clay = clay;

  return pdb;
}

void
clay_pdb_register_procedure (ClayPDB       *pdb,
                             ClayProcedure *procedure)
{
  g_return_if_fail (CLAY_IS_PDB (pdb));
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  if (! procedure->deprecated ||
      pdb->clay->pdb_compat_mode != CLAY_PDB_COMPAT_OFF)
    {
      g_signal_emit (pdb, clay_pdb_signals[REGISTER_PROCEDURE], 0,
                     procedure);
    }
}

void
clay_pdb_unregister_procedure (ClayPDB       *pdb,
                               ClayProcedure *procedure)
{
  g_return_if_fail (CLAY_IS_PDB (pdb));
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  g_signal_emit (pdb, clay_pdb_signals[UNREGISTER_PROCEDURE], 0,
                 procedure);
}

ClayProcedure *
clay_pdb_lookup_procedure (ClayPDB     *pdb,
                           const gchar *name)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list)
    return list->data;

  return NULL;
}

void
clay_pdb_register_compat_proc_name (ClayPDB     *pdb,
                                    const gchar *old_name,
                                    const gchar *new_name)
{
  g_return_if_fail (CLAY_IS_PDB (pdb));
  g_return_if_fail (old_name != NULL);
  g_return_if_fail (new_name != NULL);

  g_hash_table_insert (pdb->compat_proc_names,
                       (gpointer) old_name,
                       (gpointer) new_name);
}

const gchar *
clay_pdb_lookup_compat_proc_name (ClayPDB     *pdb,
                                  const gchar *old_name)
{
  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (old_name != NULL, NULL);

  return g_hash_table_lookup (pdb->compat_proc_names, old_name);
}

ClayValueArray *
clay_pdb_execute_procedure_by_name_args (ClayPDB         *pdb,
                                         ClayContext     *context,
                                         ClayProgress    *progress,
                                         GError         **error,
                                         const gchar     *name,
                                         ClayValueArray  *args)
{
  ClayValueArray *return_vals = NULL;
  GList          *list;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (pdb->procedures, name);

  if (list == NULL)
    {
      GError *pdb_error = g_error_new (CLAY_PDB_ERROR,
                                       CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                                       _("Procedure '%s' not found"), name);

      return_vals = clay_procedure_get_return_values (NULL, FALSE, pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  g_return_val_if_fail (args != NULL, NULL);

  for (; list; list = g_list_next (list))
    {
      ClayProcedure *procedure = list->data;

      g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

      return_vals = clay_procedure_execute (procedure,
                                            pdb->clay, context, progress,
                                            args, error);

      if (g_value_get_enum (clay_value_array_index (return_vals, 0)) ==
          CLAY_PDB_PASS_THROUGH)
        {
          /*  If the return value is CLAY_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            {
              clay_value_array_unref (return_vals);
              g_clear_error (error);
            }
        }
      else
        {
          /*  No CLAY_PDB_PASS_THROUGH, break out of the list of
           *  procedures and return the current return values.
           */
          break;
        }
    }

  return return_vals;
}

ClayValueArray *
clay_pdb_execute_procedure_by_name (ClayPDB       *pdb,
                                    ClayContext   *context,
                                    ClayProgress  *progress,
                                    GError       **error,
                                    const gchar   *name,
                                    ...)
{
  ClayProcedure  *procedure;
  ClayValueArray *args;
  ClayValueArray *return_vals;
  va_list         va_args;
  gint            i;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  procedure = clay_pdb_lookup_procedure (pdb, name);

  if (! procedure)
    {
      GError *pdb_error = g_error_new (CLAY_PDB_ERROR,
                                       CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                                       _("Procedure '%s' not found"), name);

      return_vals = clay_procedure_get_return_values (NULL, FALSE, pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  args = clay_procedure_get_arguments (procedure);

  va_start (va_args, name);

  for (i = 0; i < procedure->num_args; i++)
    {
      GValue *value;
      GType   arg_type;
      GType   value_type;
      gchar  *error_msg = NULL;

      arg_type = va_arg (va_args, GType);

      if (arg_type == G_TYPE_NONE)
        break;

      value = clay_value_array_index (args, i);

      value_type = G_VALUE_TYPE (value);

      /* G_TYPE_INT is widely abused for enums and booleans in
       * old plug-ins, silently copy stuff into integers when enums
       * and booleans are passed
       */
      if (arg_type != value_type

          &&

          value_type == G_TYPE_INT

          &&

          (arg_type == G_TYPE_BOOLEAN  ||
           g_type_is_a (arg_type, G_TYPE_ENUM)))
        {
          arg_type = value_type;
        }

      if (arg_type != value_type)
        {
          GError      *pdb_error;
          const gchar *expected = g_type_name (value_type);
          const gchar *got      = g_type_name (arg_type);

          clay_value_array_unref (args);

          pdb_error = g_error_new (CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with a "
                                     "wrong type for argument #%d. "
                                     "Expected %s, got %s."),
                                   clay_object_get_name (procedure),
                                   i + 1, expected, got);

          return_vals = clay_procedure_get_return_values (procedure,
                                                          FALSE, pdb_error);
          g_propagate_error (error, pdb_error);

          va_end (va_args);

          return return_vals;
        }

      if (CLAY_VALUE_HOLDS_INT32_ARRAY (value)  ||
          CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value)  ||
          CLAY_VALUE_HOLDS_CORE_OBJECT_ARRAY (value))
        {
          g_value_set_boxed (value, va_arg (va_args, gpointer));
        }
      else
        {
          G_VALUE_COLLECT (value, va_args, G_VALUE_NOCOPY_CONTENTS, &error_msg);
        }

      if (error_msg)
        {
          GError *pdb_error = g_error_new_literal (CLAY_PDB_ERROR,
                                                   CLAY_PDB_ERROR_INTERNAL_ERROR,
                                                   error_msg);
          g_warning ("%s: %s", G_STRFUNC, error_msg);
          g_free (error_msg);

          clay_value_array_unref (args);

          return_vals = clay_procedure_get_return_values (procedure,
                                                          FALSE, pdb_error);
          g_propagate_error (error, pdb_error);

          va_end (va_args);

          return return_vals;
        }
    }

  va_end (va_args);

  return_vals = clay_pdb_execute_procedure_by_name_args (pdb, context,
                                                         progress, error,
                                                         name, args);

  clay_value_array_unref (args);

  return return_vals;
}

/**
 * clay_pdb_get_deprecated_procedures:
 * @pdb:
 *
 * Returns: A new #GList with the deprecated procedures. Free with
 *          g_list_free().
 **/
GList *
clay_pdb_get_deprecated_procedures (ClayPDB *pdb)
{
  GList *result = NULL;
  GList *procs;
  GList *iter;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);

  procs = g_hash_table_get_values (pdb->procedures);

  for (iter = procs;
       iter;
       iter = g_list_next (iter))
    {
      GList *proc_list = iter->data;

      /* Only care about the first procedure in the list */
      ClayProcedure *procedure = CLAY_PROCEDURE (proc_list->data);

      if (procedure->deprecated)
        result = g_list_prepend (result, procedure);
    }

  result = g_list_sort (result, (GCompareFunc) clay_procedure_name_compare);

  g_list_free (procs);

  return result;
}


/*  private functions  */

static void
clay_pdb_entry_free (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  if (value)
    g_list_free_full (value, (GDestroyNotify) g_object_unref);
}

static gint64
clay_pdb_entry_get_memsize (GList  *procedures,
                            gint64 *gui_size)
{
  return clay_g_list_get_memsize_foreach (procedures,
                                          (ClayMemsizeFunc)
                                          clay_object_get_memsize,
                                          gui_size);
}
