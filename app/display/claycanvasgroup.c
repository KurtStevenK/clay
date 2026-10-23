/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasgroup.c
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

#include "claycanvasgroup.h"
#include "claydisplayshell.h"


enum
{
  PROP_0,
  PROP_GROUP_STROKING,
  PROP_GROUP_FILLING
};


struct _ClayCanvasGroupPrivate
{
  GQueue   *items;
  gboolean  group_stroking;
  gboolean  group_filling;
};


/*  local function prototypes  */

static void             clay_canvas_group_finalize     (GObject         *object);
static void             clay_canvas_group_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void             clay_canvas_group_get_property (GObject         *object,
                                                        guint            property_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);
static void             clay_canvas_group_draw         (ClayCanvasItem  *item,
                                                        cairo_t         *cr);
static cairo_region_t * clay_canvas_group_get_extents  (ClayCanvasItem  *item);
static gboolean         clay_canvas_group_hit          (ClayCanvasItem  *item,
                                                        gdouble          x,
                                                        gdouble          y);

static void             clay_canvas_group_child_update (ClayCanvasItem  *item,
                                                        cairo_region_t  *region,
                                                        ClayCanvasGroup *group);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasGroup, clay_canvas_group,
                            CLAY_TYPE_CANVAS_ITEM)

#define parent_class clay_canvas_group_parent_class


