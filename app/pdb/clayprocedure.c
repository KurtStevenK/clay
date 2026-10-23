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
#include <sys/types.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "pdb-types.h"

#include "core/clay.h"
#include "core/clay-memsize.h"
#include "core/claychannel.h"
#include "core/claydisplay.h"
#include "core/claylayer.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "path/claypath.h"

#include "claypdbcontext.h"
#include "claypdberror.h"
#include "clayprocedure.h"

#include "clay-intl.h"


static void          clay_procedure_finalize            (GObject         *object);

static gint64        clay_procedure_get_memsize         (ClayObject      *object,
                                                         gint64          *gui_size);

static const gchar * clay_procedure_real_get_label      (ClayProcedure   *procedure);
static const gchar * clay_procedure_real_get_menu_label (ClayProcedure   *procedure);
static const gchar * clay_procedure_real_get_blurb      (ClayProcedure   *procedure);
static const gchar * clay_procedure_real_get_help_id    (ClayProcedure   *procedure);
static gboolean      clay_procedure_real_get_sensitive  (ClayProcedure   *procedure,
                                                         ClayObject      *object,
                                                         const gchar    **tooltip);
static ClayValueArray * clay_procedure_real_execute     (ClayProcedure   *procedure,
                                                         Clay            *clay,
                                                         ClayContext     *context,
                                                         ClayProgress    *progress,
                                                         ClayValueArray  *args,
                                                         GError         **error);
static void        clay_procedure_real_execute_async    (ClayProcedure   *procedure,
                                                         Clay            *clay,
                                                         ClayContext     *context,
                                                         ClayProgress    *progress,
                                                         ClayValueArray  *args,
                                                         ClayDisplay     *display);

static void          clay_procedure_free_help           (ClayProcedure   *procedure);
static void          clay_procedure_free_attribution    (ClayProcedure   *procedure);

static gboolean      clay_procedure_validate_args       (ClayProcedure   *procedure,
                                                         GParamSpec     **param_specs,
                                                         gint             n_param_specs,
                                                         ClayValueArray  *args,
                                                         gboolean         return_vals,
                                                         GError         **error);


G_DEFINE_TYPE (ClayProcedure, clay_procedure, CLAY_TYPE_VIEWABLE)

#define parent_class clay_procedure_parent_class


static void
clay_procedure_class_init (ClayProcedureClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_procedure_finalize;

  clay_object_class->get_memsize = clay_procedure_get_memsize;

  klass->get_label               = clay_procedure_real_get_label;
  klass->get_menu_label          = clay_procedure_real_get_menu_label;
  klass->get_blurb               = clay_procedure_real_get_blurb;
  klass->get_help_id             = clay_procedure_real_get_help_id;
  klass->get_sensitive           = clay_procedure_real_get_sensitive;
  klass->execute                 = clay_procedure_real_execute;
  klass->execute_async           = clay_procedure_real_execute_async;
}

static void
clay_procedure_init (ClayProcedure *procedure)
{
  procedure->proc_type   = CLAY_PDB_PROC_TYPE_INTERNAL;
  procedure->is_private  = FALSE;
}

static void
clay_procedure_finalize (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);
  gint           i;

  clay_procedure_free_help        (procedure);
  clay_procedure_free_attribution (procedure);

  g_clear_pointer (&procedure->deprecated, g_free);
  g_clear_pointer (&procedure->label,      g_free);

  if (procedure->args)
    {
      for (i = 0; i < procedure->num_args; i++)
        g_param_spec_unref (procedure->args[i]);

      g_clear_pointer (&procedure->args, g_free);
    }

  if (procedure->values)
    {
      for (i = 0; i < procedure->num_values; i++)
        g_param_spec_unref (procedure->values[i]);

      g_clear_pointer (&procedure->values, g_free);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_procedure_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);
  gint64         memsize   = 0;
  gint           i;

  if (! procedure->static_help)
    {
      memsize += clay_string_get_memsize (procedure->blurb);
      memsize += clay_string_get_memsize (procedure->help);
      memsize += clay_string_get_memsize (procedure->help_id);
    }

  if (! procedure->static_attribution)
    {
      memsize += clay_string_get_memsize (procedure->authors);
      memsize += clay_string_get_memsize (procedure->copyright);
      memsize += clay_string_get_memsize (procedure->date);
    }

  memsize += clay_string_get_memsize (procedure->deprecated);

  memsize += procedure->num_args * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_args; i++)
    memsize += clay_g_param_spec_get_memsize (procedure->args[i]);

  memsize += procedure->num_values * sizeof (GParamSpec *);

  for (i = 0; i < procedure->num_values; i++)
    memsize += clay_g_param_spec_get_memsize (procedure->values[i]);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static const gchar *
