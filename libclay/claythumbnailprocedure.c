/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claythumbnailprocedure.c
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

#include "clayplugin-private.h"
#include "claythumbnailprocedure.h"
#include "clayprocedureconfig-private.h"


struct _ClayThumbnailProcedure
{
  ClayProcedure         parent_instance;

  ClayRunThumbnailFunc  run_func;
  gpointer              run_data;
  GDestroyNotify        run_data_destroy;
};


static void   clay_thumbnail_procedure_constructed (GObject              *object);
static void   clay_thumbnail_procedure_finalize    (GObject              *object);

static ClayValueArray *
              clay_thumbnail_procedure_run         (ClayProcedure        *procedure,
                                                    const ClayValueArray *args);
static ClayProcedureConfig *
              clay_thumbnail_procedure_create_config
                                                   (ClayProcedure        *procedure,
                                                    GParamSpec          **args,
                                                    gint                  n_args);


G_DEFINE_TYPE (ClayThumbnailProcedure, clay_thumbnail_procedure, CLAY_TYPE_PROCEDURE)

#define parent_class clay_thumbnail_procedure_parent_class


static void
clay_thumbnail_procedure_class_init (ClayThumbnailProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed      = clay_thumbnail_procedure_constructed;
  object_class->finalize         = clay_thumbnail_procedure_finalize;

  procedure_class->run           = clay_thumbnail_procedure_run;
  procedure_class->create_config = clay_thumbnail_procedure_create_config;
}

static void
clay_thumbnail_procedure_init (ClayThumbnailProcedure *procedure)
{
}

static void
clay_thumbnail_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_file_argument (procedure, "file",
                                    "File",
                                    "The file to load the thumbnail from",
                                    CLAY_FILE_CHOOSER_ACTION_OPEN,
                                    FALSE, NULL,
                                    CLAY_PARAM_READWRITE);

  clay_procedure_add_int_argument (procedure, "thumb-size",
                                   "Thumb Size",
                                   "Preferred thumbnail size",
                                   16, 2014, 256,
                                   CLAY_PARAM_READWRITE);

  clay_procedure_add_image_return_value (procedure, "image",
                                         "Image",
                                         "Thumbnail image",
                                         TRUE,
                                         CLAY_PARAM_READWRITE);

  clay_procedure_add_int_return_value (procedure, "image-width",
                                       "Image width",
                                       "Width of the full-sized image (0 for unknown)",
                                       0, CLAY_MAX_IMAGE_SIZE, 0,
                                       CLAY_PARAM_READWRITE);

  clay_procedure_add_int_return_value (procedure, "image-height",
                                       "Image height",
                                       "Height of the full-sized image (0 for unknown)",
                                       0, CLAY_MAX_IMAGE_SIZE, 0,
                                       CLAY_PARAM_READWRITE);

  clay_procedure_add_enum_return_value (procedure, "image-type",
                                        "Image type",
                                        "Type of the image",
                                        CLAY_TYPE_IMAGE_TYPE,
                                        CLAY_RGB_IMAGE,
                                        CLAY_PARAM_READWRITE);

  clay_procedure_add_int_return_value (procedure, "num-layers",
                                       "Num layers",
                                       "Number of layers in the image",
                                       1, G_MAXINT, 1,
                                       CLAY_PARAM_READWRITE);
}

static void
clay_thumbnail_procedure_finalize (GObject *object)
{
  ClayThumbnailProcedure *procedure = CLAY_THUMBNAIL_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 2

static ClayValueArray *
clay_thumbnail_procedure_run (ClayProcedure        *procedure,
                              const ClayValueArray *args)
{
  ClayPlugIn             *plug_in;
  ClayThumbnailProcedure *thumbnail_proc = CLAY_THUMBNAIL_PROCEDURE (procedure);
  ClayValueArray         *remaining;
  ClayValueArray         *return_values;
  ClayProcedureConfig    *config;
  GFile                  *file;
  gint                    size;
  ClayPDBStatusType       status = CLAY_PDB_EXECUTION_ERROR;
  gint                    i;

  file = CLAY_VALUES_GET_FILE (args, 0);
  size = CLAY_VALUES_GET_INT  (args, 1);

  remaining = clay_value_array_new (clay_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < clay_value_array_length (args); i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_array_append (remaining, value);
    }

  config = _clay_procedure_create_run_config (procedure);
  _clay_procedure_config_begin_run (config, NULL, CLAY_RUN_NONINTERACTIVE, remaining, NULL);
  clay_value_array_unref (remaining);

  return_values = thumbnail_proc->run_func (procedure, file, size,
                                            config, thumbnail_proc->run_data);

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
    g_printerr ("%s: ERROR: the ClayThumbnailProcedure config object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);

  return return_values;
}

static ClayProcedureConfig *
clay_thumbnail_procedure_create_config (ClayProcedure  *procedure,
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
 * clay_thumbnail_procedure_new:
 * @plug_in:          a #ClayPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #ClayPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new thumbnail procedure named @name which will call @run_func
 * when invoked.
 *
 * See clay_procedure_new() for information about @proc_type.
 *
 * #ClayThumbnailProcedure is a #ClayProcedure subclass that makes it easier
 * to write file thumbnail procedures.
 *
 * It automatically adds the standard
 *
 * (#GFile, size)
 *
 * arguments and the standard
 *
 * (#ClayImage, image-width, image-height, #ClayImageType, num-layers)
 *
 * return value of a thumbnail procedure. It is possible to add
 * additional arguments.
 *
 * When invoked via clay_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #ClayRunThumbnailFunc. The
 * "args" #ClayValueArray of #ClayRunThumbnailFunc only contains
 * additionally added arguments.
 *
 * #ClayRunThumbnailFunc must clay_value_array_truncate() the returned
 * #ClayValueArray to the number of return values it actually uses.
 *
 * Returns: a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure  *
clay_thumbnail_procedure_new (ClayPlugIn           *plug_in,
                              const gchar          *name,
                              ClayPDBProcType       proc_type,
                              ClayRunThumbnailFunc  run_func,
                              gpointer              run_data,
                              GDestroyNotify        run_data_destroy)
{
  ClayThumbnailProcedure *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_THUMBNAIL_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  return CLAY_PROCEDURE (procedure);
}
