/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorprofilecombobox.c
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claycolorprofilechooserdialog.h"
#include "claycolorprofilecombobox.h"
#include "claycolorprofilestore.h"
#include "claycolorprofilestore-private.h"


/**
 * SECTION: claycolorprofilecombobox
 * @title: ClayColorProfileComboBox
 * @short_description: A combo box for selecting color profiles.
 *
 * A combo box for selecting color profiles.
 **/


enum
{
  PROP_0,
  PROP_DIALOG,
  PROP_MODEL
};


struct _ClayColorProfileComboBox
{
  GtkComboBox  parent_instance;

  GtkWidget   *dialog;
  GtkTreePath *last_path;
};


static void  clay_color_profile_combo_box_finalize     (GObject      *object);
static void  clay_color_profile_combo_box_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void  clay_color_profile_combo_box_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
static void  clay_color_profile_combo_box_changed      (GtkComboBox  *combo);

static gboolean  clay_color_profile_row_separator_func (GtkTreeModel *model,
                                                        GtkTreeIter  *iter,
                                                        gpointer      data);

static void  clay_color_profile_combo_dialog_response  (ClayColorProfileChooserDialog *dialog,
                                                        gint                           response,
                                                        ClayColorProfileComboBox      *combo);


G_DEFINE_TYPE (ClayColorProfileComboBox, clay_color_profile_combo_box, GTK_TYPE_COMBO_BOX)

#define parent_class clay_color_profile_combo_box_parent_class


