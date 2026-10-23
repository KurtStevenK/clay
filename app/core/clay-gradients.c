/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * clay-gradients.c
 * Copyright (C) 2002 Michael Natterer  <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clay.h"
#include "clay-gradients.h"
#include "claycontext.h"
#include "claycontainer.h"
#include "claydatafactory.h"
#include "claygradient.h"

#include "clay-intl.h"


#define CUSTOM_KEY                  "clay-gradient-custom"
#define FG_BG_RGB_KEY               "clay-gradient-fg-bg-rgb"
#define FG_BG_HARDEDGE_KEY          "clay-gradient-fg-bg-rgb-hardedge"
#define FG_BG_HSV_CCW_KEY           "clay-gradient-fg-bg-hsv-ccw"
#define FG_BG_HSV_CW_KEY            "clay-gradient-fg-bg-hsv-cw"
#define FG_TRANSPARENT_KEY          "clay-gradient-fg-transparent"
#define FG_TRANSPARENT_HARDEDGE_KEY "clay-gradient-fg-transparent-hardedge"


/*  local function prototypes  */

static ClayGradient * clay_gradients_add_gradient (Clay        *clay,
                                                   const gchar *name,
                                                   const gchar *id);


/*  public functions  */

void
clay_gradients_init (Clay *clay)
{
  ClayGradient *gradient;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /* Custom */
  gradient = clay_gradients_add_gradient (clay,
                                          _("Custom"),
                                          CUSTOM_KEY);
  g_object_set (gradient,
                "writable", TRUE,
                NULL);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_BACKGROUND;

  /* FG to BG (RGB) */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to BG (RGB)"),
                                          FG_BG_RGB_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_BACKGROUND;
  clay_context_set_gradient (clay->user_context, gradient);

  /* FG to BG (Hard Edge) */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to BG (Hard Edge)"),
                                          FG_BG_HARDEDGE_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->type             = CLAY_GRADIENT_SEGMENT_STEP;

  /* FG to BG (HSV Counter-Clockwise) */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to BG (HSV Counter-Clockwise)"),
                                          FG_BG_HSV_CCW_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = CLAY_GRADIENT_SEGMENT_HSV_CCW;

  /* FG to BG (HSV Clockwise Hue) */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to BG (HSV Clockwise Hue)"),
                                          FG_BG_HSV_CW_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = CLAY_GRADIENT_SEGMENT_HSV_CW;

  /* FG to Transparent */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to Transparent"),
                                          FG_TRANSPARENT_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;

  /* FG to Transparent (Hard Edge) */
  gradient = clay_gradients_add_gradient (clay,
                                          _("FG to Transparent (Hard Edge)"),
                                          FG_TRANSPARENT_HARDEDGE_KEY);
  gradient->segments->left_color_type  = CLAY_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;
  gradient->segments->type             = CLAY_GRADIENT_SEGMENT_STEP;
}

ClayGradient *
clay_gradients_get_custom (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), CUSTOM_KEY);
}

ClayGradient *
clay_gradients_get_fg_bg_rgb (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), FG_BG_RGB_KEY);
}

ClayGradient *
clay_gradients_get_fg_bg_hsv_ccw (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), FG_BG_HSV_CCW_KEY);
}

ClayGradient *
clay_gradients_get_fg_bg_hsv_cw (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), FG_BG_HSV_CW_KEY);
}

ClayGradient *
clay_gradients_get_fg_transparent (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), FG_TRANSPARENT_KEY);
}


/*  private functions  */

static ClayGradient *
clay_gradients_add_gradient (Clay        *clay,
                             const gchar *name,
                             const gchar *id)
{
  ClayGradient *gradient;

  gradient = CLAY_GRADIENT (clay_gradient_new (clay_get_user_context (clay),
                                               name));

  clay_data_make_internal (CLAY_DATA (gradient), id);

  clay_container_add (clay_data_factory_get_container (clay->gradient_factory),
                      CLAY_OBJECT (gradient));
  g_object_unref (gradient);

  g_object_set_data (G_OBJECT (clay), id, gradient);

  return gradient;
}
