/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayprocedureconfig.c
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

#include "config.h"

#include "clay.h"
#include "clayimagemetadata.h"

#include "clayprocedureconfig-private.h"


/**
 * ClayProcedureConfig:
 *
 * The base class for [class@Procedure] specific config objects and the main
 * interface to manage aspects of [class@Procedure]'s arguments such as
 * persistency of the last used arguments across Clay sessions.
 *
 * A procedure config is created by a [class@Procedure] using
 * [method@Procedure.create_config] and its properties match the
 * procedure's arguments and auxiliary arguments in number, order and
 * type.
 *
 * It implements the [struct@Config] interface and therefore has all its
 * serialization and deserialization features.
 *
 * Since: 3.0
 **/


enum
{
  PROP_0,
  PROP_PROCEDURE,
  N_PROPS
};


typedef struct _ClayProcedureConfigPrivate
{
  ClayProcedure         *procedure;

  ClayImage             *image;
  ClayRunMode            run_mode;

  ClayMetadata          *metadata;
  gchar                 *mime_type;
  ClayMetadataSaveFlags  metadata_flags;
  gboolean               metadata_saved;
} ClayProcedureConfigPrivate;

#define GET_PRIVATE(obj) ((ClayProcedureConfigPrivate *) clay_procedure_config_get_instance_private ((ClayProcedureConfig *) (obj)))


static void       clay_procedure_config_constructed   (GObject             *object);
static void       clay_procedure_config_dispose       (GObject             *object);
static void       clay_procedure_config_set_property  (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void       clay_procedure_config_get_property  (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);


static void       clay_procedure_config_set_values    (ClayProcedureConfig  *config,
                                                       const ClayValueArray *values);

static gboolean   clay_procedure_config_load_last     (ClayProcedureConfig  *config,
                                                        GError              **error);
static gboolean   clay_procedure_config_save_last     (ClayProcedureConfig  *config,
                                                       GError              **error);

static gboolean   clay_procedure_config_load_parasite (ClayProcedureConfig  *config,
                                                       ClayImage            *image,
                                                       GError              **error);
static gboolean   clay_procedure_config_save_parasite (ClayProcedureConfig  *config,
                                                       ClayImage            *image,
                                                       GError              **error);

static GFile    * clay_procedure_config_get_file      (ClayProcedureConfig *config,
                                                       const gchar         *extension);
static gchar    * clay_procedure_config_parasite_name (ClayProcedureConfig *config,
                                                       const gchar         *suffix);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayProcedureConfig, clay_procedure_config,
                                     G_TYPE_OBJECT)

#define parent_class clay_procedure_config_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static const struct
{
  const gchar           *name;
  ClayMetadataSaveFlags  flag;
}
metadata_properties[] =
{
  { "include-exif",          CLAY_METADATA_SAVE_EXIF          },
  { "include-xmp",           CLAY_METADATA_SAVE_XMP           },
  { "include-iptc",          CLAY_METADATA_SAVE_IPTC          },
  { "include-thumbnail",     CLAY_METADATA_SAVE_THUMBNAIL     },
  { "include-color-profile", CLAY_METADATA_SAVE_COLOR_PROFILE },
  { "include-comment",       CLAY_METADATA_SAVE_COMMENT       }
};


