/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayparamspecs.c
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

#include <gio/gio.h>

#include "claybase.h"


/*
 * CLAY_TYPE_PARAM_OBJECT
 */

static void         clay_param_object_class_init            (ClayParamSpecObjectClass *klass);
static void         clay_param_object_init                  (ClayParamSpecObject      *pspec);
static void         clay_param_object_finalize              (GParamSpec               *pspec);
static void         clay_param_object_value_set_default     (GParamSpec               *pspec,
                                                             GValue                   *value);

static GParamSpec * clay_param_spec_object_real_duplicate   (GParamSpec               *pspec);
static GObject    * clay_param_spec_object_real_get_default (GParamSpec               *pspec);


GType
clay_param_object_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_object_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecObject),
        0,
        (GInstanceInitFunc) clay_param_object_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamObject", &info, G_TYPE_FLAG_ABSTRACT);
    }

  return type;
}

static void
clay_param_object_class_init (ClayParamSpecObjectClass *klass)
{
  GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (klass);

  klass->duplicate          = clay_param_spec_object_real_duplicate;
  klass->get_default        = clay_param_spec_object_real_get_default;

  pclass->value_type        = G_TYPE_OBJECT;
  pclass->finalize          = clay_param_object_finalize;
  pclass->value_set_default = clay_param_object_value_set_default;
}

static void
clay_param_object_init (ClayParamSpecObject *ospec)
{
  ospec->_default_value = NULL;
}

static void
clay_param_object_finalize (GParamSpec *pspec)
{
  ClayParamSpecObject *ospec        = CLAY_PARAM_SPEC_OBJECT (pspec);
  GParamSpecClass     *parent_class = g_type_class_peek (g_type_parent (CLAY_TYPE_PARAM_OBJECT));

  g_clear_object (&ospec->_default_value);

  parent_class->finalize (pspec);
}

static void
clay_param_object_value_set_default (GParamSpec *pspec,
                                     GValue     *value)
{
  GObject *default_value;

  g_return_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec));

  default_value = clay_param_spec_object_get_default (pspec);
  g_value_set_object (value, default_value);
}

static GParamSpec *
clay_param_spec_object_real_duplicate (GParamSpec *pspec)
{
  ClayParamSpecObject *ospec;
  ClayParamSpecObject *duplicate;

  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  ospec = CLAY_PARAM_SPEC_OBJECT (pspec);
  duplicate = g_param_spec_internal (G_TYPE_FROM_INSTANCE (pspec),
                                     pspec->name,
                                     g_param_spec_get_nick (pspec),
                                     g_param_spec_get_blurb (pspec),
                                     pspec->flags);

  duplicate->_default_value = ospec->_default_value ? g_object_ref (ospec->_default_value) : NULL;
  duplicate->_has_default   = ospec->_has_default;

  return G_PARAM_SPEC (duplicate);
}

static GObject *
clay_param_spec_object_real_get_default (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec), NULL);
  g_return_val_if_fail (CLAY_PARAM_SPEC_OBJECT (pspec)->_has_default, NULL);

  return CLAY_PARAM_SPEC_OBJECT (pspec)->_default_value;
}

/**
 * clay_param_spec_object_get_default:
 * @pspec: a #GObject #GParamSpec
 *
 * Get the default object value of the param spec.
 *
 * If the @pspec has been registered with a specific default (which can
 * be verified with [func@Clay.ParamSpecObject.has_default]), it will be
 * returned, though some specific subtypes may support returning dynamic
 * default (e.g. based on context).
 *
 * Returns: (transfer none): the default value.
 */
GObject *
clay_param_spec_object_get_default (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  return CLAY_PARAM_SPEC_OBJECT_GET_CLASS (pspec)->get_default (pspec);
}

/**
 * clay_param_spec_object_set_default:
 * @pspec: a #GObject #GParamSpec
 * @default_value: (transfer none) (nullable): a default value.
 *
 * Set the default object value of the param spec. This will switch the
 * `has_default` flag so that [func@Clay.ParamSpecObject.has_default]
 * will now return %TRUE.
 *
 * A %NULL @default_value still counts as a default (unless the specific
 * @pspec does not allow %NULL as a default).
 */
