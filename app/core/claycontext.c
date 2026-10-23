/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontext.c
 * Copyright (C) 1999-2010 Michael Natterer
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "gegl/clay-babl.h"

#include "clay.h"
#include "clay-memsize.h"
#include "claybrush.h"
#include "claybuffer.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "claydatafactory.h"
#include "claydisplay.h"
#include "claydynamics.h"
#include "clayimagefile.h"
#include "claygradient.h"
#include "clayimage.h"
#include "claylineart.h"
#include "claymybrush.h"
#include "claypaintinfo.h"
#include "claypalette.h"
#include "claypattern.h"
#include "claytemplate.h"
#include "claytoolinfo.h"
#include "claytoolpreset.h"

#include "text/clayfont.h"

#include "clay-intl.h"


typedef void (* ClayContextCopyPropFunc) (ClayContext *src,
                                          ClayContext *dest);

#define context_find_defined(context, prop)                              \
  while (!(((context)->defined_props) & (1 << (prop))) && (context)->parent) \
    (context) = (context)->parent

#define COPY_NAME(src, dest, member)            \
  g_free (dest->member);                        \
  dest->member = g_strdup (src->member)


/*  local function prototypes  */

static void    clay_context_config_iface_init (ClayConfigInterface   *iface);

static void       clay_context_constructed    (GObject               *object);
static void       clay_context_dispose        (GObject               *object);
static void       clay_context_finalize       (GObject               *object);
static void       clay_context_set_property   (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void       clay_context_get_property   (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);
static gint64     clay_context_get_memsize    (ClayObject            *object,
                                               gint64                *gui_size);

static gboolean   clay_context_serialize            (ClayConfig       *config,
                                                     ClayConfigWriter *writer,
                                                     gpointer          data);
static gboolean   clay_context_deserialize          (ClayConfig       *config,
                                                     GScanner         *scanner,
                                                     gint              nest_level,
                                                     gpointer          data);
static gboolean   clay_context_serialize_property   (ClayConfig       *config,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec,
                                                     ClayConfigWriter *writer);
static gboolean   clay_context_deserialize_property (ClayConfig       *config,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec,
                                                     GScanner         *scanner,
                                                     GTokenType       *expected);
static ClayConfig * clay_context_duplicate          (ClayConfig       *config);
static gboolean     clay_context_copy               (ClayConfig       *src,
                                                     ClayConfig       *dest,
                                                     GParamFlags       flags);

/*  image  */
static void clay_context_image_disconnect    (ClayImage        *image,
                                              ClayContext      *context);
static void clay_context_image_removed       (ClayContainer    *container,
                                              ClayImage        *image,
                                              ClayContext      *context);
static void clay_context_real_set_image      (ClayContext      *context,
                                              ClayImage        *image);

/*  display  */
static void clay_context_display_removed     (ClayContainer    *container,
                                              ClayDisplay      *display,
                                              ClayContext      *context);
static void clay_context_real_set_display    (ClayContext      *context,
                                              ClayDisplay      *display);

/*  tool  */
static void clay_context_tool_dirty          (ClayToolInfo     *tool_info,
                                              ClayContext      *context);
static void clay_context_tool_removed        (ClayContainer    *container,
                                              ClayToolInfo     *tool_info,
                                              ClayContext      *context);
static void clay_context_tool_list_thaw      (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_tool       (ClayContext      *context,
                                              ClayToolInfo     *tool_info);

/*  paint info  */
static void clay_context_paint_info_dirty    (ClayPaintInfo    *paint_info,
                                              ClayContext      *context);
static void clay_context_paint_info_removed  (ClayContainer    *container,
                                              ClayPaintInfo    *paint_info,
                                              ClayContext      *context);
static void clay_context_paint_info_list_thaw(ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_paint_info (ClayContext      *context,
                                              ClayPaintInfo    *paint_info);

/*  foreground  */
static void clay_context_real_set_foreground (ClayContext      *context,
                                              GeglColor        *color);

/*  background  */
static void clay_context_real_set_background (ClayContext      *context,
                                              GeglColor        *color);

/*  opacity  */
static void clay_context_real_set_opacity    (ClayContext      *context,
                                              gdouble           opacity);

/*  paint mode  */
static void clay_context_real_set_paint_mode (ClayContext      *context,
                                              ClayLayerMode     paint_mode);

/*  brush  */
static void clay_context_brush_dirty         (ClayBrush        *brush,
                                              ClayContext      *context);
static void clay_context_brush_removed       (ClayContainer    *brush_list,
                                              ClayBrush        *brush,
                                              ClayContext      *context);
static void clay_context_brush_list_thaw     (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_brush      (ClayContext      *context,
                                              ClayBrush        *brush);

/*  dynamics  */

static void clay_context_dynamics_dirty      (ClayDynamics     *dynamics,
                                              ClayContext      *context);
static void clay_context_dynamics_removed    (ClayContainer    *container,
                                              ClayDynamics     *dynamics,
                                              ClayContext      *context);
static void clay_context_dynamics_list_thaw  (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_dynamics   (ClayContext      *context,
                                              ClayDynamics     *dynamics);

/*  mybrush  */
static void clay_context_mybrush_dirty       (ClayMybrush      *brush,
                                              ClayContext      *context);
static void clay_context_mybrush_removed     (ClayContainer    *brush_list,
                                              ClayMybrush      *brush,
                                              ClayContext      *context);
static void clay_context_mybrush_list_thaw   (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_mybrush    (ClayContext      *context,
                                              ClayMybrush      *brush);

/*  pattern  */
static void clay_context_pattern_dirty       (ClayPattern      *pattern,
                                              ClayContext      *context);
static void clay_context_pattern_removed     (ClayContainer    *container,
                                              ClayPattern      *pattern,
                                              ClayContext      *context);
static void clay_context_pattern_list_thaw   (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_pattern    (ClayContext      *context,
                                              ClayPattern      *pattern);

/*  gradient  */
static void clay_context_gradient_dirty      (ClayGradient     *gradient,
                                              ClayContext      *context);
static void clay_context_gradient_removed    (ClayContainer    *container,
                                              ClayGradient     *gradient,
                                              ClayContext      *context);
static void clay_context_gradient_list_thaw  (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_gradient   (ClayContext      *context,
                                              ClayGradient     *gradient);

/*  palette  */
static void clay_context_palette_dirty       (ClayPalette      *palette,
                                              ClayContext      *context);
static void clay_context_palette_removed     (ClayContainer    *container,
                                              ClayPalette      *palette,
                                              ClayContext      *context);
static void clay_context_palette_list_thaw   (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_palette    (ClayContext      *context,
                                              ClayPalette      *palette);

/*  font  */
static void clay_context_font_dirty          (ClayFont         *font,
                                              ClayContext      *context);
static void clay_context_font_removed        (ClayContainer    *container,
                                              ClayFont         *font,
                                              ClayContext      *context);
static void clay_context_font_list_thaw      (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_font       (ClayContext      *context,
                                              ClayFont         *font);

/*  tool preset  */
static void clay_context_tool_preset_dirty     (ClayToolPreset   *tool_preset,
                                                ClayContext      *context);
static void clay_context_tool_preset_removed   (ClayContainer    *container,
                                                ClayToolPreset   *tool_preset,
                                                ClayContext      *context);
static void clay_context_tool_preset_list_thaw (ClayContainer    *container,
                                                ClayContext      *context);
static void clay_context_real_set_tool_preset  (ClayContext      *context,
                                                ClayToolPreset   *tool_preset);

/*  buffer  */
static void clay_context_buffer_dirty        (ClayBuffer       *buffer,
                                              ClayContext      *context);
static void clay_context_buffer_removed      (ClayContainer    *container,
                                              ClayBuffer       *buffer,
                                              ClayContext      *context);
static void clay_context_buffer_list_thaw    (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_buffer     (ClayContext      *context,
                                              ClayBuffer       *buffer);

/*  imagefile  */
static void clay_context_imagefile_dirty     (ClayImagefile    *imagefile,
                                              ClayContext      *context);
static void clay_context_imagefile_removed   (ClayContainer    *container,
                                              ClayImagefile    *imagefile,
                                              ClayContext      *context);
static void clay_context_imagefile_list_thaw (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_imagefile  (ClayContext      *context,
                                              ClayImagefile    *imagefile);

/*  template  */
static void clay_context_template_dirty      (ClayTemplate     *template,
                                              ClayContext      *context);
static void clay_context_template_removed    (ClayContainer    *container,
                                              ClayTemplate     *template,
                                              ClayContext      *context);
static void clay_context_template_list_thaw  (ClayContainer    *container,
                                              ClayContext      *context);
static void clay_context_real_set_template   (ClayContext      *context,
                                              ClayTemplate     *template);


/*  line art  */
static gboolean clay_context_free_line_art   (ClayContext      *context);


/*  utilities  */
static gpointer clay_context_find_object     (ClayContext      *context,
                                              ClayContainer    *container,
                                              const gchar      *object_name,
                                              gpointer          standard_object);


/*  properties & signals  */

enum
{
  CLAY_CONTEXT_PROP_0,
  CLAY_CONTEXT_PROP_CLAY

  /*  remaining values are in core-enums.h  (ClayContextPropType)  */
};

enum
{
  DUMMY_0,
  DUMMY_1,
  IMAGE_CHANGED,
  DISPLAY_CHANGED,
  TOOL_CHANGED,
  PAINT_INFO_CHANGED,
  FOREGROUND_CHANGED,
  BACKGROUND_CHANGED,
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  BRUSH_CHANGED,
  DYNAMICS_CHANGED,
  MYBRUSH_CHANGED,
  PATTERN_CHANGED,
  GRADIENT_CHANGED,
  PALETTE_CHANGED,
  FONT_CHANGED,
  TOOL_PRESET_CHANGED,
  BUFFER_CHANGED,
  IMAGEFILE_CHANGED,
  TEMPLATE_CHANGED,
  PROP_NAME_CHANGED,
  LAST_SIGNAL
};

static const gchar * const clay_context_prop_names[] =
{
  NULL, /* PROP_0 */
  "clay",
  "image",
  "display",
  "tool",
  "paint-info",
  "foreground",
  "background",
  "opacity",
  "paint-mode",
  "brush",
  "dynamics",
  "mybrush",
  "pattern",
  "gradient",
  "palette",
  "font",
  "tool-preset",
  "buffer",
  "imagefile",
  "template"
};

static GType clay_context_prop_types[] =
{
  G_TYPE_NONE, /* PROP_0    */
  G_TYPE_NONE, /* PROP_CLAY */
  0,
  G_TYPE_NONE,
  0,
  0,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


G_DEFINE_TYPE_WITH_CODE (ClayContext, clay_context, CLAY_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_context_config_iface_init))

#define parent_class clay_context_parent_class

static ClayConfigInterface *parent_config_iface = NULL;

static guint clay_context_signals[LAST_SIGNAL] = { 0 };


static void
clay_context_class_init (ClayContextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  GeglColor       *black             = gegl_color_new ("black");
  GeglColor       *white             = gegl_color_new ("white");

  clay_context_signals[IMAGE_CHANGED] =
    g_signal_new ("image-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, image_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_IMAGE);

  clay_context_signals[DISPLAY_CHANGED] =
    g_signal_new ("display-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, display_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DISPLAY);

  clay_context_signals[TOOL_CHANGED] =
    g_signal_new ("tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_TOOL_INFO);

  clay_context_signals[PAINT_INFO_CHANGED] =
    g_signal_new ("paint-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, paint_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PAINT_INFO);

  clay_context_signals[FOREGROUND_CHANGED] =
    g_signal_new ("foreground-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, foreground_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  clay_context_signals[BACKGROUND_CHANGED] =
    g_signal_new ("background-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, background_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  clay_context_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  clay_context_signals[PAINT_MODE_CHANGED] =
    g_signal_new ("paint-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, paint_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_LAYER_MODE);

  clay_context_signals[BRUSH_CHANGED] =
    g_signal_new ("brush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, brush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_BRUSH);

  clay_context_signals[DYNAMICS_CHANGED] =
    g_signal_new ("dynamics-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, dynamics_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DYNAMICS);

  clay_context_signals[MYBRUSH_CHANGED] =
    g_signal_new ("mybrush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, mybrush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_MYBRUSH);

  clay_context_signals[PATTERN_CHANGED] =
    g_signal_new ("pattern-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, pattern_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PATTERN);

  clay_context_signals[GRADIENT_CHANGED] =
    g_signal_new ("gradient-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, gradient_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_GRADIENT);

  clay_context_signals[PALETTE_CHANGED] =
    g_signal_new ("palette-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, palette_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PALETTE);

  clay_context_signals[FONT_CHANGED] =
    g_signal_new ("font-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, font_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_FONT);

  clay_context_signals[TOOL_PRESET_CHANGED] =
    g_signal_new ("tool-preset-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, tool_preset_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_TOOL_PRESET);

  clay_context_signals[BUFFER_CHANGED] =
    g_signal_new ("buffer-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, buffer_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_BUFFER);

  clay_context_signals[IMAGEFILE_CHANGED] =
    g_signal_new ("imagefile-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, imagefile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_IMAGEFILE);

  clay_context_signals[TEMPLATE_CHANGED] =
    g_signal_new ("template-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, template_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_TEMPLATE);

  clay_context_signals[PROP_NAME_CHANGED] =
    g_signal_new ("prop-name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayContextClass, prop_name_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  object_class->constructed      = clay_context_constructed;
  object_class->set_property     = clay_context_set_property;
  object_class->get_property     = clay_context_get_property;
  object_class->dispose          = clay_context_dispose;
  object_class->finalize         = clay_context_finalize;

  clay_object_class->get_memsize = clay_context_get_memsize;

  klass->image_changed           = NULL;
  klass->display_changed         = NULL;
  klass->tool_changed            = NULL;
  klass->paint_info_changed      = NULL;
  klass->foreground_changed      = NULL;
  klass->background_changed      = NULL;
  klass->opacity_changed         = NULL;
  klass->paint_mode_changed      = NULL;
  klass->brush_changed           = NULL;
  klass->dynamics_changed        = NULL;
  klass->mybrush_changed         = NULL;
  klass->pattern_changed         = NULL;
  klass->gradient_changed        = NULL;
  klass->palette_changed         = NULL;
  klass->font_changed            = NULL;
  klass->tool_preset_changed     = NULL;
  klass->buffer_changed          = NULL;
  klass->imagefile_changed       = NULL;
  klass->template_changed        = NULL;
  klass->prop_name_changed       = NULL;

  clay_context_prop_types[CLAY_CONTEXT_PROP_IMAGE]       = CLAY_TYPE_IMAGE;
  clay_context_prop_types[CLAY_CONTEXT_PROP_TOOL]        = CLAY_TYPE_TOOL_INFO;
  clay_context_prop_types[CLAY_CONTEXT_PROP_PAINT_INFO]  = CLAY_TYPE_PAINT_INFO;
  clay_context_prop_types[CLAY_CONTEXT_PROP_BRUSH]       = CLAY_TYPE_BRUSH;
  clay_context_prop_types[CLAY_CONTEXT_PROP_DYNAMICS]    = CLAY_TYPE_DYNAMICS;
  clay_context_prop_types[CLAY_CONTEXT_PROP_MYBRUSH]     = CLAY_TYPE_MYBRUSH;
  clay_context_prop_types[CLAY_CONTEXT_PROP_PATTERN]     = CLAY_TYPE_PATTERN;
  clay_context_prop_types[CLAY_CONTEXT_PROP_GRADIENT]    = CLAY_TYPE_GRADIENT;
  clay_context_prop_types[CLAY_CONTEXT_PROP_PALETTE]     = CLAY_TYPE_PALETTE;
  clay_context_prop_types[CLAY_CONTEXT_PROP_FONT]        = CLAY_TYPE_FONT;
  clay_context_prop_types[CLAY_CONTEXT_PROP_TOOL_PRESET] = CLAY_TYPE_TOOL_PRESET;
  clay_context_prop_types[CLAY_CONTEXT_PROP_BUFFER]      = CLAY_TYPE_BUFFER;
  clay_context_prop_types[CLAY_CONTEXT_PROP_IMAGEFILE]   = CLAY_TYPE_IMAGEFILE;
  clay_context_prop_types[CLAY_CONTEXT_PROP_TEMPLATE]    = CLAY_TYPE_TEMPLATE;

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_IMAGE,
                                   g_param_spec_object (clay_context_prop_names[CLAY_CONTEXT_PROP_IMAGE],
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_DISPLAY,
                                   g_param_spec_object (clay_context_prop_names[CLAY_CONTEXT_PROP_DISPLAY],
                                                        NULL, NULL,
                                                        CLAY_TYPE_DISPLAY,
                                                        CLAY_PARAM_READWRITE));

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_TOOL,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_TOOL],
                           NULL, NULL,
                           CLAY_TYPE_TOOL_INFO,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_PAINT_INFO,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_PAINT_INFO],
                           NULL, NULL,
                           CLAY_TYPE_PAINT_INFO,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, CLAY_CONTEXT_PROP_FOREGROUND,
                          clay_context_prop_names[CLAY_CONTEXT_PROP_FOREGROUND],
                          _("Foreground"),
                          _("Foreground color"),
                          FALSE, black,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, CLAY_CONTEXT_PROP_BACKGROUND,
                          clay_context_prop_names[CLAY_CONTEXT_PROP_BACKGROUND],
                          _("Background"),
                          _("Background color"),
                          FALSE, white,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, CLAY_CONTEXT_PROP_OPACITY,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_OPACITY],
                           _("Opacity"),
                           _("Opacity"),
                           CLAY_OPACITY_TRANSPARENT,
                           CLAY_OPACITY_OPAQUE,
                           CLAY_OPACITY_OPAQUE,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, CLAY_CONTEXT_PROP_PAINT_MODE,
                         clay_context_prop_names[CLAY_CONTEXT_PROP_PAINT_MODE],
                         _("Paint Mode"),
                         _("Paint Mode"),
                         CLAY_TYPE_LAYER_MODE,
                         CLAY_LAYER_MODE_NORMAL,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_BRUSH,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_BRUSH],
                           _("Brush"),
                           _("Brush"),
                           CLAY_TYPE_BRUSH,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_DYNAMICS,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_DYNAMICS],
                           _("Dynamics"),
                           _("Paint dynamics"),
                           CLAY_TYPE_DYNAMICS,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_MYBRUSH,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_MYBRUSH],
                           _("MyPaint Brush"),
                           _("MyPaint Brush"),
                           CLAY_TYPE_MYBRUSH,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_PATTERN,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_PATTERN],
                           _("Pattern"),
                           _("Pattern"),
                           CLAY_TYPE_PATTERN,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_GRADIENT,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_GRADIENT],
                           _("Gradient"),
                           _("Gradient"),
                           CLAY_TYPE_GRADIENT,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_PALETTE,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_PALETTE],
                           _("Palette"),
                           _("Palette"),
                           CLAY_TYPE_PALETTE,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_FONT,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_FONT],
                           _("Font"),
                           _("Font"),
                           CLAY_TYPE_FONT,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, CLAY_CONTEXT_PROP_TOOL_PRESET,
                           clay_context_prop_names[CLAY_CONTEXT_PROP_TOOL_PRESET],
                           _("Tool Preset"),
                           _("Tool Preset"),
                           CLAY_TYPE_TOOL_PRESET,
                           CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_BUFFER,
                                   g_param_spec_object (clay_context_prop_names[CLAY_CONTEXT_PROP_BUFFER],
                                                        NULL, NULL,
                                                        CLAY_TYPE_BUFFER,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_IMAGEFILE,
                                   g_param_spec_object (clay_context_prop_names[CLAY_CONTEXT_PROP_IMAGEFILE],
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGEFILE,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, CLAY_CONTEXT_PROP_TEMPLATE,
                                   g_param_spec_object (clay_context_prop_names[CLAY_CONTEXT_PROP_TEMPLATE],
                                                        NULL, NULL,
                                                        CLAY_TYPE_TEMPLATE,
                                                        CLAY_PARAM_READWRITE));

  g_object_unref (black);
  g_object_unref (white);
}

static void
clay_context_init (ClayContext *context)
{
  context->clay            = NULL;

  context->parent          = NULL;

  context->defined_props   = CLAY_CONTEXT_PROP_MASK_ALL;
  context->serialize_props = CLAY_CONTEXT_PROP_MASK_ALL;

  context->image           = NULL;
  context->display         = NULL;

  context->tool_info       = NULL;
  context->tool_name       = NULL;

  context->paint_info      = NULL;
  context->paint_name      = NULL;

  context->brush           = NULL;
  context->brush_name      = NULL;

  context->dynamics        = NULL;
  context->dynamics_name   = NULL;

  context->mybrush         = NULL;
  context->mybrush_name    = NULL;

  context->pattern         = NULL;
  context->pattern_name    = NULL;

  context->gradient        = NULL;
  context->gradient_name   = NULL;

  context->palette         = NULL;
  context->palette_name    = NULL;

  context->font            = NULL;
  context->font_name       = NULL;

  context->tool_preset      = NULL;
  context->tool_preset_name = NULL;

  context->buffer          = NULL;
  context->buffer_name     = NULL;

  context->imagefile       = NULL;
  context->imagefile_name  = NULL;

  context->template        = NULL;
  context->template_name   = NULL;

  context->line_art            = NULL;
  context->line_art_timeout_id = 0;

  context->foreground      = gegl_color_new ("black");
  context->background      = gegl_color_new ("white");
}

static void
clay_context_config_iface_init (ClayConfigInterface *iface)
{
  parent_config_iface = g_type_interface_peek_parent (iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (CLAY_TYPE_CONFIG);

  iface->serialize            = clay_context_serialize;
  iface->deserialize          = clay_context_deserialize;
  iface->serialize_property   = clay_context_serialize_property;
  iface->deserialize_property = clay_context_deserialize_property;
  iface->duplicate            = clay_context_duplicate;
  iface->copy                 = clay_context_copy;
}

static void
clay_context_constructed (GObject *object)
{
  Clay          *clay;
  ClayContainer *container;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay = CLAY_CONTEXT (object)->clay;

  clay_assert (CLAY_IS_CLAY (clay));

  clay->context_list = g_list_prepend (clay->context_list, object);

  g_signal_connect_object (clay->images, "remove",
                           G_CALLBACK (clay_context_image_removed),
                           object, 0);
  g_signal_connect_object (clay->displays, "remove",
                           G_CALLBACK (clay_context_display_removed),
                           object, 0);

  g_signal_connect_object (clay->tool_info_list, "remove",
                           G_CALLBACK (clay_context_tool_removed),
                           object, 0);
  g_signal_connect_object (clay->tool_info_list, "thaw",
                           G_CALLBACK (clay_context_tool_list_thaw),
                           object, 0);

  g_signal_connect_object (clay->paint_info_list, "remove",
                           G_CALLBACK (clay_context_paint_info_removed),
                           object, 0);
  g_signal_connect_object (clay->paint_info_list, "thaw",
                           G_CALLBACK (clay_context_paint_info_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->brush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_brush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_brush_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->dynamics_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_dynamics_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_dynamics_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->mybrush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_mybrush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_mybrush_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->pattern_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_pattern_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_pattern_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->gradient_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_gradient_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_gradient_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->palette_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_palette_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_palette_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->font_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_font_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_font_list_thaw),
                           object, 0);

  container = clay_data_factory_get_container (clay->tool_preset_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_context_tool_preset_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_context_tool_preset_list_thaw),
                           object, 0);

  g_signal_connect_object (clay->named_buffers, "remove",
                           G_CALLBACK (clay_context_buffer_removed),
                           object, 0);
  g_signal_connect_object (clay->named_buffers, "thaw",
                           G_CALLBACK (clay_context_buffer_list_thaw),
                           object, 0);

  g_signal_connect_object (clay->documents, "remove",
                           G_CALLBACK (clay_context_imagefile_removed),
                           object, 0);
  g_signal_connect_object (clay->documents, "thaw",
                           G_CALLBACK (clay_context_imagefile_list_thaw),
                           object, 0);

  g_signal_connect_object (clay->templates, "remove",
                           G_CALLBACK (clay_context_template_removed),
                           object, 0);
  g_signal_connect_object (clay->templates, "thaw",
                           G_CALLBACK (clay_context_template_list_thaw),
                           object, 0);

  clay_context_set_paint_info (CLAY_CONTEXT (object),
                               clay_paint_info_get_standard (clay));
}

static void
clay_context_dispose (GObject *object)
{
  ClayContext *context = CLAY_CONTEXT (object);

  clay_context_set_parent (context, NULL);

  if (context->clay)
    {
      context->clay->context_list = g_list_remove (context->clay->context_list,
                                                   context);
      context->clay = NULL;
    }

  g_clear_object (&context->tool_info);
  g_clear_object (&context->paint_info);
  g_clear_object (&context->brush);
  g_clear_object (&context->dynamics);
  g_clear_object (&context->mybrush);
  g_clear_object (&context->pattern);
  g_clear_object (&context->gradient);
  g_clear_object (&context->palette);
  g_clear_object (&context->font);
  g_clear_object (&context->tool_preset);
  g_clear_object (&context->buffer);
  g_clear_object (&context->imagefile);
  g_clear_object (&context->template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_context_finalize (GObject *object)
{
  ClayContext *context = CLAY_CONTEXT (object);

  context->parent  = NULL;
  context->image   = NULL;
  context->display = NULL;

  g_clear_pointer (&context->tool_name,        g_free);
  g_clear_pointer (&context->paint_name,       g_free);
  g_clear_pointer (&context->brush_name,       g_free);
  g_clear_pointer (&context->dynamics_name,    g_free);
  g_clear_pointer (&context->mybrush_name,     g_free);
  g_clear_pointer (&context->pattern_name,     g_free);
  g_clear_pointer (&context->gradient_name,    g_free);
  g_clear_pointer (&context->palette_name,     g_free);
  g_clear_pointer (&context->font_name,        g_free);
  g_clear_pointer (&context->tool_preset_name, g_free);
  g_clear_pointer (&context->buffer_name,      g_free);
  g_clear_pointer (&context->imagefile_name,   g_free);
  g_clear_pointer (&context->template_name,    g_free);

  g_clear_object (&context->line_art);
  g_clear_object (&context->foreground);
  g_clear_object (&context->background);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_context_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClayContext *context = CLAY_CONTEXT (object);

  switch (property_id)
    {
    case CLAY_CONTEXT_PROP_CLAY:
      context->clay = g_value_get_object (value);
      break;
    case CLAY_CONTEXT_PROP_IMAGE:
      clay_context_set_image (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_DISPLAY:
      clay_context_set_display (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_TOOL:
      clay_context_set_tool (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_PAINT_INFO:
      clay_context_set_paint_info (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_FOREGROUND:
      clay_context_set_foreground (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_BACKGROUND:
      clay_context_set_background (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_OPACITY:
      clay_context_set_opacity (context, g_value_get_double (value));
      break;
    case CLAY_CONTEXT_PROP_PAINT_MODE:
      clay_context_set_paint_mode (context, g_value_get_enum (value));
      break;
    case CLAY_CONTEXT_PROP_BRUSH:
      clay_context_set_brush (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_DYNAMICS:
      clay_context_set_dynamics (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_MYBRUSH:
      clay_context_set_mybrush (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_PATTERN:
      clay_context_set_pattern (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_GRADIENT:
      clay_context_set_gradient (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_PALETTE:
      clay_context_set_palette (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_FONT:
      clay_context_set_font (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_TOOL_PRESET:
      clay_context_set_tool_preset (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_BUFFER:
      clay_context_set_buffer (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_IMAGEFILE:
      clay_context_set_imagefile (context, g_value_get_object (value));
      break;
    case CLAY_CONTEXT_PROP_TEMPLATE:
      clay_context_set_template (context, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_context_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClayContext *context = CLAY_CONTEXT (object);

  switch (property_id)
    {
    case CLAY_CONTEXT_PROP_CLAY:
      g_value_set_object (value, context->clay);
      break;
    case CLAY_CONTEXT_PROP_IMAGE:
      g_value_set_object (value, clay_context_get_image (context));
      break;
    case CLAY_CONTEXT_PROP_DISPLAY:
      g_value_set_object (value, clay_context_get_display (context));
      break;
    case CLAY_CONTEXT_PROP_TOOL:
      g_value_set_object (value, clay_context_get_tool (context));
      break;
    case CLAY_CONTEXT_PROP_PAINT_INFO:
      g_value_set_object (value, clay_context_get_paint_info (context));
      break;
    case CLAY_CONTEXT_PROP_FOREGROUND:
      g_value_take_object (value, gegl_color_duplicate (clay_context_get_foreground (context)));
      break;
    case CLAY_CONTEXT_PROP_BACKGROUND:
      g_value_take_object (value, gegl_color_duplicate (clay_context_get_background (context)));
      break;
    case CLAY_CONTEXT_PROP_OPACITY:
      g_value_set_double (value, clay_context_get_opacity (context));
      break;
    case CLAY_CONTEXT_PROP_PAINT_MODE:
      g_value_set_enum (value, clay_context_get_paint_mode (context));
      break;
    case CLAY_CONTEXT_PROP_BRUSH:
      g_value_set_object (value, clay_context_get_brush (context));
      break;
    case CLAY_CONTEXT_PROP_DYNAMICS:
      g_value_set_object (value, clay_context_get_dynamics (context));
      break;
    case CLAY_CONTEXT_PROP_MYBRUSH:
      g_value_set_object (value, clay_context_get_mybrush (context));
      break;
    case CLAY_CONTEXT_PROP_PATTERN:
      g_value_set_object (value, clay_context_get_pattern (context));
      break;
    case CLAY_CONTEXT_PROP_GRADIENT:
      g_value_set_object (value, clay_context_get_gradient (context));
      break;
    case CLAY_CONTEXT_PROP_PALETTE:
      g_value_set_object (value, clay_context_get_palette (context));
      break;
    case CLAY_CONTEXT_PROP_FONT:
      g_value_set_object (value, clay_context_get_font (context));
      break;
    case CLAY_CONTEXT_PROP_TOOL_PRESET:
      g_value_set_object (value, clay_context_get_tool_preset (context));
      break;
    case CLAY_CONTEXT_PROP_BUFFER:
      g_value_set_object (value, clay_context_get_buffer (context));
      break;
    case CLAY_CONTEXT_PROP_IMAGEFILE:
      g_value_set_object (value, clay_context_get_imagefile (context));
      break;
    case CLAY_CONTEXT_PROP_TEMPLATE:
      g_value_set_object (value, clay_context_get_template (context));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_context_get_memsize (ClayObject *object,
                          gint64     *gui_size)
{
  ClayContext *context = CLAY_CONTEXT (object);
  gint64       memsize = 0;

  memsize += clay_string_get_memsize (context->tool_name);
  memsize += clay_string_get_memsize (context->paint_name);
  memsize += clay_string_get_memsize (context->brush_name);
  memsize += clay_string_get_memsize (context->dynamics_name);
  memsize += clay_string_get_memsize (context->mybrush_name);
  memsize += clay_string_get_memsize (context->pattern_name);
  memsize += clay_string_get_memsize (context->palette_name);
  memsize += clay_string_get_memsize (context->font_name);
  memsize += clay_string_get_memsize (context->tool_preset_name);
  memsize += clay_string_get_memsize (context->buffer_name);
  memsize += clay_string_get_memsize (context->imagefile_name);
  memsize += clay_string_get_memsize (context->template_name);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_context_serialize (ClayConfig       *config,
                        ClayConfigWriter *writer,
                        gpointer          data)
{
  return clay_config_serialize_changed_properties (config, writer);
}

static gboolean
clay_context_deserialize (ClayConfig *config,
                          GScanner   *scanner,
                          gint        nest_level,
                          gpointer    data)
{
  ClayContext   *context        = CLAY_CONTEXT (config);
  ClayLayerMode  old_paint_mode = context->paint_mode;
  gboolean       success;

  success = clay_config_deserialize_properties (config, scanner, nest_level);

  if (context->paint_mode != old_paint_mode)
    {
      if (context->paint_mode == CLAY_LAYER_MODE_OVERLAY_LEGACY)
        g_object_set (context,
                      "paint-mode", CLAY_LAYER_MODE_SOFTLIGHT_LEGACY,
                      NULL);
    }

  return success;
}

static gboolean
clay_context_serialize_property (ClayConfig       *config,
                                 guint             property_id,
                                 const GValue     *value,
                                 GParamSpec       *pspec,
                                 ClayConfigWriter *writer)
{
  ClayContext *context = CLAY_CONTEXT (config);
  ClayObject  *serialize_obj;

  /*  serialize nothing if the property is not in serialize_props  */
  if (! ((1 << property_id) & context->serialize_props))
    return TRUE;

  switch (property_id)
    {
    case CLAY_CONTEXT_PROP_TOOL:
    case CLAY_CONTEXT_PROP_PAINT_INFO:
    case CLAY_CONTEXT_PROP_BRUSH:
    case CLAY_CONTEXT_PROP_DYNAMICS:
    case CLAY_CONTEXT_PROP_MYBRUSH:
    case CLAY_CONTEXT_PROP_PATTERN:
    case CLAY_CONTEXT_PROP_GRADIENT:
    case CLAY_CONTEXT_PROP_PALETTE:
    case CLAY_CONTEXT_PROP_FONT:
    case CLAY_CONTEXT_PROP_TOOL_PRESET:
      serialize_obj = g_value_get_object (value);
      break;

    default:
      return FALSE;
    }

  clay_config_writer_open (writer, pspec->name);

  if (serialize_obj)
    clay_config_writer_string (writer, clay_object_get_name (serialize_obj));
  else
    clay_config_writer_print (writer, "NULL", 4);

  clay_config_writer_close (writer);

  return TRUE;
}

static gboolean
clay_context_deserialize_property (ClayConfig *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec,
                                   GScanner   *scanner,
                                   GTokenType *expected)
{
  ClayContext   *context = CLAY_CONTEXT (object);
  ClayContainer *container;
  gpointer       standard;
  gchar        **name_loc;
  gchar         *object_name;

  switch (property_id)
    {
    case CLAY_CONTEXT_PROP_TOOL:
      container = context->clay->tool_info_list;
      standard  = clay_tool_info_get_standard (context->clay);
      name_loc  = &context->tool_name;
      break;

    case CLAY_CONTEXT_PROP_PAINT_INFO:
      container = context->clay->paint_info_list;
      standard  = clay_paint_info_get_standard (context->clay);
      name_loc  = &context->paint_name;
      break;

    case CLAY_CONTEXT_PROP_BRUSH:
      container = clay_data_factory_get_container (context->clay->brush_factory);
      standard  = clay_brush_get_standard (context);
      name_loc  = &context->brush_name;
      break;

    case CLAY_CONTEXT_PROP_DYNAMICS:
      container = clay_data_factory_get_container (context->clay->dynamics_factory);
      standard  = clay_dynamics_get_standard (context);
      name_loc  = &context->dynamics_name;
      break;

    case CLAY_CONTEXT_PROP_MYBRUSH:
      container = clay_data_factory_get_container (context->clay->mybrush_factory);
      standard  = clay_mybrush_get_standard (context);
      name_loc  = &context->mybrush_name;
      break;

    case CLAY_CONTEXT_PROP_PATTERN:
      container = clay_data_factory_get_container (context->clay->pattern_factory);
      standard  = clay_pattern_get_standard (context);
      name_loc  = &context->pattern_name;
      break;

    case CLAY_CONTEXT_PROP_GRADIENT:
      container = clay_data_factory_get_container (context->clay->gradient_factory);
      standard  = clay_gradient_get_standard (context);
      name_loc  = &context->gradient_name;
      break;

    case CLAY_CONTEXT_PROP_PALETTE:
      container = clay_data_factory_get_container (context->clay->palette_factory);
      standard  = clay_palette_get_standard (context);
      name_loc  = &context->palette_name;
      break;

    case CLAY_CONTEXT_PROP_FONT:
      container = clay_data_factory_get_container (context->clay->font_factory);
      standard  = clay_font_get_standard ();
      name_loc  = &context->font_name;
      break;

    case CLAY_CONTEXT_PROP_TOOL_PRESET:
      container = clay_data_factory_get_container (context->clay->tool_preset_factory);
      standard  = NULL;
      name_loc  = &context->tool_preset_name;
      break;

    default:
      return FALSE;
    }

  if (clay_scanner_parse_identifier (scanner, "NULL"))
    {
      g_value_set_object (value, NULL);
    }
  else if (clay_scanner_parse_string (scanner, &object_name))
    {
      ClayObject *deserialize_obj;

      if (! object_name)
        object_name = g_strdup ("");

      deserialize_obj = clay_container_get_child_by_name (container,
                                                          object_name);

      if (! deserialize_obj)
        {
          g_value_set_object (value, standard);

          g_free (*name_loc);
          *name_loc = g_strdup (object_name);
        }
      else
        {
          g_value_set_object (value, deserialize_obj);
        }

      g_free (object_name);
    }
  else
    {
      *expected = G_TOKEN_STRING;
    }

  return TRUE;
}

static ClayConfig *
clay_context_duplicate (ClayConfig *config)
{
  ClayContext *context = CLAY_CONTEXT (config);
  ClayContext *new;

  new = CLAY_CONTEXT (parent_config_iface->duplicate (config));

  COPY_NAME (context, new, tool_name);
  COPY_NAME (context, new, paint_name);
  COPY_NAME (context, new, brush_name);
  COPY_NAME (context, new, dynamics_name);
  COPY_NAME (context, new, mybrush_name);
  COPY_NAME (context, new, pattern_name);
  COPY_NAME (context, new, gradient_name);
  COPY_NAME (context, new, palette_name);
  COPY_NAME (context, new, font_name);
  COPY_NAME (context, new, tool_preset_name);
  COPY_NAME (context, new, buffer_name);
  COPY_NAME (context, new, imagefile_name);
  COPY_NAME (context, new, template_name);

  return CLAY_CONFIG (new);
}

static gboolean
clay_context_copy (ClayConfig  *src,
                   ClayConfig  *dest,
                   GParamFlags  flags)
{
  ClayContext *src_context  = CLAY_CONTEXT (src);
  ClayContext *dest_context = CLAY_CONTEXT (dest);
  gboolean     success      = parent_config_iface->copy (src, dest, flags);

  COPY_NAME (src_context, dest_context, tool_name);
  COPY_NAME (src_context, dest_context, paint_name);
  COPY_NAME (src_context, dest_context, brush_name);
  COPY_NAME (src_context, dest_context, dynamics_name);
  COPY_NAME (src_context, dest_context, mybrush_name);
  COPY_NAME (src_context, dest_context, pattern_name);
  COPY_NAME (src_context, dest_context, gradient_name);
  COPY_NAME (src_context, dest_context, palette_name);
  COPY_NAME (src_context, dest_context, font_name);
  COPY_NAME (src_context, dest_context, tool_preset_name);
  COPY_NAME (src_context, dest_context, buffer_name);
  COPY_NAME (src_context, dest_context, imagefile_name);
  COPY_NAME (src_context, dest_context, template_name);

  return success;
}


/*****************************************************************************/
/*  public functions  ********************************************************/

ClayContext *
clay_context_new (Clay        *clay,
                  const gchar *name,
                  ClayContext *template)
{
  ClayContext *context;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (template == NULL || CLAY_IS_CONTEXT (template), NULL);

  context = g_object_new (CLAY_TYPE_CONTEXT,
                          "name", name,
                          "clay", clay,
                          NULL);

  if (template)
    {
      context->defined_props = template->defined_props;

      clay_context_copy_properties (template, context,
                                    CLAY_CONTEXT_PROP_MASK_ALL);
    }

  return context;
}

ClayContext *
clay_context_get_parent (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->parent;
}

static void
clay_context_parent_notify (ClayContext *parent,
                            GParamSpec  *pspec,
                            ClayContext *context)
{
  if (pspec->owner_type == CLAY_TYPE_CONTEXT)
    {
      ClayContextPropType prop = pspec->param_id;

      /*  copy from parent if the changed property is undefined;
       *  ignore properties that are not context properties, for
       *  example notifications on the context's "clay" property
       */
      if ((prop >= CLAY_CONTEXT_PROP_FIRST) &&
          (prop <= CLAY_CONTEXT_PROP_LAST)  &&
          ! ((1 << prop) & context->defined_props))
        {
          clay_context_copy_property (parent, context, prop);
        }
    }
}

void
clay_context_set_parent (ClayContext *context,
                         ClayContext *parent)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (parent == NULL || CLAY_IS_CONTEXT (parent));
  g_return_if_fail (parent == NULL || parent->parent != context);
  g_return_if_fail (context != parent);

  if (context->parent == parent)
    return;

  if (context->parent)
    {
      g_signal_handlers_disconnect_by_func (context->parent,
                                            clay_context_parent_notify,
                                            context);
    }

  g_set_weak_pointer (&context->parent, parent);

  if (parent)
    {
      /*  copy all undefined properties from the new parent  */
      clay_context_copy_properties (parent, context,
                                    ~context->defined_props &
                                    CLAY_CONTEXT_PROP_MASK_ALL);

      g_signal_connect_object (parent, "notify",
                               G_CALLBACK (clay_context_parent_notify),
                               context,
                               0);
    }
}


/*  define / undefinine context properties  */

void
clay_context_define_property (ClayContext         *context,
                              ClayContextPropType  prop,
                              gboolean             defined)
{
  ClayContextPropMask mask;

  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail ((prop >= CLAY_CONTEXT_PROP_FIRST) &&
                    (prop <= CLAY_CONTEXT_PROP_LAST));

  mask = (1 << prop);

  if (defined)
    {
      if (! (context->defined_props & mask))
        {
          context->defined_props |= mask;
        }
    }
  else
    {
      if (context->defined_props & mask)
        {
          context->defined_props &= ~mask;

          if (context->parent)
            clay_context_copy_property (context->parent, context, prop);
        }
    }
}

gboolean
clay_context_property_defined (ClayContext         *context,
                               ClayContextPropType  prop)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), FALSE);

  return (context->defined_props & (1 << prop)) ? TRUE : FALSE;
}

void
clay_context_define_properties (ClayContext         *context,
                                ClayContextPropMask  prop_mask,
                                gboolean             defined)
{
  ClayContextPropType prop;

  g_return_if_fail (CLAY_IS_CONTEXT (context));

  for (prop = CLAY_CONTEXT_PROP_FIRST; prop <= CLAY_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      clay_context_define_property (context, prop, defined);
}


/*  specify which context properties will be serialized  */

void
clay_context_set_serialize_properties (ClayContext         *context,
                                       ClayContextPropMask  props_mask)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  context->serialize_props = props_mask;
}

ClayContextPropMask
clay_context_get_serialize_properties (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), 0);

  return context->serialize_props;
}


/*  copying context properties  */

void
clay_context_copy_property (ClayContext         *src,
                            ClayContext         *dest,
                            ClayContextPropType  prop)
{
  g_return_if_fail (CLAY_IS_CONTEXT (src));
  g_return_if_fail (CLAY_IS_CONTEXT (dest));
  g_return_if_fail ((prop >= CLAY_CONTEXT_PROP_FIRST) &&
                    (prop <= CLAY_CONTEXT_PROP_LAST));

  switch (prop)
    {
    case CLAY_CONTEXT_PROP_IMAGE:
      clay_context_real_set_image (dest, src->image);
      break;

    case CLAY_CONTEXT_PROP_DISPLAY:
      clay_context_real_set_display (dest, src->display);
      break;

    case CLAY_CONTEXT_PROP_TOOL:
      clay_context_real_set_tool (dest, src->tool_info);
      COPY_NAME (src, dest, tool_name);
      break;

    case CLAY_CONTEXT_PROP_PAINT_INFO:
      clay_context_real_set_paint_info (dest, src->paint_info);
      COPY_NAME (src, dest, paint_name);
      break;

    case CLAY_CONTEXT_PROP_FOREGROUND:
      clay_context_real_set_foreground (dest, src->foreground);
      break;

    case CLAY_CONTEXT_PROP_BACKGROUND:
      clay_context_real_set_background (dest, src->background);
      break;

    case CLAY_CONTEXT_PROP_OPACITY:
      clay_context_real_set_opacity (dest, src->opacity);
      break;

    case CLAY_CONTEXT_PROP_PAINT_MODE:
      clay_context_real_set_paint_mode (dest, src->paint_mode);
      break;

    case CLAY_CONTEXT_PROP_BRUSH:
      clay_context_real_set_brush (dest, src->brush);
      COPY_NAME (src, dest, brush_name);
      break;

    case CLAY_CONTEXT_PROP_DYNAMICS:
      clay_context_real_set_dynamics (dest, src->dynamics);
      COPY_NAME (src, dest, dynamics_name);
      break;

    case CLAY_CONTEXT_PROP_MYBRUSH:
      clay_context_real_set_mybrush (dest, src->mybrush);
      COPY_NAME (src, dest, mybrush_name);
      break;

    case CLAY_CONTEXT_PROP_PATTERN:
      clay_context_real_set_pattern (dest, src->pattern);
      COPY_NAME (src, dest, pattern_name);
      break;

    case CLAY_CONTEXT_PROP_GRADIENT:
      clay_context_real_set_gradient (dest, src->gradient);
      COPY_NAME (src, dest, gradient_name);
      break;

    case CLAY_CONTEXT_PROP_PALETTE:
      clay_context_real_set_palette (dest, src->palette);
      COPY_NAME (src, dest, palette_name);
      break;

    case CLAY_CONTEXT_PROP_FONT:
      clay_context_real_set_font (dest, src->font);
      COPY_NAME (src, dest, font_name);
      break;

    case CLAY_CONTEXT_PROP_TOOL_PRESET:
      clay_context_real_set_tool_preset (dest, src->tool_preset);
      COPY_NAME (src, dest, tool_preset_name);
      break;

    case CLAY_CONTEXT_PROP_BUFFER:
      clay_context_real_set_buffer (dest, src->buffer);
      COPY_NAME (src, dest, buffer_name);
      break;

    case CLAY_CONTEXT_PROP_IMAGEFILE:
      clay_context_real_set_imagefile (dest, src->imagefile);
      COPY_NAME (src, dest, imagefile_name);
      break;

    case CLAY_CONTEXT_PROP_TEMPLATE:
      clay_context_real_set_template (dest, src->template);
      COPY_NAME (src, dest, template_name);
      break;

    default:
      break;
    }
}

void
clay_context_copy_properties (ClayContext         *src,
                              ClayContext         *dest,
                              ClayContextPropMask  prop_mask)
{
  ClayContextPropType prop;

  g_return_if_fail (CLAY_IS_CONTEXT (src));
  g_return_if_fail (CLAY_IS_CONTEXT (dest));

  for (prop = CLAY_CONTEXT_PROP_FIRST; prop <= CLAY_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      clay_context_copy_property (src, dest, prop);
}


/*  attribute access functions  */

/*****************************************************************************/
/*  manipulate by GType  *****************************************************/

ClayContextPropType
clay_context_type_to_property (GType type)
{
  ClayContextPropType prop;

  for (prop = CLAY_CONTEXT_PROP_FIRST; prop <= CLAY_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, clay_context_prop_types[prop]))
        return prop;
    }

  return -1;
}

const gchar *
clay_context_type_to_prop_name (GType type)
{
  ClayContextPropType prop;

  for (prop = CLAY_CONTEXT_PROP_FIRST; prop <= CLAY_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, clay_context_prop_types[prop]))
        return clay_context_prop_names[prop];
    }

  return NULL;
}

const gchar *
clay_context_type_to_signal_name (GType type)
{
  ClayContextPropType prop;

  for (prop = CLAY_CONTEXT_PROP_FIRST; prop <= CLAY_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, clay_context_prop_types[prop]))
        return g_signal_name (clay_context_signals[prop]);
    }

  return NULL;
}

ClayObject *
clay_context_get_by_type (ClayContext *context,
                          GType        type)
{
  ClayContextPropType  prop;
  ClayObject          *object = NULL;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  prop = clay_context_type_to_property (type);

  g_return_val_if_fail (prop != -1, NULL);

  g_object_get (context,
                clay_context_prop_names[prop], &object,
                NULL);

  /*  g_object_get() refs the object, this function however is a getter,
   *  which usually doesn't ref it's return value
   */
  if (object)
    g_object_unref (object);

  return object;
}

void
clay_context_set_by_type (ClayContext *context,
                          GType        type,
                          ClayObject  *object)
{
  ClayContextPropType  prop;
  GParamSpec          *pspec;
  GValue               value = G_VALUE_INIT;

  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (object == NULL || G_IS_OBJECT (object));

  prop = clay_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (context),
                                        clay_context_prop_names[prop]);

  g_return_if_fail (pspec != NULL);

  g_value_init (&value, pspec->value_type);
  g_value_set_object (&value, object);

  /*  we use clay_context_set_property() (which in turn only calls
   *  clay_context_set_foo() functions) instead of the much more obvious
   *  g_object_set(); this avoids g_object_freeze_notify()/thaw_notify()
   *  around the g_object_set() and makes ClayContext callbacks being
   *  called in a much more predictable order. See bug #731279.
   */
  clay_context_set_property (G_OBJECT (context),
                             pspec->param_id,
                             (const GValue *) &value,
                             pspec);

  g_value_unset (&value);
}

void
clay_context_changed_by_type (ClayContext *context,
                              GType        type)
{
  ClayContextPropType  prop;
  ClayObject          *object;

  g_return_if_fail (CLAY_IS_CONTEXT (context));

  prop = clay_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  object = clay_context_get_by_type (context, type);

  g_signal_emit (context,
                 clay_context_signals[prop], 0,
                 object);
}


/*****************************************************************************/
/*  image  *******************************************************************/

ClayImage *
clay_context_get_image (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->image;
}

void
clay_context_set_image (ClayContext *context,
                        ClayImage   *image)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  context_find_defined (context, CLAY_CONTEXT_PROP_IMAGE);

  clay_context_real_set_image (context, image);
}

void
clay_context_image_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[IMAGE_CHANGED], 0,
                 context->image);
}

/* This is a utility function to share, across the program, some common logic of
 * "which format to use when you are not sure and you need to give generic color
 * information" in RGBA.
 * @babl_type must be a valid babl type name such as "u8", "double", "float".
 * If @space_image is not NULL, it will be used to return the ClayImage
 * associated to the returned format (if the returned format is indeed using the
 * image's space).
 *
 * The logic for the format to use in RGB color actions is as follows:
 * - The space we navigate through is the active image's space.
 * - Increasing/decreasing follows the image TRC (in particular, if the image is
 *   linear or perceptual, we care about chromaticities yet don't follow the
 *   space TRC).
 * - If there is no active image or if its space is non-sRGB, we use the context
 *   color's space (if set).
 * - We discard non-RGB spaces and fallback to sRGB.
 */
const Babl *
clay_context_get_rgba_format (ClayContext  *context,
                              GeglColor    *color,
                              const gchar  *babl_type,
                              ClayImage   **space_image)
{
  ClayImage            *image  = NULL;
  const Babl           *format = NULL;
  const Babl           *space  = NULL;
  gchar                *format_name;
  ClayTRCType           trc = CLAY_TRC_NON_LINEAR;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (babl_type != NULL , NULL);
  g_return_val_if_fail (space_image == NULL || *space_image == NULL, NULL);

  image = clay_context_get_image (context);
  if (image)
    {
      format = clay_image_get_layer_format (image, FALSE);
      space  = babl_format_get_space (format);
      if (space_image)
        *space_image = image;
    }

  if (color != NULL && (space == NULL || ! babl_space_is_rgb (space)))
    {
      format = gegl_color_get_format (color);
      space  = babl_format_get_space (format);
      if (space_image)
        *space_image = NULL;
    }

  if (! babl_space_is_rgb (space))
    {
      format = NULL;
      space  = NULL;
      if (space_image)
        *space_image = NULL;
    }

  if (format != NULL)
    {
      if (image != NULL)
        {
          ClayPrecision precision;

          precision = clay_image_get_precision (image);
          trc       = clay_babl_trc (precision);
        }
      else
        {
          trc = clay_babl_format_get_trc (format);
        }
    }

  switch (trc)
    {
    case CLAY_TRC_LINEAR:
      format_name = g_strdup_printf ("RGBA %s", babl_type);
      break;
    case CLAY_TRC_NON_LINEAR:
      format_name = g_strdup_printf ("R'G'B'A %s", babl_type);
      break;
    case CLAY_TRC_PERCEPTUAL:
      format_name = g_strdup_printf ("R~G~B~A %s", babl_type);
      break;
    default:
      g_return_val_if_reached (NULL);
    }
  format = babl_format_with_space (format_name, space);
  g_free (format_name);

  return format;
}

static void
clay_context_image_disconnect (ClayImage   *image,
                               ClayContext *context)
{
  if (image == context->image)
    clay_context_real_set_image (context, NULL);
}

static void
clay_context_image_removed (ClayContainer *container,
                            ClayImage     *image,
                            ClayContext   *context)
{
  if (context->image == image)
    clay_context_real_set_image (context, NULL);
}

static void
clay_context_real_set_image (ClayContext *context,
                             ClayImage   *image)
{
  if (context->image == image)
    return;

  if (context->image)
    g_signal_handlers_disconnect_by_func (context->image,
                                          G_CALLBACK (clay_context_image_disconnect),
                                          context);

  context->image = image;

  if (image)
    g_signal_connect_object (image, "disconnect",
                             G_CALLBACK (clay_context_image_disconnect),
                             context, 0);

  g_object_notify (G_OBJECT (context), "image");
  clay_context_image_changed (context);
}


/*****************************************************************************/
/*  display  *****************************************************************/

ClayDisplay *
clay_context_get_display (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->display;
}

void
clay_context_set_display (ClayContext *context,
                          ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (display == NULL || CLAY_IS_DISPLAY (display));

  context_find_defined (context, CLAY_CONTEXT_PROP_DISPLAY);

  clay_context_real_set_display (context, display);
}

void
clay_context_display_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[DISPLAY_CHANGED], 0,
                 context->display);
}

static void
clay_context_display_removed (ClayContainer *container,
                              ClayDisplay   *display,
                              ClayContext   *context)
{
  if (context->display == display)
    clay_context_real_set_display (context, NULL);
}

static void
clay_context_real_set_display (ClayContext *context,
                               ClayDisplay *display)
{
  ClayDisplay *old_display;

  if (context->display == display)
    {
      /*  make sure that setting a display *always* sets the image
       *  to that display's image, even if the display already
       *  matches
       */
      if (display)
        {
          ClayImage *image;

          g_object_get (display, "image", &image, NULL);

          clay_context_real_set_image (context, image);

          if (image)
            g_object_unref (image);
        }

      return;
    }

  old_display = context->display;

  context->display = display;

  if (context->display)
    {
      ClayImage *image;

      g_object_get (display, "image", &image, NULL);

      clay_context_real_set_image (context, image);

      if (image)
        g_object_unref (image);
    }
  else if (old_display)
    {
      clay_context_real_set_image (context, NULL);
    }

  g_object_notify (G_OBJECT (context), "display");
  clay_context_display_changed (context);
}


/*****************************************************************************/
/*  tool  ********************************************************************/

ClayToolInfo *
clay_context_get_tool (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->tool_info;
}

void
clay_context_set_tool (ClayContext  *context,
                       ClayToolInfo *tool_info)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (tool_info == NULL || CLAY_IS_TOOL_INFO (tool_info));

  context_find_defined (context, CLAY_CONTEXT_PROP_TOOL);

  clay_context_real_set_tool (context, tool_info);
}

void
clay_context_tool_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[TOOL_CHANGED], 0,
                 context->tool_info);
}

static void
clay_context_tool_dirty (ClayToolInfo *tool_info,
                         ClayContext  *context)
{
  g_free (context->tool_name);
  context->tool_name = g_strdup (clay_object_get_name (tool_info));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_TOOL);
}

static void
clay_context_tool_list_thaw (ClayContainer *container,
                             ClayContext   *context)
{
  ClayToolInfo *tool_info;

  if (! context->tool_name)
    context->tool_name = g_strdup ("clay-paintbrush-tool");

  tool_info = clay_context_find_object (context, container,
                                        context->tool_name,
                                        clay_tool_info_get_standard (context->clay));

  clay_context_real_set_tool (context, tool_info);
}

static void
clay_context_tool_removed (ClayContainer *container,
                           ClayToolInfo  *tool_info,
                           ClayContext   *context)
{
  if (tool_info == context->tool_info)
    {
      g_signal_handlers_disconnect_by_func (context->tool_info,
                                            clay_context_tool_dirty,
                                            context);
      g_clear_object (&context->tool_info);

      if (! clay_container_frozen (container))
        clay_context_tool_list_thaw (container, context);
    }
}

static void
clay_context_real_set_tool (ClayContext  *context,
                            ClayToolInfo *tool_info)
{
  if (context->tool_info == tool_info)
    return;

  if (context->tool_name &&
      tool_info != clay_tool_info_get_standard (context->clay))
    {
      g_clear_pointer (&context->tool_name, g_free);
    }

  if (context->tool_info)
    {
      g_signal_handlers_disconnect_by_func (context->tool_info,
                                            G_CALLBACK (clay_context_tool_dirty),
                                            context);
    }

  g_set_object (&context->tool_info, tool_info);

  if (tool_info)
    {
      g_signal_connect_object (tool_info, "name-changed",
                               G_CALLBACK (clay_context_tool_dirty),
                               context,
                               0);

      if (tool_info != clay_tool_info_get_standard (context->clay))
        context->tool_name = g_strdup (clay_object_get_name (tool_info));

      if (tool_info->paint_info)
        clay_context_real_set_paint_info (context, tool_info->paint_info);
    }

  g_object_notify (G_OBJECT (context), "tool");
  clay_context_tool_changed (context);
}


/*****************************************************************************/
/*  paint info  **************************************************************/

ClayPaintInfo *
clay_context_get_paint_info (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->paint_info;
}

void
clay_context_set_paint_info (ClayContext   *context,
                             ClayPaintInfo *paint_info)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (paint_info == NULL || CLAY_IS_PAINT_INFO (paint_info));

  context_find_defined (context, CLAY_CONTEXT_PROP_PAINT_INFO);

  clay_context_real_set_paint_info (context, paint_info);
}

void
clay_context_paint_info_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[PAINT_INFO_CHANGED], 0,
                 context->paint_info);
}

static void
clay_context_paint_info_dirty (ClayPaintInfo *paint_info,
                               ClayContext   *context)
{
  g_set_str (&context->paint_name, clay_object_get_name (paint_info));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_PAINT_INFO);
}

/*  the global paint info list is there again after refresh  */
static void
clay_context_paint_info_list_thaw (ClayContainer *container,
                                   ClayContext   *context)
{
  ClayPaintInfo *paint_info;

  if (! context->paint_name)
    context->paint_name = g_strdup ("clay-paintbrush");

  paint_info = clay_context_find_object (context, container,
                                         context->paint_name,
                                         clay_paint_info_get_standard (context->clay));

  clay_context_real_set_paint_info (context, paint_info);
}

static void
clay_context_paint_info_removed (ClayContainer *container,
                                 ClayPaintInfo *paint_info,
                                 ClayContext   *context)
{
  if (paint_info == context->paint_info)
    {
      g_signal_handlers_disconnect_by_func (context->paint_info,
                                            clay_context_paint_info_dirty,
                                            context);
      g_clear_object (&context->paint_info);

      if (! clay_container_frozen (container))
        clay_context_paint_info_list_thaw (container, context);
    }
}

static void
clay_context_real_set_paint_info (ClayContext   *context,
                                  ClayPaintInfo *paint_info)
{
  if (context->paint_info == paint_info)
    return;

  if (context->paint_name &&
      paint_info != clay_paint_info_get_standard (context->clay))
    {
      g_clear_pointer (&context->paint_name, g_free);
    }

  if (context->paint_info)
    g_signal_handlers_disconnect_by_func (context->paint_info,
                                          clay_context_paint_info_dirty,
                                          context);

  g_set_object (&context->paint_info, paint_info);

  if (paint_info)
    {
      g_signal_connect_object (paint_info, "name-changed",
                               G_CALLBACK (clay_context_paint_info_dirty),
                               context,
                               0);

      if (paint_info != clay_paint_info_get_standard (context->clay))
        context->paint_name = g_strdup (clay_object_get_name (paint_info));
    }

  g_object_notify (G_OBJECT (context), "paint-info");
  clay_context_paint_info_changed (context);
}


/*****************************************************************************/
/*  foreground color  ********************************************************/

GeglColor *
clay_context_get_foreground (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->foreground;
}

void
clay_context_set_foreground (ClayContext *context,
                             GeglColor   *color)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (GEGL_IS_COLOR (color));

  context_find_defined (context, CLAY_CONTEXT_PROP_FOREGROUND);

  clay_context_real_set_foreground (context, color);
}

void
clay_context_foreground_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[FOREGROUND_CHANGED], 0,
                 context->foreground);
}

static void
clay_context_real_set_foreground (ClayContext *context,
                                  GeglColor   *color)
{
  g_clear_object (&context->foreground);
  context->foreground = gegl_color_duplicate (color);
  clay_color_set_alpha (context->foreground, CLAY_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "foreground");
  clay_context_foreground_changed (context);
}


/*****************************************************************************/
/*  background color  ********************************************************/

GeglColor *
clay_context_get_background (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->background;
}

void
clay_context_set_background (ClayContext *context,
                             GeglColor   *color)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (GEGL_IS_COLOR (color));

  context_find_defined (context, CLAY_CONTEXT_PROP_BACKGROUND);

  clay_context_real_set_background (context, color);
}

void
clay_context_background_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[BACKGROUND_CHANGED], 0,
                 context->background);
}

static void
clay_context_real_set_background (ClayContext *context,
                                  GeglColor   *color)
{
  g_clear_object (&context->background);
  context->background = gegl_color_duplicate (color);
  clay_color_set_alpha (context->background, CLAY_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "background");
  clay_context_background_changed (context);
}


/*****************************************************************************/
/*  color utility functions  *************************************************/

void
clay_context_set_default_colors (ClayContext *context)
{
  ClayContext *bg_context;
  GeglColor   *fg;
  GeglColor   *bg;

  g_return_if_fail (CLAY_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, CLAY_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, CLAY_CONTEXT_PROP_BACKGROUND);

  fg = gegl_color_new ("black");
  bg = gegl_color_new ("white");

  clay_context_real_set_foreground (context, fg);
  clay_context_real_set_background (bg_context, bg);

  g_object_unref (fg);
  g_object_unref (bg);
}

void
clay_context_swap_colors (ClayContext *context)
{
  ClayContext *bg_context;
  GeglColor   *fg;
  GeglColor   *bg;

  g_return_if_fail (CLAY_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, CLAY_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, CLAY_CONTEXT_PROP_BACKGROUND);

  fg = g_object_ref (context->foreground);
  bg = g_object_ref (context->background);

  clay_context_real_set_foreground (context, bg);
  clay_context_real_set_background (bg_context, fg);

  g_object_unref (fg);
  g_object_unref (bg);
}


/*****************************************************************************/
/*  opacity  *****************************************************************/

gdouble
clay_context_get_opacity (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), CLAY_OPACITY_OPAQUE);

  return context->opacity;
}

void
clay_context_set_opacity (ClayContext *context,
                          gdouble      opacity)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  context_find_defined (context, CLAY_CONTEXT_PROP_OPACITY);

  clay_context_real_set_opacity (context, opacity);
}

void
clay_context_opacity_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[OPACITY_CHANGED], 0,
                 context->opacity);
}

static void
clay_context_real_set_opacity (ClayContext *context,
                               gdouble      opacity)
{
  if (context->opacity == opacity)
    return;

  context->opacity = opacity;

  g_object_notify (G_OBJECT (context), "opacity");
  clay_context_opacity_changed (context);
}


/*****************************************************************************/
/*  paint mode  **************************************************************/

ClayLayerMode
clay_context_get_paint_mode (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), CLAY_LAYER_MODE_NORMAL);

  return context->paint_mode;
}

void
clay_context_set_paint_mode (ClayContext   *context,
                             ClayLayerMode  paint_mode)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  context_find_defined (context, CLAY_CONTEXT_PROP_PAINT_MODE);

  clay_context_real_set_paint_mode (context, paint_mode);
}

void
clay_context_paint_mode_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[PAINT_MODE_CHANGED], 0,
                 context->paint_mode);
}

static void
clay_context_real_set_paint_mode (ClayContext   *context,
                                  ClayLayerMode  paint_mode)
{
  if (context->paint_mode == paint_mode)
    return;

  context->paint_mode = paint_mode;

  g_object_notify (G_OBJECT (context), "paint-mode");
  clay_context_paint_mode_changed (context);
}


/*****************************************************************************/
/*  brush  *******************************************************************/

ClayBrush *
clay_context_get_brush (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->brush;
}

void
clay_context_set_brush (ClayContext *context,
                        ClayBrush   *brush)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || CLAY_IS_BRUSH (brush));

  context_find_defined (context, CLAY_CONTEXT_PROP_BRUSH);

  clay_context_real_set_brush (context, brush);
}

void
clay_context_brush_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[BRUSH_CHANGED], 0,
                 context->brush);
}

