/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowdeviceinfo.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claydeviceinfo.h"
#include "clayrowdeviceinfo.h"


struct _ClayRowDeviceInfo
{
  ClayRow  parent_instance;
};


static void   clay_row_device_info_name_changed (ClayRow *row);


G_DEFINE_TYPE (ClayRowDeviceInfo,
               clay_row_device_info,
               CLAY_TYPE_ROW)

#define parent_class clay_row_device_info_parent_class


static void
clay_row_device_info_class_init (ClayRowDeviceInfoClass *klass)
{
  ClayRowClass *row_class = CLAY_ROW_CLASS (klass);

  row_class->name_changed = clay_row_device_info_name_changed;
}

static void
clay_row_device_info_init (ClayRowDeviceInfo *row)
{
}

static void
clay_row_device_info_name_changed (ClayRow *row)
{
  ClayViewable *viewable;

  CLAY_ROW_CLASS (parent_class)->name_changed (row);

  viewable = clay_row_get_viewable (row);

  if (viewable)
    {
      ClayDeviceInfo *info = CLAY_DEVICE_INFO (viewable);

      gtk_widget_set_sensitive (_clay_row_get_label (row),
                                clay_device_info_get_device (info, NULL) != NULL);
    }
}
