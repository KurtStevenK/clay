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

#include "libclaywidgets/claywidgetstypes.h"

#include "core/core-types.h"

#include "widgets/widgets-enums.h"


/*  input devices & controllers  */

typedef struct _ClayControllerInfo           ClayControllerInfo;
typedef struct _ClayControllerKeyboard       ClayControllerKeyboard;
typedef struct _ClayControllerManager        ClayControllerManager;
typedef struct _ClayControllerMouse          ClayControllerMouse;
typedef struct _ClayControllerWheel          ClayControllerWheel;
typedef struct _ClayDeviceInfo               ClayDeviceInfo;
typedef struct _ClayDeviceManager            ClayDeviceManager;


/*  docks  */

typedef struct _ClayDock                     ClayDock;
typedef struct _ClayDockColumns              ClayDockColumns;
typedef struct _ClayDockContainer            ClayDockContainer; /* dummy typedef */
typedef struct _ClayDockWindow               ClayDockWindow;
typedef struct _ClayDockable                 ClayDockable;
typedef struct _ClayDockbook                 ClayDockbook;
typedef struct _ClayDocked                   ClayDocked; /* dummy typedef */
typedef struct _ClayMenuDock                 ClayMenuDock;
typedef struct _ClayPanedBox                 ClayPanedBox;
typedef struct _ClayToolbox                  ClayToolbox;


/*  ClayEditor widgets  */

typedef struct _ClayColorEditor              ClayColorEditor;
typedef struct _ClayDeviceStatus             ClayDeviceStatus;
typedef struct _ClayEditor                   ClayEditor;
typedef struct _ClayErrorConsole             ClayErrorConsole;
typedef struct _ClayToolOptionsEditor        ClayToolOptionsEditor;
typedef struct _ClayDashboard                ClayDashboard;


/*  ClayDataEditor widgets  */

typedef struct _ClayBrushEditor              ClayBrushEditor;
typedef struct _ClayDataEditor               ClayDataEditor;
typedef struct _ClayDynamicsEditor           ClayDynamicsEditor;
typedef struct _ClayGradientEditor           ClayGradientEditor;
typedef struct _ClayPaletteEditor            ClayPaletteEditor;
typedef struct _ClayToolPresetEditor         ClayToolPresetEditor;


/*  ClayImageEditor widgets  */

typedef struct _ClayColormapEditor           ClayColormapEditor;
typedef struct _ClayComponentEditor          ClayComponentEditor;
typedef struct _ClayHistogramEditor          ClayHistogramEditor;
typedef struct _ClayImageEditor              ClayImageEditor;
typedef struct _ClaySamplePointEditor        ClaySamplePointEditor;
typedef struct _ClaySelectionEditor          ClaySelectionEditor;
typedef struct _ClaySymmetryEditor           ClaySymmetryEditor;
typedef struct _ClayUndoEditor               ClayUndoEditor;


/*  ClayContainerView and its implementors  */

typedef struct _ClayChannelTreeView          ClayChannelTreeView;
typedef struct _ClayContainerBox             ClayContainerBox;
typedef struct _ClayContainerComboBox        ClayContainerComboBox;
typedef struct _ClayContainerEntry           ClayContainerEntry;
typedef struct _ClayContainerIconView        ClayContainerIconView;
typedef struct _ClayContainerListView        ClayContainerListView;
typedef struct _ClayContainerTreeStore       ClayContainerTreeStore;
typedef struct _ClayContainerTreeView        ClayContainerTreeView;
typedef struct _ClayContainerView            ClayContainerView; /* dummy typedef */
typedef struct _ClayDrawableTreeView         ClayDrawableTreeView;
typedef struct _ClayImageSaveView            ClayImageSaveView;
typedef struct _ClayItemTreeView             ClayItemTreeView;
typedef struct _ClayLayerTreeView            ClayLayerTreeView;
typedef struct _ClayPathTreeView             ClayPathTreeView;

typedef struct _ClayContainerPopup           ClayContainerPopup;
typedef struct _ClayViewableButton           ClayViewableButton;


/*  ClayContainerEditor widgets  */