static void
clay_context_brush_dirty (ClayBrush   *brush,
                          ClayContext *context)
{
  g_set_str (&context->brush_name, clay_object_get_name (brush));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_BRUSH);
}

static void
clay_context_brush_list_thaw (ClayContainer *container,
                              ClayContext   *context)
{
  ClayBrush *brush;

  if (! context->brush_name)
    context->brush_name = g_strdup (context->clay->config->default_brush);

  brush = clay_context_find_object (context, container,
                                    context->brush_name,
                                    clay_brush_get_standard (context));

  clay_context_real_set_brush (context, brush);
}

/*  the active brush disappeared  */
static void
clay_context_brush_removed (ClayContainer *container,
                            ClayBrush     *brush,
                            ClayContext   *context)
{
  if (brush == context->brush)
    {
      g_signal_handlers_disconnect_by_func (context->brush,
                                            clay_context_brush_dirty,
                                            context);
      g_clear_object (&context->brush);

      if (! clay_container_frozen (container))
        clay_context_brush_list_thaw (container, context);
    }
}

static void
clay_context_real_set_brush (ClayContext *context,
                             ClayBrush   *brush)
{
  if (context->brush == brush)
    return;

  if (context->brush_name &&
      brush != CLAY_BRUSH (clay_brush_get_standard (context)))
    {
      g_clear_pointer (&context->brush_name, g_free);
    }

  if (context->brush)
    g_signal_handlers_disconnect_by_func (context->brush,
                                          clay_context_brush_dirty,
                                          context);

  g_set_object (&context->brush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (clay_context_brush_dirty),
                               context,
                               0);

      if (brush != CLAY_BRUSH (clay_brush_get_standard (context)))
        context->brush_name = g_strdup (clay_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "brush");
  clay_context_brush_changed (context);
}


