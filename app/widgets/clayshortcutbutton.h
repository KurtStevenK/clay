/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayshortcutbutton.h
 * Copyright (C) 2022 Jehan
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

#pragma once


#define CLAY_TYPE_SHORTCUT_BUTTON            (clay_shortcut_button_get_type ())
#define CLAY_SHORTCUT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SHORTCUT_BUTTON, ClayShortcutButton))
#define CLAY_SHORTCUT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SHORTCUT_BUTTON, ClayShortcutButtonClass))
#define CLAY_IS_SHORTCUT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SHORTCUT_BUTTON))
#define CLAY_IS_SHORTCUT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SHORTCUT_BUTTON))
#define CLAY_SHORTCUT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SHORTCUT_BUTTON, ClayShortcutButtonClass))


typedef struct _ClayShortcutButtonPrivate ClayShortcutButtonPrivate;
typedef struct _ClayShortcutButtonClass   ClayShortcutButtonClass;

struct _ClayShortcutButton
{
  GtkToggleButton            parent_instance;

  ClayShortcutButtonPrivate *priv;
};

struct _ClayShortcutButtonClass
{
  GtkToggleButtonClass       parent_class;

  void                    (* accelerator_changed)    (ClayShortcutButton *button,
                                                      const gchar        *previous_accelerator);
};


GType          clay_shortcut_button_get_type         (void);

GtkWidget    * clay_shortcut_button_new              (const gchar        *accelerator);

gchar        * clay_shortcut_button_get_accelerator  (ClayShortcutButton  *button);
void           clay_shortcut_button_get_keys         (ClayShortcutButton  *button,
                                                      guint               *keyval,
                                                      GdkModifierType     *modifiers);
void           clay_shortcut_button_set_accelerator  (ClayShortcutButton  *button,
                                                      const gchar         *accelerator,
                                                      guint                keyval,
                                                      GdkModifierType      modifiers);

void           clay_shortcut_button_accepts_modifier (ClayShortcutButton *button,
                                                      gboolean            only,
                                                      gboolean            unique);
