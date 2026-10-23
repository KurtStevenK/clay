/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayprocedureconfig.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_PROCEDURE_CONFIG_H__
#define __CLAY_PROCEDURE_CONFIG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PROCEDURE_CONFIG (clay_procedure_config_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayProcedureConfig, clay_procedure_config, CLAY, PROCEDURE_CONFIG, GObject)

struct _ClayProcedureConfigClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


ClayProcedure  * clay_procedure_config_get_procedure         (ClayProcedureConfig *config);


void             clay_procedure_config_save_metadata         (ClayProcedureConfig  *config,
                                                              ClayImage            *exported_image,
                                                              GFile                *file);


/* Binding functions */

GObject       ** clay_procedure_config_get_core_object_array (ClayProcedureConfig  *config,
                                                              const gchar          *property_name);
void             clay_procedure_config_set_core_object_array (ClayProcedureConfig  *config,
                                                              const gchar          *property_name,
                                                              GObject             **objects,
                                                              gsize                 n_objects);

GeglColor     ** clay_procedure_config_get_color_array       (ClayProcedureConfig  *config,
                                                              const gchar          *property_name);
void             clay_procedure_config_set_color_array       (ClayProcedureConfig  *config,
                                                              const gchar          *property_name,
                                                              GeglColor           **colors,
                                                              gsize                 n_colors);

/* Utility functions */

gint             clay_procedure_config_get_choice_id         (ClayProcedureConfig  *config,
                                                              const gchar          *property_name);


G_END_DECLS

#endif /* __CLAY_PROCEDURE_CONFIG_H__ */
