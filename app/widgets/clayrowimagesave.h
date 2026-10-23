/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowimagesave.h
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "clayrowimage.h"


#define CLAY_TYPE_ROW_IMAGE_SAVE (clay_row_image_save_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayRowImageSave,
                          clay_row_image_save,
                          CLAY, ROW_IMAGE_SAVE,
                          ClayRowImage)


struct _ClayRowImageSaveClass
{
  ClayRowImageClass  parent_class;
};
