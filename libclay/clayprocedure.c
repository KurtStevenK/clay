/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include <stdarg.h>
#include <sys/types.h>

#include <gobject/gvaluecollector.h>

#include "clay.h"

#include "libclaybase/clayprotocol-private.h"
#include "libclaybase/claywire-private.h"

#include "claygpparams.h"
#include "claypdb-private.h"
#include "clayplugin-private.h"
#include "claypdb_pdb.h"
#include "claypdbprocedure.h"
#include "clayplugin-private.h"
#include "clayplugin_pdb.h"
#include "clayprocedureconfig-private.h"

#include "libclay-intl.h"


/**
 * ClayProcedure:
 *
 * Procedures are registered functions which can be run across Clay ecosystem.
 * They can be created by plug-ins and can then run by the core application
 * when called from menus (or through other interaction depending on specific
 * procedure subclasses).
 *
 * A plug-in can also run procedures created by the core, but also the ones
 * created by other plug-ins (see [class@PDB]).
 **/

enum
{
  PROP_0,
  PROP_PLUG_IN,
  PROP_NAME,
  PROP_PROCEDURE_TYPE,
  N_PROPS
};


typedef struct _ClayProcedurePrivate
{
  ClayPlugIn       *plug_in;

  gchar            *name;
  ClayPDBProcType   proc_type;

  gchar            *image_types;

  gchar            *menu_label;
  GList            *menu_paths;

  ClayIconType      icon_type;
  gpointer          icon_data;

  gchar            *blurb;
  gchar            *help;
  gchar            *help_id;

  gchar            *authors;
  gchar            *copyright;
  gchar            *date;

  gint              sensitivity_mask;

  gint32            n_args;
  GParamSpec      **args;

  gint32            n_aux_args;
  GParamSpec      **aux_args;

  gint32            n_values;
  GParamSpec      **values;

  ClayRunFunc       run_func;
  gpointer          run_data;
  GDestroyNotify    run_data_destroy;

  gboolean          installed;

  GFile            *help_file;
  gchar            *help_uri;
} ClayProcedurePrivate;


static void                  clay_procedure_constructed        (GObject              *object);
static void                  clay_procedure_finalize           (GObject              *object);
static void                  clay_procedure_set_property       (GObject              *object,
                                                                guint                 property_id,
                                                                const GValue         *value,
                                                                GParamSpec           *pspec);
static void                  clay_procedure_get_property       (GObject              *object,
                                                                guint                 property_id,
                                                                GValue               *value,
                                                                GParamSpec           *pspec);

static void                  clay_procedure_real_install       (ClayProcedure        *procedure);
static void                  clay_procedure_real_uninstall     (ClayProcedure        *procedure);
static ClayValueArray      * clay_procedure_real_run           (ClayProcedure        *procedure,
                                                                const ClayValueArray *args);
static ClayProcedureConfig * clay_procedure_real_create_config (ClayProcedure        *procedure,
                                                                GParamSpec          **args,
                                                                gint                  n_args);

static ClayProcedureConfig *
                      clay_procedure_create_config_with_prefix (ClayProcedure        *procedure,
                                                                const gchar          *prefix,
                                                                GParamSpec          **args,
                                                                gint                  n_args);

static ClayValueArray      * clay_procedure_new_arguments      (ClayProcedure        *procedure);
static gboolean              clay_procedure_validate_args      (ClayProcedure        *procedure,
                                                                GParamSpec          **param_specs,
                                                                gint                  n_param_specs,
                                                                ClayValueArray       *args,
                                                                gboolean              return_vals,
                                                                GError              **error);

static void                  clay_procedure_set_icon           (ClayProcedure        *procedure,
                                                                ClayIconType          icon_type,
                                                                gconstpointer         icon_data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayProcedure, clay_procedure, G_TYPE_OBJECT)