static void
clay_canvas_group_class_init (ClayCanvasGroupClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayCanvasItemClass *item_class   = CLAY_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = clay_canvas_group_finalize;
  object_class->set_property = clay_canvas_group_set_property;
  object_class->get_property = clay_canvas_group_get_property;

  item_class->draw           = clay_canvas_group_draw;
  item_class->get_extents    = clay_canvas_group_get_extents;
  item_class->hit            = clay_canvas_group_hit;

  g_object_class_install_property (object_class, PROP_GROUP_STROKING,
                                   g_param_spec_boolean ("group-stroking",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GROUP_FILLING,
                                   g_param_spec_boolean ("group-filling",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_canvas_group_init (ClayCanvasGroup *group)
{
  group->priv = clay_canvas_group_get_instance_private (group);

  group->priv->items = g_queue_new ();
}

static void
clay_canvas_group_finalize (GObject *object)
{
  ClayCanvasGroup *group = CLAY_CANVAS_GROUP (object);
  ClayCanvasItem  *item;

  while ((item = g_queue_peek_head (group->priv->items)))
    clay_canvas_group_remove_item (group, item);

  g_queue_free (group->priv->items);
  group->priv->items = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_canvas_group_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayCanvasGroup *group = CLAY_CANVAS_GROUP (object);

  switch (property_id)
    {
    case PROP_GROUP_STROKING:
      group->priv->group_stroking = g_value_get_boolean (value);
      break;
    case PROP_GROUP_FILLING:
      group->priv->group_filling = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_group_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayCanvasGroup *group = CLAY_CANVAS_GROUP (object);

  switch (property_id)
    {
    case PROP_GROUP_STROKING:
      g_value_set_boolean (value, group->priv->group_stroking);
      break;
    case PROP_GROUP_FILLING:
      g_value_set_boolean (value, group->priv->group_filling);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_group_draw (ClayCanvasItem *item,
                        cairo_t        *cr)
{
  ClayCanvasGroup *group = CLAY_CANVAS_GROUP (item);
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      ClayCanvasItem *sub_item = list->data;

      clay_canvas_item_draw (sub_item, cr);
    }

  if (group->priv->group_stroking)
    _clay_canvas_item_stroke (item, cr);

  if (group->priv->group_filling)
    _clay_canvas_item_fill (item, cr);
}

static cairo_region_t *
clay_canvas_group_get_extents (ClayCanvasItem *item)
{
  ClayCanvasGroup *group  = CLAY_CANVAS_GROUP (item);
  cairo_region_t  *region = NULL;
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      ClayCanvasItem *sub_item   = list->data;
      cairo_region_t *sub_region = clay_canvas_item_get_extents (sub_item);

      if (! region)
        {
          region = sub_region;
        }
      else if (sub_region)
        {
          cairo_region_union (region, sub_region);
          cairo_region_destroy (sub_region);
        }
    }

  return region;
}

static gboolean
clay_canvas_group_hit (ClayCanvasItem *item,
                       gdouble         x,
                       gdouble         y)
{
  ClayCanvasGroup *group = CLAY_CANVAS_GROUP (item);
  GList           *list;

  for (list = group->priv->items->head; list; list = g_list_next (list))
    {
      if (clay_canvas_item_hit (list->data, x, y))
        return TRUE;
    }

  return FALSE;
}

static void
clay_canvas_group_child_update (ClayCanvasItem  *item,
                                cairo_region_t  *region,
                                ClayCanvasGroup *group)
{
  if (_clay_canvas_item_needs_update (CLAY_CANVAS_ITEM (group)))
    _clay_canvas_item_update (CLAY_CANVAS_ITEM (group), region);
}


/*  public functions  */

ClayCanvasItem *
clay_canvas_group_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_GROUP,
                       "shell", shell,
                       NULL);
}

void
clay_canvas_group_add_item (ClayCanvasGroup *group,
                            ClayCanvasItem  *item)
{
  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));
  g_return_if_fail (CLAY_CANVAS_ITEM (group) != item);

  if (group->priv->group_stroking)
    clay_canvas_item_suspend_stroking (item);

  if (group->priv->group_filling)
    clay_canvas_item_suspend_filling (item);

  g_queue_push_tail (group->priv->items, g_object_ref (item));

  if (_clay_canvas_item_needs_update (CLAY_CANVAS_ITEM (group)))
    {
      cairo_region_t *region = clay_canvas_item_get_extents (item);

      if (region)
        {
          _clay_canvas_item_update (CLAY_CANVAS_ITEM (group), region);
          cairo_region_destroy (region);
        }
    }

  g_signal_connect (item, "update",
                    G_CALLBACK (clay_canvas_group_child_update),
                    group);
}

void
clay_canvas_group_remove_item (ClayCanvasGroup *group,
                               ClayCanvasItem  *item)
{
  GList *list;

  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  list = g_queue_find (group->priv->items, item);

  g_return_if_fail (list != NULL);

  g_queue_delete_link (group->priv->items, list);

  if (group->priv->group_stroking)
    clay_canvas_item_resume_stroking (item);

  if (group->priv->group_filling)
    clay_canvas_item_resume_filling (item);

  if (_clay_canvas_item_needs_update (CLAY_CANVAS_ITEM (group)))
    {
      cairo_region_t *region = clay_canvas_item_get_extents (item);

      if (region)
        {
          _clay_canvas_item_update (CLAY_CANVAS_ITEM (group), region);
          cairo_region_destroy (region);
        }
    }

  g_signal_handlers_disconnect_by_func (item,
                                        clay_canvas_group_child_update,
                                        group);

  g_object_unref (item);
}

void
clay_canvas_group_set_group_stroking (ClayCanvasGroup *group,
                                      gboolean         group_stroking)
{
  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));

  if (group->priv->group_stroking != group_stroking)
    {
      GList *list;

      clay_canvas_item_begin_change (CLAY_CANVAS_ITEM (group));

      g_object_set (group,
                    "group-stroking", group_stroking ? TRUE : FALSE,
                    NULL);

      for (list = group->priv->items->head; list; list = g_list_next (list))
        {
          if (group->priv->group_stroking)
            clay_canvas_item_suspend_stroking (list->data);
          else
            clay_canvas_item_resume_stroking (list->data);
        }

      clay_canvas_item_end_change (CLAY_CANVAS_ITEM (group));
    }
}

void
clay_canvas_group_set_group_filling (ClayCanvasGroup *group,
                                     gboolean         group_filling)
{
  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));

  if (group->priv->group_filling != group_filling)
    {
      GList *list;

      clay_canvas_item_begin_change (CLAY_CANVAS_ITEM (group));

      g_object_set (group,
                    "group-filling", group_filling ? TRUE : FALSE,
                    NULL);

      for (list = group->priv->items->head; list; list = g_list_next (list))
        {
          if (group->priv->group_filling)
            clay_canvas_item_suspend_filling (list->data);
          else
            clay_canvas_item_resume_filling (list->data);
        }

      clay_canvas_item_end_change (CLAY_CANVAS_ITEM (group));
    }
}