clay_procedure_real_get_label (ClayProcedure *procedure)
{
  gchar *ellipsis;
  gchar *label;

  if (procedure->label)
    return procedure->label;

  label = clay_strip_uline (clay_procedure_get_menu_label (procedure));

  ellipsis = strstr (label, "...");

  if (! ellipsis)
    ellipsis = strstr (label, "\342\200\246" /* U+2026 HORIZONTAL ELLIPSIS */);

  if (ellipsis && ellipsis == (label + strlen (label) - 3))
    *ellipsis = '\0';

  procedure->label = label;

  return procedure->label;
}

static const gchar *
clay_procedure_real_get_menu_label (ClayProcedure *procedure)
{
  return clay_object_get_name (procedure); /* lame fallback */
}

static const gchar *
clay_procedure_real_get_blurb (ClayProcedure *procedure)
{
  return procedure->blurb;
}

static const gchar *
clay_procedure_real_get_help_id (ClayProcedure *procedure)
{
  if (procedure->help_id)
    return procedure->help_id;

  return clay_object_get_name (procedure);
}

static gboolean
clay_procedure_real_get_sensitive (ClayProcedure  *procedure,
                                   ClayObject     *object,
                                   const gchar   **tooltip)
{
  return TRUE /* random fallback */;
}

static ClayValueArray *
clay_procedure_real_execute (ClayProcedure   *procedure,
                             Clay            *clay,
                             ClayContext     *context,
                             ClayProgress    *progress,
                             ClayValueArray  *args,
                             GError         **error)
{
  g_return_val_if_fail (clay_value_array_length (args) >=
                        procedure->num_args, NULL);

  return procedure->marshal_func (procedure, clay,
                                  context, progress,
                                  args, error);
}

static void
clay_procedure_real_execute_async (ClayProcedure  *procedure,
                                   Clay           *clay,
                                   ClayContext    *context,
                                   ClayProgress   *progress,
                                   ClayValueArray *args,
                                   ClayDisplay    *display)
{
  ClayValueArray *return_vals;
  GError         *error = NULL;

  g_return_if_fail (clay_value_array_length (args) >= procedure->num_args);

  return_vals = CLAY_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               clay,
                                                               context,
                                                               progress,
                                                               args,
                                                               &error);

  clay_value_array_unref (return_vals);

  if (error)
    {
      clay_message_literal (clay, G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}


/*  public functions  */

ClayProcedure  *
clay_procedure_new (ClayMarshalFunc marshal_func,
                    gboolean        is_private)
{
  ClayProcedure *procedure;

  g_return_val_if_fail (marshal_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_PROCEDURE, NULL);

  procedure->marshal_func = marshal_func;
  procedure->is_private   = is_private;

  return procedure;
}

void
clay_procedure_set_help (ClayProcedure *procedure,
                         const gchar   *blurb,
                         const gchar   *help,
                         const gchar   *help_id)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_help (procedure);

  procedure->blurb   = g_strdup (blurb);
  procedure->help    = g_strdup (help);
  procedure->help_id = g_strdup (help_id);

  procedure->static_help = FALSE;
}

void
clay_procedure_set_static_help (ClayProcedure *procedure,
                                const gchar   *blurb,
                                const gchar   *help,
                                const gchar   *help_id)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_help (procedure);

  procedure->blurb   = (gchar *) blurb;
  procedure->help    = (gchar *) help;
  procedure->help_id = (gchar *) help_id;

  procedure->static_help = TRUE;
}

