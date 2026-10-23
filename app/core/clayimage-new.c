/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "claybuffer.h"
#include "claychannel.h"
#include "claycontext.h"
#include "claydrawable-fill.h"
#include "claygrouplayer.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "clayimage-metadata.h"
#include "clayimage-new.h"
#include "clayimage-undo.h"
#include "claylayer.h"
#include "claylayer-new.h"
#include "claylist.h"
#include "claytemplate.h"

#include "clay-intl.h"


static void   clay_image_new_add_creation_metadata (ClayImage *image);


/*  public functions  */

ClayTemplate *
clay_image_new_get_last_template (Clay      *clay,
                                  ClayImage *image)
{
  ClayTemplate *template;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), NULL);

  template = clay_template_new ("image new values");

  if (image)
    {
      const gchar *comment;

      comment = clay_template_get_comment (clay->config->default_image);

      clay_config_sync (G_OBJECT (clay->config->default_image),
                        G_OBJECT (template), 0);
      clay_template_set_from_image (template, image);

      /* Do not pass around the comment from the current active comment. Only
       * pass comments stored in actual templates. This can be even considered
       * as data leak otherwise (creating 2 images in a row and not realizing
       * the second will have the metadata comment from the first). See #11384.
       */
      g_object_set (template, "comment", comment, NULL);
    }
  else
    {
      ClayTemplate *last_template;

      last_template = clay_get_last_template (clay);

      clay_config_sync (G_OBJECT (last_template),
                        G_OBJECT (template), 0);
    }

  return template;
}

void
clay_image_new_set_last_template (Clay         *clay,
                                  ClayTemplate *template)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_TEMPLATE (template));

  clay_set_last_template (clay, template);
}

ClayImage *
clay_image_new_from_template (Clay         *clay,
                              ClayTemplate *template,
                              ClayContext  *context)
{
  ClayImage               *image;
  ClayLayer               *layer;
  ClayColorProfile        *profile;
  ClayColorRenderingIntent intent;
  gboolean                 bpc;
  gint                     width, height;
  gboolean                 has_alpha;
  const gchar             *comment;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  image = clay_create_image (clay,
                             clay_template_get_width (template),
                             clay_template_get_height (template),
                             clay_template_get_base_type (template),
                             clay_template_get_precision (template),
                             FALSE);
  clay_context_set_image (context, image);

  clay_image_undo_disable (image);

  comment = clay_template_get_comment (template);

  if (comment)
    {
      ClayParasite *parasite;

      parasite = clay_parasite_new ("clay-comment",
                                    CLAY_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      clay_image_parasite_attach (image, parasite, FALSE);
      clay_parasite_free (parasite);
    }

  clay_image_set_resolution (image,
                             clay_template_get_resolution_x (template),
                             clay_template_get_resolution_y (template));
  clay_image_set_unit (image, clay_template_get_resolution_unit (template));

  profile = clay_template_get_color_profile (template);
  clay_image_set_color_profile (image, profile, NULL);
  if (profile)
    g_object_unref (profile);

  profile = clay_template_get_simulation_profile (template);
  clay_image_set_simulation_profile (image, profile);
  if (profile)
    g_object_unref (profile);

  intent = clay_template_get_simulation_intent (template);
  clay_image_set_simulation_intent (image, intent);
  bpc = clay_template_get_simulation_bpc (template);
  clay_image_set_simulation_bpc (image, bpc);

  width  = clay_image_get_width (image);
  height = clay_image_get_height (image);

  if (clay_template_get_fill_type (template) == CLAY_FILL_TRANSPARENT)
    has_alpha = TRUE;
  else
    has_alpha = FALSE;

  layer = clay_layer_new (image, width, height,
                          clay_image_get_layer_format (image, has_alpha),
                          _("Background"),
                          CLAY_OPACITY_OPAQUE,
                          clay_image_get_default_new_layer_mode (image));

  clay_drawable_fill (CLAY_DRAWABLE (layer),
                      context, clay_template_get_fill_type (template));

  clay_image_add_layer (image, layer, NULL, 0, FALSE);

  clay_image_new_add_creation_metadata (image);

  clay_image_undo_enable (image);
  clay_image_clean_all (image);

  return image;
}

ClayImage *
clay_image_new_from_drawable (Clay         *clay,
                              ClayDrawable *drawable)
{
  ClayItem          *item;
  ClayImage         *image;
  ClayImage         *new_image;
  ClayLayer         *new_layer;
  GType              new_type;
  gint               off_x, off_y;
  ClayImageBaseType  type;
  gdouble            xres;
  gdouble            yres;
  ClayColorProfile  *profile;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  item  = CLAY_ITEM (drawable);
  image = clay_item_get_image (item);

  type = clay_drawable_get_base_type (drawable);

  new_image = clay_create_image (clay,
                                 clay_item_get_width  (item),
                                 clay_item_get_height (item),
                                 type,
                                 clay_drawable_get_precision (drawable),
                                 TRUE);
  clay_image_undo_disable (new_image);

  if (type == CLAY_INDEXED)
    clay_image_set_colormap_palette (new_image,
                                     clay_image_get_colormap_palette (image),
                                     FALSE);

  clay_image_get_resolution (image, &xres, &yres);
  clay_image_set_resolution (new_image, xres, yres);
  clay_image_set_unit (new_image, clay_image_get_unit (image));

  profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (drawable));
  clay_image_set_color_profile (new_image, profile, NULL);

  if (CLAY_IS_LAYER (drawable))
    new_type = G_TYPE_FROM_INSTANCE (drawable);
  else
    new_type = CLAY_TYPE_LAYER;

  new_layer = CLAY_LAYER (clay_item_convert (CLAY_ITEM (drawable),
                                             new_image, new_type));

  clay_object_set_name (CLAY_OBJECT (new_layer),
                        clay_object_get_name (drawable));

  clay_item_get_offset (CLAY_ITEM (new_layer), &off_x, &off_y);
  clay_item_translate (CLAY_ITEM (new_layer), -off_x, -off_y, FALSE);
  clay_item_set_visible (CLAY_ITEM (new_layer), TRUE, FALSE);
  clay_layer_set_mode (new_layer,
                       clay_image_get_default_new_layer_mode (new_image),
                       FALSE);
  clay_layer_set_opacity (new_layer, CLAY_OPACITY_OPAQUE, FALSE);
  if (clay_layer_can_lock_alpha (new_layer))
    clay_layer_set_lock_alpha (new_layer, FALSE, FALSE);

  clay_image_add_layer (new_image, new_layer, NULL, 0, TRUE);

  clay_image_new_add_creation_metadata (new_image);

  clay_image_undo_enable (new_image);

  return new_image;
}

