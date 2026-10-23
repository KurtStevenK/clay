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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claytoolinfo.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "tools-actions.h"
#include "tools-commands.h"

#include "clay-intl.h"


static const ClayActionEntry tools_actions[] =
{
  { "tools-swap", NULL,
    NC_("tools-action", "Activate _Last Tool"), NULL, { "<shift>X", NULL },
    NC_("tools-action", "Switch back to the last activated tool"),
    tools_swap_cmd_callback,
    CLAY_HELP_TOOLBOX_SWAP_TOOLS }
};

static const ClayStringActionEntry tools_alternative_actions[] =
{
  { "tools-by-color-select-short", CLAY_ICON_TOOL_BY_COLOR_SELECT,
    NC_("tools-action", "_By Color"), NULL, { NULL },
    NC_("tools-action", "Select regions with similar colors"),
    "clay-by-color-select-tool",
    CLAY_HELP_TOOL_BY_COLOR_SELECT },

  { "tools-rotate-arbitrary", CLAY_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), NULL, { NULL },
    NC_("tools-action", "Rotate drawable by an arbitrary angle"),
    "clay-rotate-layer",
    CLAY_HELP_TOOL_ROTATE },

  { "tools-rotate-image-arbitrary", CLAY_ICON_TOOL_ROTATE,
    NC_("tools-action", "_Arbitrary Rotation..."), NULL, { NULL },
    NC_("tools-action", "Rotate image by an arbitrary angle"),
    "clay-rotate-image",
    CLAY_HELP_TOOL_ROTATE }
};

