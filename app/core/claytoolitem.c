/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolitem.c
 * Copyright (C) 2020 Ell
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claytoolitem.h"


enum
{
  VISIBLE_CHANGED,
  SHOWN_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VISIBLE,
  PROP_SHOWN
};


struct _ClayToolItemPrivate
{
  gboolean visible;
};


/*  local function prototypes  */

static void   clay_tool_item_get_property  (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void   clay_tool_item_set_property  (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolItem, clay_tool_item, CLAY_TYPE_VIEWABLE)

#define parent_class clay_tool_item_parent_class

static guint clay_tool_item_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */

static void
clay_tool_item_class_init (ClayToolItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  clay_tool_item_signals[VISIBLE_CHANGED] =
    g_signal_new ("visible-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolItemClass, visible_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_tool_item_signals[SHOWN_CHANGED] =
    g_signal_new ("shown-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolItemClass, shown_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property = clay_tool_item_get_property;
  object_class->set_property = clay_tool_item_set_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_VISIBLE,
                            "visible", NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_SHOWN,
                                   g_param_spec_boolean ("shown", NULL, NULL,
                                                         TRUE,
                                                         CLAY_PARAM_READABLE));
}

static void
clay_tool_item_init (ClayToolItem *tool_item)
{
  tool_item->priv = clay_tool_item_get_instance_private (tool_item);
}

static void
clay_tool_item_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayToolItem *tool_item = CLAY_TOOL_ITEM (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value, tool_item->priv->visible);
      break;
    case PROP_SHOWN:
      g_value_set_boolean (value, clay_tool_item_get_shown (tool_item));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_item_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayToolItem *tool_item = CLAY_TOOL_ITEM (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      clay_tool_item_set_visible (tool_item, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
clay_tool_item_set_visible (ClayToolItem *tool_item,
                            gboolean      visible)
{
  g_return_if_fail (CLAY_IS_TOOL_ITEM (tool_item));

  if (visible != tool_item->priv->visible)
    {
      gboolean old_shown;

      g_object_freeze_notify (G_OBJECT (tool_item));

      old_shown = clay_tool_item_get_shown (tool_item);

      tool_item->priv->visible = visible;

      g_signal_emit (tool_item, clay_tool_item_signals[VISIBLE_CHANGED], 0);

      if (clay_tool_item_get_shown (tool_item) != old_shown)
        clay_tool_item_shown_changed (tool_item);

      g_object_notify (G_OBJECT (tool_item), "visible");

      g_object_thaw_notify (G_OBJECT (tool_item));
    }
}

gboolean
clay_tool_item_get_visible (ClayToolItem *tool_item)
{
  g_return_val_if_fail (CLAY_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->visible;
}

gboolean
clay_tool_item_get_shown (ClayToolItem *tool_item)
{
  ClayToolItem *parent;

  g_return_val_if_fail (CLAY_IS_TOOL_ITEM (tool_item), FALSE);

  parent = CLAY_TOOL_ITEM (
    clay_viewable_get_parent (CLAY_VIEWABLE (tool_item)));

  return tool_item->priv->visible &&
         (! parent || clay_tool_item_get_shown (parent));
}


/*  protected functions  */

void
clay_tool_item_shown_changed (ClayToolItem *tool_item)
{
  g_signal_emit (tool_item, clay_tool_item_signals[SHOWN_CHANGED], 0);

  g_object_notify (G_OBJECT (tool_item), "shown");
}