/**
 * clay_image_new_copy_drawables:
 * @image: the image where @drawables belong to.
 * @drawables: the drawables to copy into @new_image.
 * @new_image: the image to insert to.
 * @tag_copies: tag copies of @drawable with "clay-image-copied-layer".
 * @copied_drawables:
 * @tagged_drawables:
 * @parent:
 * @new_parent:
 *
 * This recursive function will create copies of all @drawables
 * belonging to the same @image, and will insert them into @new_image
 * with the same layer order and hierarchy (adding layer groups when
 * needed).
 * If a single drawable is selected, it will be copied visible, with
 * full opacity and default layer mode. Otherwise, visibility, opacity
 * and layer mode will be copied as-is, allowing proper compositing.
 *
 * The @copied_drawables, @tagged_drawables, @parent and @new_parent arguments
 * are only used internally for recursive calls and must be set to NULL for the
 * initial call.
 */
static void
clay_image_new_copy_drawables (ClayImage *image,
                               GList     *drawables,
                               ClayImage *new_image,
                               gboolean   tag_copies,
                               GList     *copied_drawables,
                               GList     *tagged_drawables,
                               ClayLayer *parent,
                               ClayLayer *new_parent)
{
  GList *layers;
  GList *iter;
  gint   n_drawables;
  gint   index;

  n_drawables = g_list_length (drawables);
  if (parent == NULL)
    {
      /* Root layers. */
      layers = clay_image_get_layer_iter (image);

      copied_drawables = g_list_copy (drawables);
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          /* Tagged drawables are the explicitly copied drawables which have no
           * explicitly copied descendant items.
           */
          GList *iter2;

          for (iter2 = iter; iter2; iter2 = iter2->next)
            if (clay_viewable_is_ancestor (iter->data, iter2->data))
              break;

          if (iter2 == NULL)
            tagged_drawables = g_list_prepend (tagged_drawables, iter->data);
        }

      /* Add any item parent. */
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          ClayItem *item = iter->data;
          while ((item = clay_item_get_parent (item)))
            if (! g_list_find (copied_drawables, item))
              copied_drawables = g_list_prepend (copied_drawables, item);
        }
    }
  else
    {
      ClayContainer *container;

      container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
      layers = CLAY_LIST (container)->queue->head;
    }

  index = 0;
  for (iter = layers; iter; iter = iter->next)
    {
      if (g_list_find (copied_drawables, iter->data))
        {
          ClayLayer *new_layer;
          GType      new_type;
          gboolean   is_group;
          gboolean   is_tagged;

          if (CLAY_IS_LAYER (iter->data))
            new_type = G_TYPE_FROM_INSTANCE (iter->data);
          else
            new_type = CLAY_TYPE_LAYER;

          is_group  = (clay_viewable_get_children (iter->data) != NULL);
          is_tagged = (g_list_find (tagged_drawables, iter->data) != NULL);

          if (is_group && ! is_tagged)
            new_layer = clay_group_layer_new (new_image);
          else
            new_layer = CLAY_LAYER (clay_item_convert (CLAY_ITEM (iter->data),
                                                       new_image, new_type));

          if (tag_copies && is_tagged)
            g_object_set_data (G_OBJECT (new_layer),
                               "clay-image-copied-layer",
                               GINT_TO_POINTER (TRUE));

          clay_object_set_name (CLAY_OBJECT (new_layer),
                                clay_object_get_name (iter->data));

          /* Visibility, mode and opacity mimic the source image if
           * multiple items are copied. Otherwise we just set them to
           * defaults.
           */
          clay_item_set_visible (CLAY_ITEM (new_layer),
                                 n_drawables > 1 ?
                                 clay_item_get_visible (iter->data) : TRUE,
                                 FALSE);
          clay_layer_set_mode (new_layer,
                               n_drawables > 1 && CLAY_IS_LAYER (iter->data) ?
                               clay_layer_get_mode (iter->data) :
                               clay_image_get_default_new_layer_mode (new_image),
                               FALSE);
          clay_layer_set_opacity (new_layer,
                                  n_drawables > 1 && CLAY_IS_LAYER (iter->data) ?
                                  clay_layer_get_opacity (iter->data) : CLAY_OPACITY_OPAQUE, FALSE);

          if (clay_layer_can_lock_alpha (new_layer))
            clay_layer_set_lock_alpha (new_layer, FALSE, FALSE);

          clay_image_add_layer (new_image, new_layer, new_parent, index++, TRUE);

          /* If a group, loop through children. */
          if (is_group && ! is_tagged)
            clay_image_new_copy_drawables (image, drawables, new_image, tag_copies,
                                           copied_drawables, tagged_drawables,
                                           iter->data, new_layer);
        }
    }

  if (parent == NULL)
    {
      g_list_free (copied_drawables);
      g_list_free (tagged_drawables);
    }
}

