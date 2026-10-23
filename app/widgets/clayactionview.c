/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayactionview.c
 * Copyright (C) 2004-2005  Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"

#include "clayaction.h"
#include "clayactiongroup.h"
#include "clayactionview.h"
#include "claymessagebox.h"
#include "claymessagedialog.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     clay_action_view_finalize        (GObject         *object);

static void     clay_action_view_select_path     (ClayActionView  *view,
                                                  GtkTreePath     *path);
static void     clay_action_view_accels_changed  (ClayAction      *action,
                                                  gchar          **accels,
                                                  ClayActionView  *view);
static void     clay_action_view_accel_edited    (GtkCellRendererAccel *accel,
                                                  const char      *path_string,
                                                  guint            accel_key,
                                                  GdkModifierType  accel_mask,
                                                  guint            hardware_keycode,
                                                  ClayActionView  *view);
static void     clay_action_view_accel_cleared   (GtkCellRendererAccel *accel,
                                                  const char      *path_string,
                                                  ClayActionView  *view);


G_DEFINE_TYPE (ClayActionView, clay_action_view, GTK_TYPE_TREE_VIEW)

#define parent_class clay_action_view_parent_class


static void
clay_action_view_class_init (ClayActionViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_action_view_finalize;
}

static void
clay_action_view_init (ClayActionView *view)
{
}