static void
clay_procedure_config_class_init (ClayProcedureConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_procedure_config_constructed;
  object_class->dispose      = clay_procedure_config_dispose;
  object_class->set_property = clay_procedure_config_set_property;
  object_class->get_property = clay_procedure_config_get_property;

  props[PROP_PROCEDURE] =
    g_param_spec_object ("procedure",
                         "Procedure",
                         "The procedure this config object is used for",
                         CLAY_TYPE_PROCEDURE,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_procedure_config_init (ClayProcedureConfig *config)
{
  GET_PRIVATE (config)->run_mode = -1;
}

static void
clay_procedure_config_constructed (GObject *object)
{
  ClayProcedureConfig *config = CLAY_PROCEDURE_CONFIG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (CLAY_IS_PROCEDURE (GET_PRIVATE (config)->procedure));
}

static void
clay_procedure_config_dispose (GObject *object)
{
  ClayProcedureConfig        *config = CLAY_PROCEDURE_CONFIG (object);
  ClayProcedureConfigPrivate *priv = GET_PRIVATE (config);

  g_clear_object (&priv->procedure);
  g_clear_object (&priv->metadata);
  g_clear_pointer (&priv->mime_type, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_procedure_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayProcedureConfig *config = CLAY_PROCEDURE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      GET_PRIVATE (config)->procedure = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_procedure_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayProcedureConfig *config = CLAY_PROCEDURE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, GET_PRIVATE (config)->procedure);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * clay_procedure_config_get_procedure:
 * @config: a procedure config
 *
 * This function returns the [class@Procedure] which created @config, see
 * [method@Procedure.create_config].
 *
 * Returns: (transfer none): The procedure which created this config.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_procedure_config_get_procedure (ClayProcedureConfig *config)
{
  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), NULL);

  return GET_PRIVATE (config)->procedure;
}

static void
clay_procedure_config_get_parasite (ClayProcedureConfig *config,
                                    GParamSpec          *pspec)
{
  ClayParasite *parasite;
  gchar        *value = NULL;

  /*  for now we only support strings  */
  if (! GET_PRIVATE (config)->image ||
      ! G_IS_PARAM_SPEC_STRING (pspec))
    return;

  parasite = clay_image_get_parasite (GET_PRIVATE (config)->image,
                                      pspec->name);

  if (parasite)
    {
      guint32 parasite_size;

      value = (gchar *) clay_parasite_get_data (parasite, &parasite_size);
      value = g_strndup (value, parasite_size);
      clay_parasite_free (parasite);

      if (value && ! strlen (value))
        g_clear_pointer (&value, g_free);
    }

  if (! value)
    {
      /*  special case "clay-comment" here, yes this is bad hack  */
      if (! strcmp (pspec->name, "clay-comment"))
        {
          value = clay_get_default_comment ();
        }
    }

  if (value && strlen (value))
    g_object_set (config,
                  pspec->name, value,
                  NULL);

  g_free (value);
}

static void
clay_procedure_config_set_parasite (ClayProcedureConfig *config,
                                    GParamSpec          *pspec)
{
  ClayProcedureConfigPrivate *priv = GET_PRIVATE (config);
  ClayParasite *parasite;
  gchar        *value;

  /*  for now we only support strings  */
  if (! priv->image || ! G_IS_PARAM_SPEC_STRING (pspec))
    return;

  g_object_get (config,
                pspec->name, &value,
                NULL);

  parasite = clay_image_get_parasite (priv->image, pspec->name);

  if (parasite)
    {
      /*  it there is a parasite, always override it if its value was
       *  changed
       */
      gchar   *image_value;
      guint32  parasite_size;

      image_value = (gchar *) clay_parasite_get_data (parasite, &parasite_size);
      image_value = g_strndup (image_value, parasite_size);
      clay_parasite_free (parasite);

      if (g_strcmp0 (value, image_value))
        {
          if (value && strlen (value))
            {
              parasite = clay_parasite_new (pspec->name,
                                            CLAY_PARASITE_PERSISTENT,
                                            strlen (value) + 1,
                                            value);
              clay_image_attach_parasite (priv->image, parasite);
              clay_parasite_free (parasite);
            }
          else
            {
              clay_image_detach_parasite (priv->image, pspec->name);
            }
        }

      g_free (image_value);
    }
  else
    {
      /*  otherwise, set the parasite if the value was changed from
       *  the default value
       */
      gchar *default_value = NULL;

      /*  special case "clay-comment" here, yes this is bad hack  */
      if (! strcmp (pspec->name, "clay-comment"))
        {
          default_value = clay_get_default_comment ();
        }

      if (g_strcmp0 (value, default_value) &&
          value && strlen (value))
        {
          parasite = clay_parasite_new (pspec->name,
                                        CLAY_PARASITE_PERSISTENT,
                                        strlen (value) + 1,
                                        value);
          clay_image_attach_parasite (priv->image, parasite);
          clay_parasite_free (parasite);
        }

      g_free (default_value);
    }

  g_free (value);
}

/**
 * clay_procedure_config_save_metadata:
 * @config:         a #ClayProcedureConfig
 * @exported_image: the image that was actually exported
 * @file:           the file @exported_image was written to
 *
 * *Note: There is normally no need to call this function because it's
 * already called by [class@ExportProcedure] after the `run()` callback.*
 *
 * *Only use this function if the [class@Metadata] passed as argument of a
 * [class@ExportProcedure]'s run() method needs to be written at a specific
 * point of the export, other than its end.*
 *
 * This function syncs back @config's export properties to the
 * metadata's [flags@MetadataSaveFlags] and writes the metadata to
 * @file.
 *
 * The metadata is only ever written once. If this function has been
 * called explicitly, it will do nothing when called a second time at the end of
 * the `run()` callback.
 *
 * Since: 3.0
 **/
void
clay_procedure_config_save_metadata (ClayProcedureConfig *config,
                                     ClayImage           *exported_image,
                                     GFile               *file)
{
  ClayProcedureConfigPrivate *priv = GET_PRIVATE (config);

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (CLAY_IS_IMAGE (exported_image));
  g_return_if_fail (G_IS_FILE (file));

  if (priv->metadata && ! priv->metadata_saved)
    {
      GObjectClass *object_class = G_OBJECT_GET_CLASS (config);
      GError       *error        = NULL;
      gint          i;

      for (i = 0; i < G_N_ELEMENTS (metadata_properties); i++)
        {
          const gchar           *prop_name = metadata_properties[i].name;
          ClayMetadataSaveFlags  prop_flag = metadata_properties[i].flag;
          GParamSpec            *pspec;
          gboolean               value;

          pspec = g_object_class_find_property (object_class, prop_name);
          if (pspec)
            {
              g_object_get (config,
                            prop_name, &value,
                            NULL);

              if (value)
                priv->metadata_flags |= prop_flag;
              else
                priv->metadata_flags &= ~prop_flag;
            }
          else
            {
              priv->metadata_flags &= ~prop_flag;
            }
        }

      if (clay_update_metadata () &&
          (priv->metadata_flags & CLAY_METADATA_SAVE_COMMENT))
        {
          gchar *comment_value = NULL;
          gint   len;

          /* To be able to synchronize the comment with metadata in
           * _clay_image_metadata_save_finish, we need the comment value
           * that was possibly updated in the export dialog.
           */

          g_object_get (config,
                        "clay-comment", &comment_value,
                        NULL);
          if (comment_value && (len = strlen (comment_value)) > 0)
            {
              ClayParasite *parasite;

              parasite = clay_parasite_new ("clay-comment",
                                            CLAY_PARASITE_PERSISTENT,
                                            len + 1, comment_value);

              clay_image_attach_parasite (exported_image, parasite);
              clay_parasite_free (parasite);
            }
          g_free (comment_value);
        }

      if (! _clay_image_metadata_save_finish (exported_image,
                                              priv->mime_type,
                                              priv->metadata,
                                              priv->metadata_flags,
                                              file, &error))
        {
          if (error)
            {
              /* Even though a failure to write metadata is not enough
                 reason to say we failed to save the image, we should
                 still notify the user about the problem. */
              g_message ("%s: saving metadata failed: %s",
                         G_STRFUNC, error->message);
              g_error_free (error);
            }
        }

      priv->metadata_saved = TRUE;
    }
}

/**
 * clay_procedure_config_get_core_object_array:
 * @config:        a #ClayProcedureConfig
 * @property_name: the name of a #ClayParamSpecCoreObjectArray param spec.
 *
 * A function for bindings to get a [type@CoreObjectArray] property. Getting
 * these with [method@GObject.Object.get] or [method@GObject.Object.get_property] won't
 * [work for the time being](https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/492)
 * so all our boxed array types must be set and get using alternative
 * functions instead.
 *
 * C plug-ins should just use [method@GObject.Object.get].
 *
 * Returns: (array zero-terminated) (transfer container): an array of #GObjects.
 *
 * Since: 3.0
 **/
GObject **
clay_procedure_config_get_core_object_array (ClayProcedureConfig  *config,
                                             const gchar          *property_name)
{
  GObject    **objects = NULL;
  GParamSpec  *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return objects;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY))
    {
      g_warning ("%s: property '%s' of %s is not a ClayParamCoreObjectArray.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return objects;
    }

  g_object_get (config,
                property_name, &objects,
                NULL);

  return objects;
}

