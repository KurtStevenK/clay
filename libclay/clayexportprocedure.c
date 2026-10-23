/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportprocedure.c
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

#include "clay.h"

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "claypdb_pdb.h"
#include "clayplugin-private.h"
#include "clayexportprocedure.h"
#include "clayprocedureconfig-private.h"

#include "libclay-intl.h"


/**
 * ClayExportProcedure:
 *
 * Export procedures implement image export.
 *
 * Registered export procedures will be automatically available in the export
 * interfaces and functions of Clay. The detection (to decide which file is
 * redirected to which plug-in procedure) depends on the various methods set
 * with [class@FileProcedure] API.
 **/

enum
{
  PROP_0,
  PROP_CAPABILITIES,
  PROP_SUPPORTS_EXIF,
  PROP_SUPPORTS_IPTC,
  PROP_SUPPORTS_XMP,
  PROP_SUPPORTS_PROFILE,
  PROP_SUPPORTS_THUMBNAIL,
  PROP_SUPPORTS_COMMENT,
  N_PROPS
};

struct _ClayExportProcedure
{
  ClayFileProcedure             parent_instance;

  ClayRunExportFunc             run_func;
  gpointer                      run_data;
  GDestroyNotify                run_data_destroy;

  ClayExportCapabilities        capabilities;
  ClayExportGetCapabilitiesFunc get_capabilities_func;
  gpointer                      get_capabilities_data;
  GDestroyNotify                get_capabilities_data_destroy;

  gboolean                      supports_exif;
  gboolean                      supports_iptc;
  gboolean                      supports_xmp;
  gboolean                      supports_profile;
  gboolean                      supports_thumbnail;
  gboolean                      supports_comment;

  gboolean                      export_metadata;
};


static void   clay_export_procedure_constructed    (GObject              *object);
static void   clay_export_procedure_finalize       (GObject              *object);
static void   clay_export_procedure_set_property   (GObject              *object,
                                                    guint                 property_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);
static void   clay_export_procedure_get_property   (GObject              *object,
                                                    guint                 property_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);

static void   clay_export_procedure_install        (ClayProcedure        *procedure);
static ClayValueArray *
              clay_export_procedure_run            (ClayProcedure        *procedure,
                                                    const ClayValueArray *args);
static ClayProcedureConfig *
              clay_export_procedure_create_config  (ClayProcedure        *procedure,
                                                    GParamSpec          **args,
                                                    gint                  n_args);

static gint   clay_export_procedure_add_metadata   (ClayExportProcedure  *export_procedure);

static void   clay_export_procedure_update_options (ClayProcedureConfig  *config,
                                                    GParamSpec           *param,
                                                    gpointer              data);


G_DEFINE_TYPE (ClayExportProcedure, clay_export_procedure, CLAY_TYPE_FILE_PROCEDURE)

