/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayparamspecs.h
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_PARAM_SPECS_H__
#define __CLAY_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * CLAY_PARAM_NO_VALIDATE:
 *
 * Since 3.0
 */
#define CLAY_PARAM_NO_VALIDATE    (1 << (0 + G_PARAM_USER_SHIFT))

/**
 * CLAY_PARAM_DONT_SERIALIZE:
 *
 * This property will be ignored when serializing and deserializing.
 * This is useful for ClayProcedure arguments for which you never want
 * the last run values to be restored.
 *
 * Since 3.0
 */
#define CLAY_PARAM_DONT_SERIALIZE (1 << (1 + G_PARAM_USER_SHIFT))

/**
 * CLAY_PARAM_FLAG_SHIFT:
 *
 * Minimum shift count to be used for libclayconfig defined
 * [flags@GObject.ParamFlags] (see libclayconfig/clayconfig-params.h).
 */
#define CLAY_PARAM_FLAG_SHIFT     (2 + G_PARAM_USER_SHIFT)

/**
 * CLAY_PARAM_STATIC_STRINGS:
 *
 * Since: 2.4
 **/
#define CLAY_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | \
                                   G_PARAM_STATIC_NICK | \
                                   G_PARAM_STATIC_BLURB)

/**
 * CLAY_PARAM_READABLE:
 *
 * Since: 2.4
 **/
#define CLAY_PARAM_READABLE       (G_PARAM_READABLE    | \
                                   CLAY_PARAM_STATIC_STRINGS)

/**
 * CLAY_PARAM_WRITABLE:
 *
 * Since: 2.4
 **/
#define CLAY_PARAM_WRITABLE       (G_PARAM_WRITABLE    | \
                                   CLAY_PARAM_STATIC_STRINGS)

/**
 * CLAY_PARAM_READWRITE:
 *
 * Since: 2.4
 **/
#define CLAY_PARAM_READWRITE      (G_PARAM_READWRITE   | \
                                   CLAY_PARAM_STATIC_STRINGS)


/*
 * CLAY_TYPE_PARAM_OBJECT
 */

#define CLAY_TYPE_PARAM_OBJECT                  (clay_param_object_get_type ())
#define CLAY_PARAM_SPEC_OBJECT(pspec)           (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_OBJECT, ClayParamSpecObject))
#define CLAY_PARAM_SPEC_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PARAM_OBJECT, ClayParamSpecObjectClass))
#define CLAY_IS_PARAM_SPEC_OBJECT(pspec)        (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_OBJECT))
#define CLAY_IS_PARAM_SPEC_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PARAM_OBJECT))
#define CLAY_PARAM_SPEC_OBJECT_GET_CLASS(pspec) (G_TYPE_INSTANCE_GET_CLASS ((pspec), G_TYPE_PARAM_OBJECT, ClayParamSpecObjectClass))


typedef struct _ClayParamSpecObject      ClayParamSpecObject;
typedef struct _ClayParamSpecObjectClass ClayParamSpecObjectClass;

struct _ClayParamSpecObject
{
  GParamSpecObject  parent_instance;

  /*< private >*/
  GObject          *_default_value;
  gboolean          _has_default;
};

struct _ClayParamSpecObjectClass
{
  /* XXX: vapigen fails with the following error without the private
   * comment:
   * > error: The type name `GLib.ParamSpecClass' could not be found
   * Not sure why it doesn't search for GObject.ParamSpecClass instead.
   * Anyway putting it private is good enough and hides the parent_class
   * to bindings.
   */
  /*< private >*/
  GParamSpecClass   parent_class;

  GParamSpec      * (* duplicate)   (GParamSpec *pspec);
  GObject         * (* get_default) (GParamSpec *pspec);

  /* Padding for future expansion */
  void              (*_clay_reserved0) (void);
  void              (*_clay_reserved1) (void);
  void              (*_clay_reserved2) (void);
  void              (*_clay_reserved3) (void);
  void              (*_clay_reserved4) (void);
  void              (*_clay_reserved5) (void);
  void              (*_clay_reserved6) (void);
  void              (*_clay_reserved7) (void);
  void              (*_clay_reserved8) (void);
  void              (*_clay_reserved9) (void);
};

GType        clay_param_object_get_type         (void);

GObject    * clay_param_spec_object_get_default (GParamSpec  *pspec);
void         clay_param_spec_object_set_default (GParamSpec  *pspec,
                                                 GObject     *default_value);