void
clay_param_spec_object_set_default (GParamSpec *pspec,
                                    GObject    *default_value)
{
  g_return_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec));

  CLAY_PARAM_SPEC_OBJECT (pspec)->_has_default = TRUE;
  g_set_object (&CLAY_PARAM_SPEC_OBJECT (pspec)->_default_value, default_value);
}

/**
 * clay_param_spec_object_has_default:
 * @pspec: a #GObject #GParamSpec
 *
 * This function tells whether a default was set, typically with
 * [func@Clay.ParamSpecObject.set_default] or any other way. It
 * does not guarantee that the default is an actual object (it may be
 * %NULL if valid as a default).
 *
 * Returns: whether a default value was set.
 */
gboolean
clay_param_spec_object_has_default (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec), FALSE);

  return CLAY_PARAM_SPEC_OBJECT (pspec)->_has_default;
}

/**
 * clay_param_spec_object_duplicate:
 * @pspec: a [struct@Clay.ParamSpecObject].
 *
 * This function duplicates @pspec appropriately, depending on the
 * accurate spec type.
 *
 * Returns: (transfer floating): a newly created param spec.
 */
GParamSpec *
clay_param_spec_object_duplicate (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_OBJECT (pspec), NULL);

  return CLAY_PARAM_SPEC_OBJECT_GET_CLASS (pspec)->duplicate (pspec);
}


/*
 * CLAY_TYPE_PARAM_FILE
 */

#define CLAY_PARAM_SPEC_FILE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_FILE, ClayParamSpecFile))

typedef struct _ClayParamSpecFile ClayParamSpecFile;

struct _ClayParamSpecFile
{
  ClayParamSpecObject   parent_instance;

  /*< private >*/
  ClayFileChooserAction action;
  gboolean              none_ok;
};

static void         clay_param_file_class_init     (ClayParamSpecObjectClass *klass);
static void         clay_param_file_init           (ClayParamSpecFile        *fspec);

static gboolean     clay_param_spec_file_validate  (GParamSpec               *pspec,
                                                    GValue                   *value);
static gint         clay_param_spec_file_cmp       (GParamSpec               *pspec,
                                                    const GValue             *value1,
                                                    const GValue             *value2);
static GParamSpec * clay_param_spec_file_duplicate (GParamSpec               *pspec);


/**
 * clay_param_file_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a file param object.
 *
 * Since: 3.0
 **/
GType
clay_param_file_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_file_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecFile),
        0,
        (GInstanceInitFunc) clay_param_file_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_OBJECT,
                                     "ClayParamFile", &info, 0);
    }

  return type;
}

static void
clay_param_file_class_init (ClayParamSpecObjectClass *klass)
{
  GParamSpecClass          *pclass = G_PARAM_SPEC_CLASS (klass);
  ClayParamSpecObjectClass *oclass = CLAY_PARAM_SPEC_OBJECT_CLASS (klass);

  pclass->value_type     = G_TYPE_FILE;
  pclass->value_validate = clay_param_spec_file_validate;
  pclass->values_cmp     = clay_param_spec_file_cmp;
  oclass->duplicate      = clay_param_spec_file_duplicate;
}

static void
clay_param_file_init (ClayParamSpecFile *fspec)
{
  fspec->none_ok = TRUE;
  fspec->action  = CLAY_FILE_CHOOSER_ACTION_OPEN;
}

