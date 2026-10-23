/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claywidgetstypes.h
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

#ifndef __CLAY_WIDGETS_TYPES_H__
#define __CLAY_WIDGETS_TYPES_H__

#include <libclayconfig/clayconfigtypes.h>

#include <libclaywidgets/claywidgetsenums.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _ClayBrowser                   ClayBrowser;
typedef struct _ClayBusyBox                   ClayBusyBox;
typedef struct _ClayButton                    ClayButton;
typedef struct _ClayCellRendererColor         ClayCellRendererColor;
typedef struct _ClayCellRendererToggle        ClayCellRendererToggle;
typedef struct _ClayChainButton               ClayChainButton;
typedef struct _ClayColorArea                 ClayColorArea;
typedef struct _ClayColorButton               ClayColorButton;
typedef struct _ClayColorDisplay              ClayColorDisplay;
typedef struct _ClayColorDisplayStack         ClayColorDisplayStack;
typedef struct _ClayColorHexEntry             ClayColorHexEntry;
typedef struct _ClayColorNotebook             ClayColorNotebook;
typedef struct _ClayColorProfileChooserDialog ClayColorProfileChooserDialog;
typedef struct _ClayColorProfileComboBox      ClayColorProfileComboBox;
typedef struct _ClayColorProfileStore         ClayColorProfileStore;
typedef struct _ClayColorProfileView          ClayColorProfileView;
typedef struct _ClayColorScale                ClayColorScale;
typedef struct _ClayColorScaleEntry           ClayColorScaleEntry;
typedef struct _ClayColorScales               ClayColorScales;
typedef struct _ClayColorSelector             ClayColorSelector;
typedef struct _ClayColorSelect               ClayColorSelect;
typedef struct _ClayColorSelection            ClayColorSelection;
typedef struct _ClayController                ClayController;
typedef struct _ClayDialog                    ClayDialog;
typedef struct _ClayEnumStore                 ClayEnumStore;
typedef struct _ClayEnumComboBox              ClayEnumComboBox;
typedef struct _ClayEnumLabel                 ClayEnumLabel;
typedef struct _ClayFileChooser               ClayFileChooser;
typedef struct _ClayFileEntry                 ClayFileEntry;
typedef struct _ClayFrame                     ClayFrame;
typedef struct _ClayHintBox                   ClayHintBox;
typedef struct _ClayIntComboBox               ClayIntComboBox;
typedef struct _ClayIntRadioFrame             ClayIntRadioFrame;
typedef struct _ClayIntStore                  ClayIntStore;
typedef struct _ClayLabeled                   ClayLabeled;
typedef struct _ClayLabelColor                ClayLabelColor;
typedef struct _ClayLabelEntry                ClayLabelEntry;
typedef struct _ClayLabelSpin                 ClayLabelSpin;
typedef struct _ClayMemsizeEntry              ClayMemsizeEntry;
typedef struct _ClayNumberPairEntry           ClayNumberPairEntry;
typedef struct _ClayOffsetArea                ClayOffsetArea;
typedef struct _ClayPageSelector              ClayPageSelector;
typedef struct _ClayPathEditor                ClayPathEditor;
typedef struct _ClayPickButton                ClayPickButton;
typedef struct _ClayPreview                   ClayPreview;
typedef struct _ClayPreviewArea               ClayPreviewArea;
typedef struct _ClayRuler                     ClayRuler;
typedef struct _ClayScaleEntry                ClayScaleEntry;
typedef struct _ClayScrolledPreview           ClayScrolledPreview;
typedef struct _ClaySizeEntry                 ClaySizeEntry;
typedef struct _ClaySpinButton                ClaySpinButton;
typedef struct _ClayStringComboBox            ClayStringComboBox;
typedef struct _ClayUnitComboBox              ClayUnitComboBox;
typedef struct _ClayUnitStore                 ClayUnitStore;
typedef struct _ClayZoomModel                 ClayZoomModel;


/**
 * ClayHelpFunc:
 * @help_id:   the help ID
 * @help_data: the help user data
 *
 * This is the prototype for all functions you pass as @help_func to
 * the various Clay dialog constructors like clay_dialog_new(),
 * clay_query_int_box() etc.
 *
 * Help IDs are textual identifiers the help system uses to figure
 * which page to display.
 *
 * All these dialog constructors functions call clay_help_connect().
 *
 * In most cases it will be ok to use clay_standard_help_func() which
 * does nothing but passing the @help_id string to clay_help(). If
 * your plug-in needs some more sophisticated help handling you can
 * provide your own @help_func which has to call clay_help() to
 * actually display the help.
 **/
typedef void (* ClayHelpFunc) (const gchar *help_id,
                               gpointer     help_data);


G_END_DECLS

#endif /* __CLAY_WIDGETS_TYPES_H__ */