gboolean     clay_param_spec_object_has_default (GParamSpec  *pspec);

GParamSpec * clay_param_spec_object_duplicate   (GParamSpec  *pspec);


/*
 * CLAY_TYPE_PARAM_FILE
 */

#define CLAY_VALUE_HOLDS_FILE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_FILE))

#define CLAY_TYPE_PARAM_FILE           (clay_param_file_get_type ())
#define CLAY_IS_PARAM_SPEC_FILE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_FILE))


GType                   clay_param_file_get_type          (void);

GParamSpec            * clay_param_spec_file              (const gchar           *name,
                                                           const gchar           *nick,
                                                           const gchar           *blurb,
                                                           ClayFileChooserAction  action,
                                                           gboolean               none_ok,
                                                           GFile                 *default_value,
                                                           GParamFlags            flags);

ClayFileChooserAction   clay_param_spec_file_get_action   (GParamSpec            *pspec);
void                    clay_param_spec_file_set_action   (GParamSpec            *pspec,
                                                           ClayFileChooserAction  action);
gboolean                clay_param_spec_file_none_allowed (GParamSpec            *pspec);



/*
 * CLAY_TYPE_ARRAY
 */

/**
 * ClayArray:
 * @data: (array length=length): pointer to the array's data.
 * @length:      length of @data, in bytes.
 * @static_data: whether @data points to statically allocated memory.
 **/
typedef struct _ClayArray ClayArray;

struct _ClayArray
{
  guint8   *data;
  gsize     length;
  gboolean  static_data;
};

ClayArray * clay_array_new  (const guint8    *data,
                             gsize            length,
                             gboolean         static_data);
ClayArray * clay_array_copy (const ClayArray *array);
void        clay_array_free (ClayArray       *array);

#define CLAY_TYPE_ARRAY               (clay_array_get_type ())
#define CLAY_VALUE_HOLDS_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_ARRAY))

GType   clay_array_get_type           (void);


/*
 * CLAY_TYPE_PARAM_ARRAY
 */

#define CLAY_TYPE_PARAM_ARRAY           (clay_param_array_get_type ())
#define CLAY_IS_PARAM_SPEC_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_ARRAY))

GType        clay_param_array_get_type (void);

GParamSpec * clay_param_spec_array     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        GParamFlags   flags);


/*
 * CLAY_TYPE_INT32_ARRAY
 */

#define        CLAY_TYPE_INT32_ARRAY               (clay_int32_array_get_type ())
#define        CLAY_VALUE_HOLDS_INT32_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_INT32_ARRAY))

GType          clay_int32_array_get_type           (void);

const gint32 * clay_int32_array_get_values         (ClayArray    *array,
                                                    gsize        *length);
void           clay_int32_array_set_values         (ClayArray    *array,
                                                    const gint32 *values,
                                                    gsize         length,
                                                    gboolean      static_data);


/*
 * CLAY_TYPE_PARAM_INT32_ARRAY
 */

#define CLAY_TYPE_PARAM_INT32_ARRAY           (clay_param_int32_array_get_type ())
#define CLAY_IS_PARAM_SPEC_INT32_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_INT32_ARRAY))

GType          clay_param_int32_array_get_type   (void);