/**
 * clay_procedure_config_set_core_object_array:
 * @config:        a #ClayProcedureConfig
 * @property_name: the name of a #ClayParamSpecCoreObjectArray param spec.
 * @objects: (array length=n_objects) (transfer none): an array of #GObjects.
 * @n_objects: the numbers of @objects.
 *
 * A function for bindings to set a [type@CoreObjectArray] property. Setting
 * these with [method@GObject.Object.set] or [method@GObject.Object.set_property] won't
 * [work for the time being](https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/492)
 * so all our boxed array types must be set and get using alternative
 * functions instead.
 *
 * C plug-ins should just use [method@GObject.Object.set].
 *
 * Since: 3.0
 **/
void
clay_procedure_config_set_core_object_array (ClayProcedureConfig  *config,
                                             const gchar          *property_name,
                                             GObject             **objects,
                                             gsize                 n_objects)
{
  GObject    **null_objects;
  GParamSpec  *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY))
    {
      g_warning ("%s: property '%s' of %s is not a ClayParamCoreObjectArray.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return;
    }

  null_objects = g_new0 (GObject *, n_objects + 1);

  for (gint i = 0; i < n_objects; i++)
    null_objects[i] = objects[i];

  g_object_set (config,
                property_name, null_objects,
                NULL);

  g_free (null_objects);
}

