/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayloadprocedure.c
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

#include "clayloadprocedure.h"
#include "claypdb_pdb.h"
#include "clayplugin-private.h"
#include "clayprocedureconfig-private.h"

#include "libclay-intl.h"


/**
 * ClayLoadProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write file load
 * procedures.
 *
 * It automatically adds the standard
 *
 * ( [enum@RunMode], [iface@Gio.File] )
 *
 * arguments and the standard
 *
 * ( [class@Image] )
 *
 * return value of a load procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * "args" [struct@ValueArray] of [callback@RunImageFunc] only contains
 * additionally added arguments.
 */


typedef struct _ClayLoadProcedurePrivate
{
  ClayRunLoadFunc  run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;

  gboolean         handles_raw;
  gchar           *thumbnail_proc;
} ClayLoadProcedurePrivate;


static void   clay_load_procedure_constructed   (GObject              *object);
static void   clay_load_procedure_finalize      (GObject              *object);

static void   clay_load_procedure_install       (ClayProcedure        *procedure);
static ClayValueArray *
              clay_load_procedure_run           (ClayProcedure        *procedure,
                                                 const ClayValueArray *args);
static ClayProcedureConfig *
              clay_load_procedure_create_config (ClayProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (ClayLoadProcedure, clay_load_procedure, CLAY_TYPE_FILE_PROCEDURE)

#define parent_class clay_load_procedure_parent_class


static void
clay_load_procedure_class_init (ClayLoadProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed      = clay_load_procedure_constructed;
  object_class->finalize         = clay_load_procedure_finalize;

  procedure_class->install       = clay_load_procedure_install;
  procedure_class->run           = clay_load_procedure_run;
  procedure_class->create_config = clay_load_procedure_create_config;
}

static void
clay_load_procedure_init (ClayLoadProcedure *procedure)
{
}

static void
clay_load_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_file_argument (procedure, "file",
                                    "File",
                                    "The file to load",
                                    CLAY_FILE_CHOOSER_ACTION_OPEN,
                                    FALSE, NULL,
                                    CLAY_PARAM_READWRITE);

  clay_procedure_add_image_return_value (procedure, "image",
                                         "Image",
                                         "Output image",
                                         TRUE,
                                         CLAY_PARAM_READWRITE);
}