#define parent_class clay_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_procedure_class_init (ClayProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_procedure_constructed;
  object_class->finalize     = clay_procedure_finalize;
  object_class->set_property = clay_procedure_set_property;
  object_class->get_property = clay_procedure_get_property;

  klass->install             = clay_procedure_real_install;
  klass->uninstall           = clay_procedure_real_uninstall;
  klass->run                 = clay_procedure_real_run;
  klass->create_config       = clay_procedure_real_create_config;

  props[PROP_PLUG_IN] =
    g_param_spec_object ("plug-in",
                         "Plug-In",
                         "The ClayPlugIn of this plug-in process",
                         CLAY_TYPE_PLUG_IN,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  props[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The procedure's name",
                         NULL,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  props[PROP_PROCEDURE_TYPE] =
    g_param_spec_enum ("procedure-type",
                       "Procedure type",
                       "The procedure's type",
                       CLAY_TYPE_PDB_PROC_TYPE,
                       CLAY_PDB_PROC_TYPE_PLUGIN,
                       CLAY_PARAM_READWRITE |
                       G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_procedure_init (ClayProcedure *procedure)
{
}

static void
clay_procedure_constructed (GObject *object)
{
  ClayProcedure        *procedure = CLAY_PROCEDURE (object);
  ClayProcedurePrivate *priv      = clay_procedure_get_instance_private (procedure);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (CLAY_IS_PLUG_IN (priv->plug_in));
  g_assert (priv->name != NULL);

  clay_procedure_set_sensitivity_mask (procedure, CLAY_PROCEDURE_SENSITIVE_ALWAYS);
}

static void
clay_procedure_finalize (GObject *object)
{
  ClayProcedure        *procedure = CLAY_PROCEDURE (object);
  ClayProcedurePrivate *priv      = clay_procedure_get_instance_private (procedure);
  gint                  i;

  if (priv->run_data_destroy)
    priv->run_data_destroy (priv->run_data);

  g_clear_object  (&priv->plug_in);
  g_clear_object  (&priv->help_file);

  g_clear_pointer (&priv->name,        g_free);
  g_clear_pointer (&priv->image_types, g_free);
  g_clear_pointer (&priv->menu_label,  g_free);
  g_clear_pointer (&priv->blurb,       g_free);
  g_clear_pointer (&priv->help,        g_free);
  g_clear_pointer (&priv->help_id,     g_free);
  g_clear_pointer (&priv->authors,     g_free);
  g_clear_pointer (&priv->copyright,   g_free);
  g_clear_pointer (&priv->date,        g_free);
  g_clear_pointer (&priv->help_uri,    g_free);

  g_list_free_full (priv->menu_paths, g_free);
  priv->menu_paths = NULL;

  clay_procedure_set_icon (procedure, CLAY_ICON_TYPE_ICON_NAME, NULL);

  if (priv->args)
    {
      for (i = 0; i < priv->n_args; i++)
        g_param_spec_unref (priv->args[i]);

      g_clear_pointer (&priv->args, g_free);
    }

  if (priv->aux_args)
    {
      for (i = 0; i < priv->n_aux_args; i++)
        g_param_spec_unref (priv->aux_args[i]);

      g_clear_pointer (&priv->aux_args, g_free);
    }

  if (priv->values)
    {
      for (i = 0; i < priv->n_values; i++)
        g_param_spec_unref (priv->values[i]);

      g_clear_pointer (&priv->values, g_free);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_procedure_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayProcedure        *procedure = CLAY_PROCEDURE (object);
  ClayProcedurePrivate *priv      = clay_procedure_get_instance_private (procedure);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_set_object (&priv->plug_in, g_value_get_object (value));
      break;

    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;

    case PROP_PROCEDURE_TYPE:
      priv->proc_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_procedure_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayProcedure        *procedure = CLAY_PROCEDURE (object);
  ClayProcedurePrivate *priv      = clay_procedure_get_instance_private (procedure);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_value_set_object (value, priv->plug_in);
      break;

    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_PROCEDURE_TYPE:
      g_value_set_enum (value, priv->proc_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_procedure_install_icon (ClayProcedure *procedure)
{
  ClayIconType  icon_type;
  guint8       *icon_data        = NULL;
  gsize         icon_data_length = 0;
  GBytes       *icon_bytes       = NULL;

  icon_type = clay_procedure_get_icon_type (procedure);

  switch (icon_type)
    {
    case CLAY_ICON_TYPE_ICON_NAME:
      {
        icon_data = (guint8 *) clay_procedure_get_icon_name (procedure);
        if (icon_data)
          icon_bytes = g_bytes_new_static (icon_data,
                                           strlen ((gchar *) icon_data) + 1);
      }
      break;

    case CLAY_ICON_TYPE_PIXBUF:
      {
        GdkPixbuf *pixbuf = clay_procedure_get_icon_pixbuf (procedure);

        if (pixbuf)
          {
            if (gdk_pixbuf_save_to_buffer (pixbuf,
                                           (gchar **) &icon_data, &icon_data_length,
                                           "png", NULL, NULL))
              icon_bytes = g_bytes_new_take (icon_data, icon_data_length);
          }
      }
      break;

    case CLAY_ICON_TYPE_IMAGE_FILE:
      {
        GFile *file = clay_procedure_get_icon_file (procedure);

        if (file)
          {
            icon_data        = (guchar *) g_file_get_uri (file);
            icon_data_length = strlen ((gchar *) icon_data) + 1;
            icon_bytes       = g_bytes_new_take (icon_data, icon_data_length);
          }
      }
      break;
    }

  if (icon_bytes)
    _clay_pdb_set_proc_icon (clay_procedure_get_name (procedure),
                             icon_type, icon_bytes);
  g_bytes_unref (icon_bytes);
}

static void
clay_procedure_real_install (ClayProcedure *procedure)
{
  ClayProcedurePrivate  *priv;
  GParamSpec           **args;
  GParamSpec           **return_vals;
  gint                   n_args        = 0;
  gint                   n_return_vals = 0;
  GList                 *list;
  ClayPlugIn            *plug_in;
  GPProcInstall          proc_install;
  gint                   i;

  priv = clay_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->installed == FALSE);

  args        = clay_procedure_get_arguments (procedure, &n_args);
  return_vals = clay_procedure_get_return_values (procedure, &n_return_vals);

  proc_install.name          = (gchar *) clay_procedure_get_name (procedure);
  proc_install.type          = clay_procedure_get_proc_type (procedure);
  proc_install.n_params      = n_args;
  proc_install.n_return_vals = n_return_vals;
  proc_install.params        = g_new0 (GPParamDef, n_args);
  proc_install.return_vals   = g_new0 (GPParamDef, n_return_vals);

  for (i = 0; i < n_args; i++)
    {
      _clay_param_spec_to_gp_param_def (args[i],
                                        &proc_install.params[i], FALSE);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      _clay_param_spec_to_gp_param_def (return_vals[i],
                                        &proc_install.return_vals[i], FALSE);
    }

  plug_in = clay_procedure_get_plug_in (procedure);

  if (! gp_proc_install_write (_clay_plug_in_get_write_channel (plug_in),
                               &proc_install, plug_in))
    _clay_quit ();

  g_free (proc_install.params);
  g_free (proc_install.return_vals);

  clay_procedure_install_icon (procedure);

  if (priv->image_types)
    {
      _clay_pdb_set_proc_image_types (clay_procedure_get_name (procedure),
                                      priv->image_types);
    }

  if (priv->menu_label)
    {
      _clay_pdb_set_proc_menu_label (clay_procedure_get_name (procedure),
                                     priv->menu_label);
    }

  _clay_pdb_set_proc_sensitivity_mask (clay_procedure_get_name (procedure),
                                       priv->sensitivity_mask);

  for (list = clay_procedure_get_menu_paths (procedure);
       list;
       list = g_list_next (list))
    {
      _clay_pdb_add_proc_menu_path (clay_procedure_get_name (procedure),
                                    list->data);
    }

  if (priv->blurb ||
      priv->help  ||
      priv->help_id)
    {
      _clay_pdb_set_proc_documentation (clay_procedure_get_name (procedure),
                                        priv->blurb,
                                        priv->help,
                                        priv->help_id);
    }

  if (priv->authors   ||
      priv->copyright ||
      priv->date)
    {
      _clay_pdb_set_proc_attribution (clay_procedure_get_name (procedure),
                                      priv->authors,
                                      priv->copyright,
                                      priv->date);
    }

  if (priv->help_uri)
    {
      if (! _clay_pdb_set_proc_help_uri (clay_procedure_get_name (procedure),
                                         priv->help_uri))
        g_clear_pointer (&priv->help_uri, g_free);
    }

  priv->installed = TRUE;
}

static void
clay_procedure_real_uninstall (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv = clay_procedure_get_instance_private (procedure);
  ClayPlugIn           *plug_in;
  GPProcUninstall       proc_uninstall;

  g_return_if_fail (priv->installed == TRUE);

  proc_uninstall.name = (gchar *) clay_procedure_get_name (procedure);

  plug_in = clay_procedure_get_plug_in (procedure);

  if (! gp_proc_uninstall_write (_clay_plug_in_get_write_channel (plug_in),
                                 &proc_uninstall, plug_in))
    _clay_quit ();

  priv->installed = FALSE;
}

static ClayValueArray *
clay_procedure_real_run (ClayProcedure        *procedure,
                         const ClayValueArray *args)
{
  ClayProcedurePrivate *priv     = clay_procedure_get_instance_private (procedure);
  ClayPlugIn           *plug_in;
  ClayProcedureConfig  *config;
  ClayImage            *image    = NULL;
  ClayRunMode           run_mode = CLAY_RUN_NONINTERACTIVE;
  ClayValueArray       *retvals;
  ClayPDBStatusType     status   = CLAY_PDB_EXECUTION_ERROR;

  if (clay_value_array_length (args) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (args, 0)))
    {
      run_mode = CLAY_VALUES_GET_ENUM (args, 0);

      if (clay_value_array_length (args) > 1 &&
          CLAY_VALUE_HOLDS_IMAGE (clay_value_array_index (args, 1)))
        image = CLAY_VALUES_GET_IMAGE (args, 1);
    }

  config = _clay_procedure_create_run_config (procedure);
  _clay_procedure_config_begin_run (config, image, run_mode, args, NULL);

  retvals = priv->run_func (procedure, config, priv->run_data);

  if (retvals != NULL                       &&
      clay_value_array_length (retvals) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (retvals, 0)))
    status = CLAY_VALUES_GET_ENUM (retvals, 0);

  _clay_procedure_config_end_run (config, status);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = clay_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _clay_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the ClayProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);

  return retvals;
}

static ClayProcedureConfig *
clay_procedure_real_create_config (ClayProcedure  *procedure,
                                   GParamSpec    **args,
                                   gint            n_args)
{
  return clay_procedure_create_config_with_prefix (procedure,
                                                   "ClayProcedureConfigRun",
                                                   args, n_args);
}

static ClayProcedureConfig *
clay_procedure_create_config_with_prefix (ClayProcedure  *procedure,
                                          const gchar    *prefix,
                                          GParamSpec    **args,
                                          gint            n_args)
{
  ClayProcedurePrivate *priv = clay_procedure_get_instance_private (procedure);
  gchar                *type_name;
  GType                 type;

  type_name = g_strdup_printf ("%s-%s", prefix,
                               clay_procedure_get_name (procedure));

  type = g_type_from_name (type_name);

  if (! type)
    {
      GParamSpec **config_args;
      gint         n_config_args;

      n_config_args = n_args + priv->n_aux_args;

      config_args = g_new0 (GParamSpec *, n_config_args);

      memcpy (config_args,
              args,
              n_args * sizeof (GParamSpec *));
      memcpy (config_args + n_args,
              priv->aux_args,
              priv->n_aux_args * sizeof (GParamSpec *));

      type = clay_config_type_register (CLAY_TYPE_PROCEDURE_CONFIG,
                                        type_name,
                                        config_args, n_config_args);

      g_free (config_args);
    }

  g_free (type_name);

  return g_object_new (type,
                       "procedure", procedure,
                       NULL);
}

static gchar *
sanitize_menu_path_separators (const gchar *menu_path)
{
  gchar *menu_path_clean = g_strdup (menu_path);
  gchar *hit             = NULL;

  /* Get rid of all trailing path separators */
  while (g_str_has_suffix (menu_path_clean, "/"))
    menu_path_clean[strlen (menu_path_clean) - 1] = '\0';

  /* Search the menu path for all occurences of doubled path separators. */
  while ((hit = strstr (menu_path_clean, "//")) != 0)
    {
      /* Shift the string by one to overwrite the first path separator while
       * preserving the end of the string without having to re-allocate. */
      for (; *hit != '\0'; hit++)
        *hit = *(hit+1);
    }

  return menu_path_clean;
}


/*  public functions  */

/**
 * clay_procedure_new:
 * @plug_in:   a #ClayPlugIn.
 * @name:      the new procedure's name.
 * @proc_type: the new procedure's #ClayPDBProcType.
 * @run_func:  the run function for the new procedure.
 * @run_data:  user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new procedure named @name which will call @run_func when
 * invoked.
 *
 * The @name parameter is mandatory and should be unique, or it will
 * overwrite an already existing procedure (overwrite procedures only
 * if you know what you're doing).
 *
 * @proc_type should be [enum@Clay.PDBProcType.PLUGIN] for "normal" plug-ins.
 *
 * Using [enum@Clay.PDBProcType.PERSISTENT] means that the plug-in will
 * add temporary procedures. Therefore, the Clay core will wait until
 * the %CLAY_PDB_PROC_TYPE_PERSISTENT procedure has called
 * [method@Procedure.persistent_ready], which means that the procedure
 * has done its initialization, installed its temporary procedures and
 * is ready to run.
 *
 * *Not calling [method@Procedure.persistent_ready] from a
 * %CLAY_PDB_PROC_TYPE_PERSISTENT procedure will cause the Clay core to
 * lock up.*
 *
 * Additionally, a %CLAY_PDB_PROC_TYPE_PERSISTENT procedure with no
 * arguments added is an "automatic" extension that will be
 * automatically started on each Clay startup.
 *
 * [enum@Clay.PDBProcType.TEMPORARY] must be used for temporary procedures
 * that are created during a plug-ins lifetime. They must be added to
 * the #ClayPlugIn using [method@PlugIn.add_temp_procedure].
 *
 * @run_func is called via [method@Procedure.run].
 *
 * For [enum@Clay.PDBProcType.PLUGIN] and [Clay.PDBProcType.PERSISTENT]
 * procedures the call of @run_func is basically the lifetime of the
 * plug-in.
 *
 * Returns: a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_procedure_new (ClayPlugIn        *plug_in,
                    const gchar       *name,
                    ClayPDBProcType    proc_type,
                    ClayRunFunc        run_func,
                    gpointer           run_data,
                    GDestroyNotify     run_data_destroy)
{
  ClayProcedure        *procedure;
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  priv = clay_procedure_get_instance_private (procedure);

  priv->run_func         = run_func;
  priv->run_data         = run_data;
  priv->run_data_destroy = run_data_destroy;

  return procedure;
}
/**
 * clay_procedure_get_plug_in:
 * @procedure: A #ClayProcedure.
 *
 * Returns: (transfer none): The #ClayPlugIn given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
ClayPlugIn *
clay_procedure_get_plug_in (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->plug_in;
}

/**
 * clay_procedure_get_name:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's name given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_name (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->name;
}

/**
 * clay_procedure_get_proc_type:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's type given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
ClayPDBProcType
clay_procedure_get_proc_type (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure),
                        CLAY_PDB_PROC_TYPE_PLUGIN);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->proc_type;
}

/**
 * clay_procedure_set_image_types:
 * @procedure:   A #ClayProcedure.
 * @image_types: The image types this procedure can operate on.
 *
 * This is a comma separated list of image types, or actually drawable
 * types, that this procedure can deal with. Wildcards are possible
 * here, so you could say "RGB\*" instead of "RGB, RGBA" or "\*" for all
 * image types.
 *
 * Supported types are "RGB", "GRAY", "INDEXED" and their variants
 * with alpha.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_image_types (ClayProcedure *procedure,
                                const gchar   *image_types)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  g_free (priv->image_types);
  priv->image_types = g_strdup (image_types);

  if (priv->installed)
    _clay_pdb_set_proc_image_types (clay_procedure_get_name (procedure),
                                    priv->image_types);
}

/**
 * clay_procedure_get_image_types:
 * @procedure:  A #ClayProcedure.
 *
 * This function retrieves the list of image types the procedure can
 * operate on. See [method@Clay.Procedure.set_image_types].
 *
 * Returns: The image types.
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_image_types (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->image_types;
}

/**
 * clay_procedure_set_sensitivity_mask:
 * @procedure:        A #ClayProcedure.
 * @sensitivity_mask: A binary mask of [flags@Clay.ProcedureSensitivityMask].
 *
 * Sets the cases when @procedure is supposed to be sensitive or not.
 *
 * Note that it will be used by the core to determine whether to show a
 * procedure as sensitive (hence forbid running it otherwise), yet it
 * will not forbid thid-party plug-ins for instance to run manually your
 * registered procedure. Therefore you should still handle non-supported
 * cases appropriately by returning with
 * [enum@Clay.PDBStatusType.EXECUTION_ERROR] and a suitable error
 * message.
 *
 * Similarly third-party plug-ins should verify they are allowed to call
 * a procedure with [method@Procedure.get_sensitivity_mask] when running
 * with dynamic contents.
 *
 * Note that by default, a procedure works on an image with one or more
 * drawables selected. Hence not setting the mask, setting it with 0 or
 * setting it with `CLAY_PROCEDURE_SENSITIVE_DRAWABLE |
 * CLAY_PROCEDURE_SENSITIVE_DRAWABLES` are equivalent.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_sensitivity_mask (ClayProcedure *procedure,
                                     gint           sensitivity_mask)
{
  ClayProcedurePrivate *priv;
  gboolean              success = TRUE;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  if (CLAY_PROCEDURE_GET_CLASS (procedure)->set_sensitivity)
    success = CLAY_PROCEDURE_GET_CLASS (procedure)->set_sensitivity (procedure,
                                                                     sensitivity_mask);

  if (success)
    {
      priv->sensitivity_mask = sensitivity_mask;

      if (priv->installed)
        _clay_pdb_set_proc_sensitivity_mask (clay_procedure_get_name (procedure),
                                             priv->sensitivity_mask);
    }
}

/**
 * clay_procedure_get_sensitivity_mask:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's sensitivity mask given in
 *          [method@Procedure.set_sensitivity_mask].
 *
 * Since: 3.0
 **/
gint
clay_procedure_get_sensitivity_mask (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), 0);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->sensitivity_mask;
}

