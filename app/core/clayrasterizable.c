/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrasterizable.c
 * Copyright (C) 2025 Jehan
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

#include "core-types.h"

#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitemtree.h"
#include "clayrasterizable.h"

#include "clay-intl.h"


enum
{
  SET_RASTERIZED,
  LAST_SIGNAL
};


typedef struct _ClayRasterizablePrivate ClayRasterizablePrivate;

struct _ClayRasterizablePrivate
{
  gboolean rasterized;
  gboolean auto_rename;
};

#define CLAY_RASTERIZABLE_GET_PRIVATE(obj) (clay_rasterizable_get_private ((ClayRasterizable *) (obj)))


static ClayRasterizablePrivate * clay_rasterizable_get_private      (ClayRasterizable        *rasterizable);
static void                      clay_rasterizable_private_finalize (ClayRasterizablePrivate *private);



G_DEFINE_INTERFACE (ClayRasterizable, clay_rasterizable, CLAY_TYPE_DRAWABLE)


static guint clay_rasterizable_signals[LAST_SIGNAL] = { 0, };


static void
clay_rasterizable_default_init (ClayRasterizableInterface *iface)
{
  clay_rasterizable_signals[SET_RASTERIZED] =
    g_signal_new ("set-rasterized",
                  CLAY_TYPE_RASTERIZABLE,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayRasterizableInterface, set_rasterized),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
}


/* Public functions */


/**
 * clay_rasterizable_rasterize:
 * @rasterizable: an object that implements the %ClayRasterizable interface
 *
 * Rasterize @rasterizable.
 **/
void
clay_rasterizable_rasterize (ClayRasterizable *rasterizable,
                             gboolean          push_undo)
{
  ClayRasterizablePrivate *private;
  ClayImage               *image;

  g_return_if_fail (CLAY_IS_RASTERIZABLE (rasterizable));
  g_return_if_fail (! clay_rasterizable_is_rasterized (rasterizable));

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);
  image   = clay_item_get_image (CLAY_ITEM (rasterizable));

  if (push_undo)
    {
      gchar *undo_text;

      /* TRANSLATORS: the %s will be a type of item, i.e. "Text Layer". */
      undo_text = g_strdup_printf (_("Rasterize %s"),
                                   CLAY_VIEWABLE_GET_CLASS (rasterizable)->default_name);
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_PROPERTIES, undo_text);
      clay_image_undo_push_rasterizable (image, NULL, rasterizable);

      g_free (undo_text);
    }

  private->rasterized = TRUE;
  g_signal_emit (rasterizable, clay_rasterizable_signals[SET_RASTERIZED], 0, TRUE);

  if (push_undo)
    clay_image_undo_group_end (image);

  /* Triggers thumbnail update. */
  clay_drawable_update_all (CLAY_DRAWABLE (rasterizable));
  /* Triggers contextual menu update. */
  clay_image_flush (image);
}

/**
 * clay_rasterizable_restore:
 * @rasterizable: an object that implements the %ClayRasterizable interface
 *
 * Revert the rasterization of @rasterizable.
 **/
void
clay_rasterizable_restore (ClayRasterizable *rasterizable)
{
  ClayRasterizablePrivate *private;
  ClayImage               *image;
  gchar                   *undo_text;

  g_return_if_fail (CLAY_IS_RASTERIZABLE (rasterizable));
  g_return_if_fail (clay_rasterizable_is_rasterized (rasterizable));

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);
  image   = clay_item_get_image (CLAY_ITEM (rasterizable));

  /* TRANSLATORS: the %s will be a type of item, i.e. "Text Layer". */
  undo_text = g_strdup_printf (_("Revert Rasterize %s"),
                               CLAY_VIEWABLE_GET_CLASS (rasterizable)->default_name);
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_PROPERTIES, undo_text);
  clay_image_undo_push_rasterizable (image, NULL, rasterizable);
  clay_image_undo_push_drawable_mod (image, NULL, CLAY_DRAWABLE (rasterizable), TRUE);

  private->rasterized = FALSE;
  g_signal_emit (rasterizable, clay_rasterizable_signals[SET_RASTERIZED], 0, FALSE);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
  g_free (undo_text);
}

