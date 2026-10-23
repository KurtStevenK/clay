/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselector.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_SELECTOR_H__
#define __CLAY_COLOR_SELECTOR_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * CLAY_COLOR_SELECTOR_SIZE:
 *
 * The suggested size for a color area in a #ClayColorSelector
 * implementation.
 **/
#define CLAY_COLOR_SELECTOR_SIZE     150

/**
 * CLAY_COLOR_SELECTOR_BAR_SIZE:
 *
 * The suggested width for a color bar in a #ClayColorSelector
 * implementation.
 **/
#define CLAY_COLOR_SELECTOR_BAR_SIZE 15


#define CLAY_TYPE_COLOR_SELECTOR (clay_color_selector_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayColorSelector, clay_color_selector, CLAY, COLOR_SELECTOR, GtkBox)

struct _ClayColorSelectorClass
{
  GtkBoxClass  parent_class;

  const gchar *name;
  const gchar *help_id;
  const gchar *icon_name;

  /*  virtual functions  */
  void (* set_toggles_visible)   (ClayColorSelector        *selector,
                                  gboolean                  visible);
  void (* set_toggles_sensitive) (ClayColorSelector        *selector,
                                  gboolean                  sensitive);
  void (* set_show_alpha)        (ClayColorSelector        *selector,
                                  gboolean                  show_alpha);
  void (* set_color)             (ClayColorSelector        *selector,
                                  GeglColor                *color);
  void (* set_channel)           (ClayColorSelector        *selector,
                                  ClayColorSelectorChannel  channel);
  void (* set_model_visible)     (ClayColorSelector        *selector,
                                  ClayColorSelectorModel    model,
                                  gboolean                  visible);
  void (* set_config)            (ClayColorSelector        *selector,
                                  ClayColorConfig          *config);

  void (* set_format)            (ClayColorSelector        *selector,
                                  const Babl               *format);
  void (* set_simulation)        (ClayColorSelector        *selector,
                                  ClayColorProfile         *profile,
                                  ClayColorRenderingIntent  intent,
                                  gboolean                  bpc);

  /*  signals  */
  void (* color_changed)         (ClayColorSelector        *selector,
                                  GeglColor                *color);
  void (* channel_changed)       (ClayColorSelector        *selector,
                                  ClayColorSelectorChannel  channel);
  void (* model_visible_changed) (ClayColorSelector        *selector,
                                  ClayColorSelectorModel    model,
                                  gboolean                  visible);
  void (* simulation)            (ClayColorSelector        *selector,
                                  gboolean                  enabled);

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


GtkWidget * clay_color_selector_new                   (GType              selector_type,
                                                       GeglColor         *color,
                                                       ClayColorSelectorChannel  channel);

void        clay_color_selector_set_toggles_visible   (ClayColorSelector *selector,
                                                       gboolean           visible);
gboolean    clay_color_selector_get_toggles_visible   (ClayColorSelector *selector);

void        clay_color_selector_set_toggles_sensitive (ClayColorSelector *selector,
                                                       gboolean           sensitive);
gboolean    clay_color_selector_get_toggles_sensitive (ClayColorSelector *selector);

void        clay_color_selector_set_show_alpha        (ClayColorSelector *selector,
                                                       gboolean           show_alpha);
gboolean    clay_color_selector_get_show_alpha        (ClayColorSelector *selector);

void        clay_color_selector_set_color             (ClayColorSelector *selector,
                                                       GeglColor         *color);
GeglColor * clay_color_selector_get_color             (ClayColorSelector *selector);

void        clay_color_selector_set_channel           (ClayColorSelector *selector,
                                                       ClayColorSelectorChannel  channel);
ClayColorSelectorChannel
            clay_color_selector_get_channel           (ClayColorSelector *selector);

void        clay_color_selector_set_model_visible     (ClayColorSelector *selector,
                                                       ClayColorSelectorModel model,
                                                       gboolean           visible);
gboolean    clay_color_selector_get_model_visible     (ClayColorSelector *selector,
                                                       ClayColorSelectorModel model);

void        clay_color_selector_set_config            (ClayColorSelector *selector,
                                                       ClayColorConfig   *config);

void        clay_color_selector_set_format            (ClayColorSelector         *selector,
                                                       const Babl                *format);
void        clay_color_selector_set_simulation        (ClayColorSelector         *selector,
                                                       ClayColorProfile          *profile,
                                                       ClayColorRenderingIntent  intent,
                                                       gboolean                   bpc);
gboolean    clay_color_selector_get_simulation        (ClayColorSelector         *selector,
                                                       ClayColorProfile         **profile,
                                                       ClayColorRenderingIntent *intent,
                                                       gboolean                  *bpc);
gboolean    clay_color_selector_enable_simulation     (ClayColorSelector         *selector,
                                                       gboolean                   enabled);


G_END_DECLS

#endif /* __CLAY_COLOR_SELECTOR_H__ */
