/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "pdb-types.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claybrush.h"
#include "core/claybrushgenerated.h"
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claydatafactory.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-sample-points.h"
#include "core/clayitem.h"
#include "core/claymybrush.h"
#include "core/claypalette.h"
#include "core/claypattern.h"

#include "path/claypath.h"

#include "text/claytextlayer.h"
#include "text/clayfont.h"

#include "claypdb-utils.h"
#include "claypdberror.h"

#include "clay-intl.h"


const gchar * clay_pdb_get_data_label (GType data_type);



GList *
clay_pdb_get_resources (Clay               *clay,
                        GType               data_type,
                        const gchar        *name,
                        ClayPDBDataAccess   access,
                        GError            **error)
{
  GList           *resources;
  ClayDataFactory *factory;
  ClayContainer   *container;
  const gchar     *label;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = clay_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  factory = clay_get_data_factory (clay, data_type);
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);

  container = clay_data_factory_get_container (factory);
  resources = clay_container_get_children_by_name (container, name);

  if (! resources && ! strcmp (name, "Standard"))
    {
      ClayData *resource;

      resource = clay_data_factory_data_get_standard (factory, clay_get_user_context (clay));
      g_return_val_if_fail (resource != NULL, NULL);
      resources = g_list_prepend (NULL, resource);
    }

  if (! resources)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & CLAY_PDB_DATA_ACCESS_WRITE) ||
           (access & CLAY_PDB_DATA_ACCESS_RENAME))
    {
      for (GList *iter = resources; iter; iter = iter->next)
        {
          if ((access & CLAY_PDB_DATA_ACCESS_WRITE) &&
              ! clay_data_is_writable (CLAY_DATA (iter->data)))
            {
              g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           /* TRANSLATOR: the first %s is a data label from the
                            * PDB-error-data-label context. The second %s is a data
                            * name.
                            */
                           C_("PDB-error-message", "%s '%s' is not editable"), label, name);
              return NULL;
            }
          else if ((access & CLAY_PDB_DATA_ACCESS_RENAME) &&
                   ! clay_viewable_is_name_editable (CLAY_VIEWABLE (iter->data)))
            {
              g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           /* TRANSLATOR: the first %s is a data label from the
                            * PDB-error-data-label context. The second %s is a data
                            * name.
                            */
                           C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
              return NULL;
            }
        }
    }

  return resources;
}

ClayResource *
clay_pdb_get_resource (Clay               *clay,
                       GType               data_type,
                       const gchar        *name,
                       ClayPDBDataAccess   access,
                       GError            **error)
{
  ClayDataFactory *factory;
  ClayResource    *resource;
  const gchar     *label;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = clay_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  factory = clay_get_data_factory (clay, data_type);
  resource = CLAY_RESOURCE (clay_data_factory_get_data (factory, name, NULL, TRUE));

  if (! resource)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & CLAY_PDB_DATA_ACCESS_WRITE) &&
           ! clay_data_is_writable (CLAY_DATA (resource)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not editable"), label, name);
      return NULL;
    }
  else if ((access & CLAY_PDB_DATA_ACCESS_RENAME) &&
           ! clay_viewable_is_name_editable (CLAY_VIEWABLE (resource)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
      return NULL;
    }

  return resource;
}

ClayResource *
clay_pdb_get_resource_by_id (Clay               *clay,
                             GType               data_type,
                             const gchar        *name,
                             const gchar        *collection,
                             gboolean            is_internal,
                             ClayPDBDataAccess   access,
                             GError            **error)
{
  ClayDataFactory *factory;
  ClayResource    *resource;
  const gchar     *label;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = clay_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  factory = clay_get_data_factory (clay, data_type);
  resource = CLAY_RESOURCE (clay_data_factory_get_data (factory, name, collection, is_internal));

  if (! resource)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & CLAY_PDB_DATA_ACCESS_WRITE) &&
           ! clay_data_is_writable (CLAY_DATA (resource)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not editable"), label, name);
      return NULL;
    }
  else if ((access & CLAY_PDB_DATA_ACCESS_RENAME) &&
           ! clay_viewable_is_name_editable (CLAY_VIEWABLE (resource)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
      return NULL;
    }

  return resource;
}

ClayBrush *
clay_pdb_get_generated_brush (Clay               *clay,
                              const gchar        *name,
                              ClayPDBDataAccess   access,
                              GError            **error)
{
  ClayBrush *brush;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  brush = CLAY_BRUSH (clay_pdb_get_resource (clay, CLAY_TYPE_BRUSH_GENERATED, name, access, error));

  if (brush != NULL && ! CLAY_IS_BRUSH_GENERATED (brush))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not a generated brush"), name);
      return NULL;
    }

  return brush;
}

