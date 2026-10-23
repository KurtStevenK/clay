/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core/claytoolpreset.h"


typedef struct _ClayDeviceKey ClayDeviceKey;

struct _ClayDeviceKey
{
  guint           keyval;
  GdkModifierType modifiers;
};


#define CLAY_TYPE_DEVICE_INFO            (clay_device_info_get_type ())
#define CLAY_DEVICE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DEVICE_INFO, ClayDeviceInfo))
#define CLAY_DEVICE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DEVICE_INFO, ClayDeviceInfoClass))
#define CLAY_IS_DEVICE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DEVICE_INFO))
#define CLAY_IS_DEVICE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DEVICE_INFO))
#define CLAY_DEVICE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DEVICE_INFO, ClayDeviceInfoClass))


typedef struct _ClayDeviceInfoPrivate ClayDeviceInfoPrivate;
typedef struct _ClayDeviceInfoClass   ClayDeviceInfoClass;

struct _ClayDeviceInfo
{
  ClayToolPreset  parent_instance;

  ClayDeviceInfoPrivate *priv;
};

struct _ClayDeviceInfoClass
{
  ClayToolPresetClass  parent_class;
};


GType             clay_device_info_get_type             (void);

ClayDeviceInfo  * clay_device_info_new                  (Clay            *clay,
                                                         GdkDevice       *device,
                                                         GdkDisplay      *display);

GdkDevice       * clay_device_info_get_device           (ClayDeviceInfo  *info,
                                                         GdkDisplay     **display);
gboolean          clay_device_info_set_device           (ClayDeviceInfo  *info,
                                                         GdkDevice       *device,
                                                         GdkDisplay      *display);

void              clay_device_info_set_default_tool     (ClayDeviceInfo  *info);

void              clay_device_info_save_tool            (ClayDeviceInfo  *info);
void              clay_device_info_restore_tool         (ClayDeviceInfo  *info);

GdkInputMode      clay_device_info_get_mode             (ClayDeviceInfo  *info);
void              clay_device_info_set_mode             (ClayDeviceInfo  *info,
                                                         GdkInputMode     mode);

gboolean          clay_device_info_has_cursor           (ClayDeviceInfo  *info);

GdkInputSource    clay_device_info_get_source           (ClayDeviceInfo  *info);

const gchar     * clay_device_info_get_vendor_id        (ClayDeviceInfo  *info);
const gchar     * clay_device_info_get_product_id       (ClayDeviceInfo  *info);

GdkDeviceToolType clay_device_info_get_tool_type        (ClayDeviceInfo  *info);
guint64           clay_device_info_get_tool_serial      (ClayDeviceInfo  *info);
guint64           clay_device_info_get_tool_hardware_id (ClayDeviceInfo  *info);

gint             clay_device_info_get_n_axes            (ClayDeviceInfo  *info);
const gchar    * clay_device_info_get_axis_name         (ClayDeviceInfo  *info,
                                                         gint             axis);
GdkAxisUse       clay_device_info_get_axis_use          (ClayDeviceInfo  *info,
                                                         gint             axis);
void             clay_device_info_set_axis_use          (ClayDeviceInfo  *info,
                                                         gint             axis,
                                                         GdkAxisUse       use);

gint             clay_device_info_get_n_keys            (ClayDeviceInfo  *info);
void             clay_device_info_get_key               (ClayDeviceInfo  *info,
                                                         gint             key,
                                                         guint           *keyval,
                                                         GdkModifierType *modifiers);
void             clay_device_info_set_key               (ClayDeviceInfo  *info,
                                                         gint             key,
                                                         guint            keyval,
                                                         GdkModifierType  modifiers);

ClayCurve      * clay_device_info_get_curve             (ClayDeviceInfo  *info,
                                                         GdkAxisUse       use);
gdouble          clay_device_info_map_axis              (ClayDeviceInfo  *info,
                                                         GdkAxisUse       use,
                                                         gdouble          value);

void             clay_device_info_changed               (ClayDeviceInfo  *info);

ClayDeviceInfo * clay_device_info_get_by_device         (GdkDevice       *device);

gint             clay_device_info_compare               (ClayDeviceInfo  *a,
                                                         ClayDeviceInfo  *b);

ClayPadActions * clay_device_info_get_pad_actions       (ClayDeviceInfo  *info);

GtkPadController * clay_device_info_create_pad_controller (ClayDeviceInfo *info,
                                                           ClayWindow     *window);
