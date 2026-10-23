/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbdialog.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayresource.h"

#include "pdb/claypdb.h"

#include "claymenufactory.h"
#include "claypdbdialog.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_PDB,
  PROP_CONTEXT,
  PROP_SELECT_TYPE,
  PROP_INITIAL_OBJECT,
  PROP_CALLBACK_NAME,
  PROP_MENU_FACTORY
};


static void     clay_pdb_dialog_constructed       (GObject            *object);
static void     clay_pdb_dialog_dispose           (GObject            *object);
static void     clay_pdb_dialog_set_property      (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);

static void     clay_pdb_dialog_response          (GtkDialog          *dialog,
                                                   gint                response_id);

static void     clay_pdb_dialog_context_changed   (ClayContext        *context,
                                                   ClayObject         *object,
                                                   ClayPdbDialog      *dialog);
static void     clay_pdb_dialog_plug_in_closed    (ClayPlugInManager  *manager,
                                                   ClayPlugIn         *plug_in,
                                                   ClayPdbDialog      *dialog);

static gboolean clay_pdb_dialog_run_callback_idle (ClayPdbDialog      *dialog);


G_DEFINE_ABSTRACT_TYPE (ClayPdbDialog, clay_pdb_dialog, CLAY_TYPE_DIALOG)

#define parent_class clay_pdb_dialog_parent_class


static void
clay_pdb_dialog_class_init (ClayPdbDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = clay_pdb_dialog_constructed;
  object_class->dispose      = clay_pdb_dialog_dispose;
  object_class->set_property = clay_pdb_dialog_set_property;

  dialog_class->response     = clay_pdb_dialog_response;

  klass->run_callback        = NULL;
  klass->get_object          = NULL;
  klass->set_object          = NULL;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        CLAY_TYPE_PDB,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SELECT_TYPE,
                                   g_param_spec_gtype ("select-type",
                                                       NULL, NULL,
                                                       CLAY_TYPE_OBJECT,
                                                       CLAY_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_OBJECT,
                                   g_param_spec_object ("initial-object",
                                                        NULL, NULL,
                                                        CLAY_TYPE_OBJECT,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_MENU_FACTORY,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_pdb_dialog_init (ClayPdbDialog *dialog)
{
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_OK"), GTK_RESPONSE_OK);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_Cancel"), GTK_RESPONSE_CANCEL);
}

static void
clay_pdb_dialog_constructed (GObject *object)
{
  ClayPdbDialog      *dialog = CLAY_PDB_DIALOG (object);
  ClayPdbDialogClass *klass  = CLAY_PDB_DIALOG_GET_CLASS (object);
  const gchar        *signal_name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  klass->dialogs = g_list_prepend (klass->dialogs, dialog);

  clay_assert (CLAY_IS_PDB (dialog->pdb));
  clay_assert (CLAY_IS_CONTEXT (dialog->caller_context));
  clay_assert (g_type_is_a (dialog->select_type, CLAY_TYPE_OBJECT));

  dialog->context = clay_context_new (dialog->caller_context->clay,
                                      G_OBJECT_TYPE_NAME (object),
                                      NULL);

  if (g_type_is_a (dialog->select_type, CLAY_TYPE_RESOURCE))
    {
      clay_context_set_by_type (dialog->context, dialog->select_type,
                                dialog->initial_object);

      signal_name = clay_context_type_to_signal_name (dialog->select_type);

      g_signal_connect_object (dialog->context, signal_name,
                               G_CALLBACK (clay_pdb_dialog_context_changed),
                               dialog, 0);
    }

  g_signal_connect_object (dialog->caller_context->clay->plug_in_manager,
                           "plug-in-closed",
                           G_CALLBACK (clay_pdb_dialog_plug_in_closed),
                           dialog, 0);
}

static void
clay_pdb_dialog_dispose (GObject *object)
{
  ClayPdbDialog      *dialog = CLAY_PDB_DIALOG (object);
  ClayPdbDialogClass *klass  = CLAY_PDB_DIALOG_GET_CLASS (object);

  klass->dialogs = g_list_remove (klass->dialogs, object);

  g_clear_object (&dialog->pdb);
  g_clear_object (&dialog->caller_context);
  g_clear_object (&dialog->context);
  g_clear_object (&dialog->initial_object);

  g_clear_pointer (&dialog->callback_name, g_free);

  g_clear_object (&dialog->menu_factory);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_pdb_dialog_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayPdbDialog *dialog = CLAY_PDB_DIALOG (object);

  switch (property_id)
    {
    case PROP_PDB:
      dialog->pdb = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      dialog->caller_context = g_value_dup_object (value);
      break;

    case PROP_SELECT_TYPE:
      dialog->select_type = g_value_get_gtype (value);
      break;

    case PROP_INITIAL_OBJECT:
      dialog->initial_object = g_value_dup_object (value);
      break;

    case PROP_CALLBACK_NAME:
      g_set_str (&dialog->callback_name, g_value_get_string (value));
      break;

    case PROP_MENU_FACTORY:
      dialog->menu_factory = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_dialog_response (GtkDialog *gtk_dialog,
                          gint       response_id)
{
  ClayPdbDialog *dialog = CLAY_PDB_DIALOG (gtk_dialog);

  if (response_id != GTK_RESPONSE_OK)
    {
      if (g_type_is_a (dialog->select_type, CLAY_TYPE_RESOURCE))
        {
          clay_context_set_by_type (dialog->context, dialog->select_type,
                                    dialog->initial_object);
        }
      else
        {
          ClayPdbDialogClass *klass = CLAY_PDB_DIALOG_GET_CLASS (dialog);

          g_return_if_fail (klass->set_object != NULL);
          klass->set_object (dialog, dialog->initial_object);
        }
    }

  clay_pdb_dialog_run_callback (&dialog, TRUE);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
clay_pdb_dialog_run_callback (ClayPdbDialog **dialog,
                              gboolean        closing)
{
  ClayPdbDialogClass *klass = CLAY_PDB_DIALOG_GET_CLASS (*dialog);
  ClayObject         *object;

  g_object_add_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);

  if (g_type_is_a ((*dialog)->select_type, CLAY_TYPE_RESOURCE))
    {
      object = clay_context_get_by_type ((*dialog)->context, (*dialog)->select_type);
    }
  else
    {
      g_return_if_fail (klass->get_object != NULL);
      object = klass->get_object (*dialog);
    }

  if (*dialog                  &&
      klass->run_callback      &&
      (*dialog)->callback_name &&
      ! (*dialog)->callback_busy)
    {
      (*dialog)->callback_busy = TRUE;

      if (clay_pdb_lookup_procedure ((*dialog)->pdb, (*dialog)->callback_name) &&
          (object == NULL || g_type_is_a (G_TYPE_FROM_INSTANCE (object), (*dialog)->select_type)))
        {
          ClayValueArray *return_vals;
          GError         *error = NULL;

          return_vals = klass->run_callback (*dialog, object, closing, &error);

          if (*dialog &&
              g_value_get_enum (clay_value_array_index (return_vals, 0)) !=
              CLAY_PDB_SUCCESS)
            {
              const gchar *message;

              if (error && error->message)
                message = error->message;
              else
                message = _("The corresponding plug-in may have crashed.");

              clay_message ((*dialog)->caller_context->clay, G_OBJECT (*dialog),
                            CLAY_MESSAGE_ERROR,
                            _("Unable to run %s callback.\n%s"),
                            g_type_name (G_TYPE_FROM_INSTANCE (*dialog)),
                            message);
            }
          else if (*dialog && error)
            {
              clay_message_literal ((*dialog)->caller_context->clay, G_OBJECT (*dialog),
                                    CLAY_MESSAGE_ERROR,
                                    error->message);
              g_error_free (error);
            }

          clay_value_array_unref (return_vals);
        }

      if (*dialog)
        (*dialog)->callback_busy = FALSE;
    }

  if (*dialog)
    g_object_remove_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);
}

ClayPdbDialog *
clay_pdb_dialog_get_by_callback (ClayPdbDialogClass *klass,
                                 const gchar        *callback_name)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_PDB_DIALOG_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->dialogs; list; list = g_list_next (list))
    {
      ClayPdbDialog *dialog = list->data;

      if (dialog->callback_name &&
          ! strcmp (callback_name, dialog->callback_name))
        return dialog;
    }

  return NULL;
}