ClayBuffer *
clay_pdb_get_buffer (Clay         *clay,
                     const gchar  *name,
                     GError      **error)
{
  ClayBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty buffer name"));
      return NULL;
    }

  buffer = (ClayBuffer *)
    clay_container_get_child_by_name (clay->named_buffers, name);

  if (! buffer)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Named buffer '%s' not found"), name);
    }

  return buffer;
}

ClayPaintInfo *
clay_pdb_get_paint_info (Clay         *clay,
                         const gchar  *name,
                         GError      **error)
{
  ClayPaintInfo *paint_info;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty paint method name"));
      return NULL;
    }

  paint_info = (ClayPaintInfo *)
    clay_container_get_child_by_name (clay->paint_info_list, name);

  if (! paint_info)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint method '%s' does not exist"), name);
    }

  return paint_info;
}

gboolean
clay_pdb_item_is_attached (ClayItem           *item,
                           ClayImage          *image,
                           ClayPDBItemModify   modify,
                           GError            **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_item_is_attached (item))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it has not "
                     "been added to an image"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  if (image && image != clay_item_get_image (item))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "attached to another image"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return clay_pdb_item_is_modifiable (item, modify, error);
}

gboolean
clay_pdb_item_is_in_tree (ClayItem           *item,
                          ClayImage          *image,
                          ClayPDBItemModify   modify,
                          GError            **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_pdb_item_is_attached (item, image, modify, error))
    return FALSE;

  if (! clay_item_get_tree (item))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is not "
                     "a direct child of an item tree"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_in_same_tree (ClayItem   *item,
                               ClayItem   *item2,
                               ClayImage  *image,
                               GError    **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (item2), FALSE);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_pdb_item_is_in_tree (item, image, FALSE, error) ||
      ! clay_pdb_item_is_in_tree (item2, image, FALSE, error))
    return FALSE;

  if (clay_item_get_tree (item) != clay_item_get_tree (item2))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Items '%s' (%d) and '%s' (%d) cannot be used "
                     "because they are not part of the same item tree"),
                   clay_object_get_name (item),
                   clay_item_get_id (item),
                   clay_object_get_name (item2),
                   clay_item_get_id (item2));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_not_ancestor (ClayItem  *item,
                               ClayItem  *not_descendant,
                               GError   **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (CLAY_IS_ITEM (not_descendant), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_viewable_is_ancestor (CLAY_VIEWABLE (item),
                                 CLAY_VIEWABLE (not_descendant)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) must not be an ancestor of "
                     "'%s' (%d)"),
                   clay_object_get_name (item),
                   clay_item_get_id (item),
                   clay_object_get_name (not_descendant),
                   clay_item_get_id (not_descendant));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_floating (ClayItem  *item,
                           ClayImage *dest_image,
                           GError   **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! g_object_is_floating (item))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) has already been added to an image"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }
  else if (clay_item_get_image (item) != dest_image)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Trying to add item '%s' (%d) to wrong image"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_modifiable (ClayItem           *item,
                             ClayPDBItemModify   modify,
                             GError            **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /*  When a channel is position-locked, it is also implicitly
   *  content-locked because we translate channels by modifying their
   *  pixels.
   */
  if ((modify & CLAY_PDB_ITEM_POSITION) && CLAY_IS_CHANNEL (item))
    modify |= CLAY_PDB_ITEM_CONTENT;

  if ((modify & CLAY_PDB_ITEM_CONTENT) && clay_item_is_content_locked (item, NULL))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "contents are locked"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  if ((modify & CLAY_PDB_ITEM_POSITION) && clay_item_is_position_locked (item, NULL))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "position and size are locked"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_group (ClayItem  *item,
                        GError   **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_viewable_get_children (CLAY_VIEWABLE (item)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "not a group item"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_item_is_not_group (ClayItem  *item,
                            GError   **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_viewable_get_children (CLAY_VIEWABLE (item)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because it "
                     "is a group item"),
                   clay_object_get_name (item),
                   clay_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
clay_pdb_layer_is_text_layer (ClayLayer          *layer,
                              ClayPDBItemModify   modify,
                              GError            **error)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_item_is_text_layer (CLAY_ITEM (layer)))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Layer '%s' (%d) cannot be used because it is not "
                     "a text layer"),
                   clay_object_get_name (layer),
                   clay_item_get_id (CLAY_ITEM (layer)));

      return FALSE;
    }

  return clay_pdb_item_is_attached (CLAY_ITEM (layer), NULL, modify, error);
}

