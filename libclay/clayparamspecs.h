/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayparamspecs.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __LIBCLAY_CLAY_PARAM_SPECS_H__
#define __LIBCLAY_CLAY_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * CLAY_TYPE_PARAM_IMAGE
 */

#define CLAY_VALUE_HOLDS_IMAGE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         CLAY_TYPE_IMAGE))

#define CLAY_TYPE_PARAM_IMAGE           (clay_param_image_get_type ())
#define CLAY_IS_PARAM_SPEC_IMAGE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_IMAGE))

GType        clay_param_image_get_type          (void);

GParamSpec * clay_param_spec_image              (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 gboolean      none_ok,
                                                 GParamFlags   flags);

gboolean     clay_param_spec_image_none_allowed (GParamSpec   *pspec);


/*
 * CLAY_TYPE_PARAM_ITEM
 */

#define CLAY_VALUE_HOLDS_ITEM(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        CLAY_TYPE_ITEM))

#define CLAY_TYPE_PARAM_ITEM           (clay_param_item_get_type ())
#define CLAY_IS_PARAM_SPEC_ITEM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_ITEM))

GType        clay_param_item_get_type          (void);

GParamSpec * clay_param_spec_item              (const gchar  *name,
                                                const gchar  *nick,
                                                const gchar  *blurb,
                                                gboolean      none_ok,
                                                GParamFlags   flags);

gboolean     clay_param_spec_item_none_allowed (GParamSpec   *pspec);


/*
 * CLAY_TYPE_PARAM_DRAWABLE
 */

#define CLAY_VALUE_HOLDS_DRAWABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            CLAY_TYPE_DRAWABLE))

#define CLAY_TYPE_PARAM_DRAWABLE           (clay_param_drawable_get_type ())
#define CLAY_IS_PARAM_SPEC_DRAWABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_DRAWABLE))

GType        clay_param_drawable_get_type (void);

GParamSpec * clay_param_spec_drawable     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_LAYER
 */

#define CLAY_VALUE_HOLDS_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         CLAY_TYPE_LAYER))

#define CLAY_TYPE_PARAM_LAYER           (clay_param_layer_get_type ())
#define CLAY_IS_PARAM_SPEC_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_LAYER))

GType        clay_param_layer_get_type (void);

GParamSpec * clay_param_spec_layer     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_TEXT_LAYER
 */

#define CLAY_VALUE_HOLDS_TEXT_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              CLAY_TYPE_TEXT_LAYER))

#define CLAY_TYPE_PARAM_TEXT_LAYER           (clay_param_text_layer_get_type ())
#define CLAY_IS_PARAM_SPEC_TEXT_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_TEXT_LAYER))

GType        clay_param_text_layer_get_type (void);

GParamSpec * clay_param_spec_text_layer     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             gboolean      none_ok,
                                             GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_VECTOR_LAYER
 */

#define CLAY_VALUE_HOLDS_VECTOR_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                                CLAY_TYPE_VECTOR_LAYER))

#define CLAY_TYPE_PARAM_VECTOR_LAYER           (clay_param_vector_layer_get_type ())
#define CLAY_IS_PARAM_SPEC_VECTOR_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_VECTOR_LAYER))

GType        clay_param_vector_layer_get_type (void);

