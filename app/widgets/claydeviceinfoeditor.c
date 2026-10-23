/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claydeviceinfoeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycurve.h"
#include "core/claypadactions.h"

#include "clayactioneditor.h"
#include "clayactionview.h"
#include "claycurveview.h"
#include "claydeviceinfo.h"
#include "claydeviceinfoeditor.h"
#include "claydialogfactory.h"
#include "clayhelp-ids.h"
#include "clayviewabledialog.h"

#include "clay-intl.h"


#define CURVE_SIZE   256
#define CURVE_BORDER   4


enum
{
  PROP_0,
  PROP_INFO
};

enum
{
  AXIS_COLUMN_INDEX,
  AXIS_COLUMN_NAME,
  AXIS_COLUMN_INPUT_NAME,
  AXIS_N_COLUMNS
};

enum
{
  INPUT_COLUMN_INDEX,
  INPUT_COLUMN_NAME,
  INPUT_N_COLUMNS
};

enum
{
  KEY_COLUMN_INDEX,
  KEY_COLUMN_NAME,
  KEY_COLUMN_KEY,
  KEY_COLUMN_MASK,
  KEY_N_COLUMNS
};

enum
{
  PAD_COLUMN_TYPE,
  PAD_COLUMN_NUMBER,
  PAD_COLUMN_MODE,
  PAD_COLUMN_ACTION_ICON,
  PAD_COLUMN_ACTION_NAME,
  PAD_N_COLUMNS
};


typedef struct _ClayDeviceInfoEditorPrivate ClayDeviceInfoEditorPrivate;

struct _ClayDeviceInfoEditorPrivate
{
  ClayDeviceInfo   *info;

  GtkWidget        *vbox;

  GtkListStore     *pad_store;
  GtkWidget        *pad_action_view;
  GtkWidget        *pad_grab_button;
  GtkWidget        *pad_edit_button;
  GtkWidget        *pad_delete_button;
  GtkWidget        *pad_edit_dialog;

  GtkTreeSelection *pad_sel;
  GtkTreeSelection *pad_edit_sel;
};

#define CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE(editor) \
        ((ClayDeviceInfoEditorPrivate *) clay_device_info_editor_get_instance_private ((ClayDeviceInfoEditor *) (editor)))


static void   clay_device_info_editor_constructed   (GObject              *object);
static void   clay_device_info_editor_finalize      (GObject              *object);
static void   clay_device_info_editor_set_property  (GObject              *object,
                                                     guint                 property_id,
                                                     const GValue         *value,
                                                     GParamSpec           *pspec);
static void   clay_device_info_editor_get_property  (GObject              *object,
                                                     guint                 property_id,
                                                     GValue               *value,
                                                     GParamSpec           *pspec);

static void   clay_device_info_editor_curve_reset   (GtkWidget            *button,
                                                     ClayCurve            *curve);

static void clay_device_info_pad_name_column_func   (GtkTreeViewColumn    *column,
                                                     GtkCellRenderer      *cell,
                                                     GtkTreeModel         *tree_model,
                                                     GtkTreeIter          *iter,
                                                     gpointer              data);