void
clay_procedure_take_help (ClayProcedure *procedure,
                          gchar         *blurb,
                          gchar         *help,
                          gchar         *help_id)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_help (procedure);

  procedure->blurb   = blurb;
  procedure->help    = help;
  procedure->help_id = help_id;

  procedure->static_help = FALSE;
}

void
clay_procedure_set_attribution (ClayProcedure *procedure,
                                const gchar   *authors,
                                const gchar   *copyright,
                                const gchar   *date)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_attribution (procedure);

  procedure->authors   = g_strdup (authors);
  procedure->copyright = g_strdup (copyright);
  procedure->date      = g_strdup (date);

  procedure->static_attribution = FALSE;
}

void
clay_procedure_set_static_attribution (ClayProcedure *procedure,
                                       const gchar   *authors,
                                       const gchar   *copyright,
                                       const gchar   *date)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_attribution (procedure);

  procedure->authors   = (gchar *) authors;
  procedure->copyright = (gchar *) copyright;
  procedure->date      = (gchar *) date;

  procedure->static_attribution = TRUE;
}

void
clay_procedure_take_attribution (ClayProcedure *procedure,
                                 gchar         *authors,
                                 gchar         *copyright,
                                 gchar         *date)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_free_attribution (procedure);

  procedure->authors   = authors;
  procedure->copyright = copyright;
  procedure->date      = date;

  procedure->static_attribution = FALSE;
}

void
clay_procedure_set_deprecated (ClayProcedure *procedure,
                               const gchar   *deprecated)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  g_free (procedure->deprecated);
  procedure->deprecated = g_strdup (deprecated);
}

const gchar *
clay_procedure_get_label (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  return CLAY_PROCEDURE_GET_CLASS (procedure)->get_label (procedure);
}

const gchar *
clay_procedure_get_menu_label (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  return CLAY_PROCEDURE_GET_CLASS (procedure)->get_menu_label (procedure);
}

const gchar *
clay_procedure_get_blurb (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  return CLAY_PROCEDURE_GET_CLASS (procedure)->get_blurb (procedure);
}

const gchar *
clay_procedure_get_help (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  return procedure->help;
}

const gchar *
clay_procedure_get_help_id (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  return CLAY_PROCEDURE_GET_CLASS (procedure)->get_help_id (procedure);
}

gboolean
clay_procedure_get_sensitive (ClayProcedure  *procedure,
                              ClayObject     *object,
                              const gchar   **reason)
{
  const gchar *my_reason  = NULL;
  gboolean     sensitive;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (object == NULL || CLAY_IS_OBJECT (object), FALSE);

  sensitive = CLAY_PROCEDURE_GET_CLASS (procedure)->get_sensitive (procedure,
                                                                   object,
                                                                   &my_reason);

  if (reason)
    *reason = my_reason;

  return sensitive;
}

ClayValueArray *
clay_procedure_execute (ClayProcedure   *procedure,
                        Clay            *clay,
                        ClayContext     *context,
                        ClayProgress    *progress,
                        ClayValueArray  *args,
                        GError         **error)
{
  ClayValueArray *return_vals;
  GError         *pdb_error = NULL;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! clay_procedure_validate_args (procedure,
                                      procedure->args, procedure->num_args,
                                      args, FALSE, &pdb_error))
    {
      return_vals = clay_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      if (! error)
        /* If we can't properly propagate the error, at least print it
         * to standard error stream. This makes debugging easier.
         */
        g_printerr ("%s failed to validate arguments: %s\n", G_STRFUNC, pdb_error->message);

      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  if (CLAY_IS_PDB_CONTEXT (context))
    context = g_object_ref (context);
  else
    context = clay_pdb_context_new (clay, context, TRUE);

  if (progress)
    g_object_ref (progress);

  /*  call the procedure  */
  return_vals = CLAY_PROCEDURE_GET_CLASS (procedure)->execute (procedure,
                                                               clay,
                                                               context,
                                                               progress,
                                                               args,
                                                               error);

  if (progress)
    g_object_unref (progress);

  g_object_unref (context);

  if (return_vals)
    {
      switch (g_value_get_enum (clay_value_array_index (return_vals, 0)))
        {
        case CLAY_PDB_CALLING_ERROR:
        case CLAY_PDB_EXECUTION_ERROR:
          /*  If the error has not already been set, construct one
           *  from the error message that is optionally passed with
           *  the return values.
           */
          if (error && *error == NULL &&
              clay_value_array_length (return_vals) > 1 &&
              G_VALUE_HOLDS_STRING (clay_value_array_index (return_vals, 1)))
            {
              GValue      *value   = clay_value_array_index (return_vals, 1);
              const gchar *message = g_value_get_string (value);

              if (message)
                g_set_error_literal (error, CLAY_PDB_ERROR,
                                     CLAY_PDB_ERROR_FAILED,
                                     message);
            }
          break;

        default:
          break;
        }
    }
  else
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      pdb_error = g_error_new (CLAY_PDB_ERROR,
                               CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned no return values"),
                               clay_object_get_name (procedure));

      return_vals = clay_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      if (error && *error == NULL)
        g_propagate_error (error, pdb_error);
      else
        g_error_free (pdb_error);
    }

  return return_vals;
}