GParamSpec * clay_param_spec_vector_layer     (const gchar  *name,
                                               const gchar  *nick,
                                               const gchar  *blurb,
                                               gboolean      none_ok,
                                               GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_LINK_LAYER
 */

#define CLAY_VALUE_HOLDS_LINK_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_LINK_LAYER))
#define CLAY_TYPE_PARAM_LINK_LAYER           (clay_param_link_layer_get_type ())
#define CLAY_IS_PARAM_SPEC_LINK_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_LINK_LAYER))

GType        clay_param_link_layer_get_type   (void);

GParamSpec * clay_param_spec_link_layer       (const gchar  *name,
                                               const gchar  *nick,
                                               const gchar  *blurb,
                                               gboolean      none_ok,
                                               GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_RASTERIZABLE
 */

#define CLAY_VALUE_HOLDS_RASTERIZABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_RASTERIZABLE))
#define CLAY_TYPE_PARAM_RASTERIZABLE           (clay_param_rasterizable_get_type ())
#define CLAY_IS_PARAM_SPEC_RASTERIZABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_RASTERIZABLE))

GType        clay_param_rasterizable_get_type   (void);

GParamSpec * clay_param_spec_rasterizable       (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 gboolean      none_ok,
                                                 GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_GROUP_LAYER
 */

#define CLAY_VALUE_HOLDS_GROUP_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                               CLAY_TYPE_GROUP_LAYER))

#define CLAY_TYPE_PARAM_GROUP_LAYER           (clay_param_group_layer_get_type ())
#define CLAY_IS_PARAM_SPEC_GROUP_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_GROUP_LAYER))

GType        clay_param_group_layer_get_type (void);

GParamSpec * clay_param_spec_group_layer     (const gchar  *name,
                                              const gchar  *nick,
                                              const gchar  *blurb,
                                              gboolean      none_ok,
                                              GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_CHANNEL
 */

#define CLAY_VALUE_HOLDS_CHANNEL(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           CLAY_TYPE_CHANNEL))

#define CLAY_TYPE_PARAM_CHANNEL           (clay_param_channel_get_type ())
#define CLAY_IS_PARAM_SPEC_CHANNEL(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_CHANNEL))

GType        clay_param_channel_get_type (void);

GParamSpec * clay_param_spec_channel     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_LAYER_MASK
 */

#define CLAY_VALUE_HOLDS_LAYER_MASK(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              CLAY_TYPE_LAYER_MASK))

#define CLAY_TYPE_PARAM_LAYER_MASK           (clay_param_layer_mask_get_type ())
#define CLAY_IS_PARAM_SPEC_LAYER_MASK(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_LAYER_MASK))

GType        clay_param_layer_mask_get_type (void);

GParamSpec * clay_param_spec_layer_mask     (const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags);


/*
 * CLAY_TYPE_PARAM_SELECTION
 */

#define CLAY_VALUE_HOLDS_SELECTION(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                             CLAY_TYPE_SELECTION))

#define CLAY_TYPE_PARAM_SELECTION           (clay_param_selection_get_type ())
#define CLAY_IS_PARAM_SPEC_SELECTION(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_SELECTION))

GType        clay_param_selection_get_type (void);

GParamSpec * clay_param_spec_selection     (const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags);


/*
 * CLAY_TYPE_PARAM_PATH
 */

#define CLAY_VALUE_HOLDS_PATH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        CLAY_TYPE_PATH))

#define CLAY_TYPE_PARAM_PATH           (clay_param_path_get_type ())
#define CLAY_IS_PARAM_SPEC_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_PATH))

GType        clay_param_path_get_type (void);

GParamSpec * clay_param_spec_path     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_DRAWABLE_FILTER
 */

#define CLAY_VALUE_HOLDS_DRAWABLE_FILTER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                                                            CLAY_TYPE_DRAWABLE_FILTER))

#define CLAY_TYPE_PARAM_DRAWABLE_FILTER           (clay_param_drawable_filter_get_type ())
#define CLAY_IS_PARAM_SPEC_DRAWABLE_FILTER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_DRAWABLE_FILTER))

GType        clay_param_drawable_filter_get_type          (void);

GParamSpec * clay_param_spec_drawable_filter              (const gchar  *name,
                                                           const gchar  *nick,
                                                           const gchar  *blurb,
                                                           gboolean      none_ok,
                                                           GParamFlags   flags);

gboolean      clay_param_spec_drawable_filter_none_allowed (GParamSpec *pspec);


/*
 * CLAY_TYPE_PARAM_DISPLAY
 */

#define CLAY_VALUE_HOLDS_DISPLAY(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           CLAY_TYPE_DISPLAY))

#define CLAY_TYPE_PARAM_DISPLAY           (clay_param_display_get_type ())
#define CLAY_IS_PARAM_SPEC_DISPLAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_DISPLAY))

GType        clay_param_display_get_type         (void);

GParamSpec * clay_param_spec_display             (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  gboolean      none_ok,
                                                  GParamFlags   flags);

gboolean     clay_param_spec_display_none_allowed (GParamSpec *pspec);


/*
 * CLAY_TYPE_PARAM_RESOURCE
 */

#define CLAY_VALUE_HOLDS_RESOURCE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_RESOURCE))

#define CLAY_TYPE_PARAM_RESOURCE           (clay_param_resource_get_type ())
#define CLAY_IS_PARAM_SPEC_RESOURCE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_RESOURCE))

GType        clay_param_resource_get_type                 (void);

GParamSpec * clay_param_spec_resource                     (const gchar  *name,
                                                           const gchar  *nick,
                                                           const gchar  *blurb,
                                                           GType         resource_type,
                                                           gboolean      none_ok,
                                                           ClayResource *default_value,
                                                           gboolean      default_to_context,
                                                           GParamFlags   flags);

gboolean     clay_param_spec_resource_none_allowed        (GParamSpec   *pspec);
gboolean     clay_param_spec_resource_defaults_to_context (GParamSpec   *pspec);


/*
 * CLAY_TYPE_PARAM_BRUSH
 */

#define CLAY_VALUE_HOLDS_BRUSH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                         CLAY_TYPE_BRUSH))

#define CLAY_TYPE_PARAM_BRUSH           (clay_param_brush_get_type ())
#define CLAY_IS_PARAM_SPEC_BRUSH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_BRUSH))

GType        clay_param_brush_get_type (void);

GParamSpec * clay_param_spec_brush (const gchar  *name,
                                    const gchar  *nick,
                                    const gchar  *blurb,
                                    gboolean      none_ok,
                                    ClayBrush    *default_value,
                                    gboolean      default_to_context,
                                    GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_PATTERN
 */

#define CLAY_VALUE_HOLDS_PATTERN(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           CLAY_TYPE_PATTERN))

#define CLAY_TYPE_PARAM_PATTERN           (clay_param_pattern_get_type ())
#define CLAY_IS_PARAM_SPEC_PATTERN(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_PATTERN))

GType        clay_param_pattern_get_type (void);

GParamSpec * clay_param_spec_pattern (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      ClayPattern  *default_value,
                                      gboolean      default_to_context,
                                      GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_GRADIENT
 */

#define CLAY_VALUE_HOLDS_GRADIENT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                            CLAY_TYPE_GRADIENT))

#define CLAY_TYPE_PARAM_GRADIENT           (clay_param_gradient_get_type ())
#define CLAY_IS_PARAM_SPEC_GRADIENT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_GRADIENT))

GType        clay_param_gradient_get_type (void);

GParamSpec * clay_param_spec_gradient (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       ClayGradient *default_value,
                                       gboolean      default_to_context,
                                       GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_PALETTE
 */

#define CLAY_VALUE_HOLDS_PALETTE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           CLAY_TYPE_PALETTE))

#define CLAY_TYPE_PARAM_PALETTE           (clay_param_palette_get_type ())
#define CLAY_IS_PARAM_SPEC_PALETTE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_PALETTE))

GType        clay_param_palette_get_type (void);

GParamSpec * clay_param_spec_palette (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      ClayPalette  *default_value,
                                      gboolean      default_to_context,
                                      GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_FONT
 */

#define CLAY_VALUE_HOLDS_FONT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                        CLAY_TYPE_FONT))

#define CLAY_TYPE_PARAM_FONT           (clay_param_font_get_type ())
#define CLAY_IS_PARAM_SPEC_FONT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_FONT))

GType        clay_param_font_get_type (void);

GParamSpec * clay_param_spec_font (const gchar  *name,
                                   const gchar  *nick,
                                   const gchar  *blurb,
                                   gboolean      none_ok,
                                   ClayFont     *default_value,
                                   gboolean      default_to_context,
                                   GParamFlags   flags);


/*
 * CLAY_TYPE_PARAM_CURVE
 */

#define CLAY_VALUE_HOLDS_CURVE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                         CLAY_TYPE_CURVE))

#define CLAY_TYPE_PARAM_CURVE           (clay_param_curve_get_type ())
#define CLAY_IS_PARAM_SPEC_CURVE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_CURVE))

GType        clay_param_curve_get_type (void);

GParamSpec * clay_param_spec_curve (const gchar  *name,
                                    const gchar  *nick,
                                    const gchar  *blurb,
                                    gboolean      none_ok,
                                    GParamFlags   flags);


G_END_DECLS

#endif  /*  __LIBCLAY_CLAY_PARAM_SPECS_H__  */
