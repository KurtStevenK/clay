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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core/core-types.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/claydrawable.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "plug-in/claypluginmanager.h"
#include "plug-in/claypluginprocedure.h"

#include "xcf.h"
#include "xcf-private.h"
#include "xcf-load.h"
#include "xcf-read.h"
#include "xcf-save.h"

#include "clay-intl.h"


typedef ClayImage * ClayXcfLoaderFunc (Clay     *clay,
                                       XcfInfo  *info,
                                       GError  **error);


static ClayValueArray * xcf_load_invoker (ClayProcedure         *procedure,
                                          Clay                  *clay,
                                          ClayContext           *context,
                                          ClayProgress          *progress,
                                          const ClayValueArray  *args,
                                          GError               **error);
static ClayValueArray * xcf_save_invoker (ClayProcedure         *procedure,
                                          Clay                  *clay,
                                          ClayContext           *context,
                                          ClayProgress          *progress,
                                          const ClayValueArray  *args,
                                          GError               **error);


static ClayXcfLoaderFunc * const xcf_loaders[] =
{
  xcf_load_image,   /* version  0 */
  xcf_load_image,   /* version  1 */
  xcf_load_image,   /* version  2 */
  xcf_load_image,   /* version  3 */
  xcf_load_image,   /* version  4 */
  xcf_load_image,   /* version  5 */
  xcf_load_image,   /* version  6 */
  xcf_load_image,   /* version  7 */
  xcf_load_image,   /* version  8 */
  xcf_load_image,   /* version  9 */
  xcf_load_image,   /* version 10 */
  xcf_load_image,   /* version 11 */
  xcf_load_image,   /* version 12 */
  xcf_load_image,   /* version 13 */
  xcf_load_image,   /* version 14 */
  xcf_load_image,   /* version 15 */
  xcf_load_image,   /* version 16 */
  xcf_load_image,   /* version 17 */
  xcf_load_image,   /* version 18 */
  xcf_load_image,   /* version 19 */
  xcf_load_image,   /* version 20 */
  xcf_load_image,   /* version 21 */
  xcf_load_image,   /* version 22 */
  xcf_load_image,   /* version 23 */
  xcf_load_image,   /* version 24 */
  xcf_load_image,   /* version 25 */
  xcf_load_image,   /* version 26 */
  xcf_load_image,   /* version 27 */
};


void
xcf_init (Clay *clay)
{
  ClayPlugInProcedure *proc;
  GFile               *file;
  ClayProcedure       *procedure;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /* So this is sort of a hack, but its better than it was before.  To
   * do this right there would be a file load-save handler type and
   * the whole interface would change but there isn't, and currently
   * the plug-in structure contains all the load-save info, so it
   * makes sense to use that for the XCF load/save handlers, even
   * though they are internal.  The only thing it requires is using a
   * PlugInProcDef struct.  -josh
   */

  /*  clay-xcf-save  */
  file = g_file_new_for_path ("clay-xcf-save");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = xcf_save_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay XCF image"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-wilber",
                                   strlen ("clay-wilber") + 1,
                                   NULL);
  clay_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  clay_plug_in_procedure_set_file_proc (proc, "xcf", NULL, "", NULL);
  clay_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "clay-xcf-save");
  clay_procedure_set_static_help (procedure,
                                  "Saves file in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in Clay. "
                                  "This procedure will save the specified "
                                  "image in the xcf file format.",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file "
                                                    "to save the image in",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));
  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  clay-xcf-load  */
  file = g_file_new_for_path ("clay-xcf-load");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = xcf_load_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay XCF image"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-wilber",
                                   strlen ("clay-wilber") + 1,
                                   NULL);
  clay_plug_in_procedure_set_image_types (proc, NULL);
  clay_plug_in_procedure_set_file_proc (proc, "xcf", NULL, "",
                                        "0,string,clay\\040xcf\\040");
  clay_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "clay-xcf-load");
  clay_procedure_set_static_help (procedure,
                                  "Loads file saved in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in Clay. "
                                  "This procedure will load the specified "
                                  "file.",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                  CLAY_PARAM_READWRITE));

  clay_procedure_add_return_value (procedure,
                                   clay_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          CLAY_PARAM_READWRITE));
  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
xcf_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
}

