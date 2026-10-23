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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "display-types.h"

#include "core/clay-utils.h"
#include "core/clayimage.h"
#include "core/clayunit.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-utils.h"

#include "clay-intl.h"

void
clay_display_shell_get_constrained_line_params (ClayDisplayShell *shell,
                                                gdouble          *offset_angle,
                                                gdouble          *xres,
                                                gdouble          *yres)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (offset_angle != NULL);
  g_return_if_fail (xres != NULL);
  g_return_if_fail (yres != NULL);

  if (shell->flip_horizontally ^ shell->flip_vertically)
    *offset_angle = +shell->rotate_angle;
  else
    *offset_angle = -shell->rotate_angle;

  *xres = 1.0;
  *yres = 1.0;

  if (! shell->dot_for_dot)
    {
      ClayImage *image = clay_display_get_image (shell->display);

      if (image)
        clay_image_get_resolution (image, xres, yres);
    }
}

void
clay_display_shell_constrain_line (ClayDisplayShell *shell,
                                   gdouble           start_x,
                                   gdouble           start_y,
                                   gdouble          *end_x,
                                   gdouble          *end_y,
                                   gint              n_snap_lines)
{
  gdouble offset_angle;
  gdouble xres, yres;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (end_x != NULL);
  g_return_if_fail (end_y != NULL);

  clay_display_shell_get_constrained_line_params (shell,
                                                  &offset_angle,
                                                  &xres, &yres);

  clay_constrain_line (start_x, start_y,
                       end_x,   end_y,
                       n_snap_lines,
                       offset_angle,
                       xres, yres);
}

gdouble
clay_display_shell_constrain_angle (ClayDisplayShell *shell,
                                    gdouble           angle,
                                    gint              n_snap_lines)
{
  gdouble x, y;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), 0.0);

  x = cos (angle);
  y = sin (angle);

  clay_display_shell_constrain_line (shell,
                                     0.0, 0.0,
                                     &x,  &y,
                                     n_snap_lines);

  return atan2 (y, x);
}

/**
 * clay_display_shell_get_line_status:
 * @status:    initial status text.
 * @separator: separator text between the line information and @status.
 * @shell:     #ClayDisplayShell this status text will be displayed for.
 * @x1:        abscissa of first point.
 * @y1:        ordinate of first point.
 * @x2:        abscissa of second point.
 * @y2:        ordinate of second point.
 *
 * Utility function to prepend the status message with a distance and
 * angle value. Obviously this is only to be used for tools when it
 * makes sense, and in particular when there is a concept of line. For
 * instance when shift-clicking a painting tool or in the blend tool,
 * etc.
 * This utility prevents code duplication but also ensures a common
 * display for every tool where such a status is needed. It will take
 * into account the shell unit settings and will use the ideal digit
 * precision according to current image resolution.
 *
 * Returns: a newly allocated string containing the enhanced status.
 **/
gchar *
clay_display_shell_get_line_status (ClayDisplayShell *shell,
                                    const gchar      *status,
                                    const gchar      *separator,
                                    gdouble           x1,
                                    gdouble           y1,
                                    gdouble           x2,
                                    gdouble           y2)
{
  ClayImage *image;
  gchar     *enhanced_status;
  gdouble    xres;
  gdouble    yres;
  gdouble    dx, dy, pixel_dist;
  gdouble    angle;

  image = clay_display_get_image (shell->display);
  if (! image)
    {
      /* This makes no sense to add line information when no image is
       * attached to the display. */
      return g_strdup (status);
    }

  if (shell->unit == clay_unit_pixel ())
    xres = yres = 1.0;
  else
    clay_image_get_resolution (image, &xres, &yres);

  dx = x2 - x1;
  dy = y2 - y1;
  pixel_dist = sqrt (SQR (dx) + SQR (dy));

  if (dx)
    {
      angle = clay_rad_to_deg (atan ((dy/yres) / (dx/xres)));
      if (dx > 0)
        {
          if (dy > 0)
            angle = 360.0 - angle;
          else if (dy < 0)
            angle = -angle;
        }
      else
        {
          angle = 180.0 - angle;
        }
    }
  else if (dy)
    {
      angle = dy > 0 ? 270.0 : 90.0;
    }
  else
    {
      angle = 0.0;
    }

  if (shell->unit == clay_unit_pixel ())
    {
      enhanced_status = g_strdup_printf ("%.1f %s, %.2f\302\260%s%s",
                                         pixel_dist, _("pixels"), angle,
                                         separator, status);
    }
  else
    {
      gdouble inch_dist;
      gdouble unit_dist;
      gint    digits = 0;

      /* The distance in unit. */
      inch_dist = sqrt (SQR (dx / xres) + SQR (dy / yres));
      unit_dist = clay_unit_get_factor (shell->unit) * inch_dist;

      /* The ideal digit precision for unit in current resolution. */
      if (inch_dist)
        digits = clay_unit_get_scaled_digits (shell->unit,
                                              pixel_dist / inch_dist);

      enhanced_status = g_strdup_printf ("%.*f %s, %.2f\302\260%s%s",
                                         digits, unit_dist,
                                         clay_unit_get_symbol (shell->unit),
                                         angle, separator, status);

    }

  return enhanced_status;
}
