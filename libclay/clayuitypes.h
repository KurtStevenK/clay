/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayuitypes.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_UI_TYPES_H__
#define __CLAY_UI_TYPES_H__

#include <libclaywidgets/claywidgetstypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _ClayProcedureDialog           ClayProcedureDialog;
typedef struct _ClayExportProcedureDialog     ClayExportProcedureDialog;
typedef struct _ClayVectorLoadProcedureDialog ClayVectorLoadProcedureDialog;

typedef struct _ClayAspectPreview             ClayAspectPreview;
typedef struct _ClayDrawablePreview           ClayDrawablePreview;
typedef struct _ClayProcBrowserDialog         ClayProcBrowserDialog;
typedef struct _ClayProgressBar               ClayProgressBar;
typedef struct _ClayZoomPreview               ClayZoomPreview;

typedef struct _ClayDrawableComboBox          ClayDrawableComboBox;
typedef struct _ClayChannelComboBox           ClayChannelComboBox;
typedef struct _ClayLayerComboBox             ClayLayerComboBox;
typedef struct _ClayPathComboBox              ClayPathComboBox;
typedef struct _ClayImageComboBox             ClayImageComboBox;

typedef struct _ClayDrawableChooser           ClayDrawableChooser;

typedef struct _ClayResourceChooser           ClayResourceChooser;
typedef struct _ClayBrushChooser              ClayBrushChooser;
typedef struct _ClayFontChooser               ClayFontChooser;
typedef struct _ClayGradientChooser           ClayGradientChooser;
typedef struct _ClayImageChooser              ClayImageChooser;
typedef struct _ClayItemChooser               ClayItemChooser;
typedef struct _ClayPaletteChooser            ClayPaletteChooser;
typedef struct _ClayPatternChooser            ClayPatternChooser;


G_END_DECLS

#endif /* __CLAY_UI_TYPES_H__ */
