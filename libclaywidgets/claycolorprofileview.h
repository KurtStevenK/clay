/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayColorProfileView
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_COLOR_PROFILE_VIEW_H__
#define __CLAY_COLOR_PROFILE_VIEW_H__

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_PROFILE_VIEW (clay_color_profile_view_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorProfileView, clay_color_profile_view, CLAY, COLOR_PROFILE_VIEW, GtkTextView)


GtkWidget * clay_color_profile_view_new         (void);

void        clay_color_profile_view_set_profile (ClayColorProfileView *view,
                                                 ClayColorProfile     *profile);
void        clay_color_profile_view_set_error   (ClayColorProfileView *view,
                                                 const gchar          *message);

G_END_DECLS

#endif /* __CLAY_COLOR_PROFILE_VIEW_H__ */
