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
#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "operations/claybrightnesscontrastconfig.h"

#include "core/claydrawable.h"
#include "core/clayerror.h"
#include "core/clayimage.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claywidgets-constructors.h"

#include "display/claydisplay.h"

#include "claybrightnesscontrasttool.h"
#include "clayfilteroptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


#define SLIDER_WIDTH 200


static gboolean   clay_brightness_contrast_tool_initialize (ClayTool              *tool,
                                                            ClayDisplay           *display,
                                                            GError               **error);
static void   clay_brightness_contrast_tool_button_press   (ClayTool              *tool,
                                                            const ClayCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            ClayButtonPressType    press_type,
                                                            ClayDisplay           *display);
static void   clay_brightness_contrast_tool_button_release (ClayTool              *tool,
                                                            const ClayCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            ClayButtonReleaseType  release_type,
                                                            ClayDisplay           *display);
static void   clay_brightness_contrast_tool_motion         (ClayTool              *tool,
                                                            const ClayCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            ClayDisplay           *display);

static gchar *
              clay_brightness_contrast_tool_get_operation  (ClayFilterTool        *filter_tool,
                                                            gchar                **description);
static void   clay_brightness_contrast_tool_dialog         (ClayFilterTool        *filter_tool);

static void   brightness_contrast_to_levels_callback       (GtkWidget             *widget,
                                                            ClayFilterTool        *filter_tool);