static gboolean
clay_param_spec_file_validate (GParamSpec *pspec,
                               GValue     *value)
{
  ClayParamSpecFile   *fspec     = CLAY_PARAM_SPEC_FILE (pspec);
  ClayParamSpecObject *ospec     = CLAY_PARAM_SPEC_OBJECT (pspec);
  GFile               *file      = value->data[0].v_pointer;
  gboolean             modifying = FALSE;

  if (file == NULL && ! fspec->none_ok && ospec->_default_value != NULL)
    {
      modifying = TRUE;
    }
  else if (file != NULL && g_file_is_native (file))
    {
      gboolean  exists = g_file_query_exists (file, NULL);
      GFileType type   = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      switch (fspec->action)
        {
        case CLAY_FILE_CHOOSER_ACTION_OPEN:
          modifying = (! exists || type != G_FILE_TYPE_REGULAR);
          break;
        case CLAY_FILE_CHOOSER_ACTION_SAVE:
          modifying = (exists && type != G_FILE_TYPE_REGULAR);
          break;
        case CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER:
          modifying = (! exists || type != G_FILE_TYPE_DIRECTORY);
          break;
        case CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER:
          modifying = (exists && type != G_FILE_TYPE_DIRECTORY);
          break;
        case CLAY_FILE_CHOOSER_ACTION_ANY:
          break;
        }
    }

  if (modifying)
    {
      g_clear_object (&file);
      value->data[0].v_pointer = ospec->_default_value ? g_object_ref (ospec->_default_value) : NULL;
    }

  return modifying;
}

static gint
clay_param_spec_file_cmp (GParamSpec   *pspec,
                          const GValue *value1,
                          const GValue *value2)
{
  GFile *file1 = g_value_get_object (value1);
  GFile *file2 = g_value_get_object (value2);
  gchar *uri1;
  gchar *uri2;
  gint   retval;

  if (! file1 || ! file2)
    return file2 ? -1 : (file1 ? 1 : 0);

  uri1 = g_file_get_uri (file1);
  uri2 = g_file_get_uri (file2);

  retval = g_strcmp0 (uri1, uri2);

  g_free (uri1);
  g_free (uri2);

  return retval;
}

static GParamSpec *
clay_param_spec_file_duplicate (GParamSpec *pspec)
{
  ClayParamSpecObject *ospec;
  ClayParamSpecFile   *fspec;
  GParamSpec          *duplicate;

  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_FILE (pspec), NULL);

  ospec     = CLAY_PARAM_SPEC_OBJECT (pspec);
  fspec     = CLAY_PARAM_SPEC_FILE (pspec);
  duplicate = clay_param_spec_file (pspec->name,
                                    g_param_spec_get_nick (pspec),
                                    g_param_spec_get_blurb (pspec),
                                    fspec->action, fspec->none_ok,
                                    G_FILE (ospec->_default_value),
                                    pspec->flags);
  return duplicate;
}

/**
 * clay_param_spec_file:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @action:        The type of file to expect.
 * @none_ok:       Whether %NULL is allowed.
 * @default_value: (nullable): File to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a file param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_file (const gchar           *name,
                      const gchar           *nick,
                      const gchar           *blurb,
                      ClayFileChooserAction  action,
                      gboolean               none_ok,
                      GFile                 *default_value,
                      GParamFlags            flags)
{
  ClayParamSpecFile   *fspec;
  ClayParamSpecObject *ospec;

  g_return_val_if_fail (default_value == NULL || G_IS_FILE (default_value), NULL);

  fspec = g_param_spec_internal (CLAY_TYPE_PARAM_FILE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (fspec, NULL);

  fspec->action         = action;
  fspec->none_ok        = none_ok;

  ospec                 = CLAY_PARAM_SPEC_OBJECT (fspec);
  ospec->_has_default   = TRUE;
  /* Note that we don't check none_ok and allows even NULL as default
   * value. What we won't allow will be trying to set a NULL value
   * later.
   */
  ospec->_default_value = default_value ? g_object_ref (G_OBJECT (default_value)) : NULL;

  return G_PARAM_SPEC (fspec);
}

/**
 * clay_param_spec_file_get_action:
 * @pspec: a #GParamSpec to hold a #GFile value.
 *
 * Returns: the file action tied to @pspec.
 *
 * Since: 3.0
 **/
ClayFileChooserAction
clay_param_spec_file_get_action (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_FILE (pspec), CLAY_FILE_CHOOSER_ACTION_ANY);

  return CLAY_PARAM_SPEC_FILE (pspec)->action;
}

/**
 * clay_param_spec_file_set_action:
 * @pspec:  a #GParamSpec to hold a #GFile value.
 * @action: new action for @pspec.
 *
 * Change the file action tied to @pspec.
 *
 * Since: 3.0
 **/