static const ClayEnumActionEntry tools_color_average_radius_actions[] =
{
  { "tools-color-average-radius-set", CLAY_ICON_TOOL_COLOR_PICKER,
    "Set Color Picker Radius", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayDoubleActionEntry tools_paintbrush_pixel_size_actions[] =
{
  { "tools-paintbrush-pixel-size-set", CLAY_ICON_TOOL_MYPAINT_BRUSH,
    NC_("tools-action", "Set Brush Size in Pixels"), NULL, { NULL }, NULL,
    1.0, NULL }
};

static const ClayEnumActionEntry tools_paintbrush_size_actions[] =
{
  { "tools-paintbrush-size-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Size", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_paintbrush_aspect_ratio_actions[] =
{
  { "tools-paintbrush-aspect-ratio-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Aspect Ratio", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_paintbrush_angle_actions[] =
{
  { "tools-paintbrush-angle-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Angle", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_paintbrush_spacing_actions[] =
{
  { "tools-paintbrush-spacing-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Spacing", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_paintbrush_hardness_actions[] =
{
  { "tools-paintbrush-hardness-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Hardness", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_paintbrush_force_actions[] =
{
  { "tools-paintbrush-force-set", CLAY_ICON_TOOL_PAINTBRUSH,
    "Set Brush Force", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayDoubleActionEntry tools_ink_blob_pixel_size_actions[] =
{
  { "tools-ink-blob-pixel-size-set", CLAY_ICON_TOOL_INK,
    NC_("tools-action", "Set Ink Blob Size in Pixels"), NULL, { NULL }, NULL,
    1.0, NULL }
};

static const ClayEnumActionEntry tools_ink_blob_size_actions[] =
{
  { "tools-ink-blob-size-set", CLAY_ICON_TOOL_INK,
    "Set Ink Blob Size", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_ink_blob_aspect_actions[] =
{
  { "tools-ink-blob-aspect-set", CLAY_ICON_TOOL_INK,
    "Set Ink Blob Aspect", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_ink_blob_angle_actions[] =
{
  { "tools-ink-blob-angle-set", CLAY_ICON_TOOL_INK,
    "Set Ink Blob Angle", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_airbrush_rate_actions[] =
{
  { "tools-airbrush-rate-set", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-rate-minimum", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-rate-maximum", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-rate-decrease-skip", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-rate-increase-skip", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Rate: Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry tools_airbrush_flow_actions[] =
{
  { "tools-airbrush-flow-set", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-airbrush-flow-minimum", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-airbrush-flow-maximum", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-airbrush-flow-decrease-skip", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-airbrush-flow-increase-skip", CLAY_ICON_TOOL_AIRBRUSH,
    NC_("tools-action", "Airbrush Flow: Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry tools_mybrush_radius_actions[] =
{
  { "tools-mypaint-brush-radius-set", CLAY_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Radius", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayDoubleActionEntry tools_mybrush_pixel_size_actions[] =
{
  { "tools-mypaint-brush-pixel-size-set", CLAY_ICON_TOOL_MYPAINT_BRUSH,
    NC_("tools-action", "Set MyPaint Brush Diameter in Pixels"), NULL, { NULL }, NULL,
    1.0, NULL }
};

static const ClayEnumActionEntry tools_mybrush_hardness_actions[] =
{
  { "tools-mypaint-brush-hardness-set", CLAY_ICON_TOOL_MYPAINT_BRUSH,
    "Set MyPaint Brush Hardness", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_foreground_select_brush_size_actions[] =
{
  { "tools-foreground-select-brush-size-set",
    CLAY_ICON_TOOL_FOREGROUND_SELECT,
    "Set Foreground Select Brush Size", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_transform_preview_opacity_actions[] =
{
  { "tools-transform-preview-opacity-set", CLAY_ICON_TOOL_PERSPECTIVE,
    "Set Transform Tool Preview Opacity", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayDoubleActionEntry tools_warp_effect_pixel_size_actions[] =
{
  { "tools-warp-effect-pixel-size-set", CLAY_ICON_TOOL_WARP,
    NC_("tools-action", "Set Warp Effect Size in Pixels"), NULL, { NULL }, NULL,
    1.0, NULL }
};

static const ClayEnumActionEntry tools_warp_effect_size_actions[] =
{
  { "tools-warp-effect-size-set", CLAY_ICON_TOOL_WARP,
    "Set Warp Effect Size", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_warp_effect_hardness_actions[] =
{
  { "tools-warp-effect-hardness-set", CLAY_ICON_TOOL_WARP,
    "Set Warp Effect Hardness", NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL }
};

static const ClayEnumActionEntry tools_opacity_actions[] =
{
  { "tools-opacity-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-opacity-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Set to Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-opacity-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-opacity-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-opacity-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 1"), NULL, { "less", NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 1"), NULL, { "greater", NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease by 10"), NULL, { "<primary>less", NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase by 10"), NULL, { "<primary>greater", NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-opacity-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-opacity-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Opacity: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_size_actions[] =
{
  { "tools-size-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-size-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Set to Default Value"), NULL, { "backslash", NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-size-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-size-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-size-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 1"), NULL, { "bracketleft", NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 1"), NULL, { "bracketright", NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease by 10"), NULL, { "braceleft", NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase by 10"), NULL, { "braceright", NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-size-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-size-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Size: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_aspect_actions[] =
{
  { "tools-aspect-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-aspect-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Set To Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-aspect-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-aspect-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-aspect-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-aspect-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-aspect-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Aspect Ratio: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_angle_actions[] =
{
  { "tools-angle-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-angle-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Set Angle To Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-angle-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-angle-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-angle-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 1°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 1°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease by 15°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase by 15°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-angle-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-angle-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Angle: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_spacing_actions[] =
{
  { "tools-spacing-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-spacing-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Set To Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-spacing-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-spacing-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-spacing-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-spacing-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-spacing-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Spacing: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_hardness_actions[] =
{
  { "tools-hardness-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-hardness-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Set to Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-hardness-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-hardness-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-hardness-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-hardness-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-hardness-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Hardness: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayEnumActionEntry tools_force_actions[] =
{
  { "tools-force-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-force-set-to-default", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Set to Default Value"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET_TO_DEFAULT, FALSE,
    NULL },
  { "tools-force-minimum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Minimize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-force-maximum", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Maximize"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-force-decrease", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-skip", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "tools-force-decrease-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "tools-force-increase-percent", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Tool's Force: Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL },
};

static const ClayDoubleActionEntry tools_paint_select_pixel_size_actions[] =
{
  { "tools-paint-select-pixel-size-set", CLAY_ICON_TOOL_PAINT_SELECT,
    NC_("tools-action", "Set Paint Select Brush Size in Pixels"), NULL, { NULL }, NULL,
    1.0, NULL }
};

static const ClayEnumActionEntry tools_object_1_actions[] =
{
  { "tools-object-1-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Select Object 1 by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-1-first", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "First Object 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-1-last", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Last Object 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-1-previous", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Previous Object 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-1-next", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Next Object 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry tools_object_2_actions[] =
{
  { "tools-object-2-set", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Select Object 2 by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-2-first", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "First Object 2"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-2-last", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Last Object 2"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-2-previous", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Previous Object 2"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-2-next", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("tools-action", "Next Object 2"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};


void
tools_actions_setup (ClayActionGroup *group)
{
  GList *list;

  clay_action_group_add_actions (group, "tools-action",
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions));

  clay_action_group_add_string_actions (group, "tools-action",
                                        tools_alternative_actions,
                                        G_N_ELEMENTS (tools_alternative_actions),
                                        tools_select_cmd_callback);

  clay_action_group_add_enum_actions (group, NULL,
                                      tools_color_average_radius_actions,
                                      G_N_ELEMENTS (tools_color_average_radius_actions),
                                      tools_color_average_radius_cmd_callback);

  clay_action_group_add_double_actions (group, "tools-action",
                                        tools_paintbrush_pixel_size_actions,
                                        G_N_ELEMENTS (tools_paintbrush_pixel_size_actions),
                                        tools_paintbrush_pixel_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_size_actions,
                                      G_N_ELEMENTS (tools_paintbrush_size_actions),
                                      tools_paintbrush_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_aspect_ratio_actions,
                                      G_N_ELEMENTS (tools_paintbrush_aspect_ratio_actions),
                                      tools_paintbrush_aspect_ratio_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_angle_actions,
                                      G_N_ELEMENTS (tools_paintbrush_angle_actions),
                                      tools_paintbrush_angle_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_spacing_actions,
                                      G_N_ELEMENTS (tools_paintbrush_spacing_actions),
                                      tools_paintbrush_spacing_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_hardness_actions,
                                      G_N_ELEMENTS (tools_paintbrush_hardness_actions),
                                      tools_paintbrush_hardness_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_paintbrush_force_actions,
                                      G_N_ELEMENTS (tools_paintbrush_force_actions),
                                      tools_paintbrush_force_cmd_callback);

  clay_action_group_add_double_actions (group, "tools-action",
                                        tools_ink_blob_pixel_size_actions,
                                        G_N_ELEMENTS (tools_ink_blob_pixel_size_actions),
                                        tools_ink_blob_pixel_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_size_actions,
                                      G_N_ELEMENTS (tools_ink_blob_size_actions),
                                      tools_ink_blob_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_aspect_actions,
                                      G_N_ELEMENTS (tools_ink_blob_aspect_actions),
                                      tools_ink_blob_aspect_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_ink_blob_angle_actions,
                                      G_N_ELEMENTS (tools_ink_blob_angle_actions),
                                      tools_ink_blob_angle_cmd_callback);

  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_rate_actions,
                                      G_N_ELEMENTS (tools_airbrush_rate_actions),
                                      tools_airbrush_rate_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_airbrush_flow_actions,
                                      G_N_ELEMENTS (tools_airbrush_flow_actions),
                                      tools_airbrush_flow_cmd_callback);

  clay_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_radius_actions,
                                      G_N_ELEMENTS (tools_mybrush_radius_actions),
                                      tools_mybrush_radius_cmd_callback);
  clay_action_group_add_double_actions (group, "tools-action",
                                        tools_mybrush_pixel_size_actions,
                                        G_N_ELEMENTS (tools_mybrush_pixel_size_actions),
                                        tools_mybrush_pixel_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_mybrush_hardness_actions,
                                      G_N_ELEMENTS (tools_mybrush_hardness_actions),
                                      tools_mybrush_hardness_cmd_callback);

  clay_action_group_add_enum_actions (group, NULL,
                                      tools_foreground_select_brush_size_actions,
                                      G_N_ELEMENTS (tools_foreground_select_brush_size_actions),
                                      tools_fg_select_brush_size_cmd_callback);

  clay_action_group_add_enum_actions (group, NULL,
                                      tools_transform_preview_opacity_actions,
                                      G_N_ELEMENTS (tools_transform_preview_opacity_actions),
                                      tools_transform_preview_opacity_cmd_callback);

  clay_action_group_add_double_actions (group, "tools-action",
                                        tools_warp_effect_pixel_size_actions,
                                        G_N_ELEMENTS (tools_warp_effect_pixel_size_actions),
                                        tools_warp_effect_pixel_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_size_actions,
                                      G_N_ELEMENTS (tools_warp_effect_size_actions),
                                      tools_warp_effect_size_cmd_callback);
  clay_action_group_add_enum_actions (group, NULL,
                                      tools_warp_effect_hardness_actions,
                                      G_N_ELEMENTS (tools_warp_effect_hardness_actions),
                                      tools_warp_effect_hardness_cmd_callback);

  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_opacity_actions,
                                      G_N_ELEMENTS (tools_opacity_actions),
                                      tools_opacity_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_size_actions,
                                      G_N_ELEMENTS (tools_size_actions),
                                      tools_size_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_aspect_actions,
                                      G_N_ELEMENTS (tools_aspect_actions),
                                      tools_aspect_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_angle_actions,
                                      G_N_ELEMENTS (tools_angle_actions),
                                      tools_angle_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_spacing_actions,
                                      G_N_ELEMENTS (tools_spacing_actions),
                                      tools_spacing_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_hardness_actions,
                                      G_N_ELEMENTS (tools_hardness_actions),
                                      tools_hardness_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_force_actions,
                                      G_N_ELEMENTS (tools_force_actions),
                                      tools_force_cmd_callback);

  clay_action_group_add_double_actions (group, "tools-action",
                                        tools_paint_select_pixel_size_actions,
                                        G_N_ELEMENTS (tools_paint_select_pixel_size_actions),
                                        tools_paint_select_pixel_size_cmd_callback);

  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_object_1_actions,
                                      G_N_ELEMENTS (tools_object_1_actions),
                                      tools_object_1_cmd_callback);
  clay_action_group_add_enum_actions (group, "tools-action",
                                      tools_object_2_actions,
                                      G_N_ELEMENTS (tools_object_2_actions),
                                      tools_object_2_cmd_callback);

  for (list = clay_get_tool_info_iter (group->clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = list->data;

      if (tool_info->menu_label)
        {
          ClayStringActionEntry  entry = { 0 };
          gchar                 *name;
          gchar                 *long_label;
          const gchar           *icon_name;
          const gchar           *identifier;

          name       = clay_tool_info_get_action_name (tool_info);
          icon_name  = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));
          identifier = clay_object_get_name (tool_info);
          long_label = g_strdup_printf (_("Activate tool \"%s\""), tool_info->label);

          entry.name           = name;
          entry.icon_name      = icon_name;
          entry.label          = long_label;
          entry.short_label    = tool_info->menu_label;
          entry.accelerator[0] = tool_info->menu_accel;
          entry.tooltip        = tool_info->tooltip;
          entry.help_id        = tool_info->help_id;
          entry.value          = identifier;

          clay_action_group_add_string_actions (group, NULL,
                                                &entry, 1,
                                                tools_select_cmd_callback);

          g_free (name);
          g_free (long_label);
        }
    }
}

void
tools_actions_update (ClayActionGroup *group,
                      gpointer         data)
{
  ClayImage *image = action_data_get_image (data);

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("tools-gegl", image);
  /* Since these are listed under the Image and Layer menus,
   * they are disabled without an image to keep consistency
   */
  SET_SENSITIVE ("tools-rotate-arbitrary", image);
  SET_SENSITIVE ("tools-rotate-image-arbitrary", image);

#undef SET_SENSITIVE
}
