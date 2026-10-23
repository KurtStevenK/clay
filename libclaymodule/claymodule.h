/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymodule.h
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#ifndef __CLAY_MODULE_H__
#define __CLAY_MODULE_H__

#include <gio/gio.h>
#include <gmodule.h>

#define __CLAY_MODULE_H_INSIDE__

#include <libclaymodule/claymoduletypes.h>

#include <libclaymodule/claymoduledb.h>

#undef __CLAY_MODULE_H_INSIDE__

G_BEGIN_DECLS


/**
 * CLAY_MODULE_ABI_VERSION:
 *
 * The version of the module system's ABI. Modules put this value into
 * #ClayModuleInfo's @abi_version field so the code loading the modules
 * can check if it was compiled against the same module ABI the modules
 * are compiled against.
 *
 *  CLAY_MODULE_ABI_VERSION is incremented each time one of the
 *  following changes:
 *
 *  - the libclaymodule implementation (if the change affects modules).
 *
 *  - one of the classes implemented by modules (currently #ClayColorDisplay,
 *    #ClayColorSelector and #ClayController).
 **/
#define CLAY_MODULE_ABI_VERSION 0x0005


/**
 * ClayModuleState:
 * @CLAY_MODULE_STATE_ERROR:       Missing clay_module_register() function
 *                                 or other error.
 * @CLAY_MODULE_STATE_LOADED:      An instance of a type implemented by
 *                                 this module is allocated.
 * @CLAY_MODULE_STATE_LOAD_FAILED: clay_module_register() returned %FALSE.
 * @CLAY_MODULE_STATE_NOT_LOADED:  There are no instances allocated of
 *                                 types implemented by this module.
 *
 * The possible states a #ClayModule can be in.
 **/
typedef enum
{
  CLAY_MODULE_STATE_ERROR,
  CLAY_MODULE_STATE_LOADED,
  CLAY_MODULE_STATE_LOAD_FAILED,
  CLAY_MODULE_STATE_NOT_LOADED
} ClayModuleState;


#define CLAY_MODULE_ERROR (clay_module_error_quark ())

GQuark  clay_module_error_quark (void) G_GNUC_CONST;

/**
 * ClayModuleError:
 * @CLAY_MODULE_FAILED: Generic error condition
 *
 * Types of errors returned by modules
 **/
typedef enum
{
  CLAY_MODULE_FAILED
} ClayModuleError;


/**
 * ClayModuleInfo:
 * @abi_version: The #CLAY_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * This structure contains information about a loadable module.
 **/
struct _ClayModuleInfo
{
  guint32  abi_version;
  gchar   *purpose;
  gchar   *author;
  gchar   *version;
  gchar   *copyright;
  gchar   *date;
};


/**
 * ClayModuleQueryFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the query function a loadable Clay module must
 * implement. In the module, the function must be called [func@Module.query].
 *
 * [class@Module] will copy the returned [struct@ModuleInfo], so the
 * module doesn't need to keep these values around (however in most
 * cases the module will just return a pointer to a constant
 * structure).
 *
 * Returns: The info struct describing the module.
 **/
typedef const ClayModuleInfo * (* ClayModuleQueryFunc)    (GTypeModule *module);

/**
 * ClayModuleRegisterFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the register function a loadable Clay module must
 * implement.  In the module, the function must be called
 * [func@Module.register].
 *
 * When this function is called, the module should register all the types
 * it implements with the passed @module.
 *
 * Returns: Whether the registration was successful
 **/
typedef gboolean               (* ClayModuleRegisterFunc) (GTypeModule *module);


/* ClayModules have to implement these */
G_MODULE_EXPORT const ClayModuleInfo * clay_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               clay_module_register (GTypeModule *module);


#define CLAY_TYPE_MODULE (clay_module_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayModule, clay_module, CLAY, MODULE, GTypeModule)

struct _ClayModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (ClayModule *module);

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


ClayModule           * clay_module_new            (GFile           *file,
                                                   gboolean         auto_load,
                                                   gboolean         verbose);

GFile                * clay_module_get_file       (ClayModule      *module);

void                   clay_module_set_auto_load  (ClayModule      *module,
                                                   gboolean         auto_load);
gboolean               clay_module_get_auto_load  (ClayModule      *module);

gboolean               clay_module_is_on_disk     (ClayModule      *module);
gboolean               clay_module_is_loaded      (ClayModule      *module);

const ClayModuleInfo * clay_module_get_info       (ClayModule      *module);
ClayModuleState        clay_module_get_state      (ClayModule      *module);
const gchar          * clay_module_get_last_error (ClayModule      *module);

gboolean               clay_module_query_module   (ClayModule      *module);


G_END_DECLS

#endif  /* __CLAY_MODULE_H__ */
