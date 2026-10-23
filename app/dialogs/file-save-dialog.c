/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/clayprogress.h"

#include "plug-in/claypluginmanager-file.h"
#include "plug-in/claypluginprocedure.h"

#include "file/file-save.h"
#include "file/clay-file.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayexportdialog.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claysavedialog.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "file-save-dialog.h"

#include "clay-log.h"
#include "clay-intl.h"


typedef enum
{
  CHECK_URI_FAIL,
  CHECK_URI_OK,
  CHECK_URI_SWITCH_DIALOGS
} CheckUriResult;


/*  local function prototypes  */

static GtkFileChooserConfirmation
                 file_save_dialog_confirm_overwrite         (GtkWidget            *dialog,
                                                             Clay                 *clay);
static void      file_save_dialog_response                  (GtkWidget            *dialog,
                                                             gint                  response_id,
                                                             Clay                 *clay);
static CheckUriResult file_save_dialog_check_file           (GtkWidget            *save_dialog,
                                                             Clay                 *clay,
                                                             GFile               **ret_file,
                                                             gchar               **ret_basename,
                                                             ClayPlugInProcedure **ret_save_proc);
static gboolean  file_save_dialog_no_overwrite_confirmation (ClayFileDialog       *dialog,
                                                             Clay                 *clay);
static ClayPlugInProcedure *
                 file_save_dialog_find_procedure            (ClayFileDialog       *dialog,
                                                             GFile                *file);
static gboolean  file_save_dialog_switch_dialogs            (ClayFileDialog       *file_dialog,
                                                             Clay                 *clay,
                                                             const gchar          *basename);
static gboolean  file_save_dialog_use_extension             (GtkWidget            *save_dialog,
                                                             GFile                *file);


/*  public functions  */

GtkWidget *
file_save_dialog_new (Clay     *clay,
                      gboolean  export)
{
  GtkWidget *dialog;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (! export)
    {
      dialog = clay_save_dialog_new (clay);

      clay_file_dialog_load_state (CLAY_FILE_DIALOG (dialog),
                                   "clay-file-save-dialog-state");
    }
  else
    {
      dialog = clay_export_dialog_new (clay);

      clay_file_dialog_load_state (CLAY_FILE_DIALOG (dialog),
                                   "clay-file-export-dialog-state");
    }

  g_signal_connect (dialog, "confirm-overwrite",
                    G_CALLBACK (file_save_dialog_confirm_overwrite),
                    clay);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_save_dialog_response),
                    clay);

  return dialog;
}


/*  private functions  */

static GtkFileChooserConfirmation
file_save_dialog_confirm_overwrite (GtkWidget *dialog,
                                    Clay      *clay)
{
  ClayFileDialog *file_dialog = CLAY_FILE_DIALOG (dialog);

  if (file_save_dialog_no_overwrite_confirmation (file_dialog, clay))
    /* The URI will not be accepted whatever happens, so don't
     * bother asking the user about overwriting files
     */
    return GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
  else
    return GTK_FILE_CHOOSER_CONFIRMATION_CONFIRM;
}