/**
 * clay_procedure_set_menu_label:
 * @procedure:  A #ClayProcedure.
 * @menu_label: The @procedure's menu label.
 *
 * Sets the label to use for the @procedure's menu entry, The
 * location(s) where to register in the menu hierarchy is chosen using
 * [method@Clay.Procedure.add_menu_path].
 *
 * Plug-ins are responsible for their own translations. You are expected to send
 * localized strings to Clay if your plug-in is internationalized.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_menu_label (ClayProcedure *procedure,
                               const gchar   *menu_label)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_label != NULL && strlen (menu_label));

  priv = clay_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->menu_label, g_free);
  priv->menu_label = g_strdup (menu_label);

  if (priv->installed)
    _clay_pdb_set_proc_menu_label (clay_procedure_get_name (procedure),
                                   priv->menu_label);
}

/**
 * clay_procedure_get_menu_label:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's menu label given in
 *          [method@Clay.Procedure.set_menu_label].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_menu_label (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->menu_label;
}

/**
 * clay_procedure_add_menu_path:
 * @procedure: A #ClayProcedure.
 * @menu_path: The @procedure's additional menu path.
 *
 * Adds a menu path to the procedure. Only procedures which have a menu
 * label can add a menu path. So you must call
 * [method@Clay.Procedure.set_menu_label] first.
 *
 * Menu paths follow the syntax `<Menu>/Path/To/Submenu`, for example
 * `<Image>/Layer/Transform`. The `<Image>` menu corresponds to Clay's
 * main menu and is therefore the most commonly used. Other common menus
 * are `<Layers>`, `<Channels>` and `<Paths>` for the contextual menus
 * shown in the respective item dockables, or again `<Brushes>`,
 * `<Dynamics>`, `<MyPaintBrushes>`, `<Gradients>`, `<Palettes>`,
 * `<Patterns>`, `<ToolPresets>`, `<Fonts>` and `<Buffers>` for
 * contextual menus shown in resource dockables.
 *
 * Menu path elements to known menus and submenus must be passed
 * untranslated (i.e. in US English), and the Clay process will localize
 * these.
 * Nevertheless you should localize unknown parts of the path. For
 * instance, say you register a procedure to create customized layers
 * and want it to be under a `Create` submenu. Whereas the "Layer" menu
 * name localization will be handled by Clay itself, the new submenu is
 * created by your plug-in, so you must localize it yourself, e.g. with
 * gettext:
 *
 * ```C
 * path = g_build_path ("/", "<Image>/Layer", _("Create"), NULL);
 * clay_procedure_add_menu_path (procedure, path);
 * g_free (path);
 * ```
 *
 * Clay menus also have a concept of named section. For instance, say you are
 * creating a @procedure which you want to show next to the "Export", "Export As"
 * items in the File menu. You would add it to the menu path `"File/[Export]"`.
 * If you actually wanted to create a submenu called "[Export]" (with square
 * brackets), double the brackets: `"File/[[Export]]"`
 *
 * Finally if you want the procedure to show inside a new submenu
 * "Layers" (e.g. because your plug-in is for exporting individual
 * layers and you want to add several procedures in a single submenu),
 * which itself is in the "Export" section of the File menu, add the
 * menu path `"File/[Export]/Layers"`.
 *
 * See the [`image-menu` UI file](https://github.com/KurtStevenK/clay/-/blob/master/menus/image-menu.ui.in.in)
 * for the various "section-name" available in the main menu
 * (`<Image>`). Check out the other `.ui` files in this same directory
 * to find the "section-name" of other menus.
 *
 * This function will place your procedure to the bottom of the selected path or
 * section. Order is not assured relatively to other plug-ins.
 *
 * Since: 3.0
 **/
