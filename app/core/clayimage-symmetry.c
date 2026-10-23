/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimage-symmetry.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "claysymmetry.h"
#include "clayimage.h"
#include "clayimage-private.h"
#include "clayimage-symmetry.h"
#include "claysymmetry-mandala.h"
#include "claysymmetry-mirror.h"
#include "claysymmetry-tiling.h"


/**
 * clay_image_symmetry_list:
 *
 * Returns a list of #GType of all existing symmetries.
 **/
GList *
clay_image_symmetry_list (void)
{
  GList *list = NULL;

  list = g_list_prepend (list, GINT_TO_POINTER (CLAY_TYPE_MIRROR));
  list = g_list_prepend (list, GINT_TO_POINTER (CLAY_TYPE_TILING));
  list = g_list_prepend (list, GINT_TO_POINTER (CLAY_TYPE_MANDALA));

  return list;
}

/**
 * clay_image_symmetry_new:
 * @image: the #ClayImage
 * @type:  the #GType of the symmetry
 *
 * Creates a new #ClaySymmetry of @type attached to @image.
 * @type must be a subtype of `CLAY_TYPE_SYMMETRY`.
 * Note that using the base @type `CLAY_TYPE_SYMMETRY` creates an
 * identity transformation.
 *
 * Returns: the new #ClaySymmetry.
 **/
ClaySymmetry *
clay_image_symmetry_new (ClayImage *image,
                         GType      type)
{
  ClaySymmetry *sym = NULL;

  g_return_val_if_fail (g_type_is_a (type, CLAY_TYPE_SYMMETRY), NULL);

  sym = g_object_new (type,
                      "image", image,
                      NULL);

  return sym;
}

/**
 * clay_image_symmetry_add:
 * @image: the #ClayImage
 * @type:  the #GType of the symmetry
 *
 * Add a symmetry of type @type to @image and make it the
 * active transformation.
 **/
void
clay_image_symmetry_add (ClayImage    *image,
                         ClaySymmetry *sym)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SYMMETRY (sym));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->symmetries = g_list_prepend (private->symmetries,
                                        g_object_ref (sym));
}

/**
 * clay_image_symmetry_remove:
 * @image:   the #ClayImage
 * @sym: the #ClaySymmetry
 *
 * Remove @sym from the list of symmetries of @image.
 * If it was the active transformation, unselect it first.
 **/
void
clay_image_symmetry_remove (ClayImage    *image,
                            ClaySymmetry *sym)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_SYMMETRY (sym));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->active_symmetry == sym)
    clay_image_set_active_symmetry (image, CLAY_TYPE_SYMMETRY);

  private->symmetries = g_list_remove (private->symmetries, sym);
  g_object_unref (sym);
}

/**
 * clay_image_symmetry_get:
 * @image: the #ClayImage
 *
 * Returns: the list of #ClaySymmetry set on @image.
 * The returned list belongs to @image and should not be freed.
 **/
GList *
clay_image_symmetry_get (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return private->symmetries;
}

/**
 * clay_image_set_active_symmetry:
 * @image: the #ClayImage
 * @type:  the #GType of the symmetry
 *
 * Select the symmetry of type @type.
 * Using the GType allows to select a transformation without
 * knowing whether one of the same @type was already created.
 *
 * Returns TRUE on success, FALSE if no such symmetry was found.
 **/
gboolean
clay_image_set_active_symmetry (ClayImage *image,
                                GType      type)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  g_object_set (image,
                "symmetry", type,
                NULL);

  return TRUE;
}

/**
 * clay_image_get_active_symmetry:
 * @image: the #ClayImage
 *
 * Returns the #ClaySymmetry transformation active on @image.
 **/
ClaySymmetry *
clay_image_get_active_symmetry (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return private->active_symmetry;
}
