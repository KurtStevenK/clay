/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayer.c
 *
 *   Copyright 2006 Hendrik Boom
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

#include <stdio.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"

#include "path-types.h"

#include "core/clay.h"
#include "core/claydrawable-fill.h"
#include "core/claydrawable-stroke.h"
#include "core/clayimage.h"
#include "core/clayselection.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/claystrokeoptions.h"
#include "core/clayparasitelist.h"
#include "core/clayrasterizable.h"
#include "core/claysavable.h"
#include "core/claysavable-load.h"

#include "gegl/clay-gegl-utils.h"

#include "clayvectorlayer.h"
#include "clayvectorlayeroptions.h"
#include "claypath.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_VECTOR_LAYER_OPTIONS,
};


/* local function declarations */

static void       clay_vector_layer_rasterizable_iface_init (ClayRasterizableInterface *iface);
static void       clay_vector_layer_savable_iface_init      (ClaySavableInterface      *iface);


static void       clay_vector_layer_finalize                (GObject                   *object);
static void       clay_vector_layer_get_property            (GObject                   *object,
                                                             guint                      property_id,
                                                             GValue                    *value,
                                                             GParamSpec                *pspec);
static void       clay_vector_layer_set_property            (GObject                   *object,
                                                             guint                      property_id,
                                                             const GValue              *value,
                                                             GParamSpec                *pspec);

static void       clay_vector_layer_set_rasterized          (ClayRasterizable          *rasterizable,
                                                             gboolean                   rasterized);

static void       clay_vector_layer_savable_save            (ClaySavable               *savable,
                                                             ClaySaveState             *state);
static gboolean   clay_vector_layer_load_enter              (ClayLoadState             *state,
                                                             const gchar              **attribute_names,
                                                             const gchar              **attribute_values,
                                                             gpointer                   user_data,
                                                             GError                   **error);
static gboolean   clay_vector_layer_load_exit               (ClayLoadState             *state,
                                                             const gchar               *text,
                                                             gsize                      len,
                                                             gpointer                   user_data,
                                                             GError                   **error);

static void       clay_vector_layer_set_vector_options      (ClayVectorLayer           *layer,
                                                             ClayVectorLayerOptions    *options);

static void       clay_vector_layer_set_buffer              (ClayDrawable              *drawable,
                                                             gboolean                   push_undo,
                                                             const gchar               *undo_desc,
                                                             GeglBuffer                *buffer,
                                                             const GeglRectangle       *bounds);
static void       clay_vector_layer_push_undo               (ClayDrawable              *drawable,
                                                             const gchar               *undo_desc,
                                                             GeglBuffer                *buffer,
                                                             gint                       x,
                                                             gint                       y,
                                                             gint                       width,
                                                             gint                       height);

static gint64     clay_vector_layer_get_memsize             (ClayObject                *object,
                                                             gint64                    *gui_size);

static void       clay_vector_layer_convert                 (ClayItem                  *item,
                                                             ClayImage                 *dest_image,
                                                             GType                      old_type);
static ClayItem * clay_vector_layer_duplicate               (ClayItem                  *item,
                                                             GType                      new_type);

static void       clay_vector_layer_translate               (ClayLayer                 *layer,
                                                             gint                       offset_x,
                                                             gint                       offset_y);
static void       clay_vector_layer_scale                   (ClayItem                  *item,
                                                             gint                       new_width,
                                                             gint                       new_height,
                                                             gint                       new_offset_x,
                                                             gint                       new_offset_y,
                                                             ClayInterpolationType      interp_type,
                                                             ClayProgress              *progress);
static void       clay_vector_layer_resize                  (ClayItem                  *item,
                                                             ClayContext               *context,
                                                             ClayFillType               fill_type,
                                                             gint                       new_width,
                                                             gint                       new_height,
                                                             gint                       offset_x,
                                                             gint                       offset_y);
static void       clay_vector_layer_flip                    (ClayItem                  *item,
                                                             ClayContext               *context,
                                                             ClayOrientationType        flip_type,
                                                             gdouble                    axis,
                                                             gboolean                   clip_result);
static void       clay_vector_layer_rotate                  (ClayItem                  *item,
                                                             ClayContext               *context,
                                                             ClayRotationType           rotate_type,
                                                             gdouble                    center_x,
                                                             gdouble                    center_y,
                                                             gboolean                   clip_result);