static void
clay_action_view_finalize (GObject *object)
{
  ClayActionView *view = CLAY_ACTION_VIEW (object);

  if (view->clay)
    {
      if (view->show_shortcuts)
        {
          gchar **actions;

          actions = g_action_group_list_actions (G_ACTION_GROUP (view->clay->app));

          for (gint i = 0; actions[i] != NULL; i++)
            {
              GAction *action;

              if (clay_action_is_gui_blacklisted (actions[i]))
                continue;

              action = g_action_map_lookup_action (G_ACTION_MAP (view->clay->app), actions[i]);
              g_signal_handlers_disconnect_by_func (action,
                                                    clay_action_view_accels_changed,
                                                    view);
            }
          g_strfreev (actions);
        }

      g_clear_object (&view->clay);
    }

  g_clear_pointer (&view->filter, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
clay_action_view_name_compare (const void *name1,
                               const void *name2)
{
  return strcmp (*(gchar **) name1, *(gchar **) name2);
}

GtkWidget *
clay_action_view_new (Clay        *clay,
                      const gchar *select_action,
                      gboolean     show_shortcuts)
{
  gchar            **actions;
  gchar             *group_name = NULL;

  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkTreeStore      *store;
  GtkTreeModel      *filter;
  GtkTreePath       *select_path = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  store = gtk_tree_store_new (CLAY_ACTION_VIEW_N_COLUMNS,
                              G_TYPE_BOOLEAN,          /* COLUMN_VISIBLE        */
                              CLAY_TYPE_ACTION,        /* COLUMN_ACTION         */
                              G_TYPE_STRING,           /* COLUMN_ICON_NAME      */
                              G_TYPE_STRING,           /* COLUMN_LABEL          */
                              G_TYPE_STRING,           /* COLUMN_LABEL_CASEFOLD */
                              G_TYPE_STRING,           /* COLUMN_NAME           */
                              G_TYPE_UINT,             /* COLUMN_ACCEL_KEY      */
                              GDK_TYPE_MODIFIER_TYPE); /* COLUMN_ACCEL_MASK     */

  actions = g_action_group_list_actions (G_ACTION_GROUP (clay->app));
  qsort (actions, g_strv_length (actions), sizeof (gchar *), clay_action_view_name_compare);
  for (gint i = 0; actions[i] != NULL; i++)
    {
      gchar          **split_name;
      GtkTreeIter      group_iter;
      GAction         *action;
      const gchar     *icon_name;
      gchar           *label;
      gchar           *label_casefold;
      guint            accel_key     = 0;
      GdkModifierType  accel_mask    = 0;
      GtkTreeIter      action_iter;

      if (clay_action_is_gui_blacklisted (actions[i]))
        continue;

      split_name = g_strsplit (actions[i], "-", 2);
      if (group_name == NULL || g_strcmp0 (group_name, split_name[0]) != 0)
        {
          /* Since we sorted alphabetically and we use the first part of the
           * action name as group name, we ensure that we create each group only
           * once.
           */
          g_free (group_name);
          group_name = g_strdup (split_name[0]);

          gtk_tree_store_append (store, &group_iter, NULL);

          gtk_tree_store_set (store, &group_iter,
                              /* TODO: get back ClayActionGroup info? */
                              /*CLAY_ACTION_VIEW_COLUMN_ICON_NAME, group->icon_name,*/
                              /*CLAY_ACTION_VIEW_COLUMN_LABEL, group->label,*/
                              CLAY_ACTION_VIEW_COLUMN_LABEL, group_name,
                              -1);
        }
      g_strfreev (split_name);

      action = g_action_map_lookup_action (G_ACTION_MAP (clay->app), actions[i]);
      g_return_val_if_fail (CLAY_IS_ACTION (action), NULL);

      icon_name = clay_action_get_icon_name (CLAY_ACTION (action));
      label     = clay_strip_uline (clay_action_get_label (CLAY_ACTION (action)));

      if (! (label && strlen (label)))
        {
          g_free (label);
          label = g_strdup (actions[i]);
        }

      label_casefold = g_utf8_casefold (label, -1);

      if (show_shortcuts)
        {
          const gchar **accels = NULL;

          accels = clay_action_get_accels (CLAY_ACTION (action));

          /* TODO GAction: support multiple accelerators! */
          if (accels && accels[0])
            gtk_accelerator_parse (accels[0], &accel_key, &accel_mask);
        }

      gtk_tree_store_append (store, &action_iter, &group_iter);

      gtk_tree_store_set (store, &action_iter,
                          CLAY_ACTION_VIEW_COLUMN_VISIBLE,        TRUE,
                          CLAY_ACTION_VIEW_COLUMN_ACTION,         action,
                          CLAY_ACTION_VIEW_COLUMN_ICON_NAME,      icon_name,
                          CLAY_ACTION_VIEW_COLUMN_LABEL,          label,
                          CLAY_ACTION_VIEW_COLUMN_LABEL_CASEFOLD, label_casefold,
                          CLAY_ACTION_VIEW_COLUMN_NAME,           actions[i],
                          CLAY_ACTION_VIEW_COLUMN_ACCEL_KEY,      accel_key,
                          CLAY_ACTION_VIEW_COLUMN_ACCEL_MASK,     accel_mask,
                          -1);

      g_free (label);
      g_free (label_casefold);

      if (select_action && ! strcmp (select_action, actions[i]))
        select_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
                                               &action_iter);
    }
  g_free (group_name);

  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);

  g_object_unref (store);

  view = g_object_new (CLAY_TYPE_ACTION_VIEW,
                       "model",      filter,
                       "rules-hint", TRUE,
                       NULL);

  g_object_unref (filter);

  gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter),
                                            CLAY_ACTION_VIEW_COLUMN_VISIBLE);

  CLAY_ACTION_VIEW (view)->clay           = g_object_ref (clay);
  CLAY_ACTION_VIEW (view)->show_shortcuts = show_shortcuts;

  gtk_tree_view_set_search_column (GTK_TREE_VIEW (view),
                                   CLAY_ACTION_VIEW_COLUMN_LABEL);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Action"));

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "icon-name",
                                       CLAY_ACTION_VIEW_COLUMN_ICON_NAME,
                                       NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text",
                                       CLAY_ACTION_VIEW_COLUMN_LABEL,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  if (show_shortcuts)
    {
      for (gint i = 0; actions[i] != NULL; i++)
        {
          GAction *action;

          if (clay_action_is_gui_blacklisted (actions[i]))
            continue;

          action = g_action_map_lookup_action (G_ACTION_MAP (clay->app), actions[i]);
          g_signal_connect (action, "accels-changed",
                            G_CALLBACK (clay_action_view_accels_changed),
                            view);
        }

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Shortcut"));

      cell = gtk_cell_renderer_accel_new ();
      g_object_set (cell,
                    "mode",     GTK_CELL_RENDERER_MODE_EDITABLE,
                    "editable", TRUE,
                    NULL);
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "accel-key",
                                           CLAY_ACTION_VIEW_COLUMN_ACCEL_KEY,
                                           "accel-mods",
                                           CLAY_ACTION_VIEW_COLUMN_ACCEL_MASK,
                                           NULL);

      g_signal_connect (cell, "accel-edited",
                        G_CALLBACK (clay_action_view_accel_edited),
                        view);
      g_signal_connect (cell, "accel-cleared",
                        G_CALLBACK (clay_action_view_accel_cleared),
                        view);

      gtk_tree_view_append_column (view, column);
    }
  g_strfreev (actions);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Name"));

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text",
                                       CLAY_ACTION_VIEW_COLUMN_NAME,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  if (select_path)
    {
      clay_action_view_select_path (CLAY_ACTION_VIEW (view), select_path);
      gtk_tree_path_free (select_path);
    }

  return GTK_WIDGET (view);
}

