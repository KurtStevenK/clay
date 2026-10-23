/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-rawtherapee.c -- raw file format plug-in that uses RawTherapee
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
 * Copyright (C) 2017 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <glib/gstdio.h>

#include <libclay/clay.h>

#include "libclay/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"


#define LOAD_THUMB_PROC   "file-rawtherapee-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rawtherapee"

/* macOS app-bundle identifier candidates */
static const gchar *RAWTHERAPEE_BUNDLE_IDS[] = { "com.rawtherapee.rawtherapee5", "com.rawtherapee.rawtherapee", NULL };


typedef struct _Rawtherapee      Rawtherapee;
typedef struct _RawtherapeeClass RawtherapeeClass;

struct _Rawtherapee
{
  ClayPlugIn      parent_instance;
};

struct _RawtherapeeClass
{
  ClayPlugInClass parent_class;
};


#define RAWTHERAPEE_TYPE  (rawtherapee_get_type ())
#define RAWTHERAPEE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RAWTHERAPEE_TYPE, Rawtherapee))

GType                   rawtherapee_get_type         (void);

static GList          * rawtherapee_init_procedures  (ClayPlugIn            *plug_in);
static ClayProcedure  * rawtherapee_create_procedure (ClayPlugIn            *plug_in,
                                                      const gchar           *name);

static ClayValueArray * rawtherapee_load             (ClayProcedure         *procedure,
                                                      ClayRunMode            run_mode,
                                                      GFile                 *file,
                                                      ClayMetadata          *metadata,
                                                      ClayMetadataLoadFlags *flags,
                                                      ClayProcedureConfig   *config,
                                                      gpointer               run_data);
static ClayValueArray * rawtherapee_load_thumb       (ClayProcedure         *procedure,
                                                      GFile                 *file,
                                                      gint                   size,
                                                      ClayProcedureConfig   *config,
                                                      gpointer               run_data);

static ClayImage      * load_image                   (GFile                *file,
                                                      ClayRunMode           run_mode,
                                                      GError              **error);
static ClayImage      * load_thumbnail_image         (GFile                *file,
                                                      gint                  thumb_size,
                                                      GError              **error);


G_DEFINE_TYPE (Rawtherapee, rawtherapee, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (RAWTHERAPEE_TYPE)
DEFINE_STD_SET_I18N


static void
rawtherapee_class_init (RawtherapeeClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = rawtherapee_init_procedures;
  plug_in_class->create_procedure = rawtherapee_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
rawtherapee_init (Rawtherapee *rawtherapee)
{
}

static GList *
rawtherapee_init_procedures (ClayPlugIn *plug_in)
{
  /* check if rawtherapee is installed
   * TODO: allow setting the location of the executable in preferences
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("rawtherapee", NULL,
                                                             "RAWTHERAPEE_EXECUTABLE",
                                                             RAWTHERAPEE_BUNDLE_IDS,
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
#ifdef G_OS_WIN32
  /* Issue #2716 - Prevent RT from opening a console window */
  gchar    *argv[]             = { exec_path, "-v", "-w", NULL };
#else
  gchar    *argv[]             = { exec_path, "-v", NULL };
#endif
  gchar    *rawtherapee_stdout = NULL;
  gboolean  have_rawtherapee   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    (search_path ? G_SPAWN_SEARCH_PATH : 0) |
                    G_SPAWN_STDERR_TO_DEV_NULL,
                    FILE_RAW_SANITIZE_CHILD_ENV,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    NULL))
    {
      gint rtmajor = 0;
      gint rtminor = 0;

#ifndef _UCRT
      if (sscanf (rawtherapee_stdout,
                  "RawTherapee, version %d.%d",
                  &rtmajor, &rtminor) == 2 &&
          ((rtmajor == 5 && rtminor >= 2) || rtmajor >= 6))
#else
      if (sscanf_s (rawtherapee_stdout,
                    "RawTherapee, version %d.%d",
                    &rtmajor, &rtminor) == 2 &&
          ((rtmajor == 5 && rtminor >= 2) || rtmajor >= 6))
#endif
        {
          have_rawtherapee = TRUE;
        }

      g_free (rawtherapee_stdout);
    }

  g_free (exec_path);

  if (have_rawtherapee)
    {
      GList *list = NULL;

      list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;

          load_proc = g_strdup_printf (format->load_proc_format, "rawtherapee");

          list = g_list_append (list, load_proc);
        }

      return list;
    }

  return NULL;
}

static ClayProcedure *
rawtherapee_create_procedure (ClayPlugIn  *plug_in,
                            const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                rawtherapee_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Load thumbnail from a raw image "
                                        "via rawtherapee",
                                        "This plug-in loads a thumbnail "
                                        "from a raw image by calling "
                                        "rawtherapee-cli.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Alberto Griggio",
                                      "Alberto Griggio",
                                      "2017");
    }
  else
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;
          gchar            *load_blurb;
          gchar            *load_help;

          load_proc = g_strdup_printf (format->load_proc_format, "rawtherapee");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format, "rawtherapee");
          load_help  = g_strdup_printf (format->load_help_format,  "rawtherapee");

          procedure = clay_load_procedure_new (plug_in, name,
                                               CLAY_PDB_PROC_TYPE_PLUGIN,
                                               rawtherapee_load,
                                               (gpointer) format, NULL);

          clay_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          clay_procedure_set_attribution (procedure,
                                          "Alberto Griggio",
                                          "Alberto Griggio",
                                          "2017");

          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              format->extensions);
          clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                          format->magic);

          clay_load_procedure_set_handles_raw (CLAY_LOAD_PROCEDURE (procedure),
                                               TRUE);
          clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static ClayValueArray *