/*****************************************************************************/
/*  dynamics *****************************************************************/

ClayDynamics *
clay_context_get_dynamics (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->dynamics;
}

void
clay_context_set_dynamics (ClayContext  *context,
                           ClayDynamics *dynamics)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (dynamics == NULL || CLAY_IS_DYNAMICS (dynamics));

  context_find_defined (context, CLAY_CONTEXT_PROP_DYNAMICS);

  clay_context_real_set_dynamics (context, dynamics);
}

void
clay_context_dynamics_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[DYNAMICS_CHANGED], 0,
                 context->dynamics);
}

static void
clay_context_dynamics_dirty (ClayDynamics *dynamics,
                             ClayContext  *context)
{
  g_set_str (&context->dynamics_name, clay_object_get_name (dynamics));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_DYNAMICS);
}

static void
clay_context_dynamics_removed (ClayContainer *container,
                               ClayDynamics  *dynamics,
                               ClayContext   *context)
{
  if (dynamics == context->dynamics)
    {
      g_signal_handlers_disconnect_by_func (context->dynamics,
                                            clay_context_dynamics_dirty,
                                            context);
      g_clear_object (&context->dynamics);

      if (! clay_container_frozen (container))
        clay_context_dynamics_list_thaw (container, context);
    }
}

static void
clay_context_dynamics_list_thaw (ClayContainer *container,
                                 ClayContext   *context)
{
  ClayDynamics *dynamics;

  if (! context->dynamics_name)
    context->dynamics_name = g_strdup (context->clay->config->default_dynamics);

  dynamics = clay_context_find_object (context, container,
                                       context->dynamics_name,
                                       clay_dynamics_get_standard (context));

  clay_context_real_set_dynamics (context, dynamics);
}

