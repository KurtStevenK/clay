/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayparasite.c
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <process.h>                /* For _getpid() */
#endif

#include "claybasetypes.h"

#include "clayparasite.h"


/**
 * SECTION: clayparasite
 * @title: ClayParasite
 * @short_description: Arbitrary pieces of data which can be attached
 *                     to various Clay objects.
 * @see_also: clay_image_attach_parasite(), clay_item_attach_parasite(),
 *            clay_attach_parasite() and their related functions.
 *
 * Arbitrary pieces of data which can be attached to various Clay objects.
 **/


/*
 * CLAY_TYPE_PARASITE
 */

G_DEFINE_BOXED_TYPE (ClayParasite, clay_parasite, clay_parasite_copy, clay_parasite_free)

/*
 * CLAY_TYPE_PARAM_PARASITE
 */


static void       clay_param_parasite_class_init  (GParamSpecClass *class);
static void       clay_param_parasite_init        (GParamSpec      *pspec);
static gboolean   clay_param_parasite_validate    (GParamSpec      *pspec,
                                                   GValue          *value);
static gint       clay_param_parasite_values_cmp  (GParamSpec      *pspec,
                                                   const GValue    *value1,
                                                   const GValue    *value2);

GType
clay_param_parasite_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_parasite_class_init,
        NULL, NULL,
        sizeof (GParamSpecBoxed),
        0,
        (GInstanceInitFunc) clay_param_parasite_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "ClayParamParasite",
                                     &type_info, 0);
    }

  return type;
}

static void
clay_param_parasite_class_init (GParamSpecClass *class)
{
  class->value_type     = CLAY_TYPE_PARASITE;
  class->value_validate = clay_param_parasite_validate;
  class->values_cmp     = clay_param_parasite_values_cmp;
}

static void
clay_param_parasite_init (GParamSpec *pspec)
{
}

static gboolean
clay_param_parasite_validate (GParamSpec *pspec,
                              GValue     *value)
{
  ClayParasite *parasite = value->data[0].v_pointer;

  if (! parasite)
    {
      return TRUE;
    }
  else if (parasite->name == NULL                          ||
           *parasite->name == '\0'                         ||
           ! g_utf8_validate (parasite->name, -1, NULL)    ||
           (parasite->size == 0 && parasite->data != NULL) ||
           (parasite->size >  0 && parasite->data == NULL))
    {
      g_value_set_boxed (value, NULL);
      return TRUE;
    }

  return FALSE;
}

static gint
clay_param_parasite_values_cmp (GParamSpec   *pspec,
                                const GValue *value1,
                                const GValue *value2)
{
  ClayParasite *parasite1 = value1->data[0].v_pointer;
  ClayParasite *parasite2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! parasite1)
    return parasite2 != NULL ? -1 : 0;
  else if (! parasite2)
    return parasite1 != NULL;
  else
    return clay_parasite_compare (parasite1, parasite2);
}

/**
 * clay_param_spec_parasite:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @flags: Flags for the property specified.
 *
 * Creates a new #ClayParamSpecParasite specifying a
 * [type@Parasite] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #ClayParamSpecParasite.
 *
 * Since: 2.4
 **/
GParamSpec *
clay_param_spec_parasite (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          GParamFlags  flags)
{
  GParamSpec *parasite_spec;

  parasite_spec = g_param_spec_internal (CLAY_TYPE_PARAM_PARASITE,
                                         name, nick, blurb, flags);

  return parasite_spec;
}


#ifdef DEBUG
static void
clay_parasite_print (ClayParasite *parasite)
{
  if (parasite == NULL)
    {
      g_print ("pid %d: attempt to print a null parasite\n", getpid ());
      return;
    }

  g_print ("pid %d: parasite: %p\n", getpid (), parasite);

  if (parasite->name)
    g_print ("\tname: %s\n", parasite->name);
  else
    g_print ("\tname: NULL\n");

  g_print ("\tflags: %d\n", parasite->flags);
  g_print ("\tsize: %d\n", parasite->size);
  if (parasite->size > 0)
    g_print ("\tdata: %p\n", parasite->data);
}
#endif

/**
 * clay_parasite_new:
 * @name:  the new #ClayParasite name.
 * @flags: see [const@Clay.PARASITE_PERSISTENT] and [const@Clay.PARASITE_UNDOABLE];
 *   other values are mainly intended for internal use.
 * @size:  the size of @data, including a terminal %NULL byte if needed.
 * @data:  (nullable) (array length=size) (element-type char): the data to save in a parasite.
 *
 * Creates a new parasite and save @data which may be a proper text (in
 * which case you may want to set @size as strlen(@data) + 1) or not.
 *
 * Returns: (transfer full): a new #ClayParasite.
 */
