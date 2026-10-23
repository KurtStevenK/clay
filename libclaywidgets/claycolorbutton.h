/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorbutton.h
 * Copyright (C) 1999-2001 Sven Neumann
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

/* This provides a button with a color preview. The preview
 * can handle transparency by showing the checkerboard.
 * On click, a color selector is opened, which is already
 * fully functional wired to the preview button.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_BUTTON_H__
#define __CLAY_COLOR_BUTTON_H__

#include <libclaywidgets/claybutton.h>

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_BUTTON (clay_color_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayColorButton, clay_color_button, CLAY, COLOR_BUTTON, ClayButton)

struct _ClayColorButtonClass
{
  ClayButtonClass  parent_class;

  /*  signals  */
  void  (* color_changed)   (ClayColorButton *button);

  /*  virtual functions  */
  GType (* get_action_type) (ClayColorButton *button);

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


GtkWidget    * clay_color_button_new              (const gchar       *title,
                                                   gint               width,
                                                   gint               height,
                                                   GeglColor         *color,
                                                   ClayColorAreaType  type);

void           clay_color_button_set_title        (ClayColorButton   *button,
                                                   const gchar       *title);
const gchar  * clay_color_button_get_title        (ClayColorButton   *button);

void           clay_color_button_set_color        (ClayColorButton   *button,
                                                   GeglColor         *color);
GeglColor    * clay_color_button_get_color        (ClayColorButton   *button);

gboolean       clay_color_button_has_alpha        (ClayColorButton   *button);
void           clay_color_button_set_type         (ClayColorButton   *button,
                                                   ClayColorAreaType  type);

gboolean       clay_color_button_get_update       (ClayColorButton   *button);
void           clay_color_button_set_update       (ClayColorButton   *button,
                                                   gboolean           continuous);

void           clay_color_button_set_color_config (ClayColorButton   *button,
                                                   ClayColorConfig   *config);

GSimpleActionGroup * clay_color_button_get_action_group   (ClayColorButton   *button);


G_END_DECLS

#endif /* __CLAY_COLOR_BUTTON_H__ */