static void
clay_context_real_set_dynamics (ClayContext  *context,
                                ClayDynamics *dynamics)
{
  if (context->dynamics == dynamics)
    return;

  if (context->dynamics_name &&
      dynamics != CLAY_DYNAMICS (clay_dynamics_get_standard (context)))
    {
      g_clear_pointer (&context->dynamics_name, g_free);
    }

  if (context->dynamics)
    g_signal_handlers_disconnect_by_func (context->dynamics,
                                          clay_context_dynamics_dirty,
                                          context);

  g_set_object (&context->dynamics, dynamics);

  if (dynamics)
    {
      g_signal_connect_object (dynamics, "name-changed",
                               G_CALLBACK (clay_context_dynamics_dirty),
                               context,
                               0);

      if (dynamics != CLAY_DYNAMICS (clay_dynamics_get_standard (context)))
        context->dynamics_name = g_strdup (clay_object_get_name (dynamics));
    }

  g_object_notify (G_OBJECT (context), "dynamics");
  clay_context_dynamics_changed (context);
}


/*****************************************************************************/
/*  mybrush  *****************************************************************/

ClayMybrush *
clay_context_get_mybrush (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->mybrush;
}

void
clay_context_set_mybrush (ClayContext *context,
                          ClayMybrush *brush)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || CLAY_IS_MYBRUSH (brush));

  context_find_defined (context, CLAY_CONTEXT_PROP_MYBRUSH);

  clay_context_real_set_mybrush (context, brush);
}