rawtherapee_load (ClayProcedure         *procedure,
                  ClayRunMode            run_mode,
                  GFile                 *file,
                  ClayMetadata          *metadata,
                  ClayMetadataLoadFlags *flags,
                  ClayProcedureConfig   *config,
                  gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  image = load_image (file, run_mode, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
rawtherapee_load_thumb (ClayProcedure       *procedure,
                        GFile               *file,
                        gint                 size,
                        ClayProcedureConfig *config,
                        gpointer             run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  image = load_thumbnail_image (file, size, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);
  CLAY_VALUES_SET_INT   (return_vals, 2, 0);
  CLAY_VALUES_SET_INT   (return_vals, 3, 0);
  CLAY_VALUES_SET_ENUM  (return_vals, 4, CLAY_RGB_IMAGE);
  CLAY_VALUES_SET_INT   (return_vals, 5, 1);

  clay_value_array_truncate (return_vals, 6);

  return return_vals;
}

static ClayImage *
load_image (GFile        *file,
            ClayRunMode   run_mode,
            GError      **error)
{
  ClayImage *image              = NULL;
  GFile     *file_out           = clay_temp_file ("tif");
  gchar     *rawtherapee_stdout = NULL;

  gboolean   search_path        = FALSE;
  gchar     *exec_path          = file_raw_get_executable_path ("rawtherapee", NULL,
                                                               "RAWTHERAPEE_EXECUTABLE",
                                                               RAWTHERAPEE_BUNDLE_IDS,
                                                               REGISTRY_KEY_BASE,
                                                               &search_path);

  /* linear sRGB for now as Clay uses that internally in many places anyway */
  gchar *argv[] =
    {
      exec_path,
      "-clay",
      (gchar *) g_file_peek_path (file),
      (gchar *) g_file_peek_path (file_out),
      NULL
    };

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    /*G_SPAWN_STDOUT_TO_DEV_NULL |*/
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    FILE_RAW_SANITIZE_CHILD_ENV,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    error))
    {
      image = clay_file_load (run_mode, file_out);
    }

  /*if (rawtherapee_stdout) printf ("%s\n", rawtherapee_stdout);*/
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  g_file_delete (file_out, NULL, NULL);

  clay_progress_update (1.0);

  return image;
}

static ClayImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      GError **error)
{
  ClayImage *image            = NULL;
  GFile     *file_out         = clay_temp_file ("jpg");
  GFile     *thumb_pp3_file   = clay_temp_file ("pp3");
  FILE      *thumb_pp3_f      = g_fopen (g_file_peek_path (thumb_pp3_file), "w");
  gchar     *rawtherapee_stdout = NULL;
  const char *pp3_content =
    "[Version]\n"
    "AppVersion=5.0\n"
    "Version=326\n"
    "\n"
    "[Resize]\n"
    "Enabled=true\n"
    "AppliesTo=Cropped area\n"
    "Method=Lanczos\n"
    "Width=%d\n"
    "Height=%d\n"
    "\n"
    "[Sharpening]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenEdge]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenMicro]\n"
    "Enabled=false\n"
    "\n"
    "[Defringing]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Equalizer]\n"
    "Enabled=false\n"
    "\n"
    "[PostResizeSharpening]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Impulse Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Wavelet]\n"
    "Enabled=false\n"
    "\n"
    "[RAW Bayer]\n"
    "Method=fast\n"
    "\n"
    "[RAW X-Trans]\n"
    "Method=fast\n";


  gboolean  search_path = FALSE;
  gchar    *exec_path   = file_raw_get_executable_path ("rawtherapee", "-cli",
                                                        "RAWTHERAPEE_EXECUTABLE",
                                                        RAWTHERAPEE_BUNDLE_IDS,
                                                        REGISTRY_KEY_BASE,
                                                        &search_path);
  gchar *argv[] =
    {
      exec_path,
      "-o", (gchar *) g_file_peek_path (file_out),
      "-d",
      "-s",
      "-j",
      "-p", (gchar *) g_file_peek_path (thumb_pp3_file),
      "-f",
      "-c", (gchar *) g_file_peek_path (file),
      NULL
    };

  if (thumb_pp3_f)
    {
      if (fprintf (thumb_pp3_f, pp3_content, thumb_size, thumb_size) < 0)
        {
          fclose (thumb_pp3_f);
          thumb_pp3_f = NULL;
        }
    }

  clay_progress_init_printf (_("Opening thumbnail for '%s'"),
                             clay_file_get_utf8_name (file));

  if (thumb_pp3_f &&
      g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    FILE_RAW_SANITIZE_CHILD_ENV,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    error))
    {
      clay_progress_update (0.5);

      image = clay_file_load (CLAY_RUN_NONINTERACTIVE, file_out);
    }

  clay_progress_update (1.0);

  if (thumb_pp3_f)
    fclose (thumb_pp3_f);

  g_file_delete (thumb_pp3_file, NULL, NULL);
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  return image;
}