void
clay_procedure_execute_async (ClayProcedure  *procedure,
                              Clay           *clay,
                              ClayContext    *context,
                              ClayProgress   *progress,
                              ClayValueArray *args,
                              ClayDisplay    *display,
                              GError        **error)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));
  g_return_if_fail (args != NULL);
  g_return_if_fail (display == NULL || CLAY_IS_DISPLAY (display));
  g_return_if_fail (error == NULL || *error == NULL);

  if (clay_procedure_validate_args (procedure,
                                    procedure->args, procedure->num_args,
                                    args, FALSE, error))
    {
      if (CLAY_IS_PDB_CONTEXT (context))
        context = g_object_ref (context);
      else
        context = clay_pdb_context_new (clay, context, TRUE);

      if (progress)
        g_object_ref (progress);

      CLAY_PROCEDURE_GET_CLASS (procedure)->execute_async (procedure, clay,
                                                           context, progress,
                                                           args, display);

      if (progress)
        g_object_unref (progress);

      g_object_unref (context);
    }
}

ClayValueArray *
clay_procedure_get_arguments (ClayProcedure *procedure)
{
  ClayValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  args = clay_value_array_new (procedure->num_args);

  for (i = 0; i < procedure->num_args; i++)
    {
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->args[i]));
      g_param_value_set_default (procedure->args[i], &value);
      clay_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

ClayValueArray *
clay_procedure_get_return_values (ClayProcedure *procedure,
                                  gboolean       success,
                                  const GError  *error)
{
  ClayValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (success == FALSE || CLAY_IS_PROCEDURE (procedure),
                        NULL);

  if (success)
    {
      args = clay_value_array_new (procedure->num_values + 1);

      g_value_init (&value, CLAY_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, CLAY_PDB_SUCCESS);
      clay_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->num_values; i++)
        {
          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (procedure->values[i]));
          g_param_value_set_default (procedure->values[i], &value);
          clay_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }
  else
    {
      args = clay_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, CLAY_TYPE_PDB_STATUS_TYPE);

      /*  errors in the CLAY_PDB_ERROR domain are calling errors  */
      if (error && error->domain == CLAY_PDB_ERROR)
        {
          switch ((ClayPdbErrorCode) error->code)
            {
            case CLAY_PDB_ERROR_FAILED:
            case CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND:
            case CLAY_PDB_ERROR_INVALID_ARGUMENT:
            case CLAY_PDB_ERROR_INVALID_RETURN_VALUE:
            case CLAY_PDB_ERROR_INTERNAL_ERROR:
              g_value_set_enum (&value, CLAY_PDB_CALLING_ERROR);
              break;

            case CLAY_PDB_ERROR_CANCELLED:
              g_value_set_enum (&value, CLAY_PDB_CANCEL);
              break;

            default:
              clay_assert_not_reached ();
            }
        }
      else
        {
          g_value_set_enum (&value, CLAY_PDB_EXECUTION_ERROR);
        }

      clay_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          clay_value_array_append (args, &value);
          g_value_unset (&value);
        }
    }

  return args;
}

