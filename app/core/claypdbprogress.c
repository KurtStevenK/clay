/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbprogress.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "core-types.h"

#include "core/claycontext.h"

#include "pdb/claypdb.h"

#include "clay.h"
#include "clayparamspecs.h"
#include "claypdbprogress.h"
#include "clayprogress.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_PDB,
  PROP_CONTEXT,
  PROP_CALLBACK_NAME
};


static void           clay_pdb_progress_class_init                 (ClayPdbProgressClass  *klass);
static void           clay_pdb_progress_init                       (ClayPdbProgress       *progress,
                                                                    ClayPdbProgressClass  *klass);
static void           clay_pdb_progress_progress_iface_init        (ClayProgressInterface *iface);

static void           clay_pdb_progress_constructed                (GObject               *object);
static void           clay_pdb_progress_dispose                    (GObject               *object);
static void           clay_pdb_progress_finalize                   (GObject               *object);
static void           clay_pdb_progress_set_property               (GObject               *object,
                                                                    guint                  property_id,
                                                                    const GValue          *value,
                                                                    GParamSpec            *pspec);

static ClayProgress * clay_pdb_progress_progress_start             (ClayProgress          *progress,
                                                                    gboolean               cancellable,
                                                                    const gchar           *message);
static void           clay_pdb_progress_progress_end               (ClayProgress          *progress);
static gboolean       clay_pdb_progress_progress_is_active         (ClayProgress          *progress);
static void           clay_pdb_progress_progress_set_text          (ClayProgress          *progress,
                                                                    const gchar           *message);
static void           clay_pdb_progress_progress_set_value         (ClayProgress          *progress,
                                                                    gdouble                percentage);
static gdouble        clay_pdb_progress_progress_get_value         (ClayProgress          *progress);
static void           clay_pdb_progress_progress_pulse             (ClayProgress          *progress);
static GBytes       * clay_pdb_progress_progress_get_window_handle (ClayProgress          *progress);


static GObjectClass *parent_class = NULL;


GType
clay_pdb_progress_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayPdbProgressClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) clay_pdb_progress_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ClayPdbProgress),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) clay_pdb_progress_init,
      };

      const GInterfaceInfo progress_iface_info =
      {
        (GInterfaceInitFunc) clay_pdb_progress_progress_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "ClayPdbProgress",
                                     &info, 0);

      g_type_add_interface_static (type, CLAY_TYPE_PROGRESS,
                                   &progress_iface_info);
    }

  return type;
}

static void
clay_pdb_progress_class_init (ClayPdbProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = clay_pdb_progress_constructed;
  object_class->dispose      = clay_pdb_progress_dispose;
  object_class->finalize     = clay_pdb_progress_finalize;
  object_class->set_property = clay_pdb_progress_set_property;

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        CLAY_TYPE_PDB,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_pdb_progress_init (ClayPdbProgress      *progress,
                        ClayPdbProgressClass *klass)
{
  klass->progresses = g_list_prepend (klass->progresses, progress);
}

static void
clay_pdb_progress_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start             = clay_pdb_progress_progress_start;
  iface->end               = clay_pdb_progress_progress_end;
  iface->is_active         = clay_pdb_progress_progress_is_active;
  iface->set_text          = clay_pdb_progress_progress_set_text;
  iface->set_value         = clay_pdb_progress_progress_set_value;
  iface->get_value         = clay_pdb_progress_progress_get_value;
  iface->pulse             = clay_pdb_progress_progress_pulse;
  iface->get_window_handle = clay_pdb_progress_progress_get_window_handle;
}

static void
clay_pdb_progress_constructed (GObject *object)
{
  ClayPdbProgress *progress = CLAY_PDB_PROGRESS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_PDB (progress->pdb));
  clay_assert (CLAY_IS_CONTEXT (progress->context));
}

