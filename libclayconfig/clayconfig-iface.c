/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "clayconfigtypes.h"

#include "clayconfigwriter.h"
#include "clayconfig-iface.h"
#include "clayconfig-deserialize.h"
#include "clayconfig-serialize.h"
#include "clayconfig-params.h"
#include "clayconfig-utils.h"
#include "clayscanner.h"

#include "libclay/libclay-intl.h"


typedef struct _ClayConfigInterfacePrivate ClayConfigInterfacePrivate;

struct _ClayConfigInterfacePrivate
{
  guint xcf_version;
};

#define CLAY_CONFIG_INTERFACE_GET_PRIVATE(obj) (clay_config_iface_get_private ((ClayConfigInterface *) (obj)))


/*
 * ClayConfigIface:
 *
 * The [struct@Config] serialization and deserialization interface.
 */


/*  local function prototypes  */

static ClayConfigInterfacePrivate * clay_config_iface_get_private      (ClayConfigInterface        *iface);
static void                         clay_config_iface_private_finalize (ClayConfigInterfacePrivate *private);

static void                         clay_config_iface_default_init     (ClayConfigInterface        *iface);
static void                         clay_config_iface_base_init        (ClayConfigInterface        *iface);

static gboolean                     clay_config_iface_serialize        (ClayConfig                 *config,
                                                                        ClayConfigWriter           *writer,
                                                                        gpointer                    data);
static gboolean                     clay_config_iface_deserialize      (ClayConfig                 *config,
                                                                        GScanner                   *scanner,
                                                                        gint                        nest_level,
                                                                        gpointer                    data);
static ClayConfig                 * clay_config_iface_duplicate        (ClayConfig                 *config);
static gboolean                     clay_config_iface_equal            (ClayConfig                 *a,
                                                                        ClayConfig                 *b);
static void                         clay_config_iface_reset            (ClayConfig                 *config);
static gboolean                     clay_config_iface_copy             (ClayConfig                 *src,
                                                                        ClayConfig                 *dest,
                                                                        GParamFlags                 flags);


/*  private functions  */


GType
clay_config_get_type (void)
{
  static GType config_iface_type = 0;

  if (! config_iface_type)
    {
      const GTypeInfo config_iface_info =
      {
        sizeof (ClayConfigInterface),
        (GBaseInitFunc)      clay_config_iface_base_init,
        (GBaseFinalizeFunc)  NULL,
        (GClassInitFunc)     clay_config_iface_default_init,
        (GClassFinalizeFunc) NULL,
      };

      config_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                  "ClayConfigInterface",
                                                  &config_iface_info,
                                                  0);

      g_type_interface_add_prerequisite (config_iface_type, G_TYPE_OBJECT);
    }

  return config_iface_type;
}

static void
clay_config_iface_default_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_config_iface_serialize;
  iface->deserialize = clay_config_iface_deserialize;
  iface->duplicate   = clay_config_iface_duplicate;
  iface->equal       = clay_config_iface_equal;
  iface->reset       = clay_config_iface_reset;
  iface->copy        = clay_config_iface_copy;
}

static void
clay_config_iface_base_init (ClayConfigInterface *iface)
{
  /*  always set these to NULL since we don't want to inherit them
   *  from parent classes
   */
  iface->serialize_property   = NULL;
  iface->deserialize_property = NULL;
}

static gboolean
clay_config_iface_serialize (ClayConfig       *config,
                             ClayConfigWriter *writer,
                             gpointer          data)
{
  return clay_config_serialize_properties (config, writer);
}

static gboolean
clay_config_iface_deserialize (ClayConfig *config,
                               GScanner   *scanner,
                               gint        nest_level,
                               gpointer    data)
{
  return clay_config_deserialize_properties (config, scanner, nest_level);
}