static void       clay_vector_layer_transform               (ClayItem                  *item,
                                                             ClayContext               *context,
                                                             const ClayMatrix3         *matrix,
                                                             ClayTransformDirection     direction,
                                                             ClayInterpolationType      interp_type,
                                                             ClayTransformResize        clip_result,
                                                             ClayProgress              *progress,
                                                             gboolean                   push_undo);

static gboolean   clay_vector_layer_render                  (ClayVectorLayer           *layer);
static void       clay_vector_layer_render_path             (ClayVectorLayer           *layer);
static void       clay_vector_layer_changed_options         (ClayVectorLayer           *layer);

static void       clay_vector_layer_removed                 (ClayItem                  *item);

static void       clay_vector_layer_removed_options_path    (ClayVectorLayer           *layer);


G_DEFINE_TYPE_WITH_CODE (ClayVectorLayer, clay_vector_layer, CLAY_TYPE_LAYER,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_RASTERIZABLE,
                                                clay_vector_layer_rasterizable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_vector_layer_savable_iface_init))

#define parent_class clay_vector_layer_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;


static void
clay_vector_layer_class_init (ClayVectorLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);
  ClayLayerClass    *layer_class       = CLAY_LAYER_CLASS (klass);

  drawable_class->set_buffer        = clay_vector_layer_set_buffer;
  drawable_class->push_undo         = clay_vector_layer_push_undo;

  object_class->finalize            = clay_vector_layer_finalize;
  object_class->set_property        = clay_vector_layer_set_property;
  object_class->get_property        = clay_vector_layer_get_property;

  clay_object_class->get_memsize    = clay_vector_layer_get_memsize;

  /* TODO: make a custom icon "clay-vector-layer". */
  viewable_class->default_icon_name = "clay-tool-path";
  viewable_class->default_name      = _("Vector Layer");

  layer_class->translate            = clay_vector_layer_translate;

  item_class->removed               = clay_vector_layer_removed;
  item_class->convert               = clay_vector_layer_convert;
  item_class->duplicate             = clay_vector_layer_duplicate;
  item_class->rename                = clay_rasterizable_rename;
  item_class->scale                 = clay_vector_layer_scale;
  item_class->resize                = clay_vector_layer_resize;
  item_class->flip                  = clay_vector_layer_flip;
  item_class->rotate                = clay_vector_layer_rotate;
  item_class->transform             = clay_vector_layer_transform;
  item_class->rename_desc           = _("Rename Vector Layer");
  item_class->translate_desc        = _("Move Vector Layer");
  item_class->scale_desc            = _("Scale Vector Layer");
  item_class->resize_desc           = _("Resize Vector Layer");
  item_class->flip_desc             = _("Flip Vector Layer");
  item_class->rotate_desc           = _("Rotate Vector Layer");
  item_class->transform_desc        = _("Transform Vector Layer");

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_VECTOR_LAYER_OPTIONS,
                           "vector-layer-options", NULL, NULL,
                           CLAY_TYPE_VECTOR_LAYER_OPTIONS,
                           G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
}

static void
clay_vector_layer_init (ClayVectorLayer *layer)
{
  layer->options  = NULL;
}

static void
clay_vector_layer_rasterizable_iface_init (ClayRasterizableInterface *iface)
{
  iface->set_rasterized = clay_vector_layer_set_rasterized;
}

static void
clay_vector_layer_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "vector-layer";
  iface->save       = clay_vector_layer_savable_save;
  iface->load_enter = clay_vector_layer_load_enter;
  iface->load_exit  = clay_vector_layer_load_exit;
}