static void
file_save_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           Clay      *clay)
{
  ClayFileDialog      *file_dialog = CLAY_FILE_DIALOG (dialog);
  GFile               *file;
  gchar               *basename;
  ClayPlugInProcedure *save_proc;

  if (CLAY_IS_SAVE_DIALOG (dialog))
    {
      clay_file_dialog_save_state (file_dialog, "clay-file-save-dialog-state");
    }
  else /* CLAY_IS_EXPORT_DIALOG (dialog) */
    {
      clay_file_dialog_save_state (file_dialog, "clay-file-export-dialog-state");
    }

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! file_dialog->busy && response_id != GTK_RESPONSE_HELP)
        gtk_widget_destroy (dialog);

      return;
    }

  g_object_ref (file_dialog);
  g_object_ref (file_dialog->image);

  switch (file_save_dialog_check_file (dialog, clay,
                                       &file, &basename, &save_proc))
    {
    case CHECK_URI_FAIL:
      break;

    case CHECK_URI_OK:
      {
        ClayImage    *image              = file_dialog->image;
        ClayProgress *progress           = CLAY_PROGRESS (dialog);
        ClayDisplay  *display_to_close   = NULL;
        gboolean      xcf_compression    = FALSE;
        gboolean      is_save_dialog     = CLAY_IS_SAVE_DIALOG (dialog);
        gboolean      close_after_saving = FALSE;
        gboolean      save_a_copy        = FALSE;

        if (is_save_dialog)
          {
            close_after_saving = CLAY_SAVE_DIALOG (dialog)->close_after_saving;
            display_to_close   = CLAY_DISPLAY (CLAY_SAVE_DIALOG (dialog)->display_to_close);
            save_a_copy        = CLAY_SAVE_DIALOG (dialog)->save_a_copy;
          }

        clay_file_dialog_set_sensitive (file_dialog, FALSE);

        if (CLAY_IS_SAVE_DIALOG (dialog))
          xcf_compression = CLAY_SAVE_DIALOG (dialog)->compression;
        else
          xcf_compression = clay_image_get_xcf_compression (image);

        /* Hide the file dialog while exporting, avoid dialogs piling
         * up, even more as some formats have preview features, so the
         * file dialog is just blocking the view.
         */
        if  (CLAY_IS_EXPORT_DIALOG (dialog))
          {
            gtk_widget_set_visible (dialog, FALSE);
            progress = CLAY_PROGRESS (CLAY_EXPORT_DIALOG (dialog)->display);
          }

        g_signal_connect (dialog, "destroy",
                          G_CALLBACK (gtk_widget_destroyed),
                          &dialog);

        if (file_save_dialog_save_image (progress,
                                         clay,
                                         image,
                                         file,
                                         save_proc,
                                         CLAY_RUN_INTERACTIVE,
                                         is_save_dialog && ! save_a_copy,
                                         FALSE,
                                         CLAY_IS_EXPORT_DIALOG (dialog),
                                         xcf_compression,
                                         FALSE))
          {
            /* Save was successful, now store the URI in a couple of
             * places that depend on it being the user that made a
             * save. Lower-level URI management is handled in
             * file_save()
             */
            if (is_save_dialog)
              {
                if (save_a_copy)
                  clay_image_set_save_a_copy_file (image, file);

                g_object_set_data_full (G_OBJECT (image->clay),
                                        CLAY_FILE_SAVE_LAST_FILE_KEY,
                                        g_object_ref (file),
                                        (GDestroyNotify) g_object_unref);
              }
            else
              {
                g_object_set_data_full (G_OBJECT (image->clay),
                                        CLAY_FILE_EXPORT_LAST_FILE_KEY,
                                        g_object_ref (file),
                                        (GDestroyNotify) g_object_unref);
              }

            /*  make sure the menus are updated with the keys we've just set  */
            clay_image_flush (image);

            /* Handle close-after-saving */
            if (close_after_saving && display_to_close &&
                ! clay_image_is_dirty (clay_display_get_image (display_to_close)))
              {
                clay_display_close (display_to_close);
              }

            if (dialog)
              gtk_widget_destroy (dialog);
          }
        else
          {
            if (dialog)
              {
                GFile *parent_dir = g_file_get_parent (file);

                /* XXX Not sure why, but after reshowing the file
                 * chooser dialog, the displayed name is correct, but
                 * the parent directory is the current working dir.
                 * Force it to be the expected folder.
                 */
                gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                          parent_dir, NULL);
                gtk_widget_set_visible (dialog, TRUE);
                g_object_unref (parent_dir);
              }
          }

        g_object_unref (file);
        g_free (basename);

        if (dialog)
          {
            clay_file_dialog_set_sensitive (file_dialog, TRUE);
            g_signal_handlers_disconnect_by_func (dialog,
                                                  G_CALLBACK (gtk_widget_destroyed),
                                                  &dialog);
          }
      }
      break;

    case CHECK_URI_SWITCH_DIALOGS:
      file_dialog->busy = TRUE; /* prevent destruction */
      gtk_dialog_response (GTK_DIALOG (dialog), FILE_SAVE_RESPONSE_OTHER_DIALOG);
      file_dialog->busy = FALSE;

      gtk_widget_destroy (dialog);
      break;
    }

  g_object_unref (file_dialog->image);
  g_object_unref (file_dialog);
}