void
clay_param_spec_file_set_action (GParamSpec            *pspec,
                                 ClayFileChooserAction  action)
{
  g_return_if_fail (CLAY_IS_PARAM_SPEC_FILE (pspec));

  CLAY_PARAM_SPEC_FILE (pspec)->action = action;
}

/**
 * clay_param_spec_file_none_allowed:
 * @pspec: a #GParamSpec to hold a #GFile value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
clay_param_spec_file_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_FILE (pspec), FALSE);

  return CLAY_PARAM_SPEC_FILE (pspec)->none_ok;
}

/*
 * CLAY_TYPE_ARRAY
 */

/**
 * clay_array_new:
 * @data: (array length=length):
 * @length:
 * @static_data:
 */
ClayArray *
clay_array_new (const guint8 *data,
                gsize         length,
                gboolean      static_data)
{
  ClayArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (ClayArray);

  array->data        = static_data ? (guint8 *) data : g_memdup2 (data, length);
  array->length      = length;
  array->static_data = static_data;

  return array;
}

ClayArray *
clay_array_copy (const ClayArray *array)
{
  if (array)
    return clay_array_new (array->data, array->length, FALSE);

  return NULL;
}

void
clay_array_free (ClayArray *array)
{
  if (array)
    {
      if (! array->static_data)
        g_free (array->data);

      g_slice_free (ClayArray, array);
    }
}

G_DEFINE_BOXED_TYPE (ClayArray, clay_array, clay_array_copy, clay_array_free)


/*
 * CLAY_TYPE_PARAM_ARRAY
 */

static void       clay_param_array_class_init  (GParamSpecClass *klass);
static void       clay_param_array_init        (GParamSpec      *pspec);
static gboolean   clay_param_array_validate    (GParamSpec      *pspec,
                                                GValue          *value);
static gint       clay_param_array_values_cmp  (GParamSpec      *pspec,
                                                const GValue    *value1,
                                                const GValue    *value2);

GType
clay_param_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_array_class_init,
        NULL, NULL,
        sizeof (GParamSpecBoxed),
        0,
        (GInstanceInitFunc) clay_param_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "ClayParamArray", &info, 0);
    }

  return type;
}

static void
clay_param_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = CLAY_TYPE_ARRAY;
  klass->value_validate = clay_param_array_validate;
  klass->values_cmp     = clay_param_array_values_cmp;
}

static void
clay_param_array_init (GParamSpec *pspec)
{
}

