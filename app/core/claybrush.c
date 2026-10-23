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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "clay.h"
#include "claybezierdesc.h"
#include "claybrush.h"
#include "claybrush-boundary.h"
#include "claybrush-load.h"
#include "claybrush-mipmap.h"
#include "claybrush-private.h"
#include "claybrush-save.h"
#include "claybrush-transform.h"
#include "claybrushcache.h"
#include "claybrushgenerated.h"
#include "claybrushpipe.h"
#include "claycontext.h"
#include "claytagged.h"
#include "claytempbuf.h"

#include "clay-intl.h"


enum
{
  SPACING_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SPACING
};


static void          clay_brush_tagged_iface_init     (ClayTaggedInterface  *iface);

static void          clay_brush_finalize              (GObject              *object);
static void          clay_brush_set_property          (GObject              *object,
                                                       guint                 property_id,
                                                       const GValue         *value,
                                                       GParamSpec           *pspec);
static void          clay_brush_get_property          (GObject              *object,
                                                       guint                 property_id,
                                                       GValue               *value,
                                                       GParamSpec           *pspec);

static gint64        clay_brush_get_memsize           (ClayObject           *object,
                                                       gint64               *gui_size);

static gboolean      clay_brush_get_size              (ClayViewable         *viewable,
                                                       gint                 *width,
                                                       gint                 *height);
static ClayTempBuf * clay_brush_get_new_preview       (ClayViewable         *viewable,
                                                       ClayContext          *context,
                                                       gint                  width,
                                                       gint                  height,
                                                       gint                  scale_factor,
                                                       GeglColor            *fg_color);
static gchar       * clay_brush_get_description       (ClayViewable         *viewable,
                                                       gchar               **tooltip);

static void          clay_brush_dirty                 (ClayData             *data);
static const gchar * clay_brush_get_extension         (ClayData             *data);
static void          clay_brush_copy                  (ClayData             *data,
                                                       ClayData             *src_data);

static void          clay_brush_real_begin_use        (ClayBrush            *brush);
static void          clay_brush_real_end_use          (ClayBrush            *brush);
static ClayBrush   * clay_brush_real_select_brush     (ClayBrush            *brush,
                                                       const ClayCoords     *last_coords,
                                                       const ClayCoords     *current_coords);
static gboolean      clay_brush_real_want_null_motion (ClayBrush            *brush,
                                                       const ClayCoords     *last_coords,
                                                       const ClayCoords     *current_coords);

static gchar       * clay_brush_get_checksum          (ClayTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (ClayBrush, clay_brush, CLAY_TYPE_DATA,
                         G_ADD_PRIVATE (ClayBrush)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_TAGGED,
                                                clay_brush_tagged_iface_init))

#define parent_class clay_brush_parent_class

static guint brush_signals[LAST_SIGNAL] = { 0 };


