/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   MacPaint plug-in
 *
 *   Copyright (C) 2026 Alex S.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define LOAD_PROC      "file-macpaint-load"
#define PLUG_IN_BINARY "file-macpaint"
#define PLUG_IN_ROLE   "clay-file-macpaint"

typedef struct _Macpaint      Macpaint;
typedef struct _MacpaintClass MacpaintClass;

struct _Macpaint
{
  ClayPlugIn      parent_instance;
};

struct _MacpaintClass
{
  ClayPlugInClass parent_class;
};


#define MACPAINT_TYPE  (macpaint_get_type ())
#define MACPAINT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MACPAINT_TYPE, Macpaint))

GType                   macpaint_get_type         (void);


static GList          * macpaint_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * macpaint_create_procedure (ClayPlugIn            *plug_in,
                                                   const gchar           *name);

static ClayValueArray * macpaint_load             (ClayProcedure         *procedure,
                                                   ClayRunMode            run_mode,
                                                   GFile                 *file,
                                                   ClayMetadata          *metadata,
                                                   ClayMetadataLoadFlags *flags,
                                                   ClayProcedureConfig   *config,
                                                   gpointer               run_data);

static ClayImage      * load_image                (GFile                 *file,
                                                   GObject               *config,
                                                   ClayRunMode            run_mode,
                                                   GError               **error);


G_DEFINE_TYPE (Macpaint, macpaint, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (MACPAINT_TYPE)
DEFINE_STD_SET_I18N


static void
macpaint_class_init (MacpaintClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = macpaint_query_procedures;
  plug_in_class->create_procedure = macpaint_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
macpaint_init (Macpaint *macpaint)
{
}

static GList *
macpaint_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static ClayProcedure *
macpaint_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           macpaint_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("MacPaint image"));

      clay_procedure_set_documentation (procedure,
                                        _("Load MacPaint image format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2026");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-macpaint");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "mac,pntg");
    }

  return procedure;
}

static ClayValueArray *
macpaint_load (ClayProcedure         *procedure,
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

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), run_mode, &error);

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

static ClayImage *
load_image (GFile        *file,
            GObject      *config,
            ClayRunMode   run_mode,
            GError      **error)
{
  ClayImage  *image      = NULL;
  ClayLayer  *layer;
  GeglBuffer *buffer;
  guchar     *pixels     = NULL;
  guchar     *data       = NULL;
  FILE       *fp;
  gchar       header[128];
  gboolean    has_header = TRUE;
  gsize       file_size;
  guchar      run;
  guchar      val;
  guchar      count;
  gint        pixel_count;
  gint        i;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  if (fread (header, 128, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Cannot open file for read: %s\n",
                   g_file_peek_path (file));
      fclose (fp);
      return NULL;
    }

  /* MacPaint images may or may not have an initial 128 byte header.
   * There are several conflicting heuristics to determine whether one
   * exists. Based on sample images, the most consistent check is if the
   * the second byte is between 1 and 63, and whether the reserved parts
   * of the header are all 0 */
  for (i = 101; i <= 125; i++)
    {
      if (header[i] > 0)
        {
          has_header = FALSE;
          break;
        }
    }

  if (has_header     &&
      (header[1] > 0 &&
       header[1] < 64))
    {
      fseek (fp, 640, SEEK_SET);
      file_size -= 640;
    }
  else
    {
      fseek (fp, 512, SEEK_SET);
      file_size -= 512;
    }

  /* MacPaint images are always 576 x 720 pixels, black and white */
  image = clay_image_new_with_precision (576, 720, CLAY_GRAY,
                                         CLAY_PRECISION_U8_NON_LINEAR);

  layer = clay_layer_new (image, _("Background"), 576, 720,
                          CLAY_GRAY_IMAGE,
                          100, clay_image_get_default_new_layer_mode (image));
  clay_image_insert_layer (image, layer, NULL, 0);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  pixels = g_try_malloc0 (576 * 720);
  data   = g_try_malloc0 (file_size);

  if (! pixels || ! data)
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      g_free (pixels);
      g_free (data);
      return NULL;
    }

  if (fread (data, file_size, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Cannot open file for read: %s\n",
                   g_file_peek_path (file));
      fclose (fp);
      g_free (pixels);
      g_free (data);
      return NULL;
    }
  fclose (fp);

  pixel_count = 0;
  i           = 0;
  while (i < file_size)
    {
      run = data[i++];

      if (pixel_count >= (576 * 720))
        break;

      if (run >= 128 || run <= 0)
        {
          count = (~run) + 2;

          if (i >= file_size)
            break;

          while (count > 0)
            {
              val = data[i];

              if (pixel_count >= (576 * 720))
                break;

              for (gint j = 0; j < 8; j++)
                {
                  pixels[pixel_count++] = ((val & 0x80) != 0) ? 0 : 255;

                  val <<= 1;

                  if (pixel_count >= (576 * 720))
                    break;
                }
              count--;
            }
          i++;
        }
      else
        {
          count = 1 + run;

          while (count > 0)
            {
              if (i >= file_size)
                break;

              val = data[i++];

              if (pixel_count >= (576 * 720))
                break;

              for (gint j = 0; j < 8; j++)
                {
                  pixels[pixel_count++] = ((val & 0x80) != 0) ? 0 : 255;

                  val <<= 1;

                  if (pixel_count >= (576 * 720))
                    break;
                }
              count--;
            }
        }
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, 576, 720), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);
  g_free (pixels);
  g_free (data);

  return image;
}
