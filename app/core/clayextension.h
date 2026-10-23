/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayextension.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include "clayobject.h"


#define CLAY_TYPE_EXTENSION            (clay_extension_get_type ())
#define CLAY_EXTENSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EXTENSION, ClayExtension))
#define CLAY_EXTENSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EXTENSION, ClayExtensionClass))
#define CLAY_IS_EXTENSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EXTENSION))
#define CLAY_IS_EXTENSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EXTENSION))
#define CLAY_EXTENSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_EXTENSION, ClayExtensionClass))

typedef struct _ClayExtensionPrivate  ClayExtensionPrivate;
typedef struct _ClayExtensionClass    ClayExtensionClass;

struct _ClayExtension
{
  ClayObject            parent_instance;

  ClayExtensionPrivate *p;
};

struct _ClayExtensionClass
{
  ClayObjectClass       parent_class;
};


GType           clay_extension_get_type (void);

ClayExtension * clay_extension_new                     (const gchar    *dir,
                                                        gboolean        writable);
const gchar   * clay_extension_get_name                (ClayExtension  *extension);
const gchar   * clay_extension_get_comment             (ClayExtension  *extension);
const gchar   * clay_extension_get_description         (ClayExtension  *extension);
GdkPixbuf     * clay_extension_get_screenshot          (ClayExtension  *extension,
                                                        gint            width,
                                                        gint            height,
                                                        const gchar   **caption);
const gchar   * clay_extension_get_path                (ClayExtension  *extension);

gchar         * clay_extension_get_markup_description  (ClayExtension  *extension);

gboolean        clay_extension_load                    (ClayExtension  *extension,
                                                        GError        **error);
gboolean        clay_extension_run                     (ClayExtension  *extension,
                                                        GError        **error);
void            clay_extension_stop                    (ClayExtension  *extension);

GList         * clay_extension_get_brush_paths         (ClayExtension  *extension);
GList         * clay_extension_get_dynamics_paths      (ClayExtension  *extension);
GList         * clay_extension_get_mypaint_brush_paths (ClayExtension  *extension);
GList         * clay_extension_get_pattern_paths       (ClayExtension  *extension);
GList         * clay_extension_get_gradient_paths      (ClayExtension  *extension);
GList         * clay_extension_get_palette_paths       (ClayExtension  *extension);
GList         * clay_extension_get_tool_preset_paths   (ClayExtension  *extension);
GList         * clay_extension_get_splash_paths        (ClayExtension  *extension);
GList         * clay_extension_get_theme_paths         (ClayExtension  *extension);
GList         * clay_extension_get_plug_in_paths       (ClayExtension  *extension);

gint            clay_extension_cmp                     (ClayExtension  *extension1,
                                                        ClayExtension  *extension2);
gint            clay_extension_id_cmp                  (ClayExtension  *extension1,
                                                        const gchar    *id);