ClayParasite *
clay_parasite_new (const gchar    *name,
                   guint32         flags,
                   guint32         size,
                   gconstpointer   data)
{
  ClayParasite *parasite;

  if (! (name && *name))
    return NULL;

  parasite = g_slice_new (ClayParasite);
  parasite->name  = g_strdup (name);
  parasite->flags = (flags & 0xFF);
  parasite->size  = size;

  if (size)
    parasite->data = g_memdup2 (data, size);
  else
    parasite->data = NULL;

  return parasite;
}

/**
 * clay_parasite_free:
 * @parasite: a #ClayParasite
 *
 * Free @parasite's dynamically allocated memory.
 */
void
clay_parasite_free (ClayParasite *parasite)
{
  if (parasite == NULL)
    return;

  if (parasite->name)
    g_free (parasite->name);

  if (parasite->data)
    g_free (parasite->data);

  g_slice_free (ClayParasite, parasite);
}

/**
 * clay_parasite_is_type:
 * @parasite: a #ClayParasite
 * @name:     a parasite name.
 *
 * Compare parasite's names.
 *
 * Returns: %TRUE if @parasite is named @name, %FALSE otherwise.
 */
gboolean
clay_parasite_is_type (const ClayParasite *parasite,
                       const gchar        *name)
{
  if (!parasite || !parasite->name)
    return FALSE;

  return (strcmp (parasite->name, name) == 0);
}

/**
 * clay_parasite_copy:
 * @parasite: a #ClayParasite
 *
 * Create a new parasite with all the same values.
 *
 * Returns: (transfer full): a newly allocated #ClayParasite with same contents.
 */
ClayParasite *
clay_parasite_copy (const ClayParasite *parasite)
{
  if (parasite == NULL)
    return NULL;

  return clay_parasite_new (parasite->name, parasite->flags,
                            parasite->size, parasite->data);
}

/**
 * clay_parasite_compare:
 * @a: a #ClayParasite
 * @b: a #ClayParasite
 *
 * Compare parasite's contents.
 *
 * Returns: %TRUE if @a and @b have same contents, %FALSE otherwise.
 */
gboolean
clay_parasite_compare (const ClayParasite *a,
                       const ClayParasite *b)
{
  if (a && b &&
      a->name && b->name &&
      strcmp (a->name, b->name) == 0 &&
      a->flags == b->flags &&
      a->size == b->size)
    {
      if (a->data == NULL && b->data == NULL)
        return TRUE;
      else if (a->data && b->data && memcmp (a->data, b->data, a->size) == 0)
        return TRUE;
    }

  return FALSE;
}

/**
 * clay_parasite_get_flags:
 * @parasite: a #ClayParasite
 *
 * Get the flags of the parasite.
 *
 * Returns: @parasite flags.
 */
gulong
clay_parasite_get_flags (const ClayParasite *parasite)
{
  if (parasite == NULL)
    return 0;

  return parasite->flags;
}

/**
 * clay_parasite_is_persistent:
 * @parasite: a #ClayParasite
 *
 * Returns: %TRUE if @parasite is persistent, %FALSE otherwise.
 */
gboolean
clay_parasite_is_persistent (const ClayParasite *parasite)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & CLAY_PARASITE_PERSISTENT);
}

/**
 * clay_parasite_is_undoable:
 * @parasite: a #ClayParasite
 *
 * Returns: %TRUE if @parasite is undoable, %FALSE otherwise.
 */
gboolean
clay_parasite_is_undoable (const ClayParasite *parasite)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & CLAY_PARASITE_UNDOABLE);
}

/**
 * clay_parasite_has_flag:
 * @parasite: a #ClayParasite
 * @flag:     a parasite flag
 *
 * Returns: %TRUE if @parasite has @flag set, %FALSE otherwise.
 */
gboolean
clay_parasite_has_flag (const ClayParasite *parasite,
                        gulong              flag)
{
  if (parasite == NULL)
    return FALSE;

  return (parasite->flags & flag);
}

/**
 * clay_parasite_get_name:
 * @parasite: a #ClayParasite
 *
 * Returns: @parasite's name.
 */
const gchar *
clay_parasite_get_name (const ClayParasite *parasite)
{
  if (parasite)
    return parasite->name;

  return NULL;
}

/**
 * clay_parasite_get_data:
 * @parasite: a #ClayParasite
 * @num_bytes: (out) (nullable): size of the returned data.
 *
 * Gets the parasite's data. It may not necessarily be text, nor is it
 * guaranteed to be %NULL-terminated. It is your responsibility to know
 * how to deal with this data.
 * Even when you expect a nul-terminated string, it is advised not to
 * assume the returned data to be, as parasites can be edited by third
 * party scripts. You may end up reading out-of-bounds data. So you
 * should only ignore @num_bytes when you all you care about is checking
 * if the parasite has contents.
 *
 * Returns: (array length=num_bytes) (element-type char): parasite's data.
 */
gconstpointer
clay_parasite_get_data (const ClayParasite *parasite,
                        guint32            *num_bytes)
{
  if (parasite)
    {
      if (num_bytes)
        *num_bytes = parasite->size;

      return parasite->data;
    }

  if (num_bytes)
    *num_bytes = 0;

  return NULL;
}