static void
clay_vector_layer_finalize (GObject *object)
{
  ClayVectorLayer *layer = CLAY_VECTOR_LAYER (object);

  if (layer->options)
    {
      g_object_unref (layer->options);
      layer->options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_vector_layer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      g_value_set_object (value, vector_layer->options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_vector_layer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      clay_vector_layer_set_vector_options (vector_layer, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_vector_layer_set_rasterized (ClayRasterizable *rasterizable,
                                  gboolean          rasterized)
{
  ClayVectorLayer *layer = CLAY_VECTOR_LAYER (rasterizable);

  if (! rasterized)
    {
      clay_vector_layer_render (layer);

      g_signal_connect_object (layer->options, "notify",
                               G_CALLBACK (clay_vector_layer_changed_options),
                               layer, G_CONNECT_SWAPPED);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (layer->options,
                                            G_CALLBACK (clay_vector_layer_changed_options),
                                            layer);
    }
}

static void
clay_vector_layer_savable_save (ClaySavable   *savable,
                                ClaySaveState *state)
{
  ClayVectorLayer *layer = CLAY_VECTOR_LAYER (savable);

  clay_savable_print_element_start (state, "vector-layer",
                                    "name",        "%s", clay_object_get_name (CLAY_OBJECT (layer)),
                                    "path-tattoo", "%u", layer->options->path_tattoo,
                                    "fill",        "%b", layer->options->enable_fill,
                                    "stroke",      "%b", layer->options->enable_stroke,
                                    NULL);

  parent_savable_iface->save (savable, state);

  if (layer->options->enable_fill)
    clay_savable_save (CLAY_SAVABLE (layer->options->fill_options), state);

  if (layer->options->enable_stroke)
    clay_savable_save (CLAY_SAVABLE (layer->options->stroke_options), state);

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_savable_print_element (state, "rasterized", NULL, NULL, NULL);

  clay_savable_print_element_end (state, "vector-layer");
}

static gboolean
clay_vector_layer_load_enter (ClayLoadState  *state,
                              const gchar   **attribute_names,
                              const gchar   **attribute_values,
                              gpointer        user_data,
                              GError        **error)
{
  ClayVectorLayer *layer;
  const gchar     *name        = NULL;
  ClayPath        *path        = NULL;
  guint            path_tattoo = 0;
  gboolean         fill        = FALSE;
  gboolean         stroke      = FALSE;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        {
          name = *attribute_values;
        }
      else if (g_strcmp0 (*attribute_names, "path-tattoo") == 0)
        {
          clay_savable_load_store_from_string (state, error,
                                               "path-tattoo", G_TYPE_UINT, *attribute_values,
                                               NULL);
          clay_savable_load_get_values (state, "path-tattoo", &path_tattoo, NULL);
        }
      else if (g_strcmp0 (*attribute_names, "fill") == 0)
        {
          clay_savable_load_store_from_string (state, error,
                                               "fill", G_TYPE_BOOLEAN, *attribute_values,
                                               NULL);
          clay_savable_load_get_values (state, "fill", &fill, NULL);
        }
      else if (g_strcmp0 (*attribute_names, "stroke") == 0)
        {
          clay_savable_load_store_from_string (state, error,
                                               "stroke", G_TYPE_BOOLEAN, *attribute_values,
                                               NULL);
          clay_savable_load_get_values (state, "stroke", &stroke, NULL);
        }
      else
        {
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  parent_savable_iface->load_enter (state, attribute_names, attribute_values,
                                    user_data, error);

  if (path_tattoo > 0)
    path = clay_image_get_path_by_tattoo (state->image, (ClayTattoo) path_tattoo);

  if (path == NULL)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: failed to load path associated with vector layer \"%s\".",
                   G_STRFUNC, name);
      return TRUE;
    }

  layer = clay_vector_layer_new (state->image, path,
                                 clay_get_user_context (state->clay));

  if (name == NULL)
    g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                 "%s: missing layer group name.",
                 G_STRFUNC);
  else
    clay_object_set_name (CLAY_OBJECT (layer), name);

  g_object_set (layer->options,
                "enable-fill",   fill,
                "enable-stroke", stroke,
                NULL);

  /* We rasterize as a trick so that any settings update won't override
   * the buffer contents. We will actually set to proper state in
   * load_exit().
   */
  clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), FALSE);

  if (fill)
    clay_savable_load (CLAY_TYPE_FILL_OPTIONS, state);
  if (stroke)
    clay_savable_load (CLAY_TYPE_STROKE_OPTIONS, state);

  clay_savable_load_add_simple_handler (state, "rasterized", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);

  clay_savable_load_push_active_object (state, G_OBJECT (layer));

  return TRUE;
}

static gboolean
clay_vector_layer_load_exit (ClayLoadState  *state,
                             const gchar    *text,
                             gsize           len,
                             gpointer        user_data,
                             GError        **error)
{
  ClayVectorLayer *layer;
  gboolean         rasterized = FALSE;

  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  /* Order matters because parent's load_exit() will pop the layer. */
  layer = CLAY_VECTOR_LAYER (clay_savable_load_peek_active_object (state));
  parent_savable_iface->load_exit (state, text, len, user_data, error);

  clay_savable_load_get_values (state, "rasterized", &rasterized, NULL);
  if (rasterized && ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), FALSE);
  else if (! rasterized && clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));

  return TRUE;
}