/*  private functions  */

static void
clay_pdb_dialog_context_changed (ClayContext   *context,
                                 ClayObject    *object,
                                 ClayPdbDialog *dialog)
{
  /* XXX Long explanation below because this was annoying to debug!
   *
   * The context might change for 2 reasons: either from code coming from the
   * plug-in or because of direct GUI interaction with the dialog. This second
   * case may happen while the plug-in is doing something else completely and
   * making PDB calls too. If the callback itself triggers the plug-in to run
   * other PDB calls, we may end up in a case where PDB requests made by the
   * plug-in receive the wrong result. Here is an actual case I encountered:
   *
   * 1. The plug-in calls clay-gradient-get-uniform-samples to draw a button.
   * 2. Because you click inside the dialog (using any of the Clay*Select
   *    subclasses, e.g. ClayBrushSelect), the core dialog run its callback
   *    indicating that a resource changed. This is unrelated to step 1. It just
   *    happens nearly in the same time by a race condition.
   * 3. The plug-in receives the callback as a temp procedure request, processes
   *    it first. In particular, as part of the code in libclay/plug-in, it
   *    verifies the resource is valid calling clay-resource-id-is-valid.
   * 4. Meanwhile, the core returns clay-gradient-get-uniform-samples result.
   * 5. The plug-in takes it as a result for clay-resource-id-is-valid with
   *    invalid return values (size and types).
   * 6. Then again, it receives result of clay-resource-id-is-valid as being the
   *    result of clay-gradient-get-uniform-samples, again invalid.
   *
   * This scenario mostly happens because the callback is not the consequence of
   * the plug-in's requests (if the callback from core were the consequence of a
   * call from the plug-in, the PDB protocol would ensure that every message in
   * both direction get its return values in the proper order). The problem here
   * is really that the callback interrupts unexpectedly PDB requests coming
   * from the plug-in while the plug-in process can't really know that this
   * callback is not a consequence of its own PDB call.
   *
   * Running as idle will make the callback run when the core is not already
   * processing anything, which should also mean that the plug-in is not waiting
   * for an unrelated procedure result.
   * To be fair, I am not 100% sure if a complex race condition may not still
   * happen, but extensive testing were successful so far.
   */
  if (object)
    g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                     (GSourceFunc) clay_pdb_dialog_run_callback_idle,
                     g_object_ref (dialog), g_object_unref);
}

static void
clay_pdb_dialog_plug_in_closed (ClayPlugInManager *manager,
                                ClayPlugIn        *plug_in,
                                ClayPdbDialog     *dialog)
{
  if (dialog->caller_context && dialog->callback_name)
    {
      if (! clay_pdb_lookup_procedure (dialog->pdb, dialog->callback_name))
        {
          gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }
    }
}

static gboolean
clay_pdb_dialog_run_callback_idle (ClayPdbDialog *dialog)
{
  clay_pdb_dialog_run_callback (&dialog, FALSE);

  return G_SOURCE_REMOVE;
}