#define parent_class clay_export_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static void
clay_export_procedure_class_init (ClayExportProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed      = clay_export_procedure_constructed;
  object_class->finalize         = clay_export_procedure_finalize;
  object_class->get_property     = clay_export_procedure_get_property;
  object_class->set_property     = clay_export_procedure_set_property;

  procedure_class->install       = clay_export_procedure_install;
  procedure_class->run           = clay_export_procedure_run;
  procedure_class->create_config = clay_export_procedure_create_config;

  /**
   * ClayExportProcedure:capabilities:
   *
   * What #ClayExportCapabilities are supported
   *
   * Since: 3.0.0
   */
  props[PROP_CAPABILITIES] = g_param_spec_flags ("capabilities",
                                                 "Supported image capabilities",
                                                 NULL,
                                                 CLAY_TYPE_EXPORT_CAPABILITIES,
                                                 0,
                                                 G_PARAM_CONSTRUCT |
                                                 CLAY_PARAM_READWRITE);

  /**
   * ClayExportProcedure:supports-exif:
   *
   * Whether the export procedure supports EXIF.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_EXIF] = g_param_spec_boolean ("supports-exif",
                                                    "Supports EXIF metadata storage",
                                                    NULL,
                                                    FALSE,
                                                    G_PARAM_CONSTRUCT |
                                                    CLAY_PARAM_READWRITE);
  /**
   * ClayExportProcedure:supports-iptc:
   *
   * Whether the export procedure supports IPTC.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_IPTC] = g_param_spec_boolean ("supports-iptc",
                                                    "Supports IPTC metadata storage",
                                                    NULL,
                                                    FALSE,
                                                    G_PARAM_CONSTRUCT |
                                                    CLAY_PARAM_READWRITE);
  /**
   * ClayExportProcedure:supports-xmp:
   *
   * Whether the export procedure supports XMP.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_XMP] = g_param_spec_boolean ("supports-xmp",
                                                   "Supports XMP metadata storage",
                                                   NULL,
                                                   FALSE,
                                                   G_PARAM_CONSTRUCT |
                                                   CLAY_PARAM_READWRITE);
  /**
   * ClayExportProcedure:supports-profile:
   *
   * Whether the export procedure supports ICC color profiles.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_PROFILE] = g_param_spec_boolean ("supports-profile",
                                                       "Supports color profile storage",
                                                       NULL,
                                                       FALSE,
                                                       G_PARAM_CONSTRUCT |
                                                       CLAY_PARAM_READWRITE);
  /**
   * ClayExportProcedure:supports-thumbnail:
   *
   * Whether the export procedure supports storing a thumbnail.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_THUMBNAIL] = g_param_spec_boolean ("supports-thumbnail",
                                                         "Supports thumbnail storage",
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT |
                                                         CLAY_PARAM_READWRITE);
  /**
   * ClayExportProcedure:supports-comment:
   *
   * Whether the export procedure supports storing a comment.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_COMMENT] = g_param_spec_boolean ("supports-comment",
                                                       "Supports comment storage",
                                                       NULL,
                                                       FALSE,
                                                       G_PARAM_CONSTRUCT |
                                                       CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_export_procedure_init (ClayExportProcedure *procedure)
{
  procedure->export_metadata = FALSE;
  procedure->capabilities    = 0;
}

static void
clay_export_procedure_constructed (GObject *object)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_procedure_add_image_argument (procedure, "image",
                                    "Image",
                                     "The image to export",
                                     FALSE,
                                     G_PARAM_READWRITE);

  clay_procedure_add_file_argument (procedure, "file",
                                    "File",
                                    "The file to export to",
                                    CLAY_FILE_CHOOSER_ACTION_SAVE,
                                    FALSE, NULL,
                                    CLAY_PARAM_READWRITE);

  _clay_procedure_add_argument (procedure,
                                clay_param_spec_export_options ("options", "Options",
                                                                "Export options",
                                                                G_PARAM_READWRITE));
}

static void
clay_export_procedure_finalize (GObject *object)
{
  ClayExportProcedure *procedure = CLAY_EXPORT_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_export_procedure_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayExportProcedure *procedure = CLAY_EXPORT_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_CAPABILITIES:
      procedure->capabilities = g_value_get_flags (value);
      break;
    case PROP_SUPPORTS_EXIF:
      procedure->supports_exif = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_IPTC:
      procedure->supports_iptc = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_XMP:
      procedure->supports_xmp = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_PROFILE:
      procedure->supports_profile = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_THUMBNAIL:
      procedure->supports_thumbnail = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_COMMENT:
      procedure->supports_comment = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_export_procedure_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayExportProcedure *procedure = CLAY_EXPORT_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_CAPABILITIES:
      g_value_set_flags (value, procedure->capabilities);
      break;
    case PROP_SUPPORTS_EXIF:
      g_value_set_boolean (value, procedure->supports_exif);
      break;
    case PROP_SUPPORTS_IPTC:
      g_value_set_boolean (value, procedure->supports_iptc);
      break;
    case PROP_SUPPORTS_XMP:
      g_value_set_boolean (value, procedure->supports_xmp);
      break;
    case PROP_SUPPORTS_PROFILE:
      g_value_set_boolean (value, procedure->supports_profile);
      break;
    case PROP_SUPPORTS_THUMBNAIL:
      g_value_set_boolean (value, procedure->supports_thumbnail);
      break;
    case PROP_SUPPORTS_COMMENT:
      g_value_set_boolean (value, procedure->supports_comment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_export_procedure_install (ClayProcedure *procedure)
{
  ClayFileProcedure *file_proc       = CLAY_FILE_PROCEDURE (procedure);
  const gchar       *meta_extensions = NULL;
  const gchar       *mime_types;
  gint               priority;

  clay_export_procedure_add_metadata (CLAY_EXPORT_PROCEDURE (procedure));
  CLAY_PROCEDURE_CLASS (parent_class)->install (procedure);

  (void) clay_file_procedure_get_meta (file_proc, &meta_extensions),
  _clay_pdb_set_file_proc_export_handler (clay_procedure_get_name (procedure),
                                          clay_file_procedure_get_extensions (file_proc),
                                          meta_extensions,
                                          clay_file_procedure_get_prefixes (file_proc));

  if (clay_file_procedure_get_handles_remote (file_proc))
    _clay_pdb_set_file_proc_handles_remote (clay_procedure_get_name (procedure));

  mime_types = clay_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    _clay_pdb_set_file_proc_mime_types (clay_procedure_get_name (procedure),
                                        mime_types);

  priority = clay_file_procedure_get_priority (file_proc);
  if (priority != 0)
    _clay_pdb_set_file_proc_priority (clay_procedure_get_name (procedure),
                                      priority);
}

#define ARG_OFFSET 4

static ClayValueArray *
clay_export_procedure_run (ClayProcedure        *procedure,
                           const ClayValueArray *args)
{
  ClayPlugIn           *plug_in;
  ClayExportProcedure  *export_proc = CLAY_EXPORT_PROCEDURE (procedure);
  ClayValueArray       *remaining;
  ClayValueArray       *return_values;
  ClayProcedureConfig  *config;
  ClayRunMode           run_mode;
  ClayImage            *image;
  GFile                *file;
  ClayMetadata         *metadata;
  ClayExportOptions    *options      = NULL;
  gchar                *mimetype     = NULL;
  ClayPDBStatusType     status       = CLAY_PDB_EXECUTION_ERROR;
  gboolean              free_options = FALSE;

  run_mode = CLAY_VALUES_GET_ENUM  (args, 0);
  image    = CLAY_VALUES_GET_IMAGE (args, 1);
  file     = CLAY_VALUES_GET_FILE  (args, 2);
  options  = g_value_get_object (clay_value_array_index (args, 3));

  remaining = clay_value_array_new (clay_value_array_length (args) - ARG_OFFSET);

  for (gint i = ARG_OFFSET; i < clay_value_array_length (args); i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_array_append (remaining, value);
    }

  config = _clay_procedure_create_run_config (procedure);
  if (export_proc->export_metadata)
    {
      mimetype = (gchar *) clay_file_procedure_get_mime_types (CLAY_FILE_PROCEDURE (procedure));
      if (mimetype == NULL)
        {
          g_printerr ("%s: ERROR: the ClayExportProcedureConfig object was created "
                      "with export_metadata but you didn't set any MimeType with clay_file_procedure_set_mime_types()!\n",
                      G_STRFUNC);
        }
      else
        {
          char *delim;

          mimetype = g_strdup (mimetype);
          mimetype = g_strstrip (mimetype);
          delim = strstr (mimetype, ",");
          if (delim)
            *delim = '\0';
          /* Though docs only writes about the list being comma-separated, our
           * code apparently also split by spaces.
           */
          delim = strstr (mimetype, " ");
          if (delim)
            *delim = '\0';
          delim = strstr (mimetype, "\t");
          if (delim)
            *delim = '\0';
        }
    }
  metadata = _clay_procedure_config_begin_export (config, image, run_mode, remaining, mimetype);
  g_free (mimetype);

  /* ClayExportOptions may be null if called non-interactively from a script,
   * so we'll make sure it's initialized */
  if (options == NULL)
    {
      options      = g_object_new (CLAY_TYPE_EXPORT_OPTIONS, NULL);
      free_options = TRUE;
    }

  if (export_proc->get_capabilities_func != NULL)
    {
      ClayExportCapabilities capabilities;

      capabilities = export_proc->get_capabilities_func (procedure, config, options,
                                                         export_proc->get_capabilities_data);

      g_object_set (G_OBJECT (options),
                    "capabilities", capabilities,
                    NULL);

      g_signal_connect_object (config, "notify",
                               G_CALLBACK (clay_export_procedure_update_options),
                               options, 0);
    }
  else
    {
      g_object_set (options,
                    "capabilities", export_proc->capabilities,
                    NULL);
    }

  return_values = export_proc->run_func (procedure, run_mode, image,
                                         file, options, metadata,
                                         config, export_proc->run_data);

  if (return_values != NULL                       &&
      clay_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (clay_value_array_index (return_values, 0)))
    status = CLAY_VALUES_GET_ENUM (return_values, 0);

  _clay_procedure_config_end_export (config, image, file, status);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = clay_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _clay_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the ClayExportProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  if (free_options)
    g_object_unref (options);
  g_object_unref (config);
  clay_value_array_unref (remaining);

  if (export_proc->get_capabilities_data_destroy && export_proc->get_capabilities_data)
    export_proc->get_capabilities_data_destroy (export_proc->get_capabilities_data);

  return return_values;
}