static void
clay_vector_layer_set_vector_options (ClayVectorLayer        *layer,
                                      ClayVectorLayerOptions *options)
{
  if (layer->options)
    {
      g_signal_handlers_disconnect_by_func (layer->options,
                                            G_CALLBACK (clay_vector_layer_changed_options),
                                            layer);

      if (layer->options->path)
        g_signal_handlers_disconnect_by_func (layer->options->path,
                                              G_CALLBACK (clay_vector_layer_removed_options_path),
                                              layer);

      g_object_unref (layer->options);
      layer->options = NULL;
    }

  if (options)
    g_set_object (&layer->options, options);

  clay_vector_layer_changed_options (layer);

  if (layer->options)
    {
      if (layer->options->path)
        g_signal_connect_object (layer->options->path, "removed",
                                 G_CALLBACK (clay_vector_layer_removed_options_path),
                                 layer, G_CONNECT_SWAPPED);

      g_signal_connect_object (layer->options, "notify",
                               G_CALLBACK (clay_vector_layer_changed_options),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "vector-layer-options");
}

static void
clay_vector_layer_set_buffer (ClayDrawable        *drawable,
                              gboolean             push_undo,
                              const gchar         *undo_desc,
                              GeglBuffer          *buffer,
                              const GeglRectangle *bounds)
{
  ClayVectorLayer *layer = CLAY_VECTOR_LAYER (drawable);
  ClayImage       *image = clay_item_get_image (CLAY_ITEM (layer));
  gboolean         is_rasterized;

  is_rasterized = clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer));

  if (push_undo && ! is_rasterized)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! is_rasterized)
    {
      clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), push_undo);
      clay_image_undo_group_end (image);
    }
}

static void
clay_vector_layer_push_undo (ClayDrawable *drawable,
                             const gchar  *undo_desc,
                             GeglBuffer   *buffer,
                             gint          x,
                             gint          y,
                             gint          width,
                             gint          height)
{
  ClayVectorLayer *layer = CLAY_VECTOR_LAYER (drawable);
  ClayImage       *image = clay_item_get_image (CLAY_ITEM (layer));
  gboolean         is_rasterized;

  is_rasterized = clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer));

  if (! is_rasterized)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE, undo_desc);

  CLAY_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! is_rasterized)
    {
      clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), TRUE);
      clay_image_undo_group_end (image);
    }
}

static gint64
clay_vector_layer_get_memsize (ClayObject *object,
                               gint64     *gui_size)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (object);
  gint64           memsize      = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (vector_layer->options),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_vector_layer_convert (ClayItem  *item,
                           ClayImage *dest_image,
                           GType      old_type)
{
  ClayPath *path = clay_vector_layer_get_path (CLAY_VECTOR_LAYER (item));

  if (path)
    {
      CLAY_ITEM_GET_CLASS (path)->convert (CLAY_ITEM (path), dest_image,
                                           G_TYPE_FROM_INSTANCE (path));

      if (! clay_item_is_attached (CLAY_ITEM (path)))
        clay_image_add_path (dest_image, path, NULL, -1, FALSE);
    }

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static ClayItem *
clay_vector_layer_duplicate (ClayItem *item,
                             GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_VECTOR_LAYER (new_item))
    {
      ClayVectorLayer *vector_layer     = CLAY_VECTOR_LAYER (item);
      ClayVectorLayer *new_vector_layer = CLAY_VECTOR_LAYER (new_item);

      if (vector_layer->options)
        {
          ClayPath               *path;
          ClayVectorLayerOptions *new_options =
            clay_config_duplicate (CLAY_CONFIG (vector_layer->options));

          path = clay_vector_layer_get_path (vector_layer);
          if (path)
            {
              ClayPath *new_path;

              new_path = CLAY_PATH (clay_item_duplicate (CLAY_ITEM (path),
                                                         G_TYPE_FROM_INSTANCE (CLAY_ITEM (path))));

              g_object_set (new_options, "path", new_path, NULL);
            }

          g_object_set (new_vector_layer,
                        "vector-layer-options", new_options,
                        NULL);
          g_object_unref (new_options);
        }

      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (vector_layer)))
        clay_rasterizable_set_undo_rasterized (CLAY_RASTERIZABLE (new_vector_layer), TRUE);
    }

  return new_item;
}

