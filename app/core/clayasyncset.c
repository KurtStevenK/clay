/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayasyncset.c
 * Copyright (C) 2018 Ell
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

#include "clayasync.h"
#include "clayasyncset.h"
#include "claycancelable.h"
#include "claywaitable.h"


enum
{
  PROP_0,
  PROP_EMPTY
};


struct _ClayAsyncSetPrivate
{
  GHashTable *asyncs;
};


/*  local function prototypes  */

static void       clay_async_set_waitable_iface_init   (ClayWaitableInterface   *iface);

static void       clay_async_set_cancelable_iface_init (ClayCancelableInterface *iface);

static void       clay_async_set_dispose               (GObject                 *object);
static void       clay_async_set_finalize              (GObject                 *object);
static void       clay_async_set_set_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        const GValue            *value,
                                                        GParamSpec              *pspec);
static void       clay_async_set_get_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        GValue                  *value,
                                                        GParamSpec              *pspec);

static void       clay_async_set_wait                  (ClayWaitable            *waitable);
static gboolean   clay_async_set_try_wait              (ClayWaitable            *waitable);
static gboolean   clay_async_set_wait_until            (ClayWaitable            *waitable,
                                                        gint64                   end_time);

static void       clay_async_set_cancel                (ClayCancelable          *cancelable);

static void       clay_async_set_async_callback        (ClayAsync               *async,
                                                        ClayAsyncSet            *async_set);


G_DEFINE_TYPE_WITH_CODE (ClayAsyncSet, clay_async_set, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayAsyncSet)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_WAITABLE,
                                                clay_async_set_waitable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CANCELABLE,
                                                clay_async_set_cancelable_iface_init))

#define parent_class clay_async_set_parent_class


/*  private functions  */


static void
clay_async_set_class_init (ClayAsyncSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_async_set_dispose;
  object_class->finalize     = clay_async_set_finalize;
  object_class->set_property = clay_async_set_set_property;
  object_class->get_property = clay_async_set_get_property;

  g_object_class_install_property (object_class, PROP_EMPTY,
                                   g_param_spec_boolean ("empty",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READABLE));
}

static void
clay_async_set_waitable_iface_init (ClayWaitableInterface *iface)
{
  iface->wait       = clay_async_set_wait;
  iface->try_wait   = clay_async_set_try_wait;
  iface->wait_until = clay_async_set_wait_until;
}

static void
clay_async_set_cancelable_iface_init (ClayCancelableInterface *iface)
{
  iface->cancel = clay_async_set_cancel;
}

static void
clay_async_set_init (ClayAsyncSet *async_set)
{
  async_set->priv = clay_async_set_get_instance_private (async_set);

  async_set->priv->asyncs = g_hash_table_new (NULL, NULL);
}

static void
clay_async_set_dispose (GObject *object)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (object);

  clay_async_set_clear (async_set);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_async_set_finalize (GObject *object)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (object);

  g_hash_table_unref (async_set->priv->asyncs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_async_set_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_async_set_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (object);

  switch (property_id)
    {
    case PROP_EMPTY:
      g_value_set_boolean (value, clay_async_set_is_empty (async_set));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_async_set_wait (ClayWaitable *waitable)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (waitable);

  while (! clay_async_set_is_empty (async_set))
    {
      ClayAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      clay_waitable_wait (CLAY_WAITABLE (async));
    }
}

static gboolean
clay_async_set_try_wait (ClayWaitable *waitable)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (waitable);

  while (! clay_async_set_is_empty (async_set))
    {
      ClayAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! clay_waitable_try_wait (CLAY_WAITABLE (async)))
        return FALSE;
    }

  return TRUE;
}

static gboolean
clay_async_set_wait_until (ClayWaitable *waitable,
                           gint64        end_time)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (waitable);

  while (! clay_async_set_is_empty (async_set))
    {
      ClayAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! clay_waitable_wait_until (CLAY_WAITABLE (async), end_time))
        return FALSE;
    }

  return TRUE;
}

static void
clay_async_set_cancel (ClayCancelable *cancelable)
{
  ClayAsyncSet *async_set = CLAY_ASYNC_SET (cancelable);
  GList        *list;

  list = g_hash_table_get_keys (async_set->priv->asyncs);

  g_list_foreach (list, (GFunc) g_object_ref, NULL);

  g_list_foreach (list, (GFunc) clay_cancelable_cancel, NULL);

  g_list_free_full (list, g_object_unref);
}

static void
clay_async_set_async_callback (ClayAsync    *async,
                               ClayAsyncSet *async_set)
{
  g_hash_table_remove (async_set->priv->asyncs, async);

  if (clay_async_set_is_empty (async_set))
    g_object_notify (G_OBJECT (async_set), "empty");
}


/*  public functions  */


ClayAsyncSet *
clay_async_set_new (void)
{
  return g_object_new (CLAY_TYPE_ASYNC_SET,
                       NULL);
}

void
clay_async_set_add (ClayAsyncSet *async_set,
                    ClayAsync    *async)
{
  g_return_if_fail (CLAY_IS_ASYNC_SET (async_set));
  g_return_if_fail (CLAY_IS_ASYNC (async));

  if (g_hash_table_add (async_set->priv->asyncs, async))
    {
      if (g_hash_table_size (async_set->priv->asyncs) == 1)
        g_object_notify (G_OBJECT (async_set), "empty");

      clay_async_add_callback (
        async,
        (ClayAsyncCallback) clay_async_set_async_callback,
        async_set);
    }
}

void
clay_async_set_remove (ClayAsyncSet *async_set,
                       ClayAsync    *async)
{
  g_return_if_fail (CLAY_IS_ASYNC_SET (async_set));
  g_return_if_fail (CLAY_IS_ASYNC (async));

  if (g_hash_table_remove (async_set->priv->asyncs, async))
    {
      clay_async_remove_callback (
        async,
        (ClayAsyncCallback) clay_async_set_async_callback,
        async_set);

      if (g_hash_table_size (async_set->priv->asyncs) == 0)
        g_object_notify (G_OBJECT (async_set), "empty");
    }
}

void
clay_async_set_clear (ClayAsyncSet *async_set)
{
  ClayAsync      *async;
  GHashTableIter  iter;

  g_return_if_fail (CLAY_IS_ASYNC_SET (async_set));

  if (clay_async_set_is_empty (async_set))
    return;

  g_hash_table_iter_init (&iter, async_set->priv->asyncs);

  while (g_hash_table_iter_next (&iter, (gpointer *) &async, NULL))
    {
      clay_async_remove_callback (
        async,
        (ClayAsyncCallback) clay_async_set_async_callback,
        async_set);
    }

  g_hash_table_remove_all (async_set->priv->asyncs);

  g_object_notify (G_OBJECT (async_set), "empty");
}

gboolean
clay_async_set_is_empty (ClayAsyncSet *async_set)
{
  g_return_val_if_fail (CLAY_IS_ASYNC_SET (async_set), FALSE);

  return g_hash_table_size (async_set->priv->asyncs) == 0;
}
