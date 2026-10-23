/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayplugin.h
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

#ifndef __CLAY_PLUG_IN_H__
#define __CLAY_PLUG_IN_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_PLUG_IN_ERROR           (clay_plug_in_error_quark ())

#define CLAY_TYPE_PLUG_IN (clay_plug_in_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayPlugIn, clay_plug_in, CLAY, PLUG_IN, GObject)


/**
 * ClayPlugInClass:
 *
 * A class which every plug-in should subclass, while overriding
 * [vfunc@PlugIn.query_procedures] and/or [vfunc@PlugIn.init_procedures], as
 * well as [vfunc@PlugIn.create_procedure].
 *
 * Since: 3.0
 **/
struct _ClayPlugInClass
{
  GObjectClass  parent_class;

  /**
   * ClayPlugInClass::query_procedures:
   * @plug_in: a #ClayPlugIn.
   *
   * This method can be overridden by all plug-ins to return a newly allocated
   * list of allocated strings naming the procedures registered by this
   * plug-in. See documentation of [vfunc@PlugIn.init_procedures] for
   * differences.
   *
   * Returns: (element-type gchar*) (transfer full): the names of the procedures registered by @plug_in.
   */
  GList          * (* query_procedures) (ClayPlugIn  *plug_in);

  /**
   * ClayPlugInClass::init_procedures:
   * @plug_in: a #ClayPlugIn.
   *
   * This method can be overridden by all plug-ins to return a newly allocated
   * list of allocated strings naming procedures registered by this plug-in.
   * It is different from [vfunc@PlugIn.query_procedures] in that init happens
   * at every startup, whereas query happens only once in the life of a plug-in
   * (right after installation or update). Hence [vfunc@PlugIn.init_procedures]
   * typically returns procedures dependent to runtime conditions (such as the
   * presence of a third-party tool), whereas [vfunc@PlugIn.query_procedures]
   * would usually return procedures that are always available unconditionally.
   *
   * Most of the time, you only want to override
   * [vfunc@PlugIn.query_procedures] and leave [vfunc@PlugIn.init_procedures]
   * untouched.
   *
   * Returns: (element-type gchar*) (transfer full): the names of the procedures registered by @plug_in.
   **/
  GList          * (* init_procedures)  (ClayPlugIn  *plug_in);

  /**
   * ClayPlugInClass::create_procedure:
   * @plug_in:        a #ClayPlugIn.
   * @procedure_name: procedure name.
   *
   * This method must be overridden by all plug-ins and return a newly
   * allocated #ClayProcedure named @name.
   *
   * This method will be called for every @name as returned by
   * [vfunc@PlugIn.query_procedures] and [vfunc@PlugIn.init_procedures] so care
   * must be taken to handle them all.  Upon procedure registration,
   * [vfunc@PlugIn.create_procedure] will be called in the order of the lists
   * returned by [vfunc@PlugIn.query_procedures] and
   * [vfunc@PlugIn.init_procedures]
   *
   * Returns: (transfer full): the procedure to be registered or executed by @plug_in.
   */
  ClayProcedure  * (* create_procedure) (ClayPlugIn  *plug_in,
                                         const gchar *procedure_name);

  /**
   * ClayPlugInClass::quit:
   * @plug_in: a #ClayPlugIn.
   *
   * This method can be overridden by a plug-in which needs to perform some
   * actions upon quitting.
   */
  void             (* quit)             (ClayPlugIn  *plug_in);

  /**
   * ClayPlugInClass::set_i18n:
   * @plug_in:        a #ClayPlugIn.
   * @procedure_name: procedure name.
   * @gettext_domain: (out) (nullable): Gettext domain. If %NULL, it
   *                  defaults to the plug-in name as determined by the
   *                  directory the binary is called from.
   * @catalog_dir:    (out) (nullable) (type utf8): relative path to a
   *                  subdirectory of the plug-in folder containing the compiled
   *                  Gettext message catalogs. If %NULL, it defaults to
   *                  "locale/".
   *
   * This method can be overridden by all plug-ins to customize
   * internationalization of the plug-in.
   *
   * This method will be called before initializing, querying or running
   * @procedure_name (respectively with [vfunc@PlugIn.init_procedures],
   * [vfunc@PlugIn.query_procedures] or with the `run()` function set in
   * `clay_image_procedure_new()`).
   *
   * By default, Clay plug-ins look up gettext compiled message catalogs
   * in the subdirectory `locale/` under the plug-in folder (same folder
   * as `clay_get_progname()`) with a text domain equal to the plug-in
   * name (regardless @procedure_name). It is unneeded to override this
   * method if you follow this localization scheme.
   *
   * If you wish to disable localization or localize with another system,
   * simply set the method to %NULL, or possibly implement this method
   * to do something useful for your usage while returning %FALSE.
   *
   * If you wish to tweak the @gettext_domain or the @catalog_dir, return
   * %TRUE and allocate appropriate @gettext_domain and/or @catalog_dir
   * (these use the default if set %NULL).
   *
   * Note that @catalog_dir must be a relative path, encoded as UTF-8,
   * subdirectory of the directory of `clay_get_progname()`.
   * The domain names "clay30-std-plug-ins", "clay30-script-fu" and
   * "clay30-python" are reserved and can only be used with a %NULL
   * @catalog_dir. These will use the translation catalogs installed for
   * core plug-ins, so you are not expected to use these for your
   * plug-ins, except if you are making a core plug-in. More domain
   * names may become reserved so we discourage using a gettext domain
   * starting with "clay30-".
   *
   * When localizing your plug-in this way, Clay also binds
   * @gettext_domain to the UTF-8 encoding.
   *
   * Returns: %TRUE if this plug-in will use Gettext localization. You
   *          may return %FALSE if you wish to disable localization or
   *          set it up differently.
   *
   * Since: 3.0
   */
  gboolean         (* set_i18n)         (ClayPlugIn   *plug_in,
                                         const gchar  *procedure_name,
                                         gchar       **gettext_domain,
                                         gchar       **catalog_dir);

  /* Padding for future expansion */
  /*< private >*/
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


GQuark          clay_plug_in_error_quark            (void);

void            clay_plug_in_set_help_domain        (ClayPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_uri);

CLAY_DEPRECATED_FOR(clay_procedure_add_menu_path)
void            clay_plug_in_add_menu_branch        (ClayPlugIn    *plug_in,
                                                     const gchar   *menu_path,
                                                     const gchar   *menu_label);

void            clay_plug_in_add_temp_procedure     (ClayPlugIn    *plug_in,
                                                     ClayProcedure *procedure);
void            clay_plug_in_remove_temp_procedure  (ClayPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

GList         * clay_plug_in_get_temp_procedures    (ClayPlugIn    *plug_in);
ClayProcedure * clay_plug_in_get_temp_procedure     (ClayPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

void            clay_plug_in_persistent_enable      (ClayPlugIn    *plug_in);
void            clay_plug_in_persistent_process     (ClayPlugIn    *plug_in,
                                                     guint          timeout);

void            clay_plug_in_set_pdb_error_handler  (ClayPlugIn    *plug_in,
                                                     ClayPDBErrorHandler  handler);
ClayPDBErrorHandler
                clay_plug_in_get_pdb_error_handler  (ClayPlugIn    *plug_in);


G_END_DECLS

#endif /* __CLAY_PLUG_IN_H__ */
