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
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "gegl/clay-gegl-utils.h"

#include "core/clay-filter-history.h"
#include "core/clayimage.h"
#include "core/claygrouplayer.h"
#include "core/claylayermask.h"

#include "pdb/clayprocedure.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claystringaction.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "actions.h"
#include "filters-actions.h"
#include "filters-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     filters_actions_set_tooltips    (ClayActionGroup             *group,
                                                 const ClayStringActionEntry *entries,
                                                 gint                         n_entries);
static void     filters_actions_history_changed (Clay                        *clay,
                                                 ClayActionGroup             *group);
static gboolean filters_is_non_interactive      (const gchar                 *action_name);


/*  private variables  */

static const ClayStringActionEntry filters_actions[] =
{
  { "filters-antialias", CLAY_ICON_GEGL,
    NC_("filters-action", "_Antialias"), NULL, { NULL }, NULL,
    "gegl:antialias",
    CLAY_HELP_FILTER_ANTIALIAS },

  { "filters-color-enhance", CLAY_ICON_GEGL,
    NC_("filters-action", "_Color Enhance"), NULL, { NULL }, NULL,
    "gegl:color-enhance",
    CLAY_HELP_FILTER_COLOR_ENHANCE },

  { "filters-invert-linear", CLAY_ICON_INVERT,
    NC_("filters-action", "L_inear Invert"), NULL, { NULL }, NULL,
    "gegl:invert-linear",
    CLAY_HELP_FILTER_INVERT_LINEAR },

  { "filters-invert-perceptual", CLAY_ICON_INVERT,
    NC_("filters-action", "In_vert"), NULL, { NULL }, NULL,
    "gegl:invert-gamma",
    CLAY_HELP_FILTER_INVERT_PERCEPTUAL },

  { "filters-invert-value", CLAY_ICON_INVERT,
    NC_("filters-action", "_Value Invert"), NULL, { NULL }, NULL,
    "gegl:value-invert",
    CLAY_HELP_FILTER_INVERT_VALUE },

  { "filters-stretch-contrast-hsv", CLAY_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast HSV"), NULL, { NULL }, NULL,
    "gegl:stretch-contrast-hsv",
    CLAY_HELP_FILTER_STRETCH_CONTRAST_HSV }
};

static const ClayStringActionEntry filters_settings_actions[] =
{
  { "filters-dilate", CLAY_ICON_GEGL,
    NC_("filters-action", "_Dilate"), NULL, { NULL },
    NC_("filters-action", "Grow lighter areas of the image"),
    "gegl:value-propagate\n"
    "(mode white)"
    "(lower-threshold 0.000000)"
    "(upper-threshold 1.000000)"
    "(rate 1.000000)"
    "(top yes)"
    "(left yes)"
    "(right yes)"
    "(bottom yes)"
    "(value yes)"
    "(alpha no)",
    CLAY_HELP_FILTER_DILATE },

  { "filters-erode", CLAY_ICON_GEGL,
    NC_("filters-action", "_Erode"), NULL, { NULL },
    NC_("filters-action", "Grow darker areas of the image"),
    "gegl:value-propagate\n"
    "(mode black)"
    "(lower-threshold 0.000000)"
    "(upper-threshold 1.000000)"
    "(rate 1.000000)"
    "(top yes)"
    "(left yes)"
    "(right yes)"
    "(bottom yes)"
    "(value yes)"
    "(alpha no)",
    CLAY_HELP_FILTER_ERODE }
};