void
clay_procedure_add_menu_path (ClayProcedure *procedure,
                              const gchar   *menu_path)
{
  ClayProcedurePrivate *priv            = NULL;
  gchar                *menu_path_clean = NULL;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_path != NULL);

  priv = clay_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->menu_label != NULL);

  /* sanitize_menu_path_separators already creates a copy of menu_path */
  menu_path_clean = sanitize_menu_path_separators (menu_path);

  priv->menu_paths = g_list_append (priv->menu_paths, menu_path_clean);

  if (priv->installed)
    _clay_pdb_add_proc_menu_path (clay_procedure_get_name (procedure),
                                  menu_path_clean);
}

/**
 * clay_procedure_get_menu_paths:
 * @procedure: A #ClayProcedure.
 *
 * Returns: (transfer none) (element-type gchar*): the @procedure's
 *          menu paths as added with [method@Clay.Procedure.add_menu_path].
 *
 * Since: 3.0
 **/
GList *
clay_procedure_get_menu_paths (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->menu_paths;
}

/**
 * clay_procedure_set_icon_name:
 * @procedure: a #ClayProcedure.
 * @icon_name: (nullable): an icon name.
 *
 * Sets the icon for @procedure to the icon referenced by @icon_name.
 *
 * Since: 3.0
 */
void
clay_procedure_set_icon_name (ClayProcedure *procedure,
                              const gchar   *icon_name)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  clay_procedure_set_icon (procedure,
                           CLAY_ICON_TYPE_ICON_NAME,
                           icon_name);
}

/**
 * clay_procedure_set_icon_pixbuf:
 * @procedure: a #ClayProcedure.
 * @pixbuf:    (nullable): a #GdkPixbuf.
 *
 * Sets the icon for @procedure to @pixbuf.
 *
 * Since: 3.0
 */
void
clay_procedure_set_icon_pixbuf (ClayProcedure *procedure,
                                GdkPixbuf     *pixbuf)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  clay_procedure_set_icon (procedure,
                           CLAY_ICON_TYPE_PIXBUF,
                           pixbuf);
}

/**
 * clay_procedure_set_icon_file:
 * @procedure: a #ClayProcedure.
 * @file:      (nullable): a #GFile pointing to an image file.
 *
 * Sets the icon for @procedure to the contents of an image file.
 *
 * Since: 3.0
 */
void
clay_procedure_set_icon_file (ClayProcedure *procedure,
                              GFile         *file)
{
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  clay_procedure_set_icon (procedure,
                           CLAY_ICON_TYPE_IMAGE_FILE,
                           file);
}

/**
 * clay_procedure_get_icon_type:
 * @procedure: a #ClayProcedure.
 *
 * Gets the type of data set as @procedure's icon. Depending on the
 * result, you can call the relevant specific function, such as
 * [method@Procedure.get_icon_name].
 *
 * Returns: the #ClayIconType of @procedure's icon.
 *
 * Since: 3.0
 */
ClayIconType
clay_procedure_get_icon_type (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), -1);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->icon_type;
}

/**
 * clay_procedure_get_icon_name:
 * @procedure: a #ClayProcedure.
 *
 * Gets the name of the icon if one was set for @procedure.
 *
 * Returns: (nullable): the icon name or %NULL if no icon name was set.
 *
 * Since: 3.0
 */
const gchar *
clay_procedure_get_icon_name (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (priv->icon_type == CLAY_ICON_TYPE_ICON_NAME)
    return (const gchar *) priv->icon_data;

  return NULL;
}

/**
 * clay_procedure_get_icon_file:
 * @procedure: a #ClayProcedure.
 *
 * Gets the file of the icon if one was set for @procedure.
 *
 * Returns: (nullable) (transfer none): the icon #GFile or %NULL if no
 *          file was set.
 *
 * Since: 3.0
 */
GFile *
clay_procedure_get_icon_file (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  priv = clay_procedure_get_instance_private (procedure);
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (priv->icon_type == CLAY_ICON_TYPE_IMAGE_FILE)
    return (GFile *) priv->icon_data;

  return NULL;
}

/**
 * clay_procedure_get_icon_pixbuf:
 * @procedure: a #ClayProcedure.
 *
 * Gets the #GdkPixbuf of the icon if an icon was set this way for
 * @procedure.
 *
 * Returns: (nullable) (transfer none): the icon pixbuf or %NULL if no
 *          icon name was set.
 *
 * Since: 3.0
 */
GdkPixbuf *
clay_procedure_get_icon_pixbuf (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (priv->icon_type == CLAY_ICON_TYPE_PIXBUF)
    return (GdkPixbuf *) priv->icon_data;

  return NULL;
}