void
clay_context_mybrush_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[MYBRUSH_CHANGED], 0,
                 context->mybrush);
}

static void
clay_context_mybrush_dirty (ClayMybrush *brush,
                            ClayContext *context)
{
  g_set_str (&context->mybrush_name, clay_object_get_name (brush));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_MYBRUSH);
}

static void
clay_context_mybrush_list_thaw (ClayContainer *container,
                                ClayContext   *context)
{
  ClayMybrush *brush;

  if (! context->mybrush_name)
    context->mybrush_name = g_strdup (context->clay->config->default_mypaint_brush);

  brush = clay_context_find_object (context, container,
                                    context->mybrush_name,
                                    clay_mybrush_get_standard (context));

  clay_context_real_set_mybrush (context, brush);
}

static void
clay_context_mybrush_removed (ClayContainer *container,
                              ClayMybrush   *brush,
                              ClayContext   *context)
{
  if (brush == context->mybrush)
    {
      g_signal_handlers_disconnect_by_func (context->mybrush,
                                            clay_context_mybrush_dirty,
                                            context);
      g_clear_object (&context->mybrush);

      if (! clay_container_frozen (container))
        clay_context_mybrush_list_thaw (container, context);
    }
}

static void
clay_context_real_set_mybrush (ClayContext *context,
                               ClayMybrush *brush)
{
  if (context->mybrush == brush)
    return;

  if (context->mybrush_name &&
      brush != CLAY_MYBRUSH (clay_mybrush_get_standard (context)))
    {
      g_clear_pointer (&context->mybrush_name, g_free);
    }

  if (context->mybrush)
    g_signal_handlers_disconnect_by_func (context->mybrush,
                                          clay_context_mybrush_dirty,
                                          context);

  g_set_object (&context->mybrush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (clay_context_mybrush_dirty),
                               context,
                               0);

      if (brush != CLAY_MYBRUSH (clay_mybrush_get_standard (context)))
        context->mybrush_name = g_strdup (clay_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "mybrush");
  clay_context_mybrush_changed (context);
}


