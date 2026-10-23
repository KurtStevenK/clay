/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
#include "libclayconfig/clayconfig.h"

#include "tools-types.h"

#include "clayfilteroptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_PREVIEW,
  PROP_PREVIEW_SPLIT,
  PROP_PREVIEW_SPLIT_ALIGNMENT,
  PROP_PREVIEW_SPLIT_POSITION,
  PROP_CONTROLLER,
  PROP_BLENDING_OPTIONS_EXPANDED,
  PROP_MERGE_FILTER
};


static void   clay_filter_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   clay_filter_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (ClayFilterOptions, clay_filter_options,
               CLAY_TYPE_COLOR_OPTIONS)

#define parent_class clay_filter_options_parent_class


static void
clay_filter_options_class_init (ClayFilterOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_filter_options_set_property;
  object_class->get_property = clay_filter_options_get_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_PREVIEW,
                            "preview",
                            _("_Preview"),
                            NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT,
                                   g_param_spec_boolean ("preview-split",
                                                         _("Split _view"),
                                                         NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT_ALIGNMENT,
                                   g_param_spec_enum ("preview-split-alignment",
                                                      NULL, NULL,
                                                      CLAY_TYPE_ALIGNMENT_TYPE,
                                                      CLAY_ALIGN_LEFT,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PREVIEW_SPLIT_POSITION,
                                   g_param_spec_int ("preview-split-position",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONTROLLER,
                            "controller",
                            _("On-canvas con_trols"),
                            _("Show on-canvas filter controls"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_BLENDING_OPTIONS_EXPANDED,
                            "blending-options-expanded",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_MERGE_FILTER,
                            "merge-filter",
                            _("_Merge filter"),
                            _("If enabled, the filter is immediately merged.\n"
                              "Otherwise, it becomes an editable non-destructive filter."),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_filter_options_init (ClayFilterOptions *options)
{
}

static void
clay_filter_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayFilterOptions *options = CLAY_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      options->preview = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_SPLIT:
      options->preview_split = g_value_get_boolean (value);
      break;

    case PROP_PREVIEW_SPLIT_ALIGNMENT:
      options->preview_split_alignment = g_value_get_enum (value);
      break;

    case PROP_PREVIEW_SPLIT_POSITION:
      options->preview_split_position = g_value_get_int (value);
      break;

    case PROP_CONTROLLER:
      options->controller = g_value_get_boolean (value);
      break;

    case PROP_BLENDING_OPTIONS_EXPANDED:
      options->blending_options_expanded = g_value_get_boolean (value);
      break;

    case PROP_MERGE_FILTER:
      options->merge_filter = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_filter_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayFilterOptions *options = CLAY_FILTER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_PREVIEW:
      g_value_set_boolean (value, options->preview);
      break;

    case PROP_PREVIEW_SPLIT:
      g_value_set_boolean (value, options->preview_split);
      break;

    case PROP_PREVIEW_SPLIT_ALIGNMENT:
      g_value_set_enum (value, options->preview_split_alignment);
      break;

    case PROP_PREVIEW_SPLIT_POSITION:
      g_value_set_int (value, options->preview_split_position);
      break;

    case PROP_CONTROLLER:
      g_value_set_boolean (value, options->controller);
      break;

    case PROP_BLENDING_OPTIONS_EXPANDED:
      g_value_set_boolean (value, options->blending_options_expanded);
      break;

    case PROP_MERGE_FILTER:
      g_value_set_boolean (value, options->merge_filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
clay_filter_options_switch_preview_side (ClayFilterOptions *options)
{
  ClayAlignmentType alignment;

  g_return_if_fail (CLAY_IS_FILTER_OPTIONS (options));

  switch (options->preview_split_alignment)
    {
    case CLAY_ALIGN_LEFT:   alignment = CLAY_ALIGN_RIGHT;  break;
    case CLAY_ALIGN_RIGHT:  alignment = CLAY_ALIGN_LEFT;   break;
    case CLAY_ALIGN_TOP:    alignment = CLAY_ALIGN_BOTTOM; break;
    case CLAY_ALIGN_BOTTOM: alignment = CLAY_ALIGN_TOP;    break;
    default:
      g_return_if_reached ();
    }

  g_object_set (options, "preview-split-alignment", alignment, NULL);
}

void
clay_filter_options_switch_preview_orientation (ClayFilterOptions *options,
                                                gint               position_x,
                                                gint               position_y)
{
  ClayAlignmentType alignment;
  gint              position;

  g_return_if_fail (CLAY_IS_FILTER_OPTIONS (options));

  switch (options->preview_split_alignment)
    {
    case CLAY_ALIGN_LEFT:   alignment = CLAY_ALIGN_TOP;    break;
    case CLAY_ALIGN_RIGHT:  alignment = CLAY_ALIGN_BOTTOM; break;
    case CLAY_ALIGN_TOP:    alignment = CLAY_ALIGN_LEFT;   break;
    case CLAY_ALIGN_BOTTOM: alignment = CLAY_ALIGN_RIGHT;  break;
    default:
      g_return_if_reached ();
    }

  if (alignment == CLAY_ALIGN_LEFT ||
      alignment == CLAY_ALIGN_RIGHT)
    {
      position = position_x;
    }
  else
    {
      position = position_y;
    }

  g_object_set (options,
                "preview-split-alignment", alignment,
                "preview-split-position",  position,
                NULL);
}