static ClayProcedureConfig *
clay_export_procedure_create_config (ClayProcedure  *procedure,
                                     GParamSpec    **args,
                                     gint            n_args)
{
  gint new_n_args;

  new_n_args = clay_export_procedure_add_metadata (CLAY_EXPORT_PROCEDURE (procedure));
  new_n_args += n_args;

  /* clay_export_procedure_add_metadata() may modify the args array so
   * our pointer may become invalid (reallocated to a bigger size).
   * We need to requery the new args.
   */
  args = clay_procedure_get_arguments (procedure, &n_args);
  g_return_val_if_fail (new_n_args == n_args, NULL);

  if (n_args > ARG_OFFSET)
    {
      args   += ARG_OFFSET;
      n_args -= ARG_OFFSET;
    }
  else
    {
      args   = NULL;
      n_args = 0;
    }

  return CLAY_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}

static gint
clay_export_procedure_add_metadata (ClayExportProcedure *export_procedure)
{
  ClayProcedure   *procedure = CLAY_PROCEDURE (export_procedure);
  static gboolean  ran_once = FALSE;
  gint             new_args = 0;

  if (ran_once)
    return new_args;

  if (export_procedure->supports_exif)
    {
      clay_procedure_add_boolean_argument (procedure, "include-exif",
                                           _("Save _Exif"),
                                           _("Save Exif (Exchangeable image file format) metadata"),
                                           clay_export_exif (),
                                           G_PARAM_READWRITE);
      new_args++;
    }
  if (export_procedure->supports_iptc)
    {
      clay_procedure_add_boolean_argument (procedure, "include-iptc",
                                           _("Save _IPTC"),
                                           _("Save IPTC (International Press Telecommunications Council) metadata"),
                                           clay_export_iptc (),
                                           G_PARAM_READWRITE);
      new_args++;
    }
  if (export_procedure->supports_xmp)
    {
      clay_procedure_add_boolean_argument (procedure, "include-xmp",
                                           _("Save _XMP"),
                                           _("Save XMP (Extensible Metadata Platform) metadata"),
                                           clay_export_xmp (),
                                           G_PARAM_READWRITE);
      new_args++;
    }
  if (export_procedure->supports_profile)
    {
      clay_procedure_add_boolean_argument (procedure, "include-color-profile",
                                           _("Save color _profile"),
                                           _("Save the ICC color profile as metadata"),
                                           clay_export_color_profile (),
                                           G_PARAM_READWRITE);
      new_args++;
    }
  if (export_procedure->supports_thumbnail)
    {
      clay_procedure_add_boolean_argument (procedure, "include-thumbnail",
                                           _("Save _thumbnail"),
                                           _("Save a smaller representation of the image as metadata"),
                                           clay_export_thumbnail (),
                                           G_PARAM_READWRITE);
      new_args++;
    }
  if (export_procedure->supports_comment)
    {
      clay_procedure_add_boolean_argument (procedure, "include-comment",
                                           _("Save c_omment"),
                                           _("Save a comment as metadata"),
                                           clay_export_comment (),
                                           G_PARAM_READWRITE);
      clay_procedure_add_string_aux_argument (procedure, "clay-comment",
                                              _("Comment"),
                                              _("Image comment"),
                                              clay_get_default_comment (),
                                              G_PARAM_READWRITE);

      clay_procedure_set_argument_sync (procedure, "clay-comment",
                                        CLAY_ARGUMENT_SYNC_PARASITE);
      new_args++;
    }

  ran_once = TRUE;

  return new_args;
}