gboolean
clay_pdb_image_is_base_type (ClayImage          *image,
                             ClayImageBaseType   type,
                             GError            **error)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_image_get_base_type (image) == type)
    return TRUE;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) is of type '%s', "
                 "but an image of type '%s' is expected"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               clay_get_enum_value_nick (CLAY_TYPE_IMAGE_BASE_TYPE,
                                         clay_image_get_base_type (image)),
               clay_get_enum_value_nick (CLAY_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
clay_pdb_image_is_not_base_type (ClayImage          *image,
                                 ClayImageBaseType   type,
                                 GError            **error)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_image_get_base_type (image) != type)
    return TRUE;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of type '%s'"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               clay_get_enum_value_nick (CLAY_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
clay_pdb_image_is_precision (ClayImage      *image,
                             ClayPrecision   precision,
                             GError        **error)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_image_get_precision (image) == precision)
    return TRUE;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) has precision '%s', "
                 "but an image of precision '%s' is expected"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               clay_get_enum_value_nick (CLAY_TYPE_PRECISION,
                                         clay_image_get_precision (image)),
               clay_get_enum_value_nick (CLAY_TYPE_PRECISION, precision));

  return FALSE;
}

gboolean
clay_pdb_image_is_not_precision (ClayImage      *image,
                                 ClayPrecision   precision,
                                 GError        **error)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (clay_image_get_precision (image) != precision)
    return TRUE;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of precision '%s'"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               clay_get_enum_value_nick (CLAY_TYPE_PRECISION, precision));

  return FALSE;
}

ClayGuide *
clay_pdb_image_get_guide (ClayImage  *image,
                          gint        guide_id,
                          GError    **error)
{
  ClayGuide *guide;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  guide = clay_image_get_guide (image, guide_id);

  if (guide)
    return guide;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain guide with ID %d"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               guide_id);
  return NULL;
}

ClaySamplePoint *
clay_pdb_image_get_sample_point (ClayImage  *image,
                                 gint        sample_point_id,
                                 GError    **error)
{
  ClaySamplePoint *sample_point;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  sample_point = clay_image_get_sample_point (image, sample_point_id);

  if (sample_point)
    return sample_point;

  g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain sample point with ID %d"),
               clay_image_get_display_name (image),
               clay_image_get_id (image),
               sample_point_id);
  return NULL;
}

ClayStroke *
clay_pdb_get_path_stroke (ClayPath           *path,
                          gint                stroke_id,
                          ClayPDBItemModify   modify,
                          GError            **error)
{
  ClayStroke *stroke = NULL;

  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! clay_pdb_item_is_not_group (CLAY_ITEM (path), error))
    return NULL;

  if (! modify || clay_pdb_item_is_modifiable (CLAY_ITEM (path),
                                               modify, error))
    {
      stroke = clay_path_stroke_get_by_id (path, stroke_id);

      if (! stroke)
        g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                     _("Path object %d does not contain stroke with ID %d"),
                     clay_item_get_id (CLAY_ITEM (path)), stroke_id);
    }

  return stroke;
}

gboolean
clay_pdb_is_canonical_procedure (const gchar  *procedure_name,
                                 GError      **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! clay_is_canonical_identifier (procedure_name))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                   _("Procedure name '%s' is not a canonical identifier"),
                   procedure_name);
      return FALSE;
    }

  return TRUE;
}


/* Private functions. */

const gchar *
clay_pdb_get_data_label (GType data_type)
{
  g_return_val_if_fail (g_type_is_a (data_type, CLAY_TYPE_DATA), NULL);

  if (g_type_is_a (data_type, CLAY_TYPE_BRUSH_GENERATED))
    return C_("PDB-error-data-label", "Generated brush");
  else if (g_type_is_a (data_type, CLAY_TYPE_BRUSH))
    return C_("PDB-error-data-label", "Brush");
  else if (g_type_is_a (data_type, CLAY_TYPE_PATTERN))
    return C_("PDB-error-data-label", "Pattern");
  else if (g_type_is_a (data_type, CLAY_TYPE_GRADIENT))
    return C_("PDB-error-data-label", "Gradient");
  else if (g_type_is_a (data_type, CLAY_TYPE_PALETTE))
    return C_("PDB-error-data-label", "Palette");
  else if (g_type_is_a (data_type, CLAY_TYPE_FONT))
    return C_("PDB-error-data-label", "Font");
  else if (g_type_is_a (data_type, CLAY_TYPE_DYNAMICS))
    return C_("PDB-error-data-label", "Paint dynamics");
  else if (g_type_is_a (data_type, CLAY_TYPE_MYBRUSH))
    return C_("PDB-error-data-label", "MyPaint brush");

  /* If we reach this, it means we forgot a data type in our list! */
  g_return_val_if_reached (NULL);
}
