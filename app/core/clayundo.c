/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <time.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "clay.h"
#include "claycontext.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "claymarshal.h"
#include "claytempbuf.h"
#include "clayundo.h"
#include "clayundostack.h"

#include "clay-priorities.h"

#include "clay-intl.h"


enum
{
  POP,
  FREE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_TIME,
  PROP_UNDO_TYPE,
  PROP_DIRTY_MASK
};


static void          clay_undo_constructed         (GObject             *object);
static void          clay_undo_finalize            (GObject             *object);
static void          clay_undo_set_property        (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void          clay_undo_get_property        (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static gint64        clay_undo_get_memsize         (ClayObject          *object,
                                                    gint64              *gui_size);

static gboolean      clay_undo_get_popup_size      (ClayViewable        *viewable,
                                                    gint                 width,
                                                    gint                 height,
                                                    gboolean             dot_for_dot,
                                                    gint                *popup_width,
                                                    gint                *popup_height);
static ClayTempBuf * clay_undo_get_new_preview     (ClayViewable        *viewable,
                                                    ClayContext         *context,
                                                    gint                 width,
                                                    gint                 height,
                                                    gint                 scale_factor,
                                                    GeglColor           *fg_color);

static void          clay_undo_real_pop            (ClayUndo            *undo,
                                                    ClayUndoMode         undo_mode,
                                                    ClayUndoAccumulator *accum);
static void          clay_undo_real_free           (ClayUndo            *undo,
                                                    ClayUndoMode         undo_mode);

static gboolean      clay_undo_create_preview_idle (gpointer             data);
static void       clay_undo_create_preview_private (ClayUndo            *undo,
                                                    ClayContext         *context);


G_DEFINE_TYPE (ClayUndo, clay_undo, CLAY_TYPE_VIEWABLE)

#define parent_class clay_undo_parent_class

static guint undo_signals[LAST_SIGNAL] = { 0 };


static void
clay_undo_class_init (ClayUndoClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);

  undo_signals[POP] =
    g_signal_new ("pop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayUndoClass, pop),
                  NULL, NULL,
                  clay_marshal_VOID__ENUM_POINTER,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_UNDO_MODE,
                  G_TYPE_POINTER);

  undo_signals[FREE] =
    g_signal_new ("free",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayUndoClass, free),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_UNDO_MODE);

  object_class->constructed         = clay_undo_constructed;
  object_class->finalize            = clay_undo_finalize;
  object_class->set_property        = clay_undo_set_property;
  object_class->get_property        = clay_undo_get_property;

  clay_object_class->get_memsize    = clay_undo_get_memsize;

  viewable_class->default_icon_name = "edit-undo";
  viewable_class->get_popup_size    = clay_undo_get_popup_size;
  viewable_class->get_new_preview   = clay_undo_get_new_preview;

  klass->pop                        = clay_undo_real_pop;
  klass->free                       = clay_undo_real_free;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TIME,
                                   g_param_spec_uint ("time", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_UNDO_TYPE,
                                   g_param_spec_enum ("undo-type", NULL, NULL,
                                                      CLAY_TYPE_UNDO_TYPE,
                                                      CLAY_UNDO_GROUP_NONE,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIRTY_MASK,
                                   g_param_spec_flags ("dirty-mask",
                                                       NULL, NULL,
                                                       CLAY_TYPE_DIRTY_MASK,
                                                       CLAY_DIRTY_NONE,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_undo_init (ClayUndo *undo)
{
  undo->time = time (NULL);
}

static void
clay_undo_constructed (GObject *object)
{
  ClayUndo *undo = CLAY_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_IMAGE (undo->image));
}

static void
clay_undo_finalize (GObject *object)
{
  ClayUndo *undo = CLAY_UNDO (object);

  if (undo->preview_idle_id)
    {
      g_source_remove (undo->preview_idle_id);
      undo->preview_idle_id = 0;
    }

  g_clear_pointer (&undo->preview, clay_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_undo_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayUndo *undo = CLAY_UNDO (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      /* don't ref */
      undo->image = g_value_get_object (value);
      break;
    case PROP_TIME:
      undo->time = g_value_get_uint (value);
      break;
    case PROP_UNDO_TYPE:
      undo->undo_type = g_value_get_enum (value);
      break;
    case PROP_DIRTY_MASK:
      undo->dirty_mask = g_value_get_flags (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_undo_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  ClayUndo *undo = CLAY_UNDO (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, undo->image);
      break;
    case PROP_TIME:
      g_value_set_uint (value, undo->time);
      break;
    case PROP_UNDO_TYPE:
      g_value_set_enum (value, undo->undo_type);
      break;
    case PROP_DIRTY_MASK:
      g_value_set_flags (value, undo->dirty_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_undo_get_memsize (ClayObject *object,
                       gint64     *gui_size)
{
  ClayUndo *undo    = CLAY_UNDO (object);
  gint64    memsize = 0;

  *gui_size += clay_temp_buf_get_memsize (undo->preview);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_undo_get_popup_size (ClayViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  ClayUndo *undo = CLAY_UNDO (viewable);

  if (undo->preview &&
      (clay_temp_buf_get_width  (undo->preview) > width ||
       clay_temp_buf_get_height (undo->preview) > height))
    {
      *popup_width  = clay_temp_buf_get_width  (undo->preview);
      *popup_height = clay_temp_buf_get_height (undo->preview);

      return TRUE;
    }

  return FALSE;
}

static ClayTempBuf *
clay_undo_get_new_preview (ClayViewable *viewable,
                           ClayContext  *context,
                           gint          width,
                           gint          height,
                           gint          scale_factor,
                           GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayUndo *undo = CLAY_UNDO (viewable);

  if (undo->preview)
    {
      gint preview_width;
      gint preview_height;

      width  *= scale_factor;
      height *= scale_factor;

      clay_viewable_calc_preview_size (clay_temp_buf_get_width  (undo->preview),
                                       clay_temp_buf_get_height (undo->preview),
                                       width,
                                       height,
                                       TRUE, 1.0, 1.0,
                                       &preview_width,
                                       &preview_height,
                                       NULL);

      if (preview_width  < clay_temp_buf_get_width  (undo->preview) &&
          preview_height < clay_temp_buf_get_height (undo->preview))
        {
          return clay_temp_buf_scale (undo->preview,
                                      preview_width, preview_height);
        }

      return clay_temp_buf_copy (undo->preview);
    }

  return NULL;
}

static void
clay_undo_real_pop (ClayUndo            *undo,
                    ClayUndoMode         undo_mode,
                    ClayUndoAccumulator *accum)
{
}

static void
clay_undo_real_free (ClayUndo     *undo,
                     ClayUndoMode  undo_mode)
{
}

void
clay_undo_pop (ClayUndo            *undo,
               ClayUndoMode         undo_mode,
               ClayUndoAccumulator *accum)
{
  g_return_if_fail (CLAY_IS_UNDO (undo));
  g_return_if_fail (accum != NULL);

  if (undo->dirty_mask != CLAY_DIRTY_NONE)
    {
      switch (undo_mode)
        {
        case CLAY_UNDO_MODE_UNDO:
          clay_image_clean (undo->image, undo->dirty_mask);
          break;

        case CLAY_UNDO_MODE_REDO:
          clay_image_dirty (undo->image, undo->dirty_mask);
          break;
        }
    }

  g_signal_emit (undo, undo_signals[POP], 0, undo_mode, accum);
}

void
clay_undo_free (ClayUndo     *undo,
                ClayUndoMode  undo_mode)
{
  g_return_if_fail (CLAY_IS_UNDO (undo));

  g_signal_emit (undo, undo_signals[FREE], 0, undo_mode);
}

typedef struct _ClayUndoIdle ClayUndoIdle;

struct _ClayUndoIdle
{
  ClayUndo    *undo;
  ClayContext *context;
};

static void
clay_undo_idle_free (ClayUndoIdle *idle)
{
  if (idle->context)
    g_object_unref (idle->context);

  g_slice_free (ClayUndoIdle, idle);
}

void
clay_undo_create_preview (ClayUndo    *undo,
                          ClayContext *context,
                          gboolean     create_now)
{
  g_return_if_fail (CLAY_IS_UNDO (undo));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (undo->preview || undo->preview_idle_id)
    return;

  if (create_now)
    {
      clay_undo_create_preview_private (undo, context);
    }
  else
    {
      ClayUndoIdle *idle = g_slice_new0 (ClayUndoIdle);

      idle->undo = undo;

      if (context)
        idle->context = g_object_ref (context);

      undo->preview_idle_id =
        g_idle_add_full (CLAY_PRIORITY_VIEWABLE_IDLE,
                         clay_undo_create_preview_idle, idle,
                         (GDestroyNotify) clay_undo_idle_free);
    }
}

static gboolean
clay_undo_create_preview_idle (gpointer data)
{
  ClayUndoIdle *idle   = data;
  ClayUndoStack *stack = clay_image_get_undo_stack (idle->undo->image);

  if (idle->undo == clay_undo_stack_peek (stack))
    {
      clay_undo_create_preview_private (idle->undo, idle->context);
    }

  idle->undo->preview_idle_id = 0;

  return FALSE;
}

static void
clay_undo_create_preview_private (ClayUndo    *undo,
                                  ClayContext *context)
{
  ClayImage    *image = undo->image;
  ClayViewable *preview_viewable;
  ClayViewSize  preview_size;
  gint          width;
  gint          height;

  switch (undo->undo_type)
    {
    case CLAY_UNDO_GROUP_IMAGE_QUICK_MASK:
    case CLAY_UNDO_GROUP_MASK:
    case CLAY_UNDO_MASK:
      preview_viewable = CLAY_VIEWABLE (clay_image_get_mask (image));
      break;

    default:
      preview_viewable = CLAY_VIEWABLE (image);
      break;
    }

  preview_size = image->clay->config->undo_preview_size;

  if (clay_image_get_width  (image) <= preview_size &&
      clay_image_get_height (image) <= preview_size)
    {
      width  = clay_image_get_width  (image);
      height = clay_image_get_height (image);
    }
  else
    {
      if (clay_image_get_width (image) > clay_image_get_height (image))
        {
          width  = preview_size;
          height = MAX (1, (clay_image_get_height (image) * preview_size /
                            clay_image_get_width (image)));
        }
      else
        {
          height = preview_size;
          width  = MAX (1, (clay_image_get_width (image) * preview_size /
                            clay_image_get_height (image)));
        }
    }

  undo->preview = clay_viewable_get_new_preview (preview_viewable, context,
                                                 width, height,
                                                 2 /* better than 1 */,
                                                 NULL);

  clay_viewable_invalidate_preview (CLAY_VIEWABLE (undo));
}

void
clay_undo_refresh_preview (ClayUndo    *undo,
                           ClayContext *context)
{
  g_return_if_fail (CLAY_IS_UNDO (undo));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (undo->preview_idle_id)
    return;

  if (undo->preview)
    {
      g_clear_pointer (&undo->preview, clay_temp_buf_unref);
      clay_undo_create_preview (undo, context, FALSE);
    }
}

const gchar *
clay_undo_type_to_name (ClayUndoType type)
{
  const gchar *desc;

  if (clay_enum_get_value (CLAY_TYPE_UNDO_TYPE, type, NULL, NULL, &desc, NULL))
    return desc;
  else
    return "";
}

gboolean
clay_undo_is_weak (ClayUndo *undo)
{
  if (! undo)
    return FALSE;

  switch (undo->undo_type)
    {
    case CLAY_UNDO_GROUP_ITEM_VISIBILITY:
    case CLAY_UNDO_GROUP_ITEM_PROPERTIES:
    case CLAY_UNDO_GROUP_LAYER_APPLY_MASK:
    case CLAY_UNDO_GROUP_FILTER_VISIBILITY:
    case CLAY_UNDO_ITEM_VISIBILITY:
    case CLAY_UNDO_LAYER_MODE:
    case CLAY_UNDO_LAYER_OPACITY:
    case CLAY_UNDO_LAYER_MASK_APPLY:
    case CLAY_UNDO_LAYER_MASK_SHOW:
    case CLAY_UNDO_FILTER_VISIBILITY:
      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}

/**
 * clay_undo_get_age:
 * @undo:
 *
 * Returns: the time in seconds since this undo item was created
 */
gint
clay_undo_get_age (ClayUndo *undo)
{
  guint now = time (NULL);

  g_return_val_if_fail (CLAY_IS_UNDO (undo), 0);
  g_return_val_if_fail (now >= undo->time, 0);

  return now - undo->time;
}

/**
 * clay_undo_reset_age:
 * @undo:
 *
 * Changes the creation time of this undo item to the current time.
 */
void
clay_undo_reset_age (ClayUndo *undo)
{
  g_return_if_fail (CLAY_IS_UNDO (undo));

  undo->time = time (NULL);

  g_object_notify (G_OBJECT (undo), "time");
}
