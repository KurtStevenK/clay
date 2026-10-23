/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasproxygroup.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "display-types.h"

#include "claycanvas.h"
#include "claycanvasproxygroup.h"
#include "claydisplayshell.h"


enum
{
  PROP_0
};


typedef struct _ClayCanvasProxyGroupPrivate ClayCanvasProxyGroupPrivate;

struct _ClayCanvasProxyGroupPrivate
{
  GHashTable *proxy_hash;
};

#define GET_PRIVATE(proxy_group) \
        ((ClayCanvasProxyGroupPrivate *) clay_canvas_proxy_group_get_instance_private ((ClayCanvasProxyGroup *) (proxy_group)))


/*  local function prototypes  */

static void        clay_canvas_proxy_group_finalize     (GObject          *object);
static void        clay_canvas_proxy_group_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void        clay_canvas_proxy_group_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasProxyGroup, clay_canvas_proxy_group,
                            CLAY_TYPE_CANVAS_GROUP)

#define parent_class clay_canvas_proxy_group_parent_class


static void
clay_canvas_proxy_group_class_init (ClayCanvasProxyGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_canvas_proxy_group_finalize;
  object_class->set_property = clay_canvas_proxy_group_set_property;
  object_class->get_property = clay_canvas_proxy_group_get_property;
}

static void
clay_canvas_proxy_group_init (ClayCanvasProxyGroup *proxy_group)
{
  ClayCanvasProxyGroupPrivate *private = GET_PRIVATE (proxy_group);

  private->proxy_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
clay_canvas_proxy_group_finalize (GObject *object)
{
  ClayCanvasProxyGroupPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->proxy_hash, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_canvas_proxy_group_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  /* ClayCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_proxy_group_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  /* ClayCanvasProxyGroupPrivate *private = GET_PRIVATE (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

ClayCanvasItem *
clay_canvas_proxy_group_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_PROXY_GROUP,
                       "shell", shell,
                       NULL);
}

void
clay_canvas_proxy_group_add_item (ClayCanvasProxyGroup *group,
                                  gpointer              object,
                                  ClayCanvasItem       *proxy_item)
{
  ClayCanvasProxyGroupPrivate *private;

  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (proxy_item));
  g_return_if_fail (CLAY_CANVAS_ITEM (group) != proxy_item);

  private = GET_PRIVATE (group);

  g_return_if_fail (g_hash_table_lookup (private->proxy_hash, object) ==
                    NULL);

  g_hash_table_insert (private->proxy_hash, object, proxy_item);

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (group), proxy_item);
}

void
clay_canvas_proxy_group_remove_item (ClayCanvasProxyGroup *group,
                                     gpointer              object)
{
  ClayCanvasProxyGroupPrivate *private;
  ClayCanvasItem              *proxy_item;

  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));
  g_return_if_fail (object != NULL);

  private = GET_PRIVATE (group);

  proxy_item = g_hash_table_lookup (private->proxy_hash, object);

  g_return_if_fail (proxy_item != NULL);

  g_hash_table_remove (private->proxy_hash, object);

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (group), proxy_item);
}

ClayCanvasItem *
clay_canvas_proxy_group_get_item (ClayCanvasProxyGroup *group,
                                  gpointer              object)
{
  ClayCanvasProxyGroupPrivate *private;

  g_return_val_if_fail (CLAY_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (object != NULL, NULL);

  private = GET_PRIVATE (group);

  return g_hash_table_lookup (private->proxy_hash, object);
}