/*****************************************************************************/
/*  pattern  *****************************************************************/

ClayPattern *
clay_context_get_pattern (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->pattern;
}

void
clay_context_set_pattern (ClayContext *context,
                          ClayPattern *pattern)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (pattern == NULL || CLAY_IS_PATTERN (pattern));

  context_find_defined (context, CLAY_CONTEXT_PROP_PATTERN);

  clay_context_real_set_pattern (context, pattern);
}

void
clay_context_pattern_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[PATTERN_CHANGED], 0,
                 context->pattern);
}

static void
clay_context_pattern_dirty (ClayPattern *pattern,
                            ClayContext *context)
{
  g_set_str (&context->pattern_name, clay_object_get_name (pattern));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_PATTERN);
}

static void
clay_context_pattern_list_thaw (ClayContainer *container,
                                ClayContext   *context)
{
  ClayPattern *pattern;

  if (! context->pattern_name)
    context->pattern_name = g_strdup (context->clay->config->default_pattern);

  pattern = clay_context_find_object (context, container,
                                      context->pattern_name,
                                      clay_pattern_get_standard (context));

  clay_context_real_set_pattern (context, pattern);
}

static void
clay_context_pattern_removed (ClayContainer *container,
                              ClayPattern   *pattern,
                              ClayContext   *context)
{
  if (pattern == context->pattern)
    {
      g_signal_handlers_disconnect_by_func (context->pattern,
                                            clay_context_pattern_dirty,
                                            context);
      g_clear_object (&context->pattern);

      if (! clay_container_frozen (container))
        clay_context_pattern_list_thaw (container, context);
    }
}

