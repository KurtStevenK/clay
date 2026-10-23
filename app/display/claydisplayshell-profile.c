/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claycoreconfig.h"

#include "gegl/clay-babl.h"

#include "core/clayimage.h"
#include "core/clayprojectable.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-actions.h"
#include "claydisplayshell-filter.h"
#include "claydisplayshell-profile.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_display_shell_profile_free        (ClayDisplayShell *shell);

static void   clay_display_shell_color_config_notify (ClayColorConfig  *config,
                                                      const GParamSpec *pspec,
                                                      ClayDisplayShell *shell);


/*  public functions  */

void
clay_display_shell_profile_init (ClayDisplayShell *shell)
{
  ClayColorConfig *color_config;

  color_config = CLAY_CORE_CONFIG (shell->display->config)->color_management;

  shell->color_config = clay_config_duplicate (CLAY_CONFIG (color_config));

  /* use after so we are called after the profile cache is invalidated
   * in clay_widget_get_color_transform()
   */
  g_signal_connect_after (shell->color_config, "notify",
                          G_CALLBACK (clay_display_shell_color_config_notify),
                          shell);
}

void
clay_display_shell_profile_finalize (ClayDisplayShell *shell)
{
  g_clear_object (&shell->color_config);

  clay_display_shell_profile_free (shell);
}

void
clay_display_shell_profile_update (ClayDisplayShell *shell)
{
  ClayImage               *image;
  ClayColorProfile        *src_profile;
  const Babl              *src_format;
  ClayColorProfile        *filter_profile;
  const Babl              *filter_format;
  const Babl              *dest_format;
  ClayColorProfile        *proof_profile;
  ClayColorRenderingIntent simulation_intent =
    CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                 simulation_bpc    = FALSE;

  clay_display_shell_profile_free (shell);

  image = clay_display_get_image (shell->display);

  if (! image)
    return;

  src_profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (shell));

  if (! src_profile)
    return;

  proof_profile = clay_color_managed_get_simulation_profile (CLAY_COLOR_MANAGED (image));
  simulation_intent = clay_color_managed_get_simulation_intent (CLAY_COLOR_MANAGED (image));
  simulation_bpc = clay_color_managed_get_simulation_bpc (CLAY_COLOR_MANAGED (image));

  src_format = clay_projectable_get_format (CLAY_PROJECTABLE (image));

  if (clay_display_shell_has_filter (shell))
    {
      filter_format  = shell->filter_format;
      filter_profile = shell->filter_profile;
    }
  else
    {
      filter_format  = src_format;
      filter_profile = src_profile;
    }

  if (! clay_display_shell_profile_can_convert_to_u8 (shell))
    {
      dest_format = shell->filter_format;
    }
  else
    {
      dest_format = babl_format ("R'G'B'A u8");
    }

#if 0
  g_printerr ("src_profile:    %s\n"
              "src_format:     %s\n"
              "filter_profile: %s\n"
              "filter_format:  %s\n"
              "dest_format:    %s\n",
              clay_color_profile_get_label (src_profile),
              babl_get_name (src_format),
              clay_color_profile_get_label (filter_profile),
              babl_get_name (filter_format),
              babl_get_name (dest_format));
#endif

  if (! clay_color_transform_can_gegl_copy (src_profile, filter_profile))
    {
      shell->filter_transform =
        clay_color_transform_new (src_profile,
                                  src_format,
                                  filter_profile,
                                  filter_format,
                                  CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                  CLAY_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION |
                                  CLAY_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE);
    }

  shell->profile_transform =
    clay_widget_get_color_transform (gtk_widget_get_toplevel (GTK_WIDGET (shell)),
                                     clay_display_shell_get_color_config (shell),
                                     filter_profile,
                                     filter_format,
                                     dest_format,
                                     proof_profile,
                                     simulation_intent,
                                     simulation_bpc);

  if (shell->filter_transform || shell->profile_transform)
    {
      gint w = shell->render_buf_width;
      gint h = shell->render_buf_height;

      shell->profile_data =
        gegl_malloc (w * h * babl_format_get_bytes_per_pixel (src_format));

      shell->profile_stride =
        w * babl_format_get_bytes_per_pixel (src_format);

      shell->profile_buffer =
        gegl_buffer_linear_new_from_data (shell->profile_data,
                                          src_format,
                                          GEGL_RECTANGLE (0, 0, w, h),
                                          GEGL_AUTO_ROWSTRIDE,
                                          (GDestroyNotify) gegl_free,
                                          shell->profile_data);
    }
}