static void clay_device_info_editor_sel_changed     (GtkTreeSelection     *sel,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_pad_row_edit    (GtkTreeView          *view,
                                                     GtkTreePath          *path,
                                                     GtkTreeViewColumn    *column,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_grab_toggled    (GtkWidget            *button,
                                                     ClayDeviceInfoEditor *editor);

static void clay_device_info_editor_edit_clicked    (GtkWidget            *button,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_delete_clicked  (GtkWidget            *button,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_edit_activated  (GtkTreeView          *tv,
                                                     GtkTreePath          *path,
                                                     GtkTreeViewColumn    *column,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_edit_response   (GtkWidget            *dialog,
                                                     gint                  response_id,
                                                     ClayDeviceInfoEditor *editor);
static void clay_device_info_editor_pad_action_foreach (ClayPadActions    *pad_actions,
                                                        ClayPadActionType  action_type,
                                                        guint              number,
                                                        guint              mode,
                                                        const gchar       *action,
                                                        gpointer           data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDeviceInfoEditor, clay_device_info_editor,
                            GTK_TYPE_BOX)

#define parent_class clay_device_info_editor_parent_class


static void
clay_device_info_editor_class_init (ClayDeviceInfoEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_device_info_editor_constructed;
  object_class->finalize     = clay_device_info_editor_finalize;
  object_class->set_property = clay_device_info_editor_set_property;
  object_class->get_property = clay_device_info_editor_get_property;

  g_object_class_install_property (object_class, PROP_INFO,
                                   g_param_spec_object ("info",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DEVICE_INFO,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_device_info_editor_init (ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  private->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (editor), private->vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (private->vbox, TRUE);
}

static void
clay_device_info_editor_constructed (GObject *object)
{
  ClayDeviceInfoEditor        *editor  = CLAY_DEVICE_INFO_EDITOR (object);
  ClayDeviceInfoEditorPrivate *private;
  GtkWidget                   *frame;
  GtkWidget                   *grid;
  GtkWidget                   *label;
  GtkWidget                   *combo;
  GtkTreeViewColumn           *column;
  GtkCellRenderer             *cell;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DEVICE_INFO (private->info));

  /*  general device information  */

  frame = clay_frame_new (_("General"));
  gtk_box_pack_start (GTK_BOX (private->vbox), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (private->vbox), frame, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  combo = clay_prop_enum_combo_box_new (G_OBJECT (private->info), "mode",
                                        0, 0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Mode:"), 0.0, 0.5,
                            combo, 1);

  label = clay_prop_enum_label_new (G_OBJECT (private->info), "source");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Source:"), 0.0, 0.5,
                            label, 1);

  label = clay_prop_label_new (G_OBJECT (private->info), "vendor-id");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Vendor ID:"), 0.0, 0.5,
                            label, 1);

  label = clay_prop_label_new (G_OBJECT (private->info), "product-id");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Product ID:"), 0.0, 0.5,
                            label, 1);

  label = clay_prop_enum_label_new (G_OBJECT (private->info), "tool-type");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("Tool type:"), 0.0, 0.5,
                            label, 1);

  label = clay_prop_label_new (G_OBJECT (private->info), "tool-serial");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                            _("Tool serial:"), 0.0, 0.5,
                            label, 1);

  label = clay_prop_label_new (G_OBJECT (private->info), "tool-hardware-id");
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 6,
                            _("Tool hardware ID:"), 0.0, 0.5,
                            label, 1);

  if (clay_device_info_get_source (private->info) == GDK_SOURCE_PEN)
    {
      /* We used to list the axes in a GtkListStore, displaying their
       * use and allowing to change it. But first of all, I don't think
       * this ever worked in the last decade (I often tried and it did
       * nothing, neither on X or Wayland); and secondly I am quite
       * unsure about the use case of changing axis use.
       * The second usage of this list was to show a curve for the
       * selected axis in the list, when available, which we only
       * supported for the pressure axis anyway. The problem with this
       * second usage is that on Wayland (at least), we only got the
       * correct list of axis once we approached the stylus to proximity
       * once. Until then, only X and Y are available. Furthermore
       * gdk_device_list_axes() was returning a list of GDK_NONE for all
       * axis (again: on Wayland, but not on X), which was how we were
       * deciding whether an axis should be ignored. So the whole thing
       * ended up very flimsy on Wayland which was not showing the
       * pressure curve.
       *
       * This simpler code does not show a list of axes anymore, does
       * not allow to edit axis use, and it always shows the pressure
       * curve for all pen devices (not verifying if it has actually the
       * pressure axis, but this would be the odd one out and this would
       * not break anything in this use case anyway).
       */
      ClayCurve *curve;
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *view;
      GtkWidget *label;
      GtkWidget *combo;
      GtkWidget *button;

      frame = clay_frame_new (_("Pressure Curve"));
      gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
      gtk_widget_set_visible (frame, TRUE);

      curve = clay_device_info_get_curve (private->info, GDK_AXIS_PRESSURE);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_box_set_spacing (GTK_BOX (vbox), 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_set_visible (vbox, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
      gtk_widget_set_visible (frame, TRUE);

      view = clay_curve_view_new ();
      g_object_set (view,
                    "clay",         CLAY_TOOL_PRESET (private->info)->clay,
                    "border-width", CURVE_BORDER,
                    NULL);
      gtk_widget_set_size_request (view,
                                   CURVE_SIZE + CURVE_BORDER * 2,
                                   CURVE_SIZE + CURVE_BORDER * 2);
      gtk_container_add (GTK_CONTAINER (frame), view);
      gtk_widget_set_visible (view, TRUE);

      clay_curve_view_set_curve (CLAY_CURVE_VIEW (view), curve, NULL);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_set_spacing (GTK_BOX (hbox), 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      label = gtk_label_new_with_mnemonic (_("Curve _type:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      combo = clay_prop_enum_combo_box_new (G_OBJECT (curve),
                                            "curve-type", 0, 0);
      clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (combo),
                                           "clay-curve");
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      button = gtk_button_new_with_mnemonic (_("_Reset Curve"));
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (clay_device_info_editor_curve_reset),
                        curve);
    }

  if (clay_device_info_get_source (private->info) == GDK_SOURCE_TABLET_PAD)
    {
      ClayPadActions *pad_actions;
      GtkWidget      *vbox;
      GtkWidget      *hbox;
      GtkWidget      *sw;
      gboolean        sensitive_pad_actions = TRUE;

      frame = clay_frame_new (_("Pad Actions"));
      gtk_box_pack_start (GTK_BOX (private->vbox), frame, TRUE, TRUE, 0);
      gtk_widget_set_visible (frame, TRUE);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_set_visible (vbox, TRUE);

      sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
      gtk_widget_set_visible (sw, TRUE);

      private->pad_action_view = gtk_tree_view_new ();
      gtk_container_add (GTK_CONTAINER (sw), private->pad_action_view);
      gtk_widget_set_visible (private->pad_action_view, TRUE);

      g_signal_connect (private->pad_action_view, "row-activated",
                        G_CALLBACK (clay_device_info_editor_pad_row_edit),
                        editor);

      gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (private->pad_action_view),
                                                  -1, _("Event"),
                                                  gtk_cell_renderer_text_new (),
                                                  clay_device_info_pad_name_column_func,
                                                  NULL, NULL);

      gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (private->pad_action_view),
                                                   -1, _("Mode"),
                                                   gtk_cell_renderer_text_new (),
                                                   "text", PAD_COLUMN_MODE,
                                                   NULL);

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Action"));
      gtk_tree_view_append_column (GTK_TREE_VIEW (private->pad_action_view),
                                   column);

      cell = gtk_cell_renderer_pixbuf_new ();
      gtk_tree_view_column_pack_start (column, cell, FALSE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "icon-name", PAD_COLUMN_ACTION_ICON,
                                           NULL);

      cell = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "text", PAD_COLUMN_ACTION_NAME,
                                           NULL);

      private->pad_store = gtk_list_store_new (PAD_N_COLUMNS,
                                               /* Pad feature type      */
                                               G_TYPE_UINT,
                                               /* Number of pad feature */
                                               G_TYPE_UINT,
                                               /* Mode that applies.    */
                                               G_TYPE_UINT,
                                               /* Action icon name.     */
                                               G_TYPE_STRING,
                                               /* Action name.          */
                                               G_TYPE_STRING);

      pad_actions = clay_device_info_get_pad_actions (private->info);
      clay_pad_actions_foreach (pad_actions, clay_device_info_editor_pad_action_foreach,
                                private->pad_store);
      gtk_tree_view_set_model (GTK_TREE_VIEW (private->pad_action_view),
                               GTK_TREE_MODEL (private->pad_store));

      private->pad_sel =
        gtk_tree_view_get_selection (GTK_TREE_VIEW (private->pad_action_view));

      g_signal_connect (private->pad_sel, "changed",
                        G_CALLBACK (clay_device_info_editor_sel_changed),
                        editor);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      private->pad_grab_button = gtk_toggle_button_new_with_mnemonic (_("_Grab event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_grab_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (private->pad_grab_button, TRUE);

      g_signal_connect (private->pad_grab_button, "toggled",
                        G_CALLBACK (clay_device_info_editor_grab_toggled),
                        editor);

      clay_help_set_help_data (private->pad_grab_button,
                               _("Select the next event arriving from "
                                 "the pad"),
                               NULL);

      private->pad_edit_button =
        gtk_button_new_with_mnemonic (_("_Edit event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_edit_button,
                          TRUE, TRUE, 0);
      gtk_widget_set_visible (private->pad_edit_button, TRUE);

      g_signal_connect (private->pad_edit_button, "clicked",
                        G_CALLBACK (clay_device_info_editor_edit_clicked),
                        editor);

      private->pad_delete_button =
        gtk_button_new_with_mnemonic (_("_Clear event"));
      gtk_box_pack_start (GTK_BOX (hbox), private->pad_delete_button,
                          TRUE, TRUE, 0);
      gtk_widget_set_visible (private->pad_delete_button, TRUE);

      g_signal_connect (private->pad_delete_button, "clicked",
                        G_CALLBACK (clay_device_info_editor_delete_clicked),
                        editor);

      gtk_widget_set_sensitive (private->pad_edit_button,   FALSE);
      gtk_widget_set_sensitive (private->pad_delete_button, FALSE);

#ifdef GDK_WINDOWING_X11
      /* We only deactivate the Pad Actions frame on X11 because on
       * macOS/Windows, there is likely no GDK_SOURCE_TABLET_PAD device
       * at all, to start with. Also there might be an implementation
       * eventually (whereas seeing this come to X11 seems improbable
       * and we don't have a generic way to see if pad actions would
       * work).
       * See: https://github.com/KurtStevenK/clay/-/merge_requests/946#note_1768869
       * See also the message, 2 comments below, explaining how this
       * could be implemented on Windows with WM_POINTER API.
       */
      if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
        sensitive_pad_actions = FALSE;
#endif

      gtk_widget_set_sensitive (frame, sensitive_pad_actions);
      if (! sensitive_pad_actions)
        gtk_widget_set_tooltip_text (frame, _("Pad Actions are not supported on your platform yet"));
    }
}

static void
clay_device_info_editor_finalize (GObject *object)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  g_clear_object (&private->info);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_device_info_editor_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      private->info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_device_info_editor_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      g_value_set_object (value, private->info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_device_info_editor_curve_reset (GtkWidget *button,
                                     ClayCurve *curve)
{
  clay_curve_reset (curve, TRUE);
}

static gchar *
clay_device_info_get_pad_feature_name (ClayPadActionType type,
                                       guint             number)
{
  gchar *str = NULL;

  switch (type)
    {
    case CLAY_PAD_ACTION_BUTTON:
      str = g_strdup_printf (_("Button %d"), number + 1);
      break;
    case CLAY_PAD_ACTION_RING:
      str = g_strdup_printf (_("Ring %d"), number + 1);
      break;
    case CLAY_PAD_ACTION_STRIP:
      str = g_strdup_printf (_("Strip %d"), number + 1);
      break;
    default:
      break;
    }

  return str;
}

static void
clay_device_info_pad_name_column_func (GtkTreeViewColumn *column,
                                       GtkCellRenderer   *cell,
                                       GtkTreeModel      *tree_model,
                                       GtkTreeIter       *iter,
                                       gpointer           data)
{
  ClayPadActionType  type;
  guint              number;
  gchar             *str = NULL;

  gtk_tree_model_get (tree_model, iter,
                      PAD_COLUMN_TYPE, &type,
                      PAD_COLUMN_NUMBER, &number,
                      -1);

  str = clay_device_info_get_pad_feature_name (type, number);
  g_object_set (G_OBJECT (cell), "text", str, NULL);
  g_free (str);
}

static void
clay_device_info_editor_sel_changed (GtkTreeSelection     *sel,
                                     ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  gchar                       *edit_help        = NULL;
  gchar                       *delete_help      = NULL;
  gboolean                     edit_sensitive   = FALSE;
  gboolean                     delete_sensitive = FALSE;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      ClayPadActionType  type;
      guint              number;
      gchar             *action = NULL;
      gchar             *event;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,        &type,
                          PAD_COLUMN_NUMBER,      &number,
                          PAD_COLUMN_ACTION_NAME, &action,
                          -1);

      event = clay_device_info_get_pad_feature_name (type, number);

      if (action)
        {
          g_free (action);

          delete_sensitive = TRUE;
          delete_help =
            g_strdup_printf (_("Remove the action assigned to '%s'"), event);
        }

      edit_sensitive = TRUE;
      edit_help = g_strdup_printf (_("Assign an action to '%s'"), event);
    }

  clay_help_set_help_data (private->pad_edit_button, edit_help, NULL);
  gtk_widget_set_sensitive (private->pad_edit_button, edit_sensitive);
  g_free (edit_help);

  clay_help_set_help_data (private->pad_delete_button, delete_help, NULL);
  gtk_widget_set_sensitive (private->pad_delete_button, delete_sensitive);
  g_free (delete_help);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);
}

static void
clay_device_info_editor_pad_row_edit (GtkTreeView          *view,
                                      GtkTreePath          *path,
                                      GtkTreeViewColumn    *column,
                                      ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  if (gtk_widget_is_sensitive (private->pad_edit_button))
    gtk_button_clicked (GTK_BUTTON (private->pad_edit_button));
}

static void
clay_device_info_editor_pad_row_ensure (ClayDeviceInfoEditor *editor,
                                        ClayPadActionType     type,
                                        guint                 number,
                                        guint                 mode)
{
  ClayDeviceInfoEditorPrivate *private;
  GtkTreeView                 *view;
  GtkTreeModel                *tree_model;
  GtkTreePath                 *path;
  GtkTreeIter                  iter;
  gint                         insert_pos = 0;
  gboolean                     iter_valid;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);
  tree_model = GTK_TREE_MODEL (private->pad_store);

  for (iter_valid = gtk_tree_model_get_iter_first (tree_model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (tree_model, &iter))
    {
      guint row_type;
      guint row_number;
      guint row_mode;

      gtk_tree_model_get (tree_model, &iter,
                          PAD_COLUMN_TYPE, &row_type,
                          PAD_COLUMN_NUMBER, &row_number,
                          PAD_COLUMN_MODE, &row_mode,
                          -1);
      if (type == row_type && number == row_number && mode == row_mode)
        goto select_item;

      if (row_type > type ||
          (row_type == type && row_number > number) ||
          (row_type == type && row_number == number && row_mode > mode))
        break;

      insert_pos++;
    }

  gtk_list_store_insert_with_values (private->pad_store, &iter, insert_pos,
                                     PAD_COLUMN_TYPE, type,
                                     PAD_COLUMN_NUMBER, number,
                                     PAD_COLUMN_MODE, mode,
                                     -1);

 select_item:

  view = GTK_TREE_VIEW (private->pad_action_view);
  path = gtk_tree_model_get_path (tree_model, &iter);
  gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0.0, 0.0);
  gtk_tree_view_set_cursor (view, path, NULL, FALSE);
  gtk_tree_path_free (path);

  gtk_widget_grab_focus (private->pad_action_view);
}

static gboolean
clay_device_info_editor_pad_events (GtkWidget            *widget,
                                    GdkEvent             *event,
                                    ClayDeviceInfoEditor *editor)
{
  switch (event->type)
    {
    case GDK_PAD_BUTTON_RELEASE:
      clay_device_info_editor_pad_row_ensure (editor,
                                              CLAY_PAD_ACTION_BUTTON,
                                              event->pad_button.button,
                                              event->pad_button.mode);
      break;
    case GDK_PAD_RING:
      clay_device_info_editor_pad_row_ensure (editor,
                                              CLAY_PAD_ACTION_RING,
                                              event->pad_axis.index,
                                              event->pad_axis.mode);
      break;
    case GDK_PAD_STRIP:
      clay_device_info_editor_pad_row_ensure (editor,
                                              CLAY_PAD_ACTION_STRIP,
                                              event->pad_axis.index,
                                              event->pad_axis.mode);
      break;
    default:
      break;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
clay_device_info_editor_grab_toggled (GtkWidget            *button,
                                      ClayDeviceInfoEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      g_signal_connect (gtk_widget_get_toplevel (button), "event",
                        G_CALLBACK (clay_device_info_editor_pad_events),
                        editor);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (gtk_widget_get_toplevel (button),
                                            clay_device_info_editor_pad_events,
                                            editor);
    }
}

static void
clay_device_info_editor_edit_clicked (GtkWidget            *button,
                                      ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  GtkWidget                   *view;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);

  if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
    {
      ClayContext       *user_context;
      ClayPadActionType  type;
      guint              number;
      gchar             *label;
      gchar             *title;
      gchar             *action_name;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,        &type,
                          PAD_COLUMN_NUMBER,      &number,
                          PAD_COLUMN_ACTION_NAME, &action_name,
                          -1);

      label = clay_device_info_get_pad_feature_name (type, number);
      title = g_strdup_printf (_("Select Action for Event '%s'"),
                               label);
      g_free (label);

      user_context =
        clay_get_user_context (CLAY_TOOL_PRESET (private->info)->clay);

      g_set_weak_pointer
        (&private->pad_edit_dialog,
         clay_viewable_dialog_new (g_list_prepend (NULL, NULL), user_context,
                                   _("Select Pad Event Action"),
                                   "clay-pad-action-dialog",
                                   CLAY_ICON_EDIT,
                                   title,
                                   gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                                   clay_standard_help_func,
                                   CLAY_HELP_PREFS_INPUT_DEVICES,

                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                   _("_OK"),     GTK_RESPONSE_OK,

                                   NULL));

      g_free (title);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (private->pad_edit_dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      clay_dialog_factory_add_foreign (clay_dialog_factory_get_singleton (),
                                       "clay-pad-action-dialog",
                                       private->pad_edit_dialog,
                                       clay_widget_get_monitor (button));

      g_signal_connect (private->pad_edit_dialog, "response",
                        G_CALLBACK (clay_device_info_editor_edit_response),
                        editor);

      view = clay_action_editor_new (CLAY_TOOL_PRESET (private->info)->clay,
                                     action_name, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (view), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (private->pad_edit_dialog))),
                          view, TRUE, TRUE, 0);
      gtk_widget_set_visible (view, TRUE);

      g_signal_connect (CLAY_ACTION_EDITOR (view)->view, "row-activated",
                        G_CALLBACK (clay_device_info_editor_edit_activated),
                        editor);

      g_set_weak_pointer
        (&private->pad_edit_sel,
         gtk_tree_view_get_selection (GTK_TREE_VIEW (CLAY_ACTION_EDITOR (view)->view)));

      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
      gtk_widget_set_visible (private->pad_edit_dialog, TRUE);
    }
}

