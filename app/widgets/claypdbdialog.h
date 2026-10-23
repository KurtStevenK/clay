/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbdialog.h
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

#pragma once


#define CLAY_TYPE_PDB_DIALOG            (clay_pdb_dialog_get_type ())
#define CLAY_PDB_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PDB_DIALOG, ClayPdbDialog))
#define CLAY_PDB_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PDB_DIALOG, ClayPdbDialogClass))
#define CLAY_IS_PDB_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PDB_DIALOG))
#define CLAY_IS_PDB_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PDB_DIALOG))
#define CLAY_PDB_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PDB_DIALOG, ClayPdbDialogClass))


typedef struct _ClayPdbDialogClass  ClayPdbDialogClass;

struct _ClayPdbDialog
{
  ClayDialog       parent_instance;

  ClayPDB         *pdb;

  /*  The context we were created with. This is the context the plug-in
   *  exists in and must be used when calling the plug-in.
   */
  ClayContext     *caller_context;

  /*  The dialog's private context, serves just as model for the
   *  select widgets and must not be used when calling the plug-in.
   */
  ClayContext     *context;

  GType            select_type;
  ClayObject      *initial_object;
  gchar           *callback_name;
  gboolean         callback_busy;

  ClayMenuFactory *menu_factory;
  GtkWidget       *view;
};

struct _ClayPdbDialogClass
{
  ClayDialogClass  parent_class;

  GList           *dialogs;

  ClayValueArray * (* run_callback) (ClayPdbDialog  *dialog,
                                     ClayObject     *object,
                                     gboolean        closing,
                                     GError        **error);
  ClayObject     * (* get_object)   (ClayPdbDialog  *dialog);
  void             (* set_object)   (ClayPdbDialog  *dialog,
                                     ClayObject     *object);
};


GType           clay_pdb_dialog_get_type        (void);

void            clay_pdb_dialog_run_callback    (ClayPdbDialog     **dialog,
                                                 gboolean            closing);

ClayPdbDialog * clay_pdb_dialog_get_by_callback (ClayPdbDialogClass *klass,
                                                 const gchar        *callback_name);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPdbDialog, g_object_unref);
