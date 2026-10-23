/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "clay-utils.h"
#include "claygrid.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_FGCOLOR,
  PROP_BGCOLOR,
  PROP_XSPACING,
  PROP_YSPACING,
  PROP_SPACING_UNIT,
  PROP_XOFFSET,
  PROP_YOFFSET,
  PROP_OFFSET_UNIT
};


static void     clay_grid_savable_iface_init (ClaySavableInterface *iface);

static void     clay_grid_finalize           (GObject              *object);
static void     clay_grid_get_property       (GObject              *object,
                                              guint                 property_id,
                                              GValue               *value,
                                              GParamSpec           *pspec);
static void     clay_grid_set_property       (GObject              *object,
                                              guint                 property_id,
                                              const GValue         *value,
                                              GParamSpec           *pspec);

static void     clay_grid_save               (ClaySavable           *savable,
                                              ClaySaveState         *state);
static gboolean clay_grid_load_enter         (ClayLoadState         *state,
                                              const gchar          **attribute_names,
                                              const gchar          **attribute_values,
                                              gpointer               user_data,
                                              GError               **error);
static gboolean clay_grid_load_exit          (ClayLoadState         *state,
                                              const gchar           *text,
                                              gsize                  len,
                                              gpointer               user_data,
                                              GError               **error);
static gboolean clay_grid_enter_fg_bg        (ClayLoadState         *state,
                                              const gchar          **attribute_names,
                                              const gchar          **attribute_values,
                                              gpointer               user_data,
                                              GError               **error);
static gboolean clay_grid_exit_fg_bg         (ClayLoadState         *state,
                                              const gchar           *text,
                                              gsize                  len,
                                              gpointer               user_data,
                                              GError               **error);
static gboolean clay_grid_enter_unit         (ClayLoadState         *state,
                                              const gchar          **attribute_names,
                                              const gchar          **attribute_values,
                                              gpointer               user_data,
                                              GError               **error);
static gboolean clay_grid_exit_unit          (ClayLoadState         *state,
                                              const gchar           *text,
                                              gsize                  len,
                                              gpointer               user_data,
                                              GError               **error);


G_DEFINE_TYPE_WITH_CODE (ClayGrid, clay_grid, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_grid_savable_iface_init))

#define parent_class clay_grid_parent_class


static void
clay_grid_class_init (ClayGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *black        = gegl_color_new ("black");
  GeglColor    *white        = gegl_color_new ("white");

  object_class->finalize     = clay_grid_finalize;
  object_class->get_property = clay_grid_get_property;
  object_class->set_property = clay_grid_set_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Line style"),
                         _("Line style used for the grid."),
                         CLAY_TYPE_GRID_STYLE,
                         CLAY_GRID_SOLID,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_FGCOLOR,
                          "fgcolor",
                          _("Foreground color"),
                          _("The foreground color of the grid."),
                          TRUE, black,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_BGCOLOR,
                          "bgcolor",
                          _("Background color"),
                          _("The background color of the grid; "
                            "only used in double dashed line style."),
                          TRUE, white,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_XSPACING,
                           "xspacing",
                           _("Spacing X"),
                           _("Horizontal spacing of grid lines."),
                           0.0, CLAY_MAX_IMAGE_SIZE, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_YSPACING,
                           "yspacing",
                           _("Spacing Y"),
                           _("Vertical spacing of grid lines."),
                           0.0, CLAY_MAX_IMAGE_SIZE, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_SPACING_UNIT,
                         "spacing-unit",
                         _("Spacing unit"),
                         NULL,
                         FALSE, FALSE, clay_unit_inch (),
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_XOFFSET,
                           "xoffset",
                           _("Offset X"),
                           _("Horizontal offset of the first grid "
                             "line; this may be a negative number."),
                           - CLAY_MAX_IMAGE_SIZE,
                           CLAY_MAX_IMAGE_SIZE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_YOFFSET,
                           "yoffset",
                           _("Offset Y"),
                           _("Vertical offset of the first grid "
                             "line; this may be a negative number."),
                           - CLAY_MAX_IMAGE_SIZE,
                           CLAY_MAX_IMAGE_SIZE, 0.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_UNIT (object_class, PROP_OFFSET_UNIT,
                         "offset-unit",
                         _("Offset unit"),
                         NULL,
                         FALSE, FALSE, clay_unit_inch (),
                         CLAY_PARAM_STATIC_STRINGS);

  g_object_unref (black);
  g_object_unref (white);
}

