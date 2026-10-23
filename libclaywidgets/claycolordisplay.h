/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolordisplay.c
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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

#ifndef __CLAY_COLOR_DISPLAY_H__
#define __CLAY_COLOR_DISPLAY_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define CLAY_TYPE_COLOR_DISPLAY (clay_color_display_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayColorDisplay, clay_color_display, CLAY, COLOR_DISPLAY, GObject)


struct _ClayColorDisplayClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */
  void        (* convert_buffer) (ClayColorDisplay *display,
                                  GeglBuffer       *buffer,
                                  GeglRectangle    *area);
  GtkWidget * (* configure)      (ClayColorDisplay *display);

  /*  signals  */
  void        (* changed)        (ClayColorDisplay *display);

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


ClayColorDisplay * clay_color_display_clone           (ClayColorDisplay *display);

void               clay_color_display_convert_buffer  (ClayColorDisplay *display,
                                                       GeglBuffer       *buffer,
                                                       GeglRectangle    *area);
void               clay_color_display_load_state      (ClayColorDisplay *display,
                                                       ClayParasite     *state);
ClayParasite     * clay_color_display_save_state      (ClayColorDisplay *display);
GtkWidget        * clay_color_display_configure       (ClayColorDisplay *display);
void               clay_color_display_configure_reset (ClayColorDisplay *display);

void               clay_color_display_changed         (ClayColorDisplay *display);

void               clay_color_display_set_enabled     (ClayColorDisplay *display,
                                                       gboolean          enabled);
gboolean           clay_color_display_get_enabled     (ClayColorDisplay *display);

ClayColorConfig  * clay_color_display_get_config      (ClayColorDisplay *display);
ClayColorManaged * clay_color_display_get_managed     (ClayColorDisplay *display);


G_END_DECLS

#endif /* __CLAY_COLOR_DISPLAY_H__ */
