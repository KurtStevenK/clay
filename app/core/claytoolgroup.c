/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolgroup.c
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

#include "claylist.h"
#include "claytoolgroup.h"
#include "claytoolinfo.h"

#include "clay-intl.h"


enum
{
  ACTIVE_TOOL_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACTIVE_TOOL,
  PROP_CHILDREN
};


struct _ClayToolGroupPrivate
{
  gchar         *active_tool;
  ClayContainer *children;
};


/*  local function prototypes  */


static void            clay_tool_group_finalize        (GObject        *object);
static void            clay_tool_group_get_property    (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void            clay_tool_group_set_property    (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);

static gint64          clay_tool_group_get_memsize     (ClayObject     *object,
                                                        gint64         *gui_size);

static gchar         * clay_tool_group_get_description (ClayViewable   *viewable,
                                                        gchar         **tooltip);
static ClayContainer * clay_tool_group_get_children    (ClayViewable   *viewable);
static void            clay_tool_group_set_expanded    (ClayViewable   *viewable,
                                                        gboolean        expand);
static gboolean        clay_tool_group_get_expanded    (ClayViewable   *viewable);

static void            clay_tool_group_child_add       (ClayContainer  *container,
                                                        ClayToolInfo   *tool_info,
                                                        ClayToolGroup  *tool_group);
static void            clay_tool_group_child_remove    (ClayContainer  *container,
                                                        ClayToolInfo   *tool_info,
                                                        ClayToolGroup  *tool_group);

static void            clay_tool_group_shown_changed   (ClayToolItem   *tool_item);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolGroup, clay_tool_group, CLAY_TYPE_TOOL_ITEM)

#define parent_class clay_tool_group_parent_class

static guint clay_tool_group_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */

static void
clay_tool_group_class_init (ClayToolGroupClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayToolItemClass *tool_item_class   = CLAY_TOOL_ITEM_CLASS (klass);

  clay_tool_group_signals[ACTIVE_TOOL_CHANGED] =
    g_signal_new ("active-tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayToolGroupClass, active_tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = clay_tool_group_finalize;
  object_class->get_property        = clay_tool_group_get_property;
  object_class->set_property        = clay_tool_group_set_property;

  clay_object_class->get_memsize    = clay_tool_group_get_memsize;

  viewable_class->default_icon_name = "folder";
  viewable_class->get_description   = clay_tool_group_get_description;
  viewable_class->get_children      = clay_tool_group_get_children;
  viewable_class->get_expanded      = clay_tool_group_get_expanded;
  viewable_class->set_expanded      = clay_tool_group_set_expanded;

  tool_item_class->shown_changed    = clay_tool_group_shown_changed;

  CLAY_CONFIG_PROP_STRING (object_class, PROP_ACTIVE_TOOL,
                           "active-tool", NULL, NULL,
                           NULL,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_CHILDREN,
                           "children", NULL, NULL,
                           CLAY_TYPE_CONTAINER,
                           CLAY_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_AGGREGATE);
}

static void
clay_tool_group_init (ClayToolGroup *tool_group)
{
  tool_group->priv = clay_tool_group_get_instance_private (tool_group);

  tool_group->priv->children = g_object_new (CLAY_TYPE_LIST,
                                             "child-type", CLAY_TYPE_TOOL_INFO,
                                             "append",     TRUE,
                                             NULL);

  g_signal_connect (tool_group->priv->children, "add",
                    G_CALLBACK (clay_tool_group_child_add),
                    tool_group);
  g_signal_connect (tool_group->priv->children, "remove",
                    G_CALLBACK (clay_tool_group_child_remove),
                    tool_group);
}

static void
clay_tool_group_finalize (GObject *object)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (object);

  g_clear_pointer (&tool_group->priv->active_tool, g_free);

  g_clear_object (&tool_group->priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_tool_group_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (object);

  switch (property_id)
    {
    case PROP_ACTIVE_TOOL:
      g_value_set_string (value, tool_group->priv->active_tool);
      break;

    case PROP_CHILDREN:
      g_value_set_object (value, tool_group->priv->children);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_group_set_property_add_tool (ClayToolInfo  *tool_info,
                                       ClayToolGroup *tool_group)
{
  clay_container_add (tool_group->priv->children, CLAY_OBJECT (tool_info));
}

static void
clay_tool_group_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (object);

  switch (property_id)
    {
    case PROP_ACTIVE_TOOL:
      g_set_str (&tool_group->priv->active_tool, g_value_get_string (value));
      break;

    case PROP_CHILDREN:
      {
        ClayContainer *container = g_value_get_object (value);

        clay_container_clear (tool_group->priv->children);

        if (! container)
          break;

        clay_container_foreach (container,
                                (GFunc) clay_tool_group_set_property_add_tool,
                                tool_group);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_tool_group_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (object);
  gint64         memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (tool_group->priv->children),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
clay_tool_group_get_description (ClayViewable  *viewable,
                                 gchar        **tooltip)
{
  /* Translators: this is a noun */
  return g_strdup (C_("tool-item", "Group"));
}

static ClayContainer *
clay_tool_group_get_children (ClayViewable *viewable)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (viewable);

  return tool_group->priv->children;
}

static void
clay_tool_group_set_expanded (ClayViewable *viewable,
                              gboolean      expand)
{
  if (! expand)
    clay_viewable_expanded_changed (viewable);
}

static gboolean
clay_tool_group_get_expanded (ClayViewable *viewable)
{
  return TRUE;
}

static void
clay_tool_group_child_add (ClayContainer *container,
                           ClayToolInfo  *tool_info,
                           ClayToolGroup *tool_group)
{
  g_return_if_fail (
    clay_viewable_get_parent (CLAY_VIEWABLE (tool_info)) == NULL);

  clay_viewable_set_parent (CLAY_VIEWABLE (tool_info),
                            CLAY_VIEWABLE (tool_group));

  if (! tool_group->priv->active_tool)
    clay_tool_group_set_active_tool_info (tool_group, tool_info);
}

static void
clay_tool_group_child_remove (ClayContainer *container,
                              ClayToolInfo  *tool_info,
                              ClayToolGroup *tool_group)
{
  clay_viewable_set_parent (CLAY_VIEWABLE (tool_info), NULL);

  if (! g_strcmp0 (tool_group->priv->active_tool,
                   clay_object_get_name (CLAY_OBJECT (tool_info))))
    {
      ClayToolInfo *active_tool_info = NULL;

      if (! clay_container_is_empty (tool_group->priv->children))
        {
          active_tool_info = CLAY_TOOL_INFO (
            clay_container_get_first_child (tool_group->priv->children));
        }

      clay_tool_group_set_active_tool_info (tool_group, active_tool_info);
    }
}

static void
clay_tool_group_shown_changed (ClayToolItem *tool_item)
{
  ClayToolGroup *tool_group = CLAY_TOOL_GROUP (tool_item);
  GList         *iter;

  if (CLAY_TOOL_ITEM_CLASS (parent_class)->shown_changed)
    CLAY_TOOL_ITEM_CLASS (parent_class)->shown_changed (tool_item);

  for (iter = CLAY_LIST (tool_group->priv->children)->queue->head;
       iter;
       iter = g_list_next (iter))
    {
      ClayToolItem *tool_item = iter->data;

      if (clay_tool_item_get_visible (tool_item))
        clay_tool_item_shown_changed (tool_item);
    }
}


/*  public functions  */

ClayToolGroup *
clay_tool_group_new (void)
{
  ClayToolGroup *tool_group;

  tool_group = g_object_new (CLAY_TYPE_TOOL_GROUP, NULL);

  clay_object_set_static_name (CLAY_OBJECT (tool_group), "tool group");

  return tool_group;
}

void
clay_tool_group_set_active_tool (ClayToolGroup *tool_group,
                                 const gchar   *tool_name)
{
  g_return_if_fail (CLAY_IS_TOOL_GROUP (tool_group));

  if (g_strcmp0 (tool_group->priv->active_tool, tool_name))
    {
      g_return_if_fail (tool_name == NULL ||
                        clay_container_get_child_by_name (
                          tool_group->priv->children, tool_name) != NULL);

      g_free (tool_group->priv->active_tool);

      tool_group->priv->active_tool = g_strdup (tool_name);;

      g_signal_emit (tool_group,
                     clay_tool_group_signals[ACTIVE_TOOL_CHANGED], 0);

      g_object_notify (G_OBJECT (tool_group), "active-tool");
    }
}

const gchar *
clay_tool_group_get_active_tool (ClayToolGroup *tool_group)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GROUP (tool_group), NULL);

  return tool_group->priv->active_tool;
}

void
clay_tool_group_set_active_tool_info (ClayToolGroup *tool_group,
                                      ClayToolInfo  *tool_info)
{
  g_return_if_fail (CLAY_IS_TOOL_GROUP (tool_group));
  g_return_if_fail (tool_info == NULL || CLAY_IS_TOOL_INFO (tool_info));

  clay_tool_group_set_active_tool (
    tool_group,
    tool_info ? clay_object_get_name (CLAY_OBJECT (tool_info)) : NULL);
}

ClayToolInfo *
clay_tool_group_get_active_tool_info (ClayToolGroup *tool_group)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GROUP (tool_group), NULL);

  return CLAY_TOOL_INFO (
    clay_container_get_child_by_name (tool_group->priv->children,
                                      tool_group->priv->active_tool));
}