ClayImage *
clay_image_new_from_drawables (Clay     *clay,
                               GList    *drawables,
                               gboolean  copy_selection,
                               gboolean  tag_copies)
{
  ClayImage         *image = NULL;
  ClayImage         *new_image;
  GList             *iter;
  ClayImageBaseType  type;
  ClayPrecision      precision;
  gdouble            xres;
  gdouble            yres;
  ClayColorProfile  *profile = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), NULL);

      if (iter == drawables)
        image = clay_item_get_image (iter->data);
      else
        /* We only accept list of drawables for a same origin image. */
        g_return_val_if_fail (clay_item_get_image (iter->data) == image, NULL);
    }

  type      = clay_drawable_get_base_type (drawables->data);
  precision = clay_drawable_get_precision (drawables->data);
  profile   = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (drawables->data));

  new_image = clay_create_image (clay,
                                 clay_image_get_width  (image),
                                 clay_image_get_height (image),
                                 type, precision,
                                 TRUE);
  clay_image_undo_disable (new_image);

  if (type == CLAY_INDEXED)
    clay_image_set_colormap_palette (new_image,
                                     clay_image_get_colormap_palette (image),
                                     FALSE);

  clay_image_get_resolution (image, &xres, &yres);
  clay_image_set_resolution (new_image, xres, yres);
  clay_image_set_unit (new_image, clay_image_get_unit (image));
  if (profile)
    clay_image_set_color_profile (new_image, profile, NULL);

  if (copy_selection)
    {
      ClayChannel *selection;

      selection = clay_image_get_mask (image);
      if (! clay_channel_is_empty (selection))
        {
          ClayChannel *new_selection;
          GeglBuffer  *buffer;

          new_selection = clay_image_get_mask (new_image);
          buffer = clay_gegl_buffer_dup (clay_drawable_get_buffer (CLAY_DRAWABLE (selection)),
                                         CLAY_DRAWABLE (new_selection));
          clay_drawable_set_buffer (CLAY_DRAWABLE (new_selection),
                                    FALSE, NULL, buffer);
          g_object_unref (buffer);
        }
    }

  clay_image_new_copy_drawables (image, drawables, new_image, tag_copies, NULL, NULL, NULL, NULL);

  clay_image_new_add_creation_metadata (new_image);

  clay_image_undo_enable (new_image);

  return new_image;
}