typedef struct _ClayContainerEditor          ClayContainerEditor;
typedef struct _ClayBufferView               ClayBufferView;
typedef struct _ClayDocumentView             ClayDocumentView;
typedef struct _ClayFontView                 ClayFontView;
typedef struct _ClayImageView                ClayImageView;
typedef struct _ClayTemplateView             ClayTemplateView;
typedef struct _ClayToolEditor               ClayToolEditor;


/*  ClayDataFactoryView widgets  */

typedef struct _ClayBrushFactoryView         ClayBrushFactoryView;
typedef struct _ClayDataFactoryView          ClayDataFactoryView;
typedef struct _ClayDynamicsFactoryView      ClayDynamicsFactoryView;
typedef struct _ClayFontFactoryView          ClayFontFactoryView;
typedef struct _ClayPatternFactoryView       ClayPatternFactoryView;
typedef struct _ClayToolPresetFactoryView    ClayToolPresetFactoryView;


/*  menus  */

typedef struct _ClayAction                   ClayAction;
typedef struct _ClayActionImpl               ClayActionImpl;
typedef struct _ClayActionFactory            ClayActionFactory;
typedef struct _ClayActionGroup              ClayActionGroup;
typedef struct _ClayDoubleAction             ClayDoubleAction;
typedef struct _ClayEnumAction               ClayEnumAction;
typedef struct _ClayMenuFactory              ClayMenuFactory;
typedef struct _ClayProcedureAction          ClayProcedureAction;
typedef struct _ClayRadioAction              ClayRadioAction;
typedef struct _ClayStringAction             ClayStringAction;
typedef struct _ClayToggleAction             ClayToggleAction;
typedef struct _ClayUIManager                ClayUIManager;


/*  file dialogs  */

typedef struct _ClayExportDialog             ClayExportDialog;
typedef struct _ClayFileDialog               ClayFileDialog;
typedef struct _ClayOpenDialog               ClayOpenDialog;
typedef struct _ClaySaveDialog               ClaySaveDialog;


/*  misc dialogs  */

typedef struct _ClayColorDialog              ClayColorDialog;
typedef struct _ClayCriticalDialog           ClayCriticalDialog;
typedef struct _ClayErrorDialog              ClayErrorDialog;
typedef struct _ClayMessageDialog            ClayMessageDialog;
typedef struct _ClayProgressDialog           ClayProgressDialog;
typedef struct _ClayTextEditor               ClayTextEditor;
typedef struct _ClayViewableDialog           ClayViewableDialog;


/*  ClayPdbDialog widgets  */

typedef struct _ClayBrushSelect              ClayBrushSelect;
typedef struct _ClayFontSelect               ClayFontSelect;
typedef struct _ClayGradientSelect           ClayGradientSelect;
typedef struct _ClayImageSelect              ClayImageSelect;
typedef struct _ClayItemSelect               ClayItemSelect;
typedef struct _ClayPaletteSelect            ClayPaletteSelect;
typedef struct _ClayPatternSelect            ClayPatternSelect;
typedef struct _ClayPickableSelect           ClayPickableSelect;
typedef struct _ClayPdbDialog                ClayPdbDialog;


/*  misc widgets  */

