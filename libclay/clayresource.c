/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include "clay.h"

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "clayplugin-private.h"


/* ClayResource: base class for resources.
 *
 * Known subclasses are Font, Brush, Pattern, Palette, Gradient.
 * FUTURE: ? Dynamics, ToolPreset, ColorProfile, Color, ColorScale
 * (ClayParasite is NOT.)
 *
 * A *resource* is data that Clay loads at runtime startup,
 * where the data is used by drawing tools.
 * The ClayContext holds a user's current choice of resources.
 * The Clay core has the *resource* data.
 *
 * A resource has an identifier.
 * Currently the identifier is a string, sometimes called a name.
 * The identifier is unique among instances (resource data) loaded into Clay.
 *
 * A user can change the set of resources installed with Clay,
 * and edit or create new resources meaning datasets.
 * A user may attempt to install two different resources having the same name.
 * A user may uninstall a resource while there are still references to it in settings.
 *
 * FUTURE: the identifier is world unique, a UUID or an integer incremented by core.
 * Uniqueness is enforced by core.
 *
 * A ClayResource's identifier is serialized as a setting, i.e. a user's choice,
 * A serialization of a ClayResource is *not* the serialization of the
 * resource's underlying data e.g. not the pixels of a brush.
 *
 * The ClayResource identifier is opaque: you should only pass it around.
 * You should not assume it has any human readable meaning (although it does now.)
 * You should not assume that the "id" is a string.
 * The Resource class encapsulates the ID so in the future, the type may change.
 * PDB procedures that pass a string ID for a resource are obsolete.
 *
 * Usually a plugin lets a user choose a resource interactively
 * then sets it into a temporary context to affect subsequent operations.
 *
 * The Clay architecture for plugins uses remote procedures,
 * and identically named classes on each side of the wire.
 * A ClayBrush class in core is not the same class as ClayBrush in libclay.
 * a ClayResource on the libclay side is a proxy.
 * There is no ClayResource class in core.
 *
 * One use of ClayResource and its subclasses is as a held type of
 * GParamSpecObject, used to declare the parameters of a PDB
 * procedure.  A ClayResource just holds the id as a way to identify a
 * *resource* in calls to PDB procedures that ultimately access the
 * core instance of the resource.
 *
 * A ClayResource that has been serialized in a ClayConfig refers to a *resource*
 * that might not still exist in core in the set of loaded resources (files.)
 *
 * A ClayResource:
 *  - furnishes its ID as a property
 *  - serializes/deserializes itself (implements ClayConfigInterface)
 *
 * Some subclasses e.g. ClayFont are pure types.
 * That is, inheriting all its properties and methods from ClayResource.
 * Such a pure type exists just to distinguish (by the name of its type)
 * from other subclasses of ClayResource.
 *
 * Some subclasses have methods for getting/setting attributes of a resource.
 * Some subclasses have methods for creating, duplicating, and deleting resources.
 * Most methods are defined in the PDB (in .pdb files.)
 *
 * Internally, you may need to create a proxy object. Use:
 *    brush = g_object_new (CLAY_TYPE_BRUSH, NULL);
 *    g_object_set (CLAY_RESOURCE(brush), "id", "foo name", NULL);
 * This does NOT create the resource's data in core, only a reference to it.
 * When there is no underlying resource of that id (name) in core,
 * the brush is invalid.
 */

enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


typedef struct _ClayResourcePrivate
{
  gint id;
} ClayResourcePrivate;


static void     clay_resource_config_iface_init  (ClayConfigInterface *iface);

static void     clay_resource_set_property       (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void     clay_resource_get_property       (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

static gboolean clay_resource_serialize          (ClayConfig       *config,
                                                  ClayConfigWriter *writer,
                                                  gpointer          data);
static ClayConfig *
                clay_resource_deserialize_create (GType             type,
                                                  GScanner         *scanner,
                                                  gint              nest_level,
                                                  gpointer          data);


G_DEFINE_TYPE_EXTENDED (ClayResource, clay_resource, G_TYPE_OBJECT,
                        G_TYPE_FLAG_ABSTRACT,
                        G_ADD_PRIVATE (ClayResource)
                        G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                               clay_resource_config_iface_init))

#define parent_class clay_resource_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_resource_class_init (ClayResourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_resource_set_property;
  object_class->get_property = clay_resource_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The id",
                      "The id for internal use",
                      0, G_MAXINT32, 0,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_resource_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize          = clay_resource_serialize;
  iface->deserialize_create = clay_resource_deserialize_create;
}

static void
clay_resource_init (ClayResource *resource)
{
}