static void
clay_context_real_set_pattern (ClayContext *context,
                               ClayPattern *pattern)
{
  if (context->pattern == pattern)
    return;

  if (context->pattern_name &&
      pattern != CLAY_PATTERN (clay_pattern_get_standard (context)))
    {
      g_clear_pointer (&context->pattern_name, g_free);
    }

  if (context->pattern)
    g_signal_handlers_disconnect_by_func (context->pattern,
                                          clay_context_pattern_dirty,
                                          context);

  g_set_object (&context->pattern, pattern);

  if (pattern)
    {
      g_signal_connect_object (pattern, "name-changed",
                               G_CALLBACK (clay_context_pattern_dirty),
                               context,
                               0);

      if (pattern != CLAY_PATTERN (clay_pattern_get_standard (context)))
        context->pattern_name = g_strdup (clay_object_get_name (pattern));
    }

  g_object_notify (G_OBJECT (context), "pattern");
  clay_context_pattern_changed (context);
}


/*****************************************************************************/
/*  gradient  ****************************************************************/

ClayGradient *
clay_context_get_gradient (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->gradient;
}

void
clay_context_set_gradient (ClayContext  *context,
                           ClayGradient *gradient)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (gradient == NULL || CLAY_IS_GRADIENT (gradient));

  context_find_defined (context, CLAY_CONTEXT_PROP_GRADIENT);

  clay_context_real_set_gradient (context, gradient);
}

void
clay_context_gradient_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[GRADIENT_CHANGED], 0,
                 context->gradient);
}

static void
clay_context_gradient_dirty (ClayGradient *gradient,
                             ClayContext  *context)
{
  g_set_str (&context->gradient_name, clay_object_get_name (gradient));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_GRADIENT);
}

static void
clay_context_gradient_list_thaw (ClayContainer *container,
                                 ClayContext   *context)
{
  ClayGradient *gradient;

  if (! context->gradient_name)
    context->gradient_name = g_strdup (context->clay->config->default_gradient);

  gradient = clay_context_find_object (context, container,
                                       context->gradient_name,
                                       clay_gradient_get_standard (context));

  clay_context_real_set_gradient (context, gradient);
}

static void
clay_context_gradient_removed (ClayContainer *container,
                               ClayGradient  *gradient,
                               ClayContext   *context)
{
  if (gradient == context->gradient)
    {
      g_signal_handlers_disconnect_by_func (context->gradient,
                                            clay_context_gradient_dirty,
                                            context);
      g_clear_object (&context->gradient);

      if (! clay_container_frozen (container))
        clay_context_gradient_list_thaw (container, context);
    }
}

static void
clay_context_real_set_gradient (ClayContext  *context,
                                ClayGradient *gradient)
{
  if (context->gradient == gradient)
    return;

  if (context->gradient_name &&
      gradient != CLAY_GRADIENT (clay_gradient_get_standard (context)))
    {
      g_clear_pointer (&context->gradient_name, g_free);
    }

  if (context->gradient)
    g_signal_handlers_disconnect_by_func (context->gradient,
                                          clay_context_gradient_dirty,
                                          context);

  g_set_object (&context->gradient, gradient);

  if (gradient)
    {
      g_signal_connect_object (gradient, "name-changed",
                               G_CALLBACK (clay_context_gradient_dirty),
                               context,
                               0);

      if (gradient != CLAY_GRADIENT (clay_gradient_get_standard (context)))
        context->gradient_name = g_strdup (clay_object_get_name (gradient));
    }

  g_object_notify (G_OBJECT (context), "gradient");
  clay_context_gradient_changed (context);
}


/*****************************************************************************/
/*  palette  *****************************************************************/

ClayPalette *
clay_context_get_palette (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->palette;
}

void
clay_context_set_palette (ClayContext *context,
                          ClayPalette *palette)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (palette == NULL || CLAY_IS_PALETTE (palette));

  context_find_defined (context, CLAY_CONTEXT_PROP_PALETTE);

  clay_context_real_set_palette (context, palette);
}

void
clay_context_palette_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[PALETTE_CHANGED], 0,
                 context->palette);
}

static void
clay_context_palette_dirty (ClayPalette *palette,
                            ClayContext *context)
{
  g_set_str (&context->palette_name, clay_object_get_name (palette));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_PALETTE);
}

static void
clay_context_palette_list_thaw (ClayContainer *container,
                                ClayContext   *context)
{
  ClayPalette *palette;

  if (! context->palette_name)
    context->palette_name = g_strdup (context->clay->config->default_palette);

  palette = clay_context_find_object (context, container,
                                      context->palette_name,
                                      clay_palette_get_standard (context));

  clay_context_real_set_palette (context, palette);
}

static void
clay_context_palette_removed (ClayContainer *container,
                              ClayPalette   *palette,
                              ClayContext   *context)
{
  if (palette == context->palette)
    {
      g_signal_handlers_disconnect_by_func (context->palette,
                                            clay_context_palette_dirty,
                                            context);
      g_clear_object (&context->palette);

      if (! clay_container_frozen (container))
        clay_context_palette_list_thaw (container, context);
    }
}

static void
clay_context_real_set_palette (ClayContext *context,
                               ClayPalette *palette)
{
  if (context->palette == palette)
    return;

  if (context->palette_name &&
      palette != CLAY_PALETTE (clay_palette_get_standard (context)))
    {
      g_clear_pointer (&context->palette_name, g_free);
    }

  if (context->palette)
    g_signal_handlers_disconnect_by_func (context->palette,
                                          clay_context_palette_dirty,
                                          context);

  g_set_object (&context->palette, palette);

  if (palette)
    {
      g_signal_connect_object (palette, "name-changed",
                               G_CALLBACK (clay_context_palette_dirty),
                               context,
                               0);

      if (palette != CLAY_PALETTE (clay_palette_get_standard (context)))
        context->palette_name = g_strdup (clay_object_get_name (palette));
    }

  g_object_notify (G_OBJECT (context), "palette");
  clay_context_palette_changed (context);
}


/*****************************************************************************/
/*  font     *****************************************************************/

ClayFont *
clay_context_get_font (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->font;
}

void
clay_context_set_font (ClayContext *context,
                       ClayFont    *font)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (font == NULL || CLAY_IS_FONT (font));

  context_find_defined (context, CLAY_CONTEXT_PROP_FONT);

  clay_context_real_set_font (context, font);
}

const gchar *
clay_context_get_font_name (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->font_name;
}

void
clay_context_set_font_name (ClayContext *context,
                            const gchar *name)
{
  ClayContainer *container;
  ClayObject    *font;

  g_return_if_fail (CLAY_IS_CONTEXT (context));

  container = clay_data_factory_get_container (context->clay->font_factory);
  font      = clay_container_search (container,
                                     (ClayContainerSearchFunc) clay_font_match_by_lookup_name,
                                     (gpointer) name);

  if (font)
    {
      clay_context_set_font (context, CLAY_FONT (font));
    }
  else
    {
      /* No font with this name exists, use the standard font, but
       * keep the intended name around
       */
      clay_context_set_font (context, CLAY_FONT (clay_font_get_standard ()));

      g_set_str (&context->font_name, name);
    }
}

void
clay_context_font_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[FONT_CHANGED], 0,
                 context->font);
}

static void
clay_context_font_dirty (ClayFont    *font,
                         ClayContext *context)
{
  g_set_str (&context->font_name,
             clay_object_get_name (font));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_FONT);
}

static void
clay_context_font_list_thaw (ClayContainer *container,
                             ClayContext   *context)
{
  ClayFont *font;

  if (! context->font_name)
    context->font_name = g_strdup (context->clay->config->default_font);

  font = clay_context_find_object (context, container,
                                   context->font_name,
                                   clay_font_get_standard ());

  clay_context_real_set_font (context, font);
}

static void
clay_context_font_removed (ClayContainer *container,
                           ClayFont      *font,
                           ClayContext   *context)
{
  if (font == context->font)
    {
      g_signal_handlers_disconnect_by_func (context->font,
                                            clay_context_font_dirty,
                                            context);
      g_clear_object (&context->font);

      if (! clay_container_frozen (container))
        clay_context_font_list_thaw (container, context);
    }
}

static void
clay_context_real_set_font (ClayContext *context,
                            ClayFont    *font)
{
  if (context->font == font)
    return;

  if (context->font_name &&
      font != CLAY_FONT (clay_font_get_standard ()))
    {
      g_clear_pointer (&context->font_name, g_free);
    }

  if (context->font)
    g_signal_handlers_disconnect_by_func (context->font,
                                          clay_context_font_dirty,
                                          context);

  g_set_object (&context->font, font);

  if (font)
    {
      g_signal_connect_object (font, "name-changed",
                               G_CALLBACK (clay_context_font_dirty),
                               context,
                               0);

      if (font != CLAY_FONT (clay_font_get_standard ()))
        context->font_name = g_strdup (clay_font_get_lookup_name (font));
    }

  g_object_notify (G_OBJECT (context), "font");
  clay_context_font_changed (context);
}