typedef struct _ClayAccelLabel               ClayAccelLabel;
typedef struct _ClayActionEditor             ClayActionEditor;
typedef struct _ClayActionView               ClayActionView;
typedef struct _ClayBlobEditor               ClayBlobEditor;
typedef struct _ClayBufferSourceBox          ClayBufferSourceBox;
typedef struct _ClayCircle                   ClayCircle;
typedef struct _ClayColorBar                 ClayColorBar;
typedef struct _ClayColorDisplayEditor       ClayColorDisplayEditor;
typedef struct _ClayColorFrame               ClayColorFrame;
typedef struct _ClayColorHistory             ClayColorHistory;
typedef struct _ClayColormapSelection        ClayColormapSelection;
typedef struct _ClayColorPanel               ClayColorPanel;
typedef struct _ClayComboTagEntry            ClayComboTagEntry;
typedef struct _ClayCompressionComboBox      ClayCompressionComboBox;
typedef struct _ClayControllerEditor         ClayControllerEditor;
typedef struct _ClayControllerList           ClayControllerList;
typedef struct _ClayCurveView                ClayCurveView;
typedef struct _ClayDashEditor               ClayDashEditor;
typedef struct _ClayDeviceEditor             ClayDeviceEditor;
typedef struct _ClayDeviceInfoEditor         ClayDeviceInfoEditor;
typedef struct _ClayDial                     ClayDial;
typedef struct _ClayDynamicsOutputEditor     ClayDynamicsOutputEditor;
typedef struct _ClayExtensionDetails         ClayExtensionDetails;
typedef struct _ClayExtensionList            ClayExtensionList;
typedef struct _ClayFgBgEditor               ClayFgBgEditor;
typedef struct _ClayFgBgView                 ClayFgBgView;
typedef struct _ClayFileProcView             ClayFileProcView;
typedef struct _ClayFillEditor               ClayFillEditor;
typedef struct _ClayGridEditor               ClayGridEditor;
typedef struct _ClayHandleBar                ClayHandleBar;
typedef struct _ClayHighlightableButton      ClayHighlightableButton;
typedef struct _ClayHistogramBox             ClayHistogramBox;
typedef struct _ClayHistogramView            ClayHistogramView;
typedef struct _ClayIconPicker               ClayIconPicker;
typedef struct _ClayImageChooser             ClayImageChooser;
typedef struct _ClayImageCommentEditor       ClayImageCommentEditor;
typedef struct _ClayImageParasiteView        ClayImageParasiteView;
typedef struct _ClayImageProfileView         ClayImageProfileView;
typedef struct _ClayImagePropView            ClayImagePropView;
typedef struct _ClayItemChooser              ClayItemChooser;
typedef struct _ClayLanguageComboBox         ClayLanguageComboBox;
typedef struct _ClayLanguageEntry            ClayLanguageEntry;
typedef struct _ClayLanguageStore            ClayLanguageStore;
typedef struct _ClayLayerModeBox             ClayLayerModeBox;
typedef struct _ClayLayerModeComboBox        ClayLayerModeComboBox;
typedef struct _ClayMessageBox               ClayMessageBox;
typedef struct _ClayMenu                     ClayMenu;
typedef struct _ClayMenuBar                  ClayMenuBar;
typedef struct _ClayMenuModel                ClayMenuModel;
typedef struct _ClayMenuShell                ClayMenuShell;
typedef struct _ClayMeter                    ClayMeter;
typedef struct _ClayModifiersEditor          ClayModifiersEditor;
typedef struct _ClayOverlayBox               ClayOverlayBox;
typedef struct _ClayPickableButton           ClayPickableButton;
typedef struct _ClayPickableChooser          ClayPickableChooser;
typedef struct _ClayPickablePopup            ClayPickablePopup;
typedef struct _ClayPivotSelector            ClayPivotSelector;
typedef struct _ClayPlugInView               ClayPlugInView;
typedef struct _ClayPolar                    ClayPolar;
typedef struct _ClayPopup                    ClayPopup;
typedef struct _ClayPrefsBox                 ClayPrefsBox;
typedef struct _ClayProgressBox              ClayProgressBox;
typedef struct _ClayScaleButton              ClayScaleButton;
typedef struct _ClaySettingsBox              ClaySettingsBox;
typedef struct _ClaySettingsEditor           ClaySettingsEditor;
typedef struct _ClayShortcutButton           ClayShortcutButton;
typedef struct _ClaySizeBox                  ClaySizeBox;
typedef struct _ClayStrokeEditor             ClayStrokeEditor;
typedef struct _ClayTagEntry                 ClayTagEntry;
typedef struct _ClayTagPopup                 ClayTagPopup;
typedef struct _ClayTemplateEditor           ClayTemplateEditor;
typedef struct _ClayTextStyleEditor          ClayTextStyleEditor;
typedef struct _ClayThumbBox                 ClayThumbBox;
typedef struct _ClayToolbar                  ClayToolbar;
typedef struct _ClayToolButton               ClayToolButton;
typedef struct _ClayToolPalette              ClayToolPalette;
typedef struct _ClayTranslationStore         ClayTranslationStore;
typedef struct _ClayWindow                   ClayWindow;


