/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontext.h
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

#pragma once

#include "clayviewable.h"


#define CLAY_TYPE_CONTEXT            (clay_context_get_type ())
#define CLAY_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTEXT, ClayContext))
#define CLAY_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, CLAY_TYPE_CONTEXT, ClayContextClass))
#define CLAY_IS_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTEXT))
#define CLAY_IS_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTEXT))
#define CLAY_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS (obj, CLAY_TYPE_CONTEXT, ClayContextClass))


typedef struct _ClayContextClass ClayContextClass;

/**
 * ClayContext:
 *
 * Holds state such as the active image, active display, active brush,
 * active foreground and background color, and so on. There can many
 * instances of contexts. The user context is what the user sees and
 * interacts with but there can also be contexts for docks and for
 * plug-ins.
 */
struct _ClayContext
{
  ClayViewable          parent_instance;

  Clay                 *clay;

  ClayContext          *parent;

  guint32               defined_props;
  guint32               serialize_props;

  ClayImage            *image;
  ClayDisplay          *display;

  ClayToolInfo         *tool_info;
  gchar                *tool_name;

  ClayPaintInfo        *paint_info;
  gchar                *paint_name;

  GeglColor            *foreground;
  GeglColor            *background;

  gdouble               opacity;
  ClayLayerMode         paint_mode;

  ClayBrush            *brush;
  gchar                *brush_name;

  ClayDynamics         *dynamics;
  gchar                *dynamics_name;

  ClayMybrush          *mybrush;
  gchar                *mybrush_name;

  ClayPattern          *pattern;
  gchar                *pattern_name;

  ClayGradient         *gradient;
  gchar                *gradient_name;

  ClayPalette          *palette;
  gchar                *palette_name;

  ClayFont             *font;
  gchar                *font_name;

  ClayToolPreset       *tool_preset;
  gchar                *tool_preset_name;

  ClayBuffer           *buffer;
  gchar                *buffer_name;

  ClayImagefile        *imagefile;
  gchar                *imagefile_name;

  ClayTemplate         *template;
  gchar                *template_name;

  ClayLineArt          *line_art;
  guint                 line_art_timeout_id;
};

struct _ClayContextClass
{
  ClayViewableClass  parent_class;

  void (* image_changed)      (ClayContext          *context,
                               ClayImage            *image);
  void (* display_changed)    (ClayContext          *context,
                               ClayDisplay          *display);

  void (* tool_changed)       (ClayContext          *context,
                               ClayToolInfo         *tool_info);
  void (* paint_info_changed) (ClayContext          *context,
                               ClayPaintInfo        *paint_info);

  void (* foreground_changed) (ClayContext          *context,
                               GeglColor            *color);
  void (* background_changed) (ClayContext          *context,
                               GeglColor            *color);
  void (* opacity_changed)    (ClayContext          *context,
                               gdouble               opacity);
  void (* paint_mode_changed) (ClayContext          *context,
                               ClayLayerMode         paint_mode);
  void (* brush_changed)      (ClayContext          *context,
                               ClayBrush            *brush);
  void (* dynamics_changed)   (ClayContext          *context,
                               ClayDynamics         *dynamics);
  void (* mybrush_changed)    (ClayContext          *context,
                               ClayMybrush          *brush);
  void (* pattern_changed)    (ClayContext          *context,
                               ClayPattern          *pattern);
  void (* gradient_changed)   (ClayContext          *context,
                               ClayGradient         *gradient);
  void (* palette_changed)    (ClayContext          *context,
                               ClayPalette          *palette);
  void (* font_changed)       (ClayContext          *context,
                               ClayFont             *font);
  void (* tool_preset_changed)(ClayContext          *context,
                               ClayToolPreset       *tool_preset);
  void (* buffer_changed)     (ClayContext          *context,
                               ClayBuffer           *buffer);
  void (* imagefile_changed)  (ClayContext          *context,
                               ClayImagefile        *imagefile);
  void (* template_changed)   (ClayContext          *context,
                               ClayTemplate         *template);