/* IMPORTANT: When changing this function, keep
 * file_save_dialog_no_overwrite_confirmation() up to date. It is
 * difficult to move logic to a common place due to how the dialog is
 * implemented in GTK+ in combination with how we use it.
 */
static CheckUriResult
file_save_dialog_check_file (GtkWidget            *dialog,
                             Clay                 *clay,
                             GFile               **ret_file,
                             gchar               **ret_basename,
                             ClayPlugInProcedure **ret_save_proc)
{
  ClayFileDialog      *file_dialog = CLAY_FILE_DIALOG (dialog);
  GFile               *file;
  gchar               *uri;
  gchar               *basename;
  GFile               *basename_file;
  ClayPlugInProcedure *save_proc;
  ClayPlugInProcedure *uri_proc;
  ClayPlugInProcedure *basename_proc;

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

  if (! file)
    return CHECK_URI_FAIL;

  basename      = g_path_get_basename (clay_file_get_utf8_name (file));
  basename_file = g_file_new_for_uri (basename);

  save_proc     = file_dialog->file_proc;
  uri_proc      = file_save_dialog_find_procedure (file_dialog, file);
  basename_proc = file_save_dialog_find_procedure (file_dialog, basename_file);

  g_object_unref (basename_file);

  uri = g_file_get_uri (file);

  CLAY_LOG (SAVE_DIALOG, "URI = %s", uri);
  CLAY_LOG (SAVE_DIALOG, "basename = %s", basename);
  CLAY_LOG (SAVE_DIALOG, "selected save_proc: %s",
            save_proc ?
            clay_procedure_get_label (CLAY_PROCEDURE (save_proc)) : "NULL");
  CLAY_LOG (SAVE_DIALOG, "URI save_proc: %s",
            uri_proc ?
            clay_procedure_get_label (CLAY_PROCEDURE (uri_proc)) : "NULL");
  CLAY_LOG (SAVE_DIALOG, "basename save_proc: %s",
            basename_proc ?
            clay_procedure_get_label (CLAY_PROCEDURE (basename_proc)) : "NULL");

  g_free (uri);

  /*  first check if the user entered an extension at all  */
  if (! basename_proc)
    {
      CLAY_LOG (SAVE_DIALOG, "basename has no valid extension");

      if (! strchr (basename, '.'))
        {
          const gchar *ext = NULL;

          CLAY_LOG (SAVE_DIALOG, "basename has no '.', trying to add extension");

          if (! save_proc && CLAY_IS_SAVE_DIALOG (dialog))
            {
              ext = "xcf";
            }
          else if (save_proc && save_proc->extensions_list)
            {
              ext = save_proc->extensions_list->data;
            }

          if (ext)
            {
              gchar *ext_basename;
              gchar *dirname;
              gchar *filename;
              gchar *utf8;

              CLAY_LOG (SAVE_DIALOG, "appending .%s to basename", ext);

              ext_basename = g_strconcat (basename, ".", ext, NULL);

              g_free (basename);
              basename = ext_basename;

              dirname  = g_path_get_dirname (clay_file_get_utf8_name (file));
              filename = g_build_filename (dirname, basename, NULL);
              g_free (dirname);

              utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
              gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                                 utf8);
              g_free (utf8);

              g_free (filename);

              CLAY_LOG (SAVE_DIALOG,
                        "set basename to %s, rerunning response and bailing out",
                        basename);

              /*  call the response callback again, so the
               *  overwrite-confirm logic can check the changed uri
               */
              gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

              goto fail;
            }
          else
            {
              CLAY_LOG (SAVE_DIALOG,
                        "save_proc has no extensions, continuing without");

              /*  there may be file formats with no extension at all, use
               *  the selected proc in this case.
               */
              basename_proc = save_proc;

              if (! uri_proc)
                uri_proc = basename_proc;
            }

          if (! basename_proc)
            {
              CLAY_LOG (SAVE_DIALOG,
                        "unable to figure save_proc, bailing out");

              if (file_save_dialog_switch_dialogs (file_dialog, clay, basename))
                {
                  goto switch_dialogs;
                }

              goto fail;
            }
        }
      else if (save_proc && ! save_proc->extensions_list)
        {
          CLAY_LOG (SAVE_DIALOG,
                    "basename has '.', but save_proc has no extensions, "
                    "accepting random extension");

          /*  accept any random extension if the file format has
           *  no extensions at all
           */
          basename_proc = save_proc;

          if (! uri_proc)
            uri_proc = basename_proc;
        }
    }

  /*  then check if the selected format matches the entered extension  */
  if (! save_proc)
    {
      CLAY_LOG (SAVE_DIALOG, "no save_proc was selected from the list");

      if (! basename_proc)
        {
          CLAY_LOG (SAVE_DIALOG,
                    "basename has no useful extension, bailing out");

          if (file_save_dialog_switch_dialogs (file_dialog, clay, basename))
            {
              goto switch_dialogs;
            }

          goto fail;
        }

      CLAY_LOG (SAVE_DIALOG, "use URI's proc '%s' so indirect saving works",
                clay_procedure_get_label (CLAY_PROCEDURE (uri_proc)));

      /*  use the URI's proc if no save proc was selected  */
      save_proc = uri_proc;
    }
  else
    {
      CLAY_LOG (SAVE_DIALOG, "save_proc '%s' was selected from the list",
                clay_procedure_get_label (CLAY_PROCEDURE (save_proc)));

      if (save_proc != basename_proc)
        {
          CLAY_LOG (SAVE_DIALOG, "however the basename's proc is '%s'",
                    clay_procedure_get_label (CLAY_PROCEDURE (basename_proc)));

          if (uri_proc != basename_proc)
            {
              CLAY_LOG (SAVE_DIALOG,
                        "that's impossible for remote URIs, bailing out");

              /*  remote URI  */

              clay_message (clay, G_OBJECT (dialog), CLAY_MESSAGE_WARNING,
                            _("Saving remote files needs to determine the "
                              "file format from the file extension. "
                              "Please enter a file extension that matches "
                              "the selected file format or enter no file "
                              "extension at all."));

              goto fail;
            }
          else
            {
              CLAY_LOG (SAVE_DIALOG,
                        "ask the user if she really wants that filename");

              /*  local URI  */

              if (! file_save_dialog_use_extension (dialog, file))
                {
                  goto fail;
                }
            }
        }
      else if (save_proc != uri_proc)
        {
          CLAY_LOG (SAVE_DIALOG,
                    "use URI's proc '%s' so indirect saving works",
                    clay_procedure_get_label (CLAY_PROCEDURE (uri_proc)));

          /*  need to use the URI's proc for saving because e.g.
           *  the GIF plug-in can't save a GIF to sftp://
           */
          save_proc = uri_proc;
        }
    }

  if (! save_proc)
    {
      g_warning ("%s: EEEEEEK", G_STRFUNC);

      return CHECK_URI_FAIL;
    }

  *ret_file      = file;
  *ret_basename  = basename;
  *ret_save_proc = save_proc;

  return CHECK_URI_OK;

 fail:

  g_object_unref (file);
  g_free (basename);

  return CHECK_URI_FAIL;

 switch_dialogs:

  g_object_unref (file);
  g_free (basename);

  return CHECK_URI_SWITCH_DIALOGS;
}