static void
clay_export_procedure_update_options (ClayProcedureConfig *config,
                                      GParamSpec          *param,
                                      gpointer             data)
{
  ClayProcedure          *procedure;
  ClayExportProcedure    *export_proc;
  ClayExportOptions      *options = CLAY_EXPORT_OPTIONS (data);
  ClayExportCapabilities  capabilities;


  procedure   = clay_procedure_config_get_procedure (config);
  export_proc = CLAY_EXPORT_PROCEDURE (procedure);

  capabilities = export_proc->get_capabilities_func (procedure, config, options,
                                                     export_proc->get_capabilities_data);

  g_object_set (G_OBJECT (options),
                "capabilities", capabilities,
                NULL);
}


/*  public functions  */

/**
 * clay_export_procedure_new:
 * @plug_in:          a #ClayPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #ClayPDBProcType.
 * @export_metadata:  whether Clay should handle metadata exporting.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new export procedure named @name which will call @run_func
 * when invoked.
 *
 * See clay_procedure_new() for information about @proc_type.
 *
 * #ClayExportProcedure is a #ClayProcedure subclass that makes it easier
 * to write file export procedures.
 *
 * It automatically adds the standard
 *
 * (#ClayRunMode, #ClayImage, #GFile, #ClayExportOptions)
 *
 * arguments of an export procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via clay_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #ClayRunExportFunc. The
 * #ClayProcedureConfig of #ClayRunExportFunc only contains additionally added
 * arguments.
 *
 * If @export_metadata is TRUE, then the class will also handle the metadata
 * export if the format is supported by our backend. This requires you to also
 * set appropriate MimeType with clay_file_procedure_set_mime_types().
 *
 * Returns: a new #ClayProcedure.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_export_procedure_new (ClayPlugIn       *plug_in,
                           const gchar      *name,
                           ClayPDBProcType   proc_type,
                           gboolean          export_metadata,
                           ClayRunExportFunc run_func,
                           gpointer          run_data,
                           GDestroyNotify    run_data_destroy)
{
  ClayExportProcedure *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != CLAY_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (CLAY_TYPE_EXPORT_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->export_metadata  = export_metadata;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  return CLAY_PROCEDURE (procedure);
}

/**
 * clay_export_procedure_set_capabilities:
 * @procedure:               a #ClayProcedure.
 * @capabilities:            a #ClayExportCapabilities bitfield.
 * @get_capabilities_func: (nullable): callback function to update export options
 * @get_capabilities_data: (nullable): data for @get_capabilities_func
 * @get_capabilities_data_destroy: (nullable): free function for @get_capabilities_data, or %NULL
 *
 * Sets default #ClayExportCapabilities for image export.
 *
 * @capabilities and @get_capabilities_func are overlapping arguments.
 * Either set @capabilities if your format capabilities are stable or
 * @get_capabilities_func if they depend on other options.
 * If @get_capabilities_func is set, @capabilities must be 0.
 *
 * If set, @get_capabilities_func will be called every time an argument
 * in the [class@Clay.ProcedureConfig] is edited and it will be used to
 * edit the export capabilities dynamically.
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_capabilities (ClayExportProcedure           *procedure,
                                        ClayExportCapabilities         capabilities,
                                        ClayExportGetCapabilitiesFunc  get_capabilities_func,
                                        gpointer                       get_capabilities_data,
                                        GDestroyNotify                 get_capabilities_data_destroy)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));
  g_return_if_fail (get_capabilities_func == NULL || capabilities == 0);

  /* Assign default image capabilities */
  g_object_set (procedure,
                "capabilities", capabilities,
                NULL);

  /* TODO: Do more with this when we have user-specified callbacks for
   * image capabilities */
  if (get_capabilities_func != NULL)
    {
      procedure->get_capabilities_func         = get_capabilities_func;
      procedure->get_capabilities_data         = get_capabilities_data;
      procedure->get_capabilities_data_destroy = get_capabilities_data_destroy;
    }
}