static ClayConfig *
clay_config_iface_duplicate (ClayConfig *config)
{
  GObject       *object = G_OBJECT (config);
  GObjectClass  *klass  = G_OBJECT_GET_CLASS (object);
  GParamSpec   **property_specs;
  guint          n_property_specs;
  gint           n_construct_properties = 0;
  const gchar  **construct_names        = NULL;
  GValue        *construct_values       = NULL;
  guint          i;
  GObject       *dup;

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  construct_names  = g_new0 (const gchar *, n_property_specs);
  construct_values = g_new0 (GValue,        n_property_specs);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if ((prop_spec->flags & G_PARAM_READABLE) &&
          (prop_spec->flags & G_PARAM_WRITABLE) &&
          (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
        {
          construct_names[n_construct_properties] = prop_spec->name;

          g_value_init (&construct_values[n_construct_properties],
                        prop_spec->value_type);
          g_object_get_property (object, prop_spec->name,
                                 &construct_values[n_construct_properties]);

          n_construct_properties++;
        }
    }

  g_free (property_specs);

  dup = g_object_new_with_properties (G_TYPE_FROM_INSTANCE (object),
                                      n_construct_properties,
                                      (const gchar **) construct_names,
                                      (const GValue *) construct_values);

  for (i = 0; i < n_construct_properties; i++)
    g_value_unset (&construct_values[i]);

  g_free (construct_names);
  g_free (construct_values);

  clay_config_copy (config, CLAY_CONFIG (dup), 0);

  return CLAY_CONFIG (dup);
}

static gboolean
clay_config_iface_equal (ClayConfig *a,
                         ClayConfig *b)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  gboolean       equal = TRUE;

  klass = G_OBJECT_GET_CLASS (a);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);

  for (i = 0; equal && i < n_property_specs; i++)
    {
      GParamSpec  *prop_spec;
      GValue       a_value = G_VALUE_INIT;
      GValue       b_value = G_VALUE_INIT;

      prop_spec = property_specs[i];

      if (! (prop_spec->flags & G_PARAM_READABLE) ||
            (prop_spec->flags & CLAY_CONFIG_PARAM_DONT_COMPARE))
        {
          continue;
        }

      g_value_init (&a_value, prop_spec->value_type);
      g_value_init (&b_value, prop_spec->value_type);
      g_object_get_property (G_OBJECT (a), prop_spec->name, &a_value);
      g_object_get_property (G_OBJECT (b), prop_spec->name, &b_value);

      if (g_param_values_cmp (prop_spec, &a_value, &b_value))
        {
          if ((prop_spec->flags & CLAY_CONFIG_PARAM_AGGREGATE) &&
              G_IS_PARAM_SPEC_OBJECT (prop_spec)        &&
              g_type_interface_peek (g_type_class_peek (prop_spec->value_type),
                                     CLAY_TYPE_CONFIG))
            {
              if (! clay_config_is_equal_to (g_value_get_object (&a_value),
                                             g_value_get_object (&b_value)))
                {
                  equal = FALSE;
                }
            }
          else
            {
              equal = FALSE;
            }
        }

      g_value_unset (&a_value);
      g_value_unset (&b_value);
    }

  g_free (property_specs);

  return equal;
}

static void
clay_config_iface_reset (ClayConfig *config)
{
  clay_config_reset_properties (G_OBJECT (config));
}

static gboolean
clay_config_iface_copy (ClayConfig  *src,
                        ClayConfig  *dest,
                        GParamFlags  flags)
{
  return clay_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);
}

static ClayConfigInterfacePrivate *
clay_config_iface_get_private (ClayConfigInterface *iface)
{
  ClayConfigInterfacePrivate *private;

  static GQuark private_key = 0;

  if (! private_key)
    private_key = g_quark_from_static_string ("clay-config-iface-private");

  private = g_object_get_qdata ((GObject *) iface, private_key);

  if (! private)
    {
      private = g_slice_new0 (ClayConfigInterfacePrivate);

      private->xcf_version = G_MAXUINT32;

      g_object_set_qdata_full ((GObject *) iface, private_key, private,
                               (GDestroyNotify) clay_config_iface_private_finalize);
    }

  return private;
}

static void
clay_config_iface_private_finalize (ClayConfigInterfacePrivate *private)
{
  g_slice_free (ClayConfigInterfacePrivate, private);
}


/*  public functions  */