ClayImage *
xcf_load_stream (Clay          *clay,
                 GInputStream  *input,
                 GFile         *input_file,
                 ClayProgress  *progress,
                 GError       **error)
{
  XcfInfo      info  = { 0, };
  const gchar *filename;
  ClayImage   *image = NULL;
  gboolean     success;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (input_file == NULL || G_IS_FILE (input_file), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (input_file)
    filename = clay_file_get_utf8_name (input_file);
  else
    filename = _("Memory Stream");

  if (progress)
    clay_progress_start (progress, FALSE, _("Opening '%s'"), filename);

  success = xcf_load_magic_version (clay, input, input_file, progress, &info);

  if (success)
    {
      if (info.file_version >= 0 &&
          info.file_version < G_N_ELEMENTS (xcf_loaders))
        {
          image = (*(xcf_loaders[info.file_version])) (clay, &info, error);

          if (! image)
            success = FALSE;

          g_input_stream_close (info.input, NULL, NULL);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("XCF error: unsupported XCF file version %d "
                         "encountered"), info.file_version);
          success = FALSE;
        }
    }

  if (progress)
    clay_progress_end (progress);

  return image;
}

gboolean
xcf_save_stream (Clay           *clay,
                 ClayImage      *image,
                 GOutputStream  *output,
                 GFile          *output_file,
                 ClayProgress   *progress,
                 GError        **error)
{
  XcfInfo       info     = { 0, };
  const gchar  *filename;
  gboolean      success  = FALSE;
  GError       *my_error = NULL;
  GCancellable *cancellable;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (output_file == NULL || G_IS_FILE (output_file), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (output_file)
    filename = clay_file_get_utf8_name (output_file);
  else
    filename = _("Memory Stream");

  info.clay             = clay;
  info.output           = output;
  info.seekable         = G_SEEKABLE (output);
  info.bytes_per_offset = 4;
  info.progress         = progress;
  info.file             = output_file;

  if (clay_image_get_xcf_compression (image))
    info.compression = COMPRESS_ZLIB;
  else
    info.compression = COMPRESS_RLE;

  info.file_version = clay_image_get_xcf_version (image,
                                                  info.compression ==
                                                  COMPRESS_ZLIB,
                                                  NULL, NULL, NULL);

  if (info.file_version >= 11)
    info.bytes_per_offset = 8;

  if (progress)
    clay_progress_start (progress, FALSE, _("Saving '%s'"), filename);

  success = xcf_save_image (&info, image, &my_error);

  cancellable = g_cancellable_new ();
  if (success)
    {
      if (progress)
        clay_progress_set_text (progress, _("Closing '%s'"), filename);
    }
  else
    {
      /* When closing the stream, the image will be actually saved,
       * unless we properly cancel it with a GCancellable.
       * Not closing the stream is not an option either, as this will
       * happen anyway when finalizing the output.
       * So let's make sure now that we don't overwrite the XCF file
       * when an error occurred.
       */
      g_cancellable_cancel (cancellable);
    }
  success = g_output_stream_close (info.output, cancellable, &my_error);
  g_object_unref (cancellable);

  if (! success && my_error)
    g_propagate_prefixed_error (error, my_error,
                                _("Error writing '%s': "), filename);

  if (progress)
    clay_progress_end (progress);

  return success;
}


/*  private functions  */

static ClayValueArray *
xcf_load_invoker (ClayProcedure         *procedure,
                  Clay                  *clay,
                  ClayContext           *context,
                  ClayProgress          *progress,
                  const ClayValueArray  *args,
                  GError               **error)
{
  ClayValueArray *return_vals;
  ClayImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  clay_set_busy (clay);

  file = g_value_get_object (clay_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      image = xcf_load_stream (clay, input, file, progress, error);

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  clay_file_get_utf8_name (file));
    }

  return_vals = clay_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (clay_value_array_index (return_vals, 1), image);

  clay_unset_busy (clay);

  return return_vals;
}

static ClayValueArray *
xcf_save_invoker (ClayProcedure         *procedure,
                  Clay                  *clay,
                  ClayContext           *context,
                  ClayProgress          *progress,
                  const ClayValueArray  *args,
                  GError               **error)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GFile          *file;
  GOutputStream  *output;
  gboolean        success  = FALSE;
  GError         *my_error = NULL;

  clay_set_busy (clay);

  image = g_value_get_object (clay_value_array_index (args, 1));
  file  = g_value_get_object (clay_value_array_index (args, 2));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE,
                                            G_FILE_CREATE_REPLACE_DESTINATION,
                                            NULL, &my_error));

  /* Windows GIO: g_file_replace() can report NOT_FOUND for a new file
   * in an existing folder. Create it instead of failing the save.
   */
  if (! output &&
      my_error &&
      g_error_matches (my_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
    {
      g_clear_error (&my_error);
      output = G_OUTPUT_STREAM (g_file_create (file,
                                               G_FILE_CREATE_NONE,
                                               NULL, &my_error));
    }

  if (output)
    {
      success = xcf_save_stream (clay, image, output, file, progress, error);

      g_object_unref (output);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Error creating '%s': "),
                                  clay_file_get_utf8_name (file));
    }

  return_vals = clay_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  clay_unset_busy (clay);

  return return_vals;
}