static void
clay_device_info_editor_delete_clicked (GtkWidget            *button,
                                        ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;
  GtkTreeModel                *model;
  GtkTreeIter                  iter;
  ClayPadActions              *pad_actions;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->pad_grab_button),
                                FALSE);

  if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
    {
      ClayPadActionType type;
      guint             number;
      guint             mode;

      gtk_tree_model_get (model, &iter,
                          PAD_COLUMN_TYPE,   &type,
                          PAD_COLUMN_NUMBER, &number,
                          PAD_COLUMN_MODE,   &mode,
                          -1);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          PAD_COLUMN_ACTION_ICON, NULL,
                          PAD_COLUMN_ACTION_NAME, NULL,
                          -1);

      pad_actions = clay_device_info_get_pad_actions (private->info);
      clay_pad_actions_clear_action (pad_actions, type, number, mode);
    }

  gtk_widget_set_sensitive (button, FALSE);
}

static void
clay_device_info_editor_edit_activated (GtkTreeView          *tv,
                                        GtkTreePath          *path,
                                        GtkTreeViewColumn    *column,
                                        ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_dialog_response (GTK_DIALOG (private->pad_edit_dialog), GTK_RESPONSE_OK);
}

static void
clay_device_info_editor_edit_response (GtkWidget            *dialog,
                                       gint                  response_id,
                                       ClayDeviceInfoEditor *editor)
{
  ClayDeviceInfoEditorPrivate *private;

  private = CLAY_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gchar        *icon_name   = NULL;
      gchar        *action_name = NULL;

      if (gtk_tree_selection_get_selected (private->pad_edit_sel, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            CLAY_ACTION_VIEW_COLUMN_ICON_NAME, &icon_name,
                            CLAY_ACTION_VIEW_COLUMN_NAME,      &action_name,
                            -1);

      if (gtk_tree_selection_get_selected (private->pad_sel, &model, &iter))
        {
          if (action_name)
            {
              ClayPadActions    *pad_actions;
              ClayPadActionType  type;
              guint              number;
              guint              mode;

              gtk_tree_model_get (model, &iter,
                                  PAD_COLUMN_TYPE,   &type,
                                  PAD_COLUMN_NUMBER, &number,
                                  PAD_COLUMN_MODE,   &mode,
                                  -1);

              gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                                  PAD_COLUMN_ACTION_ICON, icon_name,
                                  PAD_COLUMN_ACTION_NAME, action_name,
                                  -1);

              pad_actions = clay_device_info_get_pad_actions (private->info);
              clay_pad_actions_set_action (pad_actions, type, number, mode,
                                           action_name);
            }
        }

      g_free (icon_name);
      g_free (action_name);

      clay_device_info_editor_sel_changed (private->pad_sel, editor);
    }

  gtk_widget_destroy (dialog);
}

static void
clay_device_info_editor_pad_action_foreach (ClayPadActions    *pad_actions,
                                            ClayPadActionType  action_type,
                                            guint              number,
                                            guint              mode,
                                            const gchar       *action,
                                            gpointer           data)
{
  GtkTreeModel *tree_model = data;

  gtk_list_store_insert_with_values (GTK_LIST_STORE (tree_model), NULL, -1,
                                     PAD_COLUMN_TYPE, action_type,
                                     PAD_COLUMN_NUMBER, number,
                                     PAD_COLUMN_MODE, mode,
                                     PAD_COLUMN_ACTION_NAME, action,
                                     -1);
}


/*  public functions  */

GtkWidget *
clay_device_info_editor_new (ClayDeviceInfo *info)
{
  g_return_val_if_fail (CLAY_IS_DEVICE_INFO (info), NULL);

  return g_object_new (CLAY_TYPE_DEVICE_INFO_EDITOR,
                       "info", info,
                       NULL);
}
