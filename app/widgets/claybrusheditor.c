/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrusheditor.c
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claybrushgenerated.h"
#include "core/claycontext.h"

#include "claybrusheditor.h"
#include "claydocked.h"
#include "clayview.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


#define BRUSH_VIEW_SIZE 96


/*  local function prototypes  */

static void   clay_brush_editor_docked_iface_init (ClayDockedInterface *face);

static void   clay_brush_editor_constructed    (GObject            *object);

static void   clay_brush_editor_set_data       (ClayDataEditor     *editor,
                                                ClayData           *data);

static void   clay_brush_editor_set_context    (ClayDocked         *docked,
                                                ClayContext        *context);

static void   clay_brush_editor_update_brush   (GtkAdjustment      *adjustment,
                                                ClayBrushEditor    *editor);
static void   clay_brush_editor_update_shape   (GtkWidget          *widget,
                                                ClayBrushEditor    *editor);
static void   clay_brush_editor_notify_brush   (ClayBrushGenerated *brush,
                                                GParamSpec         *pspec,
                                                ClayBrushEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (ClayBrushEditor, clay_brush_editor,
                         CLAY_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_brush_editor_docked_iface_init))

#define parent_class clay_brush_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_brush_editor_class_init (ClayBrushEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayDataEditorClass *editor_class = CLAY_DATA_EDITOR_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = clay_brush_editor_constructed;

  editor_class->set_data    = clay_brush_editor_set_data;
  editor_class->title       = _("Brush Editor");

  gtk_widget_class_set_css_name (widget_class, "ClayBrushEditor");
}

static void
clay_brush_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_brush_editor_set_context;
}

