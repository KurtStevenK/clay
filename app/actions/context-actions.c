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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claybrushgenerated.h"
#include "core/claycontext.h"
#include "core/claylist.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "context-actions.h"
#include "context-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static const ClayActionEntry context_actions[] =
{
  { "context-colors-default", CLAY_ICON_COLORS_DEFAULT,
    NC_("context-action", "_Default Colors"), NULL, { "D", NULL },
    NC_("context-action",
        "Set foreground color to black, background color to white"),
    context_colors_default_cmd_callback,
    CLAY_HELP_TOOLBOX_DEFAULT_COLORS },

  { "context-colors-swap", CLAY_ICON_COLORS_SWAP,
    NC_("context-action", "S_wap Colors"), NULL, { "X", NULL },
    NC_("context-action", "Exchange foreground and background colors"),
    context_colors_swap_cmd_callback,
    CLAY_HELP_TOOLBOX_SWAP_COLORS }
};

static ClayEnumActionEntry context_palette_foreground_actions[] =
{
  { "context-palette-foreground-set", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Palette"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-foreground-first", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-foreground-last", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-foreground-previous", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-foreground-previous-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static ClayEnumActionEntry context_palette_background_actions[] =
{
  { "context-palette-background-set", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Palette"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-background-first", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use First Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-background-last", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-background-previous", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-background-previous-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Skip Back Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Skip Forward Palette Color"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static ClayEnumActionEntry context_colormap_foreground_actions[] =
{
  { "context-colormap-foreground-set", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Set Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-foreground-first", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use First Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-foreground-last", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Last Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-foreground-previous", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Previous Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Next Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-foreground-previous-skip", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Back Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next-skip", CLAY_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Forward Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static ClayEnumActionEntry context_colormap_background_actions[] =
{
  { "context-colormap-background-set", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Set Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-background-first", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Use First Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-background-last", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Use Last Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-background-previous", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Use Previous Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Use Next Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-background-previous-skip", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Back Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next-skip", CLAY_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Forward Color From Colormap"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static ClayEnumActionEntry context_swatch_foreground_actions[] =
{
  { "context-swatch-foreground-set", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-foreground-first", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-foreground-last", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-foreground-previous", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Color From Swatch"), NULL, { "9", NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Color From Swatch"), NULL, { "0", NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-foreground-previous-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static ClayEnumActionEntry context_swatch_background_actions[] =
{
  { "context-swatch-background-set", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-background-first", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use First Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-background-last", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-background-previous", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Color From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-background-previous-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Back From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next-skip", CLAY_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Forward From Swatch"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_red_actions[] =
{
  { "context-foreground-red-set", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-red-minimum", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-red-maximum", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-red-decrease", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-red-decrease-skip", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase-skip", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_green_actions[] =
{
  { "context-foreground-green-set", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-green-minimum", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-green-maximum", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-green-decrease", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-green-decrease-skip", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase-skip", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_blue_actions[] =
{
  { "context-foreground-blue-set", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-blue-minimum", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-blue-maximum", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-blue-decrease", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-blue-decrease-skip", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase-skip", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_red_actions[] =
{
  { "context-background-red-set", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-red-minimum", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-red-maximum", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-red-decrease", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-red-decrease-skip", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase-skip", CLAY_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_green_actions[] =
{
  { "context-background-green-set", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-green-minimum", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-green-maximum", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-green-decrease", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-green-decrease-skip", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase-skip", CLAY_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_blue_actions[] =
{
  { "context-background-blue-set", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-blue-minimum", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-blue-maximum", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-blue-decrease", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-blue-decrease-skip", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase-skip", CLAY_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_hue_actions[] =
{
  { "context-foreground-hue-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-hue-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-hue-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-hue-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-hue-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_saturation_actions[] =
{
  { "context-foreground-saturation-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-saturation-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-saturation-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-saturation-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-saturation-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_foreground_value_actions[] =
{
  { "context-foreground-value-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-value-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-value-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-value-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-value-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_hue_actions[] =
{
  { "context-background-hue-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-hue-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-hue-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-hue-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-hue-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_saturation_actions[] =
{
  { "context-background-saturation-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-saturation-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-saturation-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-saturation-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-saturation-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_background_value_actions[] =
{
  { "context-background-value-set", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-value-minimum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-value-maximum", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-value-decrease", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 1%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-value-decrease-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase-skip", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 10%"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_opacity_actions[] =
{
  { "context-opacity-set", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Set Transparency"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-opacity-transparent", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Transparent"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-opacity-opaque", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Opaque"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-opacity-decrease", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Transparent"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Opaque"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-opacity-decrease-skip", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Transparent"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase-skip", CLAY_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Opaque"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_paint_mode_actions[] =
{
  { "context-paint-mode-first", CLAY_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-paint-mode-last", CLAY_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-paint-mode-previous", CLAY_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-paint-mode-next", CLAY_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_tool_select_actions[] =
{
  { "context-tool-select-set", CLAY_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Choose by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-tool-select-first", CLAY_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-tool-select-last", CLAY_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-tool-select-previous", CLAY_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-tool-select-next", CLAY_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_select_actions[] =
{
  { "context-brush-select-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Select by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-select-first", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-select-last", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-select-previous", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-select-next", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_pattern_select_actions[] =
{
  { "context-pattern-select-set", CLAY_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Select by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-pattern-select-first", CLAY_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-pattern-select-last", CLAY_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-pattern-select-previous", CLAY_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-pattern-select-next", CLAY_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_palette_select_actions[] =
{
  { "context-palette-select-set", CLAY_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Select by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-palette-select-first", CLAY_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-select-last", CLAY_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-select-previous", CLAY_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-select-next", CLAY_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_gradient_select_actions[] =
{
  { "context-gradient-select-set", CLAY_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Select by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-gradient-select-first", CLAY_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-gradient-select-last", CLAY_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-gradient-select-previous", CLAY_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-gradient-select-next", CLAY_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_font_select_actions[] =
{
  { "context-font-select-set", CLAY_ICON_FONT,
    NC_("context-action", "Font Selection: Select by Index"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-font-select-first", CLAY_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to First"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-font-select-last", CLAY_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Last"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-font-select-previous", CLAY_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Previous"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-font-select-next", CLAY_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Next"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_spacing_actions[] =
{
  { "context-brush-spacing-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spacing-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spacing-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spacing-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spacing-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_shape_actions[] =
{
  { "context-brush-shape-circle", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Circular"), NULL, { NULL }, NULL,
    CLAY_BRUSH_GENERATED_CIRCLE, FALSE,
    NULL },
  { "context-brush-shape-square", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Square"), NULL, { NULL }, NULL,
    CLAY_BRUSH_GENERATED_SQUARE, FALSE,
    NULL },
  { "context-brush-shape-diamond", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Diamond"), NULL, { NULL }, NULL,
    CLAY_BRUSH_GENERATED_DIAMOND, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_radius_actions[] =
{
  { "context-brush-radius-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-radius-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-radius-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-radius-decrease-less", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SMALL_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-less", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SMALL_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 10"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-percent", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-percent", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase Relative"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_spikes_actions[] =
{
  { "context-brush-spikes-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spikes-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spikes-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spikes-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spikes-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 4"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 4"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_hardness_actions[] =
{
  { "context-brush-hardness-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-hardness-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-hardness-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-hardness-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.01"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.01"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-hardness-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_aspect_actions[] =
{
  { "context-brush-aspect-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-aspect-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Minimum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-aspect-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Maximum"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-aspect-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 0.1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-aspect-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 1"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayEnumActionEntry context_brush_angle_actions[] =
{
  { "context-brush-angle-set", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Set"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-angle-minimum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Horizontal"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-angle-maximum", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Vertical"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-angle-decrease", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 1°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 1°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-angle-decrease-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 15°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase-skip", CLAY_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 15°"), NULL, { NULL }, NULL,
    CLAY_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const ClayToggleActionEntry context_toggle_actions[] =
{
  { "context-dynamics-toggle", NULL,
    NC_("context-action", "_Enable/Disable Dynamics"), NULL, { NULL },
    NC_("context-action", "Apply or ignore the dynamics when painting"),
    context_toggle_dynamics_cmd_callback,
    FALSE,
    NULL },
};


void
context_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "context-action",
                                 context_actions,
                                 G_N_ELEMENTS (context_actions));

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_palette_foreground_actions,
                                      G_N_ELEMENTS (context_palette_foreground_actions),
                                      context_palette_foreground_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_palette_background_actions,
                                      G_N_ELEMENTS (context_palette_background_actions),
                                      context_palette_background_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_foreground_actions,
                                      G_N_ELEMENTS (context_colormap_foreground_actions),
                                      context_colormap_foreground_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_background_actions,
                                      G_N_ELEMENTS (context_colormap_background_actions),
                                      context_colormap_background_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_foreground_actions,
                                      G_N_ELEMENTS (context_swatch_foreground_actions),
                                      context_swatch_foreground_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_background_actions,
                                      G_N_ELEMENTS (context_swatch_background_actions),
                                      context_swatch_background_cmd_callback);


  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_red_actions,
                                      G_N_ELEMENTS (context_foreground_red_actions),
                                      context_foreground_red_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_green_actions,
                                      G_N_ELEMENTS (context_foreground_green_actions),
                                      context_foreground_green_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_blue_actions,
                                      G_N_ELEMENTS (context_foreground_blue_actions),
                                      context_foreground_blue_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_hue_actions,
                                      G_N_ELEMENTS (context_foreground_hue_actions),
                                      context_foreground_hue_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_saturation_actions,
                                      G_N_ELEMENTS (context_foreground_saturation_actions),
                                      context_foreground_saturation_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_value_actions,
                                      G_N_ELEMENTS (context_foreground_value_actions),
                                      context_foreground_value_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_red_actions,
                                      G_N_ELEMENTS (context_background_red_actions),
                                      context_background_red_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_green_actions,
                                      G_N_ELEMENTS (context_background_green_actions),
                                      context_background_green_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_blue_actions,
                                      G_N_ELEMENTS (context_background_blue_actions),
                                      context_background_blue_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_hue_actions,
                                      G_N_ELEMENTS (context_background_hue_actions),
                                      context_background_hue_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_saturation_actions,
                                      G_N_ELEMENTS (context_background_saturation_actions),
                                      context_background_saturation_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_background_value_actions,
                                      G_N_ELEMENTS (context_background_value_actions),
                                      context_background_value_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_opacity_actions,
                                      G_N_ELEMENTS (context_opacity_actions),
                                      context_opacity_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_paint_mode_actions,
                                      G_N_ELEMENTS (context_paint_mode_actions),
                                      context_paint_mode_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_tool_select_actions,
                                      G_N_ELEMENTS (context_tool_select_actions),
                                      context_tool_select_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_select_actions,
                                      G_N_ELEMENTS (context_brush_select_actions),
                                      context_brush_select_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_pattern_select_actions,
                                      G_N_ELEMENTS (context_pattern_select_actions),
                                      context_pattern_select_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_palette_select_actions,
                                      G_N_ELEMENTS (context_palette_select_actions),
                                      context_palette_select_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_gradient_select_actions,
                                      G_N_ELEMENTS (context_gradient_select_actions),
                                      context_gradient_select_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_font_select_actions,
                                      G_N_ELEMENTS (context_font_select_actions),
                                      context_font_select_cmd_callback);

  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spacing_actions,
                                      G_N_ELEMENTS (context_brush_spacing_actions),
                                      context_brush_spacing_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_shape_actions,
                                      G_N_ELEMENTS (context_brush_shape_actions),
                                      context_brush_shape_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_radius_actions,
                                      G_N_ELEMENTS (context_brush_radius_actions),
                                      context_brush_radius_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spikes_actions,
                                      G_N_ELEMENTS (context_brush_spikes_actions),
                                      context_brush_spikes_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_hardness_actions,
                                      G_N_ELEMENTS (context_brush_hardness_actions),
                                      context_brush_hardness_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_aspect_actions,
                                      G_N_ELEMENTS (context_brush_aspect_actions),
                                      context_brush_aspect_cmd_callback);
  clay_action_group_add_enum_actions (group, "context-action",
                                      context_brush_angle_actions,
                                      G_N_ELEMENTS (context_brush_angle_actions),
                                      context_brush_angle_cmd_callback);

  clay_action_group_add_toggle_actions (group, "context-action",
                                        context_toggle_actions,
                                        G_N_ELEMENTS (context_toggle_actions));
}

void
context_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
#if 0
  ClayContext *context   = action_data_get_context (data);
  gboolean     generated = FALSE;
  gdouble      radius    = 0.0;
  gint         spikes    = 0;
  gdouble      hardness  = 0.0;
  gdouble      aspect    = 0.0;
  gdouble      angle     = 0.0;

  if (context)
    {
      ClayBrush *brush = clay_context_get_brush (context);

      if (CLAY_IS_BRUSH_GENERATED (brush))
        {
          ClayBrushGenerated *gen = CLAY_BRUSH_GENERATED (brush);

          generated = TRUE;

          radius   = clay_brush_generated_get_radius       (gen);
          spikes   = clay_brush_generated_get_spikes       (gen);
          hardness = clay_brush_generated_get_hardness     (gen);
          aspect   = clay_brush_generated_get_aspect_ratio (gen);
          angle    = clay_brush_generated_get_angle        (gen);
        }
    }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, "context-" action, (condition) != 0)

  SET_SENSITIVE ("brush-radius-minimum",       generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease",      generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease-skip", generated && radius > 1.0);

  SET_SENSITIVE ("brush-radius-maximum",       generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase",      generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase-skip", generated && radius < 4000.0);

  SET_SENSITIVE ("brush-angle-minimum",       generated);
  SET_SENSITIVE ("brush-angle-decrease",      generated);
  SET_SENSITIVE ("brush-angle-decrease-skip", generated);

  SET_SENSITIVE ("brush-angle-maximum",       generated);
  SET_SENSITIVE ("brush-angle-increase",      generated);
  SET_SENSITIVE ("brush-angle-increase-skip", generated);
#undef SET_SENSITIVE

#endif
}