void
clay_action_view_set_filter (ClayActionView *view,
                             const gchar    *filter)
{
  GtkTreeSelection    *sel;
  GtkTreeModel        *filtered_model;
  GtkTreeModel        *model;
  GtkTreeIter          iter;
  gboolean             iter_valid;
  GtkTreeRowReference *selected_row = NULL;

  g_return_if_fail (CLAY_IS_ACTION_VIEW (view));

  filtered_model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filtered_model));

  if (filter && ! strlen (filter))
    filter = NULL;

  g_clear_pointer (&view->filter, g_free);

  if (filter)
    view->filter = g_utf8_casefold (filter, -1);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (filtered_model, &iter);

      selected_row = gtk_tree_row_reference_new (filtered_model, path);
    }

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GtkTreeIter child_iter;
      gboolean    child_valid;
      gint        n_children = 0;

      for (child_valid = gtk_tree_model_iter_children (model, &child_iter,
                                                       &iter);
           child_valid;
           child_valid = gtk_tree_model_iter_next (model, &child_iter))
        {
          gboolean visible = TRUE;

          if (view->filter)
            {
              gchar *label;
              gchar *name;

              gtk_tree_model_get (model, &child_iter,
                                  CLAY_ACTION_VIEW_COLUMN_LABEL_CASEFOLD, &label,
                                  CLAY_ACTION_VIEW_COLUMN_NAME,           &name,
                                  -1);

              visible = label && name && (strstr (label, view->filter) != NULL ||
                                          strstr (name,  view->filter) != NULL);

              g_free (label);
              g_free (name);
            }

          gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                              CLAY_ACTION_VIEW_COLUMN_VISIBLE, visible,
                              -1);

          if (visible)
            n_children++;
        }

      gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                          CLAY_ACTION_VIEW_COLUMN_VISIBLE, n_children > 0,
                          -1);
    }

  if (view->filter)
    gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
  else
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (view));

  gtk_tree_view_columns_autosize (GTK_TREE_VIEW (view));

  if (selected_row)
    {
      if (gtk_tree_row_reference_valid (selected_row))
        {
          GtkTreePath *path = gtk_tree_row_reference_get_path (selected_row);

          clay_action_view_select_path (view, path);
          gtk_tree_path_free (path);
        }

      gtk_tree_row_reference_free (selected_row);
    }
}


/*  private functions  */