static void
clay_brush_editor_init (ClayBrushEditor *editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);
  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *label;
  GtkWidget      *box;
  GtkWidget      *scale;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  data_editor->view = clay_view_new_full_by_types (NULL,
                                                   CLAY_TYPE_VIEW,
                                                   CLAY_TYPE_BRUSH,
                                                   BRUSH_VIEW_SIZE,
                                                   BRUSH_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (data_editor->view, -1, BRUSH_VIEW_SIZE);
  clay_view_set_expand (CLAY_VIEW (data_editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), data_editor->view);
  gtk_widget_set_visible (data_editor->view, TRUE);

  editor->shape_group = NULL;

  editor->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->options_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->options_box, TRUE);

  /* Stock Box for the brush shape */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor->options_box), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  label = gtk_label_new (_("Shape:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  box = clay_enum_icon_box_new (CLAY_TYPE_BRUSH_GENERATED_SHAPE,
                                "clay-shape",
                                GTK_ICON_SIZE_MENU,
                                G_CALLBACK (clay_brush_editor_update_shape),
                                editor, NULL,
                                &editor->shape_group);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_set_visible (box, TRUE);

  /*  brush radius scale  */
  editor->radius_data = gtk_adjustment_new (0.0,
                                            CLAY_BRUSH_GENERATED_MIN_RADIUS,
                                            CLAY_BRUSH_GENERATED_MAX_RADIUS,
                                            1.0, 10.0, 0.0);
  scale = clay_spin_scale_new (editor->radius_data, _("Radius"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (editor->radius_data, "value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);

  /*  number of spikes  */
  editor->spikes_data = gtk_adjustment_new (2.0, 2.0, 20.0, 1.0, 1.0, 0.0);
  scale = clay_spin_scale_new (editor->spikes_data, _("Spikes"), 0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (editor->spikes_data, "value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data = gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.1, 0.0);
  scale = clay_spin_scale_new (editor->hardness_data, _("Hardness"), 2);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (editor->hardness_data, "value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data = gtk_adjustment_new (0.0, 1.0, 20.0, 0.1, 1.0, 0.0);
  scale = clay_spin_scale_new (editor->aspect_ratio_data, _("Aspect ratio"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (editor->aspect_ratio_data,"value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data = gtk_adjustment_new (0.0, 0.0, 180.0, 0.1, 1.0, 0.0);
  scale = clay_spin_scale_new (editor->angle_data, _("Angle"), 1);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (editor->angle_data, "value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);

  /*  brush spacing  */
  editor->spacing_data = gtk_adjustment_new (0.0, 1.0, 5000.0, 1.0, 10.0, 0.0);
  scale = clay_spin_scale_new (editor->spacing_data, _("Spacing"), 1);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 1.0, 200.0);
  gtk_box_pack_start (GTK_BOX (editor->options_box), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  clay_help_set_help_data (scale, _("Percentage of width of brush"), NULL);

  g_signal_connect (editor->spacing_data, "value-changed",
                    G_CALLBACK (clay_brush_editor_update_brush),
                    editor);
}

static void
clay_brush_editor_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_docked_set_show_button_bar (CLAY_DOCKED (object), FALSE);
}

static void
clay_brush_editor_set_data (ClayDataEditor *editor,
                            ClayData       *data)
{
  ClayBrushEditor         *brush_editor = CLAY_BRUSH_EDITOR (editor);
  ClayBrushGeneratedShape  shape        = CLAY_BRUSH_GENERATED_CIRCLE;
  gdouble                  radius       = 0.0;
  gint                     spikes       = 2;
  gdouble                  hardness     = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gdouble                  spacing      = 0.0;

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          clay_brush_editor_notify_brush,
                                          editor);

  CLAY_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    g_signal_connect (editor->data, "notify",
                      G_CALLBACK (clay_brush_editor_notify_brush),
                      editor);

  clay_view_set_viewable (CLAY_VIEW (editor->view), CLAY_VIEWABLE (data));

  if (editor->data)
    {
      spacing = clay_brush_get_spacing (CLAY_BRUSH (editor->data));

      if (CLAY_IS_BRUSH_GENERATED (editor->data))
        {
          ClayBrushGenerated *brush = CLAY_BRUSH_GENERATED (editor->data);

          shape    = clay_brush_generated_get_shape        (brush);
          radius   = clay_brush_generated_get_radius       (brush);
          spikes   = clay_brush_generated_get_spikes       (brush);
          hardness = clay_brush_generated_get_hardness     (brush);
          ratio    = clay_brush_generated_get_aspect_ratio (brush);
          angle    = clay_brush_generated_get_angle        (brush);
        }
    }

  gtk_widget_set_sensitive (brush_editor->options_box,
                            editor->data_editable);

  clay_int_radio_group_set_active (GTK_RADIO_BUTTON (brush_editor->shape_group),
                                   shape);

  gtk_adjustment_set_value (brush_editor->radius_data,       radius);
  gtk_adjustment_set_value (brush_editor->spikes_data,       spikes);
  gtk_adjustment_set_value (brush_editor->hardness_data,     hardness);
  gtk_adjustment_set_value (brush_editor->aspect_ratio_data, ratio);
  gtk_adjustment_set_value (brush_editor->angle_data,        angle);
  gtk_adjustment_set_value (brush_editor->spacing_data,      spacing);
}

static void
clay_brush_editor_set_context (ClayDocked  *docked,
                               ClayContext *context)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  clay_view_renderer_set_context (CLAY_VIEW (data_editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
clay_brush_editor_new (ClayContext     *context,
                       ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_BRUSH_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<BrushEditor>",
                       "ui-path",         "/brush-editor-popup",
                       "data-factory",    context->clay->brush_factory,
                       "context",         context,
                       "data",            clay_context_get_brush (context),
                       NULL);
}


/*  private functions  */

static void
clay_brush_editor_update_brush (GtkAdjustment   *adjustment,
                                ClayBrushEditor *editor)
{
  ClayBrushGenerated *brush;
  gdouble             value;

  if (! CLAY_IS_BRUSH_GENERATED (CLAY_DATA_EDITOR (editor)->data))
    return;

  brush = CLAY_BRUSH_GENERATED (CLAY_DATA_EDITOR (editor)->data);

  g_signal_handlers_block_by_func (brush,
                                   clay_brush_editor_notify_brush,
                                   editor);

  value = gtk_adjustment_get_value (adjustment);

  if (adjustment == editor->radius_data)
    {
      if (value != clay_brush_generated_get_radius (brush))
        clay_brush_generated_set_radius (brush, value);
    }
  else if (adjustment == editor->spikes_data)
    {
      if (ROUND (value) != clay_brush_generated_get_spikes (brush))
        clay_brush_generated_set_spikes (brush, ROUND (value));
    }
  else if (adjustment == editor->hardness_data)
    {
      if (value != clay_brush_generated_get_hardness (brush))
        clay_brush_generated_set_hardness (brush, value);
    }
  else if (adjustment == editor->aspect_ratio_data)
    {
      if (value != clay_brush_generated_get_aspect_ratio (brush))
        clay_brush_generated_set_aspect_ratio (brush, value);
    }
  else if (adjustment == editor->angle_data)
    {
      if (value != clay_brush_generated_get_angle (brush))
        clay_brush_generated_set_angle (brush, value);
    }
  else if (adjustment == editor->spacing_data)
    {
      if (value != clay_brush_get_spacing (CLAY_BRUSH (brush)))
        clay_brush_set_spacing (CLAY_BRUSH (brush), value);
    }

  g_signal_handlers_unblock_by_func (brush,
                                     clay_brush_editor_notify_brush,
                                     editor);
}

static void
clay_brush_editor_update_shape (GtkWidget       *widget,
                                ClayBrushEditor *editor)
{
  ClayBrushGenerated *brush;

  if (! CLAY_IS_BRUSH_GENERATED (CLAY_DATA_EDITOR (editor)->data))
    return;

  brush = CLAY_BRUSH_GENERATED (CLAY_DATA_EDITOR (editor)->data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      ClayBrushGeneratedShape shape;

      shape = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "clay-item-data"));

      if (clay_brush_generated_get_shape (brush) != shape)
        clay_brush_generated_set_shape (brush, shape);
    }
}

static void
clay_brush_editor_notify_brush (ClayBrushGenerated *brush,
                                GParamSpec         *pspec,
                                ClayBrushEditor    *editor)
{
  GtkAdjustment *adj   = NULL;
  gdouble        value = 0.0;

  if (! strcmp (pspec->name, "shape"))
    {
      g_signal_handlers_block_by_func (editor->shape_group,
                                       clay_brush_editor_update_shape,
                                       editor);

      clay_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->shape_group),
                                       brush->shape);

      g_signal_handlers_unblock_by_func (editor->shape_group,
                                         clay_brush_editor_update_shape,
                                         editor);
    }
  else if (! strcmp (pspec->name, "radius"))
    {
      adj   = editor->radius_data;
      value = clay_brush_generated_get_radius (brush);
    }
  else if (! strcmp (pspec->name, "spikes"))
    {
      adj   = editor->spikes_data;
      value = clay_brush_generated_get_spikes (brush);
    }
  else if (! strcmp (pspec->name, "hardness"))
    {
      adj   = editor->hardness_data;
      value = clay_brush_generated_get_hardness (brush);
    }
  else if (! strcmp (pspec->name, "angle"))
    {
      adj   = editor->angle_data;
      value = clay_brush_generated_get_angle (brush);
    }
  else if (! strcmp (pspec->name, "aspect-ratio"))
    {
      adj   = editor->aspect_ratio_data;
      value = clay_brush_generated_get_aspect_ratio (brush);
    }
  else if (! strcmp (pspec->name, "spacing"))
    {
      adj   = editor->spacing_data;
      value = clay_brush_get_spacing (CLAY_BRUSH (brush));
    }

  if (adj)
    {
      g_signal_handlers_block_by_func (adj,
                                       clay_brush_editor_update_brush,
                                       editor);

      gtk_adjustment_set_value (adj, value);

      g_signal_handlers_unblock_by_func (adj,
                                         clay_brush_editor_update_brush,
                                         editor);
    }
}