static void
clay_vector_layer_translate (ClayLayer *layer,
                             gint       offset_x,
                             gint       offset_y)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (layer);

  if (clay_item_is_vector_layer (CLAY_ITEM (layer)))
    {
      clay_item_translate (CLAY_ITEM (vector_layer->options->path),
                           offset_x, offset_y, FALSE);
    }
  else
    {
      CLAY_LAYER_CLASS (parent_class)->translate (layer, offset_x, offset_y);
    }
}

static void
clay_vector_layer_scale (ClayItem              *item,
                         gint                   new_width,
                         gint                   new_height,
                         gint                   new_offset_x,
                         gint                   new_offset_y,
                         ClayInterpolationType  interp_type,
                         ClayProgress          *progress)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (clay_item_is_vector_layer (item))
    {
      gdouble scale_x = new_width  / (gdouble) clay_item_get_width  (item);
      gdouble scale_y = new_height / (gdouble) clay_item_get_height (item);

      clay_item_scale_by_factors (CLAY_ITEM (vector_layer->options->path),
                                  scale_x, scale_y,
                                  interp_type, progress);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interp_type, progress);
    }
}

static void
clay_vector_layer_resize (ClayItem     *item,
                          ClayContext  *context,
                          ClayFillType  fill_type,
                          gint          new_width,
                          gint          new_height,
                          gint          offset_x,
                          gint          offset_y)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (clay_item_is_vector_layer (item))
    {
      clay_item_resize (CLAY_ITEM (vector_layer->options->path),
                        context, fill_type, new_width, new_height,
                        offset_x, offset_y);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                              new_width, new_height, offset_x,
                                              offset_y);
    }
}

static void
clay_vector_layer_flip (ClayItem            *item,
                        ClayContext         *context,
                        ClayOrientationType  flip_type,
                        gdouble              axis,
                        gboolean             clip_result)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (clay_item_is_vector_layer (item))
    {
      clay_item_flip (CLAY_ITEM (vector_layer->options->path),
                      context, flip_type, axis, clip_result);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->flip (item, context, flip_type,
                                            axis, clip_result);
    }
}

static void
clay_vector_layer_rotate (ClayItem         *item,
                          ClayContext      *context,
                          ClayRotationType  rotate_type,
                          gdouble           center_x,
                          gdouble           center_y,
                          gboolean          clip_result)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (clay_item_is_vector_layer (item))
    {
      clay_item_rotate (CLAY_ITEM (vector_layer->options->path),
                        context, rotate_type, center_x, center_y, clip_result);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->rotate (item, context, rotate_type,
                                              center_x, center_y, clip_result);
    }
}

static void
clay_vector_layer_transform (ClayItem               *item,
                             ClayContext            *context,
                             const ClayMatrix3      *matrix,
                             ClayTransformDirection  direction,
                             ClayInterpolationType   interp_type,
                             ClayTransformResize     clip_result,
                             ClayProgress           *progress,
                             gboolean                push_undo)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (clay_item_is_vector_layer (item))
    {
      clay_item_transform (CLAY_ITEM (vector_layer->options->path),
                           context, matrix, direction, interp_type,
                           clip_result, progress);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->transform (item, context, matrix,
                                                 direction, interp_type,
                                                 clip_result, progress, push_undo);
    }
}

static void
clay_vector_layer_removed_options_path (ClayVectorLayer *layer)
{
  if (layer->options)
    {
      clay_image_undo_push_vector_layer (clay_item_get_image (CLAY_ITEM (layer)),
                                         _("Discard Vector Information"),
                                         layer, NULL);

      g_object_set (layer->options, "path", NULL, NULL);
    }
}

static void
clay_vector_layer_removed (ClayItem *item)
{
  ClayVectorLayer *vector_layer = CLAY_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    g_signal_handlers_disconnect_by_func (vector_layer->options->path,
                                          G_CALLBACK (clay_vector_layer_removed_options_path),
                                          vector_layer);

  CLAY_ITEM_CLASS (parent_class)->removed (item);
}


/*  public functions  */

