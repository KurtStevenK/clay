/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfileprocedure.c
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
#include "clayfileprocedure.h"


/**
 * ClayFileProcedure:
 *
 * File procedures implement file support. They cannot be created directly.
 * Instead, you will create an instance of one of the subclasses (such as export
 * or load procedures). This provides a common interface for file-related
 * functions on these objects.
 **/

typedef struct _ClayFileProcedurePrivate
{
  gchar    *format_name;
  gchar    *mime_types;
  gchar    *extensions;
  gboolean  is_meta;
  gchar    *meta_extensions;
  gchar    *prefixes;
  gchar    *magics;
  gint      priority;
  gboolean  handles_remote;
} ClayFileProcedurePrivate;


static void   clay_file_procedure_constructed (GObject *object);
static void   clay_file_procedure_finalize    (GObject *object);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayFileProcedure, clay_file_procedure,
                                     CLAY_TYPE_PROCEDURE)

#define parent_class clay_file_procedure_parent_class


static void
clay_file_procedure_class_init (ClayFileProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_file_procedure_constructed;
  object_class->finalize     = clay_file_procedure_finalize;
}

static void
clay_file_procedure_init (ClayFileProcedure *procedure)
{
}

static void
clay_file_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    CLAY_TYPE_RUN_MODE,
                                    CLAY_RUN_NONINTERACTIVE,
                                    G_PARAM_READWRITE);
}

static void
clay_file_procedure_finalize (GObject *object)
{
  ClayFileProcedure        *procedure;
  ClayFileProcedurePrivate *priv;

  procedure = CLAY_FILE_PROCEDURE (object);
  priv      = clay_file_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->format_name,     g_free);
  g_clear_pointer (&priv->mime_types,      g_free);
  g_clear_pointer (&priv->extensions,      g_free);
  g_clear_pointer (&priv->meta_extensions, g_free);
  g_clear_pointer (&priv->prefixes,        g_free);
  g_clear_pointer (&priv->magics,          g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

/**
 * clay_file_procedure_set_format_name:
 * @procedure: A file procedure.
 * @format_name: A public-facing name for the format, e.g. "PNG".
 *
 * Associates a format name with a file handler procedure.
 *
 * This name can be used for any public-facing strings, such as
 * graphical interface labels. An example usage would be
 * %ClayExportProcedureDialog title looking like "Export Image as %s".
 *
 * Note that since the format name is public-facing, it is recommended
 * to localize it at runtime, for instance through gettext, like:
 *
 * ```c
 * clay_file_procedure_set_format_name (procedure, _("JPEG"));
 * ```
 *
 * Some language would indeed localize even some technical terms or
 * acronyms, even if sometimes just to rewrite them with the local
 * writing system.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_format_name (ClayFileProcedure *procedure,
                                     const gchar       *format_name)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_free (priv->format_name);
  priv->format_name = g_strdup (format_name);
}

/**
 * clay_file_procedure_get_format_name:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's format name, as set with
 * [method@FileProcedure.set_format_name].
 *
 * Returns: The procedure's format name.
 *
 * Since: 3.0
 **/
const gchar *
clay_file_procedure_get_format_name (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), NULL);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->format_name;
}

/**
 * clay_file_procedure_set_mime_types:
 * @procedure: A file procedure object.
 * @mime_types: A comma-separated list of MIME types, such as "image/jpeg".
 *
 * Associates MIME types with a file handler procedure.
 *
 * Registers MIME types for a file handler procedure. This allows Clay
 * to determine the MIME type of the file opened or saved using this
 * procedure. It is recommended that only one MIME type is registered
 * per file procedure; when registering more than one MIME type, Clay
 * will associate the first one with files opened or saved with this
 * procedure.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_mime_types (ClayFileProcedure *procedure,
                                    const gchar       *mime_types)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_free (priv->mime_types);
  priv->mime_types = g_strdup (mime_types);
}

/**
 * clay_file_procedure_get_mime_types:
 * @procedure: A file procedure.
 *
 * Returns the procedure's mime-type as set with
 * [method@FileProcedure.set_mime_types].
 *
 * Returns: The procedure's registered mime-types.
 *
 * Since: 3.0
 **/
const gchar *
clay_file_procedure_get_mime_types (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), NULL);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->mime_types;
}

/**
 * clay_file_procedure_set_extensions:
 * @procedure:  A file procedure.
 * @extensions: A comma separated list of extensions this procedure can
 *              handle (i.e. "jpg,jpeg").
 *
 * Registers the given list of extensions as something this procedure can
 * handle.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_extensions (ClayFileProcedure *procedure,
                                    const gchar       *extensions)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_free (priv->extensions);
  priv->extensions = g_strdup (extensions);
}

/**
 * clay_file_procedure_get_extensions:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's extensions as set with
 * [method@FileProcedure.set_extensions].
 *
 * Returns: The procedure's registered extensions.
 *
 * Since: 3.0
 **/
const gchar *
clay_file_procedure_get_extensions (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), NULL);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->extensions;
}

/**
 * clay_file_procedure_set_meta:
 * @procedure: A file procedure.
 * @is_meta: Whether @procedure is a meta file procedure.
 * @meta_extensions: A comma separated list of generic extensions this procedure can
 *                   handle (i.e. "gz").
 *
 * Some [class@Clay.LoadProcedure] or [class@Clay.ExportProcedure] may
 * actually be a *meta* procedure. Meta procedures are typically
 * supporting container formats which will call another procedure for
 * the actual format within the container.
 *
 * Note that you should still call [method@FileProcedure.set_extensions],
 * usually with a list of standard full formats, such as "xcf.gz,xcfgz",
 * or again "hgt.zip". Nevertheless the @meta_extensions should be only
 * "gz" or "zip" respectively. This would allow this meta format to work
 * even on less common combinations. For instance, I could export an
 * image as "png.gz" even though it is not explicitly listed.
 *
 * Since: 3.2
 **/