/*
 * IMPORTANT: Keep this up to date with file_save_dialog_check_uri().
 */
static gboolean
file_save_dialog_no_overwrite_confirmation (ClayFileDialog *file_dialog,
                                            Clay           *clay)
{
  GFile               *file;
  gchar               *basename;
  GFile               *basename_file;
  ClayPlugInProcedure *basename_proc;
  ClayPlugInProcedure *save_proc;
  gboolean             uri_will_change;
  gboolean             unknown_ext;

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_dialog));

  if (! file)
    return FALSE;

  basename      = g_path_get_basename (clay_file_get_utf8_name (file));
  basename_file = g_file_new_for_uri (basename);

  save_proc     = file_dialog->file_proc;
  basename_proc = file_save_dialog_find_procedure (file_dialog, basename_file);

  g_object_unref (basename_file);

  uri_will_change = (! basename_proc &&
                     ! strchr (basename, '.') &&
                     (! save_proc || save_proc->extensions_list));

  unknown_ext     = (! save_proc &&
                     ! basename_proc);

  g_free (basename);
  g_object_unref (file);

  return uri_will_change || unknown_ext;
}

static ClayPlugInProcedure *
file_save_dialog_find_procedure (ClayFileDialog *file_dialog,
                                 GFile          *file)
{
  ClayPlugInManager      *manager = file_dialog->clay->plug_in_manager;
  ClayFileProcedureGroup  group;

  if (CLAY_IS_SAVE_DIALOG (file_dialog))
    group = CLAY_FILE_PROCEDURE_GROUP_SAVE;
  else
    group = CLAY_FILE_PROCEDURE_GROUP_EXPORT;

  return clay_plug_in_manager_file_procedure_find (manager, group, file, NULL);
}