/**
 * clay_vector_layer_new:
 * @image:   the #ClayImage the layer should belong to
 * @path:    the #ClayPath object the layer should render
 * @context: the #ClayContext from which to pull context properties
 *
 * Creates a new vector layer.
 *
 * Return value: a new #ClayVectorLayer or %NULL in case of a problem
 **/
ClayVectorLayer *
clay_vector_layer_new (ClayImage   *image,
                       ClayPath    *path,
                       ClayContext *context)
{
  ClayVectorLayer        *layer;
  ClayVectorLayerOptions *options;
  gint                    x      = 0;
  gint                    y      = 0;
  gint                    width  = clay_image_get_width (image);
  gint                    height = clay_image_get_height (image);

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  options = clay_vector_layer_options_new (image, path, context);

  clay_item_bounds (CLAY_ITEM (path), &x, &y, &width, &height);

  /* Set boundaries to image size if it's a blank path */
  if (width == 0 || height == 0)
    {
      width  = clay_image_get_width (image);
      height = clay_image_get_height (image);
    }

  layer =
    CLAY_VECTOR_LAYER (clay_drawable_new (CLAY_TYPE_VECTOR_LAYER,
                                          image, NULL,
                                          x, y, width, height,
                                          clay_image_get_layer_format (image,
                                                                       TRUE)));

  clay_object_set_name (CLAY_OBJECT (layer),
                        clay_object_get_name (CLAY_OBJECT (path)));

  clay_layer_set_mode (CLAY_LAYER (layer),
                       clay_image_get_default_new_layer_mode (image),
                       FALSE);

  clay_vector_layer_set_vector_options (layer, options);
  g_object_unref (options);

  return layer;
}

void
clay_vector_layer_set_path (ClayVectorLayer *layer,
                            ClayPath        *path,
                            gboolean         push_undo)
{
  ClayImage *image = clay_item_get_image (CLAY_ITEM (layer));

  if (push_undo)
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (layer->options), "path");
      clay_image_undo_push_vector_layer (image,
                                         _("Change Vector Layer Path"),
                                         layer, pspec);
    }

  g_object_set (layer->options,
                "path", path,
                NULL);

  clay_image_flush (image);
}

/**
 * clay_vector_layer_get_path:
 * @layer: a #ClayVectorLayer
 *
 * Gets the path from @layer if one is associated with it.
 *
  * Return value: a #ClayPath or %NULL if no path is set.
 */
ClayPath *
clay_vector_layer_get_path (ClayVectorLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (layer), NULL);

  return layer->options->path;
}

/**
 * clay_vector_layer_get_options:
 * @layer: a #ClayVectorLayer
 *
 * Gets the vector layer options from @layer if one is associated with it.
 *
  * Return value: a #ClayVectorLayerOptions or %NULL if no options are set.
 */
ClayVectorLayerOptions *
clay_vector_layer_get_options (ClayVectorLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (layer), NULL);

  if (clay_item_is_vector_layer (CLAY_ITEM (layer)))
    return layer->options;

  return NULL;
}

void
clay_vector_layer_set (ClayVectorLayer *layer,
                       const gchar     *undo_desc,
                       const gchar     *first_property_name,
                       ...)
{
  ClayImage              *image;
  ClayVectorLayerOptions *options;
  va_list                 var_args;

  g_return_if_fail (clay_item_is_vector_layer (CLAY_ITEM (layer)));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (layer)));

  options = layer->options;

  if (! options)
    return;

  image = clay_item_get_image (CLAY_ITEM (layer));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_VECTOR, undo_desc);

  g_object_freeze_notify (G_OBJECT (layer));
  g_object_freeze_notify (G_OBJECT (options));

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_image_undo_push_drawable_mod (image, NULL,
                                       CLAY_DRAWABLE (layer), TRUE);

  clay_image_undo_push_vector_layer (image, undo_desc, layer, NULL);

  va_start (var_args, first_property_name);
  g_object_set_valist (G_OBJECT (options), first_property_name,
                       var_args);
  va_end (var_args);

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));

  g_object_thaw_notify (G_OBJECT (layer));
  g_object_thaw_notify (G_OBJECT (options));

  clay_image_undo_group_end (image);
}

void
clay_vector_layer_refresh (ClayVectorLayer *layer)
{
  if (layer->options)
    clay_vector_layer_render (layer);
}