void
clay_file_procedure_set_meta (ClayFileProcedure *procedure,
                              gboolean           is_meta,
                              const gchar       *meta_extensions)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));
  g_return_if_fail ((! is_meta && meta_extensions == NULL) ||
                    (is_meta && meta_extensions != NULL && strlen (meta_extensions) > 0));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_clear_pointer (&priv->meta_extensions, g_free);

  priv->is_meta = is_meta;
  if (is_meta)
    priv->meta_extensions = g_strdup (meta_extensions);
}

/**
 * clay_file_procedure_get_meta:
 * @procedure: A file procedure object.
 * @meta_extensions: (out) (nullable): The generic meta extensions,
 *                                     optionally.
 *
 * Returns whether @procedure is a meta file procedure as set with
 * [method@FileProcedure.set_meta]. If @meta_extensions is not %NULL,
 * the generic extensions will be returned too.
 *
 * Returns: %TRUE is @procedure is a meta file procedure.
 *
 * Since: 3.2
 **/
gboolean
clay_file_procedure_get_meta (ClayFileProcedure  *procedure,
                              const gchar       **meta_extensions)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (meta_extensions == NULL || *meta_extensions == NULL, FALSE);

  priv = clay_file_procedure_get_instance_private (procedure);

  if (meta_extensions)
    *meta_extensions = priv->meta_extensions;

  return priv->is_meta;
}

/**
 * clay_file_procedure_set_prefixes:
 * @procedure: A file procedure object.
 * @prefixes:  A comma separated list of prefixes this procedure can
 *             handle (i.e. "http:,ftp:").
 *
 * It should almost never be necessary to register prefixes with file
 * procedures, because most sorts of URIs should be handled by GIO.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_prefixes (ClayFileProcedure *procedure,
                                  const gchar       *prefixes)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_free (priv->prefixes);
  priv->prefixes = g_strdup (prefixes);
}

/**
 * clay_file_procedure_get_prefixes:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's prefixes as set with
 * [method@FileProcedure.set_prefixes].
 *
 * Returns: The procedure's registered prefixes.
 *
 * Since: 3.0
 **/
const gchar *
clay_file_procedure_get_prefixes (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), NULL);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->prefixes;
}

/**
 * clay_file_procedure_set_magics:
 * @procedure: A file procedure object.
 * @magics:    A comma-separated list of magic file information (i.e. "0,string,GIF").
 *
 * Registers the list of magic file information this procedure can handle.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_magics (ClayFileProcedure *procedure,
                                const gchar       *magics)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  g_free (priv->magics);
  priv->magics = g_strdup (magics);
}

/**
 * clay_file_procedure_get_magics:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's magics as set with [method@FileProcedure.set_magics].
 *
 * Returns: The procedure's registered magics.
 *
 * Since: 3.0
 **/
const gchar *
clay_file_procedure_get_magics (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), NULL);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->magics;
}

/**
 * clay_file_procedure_set_priority:
 * @procedure: A file procedure object.
 * @priority: The procedure's priority.
 *
 * Sets the priority of a file handler procedure.
 *
 * When more than one procedure matches a given file, the procedure with the
 * lowest priority is used; if more than one procedure has the lowest priority,
 * it is unspecified which one of them is used. The default priority for file
 * handler procedures is 0.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_priority (ClayFileProcedure *procedure,
                                  gint               priority)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  priv->priority = priority;
}

/**
 * clay_file_procedure_get_priority:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's priority as set with
 * [method@FileProcedure.set_priority].
 *
 * Returns: The procedure's registered priority.
 *
 * Since: 3.0
 **/
gint
clay_file_procedure_get_priority (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), 0);

  priv = clay_file_procedure_get_instance_private (procedure);

  return priv->priority;
}

/**
 * clay_file_procedure_set_handles_remote:
 * @procedure:      A #ClayFileProcedure.
 * @handles_remote: The procedure's 'handles remote' flag.
 *
 * Registers a file procedure as capable of handling arbitrary remote
 * URIs via GIO.
 *
 * When @handles_remote is set to %TRUE, the procedure will get a
 * #GFile passed that can point to a remote file.
 *
 * When @handles_remote is set to %FALSE, the procedure will get a
 * local [iface@Gio.File] passed and can use [method@Gio.File.get_path] to get
 * to a filename that can be used with whatever non-GIO means of dealing with
 * the file.
 *
 * Since: 3.0
 **/
void
clay_file_procedure_set_handles_remote (ClayFileProcedure *procedure,
                                        gint               handles_remote)
{
  ClayFileProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_FILE_PROCEDURE (procedure));

  priv = clay_file_procedure_get_instance_private (procedure);
  priv->handles_remote = handles_remote;
}

/**
 * clay_file_procedure_get_handles_remote:
 * @procedure: A file procedure object.
 *
 * Returns the procedure's 'handles remote' flags as set with
 * [method@FileProcedure.set_handles_remote].
 *
 * Returns: The procedure's 'handles remote' flag
 *
 * Since: 3.0
 **/
gint
clay_file_procedure_get_handles_remote (ClayFileProcedure *procedure)
{
  ClayFileProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_FILE_PROCEDURE (procedure), 0);

  priv = clay_file_procedure_get_instance_private (procedure);
  return priv->handles_remote;
}