/**
 * clay_procedure_config_get_color_array:
 * @config:        a #ClayProcedureConfig
 * @property_name: the name of a #GParamSpecBoxed param spec with [type@ColorArray] value type.
 *
 * A function for bindings to get a [type@ColorArray] property. Getting
 * these with [method@GObject.Object.get] or [method@GObject.Object.get_property] won't
 * [work for the time being](https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/492)
 * so all our boxed array types must be set and get using these
 * alternative functions instead.
 *
 * C plug-ins should just use [method@GObject.Object.get].
 *
 * Returns: (array zero-terminated) (transfer full): an array of #GObjects.
 *
 * Since: 3.0
 **/
GeglColor **
clay_procedure_config_get_color_array (ClayProcedureConfig  *config,
                                       const gchar          *property_name)
{
  GeglColor  **colors = NULL;
  GParamSpec  *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return colors;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_BOXED) ||
      ! g_type_is_a (param_spec->value_type, CLAY_TYPE_COLOR_ARRAY))
    {
      g_warning ("%s: property '%s' of %s is not a ClayColorArray boxed type.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return colors;
    }

  g_object_get (config,
                property_name, &colors,
                NULL);

  return colors;
}

/**
 * clay_procedure_config_set_color_array:
 * @config:        a #ClayProcedureConfig
 * @property_name: the name of a #GParamSpecBoxed param spec with [type@ColorArray] value type.
 * @colors: (array length=n_colors) (transfer none): an array of [class@Gegl.Color].
 * @n_colors: the numbers of @colors.
 *
 * A function for bindings to set a [type@ColorArray] property. Setting
 * these with [method@GObject.Object.set] or [method@GObject.Object.set_property] won't
 * [work for the time being](https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/492)
 * so all our boxed array types must be set and get using these
 * alternative functions instead.
 *
 * C plug-ins should just use [method@GObject.Object.set].
 *
 * Since: 3.0
 **/
void
clay_procedure_config_set_color_array (ClayProcedureConfig  *config,
                                       const gchar          *property_name,
                                       GeglColor           **colors,
                                       gsize                 n_colors)
{
  GeglColor  **null_colors;
  GParamSpec  *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_BOXED) ||
      ! g_type_is_a (param_spec->value_type, CLAY_TYPE_COLOR_ARRAY))
    {
      g_warning ("%s: property '%s' of %s is not a ClayColorArray boxed type.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return;
    }

  null_colors = g_new0 (GeglColor *, n_colors + 1);

  for (gint i = 0; i < n_colors; i++)
    null_colors[i] = colors[i];

  g_object_set (config,
                property_name, null_colors,
                NULL);

  g_free (null_colors);
}

/**
 * clay_procedure_config_get_choice_id:
 * @config:        a #ClayProcedureConfig
 * @property_name: the name of a #ClayParamSpecChoice property.
 *
 * A utility function which will get the current string value of a
 * #ClayParamSpecChoice property in @config and convert it to the
 * integer ID mapped to this value.
 * This makes it easy to work with an Enum type locally, within a plug-in code.
 *
 * Since: 3.0
 **/
gint
clay_procedure_config_get_choice_id (ClayProcedureConfig *config,
                                     const gchar         *property_name)
{
  GParamSpec *param_spec;
  ClayChoice *choice;
  gchar      *value = NULL;
  gint        id;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return 0;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), CLAY_TYPE_PARAM_CHOICE))
    {
      g_warning ("%s: property '%s' of %s is a %s, not a ClayParamSpecChoice.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (G_TYPE_FROM_INSTANCE (param_spec)),
                 g_type_name (param_spec->owner_type));
      return 0;
    }

  choice = clay_param_spec_choice_get_choice (param_spec);
  g_object_get (config,
                property_name, &value,
                NULL);
  id = clay_choice_get_id (choice, value);

  g_free (value);

  return id;
}


/*  Functions only used by ClayProcedure classes  */

/**
 * _clay_procedure_config_get_values:
 * @config: a #ClayProcedureConfig
 * @values: a #ClayValueArray
 *
 * Gets the values from @config's properties and stores them in
 * @values.
 *
 * See [method@ProcedureConfig.set_values].
 *
 * Since: 3.0
 **/
void
_clay_procedure_config_get_values (ClayProcedureConfig  *config,
                                   ClayValueArray       *values)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         n_aux_args;
  gint         n_values;
  gint         i;

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  clay_procedure_get_aux_arguments (GET_PRIVATE (config)->procedure, &n_aux_args);
  n_values = clay_value_array_length (values);

  /* The config will have 1 additional property: "procedure". */
  g_return_if_fail (n_pspecs == n_values + n_aux_args + 1);

  for (i = 0; i < n_values; i++)
    {
      GParamSpec *pspec = pspecs[i + 1];
      GValue     *value = clay_value_array_index (values, i);

      g_object_get_property (G_OBJECT (config), pspec->name, value);
    }

  g_free (pspecs);
}

