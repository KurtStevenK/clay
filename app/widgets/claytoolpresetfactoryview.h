/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytool_presetfactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "claydatafactoryview.h"


#define CLAY_TYPE_TOOL_PRESET_FACTORY_VIEW (clay_tool_preset_factory_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayToolPresetFactoryView,
                          clay_tool_preset_factory_view,
                          CLAY, TOOL_PRESET_FACTORY_VIEW,
                          ClayDataFactoryView)


struct _ClayToolPresetFactoryViewClass
{
  ClayDataFactoryViewClass  parent_class;
};


GtkWidget * clay_tool_preset_factory_view_new (ClayViewType     view_type,
                                               ClayDataFactory *factory,
                                               ClayContext     *context,
                                               gint             view_size,
                                               gint             view_border_width,
                                               ClayMenuFactory *menu_factory);