static void
clay_action_view_select_path (ClayActionView *view,
                              GtkTreePath    *path)
{
  GtkTreeView *tv = GTK_TREE_VIEW (view);
  GtkTreePath *expand;

  expand = gtk_tree_path_copy (path);
  gtk_tree_path_up (expand);
  gtk_tree_view_expand_row (tv, expand, FALSE);
  gtk_tree_path_free (expand);

  gtk_tree_view_set_cursor (tv, path, NULL, FALSE);
  gtk_tree_view_scroll_to_cell (tv, path, NULL, TRUE, 0.5, 0.0);
}

static void
clay_action_view_accels_changed (ClayAction      *action,
                                 gchar          **accels,
                                 ClayActionView  *view)
{
  GtkTreeModel *model, *tmpmodel;
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gchar        *pathstr = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! model)
    return;

  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
  if (! model)
    return;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
                                       &tmpmodel, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (tmpmodel, &iter);

      pathstr = gtk_tree_path_to_string(path);
    }

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GtkTreeIter child_iter;
      gboolean    child_valid;

      for (child_valid = gtk_tree_model_iter_children (model, &child_iter,
                                                       &iter);
           child_valid;
           child_valid = gtk_tree_model_iter_next (model, &child_iter))
        {
          ClayAction *it_action;

          gtk_tree_model_get (model, &child_iter,
                              CLAY_ACTION_VIEW_COLUMN_ACTION, &it_action,
                              -1);

          if (it_action)
            g_object_unref (it_action);

          if (it_action == action)
            {
              const gchar     **accels;
              guint             accel_key  = 0;
              GdkModifierType   accel_mask = 0;

              accels = clay_action_get_accels (action);

              if (accels && accels[0])
                gtk_accelerator_parse (accels[0], &accel_key, &accel_mask);

              gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                                  CLAY_ACTION_VIEW_COLUMN_ACCEL_KEY,  accel_key,
                                  CLAY_ACTION_VIEW_COLUMN_ACCEL_MASK, accel_mask,
                                  -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                                  CLAY_ACTION_VIEW_COLUMN_VISIBLE, TRUE,
                                  -1);

              if (pathstr)
                {
                  GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);

                  clay_action_view_select_path (view, path);
                  gtk_tree_path_free (path);
                }
              g_free (pathstr);
              return;
            }
        }
    }
  g_free (pathstr);
}

typedef struct
{
  Clay            *clay;
  ClayAction      *action;
  guint            accel_key;
  GdkModifierType  accel_mask;
} ConfirmData;

static void
clay_action_view_conflict_response (GtkWidget   *dialog,
                                    gint         response_id,
                                    ConfirmData *confirm_data)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      gchar **dup_actions;
      gchar  *accel;

      accel = gtk_accelerator_name (confirm_data->accel_key,
                                    confirm_data->accel_mask);
      dup_actions = gtk_application_get_actions_for_accel (GTK_APPLICATION (confirm_data->clay->app),
                                                           accel);
      for (gint i = 0; dup_actions[i] != NULL; i++)
        {
          GAction *conflict_action;
          gint     start;
          gchar   *left_paren_ptr = strchr (dup_actions[i], '(');

          if (left_paren_ptr)
            *left_paren_ptr = '\0';     /* ignore target part of detailed name */

          start = g_str_has_prefix (dup_actions[i], "app.") ? 4 : 0;
          conflict_action = g_action_map_lookup_action (G_ACTION_MAP (confirm_data->clay->app),
                                                        dup_actions[i] + start);

          g_return_if_fail (CLAY_IS_ACTION (conflict_action));
          clay_action_set_accels (CLAY_ACTION (conflict_action), (const char*[]) { NULL });
        }

      g_strfreev (dup_actions);

      clay_action_set_accels (confirm_data->action, (const char*[]) { accel, NULL });
    }

  g_slice_free (ConfirmData, confirm_data);
}