static gboolean
clay_param_array_validate (GParamSpec *pspec,
                           GValue     *value)
{
  ClayArray *array = value->data[0].v_pointer;

  if (array)
    {
      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

static gint
clay_param_array_values_cmp (GParamSpec   *pspec,
                             const GValue *value1,
                             const GValue *value2)
{
  ClayArray *array1 = value1->data[0].v_pointer;
  ClayArray *array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (array1->length < array2->length)
    return -1;
  else if (array1->length > array2->length)
    return 1;

  return 0;
}

/**
 * clay_param_spec_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #ClayParamSpecArray specifying a
 * [type@Array] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecArray.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_array (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (CLAY_TYPE_PARAM_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
}

static const guint8 *
clay_value_get_array (const GValue *value)
{
  ClayArray *array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

static guint8 *
clay_value_dup_array (const GValue *value)
{
  ClayArray *array = value->data[0].v_pointer;

  if (array)
    return g_memdup2 (array->data, array->length);

  return NULL;
}

static void
clay_value_set_array (GValue       *value,
                      const guint8 *data,
                      gsize         length)
{
  ClayArray *array = clay_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

static void
clay_value_set_static_array (GValue       *value,
                             const guint8 *data,
                             gsize         length)
{
  ClayArray *array = clay_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

static void
clay_value_take_array (GValue *value,
                       guint8 *data,
                       gsize   length)
{
  ClayArray *array = clay_array_new (data, length, TRUE);

  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * CLAY_TYPE_INT32_ARRAY
 */

typedef ClayArray ClayInt32Array;
G_DEFINE_BOXED_TYPE (ClayInt32Array, clay_int32_array, clay_array_copy, clay_array_free)

/**
 * clay_int32_array_get_values:
 * @array: the #ClayArray representing #int32 values.
 * @length: the number of #int32 values in the returned array.
 *
 * Returns: (array length=length) (transfer none): a C-array of #gint32.
 */
const gint32 *
clay_int32_array_get_values (ClayArray *array,
                             gsize     *length)
{
  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (const gint32 *) array->data;
}

/**
 * clay_int32_array_set_values:
 * @array: the array to modify.
 * @values: (array length=length): the C-array.
 * @length: the number of #int32 values in @data.
 * @static_data: whether @data is a static rather than allocated array.
 */
void
clay_int32_array_set_values (ClayArray    *array,
                             const gint32 *values,
                             gsize         length,
                             gboolean      static_data)
{
  g_return_if_fail ((values == NULL && length == 0) || (values != NULL && length  > 0));

  if (! array->static_data)
    g_free (array->data);

  array->length      = length * sizeof (gint32);
  array->data        = static_data ? (guint8 *) values : g_memdup2 (values, array->length);
  array->static_data = static_data;
}


/*
 * CLAY_TYPE_PARAM_INT32_ARRAY
 */

static void   clay_param_int32_array_class_init (GParamSpecClass *klass);
static void   clay_param_int32_array_init       (GParamSpec      *pspec);

GType
clay_param_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_int32_array_class_init,
        NULL, NULL,
        sizeof (GParamSpecBoxed),
        0,
        (GInstanceInitFunc) clay_param_int32_array_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_ARRAY,
                                     "ClayParamInt32Array", &info, 0);
    }

  return type;
}

static void
clay_param_int32_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_INT32_ARRAY;
}

static void
clay_param_int32_array_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_int32_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #ClayParamSpecInt32Array specifying a
 * %CLAY_TYPE_INT32_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecInt32Array.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_int32_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (CLAY_TYPE_PARAM_INT32_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
}

/**
 * clay_value_get_int32_array:
 * @value: A valid value of type %CLAY_TYPE_INT32_ARRAY
 * @length: the number of returned #int32 elements.
 *
 * Gets the contents of a %CLAY_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer none) (array length=length): The contents of @value
 */
const gint32 *
clay_value_get_int32_array (const GValue *value,
                            gsize        *length)
{
  ClayArray *array;

  g_return_val_if_fail (CLAY_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (const gint32 *) clay_value_get_array (value);
}

/**
 * clay_value_dup_int32_array:
 * @value: A valid value of type %CLAY_TYPE_INT32_ARRAY
 * @length: the number of returned #int32 elements.
 *
 * Gets the contents of a %CLAY_TYPE_INT32_ARRAY #GValue
 *
 * Returns: (transfer full) (array length=length): The contents of @value
 */
gint32 *
clay_value_dup_int32_array (const GValue *value,
                            gsize        *length)
{
  ClayArray *array;

  g_return_val_if_fail (CLAY_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gint32) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gint32);

  return (gint32 *) clay_value_dup_array (value);
}

/**
 * clay_value_set_int32_array:
 * @value: A valid value of type %CLAY_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
clay_value_set_int32_array (GValue       *value,
                            const gint32 *data,
                            gsize         length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_INT32_ARRAY (value));

  clay_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint32));
}

/**
 * clay_value_set_static_int32_array:
 * @value: A valid value of type %CLAY_TYPE_INT32_ARRAY
 * @data: (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
clay_value_set_static_int32_array (GValue       *value,
                                   const gint32 *data,
                                   gsize         length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_INT32_ARRAY (value));

  clay_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint32));
}

/**
 * clay_value_take_int32_array:
 * @value: A valid value of type %CLAY_TYPE_int32_ARRAY
 * @data: (transfer full) (array length=length): A #gint32 array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
clay_value_take_int32_array (GValue *value,
                             gint32 *data,
                             gsize   length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_INT32_ARRAY (value));

  clay_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint32));
}


/*
 * CLAY_TYPE_DOUBLE_ARRAY
 */

