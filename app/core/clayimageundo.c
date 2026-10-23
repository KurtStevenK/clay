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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay-memsize.h"
#include "claydrawable.h"
#include "claygrid.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "clayimage-grid.h"
#include "clayimage-metadata.h"
#include "clayimage-private.h"
#include "clayimageundo.h"
#include "claypalette.h"


enum
{
  PROP_0,
  PROP_PREVIOUS_ORIGIN_X,
  PROP_PREVIOUS_ORIGIN_Y,
  PROP_PREVIOUS_WIDTH,
  PROP_PREVIOUS_HEIGHT,
  PROP_GRID,
  PROP_PARASITE_NAME
};


static void     clay_image_undo_constructed  (GObject             *object);
static void     clay_image_undo_set_property (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void     clay_image_undo_get_property (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static gint64   clay_image_undo_get_memsize  (ClayObject          *object,
                                              gint64              *gui_size);

static void     clay_image_undo_pop          (ClayUndo            *undo,
                                              ClayUndoMode         undo_mode,
                                              ClayUndoAccumulator *accum);
static void     clay_image_undo_free         (ClayUndo            *undo,
                                              ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayImageUndo, clay_image_undo, CLAY_TYPE_UNDO)

#define parent_class clay_image_undo_parent_class


static void
clay_image_undo_class_init (ClayImageUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_image_undo_constructed;
  object_class->set_property     = clay_image_undo_set_property;
  object_class->get_property     = clay_image_undo_get_property;

  clay_object_class->get_memsize = clay_image_undo_get_memsize;

  undo_class->pop                = clay_image_undo_pop;
  undo_class->free               = clay_image_undo_free;

  g_object_class_install_property (object_class, PROP_PREVIOUS_ORIGIN_X,
                                   g_param_spec_int ("previous-origin-x",
                                                     NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_ORIGIN_Y,
                                   g_param_spec_int ("previous-origin-y",
                                                     NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_WIDTH,
                                   g_param_spec_int ("previous-width",
                                                     NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PREVIOUS_HEIGHT,
                                   g_param_spec_int ("previous-height",
                                                     NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        CLAY_TYPE_GRID,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_image_undo_init (ClayImageUndo *undo)
{
}

static void
clay_image_undo_constructed (GObject *object)
{
  ClayImageUndo *image_undo = CLAY_IMAGE_UNDO (object);
  ClayImage     *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  image = CLAY_UNDO (object)->image;

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_IMAGE_TYPE:
      image_undo->base_type = clay_image_get_base_type (image);
      break;

    case CLAY_UNDO_IMAGE_PRECISION:
      image_undo->precision = clay_image_get_precision (image);
      break;

    case CLAY_UNDO_IMAGE_SIZE:
      image_undo->width  = clay_image_get_width  (image);
      image_undo->height = clay_image_get_height (image);
      break;

    case CLAY_UNDO_IMAGE_RESOLUTION:
      clay_image_get_resolution (image,
                                 &image_undo->xresolution,
                                 &image_undo->yresolution);
      image_undo->resolution_unit = clay_image_get_unit (image);
      break;

    case CLAY_UNDO_IMAGE_GRID:
      clay_assert (CLAY_IS_GRID (image_undo->grid));
      break;

    case CLAY_UNDO_IMAGE_COLORMAP:
        {
          ClayPalette *palette;

          palette = clay_image_get_colormap_palette (image);
          image_undo->colormap = CLAY_PALETTE (clay_palette_new (NULL, clay_object_get_name (palette)));
          clay_data_copy (CLAY_DATA (image_undo->colormap), CLAY_DATA (palette));
        }
      break;

    case CLAY_UNDO_IMAGE_HIDDEN_PROFILE:
      g_set_object (&image_undo->hidden_profile,
                    _clay_image_get_hidden_profile (image));
      break;

    case CLAY_UNDO_IMAGE_METADATA:
      image_undo->metadata =
        clay_metadata_duplicate (clay_image_get_metadata (image));
      break;

    case CLAY_UNDO_PARASITE_ATTACH:
    case CLAY_UNDO_PARASITE_REMOVE:
      clay_assert (image_undo->parasite_name != NULL);

      image_undo->parasite = clay_parasite_copy
        (clay_image_parasite_find (image, image_undo->parasite_name));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_image_undo_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayImageUndo *image_undo = CLAY_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_PREVIOUS_ORIGIN_X:
      image_undo->previous_origin_x = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_ORIGIN_Y:
      image_undo->previous_origin_y = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_WIDTH:
      image_undo->previous_width = g_value_get_int (value);
      break;
    case PROP_PREVIOUS_HEIGHT:
      image_undo->previous_height = g_value_get_int (value);
      break;
    case PROP_GRID:
      {
        ClayGrid *grid = g_value_get_object (value);

        if (grid)
          image_undo->grid = clay_config_duplicate (CLAY_CONFIG (grid));
      }
      break;
    case PROP_PARASITE_NAME:
      image_undo->parasite_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_undo_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayImageUndo *image_undo = CLAY_IMAGE_UNDO (object);

  switch (property_id)
    {
    case PROP_PREVIOUS_ORIGIN_X:
      g_value_set_int (value, image_undo->previous_origin_x);
      break;
    case PROP_PREVIOUS_ORIGIN_Y:
      g_value_set_int (value, image_undo->previous_origin_y);
      break;
    case PROP_PREVIOUS_WIDTH:
      g_value_set_int (value, image_undo->previous_width);
      break;
    case PROP_PREVIOUS_HEIGHT:
      g_value_set_int (value, image_undo->previous_height);
      break;
    case PROP_GRID:
      g_value_set_object (value, image_undo->grid);
       break;
    case PROP_PARASITE_NAME:
      g_value_set_string (value, image_undo->parasite_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_image_undo_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayImageUndo *image_undo = CLAY_IMAGE_UNDO (object);
  gint64         memsize    = 0;

  if (image_undo->colormap)
    memsize += CLAY_IMAGE_COLORMAP_SIZE;

  if (image_undo->metadata)
    memsize += clay_g_object_get_memsize (G_OBJECT (image_undo->metadata));

  memsize += clay_object_get_memsize (CLAY_OBJECT (image_undo->grid),
                                      gui_size);
  memsize += clay_string_get_memsize (image_undo->parasite_name);
  memsize += clay_parasite_get_memsize (image_undo->parasite, gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_image_undo_pop (ClayUndo            *undo,
                     ClayUndoMode         undo_mode,
                     ClayUndoAccumulator *accum)
{
  ClayImageUndo    *image_undo = CLAY_IMAGE_UNDO (undo);
  ClayImage        *image      = undo->image;
  ClayImagePrivate *private    = CLAY_IMAGE_GET_PRIVATE (image);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_IMAGE_TYPE:
      {
        ClayImageBaseType base_type;

        base_type = image_undo->base_type;
        image_undo->base_type = clay_image_get_base_type (image);
        g_object_set (image, "base-type", base_type, NULL);

        clay_image_colormap_changed (image, -1);

        if (image_undo->base_type != clay_image_get_base_type (image))
          {
            if ((image_undo->base_type            == CLAY_GRAY) ||
                (clay_image_get_base_type (image) == CLAY_GRAY))
              {
                /* in case there was no profile undo, we need to emit
                 * profile-changed anyway
                 */
                clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (image));
              }

            accum->mode_changed = TRUE;
          }
      }
      break;

    case CLAY_UNDO_IMAGE_PRECISION:
      {
        ClayPrecision precision;

        precision = image_undo->precision;
        image_undo->precision = clay_image_get_precision (image);
        g_object_set (image, "precision", precision, NULL);

        if (image_undo->precision != clay_image_get_precision (image))
          accum->precision_changed = TRUE;
      }
      break;

    case CLAY_UNDO_IMAGE_SIZE:
      {
        gint width;
        gint height;
        gint previous_origin_x;
        gint previous_origin_y;
        gint previous_width;
        gint previous_height;

        width             = image_undo->width;
        height            = image_undo->height;
        previous_origin_x = image_undo->previous_origin_x;
        previous_origin_y = image_undo->previous_origin_y;
        previous_width    = image_undo->previous_width;
        previous_height   = image_undo->previous_height;

        /* Transform to a redo */
        image_undo->width             = clay_image_get_width  (image);
        image_undo->height            = clay_image_get_height (image);
        image_undo->previous_origin_x = -previous_origin_x;
        image_undo->previous_origin_y = -previous_origin_y;
        image_undo->previous_width    = width;
        image_undo->previous_height   = height;

        g_object_set (image,
                      "width",  width,
                      "height", height,
                      NULL);

        clay_drawable_invalidate_boundary
          (CLAY_DRAWABLE (clay_image_get_mask (image)));

        if (clay_image_get_width  (image) != image_undo->width ||
            clay_image_get_height (image) != image_undo->height)
          {
            accum->size_changed      = TRUE;
            accum->previous_origin_x = previous_origin_x;
            accum->previous_origin_y = previous_origin_y;
            accum->previous_width    = previous_width;
            accum->previous_height   = previous_height;
          }
      }
      break;

    case CLAY_UNDO_IMAGE_RESOLUTION:
      {
        gdouble xres;
        gdouble yres;

        clay_image_get_resolution (image, &xres, &yres);

        if (ABS (image_undo->xresolution - xres) >= 1e-5 ||
            ABS (image_undo->yresolution - yres) >= 1e-5)
          {
            private->xresolution = image_undo->xresolution;
            private->yresolution = image_undo->yresolution;

            image_undo->xresolution = xres;
            image_undo->yresolution = yres;

            accum->resolution_changed = TRUE;
          }
      }

      if (image_undo->resolution_unit != clay_image_get_unit (image))
        {
          ClayUnit *unit;

          unit = clay_image_get_unit (image);
          private->resolution_unit = image_undo->resolution_unit;
          image_undo->resolution_unit = unit;

          accum->unit_changed = TRUE;
        }
      break;

    case CLAY_UNDO_IMAGE_GRID:
      {
        ClayGrid *grid;

        grid = clay_config_duplicate (CLAY_CONFIG (clay_image_get_grid (image)));

        clay_image_set_grid (image, image_undo->grid, FALSE);

        g_object_unref (image_undo->grid);
        image_undo->grid = grid;
      }
      break;

    case CLAY_UNDO_IMAGE_COLORMAP:
      {
        ClayPalette *colormap;

        colormap = clay_image_get_colormap_palette (image);
        if (colormap)
          {
            ClayPalette *palette = colormap;

            colormap = CLAY_PALETTE (clay_palette_new (NULL, clay_object_get_name (palette)));
            clay_data_copy (CLAY_DATA (colormap), CLAY_DATA (palette));
          }

        if (image_undo->colormap)
          clay_image_set_colormap_palette (image, image_undo->colormap, FALSE);
        else
          clay_image_unset_colormap (image, FALSE);

        if (image_undo->colormap)
          g_object_unref (image_undo->colormap);

        image_undo->colormap = colormap;
      }
      break;

    case CLAY_UNDO_IMAGE_HIDDEN_PROFILE:
      {
        ClayColorProfile *hidden_profile = NULL;

        g_set_object (&hidden_profile, _clay_image_get_hidden_profile (image));
        _clay_image_set_hidden_profile (image, image_undo->hidden_profile,
                                        FALSE);
        image_undo->hidden_profile = hidden_profile;
      }
      break;

    case CLAY_UNDO_IMAGE_METADATA:
      {
        ClayMetadata *metadata;

        metadata = clay_metadata_duplicate (clay_image_get_metadata (image));

        clay_image_set_metadata (image, image_undo->metadata, FALSE);

        if (image_undo->metadata)
          g_object_unref (image_undo->metadata);
        image_undo->metadata = metadata;
      }
      break;

    case CLAY_UNDO_PARASITE_ATTACH:
    case CLAY_UNDO_PARASITE_REMOVE:
      {
        ClayParasite *parasite = image_undo->parasite;

        image_undo->parasite = clay_parasite_copy
          (clay_image_parasite_find (image, image_undo->parasite_name));

        if (parasite)
          clay_image_parasite_attach (image, parasite, FALSE);
        else
          clay_image_parasite_detach (image, image_undo->parasite_name, FALSE);

        if (parasite)
          clay_parasite_free (parasite);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_image_undo_free (ClayUndo     *undo,
                      ClayUndoMode  undo_mode)
{
  ClayImageUndo *image_undo = CLAY_IMAGE_UNDO (undo);

  g_clear_object (&image_undo->grid);
  g_clear_object (&image_undo->colormap);
  g_clear_object (&image_undo->hidden_profile);
  g_clear_object (&image_undo->metadata);
  g_clear_pointer (&image_undo->parasite_name, g_free);
  g_clear_pointer (&image_undo->parasite, clay_parasite_free);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