static void
clay_action_view_conflict_confirm (ClayActionView  *view,
                                   ClayAction      *action,
                                   ClayAction      *edit_action,
                                   guint            accel_key,
                                   GdkModifierType  accel_mask)
{
  ClayActionGroup *group;
  gchar           *label;
  gchar           *accel_string;
  ConfirmData     *confirm_data;
  GtkWidget       *dialog;
  ClayMessageBox  *box;

  group = clay_action_get_group (action);

  label = clay_strip_uline (clay_action_get_label (action));

  accel_string = gtk_accelerator_get_label (accel_key, accel_mask);

  confirm_data = g_slice_new (ConfirmData);

  confirm_data->clay       = view->clay;
  confirm_data->action     = edit_action;
  confirm_data->accel_key  = accel_key;
  confirm_data->accel_mask = accel_mask;

  dialog =
    clay_message_dialog_new (_("Conflicting Shortcuts"),
                             CLAY_ICON_DIALOG_WARNING,
                             gtk_widget_get_toplevel (GTK_WIDGET (view)), 0,
                             clay_standard_help_func, NULL,

                             _("_Cancel"),            GTK_RESPONSE_CANCEL,
                             _("_Reassign Shortcut"), GTK_RESPONSE_OK,

                             NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (clay_action_view_conflict_response),
                    confirm_data);

  box = CLAY_MESSAGE_DIALOG (dialog)->box;

  clay_message_box_set_primary_text (box,
                                     _("Shortcut \"%s\" is already taken "
                                       "by \"%s\" from the \"%s\" group."),
                                     accel_string, label, group->label);
  clay_message_box_set_text (box,
                             _("Reassigning the shortcut will cause it "
                               "to be removed from \"%s\"."),
                             label);

  g_free (label);
  g_free (accel_string);

  gtk_widget_set_visible (dialog, TRUE);
}

static void
clay_action_view_get_accel_action (ClayActionView  *view,
                                   const gchar     *path_string,
                                   ClayAction     **action_return,
                                   guint           *action_accel_key,
                                   GdkModifierType *action_accel_mask)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! model)
    return;

  path = gtk_tree_path_new_from_string (path_string);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      ClayAction *action;

      gtk_tree_model_get (model, &iter,
                          CLAY_ACTION_VIEW_COLUMN_ACTION,     &action,
                          CLAY_ACTION_VIEW_COLUMN_ACCEL_KEY,  action_accel_key,
                          CLAY_ACTION_VIEW_COLUMN_ACCEL_MASK, action_accel_mask,
                          -1);

      if (action)
        {
          g_object_unref (action);
          *action_return = action;
        }
    }

  gtk_tree_path_free (path);

  return;
}