/**
 * _clay_procedure_config_begin_run:
 * @config:   a #ClayProcedureConfig
 * @image: (nullable): a #ClayImage or %NULL
 * @run_mode: the #ClayRunMode passed to a [class@Procedure]'s run()
 * @args:     the #ClayValueArray passed to a [class@Procedure]'s run()
 * @editable_properties: whether you may tweak the properties for
 *                       metadata export.
 *
 * Populates @config with values for a [class@Procedure]'s run(),
 * depending on @run_mode.
 *
 * If @run_mode is %CLAY_RUN_INTERACTIVE or %CLAY_RUN_WITH_LAST_VALS,
 * the saved values from the procedure's last run() are loaded and set
 * on @config. If @image is not %NULL, the last used values for this
 * image are tried first, and if no image-specific values are found
 * the globally saved last used values are used. If no saved last used
 * values are found, the procedure's default argument values are used.
 *
 * If @run_mode is %CLAY_RUN_NONINTERACTIVE, the contents of @args are
 * set on @config using [method@ProcedureConfig.set_values].
 *
 * After setting @config's properties like described above, arguments
 * and auxiliary arguments are automatically synced from image
 * parasites of the same name if they were set to
 * %CLAY_ARGUMENT_SYNC_PARASITE with [class@Procedure.set_argument_sync]:
 *
 * String properties are set to the value of the image parasite if
 * @run_mode is %CLAY_RUN_INTERACTIVE or %CLAY_RUN_WITH_LAST_VALS, or
 * if the corresponding argument is an auxiliary argument. As a
 * special case, a property named "clay-comment" will default to
 * [func@get_default_comment] if there is no "clay-comment" parasite.
 *
 * After calling this function, the @args passed to run() should be
 * left alone and @config be treated as the procedure's arguments, with
 * some exception, such as metadata export properties (which may be
 * overwritten for safety measure), and only if @editable_properties is
 * returned as %TRUE, which should only happen in interactive @run_mode
 * when this is the first run with @image in particular (i.e. there is
 * no config parasite on @image stored for this procedure). In
 * particular, in non-interactive, we will use exactly @args; and in
 * last-vals run, or if a config parasite was found on image, we will
 * use exactly the last-run arguments.
 *
 * It is possible to get @config's resulting values back into @args by
 * calling [method@ProcedureConfig.get_values], as long as modified
 * @args are written back to @config using [method@ProcedureConfig.set_values]
 * before the call to [method@ProcedureConfig.end_run].
 *
 * This function should be used at the beginning of a procedure's
 * run() and be paired with a call to [method@ProcedureConfig.end_run]
 * at the end of run().
 *
 * Since: 3.0
 **/
void
_clay_procedure_config_begin_run (ClayProcedureConfig  *config,
                                  ClayImage            *image,
                                  ClayRunMode           run_mode,
                                  const ClayValueArray *args,
                                  gboolean             *editable_properties)
{
  ClayProcedureConfigPrivate  *priv;
  GParamSpec                  *run_mode_pspec;
  GParamSpec                 **pspecs;
  guint                        n_pspecs;
  gint                         i;
  gboolean                     loaded = FALSE;
  GError                      *error  = NULL;

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));
  g_return_if_fail (args != NULL);

  priv = GET_PRIVATE (config);

  priv->image    = image;
  priv->run_mode = run_mode;

  if (editable_properties)
    *editable_properties = FALSE;

  clay_procedure_config_set_values (config, args);

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE :
    case CLAY_RUN_WITH_LAST_VALS:
      if (image)
        {
          loaded = clay_procedure_config_load_parasite (config, image,
                                                        &error);
          if (! loaded && error)
            {
              g_printerr ("Loading last used values from parasite failed: %s\n",
                          error->message);
              g_clear_error (&error);
            }
        }

      if (! loaded)
        {
          if (! clay_procedure_config_load_last (config, &error) && error)
            {
              g_printerr ("Loading last used values from disk failed: %s\n",
                          error->message);
              g_clear_error (&error);

              if (editable_properties)
                *editable_properties = TRUE;
            }

          if (run_mode == CLAY_RUN_INTERACTIVE && editable_properties)
            *editable_properties = TRUE;
        }
      break;

    case CLAY_RUN_NONINTERACTIVE:
      break;
    }

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      /*  skip our own properties  */
      if (pspec->owner_type == CLAY_TYPE_PROCEDURE_CONFIG)
        continue;

      switch (clay_procedure_get_argument_sync (priv->procedure, pspec->name))
        {
        case CLAY_ARGUMENT_SYNC_PARASITE:
          /*  we sync the property from the image parasite if it is an
           *  aux argument, or if we run interactively, because the
           *  parasite should be global to the image and not depend on
           *  whatever parasite another image had when last using this
           *  procedure
           */
          if (clay_procedure_find_aux_argument (priv->procedure,
                                                pspec->name) ||
              (run_mode != CLAY_RUN_NONINTERACTIVE))
            {
              clay_procedure_config_get_parasite (config, pspec);
            }
          break;

        default:
          break;
        }
    }

  /* Always set the run-mode in the end, which should be influenced neither by
   * the default property value, nor last run's value.
   */
  run_mode_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "run-mode");
  if (run_mode_pspec != NULL && G_IS_PARAM_SPEC_ENUM (run_mode_pspec))
    g_object_set (config, "run-mode", run_mode, NULL);

  g_free (pspecs);
}