static void
clay_load_procedure_finalize (GObject *object)
{
  ClayLoadProcedure        *procedure = CLAY_LOAD_PROCEDURE (object);
  ClayLoadProcedurePrivate *priv;

  priv = clay_load_procedure_get_instance_private (procedure);

  if (priv->run_data_destroy)
    priv->run_data_destroy (priv->run_data);

  g_clear_pointer (&priv->thumbnail_proc, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_load_procedure_install (ClayProcedure *procedure)
{
  ClayLoadProcedurePrivate *priv;
  ClayLoadProcedure        *load_proc       = CLAY_LOAD_PROCEDURE (procedure);
  ClayFileProcedure        *file_proc       = CLAY_FILE_PROCEDURE (procedure);
  const gchar              *meta_extensions = NULL;
  const gchar              *mime_types;
  gint                      priority;

  priv = clay_load_procedure_get_instance_private (load_proc);

  CLAY_PROCEDURE_CLASS (parent_class)->install (procedure);

  (void) clay_file_procedure_get_meta (file_proc, &meta_extensions),
  _clay_pdb_set_file_proc_load_handler (clay_procedure_get_name (procedure),
                                        clay_file_procedure_get_extensions (file_proc),
                                        meta_extensions,
                                        clay_file_procedure_get_prefixes (file_proc),
                                        clay_file_procedure_get_magics (file_proc));

  if (clay_file_procedure_get_handles_remote (file_proc))
    _clay_pdb_set_file_proc_handles_remote (clay_procedure_get_name (procedure));

  mime_types = clay_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    _clay_pdb_set_file_proc_mime_types (clay_procedure_get_name (procedure),
                                        mime_types);

  priority = clay_file_procedure_get_priority (file_proc);
  if (priority != 0)
    _clay_pdb_set_file_proc_priority (clay_procedure_get_name (procedure),
                                      priority);

  if (priv->handles_raw)
    _clay_pdb_set_file_proc_handles_raw (clay_procedure_get_name (procedure));

  if (priv->thumbnail_proc)
    _clay_pdb_set_file_proc_thumbnail_loader (clay_procedure_get_name (procedure),
                                              priv->thumbnail_proc);
}

#define ARG_OFFSET 2

static ClayValueArray *
clay_load_procedure_run (ClayProcedure        *procedure,
                         const ClayValueArray *args)
{
  ClayLoadProcedurePrivate *priv;
  ClayPlugIn               *plug_in;
  ClayLoadProcedure        *load_proc = CLAY_LOAD_PROCEDURE (procedure);
  ClayValueArray           *remaining;
  ClayValueArray           *return_values;
  ClayProcedureConfig      *config;
  ClayImage                *image    = NULL;
  ClayMetadata             *metadata = NULL;
  gchar                    *mimetype = NULL;
  ClayMetadataLoadFlags     flags    = CLAY_METADATA_LOAD_ALL;
  ClayPDBStatusType         status   = CLAY_PDB_EXECUTION_ERROR;
  ClayRunMode               run_mode;
  GFile                    *file;
  gint                      i;

  priv = clay_load_procedure_get_instance_private (load_proc);

  run_mode = CLAY_VALUES_GET_ENUM (args, 0);
  file     = CLAY_VALUES_GET_FILE (args, 1);

  remaining = clay_value_array_new (clay_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < clay_value_array_length (args); i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_array_append (remaining, value);
    }

  config   = _clay_procedure_create_run_config (procedure);
  mimetype = (gchar *) clay_file_procedure_get_mime_types (CLAY_FILE_PROCEDURE (procedure));

  if (mimetype != NULL)
    {
      char *delim;

      mimetype = g_strdup (mimetype);
      mimetype = g_strstrip (mimetype);
      delim = strstr (mimetype, ",");
      if (delim)
        *delim = '\0';
      /* Though docs only writes about the list being comma-separated, our
       * code apparently also split by spaces.
       */
      delim = strstr (mimetype, " ");
      if (delim)
        *delim = '\0';
      delim = strstr (mimetype, "\t");
      if (delim)
        *delim = '\0';

      metadata = clay_metadata_load_from_file (file, NULL);
      g_free (mimetype);
    }
  else
    {
      flags = CLAY_METADATA_LOAD_NONE;
    }

  if (metadata == NULL)
    metadata = clay_metadata_new ();

  _clay_procedure_config_begin_run (config, image, run_mode, remaining, NULL);

  return_values = priv->run_func (procedure, run_mode,
                                  file, metadata, &flags,
                                  config, priv->run_data);

  if (return_values != NULL                       &&
      clay_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (return_values, 0)))
    status = CLAY_VALUES_GET_ENUM (return_values, 0);

  _clay_procedure_config_end_run (config, status);

  if (status == CLAY_PDB_SUCCESS)
    {
      if (clay_value_array_length (return_values) < 2 ||
          ! CLAY_VALUE_HOLDS_IMAGE (clay_value_array_index (return_values, 1)))
        {
          GError *error = NULL;

          status = CLAY_PDB_EXECUTION_ERROR;
          g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                       _("This file loading plug-in returned SUCCESS as a status without an image. "
                         "This is a bug in the plug-in code. Contact the plug-in developer."));
          clay_value_array_unref (return_values);
          return_values = clay_procedure_new_return_values (procedure, status, error);
        }
      else
        {
          image = CLAY_VALUES_GET_IMAGE (return_values, 1);
        }
    }

  if (image != NULL && metadata != NULL && flags != CLAY_METADATA_LOAD_NONE)
    _clay_image_metadata_load_finish (image, NULL, metadata, flags);

  if (image)
    {
      clay_image_clean_all (image);
      clay_image_undo_disable (image);
      clay_image_undo_enable (image);
    }

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = clay_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _clay_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the ClayExportProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
  g_clear_object (&metadata);
  clay_value_array_unref (remaining);

  return return_values;
}