/**
 * clay_export_procedure_set_export_metadata:
 * @procedure:        a #ClayProcedure.
 * @export_metadata:  whether metadata will be exported
 *
 * Determine whether @procedure will export metadata. This can be used if
 * an image export procedure has multiple variants where we need to determine
 * metadata export status based on user options.
 *
 * Since: 3.4
 **/
void
clay_export_procedure_set_export_metadata (ClayExportProcedure *procedure,
                                           gboolean             export_metadata)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  procedure->export_metadata = export_metadata;
}

/**
 * clay_export_procedure_set_support_exif:
 * @procedure: a #ClayProcedure.
 * @supports:  whether Exif metadata are supported.
 *
 * Determine whether @procedure supports exporting Exif data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-exif" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other `clay_export_procedure_set_support_*()` functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated
 *   [`ClayExportProcedureDialog`](https://developer.gimp.org/api/3.0/libclayui/class.ExportProcedureDialog.html)
 *   will contain the metadata options, once again always in the same
 *   order and with consistent GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_exif].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_exif (ClayExportProcedure *procedure,
                                        gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-exif", supports,
                NULL);
}

/**
 * clay_export_procedure_set_support_iptc:
 * @procedure: a #ClayProcedure.
 * @supports:  whether IPTC metadata are supported.
 *
 * Determine whether @procedure supports exporting IPTC data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-iptc" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other clay_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated ClayExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_iptc].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_iptc (ClayExportProcedure *procedure,
                                        gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-iptc", supports,
                NULL);
}

/**
 * clay_export_procedure_set_support_xmp:
 * @procedure: a #ClayProcedure.
 * @supports:  whether XMP metadata are supported.
 *
 * Determine whether @procedure supports exporting XMP data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-xmp" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other clay_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated ClayExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_xmp].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_xmp (ClayExportProcedure *procedure,
                                       gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-xmp", supports,
                NULL);
}

/**
 * clay_export_procedure_set_support_profile:
 * @procedure: a #ClayProcedure.
 * @supports:  whether color profiles can be stored.
 *
 * Determine whether @procedure supports exporting ICC color profiles. By
 * default, it won't (so there is usually no reason to run this function
 * with %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-color-profile" in
 *   the end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other clay_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated ClayExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_color_profile].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_profile (ClayExportProcedure *procedure,
                                           gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-profile", supports,
                NULL);
}

/**
 * clay_export_procedure_set_support_thumbnail:
 * @procedure: a #ClayProcedure.
 * @supports:  whether a thumbnail can be stored.
 *
 * Determine whether @procedure supports exporting a thumbnail. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-thumbnail" in the
 *   end of the argument list of @procedure, with relevant blurb
 *   and description.
 * - If used with other clay_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated ClayExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_thumbnail].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_thumbnail (ClayExportProcedure *procedure,
                                             gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-thumbnail", supports,
                NULL);
}

/**
 * clay_export_procedure_set_support_comment:
 * @procedure: a #ClayProcedure.
 * @supports:  whether a comment can be stored.
 *
 * Determine whether @procedure supports exporting a comment. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard argument "include-comment" in the end
 *   of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other clay_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated ClayExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * By default, the value will be [func@export_comment].
 *
 * Since: 3.0
 **/
void
clay_export_procedure_set_support_comment (ClayExportProcedure *procedure,
                                           gboolean             supports)
{
  g_return_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-comment", supports,
                NULL);
}

/**
 * clay_export_procedure_get_export_metadata:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure is set to export metadata.
 *
 * Since: 3.4
 **/
gboolean
clay_export_procedure_get_export_metadata (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->export_metadata;
}

/**
 * clay_export_procedure_get_support_exif:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports Exif exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_exif (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_exif;
}

/**
 * clay_export_procedure_get_support_iptc:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports IPTC exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_iptc (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_iptc;
}

/**
 * clay_export_procedure_get_support_xmp:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports XMP exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_xmp (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_xmp;
}

/**
 * clay_export_procedure_get_support_profile:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports ICC color profile exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_profile (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_profile;
}

/**
 * clay_export_procedure_get_support_thumbnail:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports thumbnail exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_thumbnail (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_thumbnail;
}

/**
 * clay_export_procedure_get_support_comment:
 * @procedure: a #ClayProcedure.
 *
 * Returns: %TRUE if @procedure supports comment exporting.
 *
 * Since: 3.0
 **/
gboolean
clay_export_procedure_get_support_comment (ClayExportProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->supports_comment;
}