/**
 * clay_procedure_set_documentation:
 * @procedure:           A #ClayProcedure.
 * @blurb:               The @procedure's blurb.
 * @help:    (nullable): The @procedure's help text.
 * @help_id: (nullable): The @procedure's help ID.
 *
 * Sets various optional documentation strings on @procedure:
 *
 * * @blurb is used for instance as the @procedure's tooltip when represented in
 *   the UI such as a menu entry, or in "Search Actions" widget under
 *   the main procedure's label.
 * * @help is a free-form text that's meant as additional documentation for
 *   developers of scripts and plug-ins. If the @blurb and the argument names
 *   and descriptions are enough for a quite self-explanatory procedure, you may
 *   set @help to %NULL, rather than setting an uninformative @help (avoid
 *   setting the same text as @blurb or redundant information).
 *
 * Plug-ins are responsible for their own translations. You are expected to send
 * localized strings of @blurb and @help to Clay if your plug-in is
 * internationalized.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_documentation (ClayProcedure *procedure,
                                  const gchar   *blurb,
                                  const gchar   *help,
                                  const gchar   *help_id)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->blurb,   g_free);
  g_clear_pointer (&priv->help,    g_free);
  g_clear_pointer (&priv->help_id, g_free);

  priv->blurb   = g_strdup (blurb);
  priv->help    = g_strdup (help);
  priv->help_id = g_strdup (help_id);

  if (priv->installed)
    _clay_pdb_set_proc_documentation (clay_procedure_get_name (procedure),
                                      priv->blurb,
                                      priv->help,
                                      priv->help_id);
}

/**
 * clay_procedure_get_blurb:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's blurb given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_blurb (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->blurb;
}

/**
 * clay_procedure_get_help:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's help text given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_help (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->help;
}

/**
 * clay_procedure_get_help_id:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's help ID given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_help_id (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->help_id;
}

/**
 * clay_procedure_set_help_uri:
 * @procedure:     A #ClayProcedure.
 * @uri_reference: The @procedure's help URI Reference.
 *
 * Sets a URI Reference (in the meaning of [RFC 3986, section
 * 4.1](https://datatracker.ietf.org/doc/html/rfc3986#section-4)) where
 * documentation for this procedure will be available.
 *
 * This function only accepts two types of URI references: full URI, but
 * only with the scheme `https`, or a relative-path reference such as
 * "docs/index.html" or "docs/manual.pdf". Note how this doesn't have a
 * scheme part and allows to ship documentation with a plug-in.
 * These relative paths are made relatively to the plug-in or
 * extension's root folder.
 *
 * Clay will open @uri_reference using the default application
 * registered for the file type. We recommend choosing the file type of
 * your documentation conservatively, with types with default readers on
 * every platform, such as `html` or `pdf` documentation.
 *
 * This method is both complementary and redundant to
 * [method@Procedure.set_documentation], and in particular replace its
 * @help_id argument. In common cases, set the @blurb (and optionally
 * the @help) arguments on `clay_procedure_set_documentation()` yet keep
 * @help_id to %NULL. Then set your documentation with this method
 * instead:
 *
 * ```C
 * clay_procedure_set_documentation (procedure,
 *                                   _("What this plug-in does"),
 *                                   NULL, NULL);
 * clay_procedure_set_help_uri (procedure,
 *                              "https://example.org/my-plug-in-docs");
 * ```
 *
 * Note: these functions might get merged in the v4 API.
 *
 * Since: 3.4
 **/
void
clay_procedure_set_help_uri (ClayProcedure *procedure,
                             const gchar   *uri_reference)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->help_uri, g_free);
  priv->help_uri = g_strdup (uri_reference);
  if (priv->installed)
    _clay_pdb_set_proc_help_uri (clay_procedure_get_name (procedure), uri_reference);
}

/**
 * clay_procedure_get_help_file:
 * @procedure: A #ClayProcedure.
 *
 * This function returns the #GFile to load for showing documentation
 * for @procedure.
 *
 * Returns: (transfer none): The procedure's help file constructed from
 *          the URI given in [method@Procedure.set_help_uri].
 *
 * Since: 3.4
 **/
GFile *
clay_procedure_get_help_file (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (priv->help_uri && ! priv->help_file)
    priv->help_file = _clay_pdb_get_proc_help_file (clay_procedure_get_name (procedure));

  return priv->help_file;
}

/**
 * clay_procedure_set_attribution:
 * @procedure: A #ClayProcedure.
 * @authors:   The @procedure's author(s).
 * @copyright: The @procedure's copyright.
 * @date:      The @procedure's date (written or published).
 *
 * Sets various attribution strings on @procedure.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_attribution (ClayProcedure *procedure,
                                const gchar   *authors,
                                const gchar   *copyright,
                                const gchar   *date)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  g_free (priv->authors);
  g_free (priv->copyright);
  g_free (priv->date);

  priv->authors   = g_strdup (authors);
  priv->copyright = g_strdup (copyright);
  priv->date      = g_strdup (date);

  if (priv->installed)
    _clay_pdb_set_proc_attribution (clay_procedure_get_name (procedure),
                                    priv->authors,
                                    priv->copyright,
                                    priv->date);
}

/**
 * clay_procedure_get_authors:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's authors given in [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_authors (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->authors;
}

/**
 * clay_procedure_get_copyright:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's copyright given in
 * [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_copyright (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->copyright;
}

/**
 * clay_procedure_get_date:
 * @procedure: A #ClayProcedure.
 *
 * Returns: The procedure's date given in [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
clay_procedure_get_date (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return priv->date;
}

/**
 * _clay_procedure_add_argument:
 * @procedure: the #ClayProcedure.
 * @pspec:     (transfer floating): the argument specification.
 *
 * Add a new argument to @procedure according to @pspec specifications.
 * The arguments will be ordered according to the calls order.
 *
 * If @pspec is floating, ownership will be taken over by @procedure,
 * allowing to pass directly `g*_param_spec_*()` calls as arguments.
 *
 * Returns: (transfer none): the same @pspec or %NULL in case of error.
 *
 * Since: 3.0
 **/
GParamSpec *
_clay_procedure_add_argument (ClayProcedure *procedure,
                              GParamSpec    *pspec)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (pspec->name), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (clay_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 clay_procedure_get_name (procedure));
      g_param_spec_sink (pspec);
      return NULL;
    }

  if (clay_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 clay_procedure_get_name (procedure));
      g_param_spec_sink (pspec);
      return NULL;
    }

  priv->args = g_renew (GParamSpec *, priv->args, priv->n_args + 1);

  priv->args[priv->n_args] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_args++;

  return pspec;
}

/**
 * _clay_procedure_add_aux_argument:
 * @procedure: the #ClayProcedure.
 * @pspec:     (transfer full): the argument specification.
 *
 * Add a new auxiliary argument to @procedure according to @pspec
 * specifications.
 *
 * Auxiliary arguments are not passed to the @procedure in run() and
 * are not known to the PDB. They are however members of the
 * #ClayProcedureConfig created by clay_procedure_create_config() and
 * can be used to persistently store whatever last used values the
 * @procedure wants to remember across invocations
 *
 * Returns: (transfer none): the same @pspec.
 *
 * Since: 3.0
 **/
GParamSpec *
_clay_procedure_add_aux_argument (ClayProcedure *procedure,
                                  GParamSpec    *pspec)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), pspec);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), pspec);
  g_return_val_if_fail (clay_is_canonical_identifier (pspec->name), pspec);

  priv = clay_procedure_get_instance_private (procedure);

  if (clay_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 clay_procedure_get_name (procedure));
      return pspec;
    }

  if (clay_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 clay_procedure_get_name (procedure));
      return pspec;
    }

  priv->aux_args = g_renew (GParamSpec *, priv->aux_args, priv->n_aux_args + 1);

  priv->aux_args[priv->n_aux_args] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_aux_args++;

  return pspec;
}

