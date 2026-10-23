/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claysavable.h
 * Copyright (C) 2026 Jehan
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


/* The ClaySavable interface is for various objects which will be saved
 * as part of a Clay project. Each object type handles its own format
 * to be printed as XML into the output stream passed into to_xml().
 *
 * Additionally to printing its XML, an object may also do further
 * saving operations, such as, for drawables, making sure their buffer
 * is up-to-date in the on-disk cache.
 *
 * A few utils are also provided for storing some external data.
 */

#define WLBR_VERSION 1

typedef struct _ClaySaveState ClaySaveState;
typedef struct _ClayLoadState ClayLoadState;

struct _ClaySaveState
{
  GOutputStream *output;
  ClayImage     *image;
  GFile         *xcf_file;
  GHashTable    *spaces;
  GQueue        *elements;
};

struct _ClayLoadState
{
  Clay          *clay;

  ClayImage     *image;
  gint           ID;
  GQueue        *objects;
  GFile         *xml_file;
  GFile         *subdir;

  ClayXmlParser *xml_parser;
  GMarkupParser  markup_parser;
  GQueue        *contexts;
  gint           level;
  gint           unexpected;

  GHashTable    *spaces;
};


/* Return with %FALSE and @error set if the error is fatale.
 * Return with %TRUE and @error to display a non-fatal error.
 * Return with %TRUE and a %NULL @error for normal handling.
 */
typedef gboolean (* ClayEnterElementHandler) (ClayLoadState  *state,
                                              const gchar   **attribute_names,
                                              const gchar   **attribute_values,
                                              gpointer        user_data,
                                              GError         **error);
typedef gboolean (* ClayExitElementhandler)  (ClayLoadState  *state,
                                              const gchar     *text,
                                              gsize            len,
                                              gpointer         user_data,
                                              GError         **error);


#define CLAY_TYPE_SAVABLE (clay_savable_get_type ())
G_DECLARE_INTERFACE (ClaySavable, clay_savable, CLAY, SAVABLE, GObject)


struct _ClaySavableInterface
{
  GTypeInterface base_iface;

  const gchar *tag;

  /*  virtual functions  */
  void   (* save) (ClaySavable   *savable,
                   ClaySaveState *state);

  ClayEnterElementHandler load_enter;
  ClayExitElementhandler  load_exit;
};


void         clay_savable_save                 (ClaySavable            *savable,
                                                ClaySaveState          *state);


/* Shared Utils */

void         clay_savable_print_element_start  (ClaySaveState          *state,
                                                const gchar            *element_name,
                                                ...) G_GNUC_NULL_TERMINATED;
void         clay_savable_print_element_end    (ClaySaveState          *state,
                                                const gchar            *element_name);
void         clay_savable_print_element        (ClaySaveState          *state,
                                                const gchar            *element_name,
                                                const gchar            *format_value,
                                                ...) G_GNUC_NULL_TERMINATED;

void         clay_savable_config_save          (ClayConfig             *config,
                                                const gchar            *element_name,
                                                ClaySaveState          *state);

void         clay_savable_format_save          (const Babl             *format,
                                                ClaySaveState          *state);
void         clay_savable_space_save           (const Babl             *space,
                                                ClaySaveState          *state,
                                                gint                    space_id);
void         clay_savable_color_save           (GeglColor              *color,
                                                const gchar            *name,
                                                const Babl             *restricted_format,
                                                ClaySaveState          *state);

void         clay_savable_save_all_spaces      (ClayImage              *image,
                                                ClaySaveState          *state);

void         clay_savable_unit_save            (ClayUnit               *unit,
                                                ClaySaveState          *state);
void         clay_savable_metadata_save        (ClayMetadata           *metadata,
                                                ClaySaveState          *state);
void         clay_savable_parasite_save        (ClayParasite           *parasite,
                                                ClaySaveState          *state);
void         clay_savable_matrix2_save         (ClayMatrix2            *matrix,
                                                ClaySaveState          *state);
void         clay_savable_matrix3_save         (ClayMatrix3            *matrix,
                                                ClaySaveState          *state);
void         clay_savable_value_save           (GValue                 *value,
                                                GParamSpec             *pspec,
                                                ClaySaveState          *state);
void         clay_savable_value_array_save     (ClayValueArray         *values,
                                                ClaySaveState          *state);

void         clay_savable_composite_mode_save  (ClayLayerCompositeMode  composite_mode,
                                                ClayLayerMode           mode,
                                                ClaySaveState          *state);
void         clay_savable_composite_space_save (ClayLayerColorSpace     composite_space,
                                                ClayLayerMode           mode,
                                                ClaySaveState          *state);
void         clay_savable_blend_space_save     (ClayLayerColorSpace     blend_space,
                                                ClayLayerMode           mode,
                                                ClaySaveState          *state);
