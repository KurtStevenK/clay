/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Windows Shortcut (.lnk) plug-in
 *
 *   Copyright (C) 2024 Alex S.
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


#define LOAD_PROC       "file-lnk-load"
#define PLUG_IN_BINARY  "file-lnk"
#define PLUG_IN_ROLE    "clay-file-lnk"

typedef struct _Lnk      Lnk;
typedef struct _LnkClass LnkClass;

struct _Lnk
{
  ClayPlugIn      parent_instance;
};

struct _LnkClass
{
  ClayPlugInClass parent_class;
};


#define LNK_TYPE  (lnk_get_type ())
#define LNK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LNK_TYPE, Lnk))

GType                   lnk_get_type         (void);


static GList          * lnk_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * lnk_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * lnk_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);

static ClayImage      * load_image           (GFile                 *file,
                                              GObject               *config,
                                              ClayRunMode            run_mode,
                                              GError               **error);


G_DEFINE_TYPE (Lnk, lnk, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (LNK_TYPE)
DEFINE_STD_SET_I18N


static void
lnk_class_init (LnkClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = lnk_query_procedures;
  plug_in_class->create_procedure = lnk_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
lnk_init (Lnk *lnk)
{
}

static GList *
lnk_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static ClayProcedure *
lnk_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           lnk_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("Windows Shortcut"));

      clay_procedure_set_documentation (procedure,
                                        _("Follows a link to an image in a "
                                          ".lnk file"),
                                        _("Opens a .lnk file and if it points "
                                          "to an image, it asks Clay to open "
                                          "the linked image."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2024");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "lnk");
    }

  return procedure;
}

static ClayValueArray *
lnk_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image = NULL;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  /*  We handle PDB errors by forwarding them to the caller in
   *  our return values.
   */
  clay_plug_in_set_pdb_error_handler (clay_procedure_get_plug_in (procedure),
                                      CLAY_PDB_ERROR_HANDLER_PLUGIN);

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
  ClayImage  *image = NULL;
  FILE       *fp;
  guint32     flags;
  gsize       base;
  guint       length;
  guint       link_pos;
  guint       link_size;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Skip header size and GUID, then load data flags*/
  if (fseek (fp, 20, SEEK_SET) < 0 ||
      fread (&flags, sizeof (guint32), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      fclose (fp);
      return NULL;
    }
  flags = GUINT32_FROM_LE (flags);

  /* If HasTargetIDList bit is set, we want to skip over those too */
  if (flags & 0x01)
    {
      gushort offset;

      if (fseek (fp, 76, SEEK_SET) < 0 ||
          fread (&offset, sizeof (gushort), 1, fp) != 1)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }

      offset = GUINT16_TO_LE (offset);
      if (fseek (fp, offset, SEEK_CUR) < 0)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }
    }
  base = ftell (fp);

  /* Read in data length, then skip more unnecessary data */
  if (fread (&length, sizeof (guint32), 1, fp) != 1 ||
      fseek (fp, 12, SEEK_CUR) < 0                  ||
      fread (&link_pos, sizeof (guint32), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      fclose (fp);
      return NULL;
    }

  /* Length of the filename */
  link_size = length - link_pos - 1;

  if (link_size > 0)
    {
      gchar file_name[link_size];

      /* Jump to link address and read in the real file name */
      if (fseek (fp, base + link_pos, SEEK_SET) < 0 ||
          fread (file_name, sizeof (gchar), link_size, fp) != link_size)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }

      file_name[link_size - 1] = '\0';
      image = clay_file_load (CLAY_RUN_NONINTERACTIVE,
                              g_file_new_for_path (file_name));

      if (! image)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "%s", clay_pdb_get_last_error (clay_get_pdb ()));

    }
  fclose (fp);

  return image;
}