static void
clay_grid_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "grid";
  iface->save       = clay_grid_save;
  iface->load_enter = clay_grid_load_enter;
  iface->load_exit  = clay_grid_load_exit;
}

static void
clay_grid_init (ClayGrid *grid)
{
  grid->fgcolor = gegl_color_new ("black");
  grid->bgcolor = gegl_color_new ("white");
}

static void
clay_grid_finalize (GObject *object)
{
  ClayGrid *grid = CLAY_GRID (object);

  g_object_unref (grid->fgcolor);
  g_object_unref (grid->bgcolor);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_grid_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  ClayGrid *grid = CLAY_GRID (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, grid->style);
      break;
    case PROP_FGCOLOR:
      g_value_set_object (value, grid->fgcolor);
      break;
    case PROP_BGCOLOR:
      g_value_set_object (value, grid->bgcolor);
      break;
    case PROP_XSPACING:
      g_value_set_double (value, grid->xspacing);
      break;
    case PROP_YSPACING:
      g_value_set_double (value, grid->yspacing);
      break;
    case PROP_SPACING_UNIT:
      g_value_set_object (value, grid->spacing_unit);
      break;
    case PROP_XOFFSET:
      g_value_set_double (value, grid->xoffset);
      break;
    case PROP_YOFFSET:
      g_value_set_double (value, grid->yoffset);
      break;
    case PROP_OFFSET_UNIT:
      g_value_set_object (value, grid->offset_unit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_grid_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayGrid *grid = CLAY_GRID (object);

  switch (property_id)
    {
    case PROP_STYLE:
      grid->style = g_value_get_enum (value);
      break;
    case PROP_FGCOLOR:
      g_clear_object (&grid->fgcolor);
      grid->fgcolor = gegl_color_duplicate (g_value_get_object (value));
      break;
    case PROP_BGCOLOR:
      g_clear_object (&grid->bgcolor);
      grid->bgcolor = gegl_color_duplicate (g_value_get_object (value));
      break;
    case PROP_XSPACING:
      grid->xspacing = g_value_get_double (value);
      break;
    case PROP_YSPACING:
      grid->yspacing = g_value_get_double (value);
      break;
    case PROP_SPACING_UNIT:
      grid->spacing_unit = g_value_get_object (value);
      break;
    case PROP_XOFFSET:
      grid->xoffset = g_value_get_double (value);
      break;
    case PROP_YOFFSET:
      grid->yoffset = g_value_get_double (value);
      break;
    case PROP_OFFSET_UNIT:
      grid->offset_unit = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_grid_save (ClaySavable   *savable,
                ClaySaveState *state)
{
  ClayGrid *grid = CLAY_GRID (savable);

  clay_savable_print_element_start (state, "grid", NULL);

  clay_savable_print_element (state, "style",
                              "%s", clay_get_enum_value_nick (CLAY_TYPE_GRID_STYLE, grid->style),
                              NULL);

  clay_savable_print_element_start (state, "foreground", NULL);
  clay_savable_color_save (grid->fgcolor, NULL, NULL, state);
  clay_savable_print_element_end (state, "foreground");

  clay_savable_print_element_start (state, "background", NULL);
  clay_savable_color_save (grid->bgcolor, NULL, NULL, state);
  clay_savable_print_element_end (state, "background");

  clay_savable_print_element (state, "spacing", NULL, NULL,
                              "x", "%f", grid->xspacing,
                              "y", "%f", grid->yspacing,
                              NULL);

  clay_savable_print_element_start (state, "spacing-unit", NULL);
  clay_savable_unit_save (grid->spacing_unit, state);
  clay_savable_print_element_end (state, "spacing-unit");

  clay_savable_print_element (state, "offset", NULL, NULL,
                              "x", "%f", grid->xoffset,
                              "y", "%f", grid->yoffset,
                              NULL);
  clay_savable_print_element_start (state, "offset-unit", NULL);
  clay_savable_unit_save (grid->offset_unit, state);
  clay_savable_print_element_end (state, "offset-unit");

  clay_savable_print_element_end (state, "grid");
}

static gboolean
clay_grid_load_enter (ClayLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError        **error)
{
  clay_savable_load_add_simple_handler (state, "style", CLAY_TYPE_GRID_STYLE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_handlers (state, "foreground",
                                  clay_grid_enter_fg_bg,
                                  clay_grid_exit_fg_bg,
                                  "foreground", NULL);
  clay_savable_load_add_handlers (state, "background",
                                  clay_grid_enter_fg_bg,
                                  clay_grid_exit_fg_bg,
                                  "background", NULL);
  clay_savable_load_add_simple_handler (state, "spacing", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "x", G_TYPE_DOUBLE,
                                        "y", G_TYPE_DOUBLE,
                                        NULL);
  clay_savable_load_add_handlers (state, "spacing-unit",
                                  clay_grid_enter_unit,
                                  clay_grid_exit_unit,
                                  "spacing-unit", NULL);
  clay_savable_load_add_simple_handler (state, "offset", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "x", G_TYPE_DOUBLE,
                                        "y", G_TYPE_DOUBLE,
                                        NULL);
  clay_savable_load_add_handlers (state, "offset-unit",
                                  clay_grid_enter_unit,
                                  clay_grid_exit_unit,
                                  "offset-unit", NULL);

  return TRUE;
}

static gboolean
clay_grid_load_exit (ClayLoadState  *state,
                     const gchar    *text,
                     gsize           len,
                     gpointer        user_data,
                     GError        **error)
{
  GeglColor     *fg;
  GeglColor     *bg;
  ClayGridStyle  style;
  ClayUnit      *spacing_unit;
  ClayUnit      *offset_unit;
  gdouble        x_spacing;
  gdouble        y_spacing;
  gdouble        x_offset;
  gdouble        y_offset;

  if (clay_savable_load_get_values (state,
                                "style",        &style,
                                "foreground",   &fg,
                                "background",   &bg,
                                "spacing:x",    &x_spacing,
                                "spacing:y",    &y_spacing,
                                "offset:x",     &x_offset,
                                "offset:y",     &y_offset,
                                "spacing-unit", &spacing_unit,
                                "offset-unit",  &offset_unit,
                                NULL))
    {
      ClayGrid *grid;

      grid = g_object_new (CLAY_TYPE_GRID,
                           "style",        style,
                           "fgcolor",      fg,
                           "bgcolor",      bg,
                           "xspacing",     x_spacing,
                           "yspacing",     y_spacing,
                           "spacing-unit", spacing_unit,
                           "xoffset",      x_offset,
                           "yoffset",      y_offset,
                           "offset-unit",  offset_unit,
                           NULL);

      clay_savable_load_store_value (state, "grid", grid, g_object_unref);
      clay_savable_load_bubble_up (state, "grid");
    }

  return TRUE;
}


/*  public functions  */

ClayGridStyle
clay_grid_get_style (ClayGrid *grid)
{
  g_return_val_if_fail (CLAY_IS_GRID (grid), CLAY_GRID_SOLID);

  return grid->style;
}

GeglColor *
clay_grid_get_fgcolor (ClayGrid *grid)
{
  g_return_val_if_fail (CLAY_IS_GRID (grid), NULL);

  return grid->fgcolor;
}

GeglColor *
clay_grid_get_bgcolor (ClayGrid *grid)
{
  g_return_val_if_fail (CLAY_IS_GRID (grid), NULL);

  return grid->bgcolor;
}

void
clay_grid_get_spacing (ClayGrid *grid,
                       gdouble  *xspacing,
                       gdouble  *yspacing)
{
  g_return_if_fail (CLAY_IS_GRID (grid));

  if (xspacing) *xspacing = grid->xspacing;
  if (yspacing) *yspacing = grid->yspacing;
}

void
clay_grid_get_offset (ClayGrid *grid,
                      gdouble  *xoffset,
                      gdouble  *yoffset)
{
  g_return_if_fail (CLAY_IS_GRID (grid));

  if (xoffset) *xoffset = grid->xoffset;
  if (yoffset) *yoffset = grid->yoffset;
}

const gchar *
clay_grid_parasite_name (void)
{
  return "clay-image-grid";
}

ClayParasite *
clay_grid_to_parasite (ClayGrid *grid)
{
  g_return_val_if_fail (CLAY_IS_GRID (grid), NULL);

  return clay_config_serialize_to_parasite (CLAY_CONFIG (grid),
                                            clay_grid_parasite_name (),
                                            CLAY_PARASITE_PERSISTENT,
                                            NULL);
}

ClayGrid *
clay_grid_from_parasite (const ClayParasite *parasite)
{
  ClayGrid *grid;
  GError   *error = NULL;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (clay_parasite_get_name (parasite),
                                clay_grid_parasite_name ()) == 0, NULL);

  if (! clay_parasite_get_data (parasite, NULL))
    {
      g_warning ("Empty grid parasite");

      return NULL;
    }

  grid = g_object_new (CLAY_TYPE_GRID, NULL);

  if (! clay_config_deserialize_parasite (CLAY_CONFIG (grid),
                                          parasite,
                                          NULL,
                                          &error))
    {
      g_warning ("Failed to deserialize grid parasite: %s", error->message);
      g_error_free (error);
    }

  return grid;
}


/* Private Functions */

static gboolean
clay_grid_enter_fg_bg (ClayLoadState  *state,
                       const gchar   **attribute_names,
                       const gchar   **attribute_values,
                       gpointer        user_data,
                       GError        **error)
{
  clay_savable_load_add_handlers (state, "color",
                                  clay_savable_enter_color,
                                  clay_savable_exit_color,
                                  NULL, NULL);
  return TRUE;
}

static gboolean
clay_grid_exit_fg_bg (ClayLoadState  *state,
                      const gchar    *text,
                      gsize           len,
                      gpointer        user_data,
                      GError        **error)
{
  GeglColor   *color;
  const gchar *varname = (const gchar *) user_data;

  if (clay_savable_load_get_values (state, "color", &color, NULL))
    clay_savable_load_store_value (state, varname, g_object_ref (color),
                                   g_object_unref);
  clay_savable_load_bubble_up (state, varname);

  return TRUE;
}


static gboolean
clay_grid_enter_unit (ClayLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError        **error)
{
  clay_savable_load_add_handlers (state, "unit",
                                  clay_savable_enter_unit,
                                  clay_savable_exit_unit,
                                  NULL, NULL);
  return TRUE;
}

static gboolean
clay_grid_exit_unit (ClayLoadState  *state,
                     const gchar    *text,
                     gsize           len,
                     gpointer        user_data,
                     GError        **error)
{
  ClayUnit    *unit;
  const gchar *varname = (const gchar *) user_data;

  if (clay_savable_load_get_values (state, "unit", &unit, NULL))
    clay_savable_load_store_value (state, varname, unit, NULL);
  clay_savable_load_bubble_up (state, varname);

  return TRUE;
}