static void
clay_brush_class_init (ClayBrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayDataClass     *data_class        = CLAY_DATA_CLASS (klass);

  brush_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayBrushClass, spacing_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = clay_brush_finalize;
  object_class->get_property        = clay_brush_get_property;
  object_class->set_property        = clay_brush_set_property;

  clay_object_class->get_memsize    = clay_brush_get_memsize;

  viewable_class->default_icon_name = "clay-tool-paintbrush";
  viewable_class->default_name      = _("Brush");
  viewable_class->get_size          = clay_brush_get_size;
  viewable_class->get_new_preview   = clay_brush_get_new_preview;
  viewable_class->get_description   = clay_brush_get_description;

  data_class->dirty                 = clay_brush_dirty;
  data_class->save                  = clay_brush_save;
  data_class->get_extension         = clay_brush_get_extension;
  data_class->copy                  = clay_brush_copy;

  klass->begin_use                  = clay_brush_real_begin_use;
  klass->end_use                    = clay_brush_real_end_use;
  klass->select_brush               = clay_brush_real_select_brush;
  klass->want_null_motion           = clay_brush_real_want_null_motion;
  klass->transform_size             = clay_brush_real_transform_size;
  klass->transform_mask             = clay_brush_real_transform_mask;
  klass->transform_pixmap           = clay_brush_real_transform_pixmap;
  klass->transform_boundary         = clay_brush_real_transform_boundary;
  klass->spacing_changed            = NULL;

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_double ("spacing", NULL,
                                                        _("Brush Spacing"),
                                                        1.0, 5000.0, 20.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_brush_tagged_iface_init (ClayTaggedInterface *iface)
{
  iface->get_checksum = clay_brush_get_checksum;
}

static void
clay_brush_init (ClayBrush *brush)
{
  brush->priv = clay_brush_get_instance_private (brush);

  brush->priv->spacing  = 20;
  brush->priv->x_axis.x = 15.0;
  brush->priv->x_axis.y =  0.0;
  brush->priv->y_axis.x =  0.0;
  brush->priv->y_axis.y = 15.0;

  brush->priv->blur_hardness = 1.0;
}

static void
clay_brush_finalize (GObject *object)
{
  ClayBrush *brush = CLAY_BRUSH (object);

  g_clear_pointer (&brush->priv->mask,          clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->pixmap,        clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_mask,   clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, clay_temp_buf_unref);

  clay_brush_mipmap_clear (brush);

  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_brush_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ClayBrush *brush = CLAY_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      clay_brush_set_spacing (brush, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_brush_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayBrush *brush = CLAY_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      g_value_set_double (value, clay_brush_get_spacing (brush));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_brush_get_memsize (ClayObject *object,
                        gint64     *gui_size)
{
  ClayBrush *brush   = CLAY_BRUSH (object);
  gint64     memsize = 0;

  memsize += clay_temp_buf_get_memsize (brush->priv->mask);
  memsize += clay_temp_buf_get_memsize (brush->priv->pixmap);

  memsize += clay_brush_mipmap_get_memsize (brush);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_brush_get_size (ClayViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  ClayBrush *brush = CLAY_BRUSH (viewable);

  *width  = clay_temp_buf_get_width  (brush->priv->mask);
  *height = clay_temp_buf_get_height (brush->priv->mask);

  return TRUE;
}

static ClayTempBuf *
clay_brush_get_new_preview (ClayViewable *viewable,
                            ClayContext  *context,
                            gint          width,
                            gint          height,
                            gint          scale_factor,
                            GeglColor    *fg_color)
{
  ClayBrush         *brush       = CLAY_BRUSH (viewable);
  const ClayTempBuf *mask_buf    = brush->priv->mask;
  const ClayTempBuf *pixmap_buf  = brush->priv->pixmap;
  ClayTempBuf       *return_buf  = NULL;
  gint               mask_width;
  gint               mask_height;
  guchar            *mask_data;
  guchar            *mask;
  guchar            *buf;
  gint               x, y;
  gboolean           free_mask = FALSE;
  gdouble            ratio_x;
  gdouble            ratio_y;
  gdouble            scale;

  width  *= scale_factor;
  height *= scale_factor;

  mask_width  = clay_temp_buf_get_width  (mask_buf);
  mask_height = clay_temp_buf_get_height (mask_buf);

  ratio_x = (gdouble) width  / (gdouble) mask_width;
  ratio_y = (gdouble) height / (gdouble) mask_height;

  scale = MIN (ratio_x, ratio_y);

  if (mask_width < width && mask_height < height)
    {
      scale = MIN (scale_factor, scale);
    }

  if (CLAY_IS_BRUSH_GENERATED (brush) || scale != 1.0)
    {
      clay_brush_begin_use (brush);

      if (CLAY_IS_BRUSH_GENERATED (brush))
        {
          ClayBrushGenerated *gen_brush = CLAY_BRUSH_GENERATED (brush);

          mask_buf = clay_brush_transform_mask (brush, scale,
                                                (clay_brush_generated_get_aspect_ratio (gen_brush) - 1.0) * 20.0 / 19.0,
                                                clay_brush_generated_get_angle (gen_brush) / -360.0,
                                                FALSE,
                                                clay_brush_generated_get_hardness (gen_brush));
        }
      else
        {
          mask_buf = clay_brush_transform_mask (brush, scale, 0.0, 0.0, FALSE, 1.0);
        }

      if (! mask_buf)
        {
          mask_buf = clay_temp_buf_new (1, 1, babl_format ("Y u8"));
          clay_temp_buf_data_clear ((ClayTempBuf *) mask_buf);
        }
      else
        {
          clay_temp_buf_ref ((ClayTempBuf *) mask_buf);
        }

      if (pixmap_buf)
        pixmap_buf = clay_brush_transform_pixmap (brush, scale,
                                                  0.0, 0.0, FALSE, 1.0);

      mask_width  = clay_temp_buf_get_width  (mask_buf);
      mask_height = clay_temp_buf_get_height (mask_buf);

      free_mask = TRUE;
    }

  return_buf = clay_temp_buf_new (mask_width, mask_height,
                                  babl_format ("R'G'B'A u8"));

  mask = mask_data = clay_temp_buf_lock (mask_buf, babl_format ("Y u8"),
                                         GEGL_ACCESS_READ);
  buf  = clay_temp_buf_get_data (return_buf);

  if (pixmap_buf)
    {
      guchar *pixmap_data;
      guchar *pixmap;

      pixmap = pixmap_data = clay_temp_buf_lock (pixmap_buf,
                                                 babl_format ("R'G'B' u8"),
                                                 GEGL_ACCESS_READ);

      for (y = 0; y < mask_height; y++)
        {
          for (x = 0; x < mask_width ; x++)
            {
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *mask++;
            }
        }

      clay_temp_buf_unlock (pixmap_buf, pixmap_data);
    }
  else
    {
      guint8 rgb[3] = { 0, 0, 0 };

      if (fg_color)
        gegl_color_get_pixel (fg_color, babl_format ("R'G'B' u8"), rgb);

      for (y = 0; y < mask_height; y++)
        {
          for (x = 0; x < mask_width ; x++)
            {
              *buf++ = rgb[0];
              *buf++ = rgb[1];
              *buf++ = rgb[2];
              *buf++ = *mask++;
            }
        }
    }

  clay_temp_buf_unlock (mask_buf, mask_data);

  if (free_mask)
    {
      clay_temp_buf_unref ((ClayTempBuf *) mask_buf);

      clay_brush_end_use (brush);
    }

  return return_buf;
}

static gchar *
clay_brush_get_description (ClayViewable  *viewable,
                            gchar        **tooltip)
{
  ClayBrush *brush = CLAY_BRUSH (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          clay_object_get_name (brush),
                          clay_temp_buf_get_width  (brush->priv->mask),
                          clay_temp_buf_get_height (brush->priv->mask));
}

static void
clay_brush_dirty (ClayData *data)
{
  ClayBrush *brush = CLAY_BRUSH (data);

  if (brush->priv->mask_cache)
    clay_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    clay_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    clay_brush_cache_clear (brush->priv->boundary_cache);

  clay_brush_mipmap_clear (brush);

  g_clear_pointer (&brush->priv->blurred_mask,   clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, clay_temp_buf_unref);

  CLAY_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
clay_brush_get_extension (ClayData *data)
{
  return CLAY_BRUSH_FILE_EXTENSION;
}

static void
clay_brush_copy (ClayData *data,
                 ClayData *src_data)
{
  ClayBrush *brush     = CLAY_BRUSH (data);
  ClayBrush *src_brush = CLAY_BRUSH (src_data);

  g_clear_pointer (&brush->priv->mask, clay_temp_buf_unref);
  if (src_brush->priv->mask)
    brush->priv->mask = clay_temp_buf_copy (src_brush->priv->mask);

  g_clear_pointer (&brush->priv->pixmap, clay_temp_buf_unref);
  if (src_brush->priv->pixmap)
    brush->priv->pixmap = clay_temp_buf_copy (src_brush->priv->pixmap);

  brush->priv->spacing = src_brush->priv->spacing;
  brush->priv->x_axis  = src_brush->priv->x_axis;
  brush->priv->y_axis  = src_brush->priv->y_axis;

  clay_data_dirty (data);
}

static void
clay_brush_real_begin_use (ClayBrush *brush)
{
  brush->priv->mask_cache =
    clay_brush_cache_new ((GDestroyNotify) clay_temp_buf_unref, 'M', 'm');

  brush->priv->pixmap_cache =
    clay_brush_cache_new ((GDestroyNotify) clay_temp_buf_unref, 'P', 'p');

  brush->priv->boundary_cache =
    clay_brush_cache_new ((GDestroyNotify) clay_bezier_desc_free, 'B', 'b');
}

static void
clay_brush_real_end_use (ClayBrush *brush)
{
  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  g_clear_pointer (&brush->priv->blurred_mask,   clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, clay_temp_buf_unref);
}

static ClayBrush *
clay_brush_real_select_brush (ClayBrush        *brush,
                              const ClayCoords *last_coords,
                              const ClayCoords *current_coords)
{
  return brush;
}

static gboolean
clay_brush_real_want_null_motion (ClayBrush        *brush,
                                  const ClayCoords *last_coords,
                                  const ClayCoords *current_coords)
{
  return TRUE;
}

static gchar *
clay_brush_get_checksum (ClayTagged *tagged)
{
  ClayBrush *brush           = CLAY_BRUSH (tagged);
  gchar     *checksum_string = NULL;

  if (brush->priv->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum,
                         clay_temp_buf_get_data (brush->priv->mask),
                         clay_temp_buf_get_data_size (brush->priv->mask));
      if (brush->priv->pixmap)
        g_checksum_update (checksum,
                           clay_temp_buf_get_data (brush->priv->pixmap),
                           clay_temp_buf_get_data_size (brush->priv->pixmap));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->spacing,
                         sizeof (brush->priv->spacing));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->x_axis,
                         sizeof (brush->priv->x_axis));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->y_axis,
                         sizeof (brush->priv->y_axis));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

/*  public functions  */

ClayData *
clay_brush_new (ClayContext *context,
                const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return clay_brush_generated_new (name,
                                   CLAY_BRUSH_GENERATED_CIRCLE,
                                   5.0, 2, 0.5, 1.0, 0.0);
}

ClayData *
clay_brush_get_standard (ClayContext *context)
{
  static ClayData *standard_brush = NULL;

  if (! standard_brush)
    {
      g_set_weak_pointer (&standard_brush,
                          clay_brush_new (context, "Standard"));

      clay_data_clean (standard_brush);
      clay_data_make_internal (standard_brush, "clay-brush-standard");
    }

  return standard_brush;
}

void
clay_brush_begin_use (ClayBrush *brush)
{
  g_return_if_fail (CLAY_IS_BRUSH (brush));

  brush->priv->use_count++;

  if (brush->priv->use_count == 1)
    CLAY_BRUSH_GET_CLASS (brush)->begin_use (brush);
}

void
clay_brush_end_use (ClayBrush *brush)
{
  g_return_if_fail (CLAY_IS_BRUSH (brush));
  g_return_if_fail (brush->priv->use_count > 0);

  brush->priv->use_count--;

  if (brush->priv->use_count == 0)
    CLAY_BRUSH_GET_CLASS (brush)->end_use (brush);
}

ClayBrush *
clay_brush_select_brush (ClayBrush        *brush,
                         const ClayCoords *last_coords,
                         const ClayCoords *current_coords)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (current_coords != NULL, NULL);

  return CLAY_BRUSH_GET_CLASS (brush)->select_brush (brush,
                                                     last_coords,
                                                     current_coords);
}

gboolean
clay_brush_want_null_motion (ClayBrush        *brush,
                             const ClayCoords *last_coords,
                             const ClayCoords *current_coords)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (current_coords != NULL, FALSE);

  return CLAY_BRUSH_GET_CLASS (brush)->want_null_motion (brush,
                                                         last_coords,
                                                         current_coords);
}

void
clay_brush_transform_size (ClayBrush     *brush,
                           gdouble        scale,
                           gdouble        aspect_ratio,
                           gdouble        angle,
                           gboolean       reflect,
                           gint          *width,
                           gint          *height)
{
  g_return_if_fail (CLAY_IS_BRUSH (brush));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  if (scale             == 1.0 &&
      aspect_ratio      == 0.0 &&
      fmod (angle, 0.5) == 0.0)
    {
      *width  = clay_temp_buf_get_width  (brush->priv->mask);
      *height = clay_temp_buf_get_height (brush->priv->mask);

      return;
    }

  CLAY_BRUSH_GET_CLASS (brush)->transform_size (brush,
                                                scale, aspect_ratio, angle, reflect,
                                                width, height);
}

const ClayTempBuf *
clay_brush_transform_mask (ClayBrush *brush,
                           gdouble    scale,
                           gdouble    aspect_ratio,
                           gdouble    angle,
                           gboolean   reflect,
                           gdouble    hardness)
{
  const ClayTempBuf *mask;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  clay_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  mask = clay_brush_cache_get (brush->priv->mask_cache,
                               width, height,
                               scale, aspect_ratio, angle, reflect, hardness);

  if (! mask)
    {
#if 0
      /* This code makes sure that brushes using blur for hardness
       * (all of them but generated) are blurred once and no more.
       * It also makes hardnes dynamics not work for these brushes.
       * This is intentional. Confoliving for each stamp is too expensive.*/
      if (! brush->priv->blurred_mask &&
          ! CLAY_IS_BRUSH_GENERATED(brush) &&
          ! CLAY_IS_BRUSH_PIPE(brush) && /*Can't cache pipes. Sanely anyway*/
          hardness < 1.0)
        {
           brush->priv->blurred_mask = CLAY_BRUSH_GET_CLASS (brush)->transform_mask (brush,
                                                             1.0,
                                                             0.0,
                                                             0.0,
                                                             FALSE,
                                                             hardness);
           brush->priv->blur_hardness = hardness;
        }

      if (brush->priv->blurred_mask)
        {
          effective_hardness = 1.0; /*Hardness has already been applied*/
        }
#endif

      mask = CLAY_BRUSH_GET_CLASS (brush)->transform_mask (brush,
                                                           scale,
                                                           aspect_ratio,
                                                           angle,
                                                           reflect,
                                                           effective_hardness);

      clay_brush_cache_add (brush->priv->mask_cache,
                            (gpointer) mask,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return mask;
}

const ClayTempBuf *
clay_brush_transform_pixmap (ClayBrush *brush,
                             gdouble    scale,
                             gdouble    aspect_ratio,
                             gdouble    angle,
                             gboolean   reflect,
                             gdouble    hardness)
{
  const ClayTempBuf *pixmap;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (brush->priv->pixmap != NULL, NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  clay_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  pixmap = clay_brush_cache_get (brush->priv->pixmap_cache,
                                 width, height,
                                 scale, aspect_ratio, angle, reflect, hardness);

  if (! pixmap)
    {
#if 0
     if (! brush->priv->blurred_pixmap &&
         ! CLAY_IS_BRUSH_GENERATED(brush) &&
         ! CLAY_IS_BRUSH_PIPE(brush) /*Can't cache pipes. Sanely anyway*/
         && hardness < 1.0)
      {
         brush->priv->blurred_pixmap = CLAY_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
                                                                  1.0,
                                                                  0.0,
                                                                  0.0,
                                                                  FALSE,
                                                                  hardness);
         brush->priv->blur_hardness = hardness;
       }

      if (brush->priv->blurred_pixmap) {
        effective_hardness = 1.0; /*Hardness has already been applied*/
      }
#endif

      pixmap = CLAY_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
                                                               scale,
                                                               aspect_ratio,
                                                               angle,
                                                               reflect,
                                                               effective_hardness);

      clay_brush_cache_add (brush->priv->pixmap_cache,
                            (gpointer) pixmap,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return pixmap;
}

const ClayBezierDesc *
clay_brush_transform_boundary (ClayBrush *brush,
                               gdouble    scale,
                               gdouble    aspect_ratio,
                               gdouble    angle,
                               gboolean   reflect,
                               gdouble    hardness,
                               gint      *width,
                               gint      *height)
{
  const ClayBezierDesc *boundary;

  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  clay_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             width, height);

  boundary = clay_brush_cache_get (brush->priv->boundary_cache,
                                   *width, *height,
                                   scale, aspect_ratio, angle, reflect, hardness);

  if (! boundary)
    {
      boundary = CLAY_BRUSH_GET_CLASS (brush)->transform_boundary (brush,
                                                                   scale,
                                                                   aspect_ratio,
                                                                   angle,
                                                                   reflect,
                                                                   hardness,
                                                                   width,
                                                                   height);

      /*  while the brush mask is always at least 1x1 pixels, its
       *  outline can correctly be NULL
       *
       *  FIXME: make the cache handle NULL things when it is
       *         properly implemented
       */
      if (boundary)
        clay_brush_cache_add (brush->priv->boundary_cache,
                              (gpointer) boundary,
                              *width, *height,
                              scale, aspect_ratio, angle, reflect, hardness);
    }

  return boundary;
}

ClayTempBuf *
clay_brush_get_mask (ClayBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);

  if (brush->priv->blurred_mask)
    {
      return brush->priv->blurred_mask;
    }
  return brush->priv->mask;
}

ClayTempBuf *
clay_brush_get_pixmap (ClayBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);

  if(brush->priv->blurred_pixmap)
    {
      return brush->priv->blurred_pixmap;
    }
  return brush->priv->pixmap;
}

void
clay_brush_flush_blur_caches (ClayBrush *brush)
{
#if 0
  g_clear_pointer (&brush->priv->blurred_mask,   clay_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, clay_temp_buf_unref);

  if (brush->priv->mask_cache)
    clay_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    clay_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    clay_brush_cache_clear (brush->priv->boundary_cache);
#endif
}

gdouble
clay_brush_get_blur_hardness (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), 0);

  return brush->priv->blur_hardness;
}

gint
clay_brush_get_width (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return clay_temp_buf_get_width (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return clay_temp_buf_get_width (brush->priv->blurred_pixmap);

  return clay_temp_buf_get_width (brush->priv->mask);
}

gint
clay_brush_get_height (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return clay_temp_buf_get_height (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return clay_temp_buf_get_height (brush->priv->blurred_pixmap);

  return clay_temp_buf_get_height (brush->priv->mask);
}

gint
clay_brush_get_spacing (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), 0);

  return brush->priv->spacing;
}

void
clay_brush_set_spacing (ClayBrush *brush,
                        gint       spacing)
{
  g_return_if_fail (CLAY_IS_BRUSH (brush));

  if (brush->priv->spacing != spacing)
    {
      brush->priv->spacing = spacing;

      g_signal_emit (brush, brush_signals[SPACING_CHANGED], 0);
      g_object_notify (G_OBJECT (brush), "spacing");
    }
}

static const ClayVector2 fail = { 0.0, 0.0 };

ClayVector2
clay_brush_get_x_axis (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), fail);

  return brush->priv->x_axis;
}

ClayVector2
clay_brush_get_y_axis (ClayBrush *brush)
{
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), fail);

  return brush->priv->y_axis;
}