/**
 * _clay_procedure_add_return_value:
 * @procedure: the #ClayProcedure.
 * @pspec:     (transfer full): the return value specification.
 *
 * Add a new return value to @procedure according to @pspec
 * specifications.
 *
 * The returned values will be ordered according to the calls order.
 *
 * Returns: (transfer none): the same @pspec.
 *
 * Since: 3.0
 **/
GParamSpec *
_clay_procedure_add_return_value (ClayProcedure *procedure,
                                  GParamSpec    *pspec)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), pspec);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), pspec);
  g_return_val_if_fail (clay_is_canonical_identifier (pspec->name), pspec);

  priv = clay_procedure_get_instance_private (procedure);

  if (clay_procedure_find_return_value (procedure, pspec->name))
    {
      g_warning ("Return value with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 clay_procedure_get_name (procedure));
      return pspec;
    }

  priv->values = g_renew (GParamSpec *, priv->values, priv->n_values + 1);

  priv->values[priv->n_values] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_values++;

  return pspec;
}

/**
 * clay_procedure_find_argument:
 * @procedure: A #ClayProcedure
 * @name:      An argument name
 *
 * Searches the @procedure's arguments for a #GParamSpec called @name.
 *
 * Returns: (transfer none): The @procedure's argument with @name if it
 *          exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_procedure_find_argument (ClayProcedure *procedure,
                              const gchar   *name)
{
  ClayProcedurePrivate *priv;
  gint                  i;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  for (i = 0; i < priv->n_args; i++)
    if (! strcmp (name, priv->args[i]->name))
      return priv->args[i];

  return NULL;
}

/**
 * clay_procedure_find_aux_argument:
 * @procedure: A #ClayProcedure
 * @name:      An auxiliary argument name
 *
 * Searches the @procedure's auxiliary arguments for a #GParamSpec
 * called @name.
 *
 * Returns: (transfer none): The @procedure's auxiliary argument with
 *          @name if it exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_procedure_find_aux_argument (ClayProcedure *procedure,
                                  const gchar   *name)
{
  ClayProcedurePrivate *priv;
  gint                  i;

  priv = clay_procedure_get_instance_private (procedure);

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < priv->n_aux_args; i++)
    if (! strcmp (name, priv->aux_args[i]->name))
      return priv->aux_args[i];

  return NULL;
}

/**
 * clay_procedure_find_return_value:
 * @procedure: A #ClayProcedure
 * @name:      A return value name
 *
 * Searches the @procedure's return values for a #GParamSpec called
 * @name.
 *
 * Returns: (transfer none): The @procedure's return values with @name
 *          if it exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_procedure_find_return_value (ClayProcedure *procedure,
                                  const gchar   *name)
{
  ClayProcedurePrivate *priv;
  gint                  i;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);
  for (i = 0; i < priv->n_values; i++)
    if (! strcmp (name, priv->values[i]->name))
      return priv->values[i];

  return NULL;
}

/**
 * clay_procedure_get_arguments:
 * @procedure:   A #ClayProcedure.
 * @n_arguments: (out): Returns the number of arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
clay_procedure_get_arguments (ClayProcedure *procedure,
                              gint          *n_arguments)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  *n_arguments = priv->n_args;

  return priv->args;
}

/**
 * clay_procedure_get_aux_arguments:
 * @procedure:   A #ClayProcedure.
 * @n_arguments: (out): Returns the number of auxiliary arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
clay_procedure_get_aux_arguments (ClayProcedure *procedure,
                                  gint          *n_arguments)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  *n_arguments = priv->n_aux_args;

  return priv->aux_args;
}

/**
 * clay_procedure_get_return_values:
 * @procedure:       A #ClayProcedure.
 * @n_return_values: (out): Returns the number of return values.
 *
 * Returns: (transfer none) (array length=n_return_values): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
clay_procedure_get_return_values (ClayProcedure *procedure,
                                  gint          *n_return_values)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_return_values != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  *n_return_values = priv->n_values;

  return priv->values;
}

/**
 * clay_procedure_set_argument_sync:
 * @procedure: a #ClayProcedure.
 * @arg_name:  the name of one of @procedure's arguments or auxiliary arguments.
 * @sync:      how to sync the argument or auxiliary argument.
 *
 * When the procedure's run() function exits, a #ClayProcedure's arguments
 * or auxiliary arguments can be automatically synced with a #ClayParasite of
 * the #ClayImage the procedure is running on.
 *
 * In order to enable this, set @sync to [enum@Clay.ArgumentSync.PARASITE].
 *
 * Currently, it is possible to sync a string argument of type
 * #GParamSpecString with an image parasite of the same name, for
 * example the "clay-comment" parasite in file save procedures.
 *
 * Since: 3.0
 **/
void
clay_procedure_set_argument_sync (ClayProcedure    *procedure,
                                  const gchar      *arg_name,
                                  ClayArgumentSync  sync)
{
  GParamSpec *pspec;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (arg_name != NULL);

  pspec = clay_procedure_find_argument (procedure, arg_name);

  if (! pspec)
    pspec = clay_procedure_find_aux_argument (procedure, arg_name);

  g_return_if_fail (pspec != NULL);

  switch (sync)
    {
    case CLAY_ARGUMENT_SYNC_NONE:
      gegl_param_spec_set_property_key (pspec, "clay-argument-sync", NULL);
      break;

    case CLAY_ARGUMENT_SYNC_PARASITE:
      gegl_param_spec_set_property_key (pspec, "clay-argument-sync", "parasite");
      break;
    }
}

/**
 * clay_procedure_get_argument_sync:
 * @procedure: a #ClayProcedure
 * @arg_name:  the name of one of @procedure's arguments or auxiliary arguments
 *
 * Returns: The #ClayArgumentSync value set with
 *          [method@Clay.Procedure.set_argument_sync].
 *
 * Since: 3.0
 **/
ClayArgumentSync
clay_procedure_get_argument_sync (ClayProcedure *procedure,
                                  const gchar   *arg_name)
{
  GParamSpec       *pspec;
  ClayArgumentSync  sync = CLAY_ARGUMENT_SYNC_NONE;
  const gchar      *value;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), CLAY_ARGUMENT_SYNC_NONE);
  g_return_val_if_fail (arg_name != NULL, CLAY_ARGUMENT_SYNC_NONE);

  pspec = clay_procedure_find_argument (procedure, arg_name);

  if (! pspec)
    pspec = clay_procedure_find_aux_argument (procedure, arg_name);

  g_return_val_if_fail (pspec != NULL, CLAY_ARGUMENT_SYNC_NONE);

  value = gegl_param_spec_get_property_key (pspec, "clay-argument-sync");

  if (value)
    {
      if (! strcmp (value, "parasite"))
        sync = CLAY_ARGUMENT_SYNC_PARASITE;
    }

  return sync;
}

/**
 * clay_procedure_new_return_values:
 * @procedure: the procedure.
 * @status:    the success status of the procedure run.
 * @error:     (in) (nullable) (transfer full):
 *             an optional #GError. This parameter should be set if
 *             @status is either
 *             [enum@Clay.PDBStatusType.EXECUTION_ERROR] or
 *             [enum@Clay.PDBStatusType.CALLING_ERROR].
 *
 * Format the expected return values from procedures.
 *
 * Returns: (transfer full): the expected #ClayValueArray as could be returned
 * by a [callback@RunFunc].
 *
 * Since: 3.0
 **/