/**
 * _clay_procedure_config_end_run:
 * @config: a #ClayProcedureConfig
 * @status: the return status of the [class@Procedure]'s run()
 *
 * This function is the counterpart of
 * [method@ProcedureConfig.begin_run] and must always be called in
 * pairs in a procedure's run(), before returning return values.
 *
 * If the @run_mode passed to [method@ProcedureConfig.end_run] was
 * %CLAY_RUN_INTERACTIVE, @config is saved as last used values to be
 * used when the procedure runs again. Additionally, if the #ClayImage
 * passed to [method@ProcedureConfig.begin_run] was not %NULL, @config is
 * attached to @image as last used values for this image using a
 * #ClayParasite and [method@Image.attach_parasite].
 *
 * If @run_mode was not %CLAY_RUN_NONINTERACTIVE, this function also
 * conveniently calls [func@displays_flush], which is what most
 * procedures want and doesn't do any harm if called redundantly.
 *
 * After a %CLAY_RUN_INTERACTIVE run, %CLAY_ARGUMENT_SYNC_PARASITE
 * values that have been changed are written back to their
 * corresponding image parasite.
 *
 * See [method@ProcedureConfig.begin_run].
 *
 * Since: 3.0
 **/
void
_clay_procedure_config_end_run (ClayProcedureConfig *config,
                                ClayPDBStatusType    status)
{
  ClayProcedureConfigPrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));

  priv = GET_PRIVATE (config);

  if (priv->run_mode != CLAY_RUN_NONINTERACTIVE)
    clay_displays_flush ();

  if (status == CLAY_PDB_SUCCESS &&
      priv->run_mode == CLAY_RUN_INTERACTIVE)
    {
      GParamSpec **pspecs;
      guint        n_pspecs;
      gint         i;
      GError      *error = NULL;

      if (priv->image)
        clay_procedure_config_save_parasite (config, priv->image,
                                             NULL);

      if (! clay_procedure_config_save_last (config, &error))
        {
          g_printerr ("Saving last used values to disk failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }

      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                               &n_pspecs);

      for (i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec = pspecs[i];

          /*  skip our own properties  */
          if (pspec->owner_type == CLAY_TYPE_PROCEDURE_CONFIG)
            continue;

          switch (clay_procedure_get_argument_sync (priv->procedure,
                                                    pspec->name))
            {
            case CLAY_ARGUMENT_SYNC_PARASITE:
              clay_procedure_config_set_parasite (config, pspec);
              break;

            default:
              break;
            }
        }

      g_free (pspecs);
    }

  priv->image    = NULL;
  priv->run_mode = -1;
}

/**
 * _clay_procedure_config_begin_export:
 * @config:         a #ClayProcedureConfig
 * @original_image: the image passed to run()
 * @run_mode:       the #ClayRunMode passed to a [class@Procedure]'s run()
 * @args:           the value array passed to a [class@Procedure]'s run()
 * @mime_type: (nullable):  exported file format's mime type, or %NULL.
 *
 * This is a variant of [method@ProcedureConfig.begin_run] to be used
 * by file export procedures using [class@ExportProcedure]. It must be
 * paired with a call to [method@ProcedureConfig.end_export] at the end
 * of run().
 *
 * It does everything [method@ProcedureConfig.begin_run] does but
 * provides additional features to automate file export:
 *
 * If @mime_type is non-%NULL, exporting metadata is handled
 * automatically, by calling [method@Image.metadata_save_prepare] and
 * syncing its returned [flags@MetadataSaveFlags] with @config's
 * properties. (The corresponding [method@Image.metadata_save_finish]
 * will be called by [method@ProcedureConfig.end_export]).
 *
 * The following boolean arguments of the used [class@ExportProcedure] are
 * synced. The procedure can but must not provide these arguments.
 *
 * - "include-exif" for %CLAY_METADATA_SAVE_EXIF.
 * - "include-xmp" for %CLAY_METADATA_SAVE_XMP.
 * - "include-iptc" for %CLAY_METADATA_SAVE_IPTC.
 * - "include-thumbnail" for %CLAY_METADATA_SAVE_THUMBNAIL.
 * - "include-color-profile" for %CLAY_METADATA_SAVE_COLOR_PROFILE.
 * - "include-comment" for %CLAY_METADATA_SAVE_COMMENT.
 *
 * The values from the [flags@MetadataSaveFlags] will only ever be used
 * to set these properties to %FALSE, overriding the user's saved
 * default values for the procedure, but NOT overriding the last used
 * values from exporting @original_image or the last used values from
 * exporting any other image using this procedure.
 *
 * If @mime_type is %NULL, [class@Metadata] handling is skipped. The
 * procedure can still have all of the above listed boolean arguments,
 * but must take care of their default values itself. The easiest way
 * to do this is by simply using [func@export_comment], [func@export_exif] etc.
 * as default values for these arguments when adding them using
 * clay_procedure_add_boolean_argument() or
 * clay_procedure_add_boolean_aux_argument().
 *
 * Returns: (transfer none) (nullable): The metadata to be used
 *          for this export, or %NULL if @original_image doesn't have
 *          metadata.
 *
 * Since: 3.0
 **/