static void
clay_action_view_accel_edited (GtkCellRendererAccel *accel,
                               const char           *path_string,
                               guint                 accel_key,
                               GdkModifierType       accel_mask,
                               guint                 hardware_keycode,
                               ClayActionView       *view)
{
  ClayAction      *action = NULL;
  guint            action_accel_key;
  GdkModifierType  action_accel_mask;

  clay_action_view_get_accel_action (view, path_string,
                                     &action,
                                     &action_accel_key,
                                     &action_accel_mask);

  if (! action)
    return;

  if (accel_key  == action_accel_key &&
      accel_mask == action_accel_mask)
    return;

#if defined(__APPLE__)
  /* On macOS, pressing the Command key (GDK_META_MASK) often results in
   * GDK_MOD2_MASK being added automatically by the system.
   * This causes shortcut comparisons to fail because Clay treats
   * "Command" and "Command+Mod2" as different combinations.
   * To avoid false mismatches and detect duplicates correctly,
   * we remove GDK_MOD2_MASK whenever GDK_META_MASK is present.
   */
  if ((accel_mask & (GDK_META_MASK | GDK_MOD2_MASK)) == (GDK_META_MASK | GDK_MOD2_MASK))
    accel_mask &= ~GDK_MOD2_MASK;
#endif

  if (! accel_key ||

      /* Don't allow arrow keys, they are all swallowed by the canvas
       * and cannot be invoked anyway, the same applies to space.
       */
      accel_key == GDK_KEY_Left  ||
      accel_key == GDK_KEY_Right ||
      accel_key == GDK_KEY_Up    ||
      accel_key == GDK_KEY_Down  ||
      accel_key == GDK_KEY_space ||
      accel_key == GDK_KEY_KP_Space)
    {
      clay_message_literal (view->clay,
                            G_OBJECT (view), CLAY_MESSAGE_ERROR,
                            _("Invalid shortcut."));
    }
  else if (accel_key        == GDK_KEY_F1 ||
           action_accel_key == GDK_KEY_F1)
    {
      clay_message_literal (view->clay,
                            G_OBJECT (view), CLAY_MESSAGE_ERROR,
                            _("F1 cannot be remapped."));
    }
  else if (accel_key  >= GDK_KEY_0 &&
           accel_key  <= GDK_KEY_9 &&
           accel_mask == GDK_MOD1_MASK)
    {
      clay_message (view->clay,
                    G_OBJECT (view), CLAY_MESSAGE_ERROR,
                    _("Alt+%d is used to switch to display %d and "
                      "cannot be remapped."),
                    accel_key - GDK_KEY_0,
                    accel_key == GDK_KEY_0 ? 10 : accel_key - GDK_KEY_0);
    }
  else
    {
      gchar **dup_actions;
      gchar  *accel = gtk_accelerator_name (accel_key, accel_mask);

      dup_actions = gtk_application_get_actions_for_accel (GTK_APPLICATION (view->clay->app),
                                                           accel);
      if (dup_actions != NULL && dup_actions[0] != NULL)
        {
          ClayAction *conflict_action = NULL;
          gchar      *left_paren_ptr0 = strchr (dup_actions[0], '(');

          if (left_paren_ptr0)
            *left_paren_ptr0 = '\0';     /* ignore target part of detailed name */

          for (gint i = 0; dup_actions[i] != NULL; i++)
            {
              gint   start;
              gchar *left_paren_ptr1 = strchr (dup_actions[i], '(');

              if (left_paren_ptr1)
                *left_paren_ptr1 = '\0';     /* ignore target part of detailed name */

              start = g_str_has_prefix (dup_actions[i], "app.") ? 4 : 0;
              conflict_action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (view->clay->app),
                                                                         dup_actions[i] + start));
              if (! conflict_action)
                continue;

              if (conflict_action != action)
                break;

              conflict_action = NULL;
            }

          if (conflict_action)
            clay_action_view_conflict_confirm (view, conflict_action, action,
                                               accel_key, accel_mask);
          else
            clay_message_literal (view->clay,
                                  G_OBJECT (view), CLAY_MESSAGE_ERROR,
                                  _("Changing shortcut failed."));
        }
      else
        {
          clay_action_set_accels (action, (const char*[]) { accel, NULL });
        }
      g_free (accel);
      g_strfreev (dup_actions);
    }
}

static void
clay_action_view_accel_cleared (GtkCellRendererAccel *accel,
                                const char           *path_string,
                                ClayActionView       *view)
{
  ClayAction      *action = NULL;
  guint            action_accel_key;
  GdkModifierType  action_accel_mask;

  clay_action_view_get_accel_action (view, path_string,
                                     &action,
                                     &action_accel_key,
                                     &action_accel_mask);

  if (! action)
    return;

  if (action_accel_key == GDK_KEY_F1)
    {
      clay_message_literal (view->clay,
                            G_OBJECT (view), CLAY_MESSAGE_ERROR,
                            _("F1 cannot be remapped."));
      return;
    }

  clay_action_set_accels (action, (const char*[]) { NULL });
}