typedef ClayArray ClayDoubleArray;
G_DEFINE_BOXED_TYPE (ClayDoubleArray, clay_double_array, clay_array_copy, clay_array_free)

/**
 * clay_double_array_get_values:
 * @array: the #ClayArray representing #double values.
 * @length: the number of #double values in the returned array.
 *
 * Returns: (array length=length) (transfer none): a C-array of #gdouble.
 */
const gdouble *
clay_double_array_get_values (ClayArray *array,
                              gsize     *length)
{
  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (const gdouble *) array->data;
}

/**
 * clay_double_array_set_values:
 * @array: the array to modify.
 * @values: (array length=length): the C-array.
 * @length: the number of #double values in @data.
 * @static_data: whether @data is a static rather than allocated array.
 */
void
clay_double_array_set_values (ClayArray     *array,
                              const gdouble *values,
                              gsize          length,
                              gboolean       static_data)
{
  g_return_if_fail ((values == NULL && length == 0) || (values != NULL && length  > 0));

  if (! array->static_data)
    g_free (array->data);

  array->length      = length * sizeof (gdouble);
  array->data        = static_data ? (guint8 *) values : g_memdup2 (values, array->length);
  array->static_data = static_data;
}


/*
 * CLAY_TYPE_PARAM_DOUBLE_ARRAY
 */

static void   clay_param_double_array_class_init (GParamSpecClass *klass);
static void   clay_param_double_array_init       (GParamSpec      *pspec);

GType
clay_param_double_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_double_array_class_init,
        NULL, NULL,
        sizeof (GParamSpecBoxed),
        0,
        (GInstanceInitFunc) clay_param_double_array_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_ARRAY,
                                     "ClayParamDoubleArray", &info, 0);
    }

  return type;
}

static void
clay_param_double_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_DOUBLE_ARRAY;
}

static void
clay_param_double_array_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_double_array:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #ClayParamSpecDoubleArray specifying a
 * %CLAY_TYPE_DOUBLE_ARRAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecDoubleArray.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_double_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GParamFlags  flags)
{
  GParamSpec *array_spec;

  array_spec = g_param_spec_internal (CLAY_TYPE_PARAM_DOUBLE_ARRAY,
                                      name, nick, blurb, flags);

  return array_spec;
}

/**
 * clay_value_get_double_array:
 * @value: A valid value of type %CLAY_TYPE_DOUBLE_ARRAY
 * @length: the number of returned #double elements.
 *
 * Gets the contents of a %CLAY_TYPE_DOUBLE_ARRAY #GValue
 *
 * Returns: (transfer none) (array length=length): The contents of @value
 */
const gdouble *
clay_value_get_double_array (const GValue *value,
                             gsize        *length)
{
  ClayArray *array;

  g_return_val_if_fail (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (const gdouble *) clay_value_get_array (value);
}

/**
 * clay_value_dup_double_array:
 * @value: A valid value of type %CLAY_TYPE_DOUBLE_ARRAY
 * @length: the number of returned #double elements.
 *
 * Gets the contents of a %CLAY_TYPE_DOUBLE_ARRAY #GValue
 *
 * Returns: (transfer full) (array length=length): The contents of @value
 */
gdouble *
clay_value_dup_double_array (const GValue *value,
                             gsize        *length)
{
  ClayArray *array;

  g_return_val_if_fail (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  g_return_val_if_fail (array->length % sizeof (gdouble) == 0, NULL);

  if (length)
    *length = array->length / sizeof (gdouble);

  return (gdouble *) clay_value_dup_array (value);
}

/**
 * clay_value_set_double_array:
 * @value: A valid value of type %CLAY_TYPE_DOUBLE_ARRAY
 * @data: (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data.
 */
void
clay_value_set_double_array (GValue        *value,
                             const gdouble *data,
                             gsize         length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value));

  clay_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

/**
 * clay_value_set_static_double_array:
 * @value: A valid value of type %CLAY_TYPE_DOUBLE_ARRAY
 * @data: (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, without copying the data.
 */
void
clay_value_set_static_double_array (GValue        *value,
                                    const gdouble *data,
                                    gsize         length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value));

  clay_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