static const ClayStringActionEntry filters_interactive_actions[] =
{
  { "filters-alien-map", CLAY_ICON_GEGL,
    NC_("filters-action", "_Alien Map..."), NULL, { NULL }, NULL,
    "gegl:alien-map",
    CLAY_HELP_FILTER_ALIEN_MAP },

  { "filters-apply-canvas", CLAY_ICON_GEGL,
    NC_("filters-action", "_Apply Canvas..."), NULL, { NULL }, NULL,
    "gegl:texturize-canvas",
    CLAY_HELP_FILTER_APPLY_CANVAS },

  { "filters-apply-lens", CLAY_ICON_GEGL,
    NC_("filters-action", "Apply _Lens..."), NULL, { NULL }, NULL,
    "gegl:apply-lens",
    CLAY_HELP_FILTER_APPLY_LENS },

  { "filters-bayer-matrix", CLAY_ICON_GEGL,
    NC_("filters-action", "_Bayer Matrix..."), NULL, { NULL }, NULL,
    "gegl:bayer-matrix",
    CLAY_HELP_FILTER_BAYER_MATRIX },

  { "filters-bloom", CLAY_ICON_GEGL,
    NC_("filters-action", "_Bloom..."), NULL, { NULL }, NULL,
    "gegl:bloom",
    CLAY_HELP_FILTER_BLOOM },

  { "filters-brightness-contrast", CLAY_ICON_TOOL_BRIGHTNESS_CONTRAST,
    NC_("filters-action", "B_rightness-Contrast..."), NULL, { NULL }, NULL,
    "clay:brightness-contrast",
    CLAY_HELP_TOOL_BRIGHTNESS_CONTRAST },

  { "filters-bump-map", CLAY_ICON_GEGL,
    NC_("filters-action", "_Bump Map..."), NULL, { NULL }, NULL,
    "gegl:bump-map",
    CLAY_HELP_FILTER_BUMP_MAP },

  { "filters-c2g", CLAY_ICON_GEGL,
    NC_("filters-action", "_Color to Gray..."), NULL, { NULL }, NULL,
    "gegl:c2g",
    CLAY_HELP_FILTER_C2G },

  { "filters-cartoon", CLAY_ICON_GEGL,
    NC_("filters-action", "Ca_rtoon..."), NULL, { NULL }, NULL,
    "gegl:cartoon",
    CLAY_HELP_FILTER_CARTOON },

  { "filters-channel-mixer", CLAY_ICON_GEGL,
    NC_("filters-action", "_Channel Mixer..."), NULL, { NULL }, NULL,
    "gegl:channel-mixer",
    CLAY_HELP_FILTER_CHANNEL_MIXER },

  { "filters-checkerboard", CLAY_ICON_GEGL,
    NC_("filters-action", "_Checkerboard..."), NULL, { NULL }, NULL,
    "gegl:checkerboard",
    CLAY_HELP_FILTER_CHECKERBOARD },

  { "filters-color-balance", CLAY_ICON_TOOL_COLOR_BALANCE,
    NC_("filters-action", "Color _Balance..."), NULL, { NULL }, NULL,
    "clay:color-balance",
    CLAY_HELP_TOOL_COLOR_BALANCE },

  { "filters-color-exchange", CLAY_ICON_GEGL,
    NC_("filters-action", "_Color Exchange..."), NULL, { NULL }, NULL,
    "gegl:color-exchange",
    CLAY_HELP_FILTER_COLOR_EXCHANGE },

  { "filters-colorize", CLAY_ICON_TOOL_COLORIZE,
    NC_("filters-action", "Colori_ze..."), NULL, { NULL }, NULL,
    "clay:colorize",
    CLAY_HELP_TOOL_COLORIZE },

  { "filters-dither", CLAY_ICON_GEGL,
    NC_("filters-action", "Dithe_r..."), NULL, { NULL }, NULL,
    "gegl:dither",
    CLAY_HELP_FILTER_DITHER },

  { "filters-color-rotate", CLAY_ICON_GEGL,
    NC_("filters-action", "_Rotate Colors..."), NULL, { NULL }, NULL,
    "gegl:color-rotate",
    CLAY_HELP_FILTER_COLOR_ROTATE },

  { "filters-color-temperature", CLAY_ICON_TOOL_COLOR_TEMPERATURE,
    NC_("filters-action", "Color T_emperature..."), NULL, { NULL }, NULL,
    "gegl:color-temperature",
    CLAY_HELP_FILTER_COLOR_TEMPERATURE },

  { "filters-color-to-alpha", CLAY_ICON_GEGL,
    NC_("filters-action", "Color to _Alpha..."), NULL, { NULL }, NULL,
    "gegl:color-to-alpha",
    CLAY_HELP_FILTER_COLOR_TO_ALPHA },

  { "filters-component-extract", CLAY_ICON_GEGL,
    NC_("filters-action", "_Extract Component..."), NULL, { NULL }, NULL,
    "gegl:component-extract",
    CLAY_HELP_FILTER_COMPONENT_EXTRACT },

  { "filters-convolution-matrix", CLAY_ICON_GEGL,
    NC_("filters-action", "_Convolution Matrix..."), NULL, { NULL }, NULL,
    "gegl:convolution-matrix",
    CLAY_HELP_FILTER_CONVOLUTION_MATRIX },

  { "filters-cubism", CLAY_ICON_GEGL,
    NC_("filters-action", "_Cubism..."), NULL, { NULL }, NULL,
    "gegl:cubism",
    CLAY_HELP_FILTER_CUBISM },

  { "filters-curves", CLAY_ICON_TOOL_CURVES,
    NC_("filters-action", "_Curves..."), NULL, { NULL }, NULL,
    "clay:curves",
    CLAY_HELP_TOOL_CURVES },

  { "filters-deinterlace", CLAY_ICON_GEGL,
    NC_("filters-action", "_Deinterlace..."), NULL, { NULL }, NULL,
    "gegl:deinterlace",
    CLAY_HELP_FILTER_DEINTERLACE },

  { "filters-desaturate", CLAY_ICON_TOOL_DESATURATE,
    NC_("filters-action", "_Desaturate..."), NULL, { NULL }, NULL,
    "clay:desaturate",
    CLAY_HELP_FILTER_DESATURATE },

  { "filters-difference-of-gaussians", CLAY_ICON_GEGL,
    NC_("filters-action", "Difference of _Gaussians..."), NULL, { NULL }, NULL,
    "gegl:difference-of-gaussians",
    CLAY_HELP_FILTER_DIFFERENCE_OF_GAUSSIANS },

  { "filters-diffraction-patterns", CLAY_ICON_GEGL,
    NC_("filters-action", "D_iffraction Patterns..."), NULL, { NULL }, NULL,
    "gegl:diffraction-patterns",
    CLAY_HELP_FILTER_DIFFRACTION_PATTERNS },

  { "filters-displace", CLAY_ICON_GEGL,
    NC_("filters-action", "_Displace..."), NULL, { NULL }, NULL,
    "gegl:displace",
    CLAY_HELP_FILTER_DISPLACE },

  { "filters-distance-map", CLAY_ICON_GEGL,
    NC_("filters-action", "Distance _Map..."), NULL, { NULL }, NULL,
    "gegl:distance-transform",
    CLAY_HELP_FILTER_DISTANCE_MAP },

  { "filters-dropshadow", CLAY_ICON_GEGL,
    NC_("filters-action", "_Drop Shadow..."), NULL, { NULL }, NULL,
    "gegl:dropshadow",
    CLAY_HELP_FILTER_DROPSHADOW },

  { "filters-edge", CLAY_ICON_GEGL,
    NC_("filters-action", "_Edge..."), NULL, { NULL }, NULL,
    "gegl:edge",
    CLAY_HELP_FILTER_EDGE },

  { "filters-edge-laplace", CLAY_ICON_GEGL,
    NC_("filters-action", "_Laplace..."), NULL, { NULL }, NULL,
    "gegl:edge-laplace",
    CLAY_HELP_FILTER_EDGE_LAPLACE },

  { "filters-edge-neon", CLAY_ICON_GEGL,
    NC_("filters-action", "_Neon..."), NULL, { NULL }, NULL,
    "gegl:edge-neon",
    CLAY_HELP_FILTER_EDGE_NEON },

  { "filters-edge-sobel", CLAY_ICON_GEGL,
    NC_("filters-action", "_Sobel..."), NULL, { NULL }, NULL,
    "gegl:edge-sobel",
    CLAY_HELP_FILTER_EDGE_SOBEL },

  { "filters-emboss", CLAY_ICON_GEGL,
    NC_("filters-action", "_Emboss..."), NULL, { NULL }, NULL,
    "gegl:emboss",
    CLAY_HELP_FILTER_EMBOSS },

  { "filters-engrave", CLAY_ICON_GEGL,
    NC_("filters-action", "En_grave..."), NULL, { NULL }, NULL,
    "gegl:engrave",
    CLAY_HELP_FILTER_ENGRAVE },

  { "filters-exposure", CLAY_ICON_TOOL_EXPOSURE,
    NC_("filters-action", "E_xposure..."), NULL, { NULL }, NULL,
    "gegl:exposure",
    CLAY_HELP_FILTER_EXPOSURE },

  { "filters-fattal-2002", CLAY_ICON_GEGL,
    NC_("filters-action", "_Fattal et al. 2002..."), NULL, { NULL }, NULL,
    "gegl:fattal02",
    CLAY_HELP_FILTER_FATTAL_2002 },

  { "filters-focus-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "_Focus Blur..."), NULL, { NULL }, NULL,
    "gegl:focus-blur",
    CLAY_HELP_FILTER_FOCUS_BLUR },

  { "filters-fractal-trace", CLAY_ICON_GEGL,
    NC_("filters-action", "_Fractal Trace..."), NULL, { NULL }, NULL,
    "gegl:fractal-trace",
    CLAY_HELP_FILTER_FRACTAL_TRACE },

  { "filters-gaussian-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "_Gaussian Blur..."), NULL, { NULL }, NULL,
    "gegl:gaussian-blur",
    CLAY_HELP_FILTER_GAUSSIAN_BLUR },

  { "filters-gaussian-blur-selective", CLAY_ICON_GEGL,
    NC_("filters-action", "_Selective Gaussian Blur..."), NULL, { NULL }, NULL,
    "gegl:gaussian-blur-selective",
    CLAY_HELP_FILTER_GAUSSIAN_BLUR_SELECTIVE },

  { "filters-gegl-graph", CLAY_ICON_GEGL,
    NC_("filters-action", "_GEGL Graph..."), NULL, { NULL }, NULL,
    "gegl:gegl",
    CLAY_HELP_FILTER_GEGL_GRAPH },

  { "filters-grid", CLAY_ICON_GRID,
    NC_("filters-action", "_Grid..."), NULL, { NULL }, NULL,
    "gegl:grid",
    CLAY_HELP_FILTER_GRID },

  { "filters-high-pass", CLAY_ICON_GEGL,
    NC_("filters-action", "_High Pass..."), NULL, { NULL }, NULL,
    "gegl:high-pass",
    CLAY_HELP_FILTER_HIGH_PASS },

  { "filters-hue-chroma", CLAY_ICON_GEGL,
    NC_("filters-action", "Hue-_Chroma..."), NULL, { NULL }, NULL,
    "gegl:hue-chroma",
    CLAY_HELP_FILTER_HUE_CHROMA },

  { "filters-hue-saturation", CLAY_ICON_TOOL_HUE_SATURATION,
    NC_("filters-action", "Hue-_Saturation..."), NULL, { NULL }, NULL,
    "clay:hue-saturation",
    CLAY_HELP_TOOL_HUE_SATURATION },

  { "filters-illusion", CLAY_ICON_GEGL,
    NC_("filters-action", "_Illusion..."), NULL, { NULL }, NULL,
    "gegl:illusion",
    CLAY_HELP_FILTER_ILLUSION },

  { "filters-image-gradient", CLAY_ICON_GEGL,
    NC_("filters-action", "_Image Gradient..."), NULL, { NULL }, NULL,
    "gegl:image-gradient",
    CLAY_HELP_FILTER_IMAGE_GRADIENT },

  { "filters-kaleidoscope", CLAY_ICON_GEGL,
    NC_("filters-action", "_Kaleidoscope..."), NULL, { NULL }, NULL,
    "gegl:mirrors",
    CLAY_HELP_FILTER_KALEIDOSCOPE },

  { "filters-lens-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "Le_ns Blur..."), NULL, { NULL }, NULL,
    "gegl:lens-blur",
    CLAY_HELP_FILTER_LENS_BLUR },

  { "filters-lens-distortion", CLAY_ICON_GEGL,
    NC_("filters-action", "Le_ns Distortion..."), NULL, { NULL }, NULL,
    "gegl:lens-distortion",
    CLAY_HELP_FILTER_LENS_DISTORTION },

  { "filters-lens-flare", CLAY_ICON_GEGL,
    NC_("filters-action", "Lens _Flare..."), NULL, { NULL }, NULL,
    "gegl:lens-flare",
    CLAY_HELP_FILTER_LENS_FLARE },

  { "filters-levels", CLAY_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Levels..."), NULL, { NULL }, NULL,
    "clay:levels",
    CLAY_HELP_TOOL_LEVELS },

  { "filters-linear-sinusoid", CLAY_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Linear Sinusoid..."), NULL, { NULL }, NULL,
    "gegl:linear-sinusoid",
    CLAY_HELP_FILTER_LINEAR_SINUSOID },

  { "filters-little-planet", CLAY_ICON_GEGL,
    NC_("filters-action", "_Little Planet..."), NULL, { NULL }, NULL,
    "gegl:stereographic-projection",
    CLAY_HELP_FILTER_LITTLE_PLANET },

  { "filters-long-shadow", CLAY_ICON_GEGL,
    NC_("filters-action", "_Long Shadow..."), NULL, { NULL }, NULL,
    "gegl:long-shadow",
    CLAY_HELP_FILTER_LONG_SHADOW },

  { "filters-mantiuk-2006", CLAY_ICON_GEGL,
    NC_("filters-action", "_Mantiuk 2006..."), NULL, { NULL }, NULL,
    "gegl:mantiuk06",
    CLAY_HELP_FILTER_MANTIUK_2006 },

  { "filters-maze", CLAY_ICON_GEGL,
    NC_("filters-action", "_Maze..."), NULL, { NULL }, NULL,
    "gegl:maze",
    CLAY_HELP_FILTER_MAZE },

  { "filters-mean-curvature-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "Mean C_urvature Blur..."), NULL, { NULL }, NULL,
    "gegl:mean-curvature-blur",
    CLAY_HELP_FILTER_MEAN_CURVATURE_BLUR },

  { "filters-median-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "_Median Blur..."), NULL, { NULL }, NULL,
    "gegl:median-blur",
    CLAY_HELP_FILTER_MEDIAN_BLUR },

  { "filters-mono-mixer", CLAY_ICON_GEGL,
    NC_("filters-action", "_Mono Mixer..."), NULL, { NULL }, NULL,
    "gegl:mono-mixer",
    CLAY_HELP_FILTER_MONO_MIXER },

  { "filters-mosaic", CLAY_ICON_GEGL,
    NC_("filters-action", "_Mosaic..."), NULL, { NULL }, NULL,
    "gegl:mosaic",
    CLAY_HELP_FILTER_MOSAIC },

  { "filters-motion-blur-circular", CLAY_ICON_GEGL,
    NC_("filters-action", "_Circular Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-circular",
    CLAY_HELP_FILTER_MOTION_BLUR_CIRCULAR },

  { "filters-motion-blur-linear", CLAY_ICON_GEGL,
    NC_("filters-action", "_Linear Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-linear",
    CLAY_HELP_FILTER_MOTION_BLUR_LINEAR },

  { "filters-motion-blur-zoom", CLAY_ICON_GEGL,
    NC_("filters-action", "_Zoom Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-zoom",
    CLAY_HELP_FILTER_MOTION_BLUR_ZOOM },

  { "filters-noise-cell", CLAY_ICON_GEGL,
    NC_("filters-action", "_Cell Noise..."), NULL, { NULL }, NULL,
    "gegl:cell-noise",
    CLAY_HELP_FILTER_NOISE_CELL },

  { "filters-newsprint", CLAY_ICON_GEGL,
    NC_("filters-action", "_Newsprint..."), NULL, { NULL }, NULL,
    "gegl:newsprint",
    CLAY_HELP_FILTER_NEWSPRINT },

  { "filters-noise-cie-lch", CLAY_ICON_GEGL,
    NC_("filters-action", "_CIE lch Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-cie-lch",
    CLAY_HELP_FILTER_NOISE_CIE_LCH },

  { "filters-noise-hsv", CLAY_ICON_GEGL,
    NC_("filters-action", "HS_V Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-hsv",
    CLAY_HELP_FILTER_NOISE_HSV },

  { "filters-noise-hurl", CLAY_ICON_GEGL,
    NC_("filters-action", "_Hurl..."), NULL, { NULL }, NULL,
    "gegl:noise-hurl",
    CLAY_HELP_FILTER_NOISE_HURL },

  { "filters-noise-perlin", CLAY_ICON_GEGL,
    NC_("filters-action", "Perlin _Noise..."), NULL, { NULL }, NULL,
    "gegl:perlin-noise",
    CLAY_HELP_FILTER_NOISE_PERLIN },

  { "filters-noise-pick", CLAY_ICON_GEGL,
    NC_("filters-action", "_Pick..."), NULL, { NULL }, NULL,
    "gegl:noise-pick",
    CLAY_HELP_FILTER_NOISE_PICK },

  { "filters-noise-rgb", CLAY_ICON_GEGL,
    NC_("filters-action", "_RGB Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-rgb",
    CLAY_HELP_FILTER_NOISE_RGB },

  { "filters-noise-reduction", CLAY_ICON_GEGL,
    NC_("filters-action", "Noise R_eduction..."), NULL, { NULL }, NULL,
    "gegl:noise-reduction",
    CLAY_HELP_FILTER_NOISE_REDUCTION },

  { "filters-noise-simplex", CLAY_ICON_GEGL,
    NC_("filters-action", "_Simplex Noise..."), NULL, { NULL }, NULL,
    "gegl:simplex-noise",
    CLAY_HELP_FILTER_NOISE_SIMPLEX },

  { "filters-noise-slur", CLAY_ICON_GEGL,
    NC_("filters-action", "_Slur..."), NULL, { NULL }, NULL,
    "gegl:noise-slur",
    CLAY_HELP_FILTER_NOISE_SLUR },

  { "filters-noise-solid", CLAY_ICON_GEGL,
    NC_("filters-action", "_Solid Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-solid",
    CLAY_HELP_FILTER_NOISE_SOLID },

  { "filters-noise-spread", CLAY_ICON_GEGL,
    NC_("filters-action", "Sp_read..."), NULL, { NULL }, NULL,
    "gegl:noise-spread",
    CLAY_HELP_FILTER_NOISE_SPREAD },

  { "filters-normal-map", CLAY_ICON_GEGL,
    NC_("filters-action", "_Normal Map..."), NULL, { NULL }, NULL,
    "gegl:normal-map",
    CLAY_HELP_FILTER_NORMAL_MAP },

  { "filters-offset", CLAY_ICON_TOOL_OFFSET,
    NC_("filters-action", "_Offset..."), NULL, { "<primary><shift>O", NULL }, NULL,
    "clay:offset",
    CLAY_HELP_TOOL_OFFSET },

  { "filters-oilify", CLAY_ICON_GEGL,
    NC_("filters-action", "Oili_fy..."), NULL, { NULL }, NULL,
    "gegl:oilify",
    CLAY_HELP_FILTER_OILIFY },

  { "filters-panorama-projection", CLAY_ICON_GEGL,
    NC_("filters-action", "_Panorama Projection..."), NULL, { NULL }, NULL,
    "gegl:panorama-projection",
    CLAY_HELP_FILTER_PANORAMA_PROJECTION },

  { "filters-photocopy", CLAY_ICON_GEGL,
    NC_("filters-action", "_Photocopy..."), NULL, { NULL }, NULL,
    "gegl:photocopy",
    CLAY_HELP_FILTER_PHOTOCOPY },

  { "filters-pixelize", CLAY_ICON_GEGL,
    NC_("filters-action", "_Pixelize..."), NULL, { NULL }, NULL,
    "gegl:pixelize",
    CLAY_HELP_FILTER_PIXELIZE },

  { "filters-plasma", CLAY_ICON_GEGL,
    NC_("filters-action", "_Plasma..."), NULL, { NULL }, NULL,
    "gegl:plasma",
    CLAY_HELP_FILTER_PLASMA },

  { "filters-polar-coordinates", CLAY_ICON_GEGL,
    NC_("filters-action", "P_olar Coordinates..."), NULL, { NULL }, NULL,
    "gegl:polar-coordinates",
    CLAY_HELP_FILTER_POLAR_COORDINATES },

  { "filters-posterize", CLAY_ICON_TOOL_POSTERIZE,
    NC_("filters-action", "_Posterize..."), NULL, { NULL }, NULL,
    "clay:posterize",
    CLAY_HELP_FILTER_POSTERIZE },

  { "filters-recursive-transform", CLAY_ICON_GEGL,
    NC_("filters-action", "_Recursive Transform..."), NULL, { NULL }, NULL,
    "gegl:recursive-transform",
    CLAY_HELP_FILTER_RECURSIVE_TRANSFORM },

  { "filters-red-eye-removal", CLAY_ICON_GEGL,
    NC_("filters-action", "_Red Eye Removal..."), NULL, { NULL }, NULL,
    "gegl:red-eye-removal",
    CLAY_HELP_FILTER_RED_EYE_REMOVAL },

  { "filters-reinhard-2005", CLAY_ICON_GEGL,
    NC_("filters-action", "_Reinhard 2005..."), NULL, { NULL }, NULL,
    "gegl:reinhard05",
    CLAY_HELP_FILTER_REINHARD_2005 },

  { "filters-rgb-clip", CLAY_ICON_GEGL,
    NC_("filters-action", "RGB _Clip..."), NULL, { NULL }, NULL,
    "gegl:rgb-clip",
    CLAY_HELP_FILTER_RGB_CLIP },

  { "filters-ripple", CLAY_ICON_GEGL,
    NC_("filters-action", "_Ripple..."), NULL, { NULL }, NULL,
    "gegl:ripple",
    CLAY_HELP_FILTER_RIPPLE },

  { "filters-saturation", CLAY_ICON_GEGL,
    NC_("filters-action", "Sat_uration..."), NULL, { NULL }, NULL,
    "gegl:saturation",
    CLAY_HELP_FILTER_SATURATION },

  { "filters-semi-flatten", CLAY_ICON_GEGL,
    NC_("filters-action", "_Semi-Flatten..."), NULL, { NULL }, NULL,
    "clay:semi-flatten",
    CLAY_HELP_FILTER_SEMI_FLATTEN },

  { "filters-sepia", CLAY_ICON_GEGL,
    NC_("filters-action", "_Sepia..."), NULL, { NULL }, NULL,
    "gegl:sepia",
    CLAY_HELP_FILTER_SEPIA },

  { "filters-shadows-highlights", CLAY_ICON_TOOL_SHADOWS_HIGHLIGHTS,
    NC_("filters-action", "S_hadows-Highlights..."), NULL, { NULL }, NULL,
    "gegl:shadows-highlights",
    CLAY_HELP_FILTER_SHADOWS_HIGHLIGHTS },

  { "filters-shift", CLAY_ICON_GEGL,
    NC_("filters-action", "_Shift..."), NULL, { NULL }, NULL,
    "gegl:shift",
    CLAY_HELP_FILTER_SHIFT },

  { "filters-sinus", CLAY_ICON_GEGL,
    NC_("filters-action", "_Sinus..."), NULL, { NULL }, NULL,
    "gegl:sinus",
    CLAY_HELP_FILTER_SINUS },

  { "filters-slic", CLAY_ICON_GEGL,
    NC_("filters-action", "_Simple Linear Iterative Clustering..."), NULL, { NULL }, NULL,
    "gegl:slic",
    CLAY_HELP_FILTER_SLIC },

  { "filters-snn-mean", CLAY_ICON_GEGL,
    NC_("filters-action", "_Symmetric Nearest Neighbor..."), NULL, { NULL }, NULL,
    "gegl:snn-mean",
    CLAY_HELP_FILTER_SNN_MEAN },

  { "filters-softglow", CLAY_ICON_GEGL,
    NC_("filters-action", "_Softglow..."), NULL, { NULL }, NULL,
    "gegl:softglow",
    CLAY_HELP_FILTER_SOFTGLOW },

  { "filters-spherize", CLAY_ICON_GEGL,
    NC_("filters-action", "Spheri_ze..."), NULL, { NULL }, NULL,
    "gegl:spherize",
    CLAY_HELP_FILTER_SPHERIZE },

  { "filters-spiral", CLAY_ICON_GEGL,
    NC_("filters-action", "S_piral..."), NULL, { NULL }, NULL,
    "gegl:spiral",
    CLAY_HELP_FILTER_SPIRAL },

  { "filters-stretch-contrast", CLAY_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast..."), NULL, { NULL }, NULL,
    "gegl:stretch-contrast",
    CLAY_HELP_FILTER_STRETCH_CONTRAST },

  { "filters-stress", CLAY_ICON_GEGL,
    NC_("filters-action", "_Stress..."), NULL, { NULL }, NULL,
    "gegl:stress",
    CLAY_HELP_FILTER_STRESS },

  { "filters-supernova", CLAY_ICON_GEGL,
    NC_("filters-action", "Super_nova..."), NULL, { NULL }, NULL,
    "gegl:supernova",
    CLAY_HELP_FILTER_SUPERNOVA },

  { "filters-threshold", CLAY_ICON_TOOL_THRESHOLD,
    NC_("filters-action", "_Threshold..."), NULL, { NULL }, NULL,
    "clay:threshold",
    CLAY_HELP_TOOL_THRESHOLD },

  { "filters-threshold-alpha", CLAY_ICON_GEGL,
    NC_("filters-action", "_Threshold Alpha..."), NULL, { NULL }, NULL,
    "clay:threshold-alpha",
    CLAY_HELP_FILTER_THRESHOLD_ALPHA },

  { "filters-tile-glass", CLAY_ICON_GEGL,
    NC_("filters-action", "_Glass Tile..."), NULL, { NULL }, NULL,
    "gegl:tile-glass",
    CLAY_HELP_FILTER_TILE_GLASS },

  { "filters-tile-paper", CLAY_ICON_GEGL,
    NC_("filters-action", "_Paper Tile..."), NULL, { NULL }, NULL,
    "gegl:tile-paper",
    CLAY_HELP_FILTER_TILE_PAPER },

  { "filters-tile-seamless", CLAY_ICON_GEGL,
    NC_("filters-action", "_Tile Seamless..."), NULL, { NULL }, NULL,
    "gegl:tile-seamless",
    CLAY_HELP_FILTER_TILE_SEAMLESS },

  { "filters-unsharp-mask", CLAY_ICON_GEGL,
    NC_("filters-action", "Sharpen (_Unsharp Mask)..."), NULL, { NULL }, NULL,
    "gegl:unsharp-mask",
    CLAY_HELP_FILTER_UNSHARP_MASK },

  { "filters-value-propagate", CLAY_ICON_GEGL,
    NC_("filters-action", "_Value Propagate..."), NULL, { NULL }, NULL,
    "gegl:value-propagate",
    CLAY_HELP_FILTER_VALUE_PROPAGATE },

  { "filters-variable-blur", CLAY_ICON_GEGL,
    NC_("filters-action", "_Variable Blur..."), NULL, { NULL }, NULL,
    "gegl:variable-blur",
    CLAY_HELP_FILTER_VARIABLE_BLUR },

  { "filters-video-degradation", CLAY_ICON_GEGL,
    NC_("filters-action", "Vi_deo Degradation..."), NULL, { NULL }, NULL,
    "gegl:video-degradation",
    CLAY_HELP_FILTER_VIDEO_DEGRADATION },

  { "filters-vignette", CLAY_ICON_GEGL,
    NC_("filters-action", "_Vignette..."), NULL, { NULL }, NULL,
    "gegl:vignette",
    CLAY_HELP_FILTER_VIGNETTE },

  { "filters-waterpixels", CLAY_ICON_GEGL,
    NC_("filters-action", "_Waterpixels..."), NULL, { NULL }, NULL,
    "gegl:waterpixels",
    CLAY_HELP_FILTER_WATERPIXELS },

  { "filters-waves", CLAY_ICON_GEGL,
    NC_("filters-action", "_Waves..."), NULL, { NULL }, NULL,
    "gegl:waves",
    CLAY_HELP_FILTER_WAVES },

  { "filters-whirl-pinch", CLAY_ICON_GEGL,
    NC_("filters-action", "W_hirl and Pinch..."), NULL, { NULL }, NULL,
    "gegl:whirl-pinch",
    CLAY_HELP_FILTER_WHIRL_PINCH },

  { "filters-wind", CLAY_ICON_GEGL,
    NC_("filters-action", "W_ind..."), NULL, { NULL }, NULL,
    "gegl:wind",
    CLAY_HELP_FILTER_WIND }
};

static const ClayEnumActionEntry filters_repeat_actions[] =
{
  { "filters-repeat", CLAY_ICON_SYSTEM_RUN,
    NC_("filters-action", "Re_peat Last"), NULL, { "<primary>F", NULL },
    NC_("filters-action",
        "Rerun the last used filter using the same settings"),
    CLAY_RUN_WITH_LAST_VALS, FALSE,
    CLAY_HELP_FILTER_REPEAT },

  { "filters-reshow", CLAY_ICON_DIALOG_RESHOW_FILTER,
    NC_("filters-action", "R_e-Show Last"), NULL, { "<primary><shift>F", NULL },
    NC_("filters-action", "Show the last used filter dialog again"),
    CLAY_RUN_INTERACTIVE, FALSE,
    CLAY_HELP_FILTER_RESHOW }
};


void
filters_actions_setup (ClayActionGroup *group)
{
  static gboolean           first_setup = TRUE;
  ClayProcedureActionEntry *entries;
  gint                      n_entries;
  gint                      i;
  GList                    *op_classes;
  GList                    *iter;
  GStrvBuilder             *gegl_actions;

  clay_action_group_add_string_actions (group, "filters-action",
                                        filters_actions,
                                        G_N_ELEMENTS (filters_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_actions,
                                G_N_ELEMENTS (filters_actions));

  clay_action_group_add_string_actions (group, "filters-action",
                                        filters_settings_actions,
                                        G_N_ELEMENTS (filters_settings_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_settings_actions,
                                G_N_ELEMENTS (filters_settings_actions));

  clay_action_group_add_string_actions (group, "filters-action",
                                        filters_interactive_actions,
                                        G_N_ELEMENTS (filters_interactive_actions),
                                        filters_apply_interactive_cmd_callback);
  filters_actions_set_tooltips (group, filters_interactive_actions,
                                G_N_ELEMENTS (filters_interactive_actions));

  /* XXX Hardcoded list to prevent expensive node/graph creation of a
   * well-known list of operations.
   * This whole code will disappear when we'll support filters with aux
   * input non-destructively anyway.
   */
  if (first_setup)
    {
      actions_filter_set_aux ("filters-variable-blur");
      actions_filter_set_aux ("filters-oilify");
      actions_filter_set_aux ("filters-lens-blur");
      actions_filter_set_aux ("filters-gaussian-blur-selective");
      actions_filter_set_aux ("filters-displace");
      actions_filter_set_aux ("filters-bump-map");
    }

  gegl_actions = g_strv_builder_new ();
  op_classes   = clay_gegl_get_op_classes (TRUE);

  for (iter = op_classes; iter; iter = iter->next)
    {
      GeglOperationClass    *op_class = GEGL_OPERATION_CLASS (iter->data);
      ClayStringActionEntry  entry   = { 0, };
      gchar                 *formatted_op_name;
      gchar                 *action_name;
      const gchar           *title;
      const gchar           *op_name;
      gchar                 *label;

      if (filters_actions_gegl_op_blocklisted (op_class->name))
        continue;

      formatted_op_name = g_strdup (op_class->name);
      clay_make_valid_action_name (formatted_op_name);
      action_name = g_strdup_printf ("filters-%s", formatted_op_name);
      i           = 2;
      while (clay_action_group_get_action (group, action_name) != NULL)
        {
          /* In the off-chance that after formatting to a valid action name, 2
           * operations end up generating the same action name. Typically we
           * don't want a third-party operation called "my-op" to have the same
           * action name than "my_op" (which is to say that one will be
           * overridden by the other).
           */
          g_free (action_name);
          action_name = g_strdup_printf ("filters-%s-%d", formatted_op_name, i++);
        }

      title   = gegl_operation_class_get_key (op_class, "title");
      op_name = op_class->name;

      if (g_str_has_prefix (op_name, "gegl:"))
        op_name += strlen ("gegl:");

      if (title)
        label = g_strdup_printf ("%s (%s)", title, op_name);
      else
        label = g_strdup (op_name);

      entry.name      = action_name;
      entry.icon_name = CLAY_ICON_GEGL;
      entry.label     = label;
      entry.tooltip   = gegl_operation_class_get_key (op_class, "description");
      entry.value     = op_class->name;
      entry.help_id   = CLAY_HELP_TOOL_GEGL;

      if (gegl_operation_class_get_key (op_class, "clay:menu-path") &&
          g_str_has_prefix (op_class->name, "gegl:"))
        /* We automatically create an help ID from the operation name
         * for all core GEGL operations with a menu path key.
         */
        entry.help_id = formatted_op_name;

      clay_action_group_add_string_actions (group, "filters-action",
                                            &entry, 1,
                                            filters_apply_interactive_cmd_callback);

      if (gegl_operation_class_get_key (op_class, "clay:menu-label"))
        {
          ClayAction *action;

          action = clay_action_group_get_action (group, action_name);

          clay_action_set_short_label (action,
                                       gegl_operation_class_get_key (op_class,
                                                                     "clay:menu-label"));
        }
      else if (title)
        {
          ClayAction *action;
          /* TRANSLATORS: %s is the title of a GEGL operation (filter), after
           * which we append "..." as the standardized labelling to indicate
           * that this action raises a dialog.
           */
          gchar      *short_label = g_strdup_printf (_("%s..."), title);

          action = clay_action_group_get_action (group, action_name);
          clay_action_set_short_label (action, short_label);
          g_free (short_label);
        }

      /* Identify third-party filters based on operations with an
       * auxiliary pad in first setup because of slowness on Windows.
       * See #14781.
       */
      if (first_setup)
        {
          GeglNode *node = gegl_node_new ();

          gegl_node_set (node,
                         "operation", op_class->name,
                         NULL);

          if (gegl_node_has_pad (node, "aux"))
            actions_filter_set_aux (action_name);

          g_clear_object (&node);
        }
      g_strv_builder_add (gegl_actions, action_name);

      g_free (label);
      g_free (action_name);
      g_free (formatted_op_name);
    }

  g_object_set_data_full (G_OBJECT (group),
                          "filters-group-generated-gegl-actions",
                          g_strv_builder_end (gegl_actions),
                          (GDestroyNotify) g_strfreev);
  g_strv_builder_unref (gegl_actions);
  g_list_free (op_classes);

  clay_action_group_add_enum_actions (group, "filters-action",
                                      filters_repeat_actions,
                                      G_N_ELEMENTS (filters_repeat_actions),
                                      filters_repeat_cmd_callback);

  n_entries = clay_filter_history_size (group->clay);

  entries = g_new0 (ClayProcedureActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name        = g_strdup_printf ("filters-recent-%02d", i + 1);
      entries[i].icon_name   = NULL;
      entries[i].label       = "";
      entries[i].tooltip     = NULL;
      entries[i].procedure   = NULL;
      entries[i].help_id     = CLAY_HELP_FILTER_RESHOW;
    }

  clay_action_group_add_procedure_actions (group, entries, n_entries,
                                           filters_history_cmd_callback);

  for (i = 0; i < n_entries; i++)
    {
      clay_action_group_set_action_visible (group, entries[i].name, FALSE);
      g_free ((gchar *) entries[i].name);
    }

  g_free (entries);

  g_signal_connect_object (group->clay, "filter-history-changed",
                           G_CALLBACK (filters_actions_history_changed),
                           group, 0);

  filters_actions_history_changed (group->clay, group);

  first_setup = FALSE;
}

void
filters_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
  ClayImage    *image;
  GList        *actions;
  GList        *iter;
  gboolean      writable       = FALSE;
  gboolean      gray           = FALSE;
  gboolean      alpha          = FALSE;
  gboolean      supports_alpha = FALSE;
  gboolean      force_nde      = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      GList *drawables;

      drawables = clay_image_get_selected_drawables (image);

      if (g_list_length (drawables) == 1)
        {
          ClayDrawable *drawable = drawables->data;
          ClayItem     *item;

          gray           = clay_drawable_is_gray (drawable);
          alpha          = clay_drawable_has_alpha (drawable);
          supports_alpha = clay_drawable_supports_alpha (drawable);

          if (CLAY_IS_LAYER_MASK (drawable))
            item = CLAY_ITEM (clay_layer_mask_get_layer (CLAY_LAYER_MASK (drawable)));
          else
            item = CLAY_ITEM (drawable);

          writable = ! clay_item_is_content_locked (item, NULL);

          if (CLAY_IS_GROUP_LAYER (drawable)                    ||
              (clay_item_is_rasterizable (CLAY_ITEM (drawable)) &&
               ! clay_item_is_rasterized (CLAY_ITEM (drawable))))
            force_nde = TRUE;
        }

      g_list_free (drawables);
   }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  actions = clay_action_group_list_actions (group);
  for (iter = actions; iter; iter = iter->next)
    {
      ClayAction  *action = iter->data;
      const gchar *action_name;

      action_name = clay_action_get_name (action);

      if (filters_is_non_interactive (action_name))
        {
          SET_SENSITIVE (action_name, writable);
        }
      else if (CLAY_IS_STRING_ACTION (action))
        {
          const gchar *opname;

          opname = CLAY_STRING_ACTION (action)->value;

          if (opname == NULL)
            /* These are the filters-recent-*, repeat and reshow handled
             * below.
             */
            continue;

          if (g_strcmp0 (opname, "gegl:gegl") == 0)
            {
              /* GEGL graph filter can only be run destructively, unless
               * the CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT environment
               * variable is set.
               */
              SET_SENSITIVE (clay_action_get_name (action), writable &&
                             (g_getenv ("CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT") != NULL || ! force_nde));
            }
          else if (gegl_has_operation (opname))
            {
              gboolean sensitive = writable;

              if (sensitive && force_nde)
                /* Operations with auxiliary inputs can only be applied
                 * destructively. Therefore they must be deactivated on
                 * types of layers where filters can only be applied
                 * non-destructively.
                 */
                sensitive = ! actions_filter_get_aux (action_name);

              SET_SENSITIVE (clay_action_get_name (action), sensitive);
            }
        }
    }
  g_list_free (actions);

  /* Special-cased filters */
  SET_SENSITIVE ("filters-c2g",                     writable && !gray);
  SET_SENSITIVE ("filters-color-balance",           writable && !gray);
  SET_SENSITIVE ("filters-color-enhance",           writable && !force_nde && !gray);
  SET_SENSITIVE ("filters-colorize",                writable && !gray);
  SET_SENSITIVE ("filters-color-temperature",       writable && !gray);
  SET_SENSITIVE ("filters-color-to-alpha",          writable && supports_alpha);
  SET_SENSITIVE ("filters-desaturate",              writable && !gray);

  SET_SENSITIVE ("filters-dropshadow",              writable && alpha);
  SET_SENSITIVE ("filters-edge",                    writable);
  SET_SENSITIVE ("filters-hue-saturation",          writable && !gray);
  SET_SENSITIVE ("filters-long-shadow",             writable && alpha);
  SET_SENSITIVE ("filters-mono-mixer",              writable && !gray);
  SET_SENSITIVE ("filters-noise-hsv",               writable && !gray);
  SET_SENSITIVE ("filters-red-eye-removal",         writable && !gray);
  SET_SENSITIVE ("filters-saturation",              writable && !gray);
  SET_SENSITIVE ("filters-semi-flatten",            writable && alpha);
  SET_SENSITIVE ("filters-sepia",                   writable && !gray);
  SET_SENSITIVE ("filters-threshold-alpha",         writable && alpha);

#undef SET_SENSITIVE

  {
    ClayProcedure *proc   = clay_filter_history_nth (group->clay, 0);
    const gchar   *reason = NULL;
    gint           i;

    if (proc &&
        clay_procedure_get_sensitive (proc, CLAY_OBJECT (image), &reason))
      {
        clay_action_group_set_action_sensitive (group, "filters-repeat", TRUE, NULL);
        clay_action_group_set_action_sensitive (group, "filters-reshow", TRUE, NULL);
      }
    else
      {
        clay_action_group_set_action_sensitive (group, "filters-repeat", FALSE, reason);
        clay_action_group_set_action_sensitive (group, "filters-reshow", FALSE, reason);
     }

    for (i = 0; i < clay_filter_history_length (group->clay); i++)
      {
        gchar    *name = g_strdup_printf ("filters-recent-%02d", i + 1);
        gboolean  sensitive;

        proc = clay_filter_history_nth (group->clay, i);

        reason = NULL;
        sensitive = clay_procedure_get_sensitive (proc, CLAY_OBJECT (image),
                                                  &reason);

        clay_action_group_set_action_sensitive (group, name, sensitive, reason);

        g_free (name);
      }
  }
}

gboolean
filters_actions_gegl_op_blocklisted (const gchar *operation_name)
{
  for (gint i = 0; i < G_N_ELEMENTS (filters_actions); i++)
    {
      const ClayStringActionEntry *action_entry = &filters_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  for (gint i = 0; i < G_N_ELEMENTS (filters_settings_actions); i++)
    {
      const ClayStringActionEntry *action_entry = &filters_settings_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  for (gint i = 0; i < G_N_ELEMENTS (filters_interactive_actions); i++)
    {
      const ClayStringActionEntry *action_entry = &filters_interactive_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  return FALSE;
}

static void
filters_actions_set_tooltips (ClayActionGroup             *group,
                              const ClayStringActionEntry *entries,
                              gint                         n_entries)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      const ClayStringActionEntry *entry = entries + i;
      const gchar                 *description;

      description = gegl_operation_get_key (entry->value, "description");

      if (description)
        clay_action_group_set_action_tooltip (group, entry->name,
                                              description);
    }
}

static ClayActionGroup *
filters_actions_get_plug_in_group (ClayActionGroup *group)
{
  GList *list;

  for (list = clay_ui_managers_from_name ("<Image>");
       list;
       list = g_list_next (list))
    {
      ClayUIManager *manager = list->data;

      /* if this is our UI manager */
      if (clay_ui_manager_get_action_group (manager, "filters") == group)
        return clay_ui_manager_get_action_group (manager, "plug-in");
    }

  /* this happens during initial UI manager construction */
  return NULL;
}

static void
filters_actions_history_changed (Clay            *clay,
                                 ClayActionGroup *group)
{
  ClayProcedure   *proc;
  ClayActionGroup *plug_in_group;
  gint             i;

  plug_in_group = filters_actions_get_plug_in_group (group);

  proc = clay_filter_history_nth (clay, 0);

  if (proc)
    {
      ClayAction  *actual_action = NULL;
      const gchar *label;
      gchar       *repeat;
      gchar       *reshow;
      const gchar *reason    = NULL;
      gboolean     sensitive = FALSE;

      label = clay_procedure_get_label (proc);

      repeat = g_strdup_printf (_("Re_peat \"%s\""),  label);
      reshow = g_strdup_printf (_("R_e-Show \"%s\""), label);

      clay_action_group_set_action_label (group, "filters-repeat", repeat);
      clay_action_group_set_action_label (group, "filters-reshow", reshow);

      g_free (repeat);
      g_free (reshow);

      if (g_str_has_prefix (clay_object_get_name (proc), "filters-"))
        {
          actual_action =
            clay_action_group_get_action (group,
                                          clay_object_get_name (proc));
        }
      else if (plug_in_group)
        {
          /*  copy the sensitivity of the plug-in procedure's actual
           *  action instead of calling filters_actions_update()
           *  because doing the latter would set the sensitivity of
           *  this image's action on all images' actions. See bug
           *  #517683.
           */
          actual_action =
            clay_action_group_get_action (plug_in_group,
                                          clay_object_get_name (proc));
        }

      if (actual_action)
        sensitive = clay_action_get_sensitive (actual_action, &reason);

      clay_action_group_set_action_sensitive (group, "filters-repeat",
                                              sensitive, reason);
      clay_action_group_set_action_sensitive (group, "filters-reshow",
                                              sensitive, reason);
   }
  else
    {
      clay_action_group_set_action_label (group, "filters-repeat",
                                          _("Repeat Last"));
      clay_action_group_set_action_label (group, "filters-reshow",
                                          _("Re-Show Last"));

      clay_action_group_set_action_sensitive (group, "filters-repeat",
                                              FALSE, _("No last used filters"));
      clay_action_group_set_action_sensitive (group, "filters-reshow",
                                              FALSE, _("No last used filters"));
    }

  for (i = 0; i < clay_filter_history_length (clay); i++)
    {
      ClayAction  *action;
      ClayAction  *actual_action = NULL;
      const gchar *label;
      gchar       *name;
      gboolean     sensitive = FALSE;

      name = g_strdup_printf ("filters-recent-%02d", i + 1);
      action = clay_action_group_get_action (group, name);
      g_free (name);

      proc = clay_filter_history_nth (clay, i);

      label = clay_procedure_get_menu_label (proc);

      if (g_str_has_prefix (clay_object_get_name (proc), "filters-"))
        {
          actual_action =
            clay_action_group_get_action (group,
                                          clay_object_get_name (proc));
        }
      else if (plug_in_group)
        {
          /*  see comment above  */
          actual_action =
            clay_action_group_get_action (plug_in_group,
                                          clay_object_get_name (proc));
        }

      if (actual_action)
        sensitive = clay_action_get_sensitive (actual_action, NULL);

      g_object_set (action,
                    "sensitive", sensitive,
                    "procedure", proc,
                    "label",     label,
                    "icon-name", clay_viewable_get_icon_name (CLAY_VIEWABLE (proc)),
                    "tooltip",   clay_procedure_get_blurb (proc),
                    /* It is very important that "visible" is set at the
                     * end, because the docs says that:
                     *
                     * > "notify" signals are queued and only emitted (in reverse order) after all properties have been set.
                     *
                     * If "visible" is set before "label" in particular,
                     * we end up in the inconsistent situation where the
                     * "visible" callbacks have not been run yet, so
                     * menus don't have the corresponding item whereas
                     * the action already shows as visible. In
                     * particular, g_menu_model_items_changed() may
                     * crash on an empty item list in CLAY_GTK_MENUBAR
                     * codepath.
                     */
                    "visible",   TRUE,
                    NULL);
    }

  for (; i < clay_filter_history_size (clay); i++)
    {
      ClayAction *action;
      gchar      *name = g_strdup_printf ("filters-recent-%02d", i + 1);

      action = clay_action_group_get_action (group, name);
      g_free (name);

      g_object_set (action,
                    "visible",   FALSE,
                    "procedure", NULL,
                    NULL);
    }
}

static gboolean
filters_is_non_interactive (const gchar *action_name)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (filters_actions); i++)
    if (g_strcmp0 (filters_actions[i].name, action_name) == 0)
      return TRUE;

  for (i = 0; i < G_N_ELEMENTS (filters_settings_actions); i++)
    if (g_strcmp0 (filters_settings_actions[i].name, action_name) == 0)
      return TRUE;

  return FALSE;
}
