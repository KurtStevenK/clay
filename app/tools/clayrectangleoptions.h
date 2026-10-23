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

#pragma once


typedef enum
{
  CLAY_RECTANGLE_OPTIONS_PROP_0,

  CLAY_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
  CLAY_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
  CLAY_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
  CLAY_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
  CLAY_RECTANGLE_OPTIONS_PROP_GUIDE,

  CLAY_RECTANGLE_OPTIONS_PROP_X,
  CLAY_RECTANGLE_OPTIONS_PROP_Y,
  CLAY_RECTANGLE_OPTIONS_PROP_WIDTH,
  CLAY_RECTANGLE_OPTIONS_PROP_HEIGHT,
  CLAY_RECTANGLE_OPTIONS_PROP_POSITION_UNIT,
  CLAY_RECTANGLE_OPTIONS_PROP_SIZE_UNIT,

  CLAY_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
  CLAY_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
  CLAY_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
  CLAY_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
  CLAY_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH,
  CLAY_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT,
  CLAY_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH,
  CLAY_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT,
  CLAY_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE,
  CLAY_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
  CLAY_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
  CLAY_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR,
  CLAY_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR,
  CLAY_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT,
  CLAY_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT,
  CLAY_RECTANGLE_OPTIONS_PROP_FIXED_UNIT,
  CLAY_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,

  CLAY_RECTANGLE_OPTIONS_PROP_LAST = CLAY_RECTANGLE_OPTIONS_PROP_FIXED_CENTER
} ClayRectangleOptionsProp;


#define CLAY_TYPE_RECTANGLE_OPTIONS               (clay_rectangle_options_get_type ())
#define CLAY_IS_RECTANGLE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_RECTANGLE_OPTIONS))
#define CLAY_RECTANGLE_OPTIONS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_RECTANGLE_OPTIONS, ClayRectangleOptions))
#define CLAY_RECTANGLE_OPTIONS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CLAY_TYPE_RECTANGLE_OPTIONS, ClayRectangleOptionsInterface))

#define CLAY_RECTANGLE_OPTIONS_GET_PRIVATE(obj)   (clay_rectangle_options_get_private (CLAY_RECTANGLE_OPTIONS (obj)))


typedef struct _ClayRectangleOptions          ClayRectangleOptions;
typedef struct _ClayRectangleOptionsInterface ClayRectangleOptionsInterface;
typedef struct _ClayRectangleOptionsPrivate   ClayRectangleOptionsPrivate;

struct _ClayRectangleOptionsInterface
{
  GTypeInterface base_iface;
};

struct _ClayRectangleOptionsPrivate
{
  gboolean                auto_shrink;
  gboolean                shrink_merged;

  gboolean                highlight;
  gdouble                 highlight_opacity;
  ClayGuidesType          guide;

  gdouble                 x;
  gdouble                 y;
  gdouble                 width;
  gdouble                 height;

  ClayUnit               *position_unit;
  ClayUnit               *size_unit;

  gboolean                fixed_rule_active;
  ClayRectangleFixedRule  fixed_rule;

  gdouble                 desired_fixed_width;
  gdouble                 desired_fixed_height;

  gdouble                 desired_fixed_size_width;
  gdouble                 desired_fixed_size_height;

  gdouble                 default_fixed_size_width;
  gdouble                 default_fixed_size_height;
  gboolean                overridden_fixed_size;

  gdouble                 aspect_numerator;
  gdouble                 aspect_denominator;

  gdouble                 default_aspect_numerator;
  gdouble                 default_aspect_denominator;
  gboolean                overridden_fixed_aspect;

  gboolean                fixed_center;

  /* This gboolean is not part of the actual rectangle tool options,
   * and should be refactored out along with the pointers to widgets.
   */
  gboolean                use_string_current;

  ClayUnit               *fixed_unit;

  /* options gui */

  GtkWidget              *auto_shrink_button;

  GtkWidget              *fixed_width_entry;
  GtkWidget              *fixed_height_entry;

  GtkWidget              *fixed_aspect_hbox;
  GtkWidget              *aspect_button_box;
  GtkListStore           *aspect_history;

  GtkWidget              *fixed_size_hbox;
  GtkWidget              *size_button_box;
  GtkListStore           *size_history;

  GtkWidget              *position_entry;
  GtkWidget              *size_entry;
};


GType       clay_rectangle_options_get_type           (void);

GtkWidget * clay_rectangle_options_gui                (ClayToolOptions      *tool_options);

void        clay_rectangle_options_connect            (ClayRectangleOptions *options,
                                                       ClayImage            *image,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);
void        clay_rectangle_options_disconnect         (ClayRectangleOptions *options,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);

gboolean    clay_rectangle_options_fixed_rule_active  (ClayRectangleOptions *rectangle_options,
                                                       ClayRectangleFixedRule fixed_rule);

ClayRectangleOptionsPrivate *
            clay_rectangle_options_get_private        (ClayRectangleOptions *options);


/*  convenience functions  */

void        clay_rectangle_options_install_properties (GObjectClass *klass);
void        clay_rectangle_options_set_property       (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
void        clay_rectangle_options_get_property       (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);


/*  testing helper functions  */

GtkWidget * clay_rectangle_options_get_size_entry     (ClayRectangleOptions *rectangle_options);