/**
 * clay_value_take_double_array:
 * @value: A valid value of type %CLAY_TYPE_DOUBLE_ARRAY
 * @data: (transfer full) (array length=length): A #gdouble array
 * @length: The number of elements in @data
 *
 * Sets the contents of @value to @data, and takes ownership of @data.
 */
void
clay_value_take_double_array (GValue  *value,
                              gdouble *data,
                              gsize    length)
{
  g_return_if_fail (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value));

  clay_value_take_array (value, (guint8 *) data,
                         length * sizeof (gdouble));
}


/*
 * CLAY_TYPE_COLOR_ARRAY
 */

GType
clay_color_array_get_type (void)
{
  static gsize static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      GType g_define_type_id =
        g_boxed_type_register_static (g_intern_static_string ("ClayColorArray"),
                                      (GBoxedCopyFunc) clay_color_array_copy,
                                      (GBoxedFreeFunc) clay_color_array_free);

      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

/**
 * clay_color_array_copy:
 * @array: an array of colors.
 *
 * Creates a new #ClayColorArray containing a deep copy of a %NULL-terminated
 * array of [class@Gegl.Color].
 *
 * Returns: (transfer full): a new #ClayColorArray.
 **/
ClayColorArray
clay_color_array_copy (ClayColorArray array)
{
  GeglColor **copy;
  gint        length = clay_color_array_get_length (array);

  copy = g_malloc0 (sizeof (GeglColor *) * (length + 1));

  for (gint i = 0; i < length; i++)
    copy[i] = gegl_color_duplicate (array[i]);

  return copy;
}

/**
 * clay_color_array_free:
 * @array: an array of colors.
 *
 * Frees a %NULL-terminated array of [class@Gegl.Color].
 **/
void
clay_color_array_free (ClayColorArray array)
{
  gint i = 0;

  while (array[i] != NULL)
    g_object_unref (array[i++]);

  g_free (array);
}

/**
 * clay_color_array_get_length:
 * @array: an array of colors.
 *
 * Returns: the number of [class@Gegl.Color] in @array.
 **/
gint
clay_color_array_get_length (ClayColorArray array)
{
  gint length = 0;

  while (array[length] != NULL)
    length++;

  return length;
}


/*
 * CLAY_TYPE_CORE_OBJECT_ARRAY
 */

static ClayCoreObjectArray clay_core_object_array_copy (ClayCoreObjectArray array);

GType
clay_core_object_array_get_type (void)
{
  static gsize static_g_define_type_id = 0;

  if (g_once_init_enter (&static_g_define_type_id))
    {
      GType g_define_type_id =
        g_boxed_type_register_static (g_intern_static_string ("ClayCoreObjectArray"),
                                      (GBoxedCopyFunc) clay_core_object_array_copy,
                                      (GBoxedFreeFunc) g_free);

      g_once_init_leave (&static_g_define_type_id, g_define_type_id);
    }

  return static_g_define_type_id;
}

/**
 * clay_core_object_array_get_length:
 * @array: a %NULL-terminated array of objects.
 *
 * Returns: the number of [class@GObject.Object] in @array.
 **/
gsize
clay_core_object_array_get_length (GObject **array)
{
  gsize length = 0;

  while (array && array[length] != NULL)
    length++;

  return length;
}

/**
 * clay_core_object_array_copy:
 * @array: an array of core_objects.
 *
 * Duplicate a new #ClayCoreObjectArray, which is basically simply a
 * %NULL-terminated array of [class@GObject.Object]. Note that you
 * should only use this alias type for arrays of core type objects
 * internally hold by `libclay`, such as layers, channels, paths, images
 * and so on, because no reference is hold to the element objects.
 *
 * As a consequence, the copy also makes a shallow copy of the elements.
 *
 * Returns: (transfer container) (array zero-terminated=1): a new #ClayCoreObjectArray.
 **/
static ClayCoreObjectArray
clay_core_object_array_copy (ClayCoreObjectArray array)
{
  GObject **copy;
  gsize     length = clay_core_object_array_get_length (array);

  copy = g_malloc0 (sizeof (GObject *) * (length + 1));

  for (gint i = 0; i < length; i++)
    copy[i] = array[i];

  return copy;
}


/*
 * CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY
 */

#define CLAY_PARAM_SPEC_CORE_OBJECT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY, ClayParamSpecCoreObjectArray))

typedef struct _ClayParamSpecCoreObjectArray ClayParamSpecCoreObjectArray;

struct _ClayParamSpecCoreObjectArray
{
  GParamSpecBoxed parent_instance;

  GType           object_type;
};

static void       clay_param_core_object_array_class_init  (GParamSpecClass *klass);
static void       clay_param_core_object_array_init        (GParamSpec      *pspec);
static gboolean   clay_param_core_object_array_validate    (GParamSpec      *pspec,
                                                            GValue          *value);
static gint       clay_param_core_object_array_values_cmp  (GParamSpec      *pspec,
                                                            const GValue    *value1,
                                                            const GValue    *value2);

GType
clay_param_core_object_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_core_object_array_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecCoreObjectArray),
        0,
        (GInstanceInitFunc) clay_param_core_object_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "ClayParamCoreObjectArray", &info, 0);
    }

  return type;
}