static gboolean
file_save_other_dialog_activated (GtkWidget   *label,
                                  const gchar *uri,
                                  GtkDialog   *dialog)
{
  gtk_dialog_response (dialog, FILE_SAVE_RESPONSE_OTHER_DIALOG);

  return TRUE;
}

static gboolean
file_save_dialog_switch_dialogs (ClayFileDialog *file_dialog,
                                 Clay           *clay,
                                 const gchar    *basename)
{
  ClayPlugInProcedure    *proc_in_other_group;
  ClayFileProcedureGroup  other_group;
  GFile                  *file;
  gboolean                switch_dialogs = FALSE;

  file = g_file_new_for_uri (basename);

  if (CLAY_IS_EXPORT_DIALOG (file_dialog))
    other_group = CLAY_FILE_PROCEDURE_GROUP_SAVE;
  else
    other_group = CLAY_FILE_PROCEDURE_GROUP_EXPORT;

  proc_in_other_group =
    clay_plug_in_manager_file_procedure_find (clay->plug_in_manager,
                                              other_group, file, NULL);

  g_object_unref (file);

  if (proc_in_other_group)
    {
      GtkWidget   *dialog;
      const gchar *primary;
      const gchar *message;
      const gchar *link;

      if (CLAY_IS_EXPORT_DIALOG (file_dialog))
        {
          primary = _("The given filename cannot be used for exporting");
          message = _("You can use this dialog to export to various file formats. "
                      "If you want to save the image to the Clay XCF format, use "
                      "File→Save instead.");
          link    = _("Take me to the Save dialog");
        }
      else
        {
          primary = _("The given filename cannot be used for saving");
          message = _("You can use this dialog to save to the Clay XCF "
                      "format. Use File→Export to export to other file formats.");
          link    = _("Take me to the Export dialog");
        }

      dialog = clay_message_dialog_new (_("Extension Mismatch"),
                                        CLAY_ICON_DIALOG_WARNING,
                                        GTK_WIDGET (file_dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        clay_standard_help_func, NULL,

                                        _("_OK"), GTK_RESPONSE_OK,

                                        NULL);

      clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                         "%s", primary);

      clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                 "%s", message);

      if (CLAY_IS_EXPORT_DIALOG (file_dialog) ||
          (! CLAY_SAVE_DIALOG (file_dialog)->save_a_copy &&
          ! CLAY_SAVE_DIALOG (file_dialog)->close_after_saving))
        {
          GtkWidget *label;
          gchar     *markup;

          markup = g_strdup_printf ("<a href=\"other-dialog\">%s</a>", link);
          label = gtk_label_new (markup);
          g_free (markup);

          gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_box_pack_start (GTK_BOX (CLAY_MESSAGE_DIALOG (dialog)->box), label,
                              FALSE, FALSE, 0);
          gtk_widget_set_visible (label, TRUE);

          g_signal_connect (label, "activate-link",
                            G_CALLBACK (file_save_other_dialog_activated),
                            dialog);
        }

      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_OK, FALSE);

      g_object_ref (dialog);

      if (clay_dialog_run (CLAY_DIALOG (dialog)) == FILE_SAVE_RESPONSE_OTHER_DIALOG)
        {
          switch_dialogs = TRUE;
        }

      gtk_widget_destroy (dialog);
      g_object_unref (dialog);

      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_CANCEL, TRUE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (file_dialog),
                                         GTK_RESPONSE_OK, TRUE);
    }
  else
    {
      clay_message (clay, G_OBJECT (file_dialog), CLAY_MESSAGE_WARNING,
                    _("The given filename does not have any known "
                      "file extension. Please enter a known file "
                      "extension or select a file format from the "
                      "file format list."));
    }

  return switch_dialogs;
}