ClayValueArray *
clay_procedure_new_return_values (ClayProcedure     *procedure,
                                  ClayPDBStatusType  status,
                                  GError            *error)
{
  ClayProcedurePrivate *priv;
  ClayValueArray       *args;
  GValue                value = G_VALUE_INIT;
  gint                  i;

  g_return_val_if_fail (status != CLAY_PDB_PASS_THROUGH, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  switch (status)
    {
    case CLAY_PDB_SUCCESS:
    case CLAY_PDB_CANCEL:
      g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

      args = clay_value_array_new (priv->n_values + 1);

      g_value_init (&value, CLAY_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      clay_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < priv->n_values; i++)
        {
          GParamSpec *pspec = priv->values[i];

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          g_param_value_set_default (pspec, &value);
          clay_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    case CLAY_PDB_EXECUTION_ERROR:
    case CLAY_PDB_CALLING_ERROR:
      args = clay_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, CLAY_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      clay_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          clay_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  g_clear_error (&error);

  return args;
}

/**
 * clay_procedure_run: (skip)
 * @procedure:      the [class@Clay.Procedure] to run.
 * @first_arg_name: the name of an argument of @procedure or %NULL to
 *                  run @procedure with default arguments.
 * @...:            the value of @first_arg_name and any more argument
 *                  names and values as needed.
 *
 * Runs the procedure named @procedure_name with arguments given as
 * list of `(name, value)` pairs, terminated by %NULL.
 *
 * The order of arguments does not matter and if any argument is missing, its
 * default value will be used. The value type must correspond to the argument
 * type as registered for @procedure_name.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
ClayValueArray *
clay_procedure_run (ClayProcedure *procedure,
                    const gchar   *first_arg_name,
                    ...)
{
  ClayValueArray *return_values;
  va_list         args;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  va_start (args, first_arg_name);

  return_values = clay_procedure_run_valist (procedure, first_arg_name, args);

  va_end (args);

  return return_values;
}

/**
 * clay_procedure_run_valist: (skip)
 * @procedure:      the [class@Clay.Procedure] to run.
 * @first_arg_name: the name of an argument of @procedure or %NULL to
 *                  run @procedure with default arguments.
 * @args            the value of @first_arg_name and any more argument
 *                  names and values as needed.
 *
 * Runs @procedure with arguments names and values, given in the order as passed
 * to [method@Procedure.run].
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
ClayValueArray *
clay_procedure_run_valist (ClayProcedure *procedure,
                           const gchar   *first_arg_name,
                           va_list        args)
{
  ClayValueArray      *return_values;
  ClayProcedureConfig *config;
  const gchar         *arg_name;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  config   = clay_procedure_create_config (procedure);
  arg_name = first_arg_name;

  while (arg_name != NULL)
    {
      GParamSpec *pspec;
      gchar      *error = NULL;
      GValue      value = G_VALUE_INIT;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), arg_name);

      if (pspec == NULL)
        {
          g_warning ("%s: %s has no property named '%s'",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (config)),
                     arg_name);
          g_clear_object (&config);
          return NULL;
        }

      g_value_init (&value, pspec->value_type);
      G_VALUE_COLLECT (&value, args, G_VALUE_NOCOPY_CONTENTS, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_clear_object (&config);
          return NULL;
        }

      g_object_set_property (G_OBJECT (config), arg_name, &value);
      g_value_unset (&value);

      arg_name = va_arg (args, const gchar *);
    }

  return_values = clay_procedure_run_config (procedure, config);
  g_clear_object (&config);

  return return_values;
}

/**
 * clay_procedure_run_config: (rename-to clay_procedure_run)
 * @procedure:          the [class@Clay.Procedure] to run.
 * @config: (nullable): the @procedure's arguments.
 *
 * Runs @procedure, calling the run_func given in [ctor@Procedure.new].
 *
 * Create @config at default values with
 * [method@Clay.Procedure.create_config] then set any argument you wish
 * to change from defaults with [method@GObject.Object.set].
 *
 * If @config is %NULL, the default arguments of @procedure will be used.
 *
 * Returns: (transfer full): The @procedure's return values.
 *
 * Since: 3.0
 **/
ClayValueArray *
clay_procedure_run_config (ClayProcedure       *procedure,
                           ClayProcedureConfig *config)
{
  ClayValueArray *return_vals;
  ClayValueArray *args;
  gboolean        free_config = FALSE;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (config == NULL || CLAY_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (config == NULL || clay_procedure_config_get_procedure (config) == procedure, NULL);

  if (config == NULL)
    {
      config = clay_procedure_create_config (procedure);
      free_config = TRUE;
    }

  args = clay_procedure_new_arguments (procedure);
  _clay_procedure_config_get_values (config, args);

  return_vals = _clay_procedure_run_array (procedure, args);

  clay_value_array_unref (args);

  if (free_config)
    g_object_unref (config);

  return return_vals;
}

/**
 * _clay_procedure_create_run_config:
 * @procedure: A #ClayProcedure
 *
 * Create a #ClayConfig with properties that match @procedure's arguments, to be
 * used in the run() method for @procedure.
 *
 * This is an internal function to be used only by libclay code.
 *
 * Returns: (transfer full): The new #ClayConfig.
 *
 * Since: 3.0
 **/
ClayProcedureConfig *
_clay_procedure_create_run_config (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return CLAY_PROCEDURE_GET_CLASS (procedure)->create_config (procedure,
                                                              priv->args,
                                                              priv->n_args);
}

ClayValueArray *
_clay_procedure_run_array (ClayProcedure  *procedure,
                           ClayValueArray *args)
{
  ClayProcedurePrivate *priv;
  ClayValueArray       *return_vals;
  GError               *error = NULL;
  gint                  i;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  priv = clay_procedure_get_instance_private (procedure);

  if (! clay_procedure_validate_args (procedure,
                                      priv->args,
                                      priv->n_args,
                                      args, FALSE, &error))
    {
      return_vals = clay_procedure_new_return_values (procedure,
                                                      CLAY_PDB_CALLING_ERROR,
                                                      error);
      return return_vals;
    }

  /*  add missing args with default values  */
  if (clay_value_array_length (args) < priv->n_args)
    {
      ClayProcedureConfig *config;
      GObjectClass        *config_class = NULL;
      ClayValueArray      *complete;

      /*  if saved defaults exist, they override GParamSpec  */
      config = _clay_procedure_create_run_config (procedure);
      if (clay_procedure_config_load_default (config, NULL))
        config_class = G_OBJECT_GET_CLASS (config);
      else
        g_clear_object (&config);

      complete = clay_value_array_new (priv->n_args);

      for (i = 0; i < priv->n_args; i++)
        {
          GParamSpec *pspec = priv->args[i];
          GValue      value = G_VALUE_INIT;

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

          if (i < clay_value_array_length (args))
            {
              GValue *orig = clay_value_array_index (args, i);

              g_value_copy (orig, &value);
            }
          else if (config &&
                   g_object_class_find_property (config_class, pspec->name))
            {
              g_object_get_property (G_OBJECT (config), pspec->name,
                                     &value);
            }
          else
            {
              g_param_value_set_default (pspec, &value);
            }

          clay_value_array_append (complete, &value);
          g_value_unset (&value);
        }

      g_clear_object (&config);

      /*  call the procedure  */
      return_vals = CLAY_PROCEDURE_GET_CLASS (procedure)->run (procedure,
                                                               complete);
      clay_value_array_unref (complete);
    }
  else
    {
      /*  call the procedure  */
      return_vals = CLAY_PROCEDURE_GET_CLASS (procedure)->run (procedure,
                                                               args);
    }

  if (! return_vals)
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      error = g_error_new (0, 0,
                           _("Procedure '%s' returned no return values"),
                           clay_procedure_get_name (procedure));

      return_vals = clay_procedure_new_return_values (procedure,
                                                      CLAY_PDB_EXECUTION_ERROR,
                                                      error);
    }

  return return_vals;
}

/**
 * clay_procedure_persistent_ready:
 * @procedure: A #ClayProcedure
 *
 * Notify the main Clay application that the persistent procedure has
 * been properly initialized and is ready to run.
 *
 * This function _must_ be called from every procedure's [callback@RunFunc]
 * that was created as [enum@Clay.PDBProcType.PERSISTENT].
 *
 * Subsequently, persistent procedures can process temporary procedure
 * run requests using either [method@PlugIn.persistent_enable] or
 * [method@PlugIn.persistent_process].
 *
 * See also: [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
void
clay_procedure_persistent_ready (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;
  ClayPlugIn           *plug_in;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->proc_type == CLAY_PDB_PROC_TYPE_PERSISTENT);

  plug_in = clay_procedure_get_plug_in (procedure);

  if (! gp_persistent_ack_write (_clay_plug_in_get_write_channel (plug_in),
                                 plug_in))
    _clay_quit ();
}

/**
 * clay_procedure_create_config:
 * @procedure: A #ClayProcedure
 *
 * Create a #ClayConfig with properties that match @procedure's arguments, to be
 * used in [method@Procedure.run_config] method.
 *
 * Returns: (transfer full): The new #ClayConfig.
 *
 * Since: 3.0
 **/
ClayProcedureConfig *
clay_procedure_create_config (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  return clay_procedure_create_config_with_prefix (procedure,
                                                   "ClayProcedureConfig",
                                                   priv->args,
                                                   priv->n_args);
}

/**
 * clay_procedure_is_internal:
 * @procedure: A #ClayProcedure
 *
 * Provide the information if @procedure is an internal procedure. Only
 * a procedure looked up in the [class@Clay.PDB] can be internal.
 * Procedures created by a plug-in in particular are never internal.
 *
 * Returns: Whether @procedure is an internal procedure or not.
 *
 * Since: 3.0
 **/
gboolean
clay_procedure_is_internal (ClayProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), FALSE);

  if (CLAY_IS_PDB_PROCEDURE (procedure))
    {
      ClayProcedurePrivate *priv = clay_procedure_get_instance_private (procedure);

      return (priv->proc_type == CLAY_PDB_PROC_TYPE_INTERNAL);
    }
  else
    {
      return FALSE;
    }
}

/**
 * _clay_procedure_get_ref_count:
 * @procedure: A #ClayProcedure
 *
 * Internal function to count the number of reference held by this
 * procedure inside its list of arguments's defaults.
 *
 * Returns: a reference count, which you will probably have to multiply
 *          by 2, if a config object has been created too.
 *
 * Since: 3.0
 **/
gint
_clay_procedure_get_ref_count (ClayProcedure *procedure,
                               GObject       *object)
{
  ClayProcedurePrivate *priv;
  gint                  ref_count = 0;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), 0);

  priv = clay_procedure_get_instance_private (procedure);
  for (gint i = 0; i < priv->n_args; i++)
    {
      GParamSpec *pspec = priv->args[i];

      if (CLAY_IS_PARAM_SPEC_OBJECT (pspec))
        {
          if (clay_param_spec_object_has_default (pspec) &&
              clay_param_spec_object_get_default (pspec) == object)
            ref_count++;
        }
    }

  return ref_count;
}