G_DEFINE_TYPE (ClayBrightnessContrastTool, clay_brightness_contrast_tool,
               CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_brightness_contrast_tool_parent_class


void
clay_brightness_contrast_tool_register (ClayToolRegisterCallback  callback,
                                        gpointer                  data)
{
  (* callback) (CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL,
                CLAY_TYPE_FILTER_OPTIONS, NULL,
                0,
                "clay-brightness-contrast-tool",
                _("Brightness-Contrast"),
                _("Adjust brightness and contrast"),
                N_("B_rightness-Contrast..."), NULL,
                NULL, CLAY_HELP_TOOL_BRIGHTNESS_CONTRAST,
                CLAY_ICON_TOOL_BRIGHTNESS_CONTRAST,
                data);
}

static void
clay_brightness_contrast_tool_class_init (ClayBrightnessContrastToolClass *klass)
{
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  tool_class->initialize           = clay_brightness_contrast_tool_initialize;
  tool_class->button_press         = clay_brightness_contrast_tool_button_press;
  tool_class->button_release       = clay_brightness_contrast_tool_button_release;
  tool_class->motion               = clay_brightness_contrast_tool_motion;

  filter_tool_class->get_operation = clay_brightness_contrast_tool_get_operation;
  filter_tool_class->dialog        = clay_brightness_contrast_tool_dialog;
}

static void
clay_brightness_contrast_tool_init (ClayBrightnessContrastTool *bc_tool)
{
}

static gboolean
clay_brightness_contrast_tool_initialize (ClayTool     *tool,
                                          ClayDisplay  *display,
                                          GError      **error)
{
  ClayBrightnessContrastTool *bc_tool   = CLAY_BRIGHTNESS_CONTRAST_TOOL (tool);
  ClayImage                  *image     = clay_display_get_image (display);
  GList                      *drawables;

  if (! CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = clay_image_get_selected_drawables (image);
  /* Single drawable selection has been checked in parent initialize(). */
  g_return_val_if_fail (g_list_length (drawables) == 1, FALSE);

  if (clay_drawable_get_component_type (drawables->data) == CLAY_COMPONENT_TYPE_U8)
    {
      clay_prop_widget_set_factor (bc_tool->brightness_scale,
                                   127.0, 1.0, 8.0, 0);
      clay_prop_widget_set_factor (bc_tool->contrast_scale,
                                   127.0, 1.0, 8.0, 0);
    }
  else
    {
      clay_prop_widget_set_factor (bc_tool->brightness_scale,
                                   0.5, 0.01, 0.1, 3);
      clay_prop_widget_set_factor (bc_tool->contrast_scale,
                                   0.5, 0.01, 0.1, 3);
    }

  g_list_free (drawables);

  return TRUE;
}

static gchar *
clay_brightness_contrast_tool_get_operation (ClayFilterTool  *filter_tool,
                                             gchar          **description)
{
  *description = g_strdup (_("Adjust Brightness and Contrast"));

  return g_strdup ("clay:brightness-contrast");
}

static void
clay_brightness_contrast_tool_button_press (ClayTool            *tool,
                                            const ClayCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            ClayButtonPressType  press_type,
                                            ClayDisplay         *display)
{
  ClayBrightnessContrastTool *bc_tool = CLAY_BRIGHTNESS_CONTRAST_TOOL (tool);

  bc_tool->dragging = ! clay_filter_tool_on_guide (CLAY_FILTER_TOOL (tool),
                                                   coords, display);

  if (! bc_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      gdouble brightness;
      gdouble contrast;

      g_object_get (CLAY_FILTER_TOOL (tool)->config,
                    "brightness", &brightness,
                    "contrast",   &contrast,
                    NULL);

      bc_tool->x  = coords->x - contrast   * 127.0;
      bc_tool->y  = coords->y + brightness * 127.0;
      bc_tool->dx =   contrast   * 127.0;
      bc_tool->dy = - brightness * 127.0;

      tool->display = display;

      clay_tool_control_activate (tool->control);
    }
}

static void
clay_brightness_contrast_tool_button_release (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonReleaseType  release_type,
                                              ClayDisplay           *display)
{
  ClayBrightnessContrastTool *bc_tool = CLAY_BRIGHTNESS_CONTRAST_TOOL (tool);

  if (! bc_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      clay_tool_control_halt (tool->control);

      bc_tool->dragging = FALSE;

      if (bc_tool->dx == 0 && bc_tool->dy == 0)
        return;

      if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
        clay_config_reset (CLAY_CONFIG (CLAY_FILTER_TOOL (tool)->config));
    }
}

static void
clay_brightness_contrast_tool_motion (ClayTool         *tool,
                                      const ClayCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      ClayDisplay      *display)
{
  ClayBrightnessContrastTool *bc_tool = CLAY_BRIGHTNESS_CONTRAST_TOOL (tool);

  if (! bc_tool->dragging)
    {
      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      bc_tool->dx =   (coords->x - bc_tool->x);
      bc_tool->dy = - (coords->y - bc_tool->y);

      g_object_set (CLAY_FILTER_TOOL (tool)->config,
                    "brightness", CLAMP (bc_tool->dy, -127.0, 127.0) / 127.0,
                    "contrast",   CLAMP (bc_tool->dx, -127.0, 127.0) / 127.0,
                    NULL);
    }
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

static void
clay_brightness_contrast_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayBrightnessContrastTool *bc_tool = CLAY_BRIGHTNESS_CONTRAST_TOOL (filter_tool);
  GtkWidget                  *main_vbox;
  GtkWidget                  *scale;
  GtkWidget                  *button;

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

  /*  Create the brightness scale widget  */
  scale = clay_prop_spin_scale_new (filter_tool->config, "brightness",
                                    0.01, 0.1, 3);
  clay_spin_scale_set_label (CLAY_SPIN_SCALE (scale), _("_Brightness"));
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);

  bc_tool->brightness_scale = scale;

  /*  Create the contrast scale widget  */
  scale = clay_prop_spin_scale_new (filter_tool->config, "contrast",
                                    0.01, 0.1, 3);
  clay_spin_scale_set_label (CLAY_SPIN_SCALE (scale), _("_Contrast"));
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);

  bc_tool->contrast_scale = scale;

  button = clay_icon_button_new (CLAY_ICON_TOOL_LEVELS,
                                 _("Edit these Settings as Levels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (brightness_contrast_to_levels_callback),
                    filter_tool);
}

static void
brightness_contrast_to_levels_callback (GtkWidget      *widget,
                                        ClayFilterTool *filter_tool)
{
  ClayLevelsConfig *levels;

  levels = clay_brightness_contrast_config_to_levels_config (CLAY_BRIGHTNESS_CONTRAST_CONFIG (filter_tool->config));

  clay_filter_tool_edit_as (filter_tool,
                            "clay-levels-tool",
                            CLAY_CONFIG (levels));

  g_object_unref (levels);
}