/**
 * clay_config_serialize_to_file:
 * @config: an object that implements [iface@ConfigInterface].
 * @file:   the file to write the configuration to.
 * @header: (nullable): optional file header (must be ASCII only)
 * @footer: (nullable): optional file footer (must be ASCII only)
 * @data: user data passed to the serialize implementation.
 * @error: return location for a possible error
 *
 * Serializes the object properties of @config to the file specified
 * by @file. If a file with that name already exists, it is
 * overwritten. Basically this function opens @file for you and calls
 * the serialize function of the @config's [iface@ConfigInterface].
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
clay_config_serialize_to_file (ClayConfig   *config,
                               GFile        *file,
                               const gchar  *header,
                               const gchar  *footer,
                               gpointer      data,
                               GError      **error)
{
  ClayConfigWriter *writer;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = clay_config_writer_new_from_file (file, TRUE, header, error);
  if (!writer)
    return FALSE;

  CLAY_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return clay_config_writer_finish (writer, footer, error);
}

/**
 * clay_config_serialize_to_stream:
 * @config: an object that implements [iface@ConfigInterface].
 * @output: the #GOutputStream to write the configuration to.
 * @header: (nullable): optional file header (must be ASCII only)
 * @footer: (nullable): optional file footer (must be ASCII only)
 * @data: user data passed to the serialize implementation.
 * @error: return location for a possible error
 *
 * Serializes the object properties of @config to the stream specified
 * by @output.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
clay_config_serialize_to_stream (ClayConfig     *config,
                                 GOutputStream  *output,
                                 const gchar    *header,
                                 const gchar    *footer,
                                 gpointer        data,
                                 GError        **error)
{
  ClayConfigWriter *writer;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = clay_config_writer_new_from_stream (output, header, error);
  if (!writer)
    return FALSE;

  CLAY_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return clay_config_writer_finish (writer, footer, error);
}

/**
 * clay_config_serialize_to_fd:
 * @config: an object that implements [iface@ConfigInterface].
 * @fd: a file descriptor, opened for writing
 * @data: user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to the given file
 * descriptor.
 *
 * Returns: %TRUE if serialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
clay_config_serialize_to_fd (ClayConfig *config,
                             gint        fd,
                             gpointer    data)
{
  ClayConfigWriter *writer;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (fd > 0, FALSE);

  writer = clay_config_writer_new_from_fd (fd);
  if (!writer)
    return FALSE;

  CLAY_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  return clay_config_writer_finish (writer, NULL, NULL);
}

/**
 * clay_config_serialize_to_string:
 * @config: an object that implements the [iface@ConfigInterface].
 * @data: user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to a string.
 *
 * Returns: a newly allocated NUL-terminated string.
 *
 * Since: 2.4
 **/
gchar *
clay_config_serialize_to_string (ClayConfig *config,
                                 gpointer    data)
{
  ClayConfigWriter *writer;
  GString          *str;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), NULL);

  str    = g_string_new (NULL);
  writer = clay_config_writer_new_from_string (str);

  CLAY_CONFIG_GET_IFACE (config)->serialize (config, writer, data);

  clay_config_writer_finish (writer, NULL, NULL);

  return g_string_free (str, FALSE);
}

/**
 * clay_config_serialize_to_parasite:
 * @config:         an object that implements the [iface@ConfigInterface].
 * @parasite_name:  the new parasite's name
 * @parasite_flags: the new parasite's flags
 * @data:           user data passed to the serialize implementation.
 *
 * Serializes the object properties of @config to a [struct@Parasite].
 *
 * Returns: (transfer full): the newly allocated parasite.
 *
 * Since: 3.0
 **/
ClayParasite *
clay_config_serialize_to_parasite (ClayConfig  *config,
                                   const gchar *parasite_name,
                                   guint        parasite_flags,
                                   gpointer     data)
{
  ClayParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), NULL);
  g_return_val_if_fail (parasite_name != NULL, NULL);

  str = clay_config_serialize_to_string (config, data);

  if (! str)
    return NULL;

  parasite = clay_parasite_new (parasite_name,
                                parasite_flags,
                                0, NULL);

  parasite->size = strlen (str) + 1;
  parasite->data = str;

  return parasite;
}

/**
 * clay_config_deserialize_file:
 * @config: an object that implements the #ClayConfigInterface.
 * @file: the file to read configuration from.
 * @data: user data passed to the deserialize implementation.
 * @error: return location for a possible error
 *
 * Opens the file specified by @file, reads configuration data from it
 * and configures @config accordingly. Basically this function creates
 * a properly configured [struct@GLib.Scanner] for you and calls the deserialize
 * function of the @config's [iface@ConfigInterface].
 *
 * Returns: Whether deserialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
clay_config_deserialize_file (ClayConfig  *config,
                              GFile       *file,
                              gpointer     data,
                              GError     **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = clay_scanner_new_file (file, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = CLAY_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  clay_scanner_unref (scanner);

  if (! success)
    /* If we get this assert, it means we have a bug in one of the
     * deserialize() implementations. Any failure case should report the
     * error condition with g_scanner_error() which will populate the
     * error object passed in clay_scanner_new*().
     */
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * clay_config_deserialize_stream:
 * @config: an object that implements the #ClayConfigInterface.
 * @input: the input stream to read configuration from.
 * @data: user data passed to the deserialize implementation.
 * @error: return location for a possible error
 *
 * Reads configuration data from @input and configures @config
 * accordingly. Basically this function creates a properly configured
 * #GScanner for you and calls the deserialize function of the
 * @config's #ClayConfigInterface.
 *
 * Returns: Whether deserialization succeeded.
 *
 * Since: 2.10
 **/
