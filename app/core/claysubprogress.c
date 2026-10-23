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

#include <glib-object.h>

#include "core-types.h"

#include "core/claysubprogress.h"
#include "core/clayprogress.h"


enum
{
  PROP_0,
  PROP_PROGRESS
};


static void           clay_sub_progress_iface_init        (ClayProgressInterface *iface);

static void           clay_sub_progress_finalize          (GObject             *object);
static void           clay_sub_progress_set_property      (GObject             *object,
                                                           guint                property_id,
                                                           const GValue        *value,
                                                           GParamSpec          *pspec);
static void           clay_sub_progress_get_property      (GObject             *object,
                                                           guint                property_id,
                                                           GValue              *value,
                                                           GParamSpec          *pspec);

static ClayProgress * clay_sub_progress_start             (ClayProgress        *progress,
                                                           gboolean             cancellable,
                                                           const gchar         *message);
static void           clay_sub_progress_end               (ClayProgress        *progress);
static gboolean       clay_sub_progress_is_active         (ClayProgress        *progress);
static void           clay_sub_progress_set_text          (ClayProgress        *progress,
                                                           const gchar         *message);
static void           clay_sub_progress_set_value         (ClayProgress        *progress,
                                                           gdouble              percentage);
static gdouble        clay_sub_progress_get_value         (ClayProgress        *progress);
static void           clay_sub_progress_pulse             (ClayProgress        *progress);
static GBytes       * clay_sub_progress_get_window_handle (ClayProgress        *progress);
static gboolean       clay_sub_progress_message           (ClayProgress        *progress,
                                                           Clay                *clay,
                                                           ClayMessageSeverity  severity,
                                                           const gchar         *domain,
                                                           const gchar         *message);


G_DEFINE_TYPE_WITH_CODE (ClaySubProgress, clay_sub_progress, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_sub_progress_iface_init))

#define parent_class clay_sub_progress_parent_class


static void
clay_sub_progress_class_init (ClaySubProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_sub_progress_finalize;
  object_class->set_property = clay_sub_progress_set_property;
  object_class->get_property = clay_sub_progress_get_property;

  g_object_class_install_property (object_class, PROP_PROGRESS,
                                   g_param_spec_object ("progress",
                                                        NULL, NULL,
                                                        CLAY_TYPE_PROGRESS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_sub_progress_init (ClaySubProgress *sub)
{
  sub->progress = NULL;
  sub->start    = 0.0;
  sub->end      = 1.0;
}

static void
clay_sub_progress_finalize (GObject *object)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (object);

  g_clear_object (&sub->progress);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_sub_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start             = clay_sub_progress_start;
  iface->end               = clay_sub_progress_end;
  iface->is_active         = clay_sub_progress_is_active;
  iface->set_text          = clay_sub_progress_set_text;
  iface->set_value         = clay_sub_progress_set_value;
  iface->get_value         = clay_sub_progress_get_value;
  iface->pulse             = clay_sub_progress_pulse;
  iface->get_window_handle = clay_sub_progress_get_window_handle;
  iface->message           = clay_sub_progress_message;
}

static void
clay_sub_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      g_return_if_fail (sub->progress == NULL);
      sub->progress = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_sub_progress_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      g_value_set_object (value, sub->progress);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static ClayProgress *
clay_sub_progress_start (ClayProgress *progress,
                         gboolean      cancellable,
                         const gchar  *message)
{
  /* does nothing */
  return NULL;
}

static void
clay_sub_progress_end (ClayProgress *progress)
{
  /* does nothing */
}

static gboolean
clay_sub_progress_is_active (ClayProgress *progress)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    return clay_progress_is_active (sub->progress);

  return FALSE;
}

static void
clay_sub_progress_set_text (ClayProgress *progress,
                            const gchar  *message)
{
  /* does nothing */
}

static void
clay_sub_progress_set_value (ClayProgress *progress,
                             gdouble       percentage)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    clay_progress_set_value (sub->progress,
                             sub->start + percentage * (sub->end - sub->start));
}

static gdouble
clay_sub_progress_get_value (ClayProgress *progress)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    return clay_progress_get_value (sub->progress);

  return 0.0;
}

static void
clay_sub_progress_pulse (ClayProgress *progress)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    clay_progress_pulse (sub->progress);
}

static GBytes *
clay_sub_progress_get_window_handle (ClayProgress *progress)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    return clay_progress_get_window_handle (sub->progress);

  return NULL;
}

static gboolean
clay_sub_progress_message (ClayProgress        *progress,
                           Clay                *clay,
                           ClayMessageSeverity  severity,
                           const gchar         *domain,
                           const gchar         *message)
{
  ClaySubProgress *sub = CLAY_SUB_PROGRESS (progress);

  if (sub->progress)
    return clay_progress_message (sub->progress,
                                  clay, severity, domain, message);

  return FALSE;
}

/**
 * clay_sub_progress_new:
 * @progress: parent progress or %NULL
 *
 * ClaySubProgress implements the ClayProgress interface and can be
 * used wherever a ClayProgress is needed. It maps progress
 * information to a sub-range of its parent @progress. This is useful
 * when an action breaks down into multiple sub-actions that itself
 * need a #ClayProgress pointer. See clay_image_scale() for an example.
 *
 * Returns: a new #ClayProgress object
 */
ClayProgress *
clay_sub_progress_new (ClayProgress *progress)
{
  ClaySubProgress *sub;

  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);

  sub = g_object_new (CLAY_TYPE_SUB_PROGRESS,
                      "progress", progress,
                      NULL);

  return CLAY_PROGRESS (sub);
}

/**
 * clay_sub_progress_set_range:
 * @start: start value of range on the parent process
 * @end:   end value of range on the parent process
 *
 * Sets a range on the parent progress that this @progress should be
 * mapped to.
 */
void
clay_sub_progress_set_range (ClaySubProgress *progress,
                             gdouble          start,
                             gdouble          end)
{
  g_return_if_fail (CLAY_IS_SUB_PROGRESS (progress));
  g_return_if_fail (start < end);

  progress->start = start;
  progress->end   = end;
}

/**
 * clay_sub_progress_set_step:
 * @index:     step index
 * @num_steps: number of steps
 *
 * A more convenient form of clay_sub_progress_set_range().
 */
void
clay_sub_progress_set_step (ClaySubProgress *progress,
                            gint             index,
                            gint             num_steps)
{
  g_return_if_fail (CLAY_IS_SUB_PROGRESS (progress));
  g_return_if_fail (index < num_steps && num_steps > 0);

  progress->start = (gdouble) index / num_steps;
  progress->end   = (gdouble) (index + 1) / num_steps;
}
