/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydata.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "clayresource.h"


typedef enum
{
  CLAY_DATA_ERROR_OPEN,   /*  opening data file failed   */
  CLAY_DATA_ERROR_READ,   /*  reading data file failed   */
  CLAY_DATA_ERROR_WRITE,  /*  writing data file failed   */
  CLAY_DATA_ERROR_DELETE  /*  deleting data file failed  */
} ClayDataError;


#define CLAY_TYPE_DATA (clay_data_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayData,
                          clay_data,
                          CLAY, DATA,
                          ClayResource)


struct _ClayDataClass
{
  ClayResourceClass  parent_class;

  /*  signals  */
  void          (* dirty)         (ClayData  *data);

  /*  virtual functions  */
  gboolean      (* save)          (ClayData       *data,
                                   GOutputStream  *output,
                                   GError        **error);
  const gchar * (* get_extension) (ClayData       *data);
  void          (* copy)          (ClayData       *data,
                                   ClayData       *src_data);
  ClayData    * (* duplicate)     (ClayData       *data);
  gint          (* compare)       (ClayData       *data1,
                                   ClayData       *data2);
};


gint          clay_data_get_id           (ClayData     *data);
ClayData    * clay_data_get_by_id        (gint          id);

gboolean      clay_data_save             (ClayData     *data,
                                          GError      **error);

void          clay_data_dirty            (ClayData     *data);
void          clay_data_clean            (ClayData     *data);
gboolean      clay_data_is_dirty         (ClayData     *data);

void          clay_data_freeze           (ClayData     *data);
void          clay_data_thaw             (ClayData     *data);
gboolean      clay_data_is_frozen        (ClayData     *data);

gboolean      clay_data_delete_from_disk (ClayData     *data,
                                          GError      **error);

const gchar * clay_data_get_extension    (ClayData     *data);

void          clay_data_set_file         (ClayData     *data,
                                          GFile        *file,
                                          gboolean      writable,
                                          gboolean      deletable);
GFile       * clay_data_get_file         (ClayData     *data);
void          clay_data_set_image        (ClayData     *data,
                                          ClayImage    *image,
                                          gboolean      writable,
                                          gboolean      deletable);
ClayImage   * clay_data_get_image        (ClayData     *data);

void          clay_data_create_filename  (ClayData     *data,
                                          GFile        *dest_dir);

void          clay_data_set_folder_tags  (ClayData     *data,
                                          GFile        *top_directory);

const gchar * clay_data_get_mime_type    (ClayData     *data);

gboolean      clay_data_is_writable      (ClayData     *data);
gboolean      clay_data_is_deletable     (ClayData     *data);

void          clay_data_set_mtime        (ClayData     *data,
                                          gint64        mtime);
gint64        clay_data_get_mtime        (ClayData     *data);

gboolean      clay_data_is_copyable      (ClayData     *data);
void          clay_data_copy             (ClayData     *data,
                                          ClayData     *src_data);

gboolean      clay_data_is_duplicatable  (ClayData     *data);
ClayData    * clay_data_duplicate        (ClayData     *data);

void          clay_data_make_internal    (ClayData     *data,
                                          const gchar  *collection);
gboolean      clay_data_is_internal      (ClayData     *data);

gint          clay_data_compare          (ClayData     *data1,
                                          ClayData     *data2);
gboolean      clay_data_identify         (ClayData     *data,
                                          const gchar  *name,
                                          const gchar  *collection_id,
                                          gboolean      is_internal);
void          clay_data_get_identifiers  (ClayData     *data,
                                          gchar       **name,
                                          gchar       **collection_id,
                                          gboolean     *is_internal);

#define CLAY_DATA_ERROR (clay_data_error_quark ())

GQuark        clay_data_error_quark      (void) G_GNUC_CONST;
