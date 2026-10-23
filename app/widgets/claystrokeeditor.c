/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claydashpattern.h"
#include "core/claystrokeoptions.h"

#include "claycellrendererdashes.h"
#include "claydasheditor.h"
#include "claystrokeeditor.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_RESOLUTION
};


static void      clay_stroke_editor_constructed  (GObject           *object);
static void      clay_stroke_editor_set_property (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void      clay_stroke_editor_get_property (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static gboolean  clay_stroke_editor_draw_button  (GtkWidget         *widget,
                                                  cairo_t           *cr,
                                                  gpointer           data);
static void      clay_stroke_editor_dash_preset  (GtkWidget         *widget,
                                                  ClayStrokeOptions *options);

static void      clay_stroke_editor_combo_fill   (ClayStrokeOptions *options,
                                                  GtkComboBox       *box);


G_DEFINE_TYPE (ClayStrokeEditor, clay_stroke_editor, CLAY_TYPE_FILL_EDITOR)

#define parent_class clay_stroke_editor_parent_class


static void
clay_stroke_editor_class_init (ClayStrokeEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_stroke_editor_constructed;
  object_class->set_property = clay_stroke_editor_set_property;
  object_class->get_property = clay_stroke_editor_get_property;

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options", NULL, NULL,
                                                        CLAY_TYPE_STROKE_OPTIONS,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_RESOLUTION,
                                   g_param_spec_double ("resolution", NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        72.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_stroke_editor_init (ClayStrokeEditor *editor)
{
}

static void
clay_stroke_editor_constructed (GObject *object)
{
  ClayFillEditor    *fill_editor = CLAY_FILL_EDITOR (object);
  ClayStrokeEditor  *editor      = CLAY_STROKE_EDITOR (object);
  ClayStrokeOptions *options;
  ClayEnumStore     *store;
  GEnumClass        *enum_class;
  GEnumValue        *value;
  GtkWidget         *scale;
  GtkWidget         *box;
  GtkWidget         *size;
  GtkWidget         *frame;
  GtkWidget         *grid;
  GtkWidget         *dash_editor;
  GtkWidget         *button;
  GtkCellRenderer   *cell;
  gint               row = 0;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_STROKE_OPTIONS (fill_editor->options));

  options = CLAY_STROKE_OPTIONS (fill_editor->options);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 3);
  gtk_container_add (GTK_CONTAINER (editor), grid);
  gtk_widget_set_visible (grid, TRUE);

  size = clay_prop_size_entry_new (G_OBJECT (options),
                                   "width", FALSE, "unit",
                                   "%a", CLAY_SIZE_ENTRY_UPDATE_SIZE,
                                   editor->resolution);
  clay_size_entry_set_pixel_digits (CLAY_SIZE_ENTRY (size), 1);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Line width:"), 0.0, 0.5,
                            size, 2);

  box = clay_prop_enum_icon_box_new (G_OBJECT (options), "cap-style",
                                     "clay-cap", 0, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_START);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Cap style:"), 0.0, 0.5,
                            box, 2);

  box = clay_prop_enum_icon_box_new (G_OBJECT (options), "join-style",
                                     "clay-join", 0, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_START);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Join style:"), 0.0, 0.5,
                            box, 2);

  scale = clay_prop_spin_scale_new (G_OBJECT (options), "miter-limit",
                                    1.0, 10.0, 1);
  clay_spin_scale_set_label (CLAY_SPIN_SCALE (scale), NULL);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Miter limit:"),
                            0.0, 0.5, scale, 2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Dash pattern:"), 0.0, 0.5,
                            frame, 2);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_set_visible (box, TRUE);

  dash_editor = clay_dash_editor_new (options);

  button = g_object_new (GTK_TYPE_BUTTON,
                         "width-request", 14,
                         NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (clay_dash_editor_shift_left),
                           dash_editor, G_CONNECT_SWAPPED);
  g_signal_connect_after (button, "draw",
                          G_CALLBACK (clay_stroke_editor_draw_button),
                          button);

  gtk_box_pack_start (GTK_BOX (box), dash_editor, TRUE, TRUE, 0);
  gtk_widget_set_visible (dash_editor, TRUE);

  button = g_object_new (GTK_TYPE_BUTTON,
                         "width-request", 14,
                         NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (clay_dash_editor_shift_right),
                           dash_editor, G_CONNECT_SWAPPED);
  g_signal_connect_after (button, "draw",
                          G_CALLBACK (clay_stroke_editor_draw_button),
                          NULL);


  store = g_object_new (CLAY_TYPE_ENUM_STORE,
                        "enum-type",      CLAY_TYPE_DASH_PRESET,
                        "user-data-type", CLAY_TYPE_DASH_PATTERN,
                        NULL);

  enum_class = g_type_class_ref (CLAY_TYPE_DASH_PRESET);

  for (value = enum_class->values; value->value_name; value++)
    {
      GtkTreeIter  iter = { 0, };
      const gchar *desc;

      desc = clay_enum_value_get_desc (enum_class, value);

      gtk_list_store_append (GTK_LIST_STORE (store), &iter);
      gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                          CLAY_INT_STORE_VALUE, value->value,
                          CLAY_INT_STORE_LABEL, desc,
                          -1);
    }

  g_type_class_unref (enum_class);

  box = clay_enum_combo_box_new_with_model (store);
  g_object_unref (store);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (box), CLAY_DASH_CUSTOM);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Dash _preset:"), 0.0, 0.5,
                            box, 2);

  cell = g_object_new (CLAY_TYPE_CELL_RENDERER_DASHES,
                       "xpad", 2,
                       NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (box), cell,
                                 "pattern", CLAY_INT_STORE_USER_DATA);

  clay_stroke_editor_combo_fill (options, GTK_COMBO_BOX (box));

  g_signal_connect (box, "changed",
                    G_CALLBACK (clay_stroke_editor_dash_preset),
                    options);
  g_signal_connect_object (options, "dash-info-changed",
                           G_CALLBACK (clay_int_combo_box_set_active),
                           box, G_CONNECT_SWAPPED);
}