gboolean
clay_item_is_vector_layer (ClayItem *item)
{
  return (CLAY_IS_VECTOR_LAYER (item) &&
          ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (item)));
}


/* private functions  */

static gboolean
clay_vector_layer_render (ClayVectorLayer *layer)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (layer);
  GeglBuffer   *buffer   = NULL;
  ClayItem     *item     = CLAY_ITEM (layer);
  ClayImage    *image    = clay_item_get_image (item);
  gint          x        = 0;
  gint          y        = 0;
  gint          width    = clay_image_get_width (image);
  gint          height   = clay_image_get_height (image);
  gdouble       stroke   = 0;

  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), FALSE);

  g_object_freeze_notify (G_OBJECT (drawable));

  if (layer->options->enable_stroke)
    stroke = clay_stroke_options_get_width (layer->options->stroke_options);

  /* Resize layer according to path size */
  clay_item_bounds (CLAY_ITEM (layer->options->path), &x, &y, &width, &height);

  buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 (gint) ceil (width + stroke),
                                                 (gint) ceil (height + stroke)),
                                 clay_drawable_get_format (drawable),
                                 drawable);
  clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  clay_item_set_offset (CLAY_ITEM (layer), x - (stroke / 2), y - (stroke / 2));

  /* make the layer background transparent */
  clay_drawable_fill (CLAY_DRAWABLE (layer),
                      clay_get_user_context (image->clay),
                      CLAY_FILL_TRANSPARENT);

  /* render path to the layer */
  clay_vector_layer_render_path (layer);

  g_object_thaw_notify (G_OBJECT (drawable));

  return TRUE;
}

static void
clay_vector_layer_render_path (ClayVectorLayer *layer)
{
  ClayImage              *image     = clay_item_get_image (CLAY_ITEM (layer));
  ClayVectorLayerOptions *options   = layer->options;
  ClayPath               *path      = NULL;
  ClayChannel            *selection = clay_image_get_mask (image);
  GList                  *drawables;
  ClayCustomStyle         style;

  if (options)
    path = options->path;

  /* Don't mask these fill/stroke operations  */
  clay_selection_suspend (CLAY_SELECTION (selection));

  /* Convert from custom to standard styles */
  style = clay_fill_options_get_custom_style (options->fill_options);
  if (style == CLAY_CUSTOM_STYLE_SOLID_COLOR ||
      ! clay_context_get_pattern (CLAY_CONTEXT (options->fill_options)))
    clay_fill_options_set_style (options->fill_options,
                                 CLAY_FILL_STYLE_FG_COLOR);
  else
    clay_fill_options_set_style (options->fill_options,
                                 CLAY_FILL_STYLE_PATTERN);

  style =
    clay_fill_options_get_custom_style (CLAY_FILL_OPTIONS (options->stroke_options));
  if (style == CLAY_CUSTOM_STYLE_SOLID_COLOR ||
      ! clay_context_get_pattern (CLAY_CONTEXT (options->stroke_options)))
    clay_fill_options_set_style (CLAY_FILL_OPTIONS (options->stroke_options),
                                 CLAY_FILL_STYLE_FG_COLOR);
  else
    clay_fill_options_set_style (CLAY_FILL_OPTIONS (options->stroke_options),
                                 CLAY_FILL_STYLE_PATTERN);

  /* Fill the path object onto the layer */
  if (options->enable_fill)
    clay_drawable_fill_path (CLAY_DRAWABLE (layer),
                             options->fill_options,
                             path, FALSE, NULL);

  drawables = g_list_prepend (NULL, CLAY_DRAWABLE (layer));
  /* stroke the path object onto the layer */
  if (options->enable_stroke && clay_item_is_attached (CLAY_ITEM (path)))
    clay_item_stroke (CLAY_ITEM (path), drawables,
                      clay_get_user_context (image->clay),
                      options->stroke_options,
                      FALSE, FALSE,
                      NULL, NULL);

  g_list_free (drawables);

  clay_selection_resume (CLAY_SELECTION (selection));
}

static void
clay_vector_layer_changed_options (ClayVectorLayer *layer)
{
  ClayItem *item = CLAY_ITEM (layer);

  if (layer->options && ! layer->options->path)
    clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), TRUE);
  else if (clay_item_is_attached (item) &&
           ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_vector_layer_refresh (layer);
}