/*  views  */

typedef struct _ClayNavigationView           ClayNavigationView;
typedef struct _ClayPaletteView              ClayPaletteView;
typedef struct _ClayView                     ClayView;


/*  view renderers  */

typedef struct _ClayViewRenderer             ClayViewRenderer;
typedef struct _ClayViewRendererBrush        ClayViewRendererBrush;
typedef struct _ClayViewRendererBuffer       ClayViewRendererBuffer;
typedef struct _ClayViewRendererDrawable     ClayViewRendererDrawable;
typedef struct _ClayViewRendererFont         ClayViewRendererFont;
typedef struct _ClayViewRendererGradient     ClayViewRendererGradient;
typedef struct _ClayViewRendererImage        ClayViewRendererImage;
typedef struct _ClayViewRendererImagefile    ClayViewRendererImagefile;
typedef struct _ClayViewRendererLayer        ClayViewRendererLayer;
typedef struct _ClayViewRendererPalette      ClayViewRendererPalette;
typedef struct _ClayViewRendererPath         ClayViewRendererPath;


/*  cell renderers  */

typedef struct _ClayCellRendererButton       ClayCellRendererButton;
typedef struct _ClayCellRendererDashes       ClayCellRendererDashes;
typedef struct _ClayCellRendererViewable     ClayCellRendererViewable;


/*  list rows  */

typedef struct _ClayRow                      ClayRow;


/*  misc objects  */

typedef struct _ClayDialogFactory            ClayDialogFactory;
typedef struct _ClayTextBuffer               ClayTextBuffer;
typedef struct _ClayUIConfigurer             ClayUIConfigurer;
typedef struct _ClayWindowStrategy           ClayWindowStrategy;


/*  session management objects and structs  */

typedef struct _ClaySessionInfo              ClaySessionInfo;
typedef struct _ClaySessionInfoAux           ClaySessionInfoAux;
typedef struct _ClaySessionInfoBook          ClaySessionInfoBook;
typedef struct _ClaySessionInfoDock          ClaySessionInfoDock;
typedef struct _ClaySessionInfoDockable      ClaySessionInfoDockable;
typedef struct _ClaySessionManaged           ClaySessionManaged;


/*  structs  */

typedef struct _ClayActionEntry              ClayActionEntry;
typedef struct _ClayDoubleActionEntry        ClayDoubleActionEntry;
typedef struct _ClayEnumActionEntry          ClayEnumActionEntry;
typedef struct _ClayProcedureActionEntry     ClayProcedureActionEntry;
typedef struct _ClayRadioActionEntry         ClayRadioActionEntry;
typedef struct _ClayStringActionEntry        ClayStringActionEntry;
typedef struct _ClayToggleActionEntry        ClayToggleActionEntry;

typedef struct _ClayDialogFactoryEntry       ClayDialogFactoryEntry;

typedef struct _ClayDashboardLogParams       ClayDashboardLogParams;


/*  function types  */

typedef GtkWidget * (* ClayDialogRestoreFunc)        (ClayDialogFactory *factory,
                                                      GdkMonitor        *monitor,
                                                      ClaySessionInfo   *info);
typedef void        (* ClayActionGroupSetupFunc)     (ClayActionGroup   *group);
typedef void        (* ClayActionGroupUpdateFunc)    (ClayActionGroup   *group,
                                                      gpointer           data);

typedef void        (* ClayUIManagerSetupFunc)       (ClayUIManager     *manager,
                                                      const gchar       *ui_path);

typedef void        (* ClayMenuPositionFunc)         (GtkMenu           *menu,
                                                      gint              *x,
                                                      gint              *y,
                                                      gpointer           data);
typedef gboolean    (* ClayPanedBoxDroppedFunc)      (GtkWidget         *notebook,
                                                      GtkWidget         *child,
                                                      gint               insert_index,
                                                      gpointer           data);

typedef GtkWidget * (* ClayToolOptionsGUIFunc)       (ClayToolOptions   *tool_options);
