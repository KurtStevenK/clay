/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGridEditor
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "core/claycontext.h"
#include "core/claygrid.h"

#include "claycolorpanel.h"
#include "claygrideditor.h"
#include "claypropwidgets.h"

#include "clay-intl.h"


#define GRID_EDITOR_DEFAULT_RESOLUTION   72.0
#define GRID_EDITOR_COLOR_BUTTON_WIDTH   60
#define GRID_EDITOR_COLOR_BUTTON_HEIGHT  24


enum
{
  PROP_0,
  PROP_GRID,
  PROP_CONTEXT,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION
};


static void   clay_grid_editor_constructed  (GObject      *object);
static void   clay_grid_editor_finalize     (GObject      *object);
static void   clay_grid_editor_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   clay_grid_editor_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE (ClayGridEditor, clay_grid_editor, GTK_TYPE_BOX)

#define parent_class clay_grid_editor_parent_class


static void
clay_grid_editor_class_init (ClayGridEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_grid_editor_constructed;
  object_class->set_property = clay_grid_editor_set_property;
  object_class->get_property = clay_grid_editor_get_property;
  object_class->finalize     = clay_grid_editor_finalize;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        CLAY_TYPE_GRID,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_XRESOLUTION,
                                   g_param_spec_double ("xresolution", NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        GRID_EDITOR_DEFAULT_RESOLUTION,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_YRESOLUTION,
                                   g_param_spec_double ("yresolution", NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        GRID_EDITOR_DEFAULT_RESOLUTION,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_grid_editor_init (ClayGridEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);
}

static void
clay_grid_editor_constructed (GObject *object)
{
  ClayGridEditor *editor = CLAY_GRID_EDITOR (object);
  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *grid;
  GtkWidget      *style;
  GtkWidget      *color_button;
  GtkWidget      *sizeentry;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (editor->grid != NULL);

  frame = clay_frame_new (_("Appearance"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);

  style = clay_prop_enum_combo_box_new (G_OBJECT (editor->grid), "style",
                                        CLAY_GRID_DOTS,
                                        CLAY_GRID_SOLID);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Line _style:"), 0.0, 0.5,
                            style, 1);

  color_button = clay_prop_color_button_new (G_OBJECT (editor->grid), "fgcolor",
                                             _("Change grid foreground color"),
                                             FALSE,
                                             GRID_EDITOR_COLOR_BUTTON_WIDTH,
                                             GRID_EDITOR_COLOR_BUTTON_HEIGHT,
                                             CLAY_COLOR_AREA_FLAT);
  gtk_widget_set_halign (color_button, GTK_ALIGN_START);
  clay_color_panel_set_context (CLAY_COLOR_PANEL (color_button),
                                editor->context);
  clay_color_panel_set_user_context_aware  (CLAY_COLOR_PANEL (color_button),
                                            FALSE);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Foreground color:"), 0.0, 0.5,
                            color_button, 1);

  color_button = clay_prop_color_button_new (G_OBJECT (editor->grid), "bgcolor",
                                             _("Change grid background color"),
                                             FALSE,
                                             GRID_EDITOR_COLOR_BUTTON_WIDTH,
                                             GRID_EDITOR_COLOR_BUTTON_HEIGHT,
                                             CLAY_COLOR_AREA_FLAT);
  gtk_widget_set_halign (color_button, GTK_ALIGN_START);
  clay_color_panel_set_context (CLAY_COLOR_PANEL (color_button),
                                editor->context);
  clay_color_panel_set_user_context_aware  (CLAY_COLOR_PANEL (color_button),
                                            FALSE);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Background color:"), 0.0, 0.5,
                            color_button, 1);

  gtk_widget_set_visible (grid, TRUE);

  frame = clay_frame_new (_("Spacing"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry = clay_prop_coordinates_new (G_OBJECT (editor->grid),
                                         "xspacing",
                                         "yspacing",
                                         "spacing-unit",
                                         "%a",
                                         CLAY_SIZE_ENTRY_UPDATE_SIZE,
                                         editor->xresolution,
                                         editor->yresolution,
                                         TRUE);

  gtk_grid_set_column_spacing (GTK_GRID (sizeentry), 2);
  gtk_grid_set_row_spacing (GTK_GRID (sizeentry), 2);

  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Horizontal"), 0, 1, 0.0);
  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Vertical"), 0, 2, 0.0);
  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Pixels"), 1, 4, 0.0);

  clay_size_entry_set_refval_digits (CLAY_SIZE_ENTRY (sizeentry), 0, 2);
  clay_size_entry_set_refval_digits (CLAY_SIZE_ENTRY (sizeentry), 1, 2);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);

  gtk_widget_set_visible (hbox, TRUE);

  frame = clay_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  sizeentry = clay_prop_coordinates_new (G_OBJECT (editor->grid),
                                         "xoffset",
                                         "yoffset",
                                         "offset-unit",
                                         "%a",
                                         CLAY_SIZE_ENTRY_UPDATE_SIZE,
                                         editor->xresolution,
                                         editor->yresolution,
                                         TRUE);

  gtk_grid_set_column_spacing (GTK_GRID (sizeentry), 2);
  gtk_grid_set_row_spacing (GTK_GRID (sizeentry), 2);

  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Horizontal"), 0, 1, 0.0);
  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Vertical"), 0, 2, 0.0);
  clay_size_entry_attach_label (CLAY_SIZE_ENTRY (sizeentry),
                                _("Pixels"), 1, 4, 0.0);

  clay_size_entry_set_refval_digits (CLAY_SIZE_ENTRY (sizeentry), 0, 2);
  clay_size_entry_set_refval_digits (CLAY_SIZE_ENTRY (sizeentry), 1, 2);

  gtk_box_pack_start (GTK_BOX (hbox), sizeentry, FALSE, FALSE, 0);

  gtk_widget_set_visible (hbox, TRUE);
}

static void
clay_grid_editor_finalize (GObject *object)
{
  ClayGridEditor *editor = CLAY_GRID_EDITOR (object);

  g_clear_object (&editor->grid);
  g_clear_object (&editor->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_grid_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayGridEditor *editor = CLAY_GRID_EDITOR (object);

  switch (property_id)
    {
    case PROP_GRID:
      editor->grid = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      editor->context = g_value_dup_object (value);
      break;

    case PROP_XRESOLUTION:
      editor->xresolution = g_value_get_double (value);
      break;

    case PROP_YRESOLUTION:
      editor->yresolution = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_grid_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayGridEditor *editor = CLAY_GRID_EDITOR (object);

  switch (property_id)
    {
    case PROP_GRID:
      g_value_set_object (value, editor->grid);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;

    case PROP_XRESOLUTION:
      g_value_set_double (value, editor->xresolution);
      break;

    case PROP_YRESOLUTION:
      g_value_set_double (value, editor->yresolution);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_grid_editor_new (ClayGrid    *grid,
                      ClayContext *context,
                      gdouble      xresolution,
                      gdouble      yresolution)
{
  g_return_val_if_fail (CLAY_IS_GRID (grid), NULL);

  return g_object_new (CLAY_TYPE_GRID_EDITOR,
                       "grid",        grid,
                       "context",     context,
                       "xresolution", xresolution,
                       "yresolution", yresolution,
                       NULL);
}
