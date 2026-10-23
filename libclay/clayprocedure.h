/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprocedure.h
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

#ifndef __CLAY_PROCEDURE_H__
#define __CLAY_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayRunFunc:
 * @procedure: the #ClayProcedure that runs.
 * @config:    the @procedure's arguments in a config object.
 * @run_data: (closure): the run_data given in clay_procedure_new().
 *
 * The run function is run during the lifetime of the Clay session,
 * each time a plug-in procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef ClayValueArray * (* ClayRunFunc) (ClayProcedure       *procedure,
                                          ClayProcedureConfig *config,
                                          gpointer             run_data);


/**
 * ClayArgumentSync:
 * @CLAY_ARGUMENT_SYNC_NONE:     Don't sync this argument
 * @CLAY_ARGUMENT_SYNC_PARASITE: Sync this argument with an image parasite
 *
 * Methods of syncing procedure arguments.
 *
 * Since: 3.0
 **/
typedef enum
{
  CLAY_ARGUMENT_SYNC_NONE,
  CLAY_ARGUMENT_SYNC_PARASITE
} ClayArgumentSync;


#define CLAY_TYPE_PROCEDURE (clay_procedure_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayProcedure, clay_procedure, CLAY, PROCEDURE, GObject)


/**
 * ClayProcedureClass:
 * @install: called to install the procedure with the main Clay
 *   application. This is an implementation detail and must never
 *   be called by any plug-in code.
 * @uninstall: called to uninstall the procedure from the main Clay
 *   application. This is an implementation detail and must never
 *   be called by any plug-in code.
 * @run: called when the procedure is executed via clay_procedure_run().
 *   the default implementation simply calls the procedure's #ClayRunFunc,
 *   #ClayProcedure subclasses are free to modify the passed @args and
 *   call their own, subclass-specific run functions.
 * @create_config: called when a #ClayConfig object is created using
 *   clay_procedure_create_config().
 *
 * Since: 3.0
 **/
struct _ClayProcedureClass
{
  GObjectClass parent_class;

  void                  (* install)         (ClayProcedure         *procedure);
  void                  (* uninstall)       (ClayProcedure         *procedure);

  ClayValueArray      * (* run)             (ClayProcedure         *procedure,
                                             const ClayValueArray  *args);

  ClayProcedureConfig * (* create_config)   (ClayProcedure         *procedure,
                                             GParamSpec           **args,
                                             gint                   n_args);

  gboolean              (* set_sensitivity) (ClayProcedure         *procedure,
                                             gint                   sensitivity_mask);

