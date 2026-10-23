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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/claytoolinfo.h"

#include "paint/claypaintoptions.h"

#include "widgets/claylayermodebox.h"
#include "widgets/claypropwidgets.h"
#include "widgets/clayviewablebox.h"
#include "widgets/claywidgets-constructors.h"
#include "widgets/claywidgets-utils.h"

#include "clayairbrushtool.h"
#include "clayclonetool.h"
#include "clayconvolvetool.h"
#include "claydodgeburntool.h"
#include "clayerasertool.h"
#include "clayhealtool.h"
#include "clayinktool.h"
#include "claymybrushtool.h"
#include "claypaintoptions-gui.h"
#include "claypenciltool.h"
#include "clayperspectiveclonetool.h"
#include "claysmudgetool.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


static void clay_paint_options_gui_reset_size  (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);
static void clay_paint_options_gui_reset_aspect_ratio
                                               (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);
static void clay_paint_options_gui_reset_angle (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);
static void clay_paint_options_gui_reset_spacing
                                               (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);
static void clay_paint_options_gui_reset_hardness
                                               (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);
static void clay_paint_options_gui_reset_force (GtkWidget        *button,
                                                ClayPaintOptions *paint_options);

static GtkWidget * dynamics_options_gui        (ClayPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * jitter_options_gui          (ClayPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * smoothing_options_gui       (ClayPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * expand_options_gui          (ClayPaintOptions *paint_options,
                                                GType             tool_type);

static GtkWidget * clay_paint_options_gui_scale_with_buttons
                                               (GObject      *config,
                                                gchar        *prop_name,
                                                gchar        *link_prop_name,
                                                gchar        *reset_tooltip,
                                                gdouble       step_increment,
                                                gdouble       page_increment,
                                                gint          digits,
                                                gdouble       scale_min,
                                                gdouble       scale_max,
                                                gdouble       factor,
                                                gdouble       gamma,
                                                GCallback     reset_callback,
                                                GtkSizeGroup *link_group);


/*  public functions  */

GtkWidget *
clay_paint_options_gui (ClayToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  ClayPaintOptions *options = CLAY_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = clay_tool_options_gui (tool_options);
  GtkWidget        *menu;
  GtkWidget        *scale;
  GType             tool_type;

  tool_type = tool_options->tool_info->tool_type;

  /*  the paint mode menu  */
  menu = clay_prop_layer_mode_box_new (config, "paint-mode",
                                       CLAY_LAYER_MODE_CONTEXT_PAINT);
  clay_layer_mode_box_set_label (CLAY_LAYER_MODE_BOX (menu), _("Mode"));
  clay_layer_mode_box_set_ellipsize (CLAY_LAYER_MODE_BOX (menu),
                                     PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (vbox),
                     "clay-paint-options-gui-paint-mode-box", menu);

  if (tool_type == CLAY_TYPE_ERASER_TOOL     ||
      tool_type == CLAY_TYPE_CONVOLVE_TOOL   ||
      tool_type == CLAY_TYPE_DODGE_BURN_TOOL ||
      tool_type == CLAY_TYPE_HEAL_TOOL       ||
      tool_type == CLAY_TYPE_MYBRUSH_TOOL    ||
      tool_type == CLAY_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (menu, FALSE);
    }

  /*  the opacity scale  */
  scale = clay_prop_spin_scale_new (config, "opacity", 0.01, 0.1, 0);
  clay_spin_scale_set_constrain_drag (CLAY_SPIN_SCALE (scale), TRUE);
  clay_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /*  temp debug foo, disabled in stable  */
  if (FALSE &&
      g_type_is_a (tool_type, CLAY_TYPE_PAINT_TOOL) &&
      tool_type != CLAY_TYPE_MYBRUSH_TOOL)
    {
      GtkWidget *button;

      button = clay_prop_check_button_new (config, "use-applicator", NULL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    }

  /*  the brush  */
  if (g_type_is_a (tool_type, CLAY_TYPE_BRUSH_TOOL))
    {
      GtkSizeGroup *link_group;
      GtkWidget    *button;
      GtkWidget    *frame;
      GtkWidget    *hbox;

      button = clay_prop_brush_box_new (NULL, CLAY_CONTEXT (tool_options),
                                        _("Brush"), 2,
                                        "brush-view-type", "brush-view-size",
                                        "clay-brush-editor",
                                        _("Edit this brush"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      link_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      hbox = clay_paint_options_gui_scale_with_buttons
        (config, "brush-size", "brush-link-size",
         _("Reset size to brush's native size"),
         1.0, 10.0, 2, 1.0, 1000.0, 1.0, 1.7,
         G_CALLBACK (clay_paint_options_gui_reset_size), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      hbox = clay_paint_options_gui_scale_with_buttons
        (config, "brush-aspect-ratio", "brush-link-aspect-ratio",
         _("Reset aspect ratio to brush's native aspect ratio"),
         0.1, 1.0, 2, -20.0, 20.0, 1.0, 1.0,
         G_CALLBACK (clay_paint_options_gui_reset_aspect_ratio), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      hbox = clay_paint_options_gui_scale_with_buttons
        (config, "brush-angle", "brush-link-angle",
         _("Reset angle to brush's native angle"),
         0.1, 1.0, 2, -180.0, 180.0, 1.0, 1.0,
         G_CALLBACK (clay_paint_options_gui_reset_angle), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      hbox = clay_paint_options_gui_scale_with_buttons
        (config, "brush-spacing", "brush-link-spacing",
         _("Reset spacing to brush's native spacing"),
         0.1, 1.0, 1, 1.0, 200.0, 100.0, 1.7,
         G_CALLBACK (clay_paint_options_gui_reset_spacing), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      hbox = clay_paint_options_gui_scale_with_buttons
        (config, "brush-hardness", "brush-link-hardness",
         _("Reset hardness to brush's native hardness"),
         0.1, 1.0, 1, 0.0, 100.0, 100.0, 1.0,
         G_CALLBACK (clay_paint_options_gui_reset_hardness), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      if (tool_type != CLAY_TYPE_PENCIL_TOOL)
        {
          hbox = clay_paint_options_gui_scale_with_buttons (config, "brush-force",
                                                            NULL,
                                                            _("Reset force to default"),
                                                            0.1, 1.0, 1, 0.0,
                                                            100.0, 100.0, 1.0,
                                                            G_CALLBACK (clay_paint_options_gui_reset_force),
                                                            link_group);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_set_visible (hbox, TRUE);
        }

      g_object_unref (link_group);

      frame = dynamics_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);

      frame = jitter_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);
    }

  /*  the "smooth stroke" options  */
  if (g_type_is_a (tool_type, CLAY_TYPE_PAINT_TOOL))
    {
      GtkWidget *frame;

      frame = smoothing_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);
    }

  /*  the "Lock brush to view" toggle  */
  if (g_type_is_a (tool_type, CLAY_TYPE_BRUSH_TOOL))
    {
      GtkWidget *button;

      button = clay_prop_check_button_new (config, "brush-lock-to-view", NULL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    }

  /*  the "incremental" toggle  */
  if (tool_type == CLAY_TYPE_PENCIL_TOOL     ||
      tool_type == CLAY_TYPE_PAINTBRUSH_TOOL ||
      tool_type == CLAY_TYPE_ERASER_TOOL     ||
      tool_type == CLAY_TYPE_DODGE_BURN_TOOL)
    {
      GtkWidget *button;

      button = clay_prop_enum_check_button_new (config, "application-mode",
                                                NULL,
                                                CLAY_PAINT_CONSTANT,
                                                CLAY_PAINT_INCREMENTAL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    }

  /* the "hard edge" toggle */
  if (tool_type == CLAY_TYPE_ERASER_TOOL            ||
      tool_type == CLAY_TYPE_CLONE_TOOL             ||
      tool_type == CLAY_TYPE_HEAL_TOOL              ||
      tool_type == CLAY_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == CLAY_TYPE_CONVOLVE_TOOL          ||
      tool_type == CLAY_TYPE_DODGE_BURN_TOOL        ||
      tool_type == CLAY_TYPE_SMUDGE_TOOL)
    {
      GtkWidget *button;

      button = clay_prop_check_button_new (config, "hard", NULL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    }

  /* the "expand layers" options */
  if (tool_type == CLAY_TYPE_PAINTBRUSH_TOOL ||
      tool_type == CLAY_TYPE_PENCIL_TOOL     ||
      tool_type == CLAY_TYPE_AIRBRUSH_TOOL   ||
      tool_type == CLAY_TYPE_CLONE_TOOL      ||
      tool_type == CLAY_TYPE_HEAL_TOOL       ||
      tool_type == CLAY_TYPE_CONVOLVE_TOOL   ||
      tool_type == CLAY_TYPE_SMUDGE_TOOL)
    {
      GtkWidget *frame;

      frame = expand_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);
    }

  return vbox;
}

GtkWidget *
clay_paint_options_gui_get_paint_mode_box (GtkWidget *options_gui)
{
  return g_object_get_data (G_OBJECT (options_gui),
                            "clay-paint-options-gui-paint-mode-box");
}


/*  private functions  */

static GtkWidget *
dynamics_options_gui (ClayPaintOptions *paint_options,
                      GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *inner_frame;
  GtkWidget *scale;
  GtkWidget *menu;
  GtkWidget *combo;
  GtkWidget *checkbox;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *inner_vbox;
  GtkWidget *hbox;
  GtkWidget *box;


  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  frame = clay_prop_expanding_frame_new (config, "dynamics-enabled",
                                         NULL, vbox, NULL);
  button = clay_prop_dynamics_box_new (NULL,
                                       CLAY_CONTEXT (config),
                                       _("Dynamics"), 2,
                                       "dynamics-view-type",
                                       "dynamics-view-size",
                                       "clay-dynamics-editor",
                                       _("Edit this dynamics"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  inner_frame = clay_frame_new (_("Fade Options"));
  gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (inner_frame, TRUE);

  inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (inner_frame), inner_vbox);
  gtk_widget_set_visible (inner_vbox, TRUE);

  /*  the fade-out scale & unitmenu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  scale = clay_prop_spin_scale_new (config, "fade-length",
                                    1.0, 50.0, 0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  menu = clay_prop_unit_combo_box_new (config, "fade-unit");
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

#if 0
  /* FIXME pixel digits */
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  clay_unit_menu_set_pixel_digits (CLAY_UNIT_MENU (menu), 0);
#endif

  /*  the repeat type  */
  combo = clay_prop_enum_combo_box_new (config, "fade-repeat", 0, 0);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Repeat"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (inner_vbox), combo, TRUE, TRUE, 0);

  checkbox = clay_prop_check_button_new (config, "fade-reverse", NULL);
  gtk_box_pack_start (GTK_BOX (inner_vbox), checkbox, FALSE, FALSE, 0);

  /* Color UI */
  if (g_type_is_a (tool_type, CLAY_TYPE_PAINTBRUSH_TOOL) ||
      tool_type == CLAY_TYPE_SMUDGE_TOOL)
    {
      inner_frame = clay_frame_new (_("Color Options"));
      gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (inner_frame, TRUE);

      inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_container_add (GTK_CONTAINER (inner_frame), inner_vbox);
      gtk_widget_set_visible (inner_vbox, TRUE);

      box = clay_prop_gradient_box_new (NULL, CLAY_CONTEXT (config),
                                        _("Gradient"), 2,
                                        "gradient-view-type",
                                        "gradient-view-size",
                                        "gradient-reverse",
                                        "gradient-blend-color-space",
                                        "clay-gradient-editor",
                                        _("Edit this gradient"));
      gtk_box_pack_start (GTK_BOX (inner_vbox), box, FALSE, FALSE, 0);

      /*  the blend color space  */
      combo = clay_prop_enum_combo_box_new (config, "gradient-blend-color-space",
                                            0, 0);
      clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo),
                                    _("Blend Color Space"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (inner_vbox), combo, TRUE, TRUE, 0);
    }

  return frame;
}

static GtkWidget *
jitter_options_gui (ClayPaintOptions *paint_options,
                    GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *scale;

  scale = clay_prop_spin_scale_new (config, "jitter-amount",
                                    0.01, 1.0, 2);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0.0, 5.0);

  frame = clay_prop_expanding_frame_new (config, "use-jitter", NULL,
                                         scale, NULL);

  return frame;
}

static GtkWidget *
smoothing_options_gui (ClayPaintOptions *paint_options,
                       GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  frame = clay_prop_expanding_frame_new (config, "use-smoothing", NULL,
                                         vbox, NULL);

  scale = clay_prop_spin_scale_new (config, "smoothing-quality",
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = clay_prop_spin_scale_new (config, "smoothing-factor",
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return frame;
}

static void
clay_paint_options_gui_reset_size (GtkWidget        *button,
                                   ClayPaintOptions *paint_options)
{
  ClayBrush *brush = clay_context_get_brush (CLAY_CONTEXT (paint_options));

  if (brush)
    clay_paint_options_set_default_brush_size (paint_options, brush);
}

static void
clay_paint_options_gui_reset_aspect_ratio (GtkWidget        *button,
                                           ClayPaintOptions *paint_options)
{
  ClayBrush *brush = clay_context_get_brush (CLAY_CONTEXT (paint_options));

  if (brush)
    clay_paint_options_set_default_brush_aspect_ratio (paint_options, brush);
}

static void
clay_paint_options_gui_reset_angle (GtkWidget        *button,
                                    ClayPaintOptions *paint_options)
{
  ClayBrush *brush = clay_context_get_brush (CLAY_CONTEXT (paint_options));

  if (brush)
    clay_paint_options_set_default_brush_angle (paint_options, brush);
}

static void
clay_paint_options_gui_reset_spacing (GtkWidget        *button,
                                      ClayPaintOptions *paint_options)
{
  ClayBrush *brush = clay_context_get_brush (CLAY_CONTEXT (paint_options));

  if (brush)
    clay_paint_options_set_default_brush_spacing (paint_options, brush);
}

static void
clay_paint_options_gui_reset_hardness (GtkWidget        *button,
                                       ClayPaintOptions *paint_options)
{
  ClayBrush *brush = clay_context_get_brush (CLAY_CONTEXT (paint_options));

  if (brush)
    clay_paint_options_set_default_brush_hardness (paint_options, brush);
}

static void
clay_paint_options_gui_reset_force (GtkWidget        *button,
                                    ClayPaintOptions *paint_options)
{
  g_object_set (paint_options,
                "brush-force", 0.5,
                NULL);
}

static GtkWidget *
expand_options_gui (ClayPaintOptions *paint_options,
                    GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *combo_box;
  GtkWidget *vbox;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  scale = clay_prop_spin_scale_new (config, "expand-amount",
                                    1, 10, 2);
  clay_spin_scale_set_constrain_drag (CLAY_SPIN_SCALE (scale), TRUE);

  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 1.0, 1000.0);
  clay_spin_scale_set_gamma (CLAY_SPIN_SCALE (scale), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  combo_box = clay_prop_enum_combo_box_new (config, "expand-fill-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), combo_box, FALSE, FALSE, 0);

  frame = clay_prop_enum_radio_frame_new (config, "expand-mask-fill-type",
                                          _("Fill Layer Mask With"), 0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  frame = clay_prop_expanding_frame_new (config, "expand-use", NULL,
                                         vbox, NULL);

  return frame;
}

static GtkWidget *
clay_paint_options_gui_scale_with_buttons (GObject      *config,
                                           gchar        *prop_name,
                                           gchar        *link_prop_name,
                                           gchar        *reset_tooltip,
                                           gdouble       step_increment,
                                           gdouble       page_increment,
                                           gint          digits,
                                           gdouble       scale_min,
                                           gdouble       scale_max,
                                           gdouble       factor,
                                           gdouble       gamma,
                                           GCallback     reset_callback,
                                           GtkSizeGroup *link_group)
{
  GtkWidget *scale;
  GtkWidget *hbox;
  GtkWidget *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  scale = clay_prop_spin_scale_new (config, prop_name,
                                    step_increment, page_increment, digits);
  clay_spin_scale_set_constrain_drag (CLAY_SPIN_SCALE (scale), TRUE);

  clay_prop_widget_set_factor (scale, factor,
                               step_increment, page_increment, digits);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale),
                                    scale_min, scale_max);
  clay_spin_scale_set_gamma (CLAY_SPIN_SCALE (scale), gamma);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_set_visible (scale, TRUE);

  button = clay_icon_button_new (CLAY_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                CLAY_ICON_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    reset_callback,
                    config);

  clay_help_set_help_data (button,
                           reset_tooltip, NULL);

  if (link_prop_name)
    {
      GtkWidget *image;

      button = gtk_toggle_button_new ();
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

      image = gtk_image_new_from_icon_name (CLAY_ICON_LINKED,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_set_visible (image, TRUE);

      g_object_bind_property (config, link_prop_name,
                              button, "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    }
  else
    {
      button = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    }

  gtk_size_group_add_widget (link_group, button);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  clay_help_set_help_data (button,
                           _("Link to brush default"), NULL);

  return hbox;
}