static gboolean
file_save_dialog_use_extension (GtkWidget *save_dialog,
                                GFile     *file)
{
  GtkWidget *dialog;
  gboolean   use_name = FALSE;

  dialog = clay_message_dialog_new (_("Extension Mismatch"),
                                    CLAY_ICON_DIALOG_QUESTION,
                                    save_dialog, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    clay_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Save"),   GTK_RESPONSE_OK,

                                    NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("The given file extension does "
                                       "not match the chosen file type."));

  clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                             _("Do you want to save the image using this "
                               "name anyway?"));

  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_OK, FALSE);

  g_object_ref (dialog);

  use_name = (clay_dialog_run (CLAY_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);
  g_object_unref (dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, TRUE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_OK, TRUE);

  return use_name;
}

gboolean
file_save_dialog_save_image (ClayProgress        *progress,
                             Clay                *clay,
                             ClayImage           *image,
                             GFile               *file,
                             ClayPlugInProcedure *save_proc,
                             ClayRunMode          run_mode,
                             gboolean             change_saved_state,
                             gboolean             export_backward,
                             gboolean             export_forward,
                             gboolean             xcf_compression,
                             gboolean             verbose_cancel)
{
  ClayPDBStatusType  status;
  GError            *error   = NULL;
  GList             *list;
  gboolean           success = FALSE;

  for (list = clay_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      clay_action_group_set_action_sensitive (list->data, "file-quit", FALSE, NULL);
    }

  clay_image_set_xcf_compression (image, xcf_compression);

  /* The save may fail and the progress widget be already freed if we
   * close the main window while the save dialog is running. So add a
   * weak pointer to avoid sending an error message to an already-freed
   * ClayProgress. See #11922.
   */
  g_object_add_weak_pointer (G_OBJECT (progress), (gpointer *) &progress);

  status = file_save (clay, image, progress, file,
                      save_proc, run_mode,
                      change_saved_state, export_backward, export_forward,
                      &error);

  switch (status)
    {
    case CLAY_PDB_SUCCESS:
      success = TRUE;
      break;

    case CLAY_PDB_CANCEL:
      if (verbose_cancel && progress)
        clay_message_literal (clay,
                              G_OBJECT (progress), CLAY_MESSAGE_INFO,
                              _("Saving canceled"));
      break;

    default:
      {
        if (progress)
          clay_message (clay, G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                        _("Saving '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file),
                        error ? error->message : _("Unknown error"));
        g_clear_error (&error);
      }
      break;
    }

  if (progress)
    g_object_remove_weak_pointer (G_OBJECT (progress), (gpointer *) &progress);

  for (list = clay_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      clay_action_group_set_action_sensitive (list->data, "file-quit", TRUE, NULL);
    }

  return success;
}