  /* Padding for future expansion */
  /*< private >*/
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


ClayProcedure  * clay_procedure_new                (ClayPlugIn           *plug_in,
                                                    const gchar          *name,
                                                    ClayPDBProcType       proc_type,
                                                    ClayRunFunc           run_func,
                                                    gpointer              run_data,
                                                    GDestroyNotify        run_data_destroy);

ClayPlugIn     * clay_procedure_get_plug_in        (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_name           (ClayProcedure        *procedure);
ClayPDBProcType  clay_procedure_get_proc_type      (ClayProcedure        *procedure);

void             clay_procedure_set_image_types    (ClayProcedure        *procedure,
                                                    const gchar          *image_types);
const gchar    * clay_procedure_get_image_types    (ClayProcedure        *procedure);

void           clay_procedure_set_sensitivity_mask (ClayProcedure        *procedure,
                                                    gint                  sensitivity_mask);
gint           clay_procedure_get_sensitivity_mask (ClayProcedure        *procedure);


void             clay_procedure_set_menu_label     (ClayProcedure        *procedure,
                                                    const gchar          *menu_label);
const gchar    * clay_procedure_get_menu_label     (ClayProcedure        *procedure);

void             clay_procedure_add_menu_path      (ClayProcedure        *procedure,
                                                    const gchar          *menu_path);
GList          * clay_procedure_get_menu_paths     (ClayProcedure        *procedure);

void             clay_procedure_set_icon_name      (ClayProcedure        *procedure,
                                                    const gchar          *icon_name);
void             clay_procedure_set_icon_file      (ClayProcedure        *procedure,
                                                    GFile                *file);
void             clay_procedure_set_icon_pixbuf    (ClayProcedure        *procedure,
                                                    GdkPixbuf            *pixbuf);

ClayIconType     clay_procedure_get_icon_type      (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_icon_name      (ClayProcedure        *procedure);
GFile          * clay_procedure_get_icon_file      (ClayProcedure        *procedure);
GdkPixbuf      * clay_procedure_get_icon_pixbuf    (ClayProcedure        *procedure);

void             clay_procedure_set_documentation  (ClayProcedure        *procedure,
                                                    const gchar          *blurb,
                                                    const gchar          *help,
                                                    const gchar          *help_id);
const gchar    * clay_procedure_get_blurb          (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_help           (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_help_id        (ClayProcedure        *procedure);
void             clay_procedure_set_help_uri       (ClayProcedure        *procedure,
                                                    const gchar          *uri_reference);
GFile          * clay_procedure_get_help_file      (ClayProcedure        *procedure);

void             clay_procedure_set_attribution    (ClayProcedure        *procedure,
                                                    const gchar          *authors,
                                                    const gchar          *copyright,
                                                    const gchar          *date);
const gchar    * clay_procedure_get_authors        (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_copyright      (ClayProcedure        *procedure);
const gchar    * clay_procedure_get_date           (ClayProcedure        *procedure);

GParamSpec     * clay_procedure_find_argument      (ClayProcedure        *procedure,
                                                    const gchar          *name);
GParamSpec     * clay_procedure_find_aux_argument  (ClayProcedure        *procedure,
                                                    const gchar          *name);
GParamSpec     * clay_procedure_find_return_value  (ClayProcedure        *procedure,
                                                    const gchar          *name);

GParamSpec    ** clay_procedure_get_arguments      (ClayProcedure        *procedure,
                                                    gint                 *n_arguments);
GParamSpec    ** clay_procedure_get_aux_arguments  (ClayProcedure        *procedure,
                                                    gint                 *n_arguments);
GParamSpec    ** clay_procedure_get_return_values  (ClayProcedure        *procedure,
                                                    gint                 *n_return_values);

void             clay_procedure_set_argument_sync  (ClayProcedure        *procedure,
                                                    const gchar          *arg_name,
                                                    ClayArgumentSync      sync);
ClayArgumentSync clay_procedure_get_argument_sync  (ClayProcedure        *procedure,
                                                    const gchar          *arg_name);

ClayValueArray * clay_procedure_new_return_values  (ClayProcedure        *procedure,
                                                    ClayPDBStatusType     status,
                                                    GError               *error);

ClayValueArray * clay_procedure_run                (ClayProcedure        *procedure,
                                                    const gchar          *first_arg_name,
                                                    ...) G_GNUC_NULL_TERMINATED;
ClayValueArray * clay_procedure_run_valist         (ClayProcedure        *procedure,
                                                    const gchar          *first_arg_name,
                                                    va_list               args);
ClayValueArray * clay_procedure_run_config         (ClayProcedure        *procedure,
                                                    ClayProcedureConfig  *config);

void             clay_procedure_persistent_ready   (ClayProcedure        *procedure);

ClayProcedureConfig *
                 clay_procedure_create_config      (ClayProcedure        *procedure);

gboolean         clay_procedure_is_internal        (ClayProcedure        *procedure);


/* Internal use */

G_GNUC_INTERNAL GParamSpec          * _clay_procedure_add_argument       (ClayProcedure  *procedure,
                                                                         GParamSpec     *pspec);
G_GNUC_INTERNAL GParamSpec          * _clay_procedure_add_aux_argument   (ClayProcedure  *procedure,
                                                                         GParamSpec     *pspec);
G_GNUC_INTERNAL GParamSpec          * _clay_procedure_add_return_value   (ClayProcedure  *procedure,
                                                                         GParamSpec     *pspec);

G_GNUC_INTERNAL ClayProcedureConfig * _clay_procedure_create_run_config (ClayProcedure  *procedure);

G_GNUC_INTERNAL ClayValueArray      * _clay_procedure_run_array         (ClayProcedure  *procedure,
                                                                         ClayValueArray *args);

G_GNUC_INTERNAL gint                  _clay_procedure_get_ref_count     (ClayProcedure  *procedure,
                                                                         GObject        *object);


G_END_DECLS

#endif  /*  __CLAY_PROCEDURE_H__  */