static void
clay_color_profile_combo_box_class_init (ClayColorProfileComboBoxClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *combo_class  = GTK_COMBO_BOX_CLASS (klass);

  object_class->set_property = clay_color_profile_combo_box_set_property;
  object_class->get_property = clay_color_profile_combo_box_get_property;
  object_class->finalize     = clay_color_profile_combo_box_finalize;

  combo_class->changed       = clay_color_profile_combo_box_changed;

  /**
   * ClayColorProfileComboBox:dialog:
   *
   * #GtkDialog to present when the user selects the
   * "Select color profile from disk..." item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_DIALOG,
                                   g_param_spec_object ("dialog",
                                                        "Dialog",
                                                        "The dialog to present when selecting profiles from disk",
                                                        GTK_TYPE_DIALOG,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        CLAY_PARAM_READWRITE));
  /**
   * ClayColorProfileComboBox:model:
   *
   * Overrides the "model" property of the #GtkComboBox class.
   * #ClayColorProfileComboBox requires the model to be a
   * #ClayColorProfileStore.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "The profile store used for this combo box",
                                                        CLAY_TYPE_COLOR_PROFILE_STORE,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_color_profile_combo_box_init (ClayColorProfileComboBox *combo_box)
{
  GtkCellRenderer *cell;

  cell = gtk_cell_renderer_text_new ();

  g_object_set (cell,
                "width-chars", 42,
                "ellipsize",   PANGO_ELLIPSIZE_END,
                NULL);


  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "text", CLAY_COLOR_PROFILE_STORE_LABEL,
                                  NULL);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo_box),
                                        clay_color_profile_row_separator_func,
                                        NULL, NULL);
}

static void
clay_color_profile_combo_box_finalize (GObject *object)
{
  ClayColorProfileComboBox *combo = CLAY_COLOR_PROFILE_COMBO_BOX (object);

  if (combo->dialog)
    {
      if (CLAY_IS_COLOR_PROFILE_CHOOSER_DIALOG (combo->dialog))
        gtk_widget_destroy (combo->dialog);

      g_object_unref (combo->dialog);
      combo->dialog = NULL;
    }

  g_clear_pointer (&combo->last_path, gtk_tree_path_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_profile_combo_box_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  ClayColorProfileComboBox *combo = CLAY_COLOR_PROFILE_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_return_if_fail (combo->dialog == NULL);
      combo->dialog = g_value_dup_object (value);

      if (CLAY_IS_COLOR_PROFILE_CHOOSER_DIALOG (combo->dialog))
        g_signal_connect (combo->dialog, "response",
                          G_CALLBACK (clay_color_profile_combo_dialog_response),
                          object);
      break;

    case PROP_MODEL:
      gtk_combo_box_set_model (GTK_COMBO_BOX (object),
                               g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_profile_combo_box_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  ClayColorProfileComboBox *combo = CLAY_COLOR_PROFILE_COMBO_BOX (object);

  switch (property_id)
    {
    case PROP_DIALOG:
      g_value_set_object (value, combo->dialog);
      break;

    case PROP_MODEL:
      g_value_set_object (value,
                          gtk_combo_box_get_model (GTK_COMBO_BOX (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_profile_combo_box_changed (GtkComboBox *combo)
{
  ClayColorProfileComboBox *color_combo = CLAY_COLOR_PROFILE_COMBO_BOX (combo);
  GtkTreeModel             *model       = gtk_combo_box_get_model (combo);
  GtkTreeIter               iter;
  gint                      type;

  if (! gtk_combo_box_get_active_iter (combo, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case CLAY_COLOR_PROFILE_STORE_ITEM_DIALOG:
      {
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (combo));

        if (GTK_IS_WINDOW (parent))
          gtk_window_set_transient_for (GTK_WINDOW (color_combo->dialog),
                                        GTK_WINDOW (parent));

        gtk_window_present (GTK_WINDOW (color_combo->dialog));

        if (color_combo->last_path &&
            gtk_tree_model_get_iter (model, &iter, color_combo->last_path))
          {
            gtk_combo_box_set_active_iter (combo, &iter);
          }
      }
      break;

    case CLAY_COLOR_PROFILE_STORE_ITEM_FILE:
      if (color_combo->last_path)
        gtk_tree_path_free (color_combo->last_path);

      color_combo->last_path = gtk_tree_model_get_path (model, &iter);

      _clay_color_profile_store_history_reorder (CLAY_COLOR_PROFILE_STORE (model),
                                                 &iter);
      break;

    default:
      break;
    }
}


/**
 * clay_color_profile_combo_box_new:
 * @dialog:  a #GtkDialog to present when the user selects the
 *           "Select color profile from disk..." item
 * @history: #GFile of the profilerc (or %NULL for no history)
 *
 * Create a combo-box widget for selecting color profiles. The combo-box
 * is populated from the file specified as @history. This filename is
 * typically created using the following code snippet:
 * <informalexample><programlisting>
 *  gchar *history = clay_personal_rc_file ("profilerc");
 * </programlisting></informalexample>
 *
 * The recommended @dialog type to use is a #ClayColorProfileChooserDialog.
 * If a #ClayColorProfileChooserDialog is passed, #ClayColorProfileComboBox
 * will take complete control over the dialog, which means connecting
 * a GtkDialog::response() callback by itself, and take care of destroying
 * the dialog when the combo box is destroyed.
 *
 * If another type of @dialog is passed, this has to be implemented
 * separately.
 *
 * See also clay_color_profile_combo_box_new_with_model().
 *
 * Returns: a new #ClayColorProfileComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_color_profile_combo_box_new (GtkWidget *dialog,
                                  GFile     *history)
{
  GtkWidget    *combo;
  GtkListStore *store;

  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (history == NULL || G_IS_FILE (history), NULL);

  store = clay_color_profile_store_new (history);
  combo = clay_color_profile_combo_box_new_with_model (dialog,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  return combo;
}

/**
 * clay_color_profile_combo_box_new_with_model:
 * @dialog: a #GtkDialog to present when the user selects the
 *          "Select color profile from disk..." item
 * @model:  a #ClayColorProfileStore object
 *
 * This constructor is useful when you want to create several
 * combo-boxes for profile selection that all share the same
 * #ClayColorProfileStore. This is for example done in the
 * Clay Preferences dialog.
 *
 * See also clay_color_profile_combo_box_new().
 *
 * Returns: a new #ClayColorProfileComboBox.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_color_profile_combo_box_new_with_model (GtkWidget    *dialog,
                                             GtkTreeModel *model)
{
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (CLAY_IS_COLOR_PROFILE_STORE (model), NULL);

  return g_object_new (CLAY_TYPE_COLOR_PROFILE_COMBO_BOX,
                       "dialog", dialog,
                       "model",  model,
                       NULL);
}

/**
 * clay_color_profile_combo_box_add_file:
 * @combo: a #ClayColorProfileComboBox
 * @file:  file of the profile to add (or %NULL)
 * @label: label to use for the profile
 *         (may only be %NULL if @file is %NULL)
 *
 * This function delegates to the underlying
 * #ClayColorProfileStore. Please refer to the documentation of
 * clay_color_profile_store_add_file() for details.
 *
 * Since: 2.10
 **/
void
clay_color_profile_combo_box_add_file (ClayColorProfileComboBox *combo,
                                       GFile                    *file,
                                       const gchar              *label)
{
  GtkTreeModel *model;

  g_return_if_fail (CLAY_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (label != NULL || file == NULL);
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  clay_color_profile_store_add_file (CLAY_COLOR_PROFILE_STORE (model),
                                     file, label);
}

/**
 * clay_color_profile_combo_box_set_active_file:
 * @combo: a #ClayColorProfileComboBox
 * @file:  file of the profile to select
 * @label: label to use when adding a new entry (can be %NULL)
 *
 * Selects a color profile from the @combo and makes it the active
 * item.  If the profile is not listed in the @combo, then it is added
 * with the given @label (or @file in case that @label is %NULL).
 *
 * Since: 2.10
 **/
void
clay_color_profile_combo_box_set_active_file (ClayColorProfileComboBox *combo,
                                              GFile                    *file,
                                              const gchar              *label)
{
  ClayColorProfile *profile = NULL;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (CLAY_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (file && ! (label && *label))
    {
      GError *error = NULL;

      profile = clay_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          label = clay_color_profile_get_label (profile);
        }
    }

  if (_clay_color_profile_store_history_add (CLAY_COLOR_PROFILE_STORE (model),
                                             file, label, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }

  if (profile)
    g_object_unref (profile);
}

/**
 * clay_color_profile_combo_box_set_active_profile:
 * @combo:   a #ClayColorProfileComboBox
 * @profile: a #ClayColorProfile to set
 *
 * Selects a color profile from the @combo and makes it the active
 * item.
 *
 * Since: 3.0
 **/
void
clay_color_profile_combo_box_set_active_profile (ClayColorProfileComboBox *combo,
                                                 ClayColorProfile         *profile)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (CLAY_IS_COLOR_PROFILE_COMBO_BOX (combo));
  g_return_if_fail (profile == NULL || CLAY_IS_COLOR_PROFILE (profile));

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (_clay_color_profile_store_history_find_profile (CLAY_COLOR_PROFILE_STORE (model),
                                                      profile, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }
}

/**
 * clay_color_profile_combo_box_get_active_file:
 * @combo: a #ClayColorProfileComboBox
 *
 * Returns: (transfer none): The file of the currently selected
 *               color profile, release using g_object_unref() when it
 *               is not any longer needed.
 *
 * Since: 2.10
 **/
GFile *
clay_color_profile_combo_box_get_active_file (ClayColorProfileComboBox *combo)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (CLAY_IS_COLOR_PROFILE_COMBO_BOX (combo), NULL);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
      GFile *file;
      gint   type;

      gtk_tree_model_get (model, &iter,
                          CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                          CLAY_COLOR_PROFILE_STORE_FILE,      &file,
                          -1);

      if (type == CLAY_COLOR_PROFILE_STORE_ITEM_FILE)
        return file;

      if (file)
        g_object_unref (file);
    }

  return NULL;
}

static gboolean
clay_color_profile_row_separator_func (GtkTreeModel *model,
                                       GtkTreeIter  *iter,
                                       gpointer      data)
{
  gint type;

  gtk_tree_model_get (model, iter,
                      CLAY_COLOR_PROFILE_STORE_ITEM_TYPE, &type,
                      -1);

  switch (type)
    {
    case CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_TOP:
    case CLAY_COLOR_PROFILE_STORE_ITEM_SEPARATOR_BOTTOM:
      return TRUE;

    default:
      return FALSE;
    }
}

static void
clay_color_profile_combo_dialog_response (ClayColorProfileChooserDialog *dialog,
                                          gint                           response,
                                          ClayColorProfileComboBox      *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GFile *file;

      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (file)
        {
          clay_color_profile_combo_box_set_active_file (combo, file, NULL);

          g_object_unref (file);
        }
    }

  gtk_widget_set_visible (GTK_WIDGET (dialog), FALSE);
}
