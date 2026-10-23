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

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/claytoolinfo.h"

#include "actions/filters-actions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"

#include "clayfilteroptions.h"
#include "claygegltool.h"

#include "clay-intl.h"


enum
{
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_ICON_NAME,
  N_COLUMNS
};


/*  local function prototypes  */

static void   clay_gegl_tool_control           (ClayTool       *tool,
                                                ClayToolAction  action,
                                                ClayDisplay    *display);

static void   clay_gegl_tool_dialog            (ClayFilterTool *filter_tool);

static void   clay_gegl_tool_halt              (ClayGeglTool   *gegl_tool);

static void   clay_gegl_tool_operation_changed (GtkWidget      *widget,
                                                ClayGeglTool   *gegl_tool);


G_DEFINE_TYPE (ClayGeglTool, clay_gegl_tool, CLAY_TYPE_OPERATION_TOOL)

#define parent_class clay_gegl_tool_parent_class


void
clay_gegl_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_GEGL_TOOL,
                CLAY_TYPE_FILTER_OPTIONS,
                clay_color_options_gui,
                0,
                "clay-gegl-tool",
                _("GEGL Operation"),
                _("Run an arbitrary GEGL operation"),
                N_("_GEGL Operation..."), NULL,
                NULL, CLAY_HELP_TOOL_GEGL,
                CLAY_ICON_GEGL,
                data);
}

static void
clay_gegl_tool_class_init (ClayGeglToolClass *klass)
{
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  tool_class->control       = clay_gegl_tool_control;

  filter_tool_class->dialog = clay_gegl_tool_dialog;
}

static void
clay_gegl_tool_init (ClayGeglTool *tool)
{
}

static void
clay_gegl_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayGeglTool *gegl_tool = CLAY_GEGL_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_gegl_tool_halt (gegl_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}


/*****************/
/*  Gegl dialog  */
/*****************/

static void
clay_gegl_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayGeglTool      *tool   = CLAY_GEGL_TOOL (filter_tool);
  ClayOperationTool *o_tool = CLAY_OPERATION_TOOL (filter_tool);
  GtkListStore      *store;
  GtkCellRenderer   *cell;
  GtkWidget         *main_vbox;
  GtkWidget         *hbox;
  GtkWidget         *combo;
  GtkWidget         *options_gui;
  GtkWidget         *options_box;
  GList             *opclasses;
  GList             *iter;

  CLAY_FILTER_TOOL_CLASS (parent_class)->dialog (filter_tool);

  options_box = g_weak_ref_get (&o_tool->options_box_ref);
  g_return_if_fail (options_box);

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

  /*  The operation combo box  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), hbox, 0);
  gtk_widget_set_visible (hbox, TRUE);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  opclasses = clay_gegl_get_op_classes (TRUE);

  for (iter = opclasses; iter; iter = iter->next)
    {
      GeglOperationClass *opclass   = GEGL_OPERATION_CLASS (iter->data);
      const gchar        *icon_name = NULL;
      const gchar        *op_name   = opclass->name;
      const gchar        *title;
      gchar              *label;

      if (filters_actions_gegl_op_blocklisted (opclass->name))
        continue;

      if (g_str_has_prefix (opclass->name, "gegl:"))
        icon_name = CLAY_ICON_GEGL;

      if (g_str_has_prefix (op_name, "gegl:"))
        op_name += strlen ("gegl:");

      title = gegl_operation_class_get_key (opclass, "title");

      if (title)
        label = g_strdup_printf ("%s (%s)", title, op_name);
      else
        label = g_strdup (op_name);

      gtk_list_store_insert_with_values (store, NULL, -1,
                                         COLUMN_NAME,      opclass->name,
                                         COLUMN_LABEL,     label,
                                         COLUMN_ICON_NAME, icon_name,
                                         -1);

      g_free (label);
    }

  g_list_free (opclasses);

  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_set_visible (combo, TRUE);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "icon-name", COLUMN_ICON_NAME);

  cell = gtk_cell_renderer_text_new ();
  g_object_set (cell,
                "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "text", COLUMN_LABEL);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (clay_gegl_tool_operation_changed),
                    tool);

  tool->operation_combo = combo;

  tool->description_label = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (tool->description_label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (tool->description_label), 0.0);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->description_label,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), tool->description_label, 1);

  /*  The options vbox  */
  options_gui = gtk_label_new (_("Select an operation from the list above"));
  clay_label_set_attributes (GTK_LABEL (options_gui),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  g_object_set (options_gui,
                "margin-top",    4,
                "margin-bottom", 4,
                NULL);
  gtk_container_add (GTK_CONTAINER (options_box), options_gui);
  g_object_unref (options_box);
  g_weak_ref_set (&o_tool->options_gui_ref, options_gui);
  gtk_widget_set_visible (options_gui, TRUE);
}

static void
clay_gegl_tool_halt (ClayGeglTool *gegl_tool)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (gegl_tool);

  clay_operation_tool_set_operation (op_tool, NULL, NULL,
                                     NULL, NULL, NULL, NULL, NULL);
}

static void
clay_gegl_tool_operation_changed (GtkWidget    *widget,
                                  ClayGeglTool *tool)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *operation;

  if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  gtk_tree_model_get (model, &iter,
                      COLUMN_NAME, &operation,
                      -1);

  if (operation)
    {
      const gchar *title;
      const gchar *description;

      title       = gegl_operation_get_key (operation, "title");
      description = gegl_operation_get_key (operation, "description");

      if (! title)
        title = gegl_operation_get_key (operation, "name");

      if (description)
        {
          gtk_label_set_text (GTK_LABEL (tool->description_label), description);
          gtk_widget_set_visible (tool->description_label, TRUE);
        }
      else
        {
          gtk_widget_set_visible (tool->description_label, FALSE);
        }

      clay_operation_tool_set_operation (CLAY_OPERATION_TOOL (tool),
                                         NULL,
                                         operation,
                                         _("GEGL Operation"),
                                         title ?
                                         title : _("GEGL Operation"),
                                         NULL,
                                         CLAY_ICON_GEGL,
                                         CLAY_HELP_TOOL_GEGL);
      g_free (operation);
    }
}