static void
clay_stroke_editor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayFillEditor   *fill_editor = CLAY_FILL_EDITOR (object);
  ClayStrokeEditor *editor      = CLAY_STROKE_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      fill_editor->options = g_value_dup_object (value);
      break;

    case PROP_RESOLUTION:
      editor->resolution = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_stroke_editor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayFillEditor   *fill_editor = CLAY_FILL_EDITOR (object);
  ClayStrokeEditor *editor      = CLAY_STROKE_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, fill_editor->options);
      break;

    case PROP_RESOLUTION:
      g_value_set_double (value, editor->resolution);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_stroke_editor_new (ClayStrokeOptions *options,
                        gdouble            resolution,
                        gboolean           edit_context,
                        gboolean           use_custom_style)
{
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (options), NULL);

  return g_object_new (CLAY_TYPE_STROKE_EDITOR,
                       "options",          options,
                       "resolution",       resolution,
                       "edit-context",     edit_context ? TRUE : FALSE,
                       "use-custom-style", use_custom_style ? TRUE: FALSE,
                       NULL);
}

static gboolean
clay_stroke_editor_draw_button (GtkWidget *widget,
                                cairo_t   *cr,
                                gpointer   data)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  gint             w;

  gtk_widget_get_allocation (widget, &allocation);

  w = MIN (allocation.width, allocation.height) * 2 / 3;

  gtk_style_context_save (style);
  gtk_style_context_set_state (style, gtk_widget_get_state_flags (widget));

  gtk_render_arrow (style, cr,
                    data ? 3 * (G_PI / 2) : 1 * (G_PI / 2),
                    (allocation.width  - w) / 2,
                    (allocation.height - w) / 2,
                    w);

  gtk_style_context_restore (style);

  return FALSE;
}

static void
clay_stroke_editor_dash_preset (GtkWidget         *widget,
                                ClayStrokeOptions *options)
{
  gint value;

  if (clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (widget), &value) &&
      value != CLAY_DASH_CUSTOM)
    {
      clay_stroke_options_take_dash_pattern (options, value, NULL);
    }
}

static void
clay_stroke_editor_combo_update (GtkTreeModel      *model,
                                 GParamSpec        *pspec,
                                 ClayStrokeOptions *options)
{
  GtkTreeIter iter;

  if (clay_int_store_lookup_by_value (model, CLAY_DASH_CUSTOM, &iter))
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          CLAY_INT_STORE_USER_DATA,
                          clay_stroke_options_get_dash_info (options),
                          -1);
    }
}

static void
clay_stroke_editor_combo_fill (ClayStrokeOptions *options,
                               GtkComboBox       *box)
{
  GtkTreeModel *model = gtk_combo_box_get_model (box);
  GtkTreeIter   iter;
  gboolean      iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint value;

      gtk_tree_model_get (model, &iter,
                          CLAY_INT_STORE_VALUE, &value,
                          -1);

      if (value == CLAY_DASH_CUSTOM)
        {
          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              CLAY_INT_STORE_USER_DATA,
                              clay_stroke_options_get_dash_info (options),
                              -1);

          g_signal_connect_object (options, "notify::dash-info",
                                   G_CALLBACK (clay_stroke_editor_combo_update),
                                   model, G_CONNECT_SWAPPED);
        }
      else
        {
          GArray *pattern = clay_dash_pattern_new_from_preset (value);

          gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                              CLAY_INT_STORE_USER_DATA, pattern,
                              -1);
          clay_dash_pattern_free (pattern);
        }
    }
}