ClayMetadata *
_clay_procedure_config_begin_export (ClayProcedureConfig  *config,
                                     ClayImage            *original_image,
                                     ClayRunMode           run_mode,
                                     const ClayValueArray *args,
                                     const gchar          *mime_type)
{
  ClayProcedureConfigPrivate *priv;
  GObjectClass               *object_class;
  gboolean                    editable_properties;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (original_image), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  priv = GET_PRIVATE (config);

  object_class = G_OBJECT_GET_CLASS (config);

  /* This must be run before we process the metadata, because we may
   * overwrite various metadata export properties when they are globally
   * disabled in Preferences, for security reasons.
   */
  _clay_procedure_config_begin_run (config, original_image, run_mode, args,
                                    &editable_properties);

  if (mime_type)
    {
      ClayMetadataSaveFlags metadata_flags;
      gint                  i;

      priv->metadata =
        clay_image_metadata_save_prepare (original_image,
                                          mime_type,
                                          &metadata_flags);

      if (priv->metadata)
        {
          priv->mime_type      = g_strdup (mime_type);
          priv->metadata_flags = metadata_flags;
        }

      if (editable_properties)
        for (i = 0; i < G_N_ELEMENTS (metadata_properties); i++)
          {
            /*  we only disable properties based on metadata flags here
             *  and never enable them, so we don't override the user's
             *  saved default values that are passed to us via "args"
             */
            if (! (metadata_flags & metadata_properties[i].flag))
              {
                const gchar *prop_name = metadata_properties[i].name;
                GParamSpec  *pspec;

                pspec = g_object_class_find_property (object_class, prop_name);
                if (pspec)
                  g_object_set (config,
                                prop_name, FALSE,
                                NULL);
              }
          }
    }

  return priv->metadata;
}

/**
 * _clay_procedure_config_end_export:
 * @config:         a #ClayProcedureConfig
 * @exported_image: the image that was actually exported
 * @file:           the #GFile @exported_image was written to
 * @status:         the return status of the [class@Procedure]'s run()
 *
 * This is a variant of [method@ProcedureConfig.end_run] to be used by
 * file export procedures using [class@ExportProcedure]. It must be paired
 * with a call to [method@ProcedureConfig.begin_export] at the
 * beginning of run().
 *
 * It does everything [method@ProcedureConfig.begin_run] does but
 * provides additional features to automate file export:
 *
 * If @status is %CLAY_PDB_SUCCESS, and
 * [method@ProcedureConfig.begin_export] returned a [class@Metadata], this
 * function calls [method@ProcedureConfig.save_metadata], which syncs
 * back @config's export properties to the metadata's
 * [flags@MetadataSaveFlags] and writes metadata to @file using
 * [method@Image.metadata_save_finish].
 *
 * Since: 3.0
 **/
void
_clay_procedure_config_end_export (ClayProcedureConfig *config,
                                   ClayImage           *exported_image,
                                   GFile               *file,
                                   ClayPDBStatusType    status)
{
  ClayProcedure              *procedure;
  ClayExportProcedure        *export_proc;
  ClayProcedureConfigPrivate *priv;

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (CLAY_IS_IMAGE (exported_image));
  g_return_if_fail (G_IS_FILE (file));

  priv = GET_PRIVATE (config);

  procedure   = clay_procedure_config_get_procedure (config);
  export_proc = CLAY_EXPORT_PROCEDURE (procedure);

  if (status == CLAY_PDB_SUCCESS &&
      clay_export_procedure_get_export_metadata (export_proc))
    {
      clay_procedure_config_save_metadata (config, exported_image, file);
    }

  g_clear_object (&priv->metadata);
  g_clear_pointer (&priv->mime_type, g_free);
  priv->metadata_flags = 0;
  priv->metadata_saved = FALSE;

  _clay_procedure_config_end_run (config, status);
}

