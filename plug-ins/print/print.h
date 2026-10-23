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


#ifndef G_OS_WIN32
#define EMBED_PAGE_SETUP 1
#endif

#define CLAY_PLUGIN_PRINT_ERROR clay_plugin_print_error_quark ()

typedef enum
{
  CLAY_PLUGIN_PRINT_ERROR_FAILED
} ClayPluginPrintError;

GQuark clay_plugin_print_error_quark (void);

typedef enum
{
  CENTER_NONE         = 0,
  CENTER_HORIZONTALLY = 1,
  CENTER_VERTICALLY   = 2,
  CENTER_BOTH         = 3
} PrintCenterMode;

typedef struct
{
  ClayImage          *image;
  ClayDrawable       *drawable;
  ClayUnit           *unit;
  gdouble             xres;
  gdouble             yres;
  gdouble             min_xres;
  gdouble             min_yres;
  ClayUnit           *image_unit;
  gdouble             offset_x;
  gdouble             offset_y;
  PrintCenterMode     center;
  gboolean            use_full_page;
  gboolean            draw_crop_marks;
  GtkPrintOperation  *operation;
  GtkWidget          *layout_gui;
  gboolean            has_layout_gui;
} PrintData;
