/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * clay-internal-data.c
 * Copyright (C) 2017 Ell
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clay-gradients.h"
#include "clay-internal-data.h"
#include "claydata.h"
#include "claydataloaderfactory.h"
#include "clayerror.h"
#include "claygradient-load.h"

#include "clay-intl.h"


#define CLAY_INTERNAL_DATA_DIRECTORY "internal-data"


typedef ClayData * (* ClayDataGetFunc) (Clay *clay);


typedef struct _ClayInternalDataFile ClayInternalDataFile;

struct _ClayInternalDataFile
{
  const gchar      *name;
  ClayDataGetFunc   get_func;
  ClayDataLoadFunc  load_func;
};


/*  local function prototypes  */

static gboolean   clay_internal_data_create_directory (GError                     **error);

static GFile    * clay_internal_data_get_file         (const ClayInternalDataFile  *data_file);

static gboolean   clay_internal_data_load_data_file   (Clay                        *clay,
                                                       const ClayInternalDataFile  *data_file,
                                                       GError                     **error);
static gboolean   clay_internal_data_save_data_file   (Clay                        *clay,
                                                       const ClayInternalDataFile  *data_file,
                                                       GError                     **error);
static gboolean   clay_internal_data_delete_data_file (Clay                        *clay,
                                                       const ClayInternalDataFile  *data_file,
                                                       GError                     **error);

/*  static variables  */

static const ClayInternalDataFile internal_data_files[] =
{
  /* Custom gradient */
  {
    .name      = "custom" CLAY_GRADIENT_FILE_EXTENSION,
    .get_func  = (ClayDataGetFunc) clay_gradients_get_custom,
    .load_func = clay_gradient_load
  }
};


/*  public functions  */

gboolean
clay_internal_data_load (Clay    *clay,
                         GError **error)
{
  gint i;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const ClayInternalDataFile *data_file = &internal_data_files[i];

      if (! clay_internal_data_load_data_file (clay, data_file, error))
        return FALSE;
    }

  return TRUE;
}

gboolean
clay_internal_data_save (Clay    *clay,
                         GError **error)
{
  gint i;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_internal_data_create_directory (error))
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const ClayInternalDataFile *data_file = &internal_data_files[i];

      if (! clay_internal_data_save_data_file (clay, data_file, error))
        return FALSE;
    }

  return TRUE;
}

gboolean
clay_internal_data_clear (Clay    *clay,
                          GError **error)
{
  gint i;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const ClayInternalDataFile *data_file = &internal_data_files[i];

      if (! clay_internal_data_delete_data_file (clay, data_file, error))
        return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static gboolean
clay_internal_data_create_directory (GError **error)
{
  GFile    *directory;
  GError   *my_error = NULL;
  gboolean  success;

  directory = clay_directory_file (CLAY_INTERNAL_DATA_DIRECTORY, NULL);

  success = g_file_make_directory (directory, NULL, &my_error);

  g_object_unref (directory);

  if (! success)
    {
      if (my_error->code == G_IO_ERROR_EXISTS)
        {
          g_clear_error (&my_error);
          success = TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
        }
    }

  return success;
}

static GFile *
clay_internal_data_get_file (const ClayInternalDataFile *data_file)
{
  return clay_directory_file (CLAY_INTERNAL_DATA_DIRECTORY,
                              data_file->name,
                              NULL);
}

static gboolean
clay_internal_data_load_data_file (Clay                        *clay,
                                   const ClayInternalDataFile  *data_file,
                                   GError                     **error)
{
  GFile        *file;
  GInputStream *input;
  ClayData     *data;
  GList        *list;
  GError       *my_error = NULL;

  file = clay_internal_data_get_file (data_file);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (! input)
    {
      g_object_unref (file);

      if (my_error->code == G_IO_ERROR_NOT_FOUND)
        {
          g_clear_error (&my_error);
          return TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
          return FALSE;
        }
    }

  list = data_file->load_func (clay->user_context, file, input, error);

  g_object_unref (input);
  g_object_unref (file);

  if (! list)
    return FALSE;

  data = data_file->get_func (clay);

  clay_data_copy (data, CLAY_DATA (list->data));

  g_list_free_full (list, g_object_unref);

  return TRUE;
}

static gboolean
clay_internal_data_save_data_file (Clay                        *clay,
                                   const ClayInternalDataFile  *data_file,
                                   GError                     **error)
{
  GFile         *file;
  GOutputStream *output;
  ClayData      *data;
  gboolean       success;

  file = clay_internal_data_get_file (data_file);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE,
                                            G_FILE_CREATE_NONE,
                                            NULL, error));

  if (! output)
    {
      g_object_unref (file);

      return FALSE;
    }

  data = data_file->get_func (clay);

  /* we bypass clay_data_save() and call the data's save() virtual function
   * directly, since clay_data_save() is a nop for internal data.
   *
   * FIXME:  we save the data whether it's dirty or not, since it might not
   * exist on disk.  currently, we only use this for cheap data, such as
   * gradients, so this is not a big concern, but if we save more expensive
   * data in the future, we should fix this.
   */
  clay_assert (CLAY_DATA_GET_CLASS (data)->save);
  success = CLAY_DATA_GET_CLASS (data)->save (data, output, error);

  if (success)
    {
      if (! g_output_stream_close (output, NULL, error))
        {
          g_prefix_error (error,
                          _("Error saving '%s': "),
                          clay_file_get_utf8_name (file));
          success = FALSE;
        }
    }
  else
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      if (error && *error)
        {
          g_prefix_error (error,
                          _("Error saving '%s': "),
                          clay_file_get_utf8_name (file));
        }
      else
        {
          g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_WRITE,
                       _("Error saving '%s'"),
                       clay_file_get_utf8_name (file));
        }
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }

  g_object_unref (output);
  g_object_unref (file);

  return success;
}

static gboolean
clay_internal_data_delete_data_file (Clay                        *clay,
                                     const ClayInternalDataFile  *data_file,
                                     GError                     **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  file = clay_internal_data_get_file (data_file);

  if (clay->be_verbose)
    g_print ("Deleting '%s'\n", clay_file_get_utf8_name (file));

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}
