/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ClayColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_COLOR_H_INSIDE__) && !defined (CLAY_COLOR_COMPILATION)
#error "Only <libclaycolor/claycolor.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_MANAGED_H__
#define __CLAY_COLOR_MANAGED_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_COLOR_MANAGED (clay_color_managed_get_type ())
G_DECLARE_INTERFACE (ClayColorManaged, clay_color_managed, CLAY, COLOR_MANAGED, GObject)

/**
 * ClayColorManagedInterface:
 * @base_iface: The parent interface
 * @get_icc_profile: Returns the ICC profile of the pixels managed by
 *                   the object
 * @profile_changed: This signal is emitted when the object's color profile
 *                   has changed
 * @get_color_profile: Returns the #ClayColorProfile of the pixels managed
 *                     by the object
 * @get_simulation_profile: Returns the simulation #ClayColorProfile of the
 *                          pixels managed by the object
 * @get_simulation_rendering_intent: Returns the simulation #ClayColorRenderingIntent
 *                                   of the pixels managed by the object
 * @get_simulation_bpc: Returns whether black point compensation is enabled for the
 *                      simulation of the pixels managed by the object
 **/
struct _ClayColorManagedInterface
{
  GTypeInterface  base_iface;

  /**
   * ClayColorManagedInterface::get_icc_profile:
   * @managed: an object the implements the #ClayColorManaged interface
   * @len: (out): return location for the number of bytes in the profile data
   *
   * Returns: (array length=len): A blob of data that represents an ICC color
   *                              profile.
   *
   * Since: 2.4
   */
  const guint8     * (* get_icc_profile)                     (ClayColorManaged *managed,
                                                              gsize            *len);

  /*  signals  */
  void               (* profile_changed)                     (ClayColorManaged *managed);

  void               (* simulation_profile_changed)          (ClayColorManaged *managed);

  void               (* simulation_intent_changed)           (ClayColorManaged *managed);

  void               (* simulation_bpc_changed)              (ClayColorManaged *managed);

  /*  virtual functions  */
  ClayColorProfile * (* get_color_profile)               (ClayColorManaged *managed);
  ClayColorProfile * (* get_simulation_profile)          (ClayColorManaged *managed);
  ClayColorRenderingIntent
                     (* get_simulation_intent)           (ClayColorManaged *managed);
  gboolean           (* get_simulation_bpc)              (ClayColorManaged *managed);
};


const guint8     *       clay_color_managed_get_icc_profile            (ClayColorManaged *managed,
                                                                        gsize            *len);
ClayColorProfile *       clay_color_managed_get_color_profile          (ClayColorManaged *managed);

ClayColorProfile *       clay_color_managed_get_simulation_profile     (ClayColorManaged *managed);

ClayColorRenderingIntent clay_color_managed_get_simulation_intent      (ClayColorManaged *managed);

gboolean                 clay_color_managed_get_simulation_bpc         (ClayColorManaged *managed);

void                     clay_color_managed_profile_changed            (ClayColorManaged *managed);

void                     clay_color_managed_simulation_profile_changed (ClayColorManaged *managed);

void                     clay_color_managed_simulation_intent_changed  (ClayColorManaged *managed);

void                     clay_color_managed_simulation_bpc_changed     (ClayColorManaged *managed);


G_END_DECLS

#endif  /* __CLAY_COLOR_MANAGED_IFACE_H__ */
