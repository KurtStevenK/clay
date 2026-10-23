/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenumlabel.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "claywidgetstypes.h"

#include "clayenumlabel.h"


/**
 * SECTION: clayenumlabel
 * @title: ClayEnumLabel
 * @short_description: A #GtkLabel subclass that displays an enum value.
 *
 * A #GtkLabel subclass that displays an enum value.
 **/


enum
{
  PROP_0,
  PROP_ENUM_TYPE,
  PROP_ENUM_VALUE
};


struct _ClayEnumLabel
{
  GtkLabel    parent_instance;

  GEnumClass *enum_class;
};


static void   clay_enum_label_finalize     (GObject      *object);
static void   clay_enum_label_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void   clay_enum_label_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE (ClayEnumLabel, clay_enum_label, GTK_TYPE_LABEL)

#define parent_class clay_enum_label_parent_class


static void
clay_enum_label_class_init (ClayEnumLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_enum_label_finalize;
  object_class->get_property = clay_enum_label_get_property;
  object_class->set_property = clay_enum_label_set_property;

  /**
   * ClayEnumLabel:enum-type:
   *
   * The #GType of the enum.
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class, PROP_ENUM_TYPE,
                                   g_param_spec_gtype ("enum-type",
                                                       "Enum Type",
                                                       "The type of the displayed enum",
                                                       G_TYPE_NONE,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  /**
   * ClayEnumLabel:enum-value:
   *
   * The value to display.
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class, PROP_ENUM_VALUE,
                                   g_param_spec_int ("enum-value",
                                                     "Enum Value",
                                                     "The enum value to display",
                                                     G_MININT, G_MAXINT, 0,
                                                     CLAY_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static void
clay_enum_label_init (ClayEnumLabel *enum_label)
{
}

static void
clay_enum_label_finalize (GObject *object)
{
  ClayEnumLabel *label = CLAY_ENUM_LABEL (object);

  g_clear_pointer (&label->enum_class, g_type_class_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_enum_label_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayEnumLabel *label = CLAY_ENUM_LABEL (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      if (label->enum_class)
        g_value_set_gtype (value, G_TYPE_FROM_CLASS (label->enum_class));
      else
        g_value_set_gtype (value, G_TYPE_NONE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_enum_label_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayEnumLabel *label = CLAY_ENUM_LABEL (object);

  switch (property_id)
    {
    case PROP_ENUM_TYPE:
      label->enum_class = g_type_class_ref (g_value_get_gtype (value));
      break;

    case PROP_ENUM_VALUE:
      clay_enum_label_set_value (label, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_enum_label_new:
 * @enum_type: the #GType of an enum
 * @value:     an enum value
 *
 * Returns: a new #ClayEnumLabel.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_enum_label_new (GType enum_type,
                     gint  value)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  return g_object_new (CLAY_TYPE_ENUM_LABEL,
                       "enum-type",  enum_type,
                       "enum-value", value,
                       NULL);
}

/**
 * clay_enum_label_set_value
 * @label: a #ClayEnumLabel
 * @value: an enum value
 *
 * Since: 2.4
 **/
void
clay_enum_label_set_value (ClayEnumLabel *label,
                           gint           value)
{
  const gchar *nick;
  const gchar *desc;

  g_return_if_fail (CLAY_IS_ENUM_LABEL (label));

  if (! clay_enum_get_value (G_TYPE_FROM_CLASS (label->enum_class), value,
                             NULL, &nick, &desc, NULL))
    {
      g_warning ("%s: %d is not valid for enum of type '%s'",
                 G_STRLOC, value,
                 g_type_name (G_TYPE_FROM_CLASS (label->enum_class)));
      return;
    }

  if (! desc)
    desc = nick;

  gtk_label_set_text (GTK_LABEL (label), desc);
}
