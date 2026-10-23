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

#pragma once


#define CLAY_PALETTE_FILE_EXTENSION ".gpl"


typedef enum
{
  CLAY_PALETTE_FILE_FORMAT_UNKNOWN,
  CLAY_PALETTE_FILE_FORMAT_GPL,       /* Clay palette                        */
  CLAY_PALETTE_FILE_FORMAT_RIFF_PAL,  /* RIFF palette                        */
  CLAY_PALETTE_FILE_FORMAT_ACT,       /* Photoshop binary color palette      */
  CLAY_PALETTE_FILE_FORMAT_PSP_PAL,   /* JASC's Paint Shop Pro color palette */
  CLAY_PALETTE_FILE_FORMAT_ACO,       /* Photoshop ACO color file            */
  CLAY_PALETTE_FILE_FORMAT_ACB,       /* Photoshop ACB color book            */
  CLAY_PALETTE_FILE_FORMAT_ASE,       /* Photoshop ASE color palette         */
  CLAY_PALETTE_FILE_FORMAT_CSS,       /* Cascaded Stylesheet file (CSS)      */
  CLAY_PALETTE_FILE_FORMAT_SBZ,       /* Swatchbooker SBZ file               */
  CLAY_PALETTE_FILE_FORMAT_PROCREATE, /* Procreate .swatches file            */
  CLAY_PALETTE_FILE_FORMAT_KRITA      /* Krita KPL palette                   */
} ClayPaletteFileFormat;


GList               * clay_palette_load               (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_act           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_riff          (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_psp           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_aco           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_acb           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_ase           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_css           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_sbz           (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_procreate     (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * clay_palette_load_krita         (ClayContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);

ClayPaletteFileFormat clay_palette_load_detect_format (GFile         *file,
                                                       GInputStream  *input);