static void
clay_resource_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClayResource        *resource = CLAY_RESOURCE (object);
  ClayResourcePrivate *priv     = clay_resource_get_instance_private (resource);

  switch (property_id)
    {
    case PROP_ID:
      priv->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_resource_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClayResource        *resource = CLAY_RESOURCE (object);
  ClayResourcePrivate *priv     = clay_resource_get_instance_private (resource);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, priv->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_resource_serialize (ClayConfig       *config,
                         ClayConfigWriter *writer,
                         gpointer          data)
{
  ClayResource  *resource;
  gchar         *name;
  gchar         *collection;
  gboolean       is_internal;

  g_return_val_if_fail (CLAY_IS_RESOURCE (config), FALSE);

  resource    = CLAY_RESOURCE (config);
  is_internal = _clay_resource_get_identifiers (resource, &name, &collection);

  if (is_internal)
    clay_config_writer_identifier (writer, "internal");
  clay_config_writer_string (writer, name);
  clay_config_writer_string (writer, collection);

  g_free (name);
  g_free (collection);

  return TRUE;
}

static ClayConfig *
clay_resource_deserialize_create (GType     type,
                                  GScanner *scanner,
                                  gint      nest_level,
                                  gpointer  data)
{
  ClayResource *resource    = NULL;
  gchar        *name        = NULL;
  gchar        *collection  = NULL;
  gboolean      is_internal = FALSE;

  if (clay_scanner_parse_identifier (scanner, "internal"))
    is_internal = TRUE;

  if (clay_scanner_parse_string (scanner, &name) &&
      clay_scanner_parse_string (scanner, &collection))
    resource = _clay_resource_get_by_identifiers (g_type_name (type), name, collection, is_internal);

  if (resource == NULL)
    /* Default to context resource. */
    resource = _clay_context_get_resource (g_type_name (type));

  if (resource)
    g_object_ref (resource);

  g_free (collection);
  g_free (name);;

  return CLAY_CONFIG (resource);;
}


/* Public functions */

/**
 * clay_resources_loaded:
 * @resource_type: The #GType of the resource.
 *
 * Returns whether the resource with the given @resource_type were
 * loaded.
 *
 * In particular, it would return FALSE if Clay was started with
 * `--no-data` (or `--no-fonts` for fonts).
 *
 * Returns: Whether resources of @resource_type were loaded.
 *
 * Since: 3.2.4
 **/
gboolean
clay_resources_loaded (GType resource_type)
{
  g_return_val_if_fail (g_type_is_a (resource_type, CLAY_TYPE_RESOURCE), FALSE);

  return _clay_resources_loaded (g_type_name (resource_type));
}

/**
 * clay_resource_get_id:
 * @resource: The resource.
 *
 * Note: in most use cases, you should not need a resource's ID which is
 * mostly internal data and not reusable across sessions.
 *
 * Returns: the resource ID.
 *
 * Since: 3.0
 **/
gint32
clay_resource_get_id (ClayResource *resource)
{
  if (resource)
    {
      ClayResourcePrivate *priv = clay_resource_get_instance_private (resource);

      return priv->id;
    }
  else
    {
      return -1;
    }
}

/**
 * clay_resource_get_by_id:
 * @resource_id: The resource id.
 *
 * Returns a #ClayResource representing @resource_id. Since #ClayResource is an
 * abstract class, the real object type will actually be the proper
 * subclass.
 *
 * Note: in most use cases, you should not need to retrieve a
 * #ClayResource by its ID, which is mostly internal data and not
 * reusable across sessions. Use the appropriate functions for your use
 * case instead.
 *
 * Returns: (nullable) (transfer none): a #ClayResource for @resource_id or
 *          %NULL if @resource_id does not represent a valid resource.
 *          The object belongs to libclay and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
ClayResource *
clay_resource_get_by_id (gint32 resource_id)
{
  if (resource_id > 0)
    {
      ClayPlugIn *plug_in = clay_get_plug_in ();

      return _clay_plug_in_get_resource (plug_in, resource_id);
    }

  return NULL;
}

/**
 * clay_resource_get_by_name:
 * @resource_type: The #GType of the resource.
 * @resource_name: The name of the resource.
 *
 * Returns the resource with the given @resource_type and
 * @resource_name.
 *
 * Returns: (nullable) (transfer none): The resource.
 *
 * Since: 3.0
 **/
ClayResource *
clay_resource_get_by_name (GType        resource_type,
                           const gchar *resource_name)
{
  g_return_val_if_fail (g_type_is_a (resource_type, CLAY_TYPE_RESOURCE), NULL);

  if (resource_name == NULL)
    return NULL;

  return _clay_resource_get_by_name (g_type_name (resource_type), resource_name);
}

/**
 * clay_resource_is_valid:
 * @resource: The resource to check.
 *
 * Returns TRUE if the resource is valid.
 *
 * This procedure checks if the given resource is valid and refers to an
 * existing resource.
 *
 * Returns: Whether the resource is valid.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_valid (ClayResource *resource)
{
  return clay_resource_id_is_valid (clay_resource_get_id (resource));
}

/**
 * clay_resource_is_brush:
 * @resource: The resource.
 *
 * Returns whether the resource is a brush.
 *
 * This procedure returns TRUE if the specified resource is a brush.
 *
 * Returns: TRUE if the resource is a brush, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_brush (ClayResource *resource)
{
  return clay_resource_id_is_brush (clay_resource_get_id (resource));
}

/**
 * clay_resource_is_pattern:
 * @resource: The resource.
 *
 * Returns whether the resource is a pattern.
 *
 * This procedure returns TRUE if the specified resource is a pattern.
 *
 * Returns: TRUE if the resource is a pattern, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_pattern (ClayResource *resource)
{
  return clay_resource_id_is_pattern (clay_resource_get_id (resource));
}

/**
 * clay_resource_is_gradient:
 * @resource: The resource.
 *
 * Returns whether the resource is a gradient.
 *
 * This procedure returns TRUE if the specified resource is a gradient.
 *
 * Returns: TRUE if the resource is a gradient, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_gradient (ClayResource *resource)
{
  return clay_resource_id_is_gradient (clay_resource_get_id (resource));
}

/**
 * clay_resource_is_palette:
 * @resource: The resource.
 *
 * Returns whether the resource is a palette.
 *
 * This procedure returns TRUE if the specified resource is a palette.
 *
 * Returns: TRUE if the resource is a palette, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_palette (ClayResource *resource)
{
  return clay_resource_id_is_palette (clay_resource_get_id (resource));
}

/**
 * clay_resource_is_font:
 * @resource: The resource.
 *
 * Returns whether the resource is a font.
 *
 * This procedure returns TRUE if the specified resource is a font.
 *
 * Returns: TRUE if the resource is a font, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_resource_is_font (ClayResource *resource)
{
  return clay_resource_id_is_font (clay_resource_get_id (resource));
}