gboolean
clay_config_deserialize_stream (ClayConfig    *config,
                                GInputStream  *input,
                                gpointer       data,
                                GError       **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = clay_scanner_new_stream (input, error);
  if (! scanner)
    return FALSE;

  g_object_freeze_notify (G_OBJECT (config));

  success = CLAY_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  clay_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * clay_config_deserialize_string:
 * @config:   a #GObject that implements the #ClayConfigInterface.
 * @text: (array length=text_len): string to deserialize (in UTF-8 encoding)
 * @text_len: length of @text in bytes or -1
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @text. Basically this function creates a
 * properly configured #GScanner for you and calls the deserialize
 * function of the @config's #ClayConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
clay_config_deserialize_string (ClayConfig   *config,
                                const gchar  *text,
                                gint          text_len,
                                gpointer      data,
                                GError      **error)
{
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (text != NULL || text_len == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = clay_scanner_new_string (text, text_len, error);

  g_object_freeze_notify (G_OBJECT (config));

  success = CLAY_CONFIG_GET_IFACE (config)->deserialize (config,
                                                         scanner, 0, data);

  g_object_thaw_notify (G_OBJECT (config));

  clay_scanner_unref (scanner);

  if (! success)
    g_assert (error == NULL || *error != NULL);

  return success;
}

/**
 * clay_config_deserialize_parasite:
 * @config:   a #GObject that implements the #ClayConfigInterface.
 * @parasite: parasite containing a serialized config string
 * @data:     client data
 * @error:    return location for a possible error
 *
 * Configures @config from @parasite. Basically this function creates
 * a properly configured #GScanner for you and calls the deserialize
 * function of the @config's #ClayConfigInterface.
 *
 * Returns: %TRUE if deserialization succeeded, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_config_deserialize_parasite (ClayConfig          *config,
                                  const ClayParasite  *parasite,
                                  gpointer             data,
                                  GError             **error)
{
  const gchar *parasite_data;
  guint32      parasite_size;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  parasite_data = clay_parasite_get_data (parasite, &parasite_size);
  if (! parasite_data)
    return TRUE;

  return clay_config_deserialize_string (config, parasite_data, parasite_size,
                                         data, error);
}

/**
 * clay_config_deserialize_return:
 * @scanner:        a #GScanner
 * @expected_token: the expected token
 * @nest_level:     the nest level
 *
 * Returns:
 *
 * Since: 2.4
 **/
gboolean
clay_config_deserialize_return (GScanner     *scanner,
                                GTokenType    expected_token,
                                gint          nest_level)
{
  GTokenType next_token;

  g_return_val_if_fail (scanner != NULL, FALSE);

  next_token = g_scanner_peek_next_token (scanner);

  if (expected_token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
      return FALSE;
    }
  else
    {
      if (nest_level > 0 && next_token == G_TOKEN_RIGHT_PAREN)
        {
          return TRUE;
        }
      else if (next_token != G_TOKEN_EOF)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
          return FALSE;
        }
    }

  return TRUE;
}


/**
 * clay_config_serialize:
 * @config: an object that implements the #ClayConfigInterface.
 * @writer: the #ClayConfigWriter to use.
 * @data: client data
 *
 * Serialize the #ClayConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
clay_config_serialize (ClayConfig       *config,
                       ClayConfigWriter *writer,
                       gpointer          data)
{
  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);

  return CLAY_CONFIG_GET_IFACE (config)->serialize (config, writer, data);
}

/**
 * clay_config_deserialize:
 * @config: a #GObject that implements the #ClayConfigInterface.
 * @scanner: the #GScanner to use.
 * @nest_level: the nest level.
 * @data: client data.
 *
 * Deserialize the #ClayConfig object.
 *
 * Returns: Whether serialization succeeded.
 *
 * Since: 2.8
 **/
gboolean
clay_config_deserialize (ClayConfig *config,
                         GScanner   *scanner,
                         gint        nest_level,
                         gpointer    data)
{
  g_return_val_if_fail (CLAY_IS_CONFIG (config), FALSE);

  return CLAY_CONFIG_GET_IFACE (config)->deserialize (config,
                                                      scanner,
                                                      nest_level,
                                                      data);
}

/**
 * clay_config_duplicate:
 * @config: a #GObject that implements the #ClayConfigInterface.
 *
 * Creates a copy of the passed object by copying all object
 * properties. The default implementation of the #ClayConfigInterface
 * only works for objects that are completely defined by their
 * properties.
 *
 * Returns: the duplicated #ClayConfig object
 *
 * Since: 2.4
 **/
gpointer
clay_config_duplicate (ClayConfig *config)
{
  g_return_val_if_fail (CLAY_IS_CONFIG (config), NULL);

  return CLAY_CONFIG_GET_IFACE (config)->duplicate (config);
}

/**
 * clay_config_is_equal_to:
 * @a: a #GObject that implements the #ClayConfigInterface.
 * @b: another #GObject of the same type as @a.
 *
 * Compares the two objects. The default implementation of the
 * #ClayConfigInterface compares the object properties and thus only
 * works for objects that are completely defined by their
 * properties.
 *
 * Returns: %TRUE if the two objects are equal.
 *
 * Since: 2.4
 **/
gboolean
clay_config_is_equal_to (ClayConfig *a,
                         ClayConfig *b)
{
  g_return_val_if_fail (CLAY_IS_CONFIG (a), FALSE);
  g_return_val_if_fail (CLAY_IS_CONFIG (b), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (a) == G_TYPE_FROM_INSTANCE (b),
                        FALSE);

  return CLAY_CONFIG_GET_IFACE (a)->equal (a, b);
}

/**
 * clay_config_reset:
 * @config: a #GObject that implements the #ClayConfigInterface.
 *
 * Resets the object to its default state. The default implementation of the
 * #ClayConfigInterface only works for objects that are completely defined by
 * their properties.
 *
 * Since: 2.4
 **/
void
clay_config_reset (ClayConfig *config)
{
  g_return_if_fail (CLAY_IS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  CLAY_CONFIG_GET_IFACE (config)->reset (config);

  g_object_thaw_notify (G_OBJECT (config));
}

/**
 * clay_config_copy:
 * @src: a #GObject that implements the #ClayConfigInterface.
 * @dest: another #GObject of the same type as @a.
 * @flags: a mask of GParamFlags
 *
 * Compares all read- and write-able properties from @src and @dest
 * that have all @flags set. Differing values are then copied from
 * @src to @dest. If @flags is 0, all differing read/write properties.
 *
 * Properties marked as "construct-only" are not touched.
 *
 * Returns: %TRUE if @dest was modified, %FALSE otherwise
 *
 * Since: 2.6
 **/
gboolean
clay_config_copy (ClayConfig  *src,
                  ClayConfig  *dest,
                  GParamFlags  flags)
{
  gboolean changed;

  g_return_val_if_fail (CLAY_IS_CONFIG (src), FALSE);
  g_return_val_if_fail (CLAY_IS_CONFIG (dest), FALSE);
  g_return_val_if_fail (G_TYPE_FROM_INSTANCE (src) == G_TYPE_FROM_INSTANCE (dest),
                        FALSE);

  g_object_freeze_notify (G_OBJECT (dest));

  changed = CLAY_CONFIG_GET_IFACE (src)->copy (src, dest, flags);

  g_object_thaw_notify (G_OBJECT (dest));

  return changed;
}

/**
 * clay_config_get_xcf_version:
 * @config: a #GObject that implements the #ClayConfigInterface.
 *
 * Returns the current XCF version of the @config.
 *
 * Returns: the XCF version associated with the @config.
 *
 * Since: 3.0.8
 **/
guint
clay_config_get_xcf_version (ClayConfig *config)
{
  ClayConfigInterfacePrivate *private;

  g_return_val_if_fail (CLAY_IS_CONFIG (config), G_MAXUINT32);

  private = CLAY_CONFIG_INTERFACE_GET_PRIVATE (config);

  return private->xcf_version;
}

/**
 * clay_config_set_xcf_version:
 * @config: a #GObject that implements the #ClayConfigInterface.
 * @xcf_version: a mask of GParamFlags
 *
 * Sets the current XCF version of the @config. This information can be used
 * to adjust how properties are serialized depending on the version of the XCF
 * that it is being saved to.
 *
 * Since: 3.0.8
 **/
void
clay_config_set_xcf_version (ClayConfig *config,
                             guint       xcf_version)
{
  ClayConfigInterfacePrivate *private;

  g_return_if_fail (CLAY_IS_CONFIG (config));

  private = CLAY_CONFIG_INTERFACE_GET_PRIVATE (config);

  private->xcf_version = xcf_version;
}