gboolean
clay_display_shell_profile_can_convert_to_u8 (ClayDisplayShell *shell)
{
  ClayImage *image = clay_display_get_image (shell->display);

  if (image)
    {
      ClayComponentType component_type;

      if (! clay_display_shell_has_filter (shell))
        component_type = clay_image_get_component_type (image);
      else
        component_type = clay_babl_format_get_component_type (shell->filter_format);

      switch (component_type)
        {
        case CLAY_COMPONENT_TYPE_U8:
#if 0
          /* would like to convert directly for these too, but it
           * produces inferior results, see bug 750874
           */
        case CLAY_COMPONENT_TYPE_U16:
        case CLAY_COMPONENT_TYPE_U32:
#endif
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}


/*  private functions  */

static void
clay_display_shell_profile_free (ClayDisplayShell *shell)
{
  g_clear_object (&shell->profile_transform);
  g_clear_object (&shell->filter_transform);
  g_clear_object (&shell->profile_buffer);
  shell->profile_data   = NULL;
  shell->profile_stride = 0;
}

static void
clay_display_shell_color_config_notify (ClayColorConfig  *config,
                                        const GParamSpec *pspec,
                                        ClayDisplayShell *shell)
{
  if (! strcmp (pspec->name, "mode")                                    ||
      ! strcmp (pspec->name, "display-rendering-intent")                ||
      ! strcmp (pspec->name, "display-use-black-point-compensation")    ||
      ! strcmp (pspec->name, "simulation-gamut-check"))
    {
      gboolean     managed   = FALSE;
      gboolean     softproof = FALSE;
      const gchar *action    = NULL;

#define SET_SENSITIVE(action, sensitive) \
      clay_display_shell_set_action_sensitive (shell, action, sensitive);

#define SET_ACTIVE(action, active) \
      clay_display_shell_set_action_active (shell, action, active);

      switch (clay_color_config_get_mode (config))
        {
        case CLAY_COLOR_MANAGEMENT_OFF:
          break;

        case CLAY_COLOR_MANAGEMENT_DISPLAY:
          managed = TRUE;
          break;

        case CLAY_COLOR_MANAGEMENT_SOFTPROOF:
          managed   = TRUE;
          softproof = TRUE;
          break;
        }

      SET_ACTIVE ("view-color-management-enable", managed);
      SET_ACTIVE ("view-color-management-softproof", softproof);

      switch (clay_color_config_get_display_intent (config))
        {
        case CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL:
          action = "view-display-intent-perceptual";
          break;

        case CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC:
          action = "view-display-intent-relative-colorimetric";
          break;

        case CLAY_COLOR_RENDERING_INTENT_SATURATION:
          action = "view-display-intent-saturation";
          break;

        case CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
          action = "view-display-intent-absolute-colorimetric";
          break;
        }

      SET_SENSITIVE ("view-display-intent-perceptual",            managed);
      SET_SENSITIVE ("view-display-intent-relative-colorimetric", managed);
      SET_SENSITIVE ("view-display-intent-saturation",            managed);
      SET_SENSITIVE ("view-display-intent-absolute-colorimetric", managed);

      SET_ACTIVE (action, TRUE);

      SET_SENSITIVE ("view-display-black-point-compensation", managed);
      SET_ACTIVE    ("view-display-black-point-compensation",
                     clay_color_config_get_display_bpc (config));
      SET_SENSITIVE ("view-softproof-gamut-check", softproof);
      SET_ACTIVE    ("view-softproof-gamut-check",
                     clay_color_config_get_simulation_gamut_check (config));
    }

  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (shell));
}