void
clay_procedure_add_argument (ClayProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->args = g_renew (GParamSpec *, procedure->args,
                             procedure->num_args + 1);

  procedure->args[procedure->num_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_args++;
}

void
clay_procedure_add_return_value (ClayProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  procedure->values = g_renew (GParamSpec *, procedure->values,
                               procedure->num_values + 1);

  procedure->values[procedure->num_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->num_values++;
}

/**
 * clay_procedure_create_override:
 * @procedure:
 * @new_marshal_func:
 *
 * Creates a new ClayProcedure that can be used to override the
 * existing @procedure.
 *
 * Returns: The new #ClayProcedure.
 **/
ClayProcedure *
clay_procedure_create_override (ClayProcedure   *procedure,
                                ClayMarshalFunc  new_marshal_func)
{
  ClayProcedure *new_procedure = NULL;
  const gchar   *name          = NULL;
  int            i             = 0;

  new_procedure = clay_procedure_new (new_marshal_func, procedure->is_private);
  name          = clay_object_get_name (procedure);

  clay_object_set_static_name (CLAY_OBJECT (new_procedure), name);

  for (i = 0; i < procedure->num_args; i++)
    clay_procedure_add_argument (new_procedure, procedure->args[i]);

  for (i = 0; i < procedure->num_values; i++)
    clay_procedure_add_return_value (new_procedure, procedure->values[i]);

  return new_procedure;
}

gint
clay_procedure_name_compare (ClayProcedure *proc1,
                             ClayProcedure *proc2)
{
  /* Assume there always is a name, don't bother with NULL checks */
  return strcmp (clay_object_get_name (proc1),
                 clay_object_get_name (proc2));
}

/*  private functions  */

static void
clay_procedure_free_help (ClayProcedure *procedure)
{
  if (! procedure->static_help)
    {
      g_free (procedure->blurb);
      g_free (procedure->help);
      g_free (procedure->help_id);
    }

  procedure->blurb   = NULL;
  procedure->help    = NULL;
  procedure->help_id = NULL;

  procedure->static_help = FALSE;
}

static void
clay_procedure_free_attribution (ClayProcedure *procedure)
{
  if (! procedure->static_attribution)
    {
      g_free (procedure->authors);
      g_free (procedure->copyright);
      g_free (procedure->date);
    }

  procedure->authors   = NULL;
  procedure->copyright = NULL;
  procedure->date      = NULL;

  procedure->static_attribution = FALSE;
}

static gboolean
clay_procedure_validate_args (ClayProcedure  *procedure,
                              GParamSpec    **param_specs,
                              gint            n_param_specs,
                              ClayValueArray *args,
                              gboolean        return_vals,
                              GError        **error)
{
  gint i;

  for (i = 0; i < MIN (clay_value_array_length (args), n_param_specs); i++)
    {
      GValue     *arg       = clay_value_array_index (args, i);
      GParamSpec *pspec     = param_specs[i];
      GType       arg_type  = G_VALUE_TYPE (arg);
      GType       spec_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (! g_type_is_a (arg_type, spec_type))
        {
          if (return_vals)
            {
              g_set_error (error,
                           CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                           _("Procedure '%s' returned a wrong value type "
                             "for return value '%s' (#%d). "
                             "Expected %s, got %s."),
                           clay_object_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }
          else
            {
              g_set_error (error,
                           CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           _("Procedure '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           clay_object_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else if (! (pspec->flags & CLAY_PARAM_NO_VALIDATE))
        {
          GObject *old_value    = NULL;
          GValue   string_value = G_VALUE_INIT;

          g_value_init (&string_value, G_TYPE_STRING);

          if (g_value_type_transformable (arg_type, G_TYPE_STRING))
            g_value_transform (arg, &string_value);
          else
            g_value_set_static_string (&string_value,
                                       "<not transformable to string>");

          if (G_IS_PARAM_SPEC_OBJECT (pspec))
            old_value = g_value_dup_object (arg);

          if (g_param_value_validate (pspec, arg))
            {
              if (CLAY_IS_PARAM_SPEC_DRAWABLE (pspec) &&
                  g_value_get_object (arg) == NULL)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on a layer that doesn't "
                                     "exist any longer."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                }
              else if (CLAY_IS_PARAM_SPEC_IMAGE (pspec) &&
                       g_value_get_object (arg) == NULL)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid ID for argument '%s'. "
                                     "Most likely a plug-in is trying "
                                     "to work on an image that doesn't "
                                     "exist any longer."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                }
              else if (CLAY_IS_PARAM_SPEC_UNIT (pspec) &&
                       ((CLAY_UNIT (old_value) == clay_unit_pixel () &&
                         ! clay_param_spec_unit_pixel_allowed (pspec)) ||
                        (CLAY_UNIT (old_value) == clay_unit_percent () &&
                         ! clay_param_spec_unit_percent_allowed (pspec))))
                {
                  if (return_vals)
                    {
                      if (CLAY_UNIT (old_value) == clay_unit_pixel ())
                        g_set_error (error,
                                     CLAY_PDB_ERROR,
                                     CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                     _("Procedure '%s' returned "
                                       "`clay_unit_pixel()` as ClayUnit return value #%d '%s'. "
                                       "This return value does not allow pixel unit."),
                                     clay_object_get_name (procedure),
                                     i + 1, g_param_spec_get_name (pspec));
                      else
                        g_set_error (error,
                                     CLAY_PDB_ERROR,
                                     CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                     _("Procedure '%s' returned "
                                       "`clay_unit_percent()` as ClayUnit return value #%d '%s'. "
                                       "This return value does not allow percent unit."),
                                     clay_object_get_name (procedure),
                                     i + 1, g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      if (CLAY_UNIT (old_value) == clay_unit_pixel ())
                        g_set_error (error,
                                     CLAY_PDB_ERROR,
                                     CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                     _("Procedure '%s' has been called with "
                                       "`clay_unit_pixel()` for ClayUnit "
                                       "argument #%d '%s'. "
                                       "This argument does not allow pixel unit."),
                                     clay_object_get_name (procedure),
                                     i + 1, g_param_spec_get_name (pspec));
                      else
                        g_set_error (error,
                                     CLAY_PDB_ERROR,
                                     CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                     _("Procedure '%s' has been called with "
                                       "`clay_unit_percent()` for ClayUnit "
                                       "argument #%d '%s'. "
                                       "This argument does not allow percent unit."),
                                     clay_object_get_name (procedure),
                                     i + 1, g_param_spec_get_name (pspec));
                    }
                }
              else
                {
                  const gchar *value = g_value_get_string (&string_value);

                  if (value == NULL)
                    value = "(null)";

                  if (return_vals)
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned "
                                     "'%s' as return value '%s' "
                                     "(#%d, type %s). "
                                     "This value is out of range."),
                                   clay_object_get_name (procedure),
                                   value,
                                   g_param_spec_get_name (pspec),
                                   i + 1, g_type_name (spec_type));
                    }
                  else
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with "
                                     "value '%s' for argument '%s' "
                                     "(#%d, type %s). "
                                     "This value is out of range."),
                                   clay_object_get_name (procedure),
                                   value,
                                   g_param_spec_get_name (pspec),
                                   i + 1, g_type_name (spec_type));
                    }
                }

              g_value_unset (&string_value);
              g_clear_object (&old_value);

              return FALSE;
            }
          g_clear_object (&old_value);

          /*  UTT-8 validate all strings  */
          if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
              (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_BOXED &&
               G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRV))
            {
              gboolean valid = TRUE;

              if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING)
                {
                  const gchar *string = g_value_get_string (arg);

                  if (string)
                    valid = g_utf8_validate (string, -1, NULL);
                }
              else
                {
                  const gchar **array = g_value_get_boxed (arg);

                  if (array)
                    {
                      gint i;

                      for (i = 0; array[i]; i++)
                        {
                          valid = g_utf8_validate (array[i], -1, NULL);
                          if (! valid)
                            break;
                        }
                    }
                }

              if (! valid)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   clay_object_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }

                  g_value_unset (&string_value);

                  return FALSE;
                }
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}