/********************************************************************************/
/*  tool preset *****************************************************************/

ClayToolPreset *
clay_context_get_tool_preset (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->tool_preset;
}

void
clay_context_set_tool_preset (ClayContext    *context,
                              ClayToolPreset *tool_preset)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (tool_preset == NULL || CLAY_IS_TOOL_PRESET (tool_preset));

  context_find_defined (context, CLAY_CONTEXT_PROP_TOOL_PRESET);

  clay_context_real_set_tool_preset (context, tool_preset);
}

void
clay_context_tool_preset_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[TOOL_PRESET_CHANGED], 0,
                 context->tool_preset);
}

static void
clay_context_tool_preset_dirty (ClayToolPreset *tool_preset,
                                ClayContext    *context)
{
  g_set_str (&context->tool_preset_name, clay_object_get_name (tool_preset));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_TOOL_PRESET);
}

static void
clay_context_tool_preset_removed (ClayContainer  *container,
                                  ClayToolPreset *tool_preset,
                                  ClayContext    *context)
{
  if (tool_preset == context->tool_preset)
    {
      g_signal_handlers_disconnect_by_func (context->tool_preset,
                                            clay_context_tool_preset_dirty,
                                            context);
      g_clear_object (&context->tool_preset);

      if (! clay_container_frozen (container))
        clay_context_tool_preset_list_thaw (container, context);
    }
}

static void
clay_context_tool_preset_list_thaw (ClayContainer *container,
                                    ClayContext   *context)
{
  ClayToolPreset *tool_preset;

  tool_preset = clay_context_find_object (context, container,
                                          context->tool_preset_name,
                                          NULL);

  clay_context_real_set_tool_preset (context, tool_preset);
}

static void
clay_context_real_set_tool_preset (ClayContext    *context,
                                   ClayToolPreset *tool_preset)
{
  if (context->tool_preset == tool_preset)
    return;

  if (context->tool_preset_name)
    {
      g_clear_pointer (&context->tool_preset_name, g_free);
    }

  if (context->tool_preset)
    g_signal_handlers_disconnect_by_func (context->tool_preset,
                                          clay_context_tool_preset_dirty,
                                          context);

  g_set_object (&context->tool_preset, tool_preset);

  if (tool_preset)
    {
      g_signal_connect_object (tool_preset, "name-changed",
                               G_CALLBACK (clay_context_tool_preset_dirty),
                               context,
                               0);

      context->tool_preset_name = g_strdup (clay_object_get_name (tool_preset));
    }

  g_object_notify (G_OBJECT (context), "tool-preset");
  clay_context_tool_preset_changed (context);
}


/*****************************************************************************/
/*  buffer  ******************************************************************/

ClayBuffer *
clay_context_get_buffer (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->buffer;
}

void
clay_context_set_buffer (ClayContext *context,
                         ClayBuffer *buffer)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (buffer == NULL || CLAY_IS_BUFFER (buffer));

  context_find_defined (context, CLAY_CONTEXT_PROP_BUFFER);

  clay_context_real_set_buffer (context, buffer);
}

void
clay_context_buffer_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[BUFFER_CHANGED], 0,
                 context->buffer);
}

static void
clay_context_buffer_dirty (ClayBuffer  *buffer,
                           ClayContext *context)
{
  g_set_str (&context->buffer_name, clay_object_get_name (buffer));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_BUFFER);
}

static void
clay_context_buffer_list_thaw (ClayContainer *container,
                               ClayContext   *context)
{
  ClayBuffer *buffer;

  buffer = clay_context_find_object (context, container,
                                     context->buffer_name,
                                     NULL);

  if (buffer)
    {
      clay_context_real_set_buffer (context, buffer);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "buffer");
      clay_context_buffer_changed (context);
    }
}

static void
clay_context_buffer_removed (ClayContainer *container,
                             ClayBuffer    *buffer,
                             ClayContext   *context)
{
  if (buffer == context->buffer)
    {
      g_signal_handlers_disconnect_by_func (context->buffer,
                                            clay_context_buffer_dirty,
                                            context);
      g_clear_object (&context->buffer);

      if (! clay_container_frozen (container))
        clay_context_buffer_list_thaw (container, context);
    }
}

static void
clay_context_real_set_buffer (ClayContext *context,
                              ClayBuffer  *buffer)
{
  if (context->buffer == buffer)
    return;

  if (context->buffer_name)
    {
      g_clear_pointer (&context->buffer_name, g_free);
    }

  if (context->buffer)
    g_signal_handlers_disconnect_by_func (context->buffer,
                                          clay_context_buffer_dirty,
                                          context);

  g_set_object (&context->buffer, buffer);

  if (buffer)
    {
      g_signal_connect_object (buffer, "name-changed",
                               G_CALLBACK (clay_context_buffer_dirty),
                               context,
                               0);

      context->buffer_name = g_strdup (clay_object_get_name (buffer));
    }

  g_object_notify (G_OBJECT (context), "buffer");
  clay_context_buffer_changed (context);
}


/*****************************************************************************/
/*  imagefile  ***************************************************************/

ClayImagefile *
clay_context_get_imagefile (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->imagefile;
}

void
clay_context_set_imagefile (ClayContext   *context,
                            ClayImagefile *imagefile)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (imagefile == NULL || CLAY_IS_IMAGEFILE (imagefile));

  context_find_defined (context, CLAY_CONTEXT_PROP_IMAGEFILE);

  clay_context_real_set_imagefile (context, imagefile);
}

void
clay_context_imagefile_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[IMAGEFILE_CHANGED], 0,
                 context->imagefile);
}

static void
clay_context_imagefile_dirty (ClayImagefile *imagefile,
                              ClayContext   *context)
{
  g_set_str (&context->imagefile_name, clay_object_get_name (imagefile));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_IMAGEFILE);
}

static void
clay_context_imagefile_list_thaw (ClayContainer *container,
                                  ClayContext   *context)
{
  ClayImagefile *imagefile;

  imagefile = clay_context_find_object (context, container,
                                        context->imagefile_name,
                                        NULL);

  if (imagefile)
    {
      clay_context_real_set_imagefile (context, imagefile);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "imagefile");
      clay_context_imagefile_changed (context);
    }
}

static void
clay_context_imagefile_removed (ClayContainer *container,
                                ClayImagefile *imagefile,
                                ClayContext   *context)
{
  if (imagefile == context->imagefile)
    {
      g_signal_handlers_disconnect_by_func (context->imagefile,
                                            clay_context_imagefile_dirty,
                                            context);
      g_clear_object (&context->imagefile);

      if (! clay_container_frozen (container))
        clay_context_imagefile_list_thaw (container, context);
    }
}

static void
clay_context_real_set_imagefile (ClayContext   *context,
                                 ClayImagefile *imagefile)
{
  if (context->imagefile == imagefile)
    return;

  if (context->imagefile_name)
    {
      g_clear_pointer (&context->imagefile_name, g_free);
    }

  if (context->imagefile)
    g_signal_handlers_disconnect_by_func (context->imagefile,
                                          clay_context_imagefile_dirty,
                                          context);

  g_set_object (&context->imagefile, imagefile);

  if (imagefile)
    {
      g_signal_connect_object (imagefile, "name-changed",
                               G_CALLBACK (clay_context_imagefile_dirty),
                               context,
                               0);

      context->imagefile_name = g_strdup (clay_object_get_name (imagefile));
    }

  g_object_notify (G_OBJECT (context), "imagefile");
  clay_context_imagefile_changed (context);
}


/*****************************************************************************/
/*  template  ***************************************************************/

ClayTemplate *
clay_context_get_template (ClayContext *context)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return context->template;
}

void
clay_context_set_template (ClayContext  *context,
                           ClayTemplate *template)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (template == NULL || CLAY_IS_TEMPLATE (template));

  context_find_defined (context, CLAY_CONTEXT_PROP_TEMPLATE);

  clay_context_real_set_template (context, template);
}

void
clay_context_template_changed (ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  g_signal_emit (context,
                 clay_context_signals[TEMPLATE_CHANGED], 0,
                 context->template);
}

static void
clay_context_template_dirty (ClayTemplate *template,
                             ClayContext  *context)
{
  g_set_str (&context->template_name, clay_object_get_name (template));

  g_signal_emit (context, clay_context_signals[PROP_NAME_CHANGED], 0,
                 CLAY_CONTEXT_PROP_TEMPLATE);
}

static void
clay_context_template_list_thaw (ClayContainer *container,
                                 ClayContext   *context)
{
  ClayTemplate *template;

  template = clay_context_find_object (context, container,
                                       context->template_name,
                                       NULL);

  if (template)
    {
      clay_context_real_set_template (context, template);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "template");
      clay_context_template_changed (context);
    }
}

static void
clay_context_template_removed (ClayContainer *container,
                               ClayTemplate  *template,
                               ClayContext   *context)
{
  if (template == context->template)
    {
      g_signal_handlers_disconnect_by_func (context->template,
                                            clay_context_template_dirty,
                                            context);
      g_clear_object (&context->template);

      if (! clay_container_frozen (container))
        clay_context_template_list_thaw (container, context);
    }
}

static void
clay_context_real_set_template (ClayContext  *context,
                                ClayTemplate *template)
{
  if (context->template == template)
    return;

  if (context->template_name)
    {
      g_clear_pointer (&context->template_name, g_free);
    }

  if (context->template)
    g_signal_handlers_disconnect_by_func (context->template,
                                          clay_context_template_dirty,
                                          context);

  g_set_object (&context->template, template);

  if (template)
    {
      g_signal_connect_object (template, "name-changed",
                               G_CALLBACK (clay_context_template_dirty),
                               context,
                               0);

      context->template_name = g_strdup (clay_object_get_name (template));
    }

  g_object_notify (G_OBJECT (context), "template");
  clay_context_template_changed (context);
}


/*****************************************************************************/
/*  Line Art  ****************************************************************/

ClayLineArt *
clay_context_take_line_art (ClayContext *context)
{
  ClayLineArt *line_art;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;

      line_art = context->line_art;
      context->line_art = NULL;
    }
  else
    {
      line_art = clay_line_art_new ();
    }

  return line_art;
}

/*
 * clay_context_store_line_art:
 * @context:
 * @line_art:
 *
 * The @context takes ownership of @line_art until the next time it is
 * requested with clay_context_take_line_art() or until 3 minutes have
 * passed.
 * This function allows to temporarily store the computed line art data
 * in case it is needed very soon again, so that not to free and
 * recompute all the time the data when quickly switching tools.
 */
void
clay_context_store_line_art (ClayContext *context,
                             ClayLineArt *line_art)
{
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (CLAY_IS_LINE_ART (line_art));

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;
    }

  context->line_art            = line_art;
  context->line_art_timeout_id = g_timeout_add (180000,
                                                (GSourceFunc) clay_context_free_line_art,
                                                context);
}

static gboolean
clay_context_free_line_art (ClayContext *context)
{
  g_clear_object (&context->line_art);

  context->line_art_timeout_id = 0;

  return G_SOURCE_REMOVE;
}


/*****************************************************************************/
/*  utility functions  *******************************************************/

static gpointer
clay_context_find_object (ClayContext   *context,
                          ClayContainer *container,
                          const gchar   *object_name,
                          gpointer       standard_object)
{
  ClayObject *object = NULL;

  if (object_name)
    object = clay_container_get_child_by_name (container, object_name);

  if (! object && ! clay_container_is_empty (container))
    object = clay_container_get_child_by_index (container, 0);

  if (! object)
    object = standard_object;

  return object;
}