GParamSpec   * clay_param_spec_int32_array       (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gint32 * clay_value_get_int32_array        (const GValue *value,
                                                  gsize        *length);
gint32       * clay_value_dup_int32_array        (const GValue *value,
                                                  gsize        *length);
void           clay_value_set_int32_array        (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           clay_value_set_static_int32_array (GValue       *value,
                                                  const gint32 *data,
                                                  gsize         length);
void           clay_value_take_int32_array       (GValue       *value,
                                                  gint32       *data,
                                                  gsize         length);


/*
 * CLAY_TYPE_DOUBLE_ARRAY
 */

#define         CLAY_TYPE_DOUBLE_ARRAY               (clay_double_array_get_type ())
#define         CLAY_VALUE_HOLDS_DOUBLE_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_DOUBLE_ARRAY))

GType           clay_double_array_get_type           (void);

const gdouble * clay_double_array_get_values         (ClayArray     *array,
                                                      gsize         *length);
void            clay_double_array_set_values         (ClayArray     *array,
                                                      const gdouble *values,
                                                      gsize         length,
                                                      gboolean      static_data);


/*
 * CLAY_TYPE_PARAM_DOUBLE_ARRAY
 */

#define CLAY_TYPE_PARAM_DOUBLE_ARRAY           (clay_param_double_array_get_type ())
#define CLAY_IS_PARAM_SPEC_DOUBLE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_DOUBLE_ARRAY))

GType           clay_param_double_array_get_type   (void);

GParamSpec    * clay_param_spec_double_array       (const gchar  *name,
                                                    const gchar  *nick,
                                                    const gchar  *blurb,
                                                    GParamFlags   flags);

const gdouble * clay_value_get_double_array        (const GValue  *value,
                                                    gsize         *length);
gdouble       * clay_value_dup_double_array        (const GValue  *value,
                                                    gsize         *length);
void            clay_value_set_double_array        (GValue        *value,
                                                    const gdouble *data,
                                                    gsize         length);
void            clay_value_set_static_double_array (GValue        *value,
                                                    const gdouble *data,
                                                    gsize         length);
void            clay_value_take_double_array       (GValue        *value,
                                                    gdouble       *data,
                                                    gsize         length);


/*
 * CLAY_TYPE_COLOR_ARRAY
 */

/**
 * ClayColorArray:
 *
 * A boxed type which is nothing more than an alias to a %NULL-terminated array
 * of [class@Gegl.Color].
 *
 * The code fragments in the following example show the use of a property of
 * type %CLAY_TYPE_COLOR_ARRAY with g_object_class_install_property(),
 * g_object_set() and g_object_get().
 *
 * ```C
 * g_object_class_install_property (object_class,
 *                                  PROP_COLORS,
 *                                  g_param_spec_boxed ("colors",
 *                                                      _("Colors"),
 *                                                      _("List of colors"),
 *                                                      CLAY_TYPE_COLOR_ARRAY,
 *                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
 *
 * GeglColor *colors[] = { gegl_color_new ("red"), gegl_color_new ("blue"), NULL };
 *
 * g_object_set (obj, "colors", colors, NULL);
 *
 * GeglColors **colors;
 *
 * g_object_get (obj, "colors", &colors, NULL);
 * clay_color_array_free (colors);
 * ```
 *
 * Since: 3.0
 */
typedef GeglColor**                          ClayColorArray;

#define CLAY_TYPE_COLOR_ARRAY               (clay_color_array_get_type ())
#define CLAY_VALUE_HOLDS_COLOR_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_COLOR_ARRAY))


GType            clay_color_array_get_type            (void);

ClayColorArray   clay_color_array_copy                (ClayColorArray  array);
void             clay_color_array_free                (ClayColorArray  array);
gint             clay_color_array_get_length          (ClayColorArray  array);


/*
 * CLAY_TYPE_CORE_OBJECT_ARRAY
 */

/**
 * ClayCoreObjectArray:
 *
 * A boxed type which is nothing more than an alias to a %NULL-terminated array
 * of [class@GObject.Object]. No reference is being hold on contents
 * because core objects are owned by `libclay`.
 *
 * The reason of existence for this alias is to have common arrays of
 * objects as a boxed type easy to use as plug-in's procedure argument.
 *
 * You should never have to interact with this type directly, though
 * [func@Clay.core_object_array_get_length] might be convenient.
 *
 * Since: 3.0
 */
typedef GObject**                                 ClayCoreObjectArray;

#define CLAY_TYPE_CORE_OBJECT_ARRAY               (clay_core_object_array_get_type ())
#define CLAY_VALUE_HOLDS_CORE_OBJECT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_CORE_OBJECT_ARRAY))

GType clay_core_object_array_get_type   (void);
gsize clay_core_object_array_get_length (GObject **array);


/*
 * CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY
 */

#define CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY           (clay_param_core_object_array_get_type ())
#define CLAY_IS_PARAM_SPEC_CORE_OBJECT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY))

GType         clay_param_core_object_array_get_type             (void);

GParamSpec  * clay_param_spec_core_object_array                 (const gchar   *name,
                                                                 const gchar   *nick,
                                                                 const gchar   *blurb,
                                                                 GType          object_type,
                                                                 GParamFlags    flags);

GType         clay_param_spec_core_object_array_get_object_type (GParamSpec *pspec);


G_END_DECLS

#endif  /*  __CLAY_PARAM_SPECS_H__  */