  void (* prop_name_changed)  (ClayContext          *context,
                               ClayContextPropType   prop);
};


GType         clay_context_get_type          (void);

ClayContext * clay_context_new               (Clay                *clay,
                                              const gchar         *name,
                                              ClayContext         *template);

ClayContext * clay_context_get_parent        (ClayContext         *context);
void          clay_context_set_parent        (ClayContext         *context,
                                              ClayContext         *parent);

/*  define / undefinine context properties
 *
 *  the value of an undefined property will be taken from the parent context.
 */
void          clay_context_define_property   (ClayContext         *context,
                                              ClayContextPropType  prop,
                                              gboolean             defined);

gboolean      clay_context_property_defined  (ClayContext         *context,
                                              ClayContextPropType  prop);

void          clay_context_define_properties (ClayContext         *context,
                                              ClayContextPropMask  props_mask,
                                              gboolean             defined);


/*  specify which context properties will be serialized
 */
void   clay_context_set_serialize_properties (ClayContext         *context,
                                              ClayContextPropMask  props_mask);

ClayContextPropMask
       clay_context_get_serialize_properties (ClayContext         *context);


/*  copying context properties
 */
void          clay_context_copy_property     (ClayContext         *src,
                                              ClayContext         *dest,
                                              ClayContextPropType  prop);

void          clay_context_copy_properties   (ClayContext         *src,
                                              ClayContext         *dest,
                                              ClayContextPropMask  props_mask);


/*  manipulate by GType  */
ClayContextPropType   clay_context_type_to_property    (GType     type);
const gchar         * clay_context_type_to_prop_name   (GType     type);
const gchar         * clay_context_type_to_signal_name (GType     type);

ClayObject     * clay_context_get_by_type         (ClayContext     *context,
                                                   GType            type);
void             clay_context_set_by_type         (ClayContext     *context,
                                                   GType            type,
                                                   ClayObject      *object);
void             clay_context_changed_by_type     (ClayContext     *context,
                                                   GType            type);


/*  image  */
ClayImage      * clay_context_get_image           (ClayContext     *context);
void             clay_context_set_image           (ClayContext     *context,
                                                   ClayImage       *image);
void             clay_context_image_changed       (ClayContext     *context);


const Babl *     clay_context_get_rgba_format     (ClayContext     *context,
                                                   GeglColor       *color,
                                                   const gchar     *babl_type,
                                                   ClayImage      **space_image);


/*  display  */
ClayDisplay    * clay_context_get_display         (ClayContext     *context);
void             clay_context_set_display         (ClayContext     *context,
                                                   ClayDisplay     *display);
void             clay_context_display_changed     (ClayContext     *context);


/*  tool  */
ClayToolInfo   * clay_context_get_tool            (ClayContext     *context);
void             clay_context_set_tool            (ClayContext     *context,
                                                   ClayToolInfo    *tool_info);

void             clay_context_tool_changed        (ClayContext     *context);

/*  paint info  */
ClayPaintInfo  * clay_context_get_paint_info      (ClayContext     *context);
void             clay_context_set_paint_info      (ClayContext     *context,
                                                   ClayPaintInfo   *paint_info);
void             clay_context_paint_info_changed  (ClayContext     *context);


/*  foreground color  */
GeglColor      * clay_context_get_foreground       (ClayContext     *context);
void             clay_context_set_foreground       (ClayContext     *context,
                                                    GeglColor       *color);
void             clay_context_foreground_changed   (ClayContext     *context);


/*  background color  */
GeglColor      * clay_context_get_background       (ClayContext     *context);
void             clay_context_set_background       (ClayContext     *context,
                                                    GeglColor       *color);
void             clay_context_background_changed   (ClayContext     *context);


/*  color utility functions  */
void             clay_context_set_default_colors  (ClayContext     *context);
void             clay_context_swap_colors         (ClayContext     *context);