/*  private functions  */

/**
 * clay_procedure_new_arguments:
 * @procedure: the #ClayProcedure.
 *
 * Format the expected argument values of procedures, in the order as
 * added with [method@Procedure.add_argument].
 *
 * Returns: (transfer full): the expected #ClayValueArray which could be given as
 *          arguments to run @procedure, with all values set to
 *          defaults. Free with [method@Clay.ValueArray.unref].
 *
 * Since: 3.0
 **/
static ClayValueArray *
clay_procedure_new_arguments (ClayProcedure *procedure)
{
  ClayProcedurePrivate *priv;
  ClayValueArray       *args;
  GValue                value = G_VALUE_INIT;
  gint                  i;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), NULL);

  priv = clay_procedure_get_instance_private (procedure);

  args = clay_value_array_new (priv->n_args);

  for (i = 0; i < priv->n_args; i++)
    {
      GParamSpec *pspec = priv->args[i];

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &value);
      clay_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

static gboolean
clay_procedure_validate_args (ClayProcedure   *procedure,
                              GParamSpec     **param_specs,
                              gint             n_param_specs,
                              ClayValueArray  *args,
                              gboolean         return_vals,
                              GError         **error)
{
  gint i;

  for (i = 0; i < MIN (clay_value_array_length (args), n_param_specs); i++)
    {
      GValue     *arg       = clay_value_array_index (args, i);
      GParamSpec *pspec     = param_specs[i];
      GType       arg_type  = G_VALUE_TYPE (arg);
      GType       spec_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (! g_type_is_a (arg_type, spec_type))
        {
          if (return_vals)
            {
              g_set_error (error,
                           CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                           _("Procedure '%s' returned a wrong value type "
                             "for return value '%s' (#%d). "
                             "Expected %s, got %s."),
                           clay_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }
          else
            {
              g_set_error (error,
                           CLAY_PDB_ERROR, CLAY_PDB_ERROR_INVALID_ARGUMENT,
                           _("Procedure '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           clay_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else if (! (pspec->flags & CLAY_PARAM_NO_VALIDATE))
        {
          GValue string_value = G_VALUE_INIT;

          g_value_init (&string_value, G_TYPE_STRING);

          if (g_value_type_transformable (arg_type, G_TYPE_STRING))
            g_value_transform (arg, &string_value);
          else
            g_value_set_static_string (&string_value,
                                       "<not transformable to string>");

          if (g_param_value_validate (pspec, arg))
            {
              const gchar *value = g_value_get_string (&string_value);

              if (value == NULL)
                value = "(null)";

              if (return_vals)
                {
                  g_set_error (error,
                               CLAY_PDB_ERROR,
                               CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned "
                                 "'%s' as return value '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               clay_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }
              else
                {
                  g_set_error (error,
                               CLAY_PDB_ERROR,
                               CLAY_PDB_ERROR_INVALID_ARGUMENT,
                               _("Procedure '%s' has been called with "
                                 "value '%s' for argument '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               clay_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }

              g_value_unset (&string_value);

              return FALSE;
            }

          /*  UTT-8 validate all strings  */
          if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
              (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_BOXED &&
               G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRV))
            {
              gboolean valid = TRUE;

              if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING)
                {
                  const gchar *string = g_value_get_string (arg);

                  if (string)
                    valid = g_utf8_validate (string, -1, NULL);
                }
              else
                {
                  const gchar **strings = g_value_get_boxed (arg);

                  for (const gchar **sp = strings; sp && *sp && valid; sp++)
                    valid = g_utf8_validate (*sp, -1, NULL);
                }

              if (! valid)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   clay_procedure_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   CLAY_PDB_ERROR,
                                   CLAY_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   clay_procedure_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }

                  g_value_unset (&string_value);

                  return FALSE;
                }
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}

static void
clay_procedure_set_icon (ClayProcedure *procedure,
                         ClayIconType   icon_type,
                         gconstpointer  icon_data)
{
  ClayProcedurePrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));

  priv = clay_procedure_get_instance_private (procedure);

  if (icon_data == priv->icon_data)
    return;

  switch (priv->icon_type)
    {
    case CLAY_ICON_TYPE_ICON_NAME:
      g_clear_pointer (&priv->icon_data, g_free);
      break;

    case CLAY_ICON_TYPE_PIXBUF:
    case CLAY_ICON_TYPE_IMAGE_FILE:
      g_clear_object (&priv->icon_data);
      break;
    }

  priv->icon_type = icon_type;

  switch (icon_type)
    {
    case CLAY_ICON_TYPE_ICON_NAME:
      priv->icon_data = g_strdup (icon_data);
      break;

    case CLAY_ICON_TYPE_PIXBUF:
    case CLAY_ICON_TYPE_IMAGE_FILE:
      g_set_object (&priv->icon_data, (GObject *) icon_data);
      break;

    default:
      g_return_if_reached ();
    }

  if (priv->installed)
    clay_procedure_install_icon (procedure);
}
