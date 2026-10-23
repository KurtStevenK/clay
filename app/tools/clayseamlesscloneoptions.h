/* xyz.kainzmayer.ClayDesign
 *
 * clayseamlesscloneoptions.h
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#include "core/claytooloptions.h"


#define CLAY_TYPE_SEAMLESS_CLONE_OPTIONS            (clay_seamless_clone_options_get_type ())
#define CLAY_SEAMLESS_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SEAMLESS_CLONE_OPTIONS, ClaySeamlessCloneOptions))
#define CLAY_SEAMLESS_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SEAMLESS_CLONE_OPTIONS, ClaySeamlessCloneOptionsClass))
#define CLAY_IS_SEAMLESS_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SEAMLESS_CLONE_OPTIONS))
#define CLAY_IS_SEAMLESS_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SEAMLESS_CLONE_OPTIONS))
#define CLAY_SEAMLESS_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SEAMLESS_CLONE_OPTIONS, ClaySeamlessCloneOptionsClass))


typedef struct _ClaySeamlessCloneOptions      ClaySeamlessCloneOptions;
typedef struct _ClaySeamlessCloneOptionsClass ClaySeamlessCloneOptionsClass;

struct _ClaySeamlessCloneOptions
{
  ClayToolOptions parent_instance;

  gint            max_refine_scale;
  gboolean        temp;
};

struct _ClaySeamlessCloneOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType       clay_seamless_clone_options_get_type (void);

GtkWidget * clay_seamless_clone_options_gui      (ClayToolOptions *tool_options);