ClayImage *
clay_image_new_from_component (Clay            *clay,
                               ClayImage       *image,
                               ClayChannelType  component)
{
  ClayImage   *new_image;
  ClayChannel *channel;
  ClayLayer   *layer;
  const gchar *desc;
  gdouble      xres;
  gdouble      yres;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  new_image = clay_create_image (clay,
                                 clay_image_get_width  (image),
                                 clay_image_get_height (image),
                                 CLAY_GRAY,
                                 clay_image_get_precision (image),
                                 TRUE);

  clay_image_undo_disable (new_image);

  clay_image_get_resolution (image, &xres, &yres);
  clay_image_set_resolution (new_image, xres, yres);
  clay_image_set_unit (new_image, clay_image_get_unit (image));

  channel = clay_channel_new_from_component (image, component, NULL, NULL);

  layer = CLAY_LAYER (clay_item_convert (CLAY_ITEM (channel),
                                         new_image, CLAY_TYPE_LAYER));
  g_object_unref (channel);

  clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  clay_object_take_name (CLAY_OBJECT (layer),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  clay_image_add_layer (new_image, layer, NULL, 0, TRUE);

  clay_image_new_add_creation_metadata (new_image);

  clay_image_undo_enable (new_image);

  return new_image;
}

ClayImage *
clay_image_new_from_buffer (Clay       *clay,
                            ClayBuffer *buffer)
{
  ClayImage        *image;
  ClayLayer        *layer;
  const Babl       *format;
  gboolean          has_alpha;
  gdouble           res_x;
  gdouble           res_y;
  ClayColorProfile *profile;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), NULL);

  format    = clay_buffer_get_format (buffer);
  has_alpha = babl_format_has_alpha (format);

  image = clay_create_image (clay,
                             clay_buffer_get_width  (buffer),
                             clay_buffer_get_height (buffer),
                             clay_babl_format_get_base_type (format),
                             clay_babl_format_get_precision (format),
                             TRUE);
  clay_image_undo_disable (image);

  if (clay_buffer_get_resolution (buffer, &res_x, &res_y))
    {
      clay_image_set_resolution (image, res_x, res_y);
      clay_image_set_unit (image, clay_buffer_get_unit (buffer));
    }

  profile = clay_buffer_get_color_profile (buffer);
  clay_image_set_color_profile (image, profile, NULL);

  layer = clay_layer_new_from_buffer (buffer, image,
                                      clay_image_get_layer_format (image,
                                                                   has_alpha),
                                      _("Pasted Layer"),
                                      CLAY_OPACITY_OPAQUE,
                                      clay_image_get_default_new_layer_mode (image));

  clay_image_add_layer (image, layer, NULL, 0, TRUE);

  clay_image_new_add_creation_metadata (image);

  clay_image_undo_enable (image);

  return image;
}

ClayImage *
clay_image_new_from_pixbuf (Clay        *clay,
                            GdkPixbuf   *pixbuf,
                            const gchar *layer_name)
{
  ClayImage         *new_image;
  ClayLayer         *layer;
  ClayImageBaseType  base_type;
  gboolean           has_alpha = FALSE;
  guint8            *icc_data;
  gsize              icc_len;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 2: has_alpha = TRUE;
    case 1: base_type = CLAY_GRAY;
      break;

    case 4: has_alpha = TRUE;
    case 3: base_type = CLAY_RGB;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  new_image = clay_create_image (clay,
                                 gdk_pixbuf_get_width  (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 base_type,
                                 CLAY_PRECISION_U8_NON_LINEAR,
                                 FALSE);

  clay_image_undo_disable (new_image);

  icc_data = clay_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      clay_image_set_icc_profile (new_image, icc_data, icc_len,
                                  CLAY_ICC_PROFILE_PARASITE_NAME,
                                  NULL);
      g_free (icc_data);
    }

  layer = clay_layer_new_from_pixbuf (pixbuf, new_image,
                                      clay_image_get_layer_format (new_image,
                                                                   has_alpha),
                                      layer_name,
                                      CLAY_OPACITY_OPAQUE,
                                      clay_image_get_default_new_layer_mode (new_image));

  clay_image_add_layer (new_image, layer, NULL, 0, TRUE);

  clay_image_new_add_creation_metadata (new_image);

  clay_image_undo_enable (new_image);

  return new_image;
}


/*  private functions  */

static void
clay_image_new_add_creation_metadata (ClayImage *image)
{
  ClayMetadata *metadata;

  metadata = clay_image_get_metadata (image);
  if (! metadata)
    {
      g_critical ("Metadata not found. Should not happen!");
    }
  else
    {
      GDateTime *datetime;

      datetime = g_date_time_new_now_local ();
      clay_metadata_set_creation_date (metadata, datetime);
      g_date_time_unref (datetime);
    }
}