/*  opacity  */
gdouble          clay_context_get_opacity         (ClayContext     *context);
void             clay_context_set_opacity         (ClayContext     *context,
                                                   gdouble          opacity);
void             clay_context_opacity_changed     (ClayContext     *context);


/*  paint mode  */
ClayLayerMode    clay_context_get_paint_mode      (ClayContext     *context);
void             clay_context_set_paint_mode      (ClayContext     *context,
                                                   ClayLayerMode    paint_mode);
void             clay_context_paint_mode_changed  (ClayContext     *context);


/*  brush  */
ClayBrush      * clay_context_get_brush           (ClayContext     *context);
void             clay_context_set_brush           (ClayContext     *context,
                                                   ClayBrush       *brush);
void             clay_context_brush_changed       (ClayContext     *context);


/*  dynamics  */
ClayDynamics   * clay_context_get_dynamics        (ClayContext     *context);
void             clay_context_set_dynamics        (ClayContext     *context,
                                                   ClayDynamics    *dynamics);
void             clay_context_dynamics_changed    (ClayContext     *context);


/*  mybrush  */
ClayMybrush    * clay_context_get_mybrush         (ClayContext     *context);
void             clay_context_set_mybrush         (ClayContext     *context,
                                                   ClayMybrush     *brush);
void             clay_context_mybrush_changed     (ClayContext     *context);


/*  pattern  */
ClayPattern    * clay_context_get_pattern         (ClayContext     *context);
void             clay_context_set_pattern         (ClayContext     *context,
                                                   ClayPattern     *pattern);
void             clay_context_pattern_changed     (ClayContext     *context);


/*  gradient  */
ClayGradient   * clay_context_get_gradient        (ClayContext     *context);
void             clay_context_set_gradient        (ClayContext     *context,
                                                   ClayGradient    *gradient);
void             clay_context_gradient_changed    (ClayContext     *context);


/*  palette  */
ClayPalette    * clay_context_get_palette         (ClayContext     *context);
void             clay_context_set_palette         (ClayContext     *context,
                                                   ClayPalette     *palette);
void             clay_context_palette_changed     (ClayContext     *context);


/*  font  */
ClayFont       * clay_context_get_font            (ClayContext     *context);
void             clay_context_set_font            (ClayContext     *context,
                                                   ClayFont        *font);
const gchar    * clay_context_get_font_name       (ClayContext     *context);
void             clay_context_set_font_name       (ClayContext     *context,
                                                   const gchar     *name);
void             clay_context_font_changed        (ClayContext     *context);


/*  tool_preset  */
ClayToolPreset * clay_context_get_tool_preset     (ClayContext     *context);
void             clay_context_set_tool_preset     (ClayContext     *context,
                                                   ClayToolPreset  *tool_preset);
void             clay_context_tool_preset_changed (ClayContext     *context);


/*  buffer  */
ClayBuffer     * clay_context_get_buffer          (ClayContext     *context);
void             clay_context_set_buffer          (ClayContext     *context,
                                                   ClayBuffer      *palette);
void             clay_context_buffer_changed      (ClayContext     *context);


/*  imagefile  */
ClayImagefile  * clay_context_get_imagefile       (ClayContext     *context);
void             clay_context_set_imagefile       (ClayContext     *context,
                                                   ClayImagefile   *imagefile);
void             clay_context_imagefile_changed   (ClayContext     *context);


/*  template  */
ClayTemplate   * clay_context_get_template        (ClayContext     *context);
void             clay_context_set_template        (ClayContext     *context,
                                                   ClayTemplate    *template);
void             clay_context_template_changed    (ClayContext     *context);

/*  line art  */
ClayLineArt    * clay_context_take_line_art       (ClayContext     *context);
void             clay_context_store_line_art      (ClayContext     *context,
                                                   ClayLineArt     *line_art);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayContext, g_object_unref);