static void
clay_param_core_object_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = CLAY_TYPE_CORE_OBJECT_ARRAY;
  klass->value_validate = clay_param_core_object_array_validate;
  klass->values_cmp     = clay_param_core_object_array_values_cmp;
}

static void
clay_param_core_object_array_init (GParamSpec *pspec)
{
}

static gboolean
clay_param_core_object_array_validate (GParamSpec *pspec,
                                       GValue     *value)
{
  ClayParamSpecCoreObjectArray  *array_spec = CLAY_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec);
  GObject                      **array      = value->data[0].v_pointer;

  if (array)
    {
      gsize length = clay_core_object_array_get_length (array);
      gsize i;

      if (length == 0)
        {
          g_value_set_boxed (value, NULL);
          return FALSE;
        }

      for (i = 0; i < length; i++)
        {
          if (array[i] && ! g_type_is_a (G_OBJECT_TYPE (array[i]), array_spec->object_type))
            {
              g_value_set_boxed (value, NULL);
              return TRUE;
            }
        }
    }

  return FALSE;
}

static gint
clay_param_core_object_array_values_cmp (GParamSpec   *pspec,
                                         const GValue *value1,
                                         const GValue *value2)
{
  GObject **array1 = value1->data[0].v_pointer;
  GObject **array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (clay_core_object_array_get_length (array1) < clay_core_object_array_get_length (array2))
    return -1;
  else if (clay_core_object_array_get_length (array1) > clay_core_object_array_get_length (array2))
    return 1;

  for (gsize i = 0; i < clay_core_object_array_get_length (array1); i++)
    if (array1[0] != array2[0])
      return 1;

  return 0;
}

/**
 * clay_param_spec_core_object_array:
 * @name:        Canonical name of the property specified.
 * @nick:        Nick name of the property specified.
 * @blurb:       Description of the property specified.
 * @object_type: GType of the array's elements.
 * @flags:       Flags for the property specified.
 *
 * Creates a new #ClayParamSpecCoreObjectArray specifying a
 * [type@CoreObjectArray] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecCoreObjectArray.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_core_object_array (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   GType        object_type,
                                   GParamFlags  flags)
{
  ClayParamSpecCoreObjectArray *array_spec;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);

  array_spec = g_param_spec_internal (CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY,
                                      name, nick, blurb, flags);

  g_return_val_if_fail (array_spec, NULL);

  array_spec->object_type = object_type;

  return G_PARAM_SPEC (array_spec);
}

/**
 * clay_param_spec_core_object_array_get_object_type:
 * @pspec: a #GParamSpec to hold a #ClayParamSpecCoreObjectArray value.
 *
 * Returns: the type for objects in the object array.
 *
 * Since: 3.0
 **/
GType
clay_param_spec_core_object_array_get_object_type (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec), G_TYPE_NONE);

  return CLAY_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec)->object_type;
}