static void
clay_pdb_progress_dispose (GObject *object)
{
  ClayPdbProgressClass *klass = CLAY_PDB_PROGRESS_GET_CLASS (object);

  klass->progresses = g_list_remove (klass->progresses, object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_pdb_progress_finalize (GObject *object)
{
  ClayPdbProgress *progress = CLAY_PDB_PROGRESS (object);

  g_clear_object (&progress->pdb);
  g_clear_object (&progress->context);
  g_clear_pointer (&progress->callback_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pdb_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayPdbProgress *progress = CLAY_PDB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_set_object (&progress->pdb, g_value_get_object (value));
      break;
    case PROP_CONTEXT:
      g_set_object (&progress->context, g_value_get_object (value));
      break;
    case PROP_CALLBACK_NAME:
      g_set_str (&progress->callback_name, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_progress_run_callback (ClayPdbProgress      *progress,
                                ClayProgressCommand   command,
                                const gchar          *text,
                                gdouble               value,
                                GBytes              **handle)
{
  if (progress->callback_name && ! progress->callback_busy)
    {
      ClayValueArray *return_vals;

      progress->callback_busy = TRUE;

      return_vals =
        clay_pdb_execute_procedure_by_name (progress->pdb,
                                            progress->context,
                                            NULL, NULL,
                                            progress->callback_name,
                                            CLAY_TYPE_PROGRESS_COMMAND, command,
                                            G_TYPE_STRING,              text,
                                            G_TYPE_DOUBLE,              value,
                                            G_TYPE_NONE);

      if (g_value_get_enum (clay_value_array_index (return_vals, 0)) !=
          CLAY_PDB_SUCCESS)
        {
          clay_message (progress->context->clay, NULL, CLAY_MESSAGE_ERROR,
                        _("Unable to run %s callback. "
                          "The corresponding plug-in may have crashed."),
                        g_type_name (G_TYPE_FROM_INSTANCE (progress)));
        }
      else if (clay_value_array_length (return_vals) >= 2 &&
               handle != NULL                             &&
               G_VALUE_HOLDS_BOXED (clay_value_array_index (return_vals, 1)))
        {
          *handle = g_value_dup_boxed (clay_value_array_index (return_vals, 1));
        }

      clay_value_array_unref (return_vals);

      progress->callback_busy = FALSE;
    }
}

static ClayProgress *
clay_pdb_progress_progress_start (ClayProgress *progress,
                                  gboolean      cancellable,
                                  const gchar  *message)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  if (! pdb_progress->active)
    {
      clay_pdb_progress_run_callback (pdb_progress,
                                      CLAY_PROGRESS_COMMAND_START,
                                      message, 0.0, NULL);

      pdb_progress->active = TRUE;
      pdb_progress->value  = 0.0;

      return progress;
    }

  return NULL;
}

static void
clay_pdb_progress_progress_end (ClayProgress *progress)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      clay_pdb_progress_run_callback (pdb_progress,
                                      CLAY_PROGRESS_COMMAND_END,
                                      NULL, 0.0, NULL);

      pdb_progress->active = FALSE;
      pdb_progress->value  = 0.0;
    }
}

static gboolean
clay_pdb_progress_progress_is_active (ClayProgress *progress)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  return pdb_progress->active;
}

static void
clay_pdb_progress_progress_set_text (ClayProgress *progress,
                                     const gchar  *message)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    clay_pdb_progress_run_callback (pdb_progress,
                                    CLAY_PROGRESS_COMMAND_SET_TEXT,
                                    message, 0.0, NULL);
}

static void
clay_pdb_progress_progress_set_value (ClayProgress *progress,
                                      gdouble       percentage)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      clay_pdb_progress_run_callback (pdb_progress,
                                      CLAY_PROGRESS_COMMAND_SET_VALUE,
                                      NULL, percentage, NULL);
      pdb_progress->value = percentage;
    }
}

static gdouble
clay_pdb_progress_progress_get_value (ClayProgress *progress)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  return pdb_progress->value;

}

static void
clay_pdb_progress_progress_pulse (ClayProgress *progress)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    clay_pdb_progress_run_callback (pdb_progress,
                                    CLAY_PROGRESS_COMMAND_PULSE,
                                    NULL, 0.0, NULL);
}

static GBytes *
clay_pdb_progress_progress_get_window_handle (ClayProgress *progress)
{
  ClayPdbProgress *pdb_progress = CLAY_PDB_PROGRESS (progress);
  GBytes          *handle       = NULL;

  clay_pdb_progress_run_callback (pdb_progress,
                                  CLAY_PROGRESS_COMMAND_GET_WINDOW,
                                  NULL, 0.0, &handle);
  return handle;
}

ClayPdbProgress *
clay_pdb_progress_get_by_callback (ClayPdbProgressClass *klass,
                                   const gchar          *callback_name)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_PDB_PROGRESS_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->progresses; list; list = g_list_next (list))
    {
      ClayPdbProgress *progress = list->data;

      if (! g_strcmp0 (callback_name, progress->callback_name))
        return progress;
    }

  return NULL;
}
