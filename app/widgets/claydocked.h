/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claydocked.h
 * Copyright (C) 2003  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_DOCKED (clay_docked_get_type ())
G_DECLARE_INTERFACE (ClayDocked,
                     clay_docked,
                     CLAY, DOCKED,
                     GtkWidget)


/**
 * ClayDockedInterface:
 *
 * Interface with common methods for stuff that is docked.
 */
struct _ClayDockedInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void            (* title_changed)       (ClayDocked   *docked);

  /*  virtual functions  */
  void            (* set_aux_info)        (ClayDocked   *docked,
                                           GList        *aux_info);
  GList         * (* get_aux_info)        (ClayDocked   *docked);

  GtkWidget     * (* get_preview)         (ClayDocked   *docked,
                                           ClayContext  *context,
                                           GtkIconSize   size);
  gboolean        (* get_prefer_icon)     (ClayDocked   *docked);
  ClayUIManager * (* get_menu)            (ClayDocked   *docked,
                                           const gchar **ui_path,
                                           gpointer     *popup_data);
  gchar         * (* get_title)           (ClayDocked   *docked);

  void            (* set_context)         (ClayDocked   *docked,
                                           ClayContext  *context);

  gboolean        (* has_button_bar)      (ClayDocked   *docked);
  void            (* set_show_button_bar) (ClayDocked   *docked,
                                           gboolean      show);
  gboolean        (* get_show_button_bar) (ClayDocked   *docked);
};


void            clay_docked_title_changed       (ClayDocked   *docked);

void            clay_docked_set_aux_info        (ClayDocked   *docked,
                                                 GList        *aux_info);
GList         * clay_docked_get_aux_info        (ClayDocked   *docked);

GtkWidget     * clay_docked_get_preview         (ClayDocked   *docked,
                                                 ClayContext  *context,
                                                 GtkIconSize   size);
gboolean        clay_docked_get_prefer_icon     (ClayDocked   *docked);
ClayUIManager * clay_docked_get_menu            (ClayDocked   *docked,
                                                 const gchar **ui_path,
                                                 gpointer     *popup_data);
gchar         * clay_docked_get_title           (ClayDocked   *docked);

void            clay_docked_set_context         (ClayDocked   *docked,
                                                 ClayContext  *context);

gboolean        clay_docked_has_button_bar      (ClayDocked   *docked);
void            clay_docked_set_show_button_bar (ClayDocked   *docked,
                                                 gboolean      show);
gboolean        clay_docked_get_show_button_bar (ClayDocked   *docked);
