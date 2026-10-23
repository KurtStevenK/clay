/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimageprocedure.c
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

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "clayimageprocedure.h"
#include "clayplugin-private.h"
#include "clayprocedureconfig-private.h"


/**
 * ClayImageProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write standard plug-in
 * procedures that operate on drawables.
 *
 * It automatically adds the standard
 *
 * ( [enum@RunMode], [class@Image], [class@Drawable] )
 *
 * arguments of an image procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * "args" [struct@ValueArray] of [callback@RunImageFunc] only contains
 * additionally added arguments.
 */


struct _ClayImageProcedure
{
  ClayProcedure    parent_instance;

  ClayRunImageFunc run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;
};


static void   clay_image_procedure_constructed   (GObject              *object);
static void   clay_image_procedure_finalize      (GObject              *object);

static ClayValueArray *
              clay_image_procedure_run           (ClayProcedure        *procedure,
                                                  const ClayValueArray *args);
static ClayProcedureConfig *
              clay_image_procedure_create_config (ClayProcedure        *procedure,
                                                  GParamSpec          **args,
                                                  gint                  n_args);
static gboolean
            clay_image_procedure_set_sensitivity (ClayProcedure        *procedure,
                                                  gint                  sensitivity_mask);


G_DEFINE_TYPE (ClayImageProcedure, clay_image_procedure, CLAY_TYPE_PROCEDURE)

#define parent_class clay_image_procedure_parent_class


static void
clay_image_procedure_class_init (ClayImageProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed        = clay_image_procedure_constructed;
  object_class->finalize           = clay_image_procedure_finalize;

  procedure_class->run             = clay_image_procedure_run;
  procedure_class->create_config   = clay_image_procedure_create_config;
  procedure_class->set_sensitivity = clay_image_procedure_set_sensitivity;
}

static void
clay_image_procedure_init (ClayImageProcedure *procedure)
{
}

static void
clay_image_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    CLAY_TYPE_RUN_MODE,
                                    CLAY_RUN_NONINTERACTIVE,
                                    G_PARAM_READWRITE);

  clay_procedure_add_image_argument (procedure, "image",
                                     "Image",
                                     "The input image",
                                     FALSE,
                                     G_PARAM_READWRITE);

  clay_procedure_add_core_object_array_argument (procedure, "drawables",
                                                 "Drawables",
                                                 "The input drawables",
                                                 CLAY_TYPE_DRAWABLE,
                                                 G_PARAM_READWRITE | CLAY_PARAM_NO_VALIDATE);

  clay_procedure_set_sensitivity_mask (CLAY_PROCEDURE (procedure),
                                       CLAY_PROCEDURE_SENSITIVE_DRAWABLE |
                                       CLAY_PROCEDURE_SENSITIVE_DRAWABLES);
}

static void
clay_image_procedure_finalize (GObject *object)
{
  ClayImageProcedure *procedure = CLAY_IMAGE_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 3

static ClayValueArray *
clay_image_procedure_run (ClayProcedure        *procedure,
                          const ClayValueArray *args)
{
  ClayPlugIn          *plug_in;
  ClayImageProcedure  *image_proc = CLAY_IMAGE_PROCEDURE (procedure);
  ClayPDBStatusType    status     = CLAY_PDB_EXECUTION_ERROR;
  ClayProcedureConfig *config;
  ClayValueArray      *remaining;
  ClayValueArray      *return_values;
  ClayRunMode          run_mode;
  ClayImage           *image;
  ClayDrawable       **drawables;
  gint                 i;

  run_mode    = CLAY_VALUES_GET_ENUM              (args, 0);
  image       = CLAY_VALUES_GET_IMAGE             (args, 1);
  drawables   = CLAY_VALUES_GET_CORE_OBJECT_ARRAY (args, 2);

  remaining   = clay_value_array_new (clay_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < clay_value_array_length (args); i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_array_append (remaining, value);
    }

  config = _clay_procedure_create_run_config (procedure);
  _clay_procedure_config_begin_run (config, image, run_mode, remaining, NULL);

  return_values = image_proc->run_func (procedure, run_mode, image, drawables,
                                        config, image_proc->run_data);

  if (return_values != NULL                       &&
      clay_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (return_values, 0)))
    status = CLAY_VALUES_GET_ENUM (return_values, 0);

  _clay_procedure_config_end_run (config, status);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = clay_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _clay_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the ClayProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
  clay_value_array_unref (remaining);

  return return_values;
}

static ClayProcedureConfig *
clay_image_procedure_create_config (ClayProcedure  *procedure,
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

static gboolean
clay_image_procedure_set_sensitivity (ClayProcedure *procedure,
                                      gint           sensitivity_mask)
{
  GParamSpec *pspec;

  g_return_val_if_fail (CLAY_IS_IMAGE_PROCEDURE (procedure), FALSE);

  pspec = clay_procedure_find_argument (procedure, "image");
  /* The pspec may not exist yet while we are constructing the
   * procedure.
   */
  if (pspec)
    {
      if (sensitivity_mask & CLAY_PROCEDURE_SENSITIVE_NO_IMAGE)
        pspec->flags |= CLAY_PARAM_NO_VALIDATE;
      else
        pspec->flags &= ~CLAY_PARAM_NO_VALIDATE;
    }

  return TRUE;
}

/*  public functions  */

/**
 * clay_image_procedure_new:
 * @plug_in:          a #ClayPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #ClayPDBProcType.
 * @run_func:         (closure run_data): the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (destroy run_data) (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new image procedure named @name which will call @run_func
 * when invoked.
 *
 * See [ctor@Procedure.new] for information about @proc_type.
 *
 * Returns: (transfer full): a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_image_procedure_new (ClayPlugIn       *plug_in,
                          const gchar      *name,
                          ClayPDBProcType   proc_type,
                          ClayRunImageFunc  run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  ClayImageProcedure *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_IMAGE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  return CLAY_PROCEDURE (procedure);
}