static ClayProcedureConfig *
clay_load_procedure_create_config (ClayProcedure  *procedure,
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
 * clay_load_procedure_new:
 * @plug_in:          a #ClayPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #ClayPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new load procedure named @name which will call @run_func
 * when invoked.
 *
 * See clay_procedure_new() for information about @proc_type.
 *
 * Returns: (transfer full): a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure  *
clay_load_procedure_new (ClayPlugIn      *plug_in,
                         const gchar     *name,
                         ClayPDBProcType  proc_type,
                         ClayRunLoadFunc  run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  ClayLoadProcedure        *procedure;
  ClayLoadProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_LOAD_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  priv = clay_load_procedure_get_instance_private (procedure);

  priv->run_func         = run_func;
  priv->run_data         = run_data;
  priv->run_data_destroy = run_data_destroy;

  return CLAY_PROCEDURE (procedure);
}

/**
 * clay_load_procedure_set_handles_raw:
 * @procedure:   A load procedure object.
 * @handles_raw: The procedure's handles raw flag.
 *
 * Registers a load procedure as capable of handling raw digital camera loads.
 *
 * Note that you cannot call this function on [class@VectorLoadProcedure]
 * subclass objects.
 *
 * Since: 3.0
 **/
void
clay_load_procedure_set_handles_raw (ClayLoadProcedure *procedure,
                                     gint               handles_raw)
{
  ClayLoadProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_LOAD_PROCEDURE (procedure));

  priv = clay_load_procedure_get_instance_private (procedure);

  priv->handles_raw = handles_raw;
}

/**
 * clay_load_procedure_get_handles_raw:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's 'handles raw' flag as set with
 * [method@ClayLoadProcedure.set_handles_raw].
 *
 * Returns: The procedure's 'handles raw' flag.
 *
 * Since: 3.0
 **/
gint
clay_load_procedure_get_handles_raw (ClayLoadProcedure *procedure)
{
  ClayLoadProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_LOAD_PROCEDURE (procedure), 0);

  priv = clay_load_procedure_get_instance_private (procedure);

  return priv->handles_raw;
}

/**
 * clay_load_procedure_set_thumbnail_loader:
 * @procedure:      A load procedure object.
 * @thumbnail_proc: The name of the thumbnail load procedure.
 *
 * Associates a thumbnail loader with a file load procedure.
 *
 * Some file formats allow for embedded thumbnails, other file formats
 * contain a scalable image or provide the image data in different
 * resolutions. A file plug-in for such a format may register a
 * special procedure that allows Clay to load a thumbnail preview of
 * the image. This procedure is then associated with the standard
 * load procedure using this function.
 *
 * Since: 3.0
 **/
void
clay_load_procedure_set_thumbnail_loader (ClayLoadProcedure *procedure,
                                          const gchar       *thumbnail_proc)
{
  ClayLoadProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_LOAD_PROCEDURE (procedure));

  priv = clay_load_procedure_get_instance_private (procedure);
  g_free (priv->thumbnail_proc);
  priv->thumbnail_proc = g_strdup (thumbnail_proc);
}

/**
 * clay_load_procedure_get_thumbnail_loader:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's thumbnail loader procedure as set with
 * [method@ClayLoadProcedure.set_thumbnail_loader].
 *
 * Returns: The procedure's thumbnail loader procedure
 *
 * Since: 3.0
 **/
const gchar *
clay_load_procedure_get_thumbnail_loader (ClayLoadProcedure *procedure)
{
  ClayLoadProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_LOAD_PROCEDURE (procedure), NULL);

  priv = clay_load_procedure_get_instance_private (procedure);

  return priv->thumbnail_proc;
}