/**
 * clay_rasterizable_is_rasterized:
 * @rasterizable: an object that implements the %ClayRasterizable interface
 *
 * Returns: whether @rasterizable is rasterized or not.
 **/
gboolean
clay_rasterizable_is_rasterized (ClayRasterizable *rasterizable)
{
  ClayRasterizablePrivate *private;

  g_return_val_if_fail (CLAY_IS_RASTERIZABLE (rasterizable), FALSE);

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);

  return private->rasterized;
}

void
clay_rasterizable_set_auto_rename (ClayRasterizable *rasterizable,
                                   gboolean          auto_rename)
{
  ClayRasterizablePrivate *private;

  g_return_if_fail (CLAY_IS_RASTERIZABLE (rasterizable));

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);
  private->auto_rename = auto_rename;
}

gboolean
clay_rasterizable_get_auto_rename (ClayRasterizable *rasterizable)
{
  ClayRasterizablePrivate *private;

  g_return_val_if_fail (CLAY_IS_RASTERIZABLE (rasterizable), TRUE);

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);

  return private->auto_rename;
}

gboolean
clay_rasterizable_rename (ClayItem     *item,
                          const gchar  *new_name,
                          const gchar  *undo_desc,
                          GError      **error)
{
  GTypeClass *klass;
  GTypeClass *parent_class;

  klass        = g_type_class_peek (G_TYPE_FROM_INSTANCE (item));
  parent_class = g_type_class_peek_parent (klass);
  if (CLAY_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error))
    {
      clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (item), FALSE);

      return TRUE;
    }

  return FALSE;
}

void
clay_rasterizable_auto_rename (ClayRasterizable *rasterizable,
                               ClayObject       *rename_data,
                               const gchar      *rename_string)
{
  if (clay_rasterizable_get_auto_rename (rasterizable))
    {
      ClayItem *item = CLAY_ITEM (rasterizable);
      gchar    *name = NULL;

      if (rename_data)
        name = g_strdup (clay_object_get_name (rename_data));
      else if (rename_string)
        name = g_strdup (rename_string);

      if (! name || ! name[0])
        {
          g_free (name);
          name = g_strdup (CLAY_VIEWABLE_GET_CLASS (rasterizable)->default_name);
        }

      if (clay_item_is_attached (item))
        {
          clay_item_tree_rename_item (clay_item_get_tree (item), item,
                                      name, FALSE, NULL);
          g_free (name);
        }
      else
        {
          clay_object_take_name (CLAY_OBJECT (rasterizable), name);
        }
    }
}

void
clay_rasterizable_set_undo_rasterized (ClayRasterizable *rasterizable,
                                       gboolean          rasterized)
{
  ClayRasterizablePrivate *private;

  g_return_if_fail (CLAY_IS_RASTERIZABLE (rasterizable));

  private = CLAY_RASTERIZABLE_GET_PRIVATE (rasterizable);

  private->rasterized = rasterized;
}

/* Private functions */

static ClayRasterizablePrivate *
clay_rasterizable_get_private (ClayRasterizable *rasterizable)
{
  ClayRasterizablePrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (CLAY_IS_RASTERIZABLE (rasterizable), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("clay-rasterizable-private");

  private = g_object_get_qdata ((GObject *) rasterizable, private_key);

  if (! private)
    {
      private = g_slice_new0 (ClayRasterizablePrivate);

      private->rasterized  = FALSE;
      private->auto_rename = TRUE;

      g_object_set_qdata_full ((GObject *) rasterizable, private_key, private,
                               (GDestroyNotify) clay_rasterizable_private_finalize);
    }

  return private;
}

static void
clay_rasterizable_private_finalize (ClayRasterizablePrivate *private)
{
  g_slice_free (ClayRasterizablePrivate, private);
}