gboolean
clay_procedure_config_has_default (ClayProcedureConfig *config)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);

  file = clay_procedure_config_get_file (config, ".default");

  success = g_file_query_exists (file, NULL);
  g_object_unref (file);

  return success;
}

gboolean
clay_procedure_config_load_default (ClayProcedureConfig  *config,
                                    GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_procedure_config_get_file (config, ".default");

  success = clay_config_deserialize_file (CLAY_CONFIG (config),
                                          file,
                                          NULL, error);

  if (! success && error && (*error)->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (error);
    }

  g_object_unref (file);

  return success;
}

gboolean
clay_procedure_config_save_default (ClayProcedureConfig  *config,
                                    GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_procedure_config_get_file (config, ".default");

  success = clay_config_serialize_to_file (CLAY_CONFIG (config),
                                           file,
                                           "settings",
                                           "end of settings",
                                           NULL, error);

  g_object_unref (file);

  return success;
}


/*  private functions  */

/**
 * clay_procedure_config_set_values:
 * @config: a #ClayProcedureConfig
 * @values: a #ClayValueArray
 *
 * Sets the values from @values on @config's properties.
 *
 * The number, order and types of values in @values must match the
 * number, order and types of @config's properties.
 *
 * This function is meant to be used on @values which are passed as
 * arguments to the run() function of the [class@Procedure] which created
 * this @config. See [method@Procedure.create_config].
 *
 * Since: 3.0
 **/
static void
clay_procedure_config_set_values (ClayProcedureConfig  *config,
                                  const ClayValueArray *values)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         n_aux_args;
  gint         n_values;
  gint         i;

  g_return_if_fail (CLAY_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  clay_procedure_get_aux_arguments (GET_PRIVATE (config)->procedure,
                                    &n_aux_args);
  n_values = clay_value_array_length (values);

  /* The first property is the procedure, all others are arguments. */
  g_return_if_fail (n_pspecs == n_values + n_aux_args + 1);

  for (i = 0; i < n_values; i++)
    {
      GParamSpec *pspec = pspecs[i + 1];
      GValue     *value = clay_value_array_index (values, i);

      g_object_set_property (G_OBJECT (config), pspec->name, value);
    }

  g_free (pspecs);
}

static gboolean
clay_procedure_config_load_last (ClayProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_procedure_config_get_file (config, ".last");

  success = clay_config_deserialize_file (CLAY_CONFIG (config),
                                          file,
                                          NULL, error);

  if (! success && (*error)->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (error);
    }

  g_object_unref (file);

  return success;
}

static gboolean
clay_procedure_config_save_last (ClayProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_procedure_config_get_file (config, ".last");

  success = clay_config_serialize_to_file (CLAY_CONFIG (config),
                                           file,
                                           "settings",
                                           "end of settings",
                                           NULL, error);

  g_object_unref (file);

  return success;
}

static gboolean
clay_procedure_config_load_parasite (ClayProcedureConfig  *config,
                                     ClayImage            *image,
                                     GError              **error)
{
  gchar        *name;
  ClayParasite *parasite;
  gboolean      success;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = clay_procedure_config_parasite_name (config, "-last");
  parasite = clay_image_get_parasite (image, name);
  g_free (name);

  if (! parasite)
    return FALSE;

  success = clay_config_deserialize_parasite (CLAY_CONFIG (config),
                                              parasite,
                                              NULL, error);
  clay_parasite_free (parasite);

  return success;
}

static gboolean
clay_procedure_config_save_parasite (ClayProcedureConfig  *config,
                                     ClayImage            *image,
                                     GError              **error)
{
  gchar        *name;
  ClayParasite *parasite;

  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = clay_procedure_config_parasite_name (config, "-last");
  parasite = clay_config_serialize_to_parasite (CLAY_CONFIG (config),
                                                name,
                                                CLAY_PARASITE_PERSISTENT,
                                                NULL);
  g_free (name);

  if (! parasite)
    return FALSE;

  clay_image_attach_parasite (image, parasite);
  clay_parasite_free (parasite);

  return TRUE;
}

static GFile *
clay_procedure_config_get_file (ClayProcedureConfig *config,
                                const gchar         *extension)
{
  GFile *file;
  gchar *basename;

  basename = g_strconcat (G_OBJECT_TYPE_NAME (config), extension, NULL);
  file = clay_directory_file ("plug-in-settings", basename, NULL);
  g_free (basename);

  return file;
}

static gchar *
clay_procedure_config_parasite_name (ClayProcedureConfig *config,
                                     const gchar         *suffix)
{
  return g_strconcat (G_OBJECT_TYPE_NAME (config), suffix, NULL);
}
