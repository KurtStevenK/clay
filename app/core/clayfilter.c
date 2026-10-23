/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfilter.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clay-memsize.h"
#include "clayfilter.h"


enum
{
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_IS_LAST_NODE
};


typedef struct _ClayFilterPrivate ClayFilterPrivate;

struct _ClayFilterPrivate
{
  GeglNode       *node;

  guint           active       : 1;
  guint           is_last_node : 1;

  ClayApplicator *applicator;
};

#define GET_PRIVATE(filter) \
  ((ClayFilterPrivate *) clay_filter_get_instance_private ((ClayFilter *) (filter)))


/*  local function prototypes  */

static void       clay_filter_finalize      (GObject      *object);
static void       clay_filter_set_property  (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void       clay_filter_get_property  (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

static gint64     clay_filter_get_memsize   (ClayObject   *object,
                                             gint64       *gui_size);

static GeglNode * clay_filter_real_get_node (ClayFilter   *filter);


G_DEFINE_TYPE_WITH_PRIVATE (ClayFilter, clay_filter, CLAY_TYPE_VIEWABLE)

#define parent_class clay_filter_parent_class

static guint clay_filter_signals[LAST_SIGNAL] = { 0 };


static void
clay_filter_class_init (ClayFilterClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  clay_filter_signals[ACTIVE_CHANGED] =
    g_signal_new ("active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayFilterClass, active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize         = clay_filter_finalize;
  object_class->set_property     = clay_filter_set_property;
  object_class->get_property     = clay_filter_get_property;

  clay_object_class->get_memsize = clay_filter_get_memsize;

  klass->active_changed          = NULL;
  klass->get_node                = clay_filter_real_get_node;

  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active", NULL, NULL,
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_IS_LAST_NODE,
                                   g_param_spec_boolean ("is-last-node",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_filter_init (ClayFilter *filter)
{
  ClayFilterPrivate *private = GET_PRIVATE (filter);

  private->active = TRUE;
}

static void
clay_filter_finalize (GObject *object)
{
  ClayFilterPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->node);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_filter_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ClayFilter *filter = CLAY_FILTER (object);

  switch (property_id)
    {
    case PROP_ACTIVE:
      clay_filter_set_active (filter, g_value_get_boolean (value));
      break;
    case PROP_IS_LAST_NODE:
      clay_filter_set_is_last_node (filter, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_filter_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ClayFilterPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, private->active);
      break;
    case PROP_IS_LAST_NODE:
      g_value_set_boolean (value, private->is_last_node);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_filter_get_memsize (ClayObject *object,
                         gint64     *gui_size)
{
  ClayFilterPrivate *private = GET_PRIVATE (object);
  gint64             memsize = 0;

  memsize += clay_g_object_get_memsize (G_OBJECT (private->node));

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GeglNode *
clay_filter_real_get_node (ClayFilter *filter)
{
  ClayFilterPrivate *private = GET_PRIVATE (filter);

  private->node = gegl_node_new ();

  return private->node;
}


/*  public functions  */

ClayFilter *
clay_filter_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CLAY_TYPE_FILTER,
                       "name", name,
                       NULL);
}

GeglNode *
clay_filter_get_node (ClayFilter *filter)
{
  ClayFilterPrivate *private = GET_PRIVATE (filter);

  g_return_val_if_fail (CLAY_IS_FILTER (filter), NULL);

  if (private->node)
    return private->node;

  return CLAY_FILTER_GET_CLASS (filter)->get_node (filter);
}

GeglNode *
clay_filter_peek_node (ClayFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->node;
}

void
clay_filter_set_active (ClayFilter *filter,
                        gboolean    active)
{
  g_return_if_fail (CLAY_IS_FILTER (filter));

  active = active ? TRUE : FALSE;

  if (active != clay_filter_get_active (filter))
    {
      GET_PRIVATE (filter)->active = active;

      g_signal_emit (filter, clay_filter_signals[ACTIVE_CHANGED], 0);

      g_object_notify (G_OBJECT (filter), "active");
    }
}

gboolean
clay_filter_get_active (ClayFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->active;
}

void
clay_filter_set_is_last_node (ClayFilter *filter,
                              gboolean    is_last_node)
{
  g_return_if_fail (CLAY_IS_FILTER (filter));

  is_last_node = is_last_node ? TRUE : FALSE;

  if (is_last_node != clay_filter_get_is_last_node (filter))
    {
      GET_PRIVATE (filter)->is_last_node = is_last_node;

      g_object_notify (G_OBJECT (filter), "is-last-node");
    }
}

gboolean
clay_filter_get_is_last_node (ClayFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->is_last_node;
}

void
clay_filter_set_applicator (ClayFilter     *filter,
                            ClayApplicator *applicator)
{
  ClayFilterPrivate *private = GET_PRIVATE (filter);

  g_return_if_fail (CLAY_IS_FILTER (filter));

  private->applicator = applicator;
}

ClayApplicator *
clay_filter_get_applicator (ClayFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->applicator;
}
