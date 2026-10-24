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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"

#include "widgets/clayactiongroup.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claytoolbox.h"

#include "display/clayimagewindow.h"

#ifdef PLATFORM_OSX
#include "menus/menus.h"
#endif

#include "actions.h"
#include "dialogs-actions.h"
#include "dialogs-commands.h"

#include "clay-intl.h"


const ClayStringActionEntry dialogs_dockable_actions[] =
{
  { "dialogs-toolbox", NULL,
    NC_("windows-action", "Tool_box"), NULL, { "<primary>B", NULL },
    NULL /* set in dialogs_actions_update() */,
    "clay-toolbox",
    CLAY_HELP_TOOLBOX },

  { "dialogs-tool-options", CLAY_ICON_DIALOG_TOOL_OPTIONS,
    NC_("dialogs-action", "Tool _Options Dialog"),
    NC_("dialogs-action", "Tool _Options"),
    { NULL },
    NC_("dialogs-action", "Open the tool options dialog"),
    "clay-tool-options",
    CLAY_HELP_TOOL_OPTIONS_DIALOG },

  { "dialogs-device-status", CLAY_ICON_DIALOG_DEVICE_STATUS,
    NC_("dialogs-action", "_Device Status Dialog"),
    NC_("dialogs-action", "_Device Status"),
    { NULL },
    NC_("dialogs-action", "Open the device status dialog"),
    "clay-device-status",
    CLAY_HELP_DEVICE_STATUS_DIALOG },

  { "dialogs-symmetry", CLAY_ICON_SYMMETRY,
    NC_("dialogs-action", "_Symmetry Painting Dialog"),
    NC_("dialogs-action", "_Symmetry Painting"),
    { NULL },
    NC_("dialogs-action", "Open the symmetry dialog"),
    "clay-symmetry-editor",
    CLAY_HELP_SYMMETRY_DIALOG },

  { "dialogs-layers", CLAY_ICON_DIALOG_LAYERS,
    NC_("dialogs-action", "_Layers Dialog"),
    NC_("dialogs-action", "_Layers"),
    { "<primary>L", NULL },
    NC_("dialogs-action", "Open the layers dialog"),
    "clay-layer-list",
    CLAY_HELP_LAYER_DIALOG },

  { "dialogs-channels", CLAY_ICON_DIALOG_CHANNELS,
    NC_("dialogs-action", "_Channels Dialog"),
    NC_("dialogs-action", "_Channels"),
    { NULL },
    NC_("dialogs-action", "Open the channels dialog"),
    "clay-channel-list",
    CLAY_HELP_CHANNEL_DIALOG },

  { "dialogs-paths", CLAY_ICON_DIALOG_PATHS,
    NC_("dialogs-action", "_Paths Dialog"),
    NC_("dialogs-action", "_Paths"),
    { NULL },
    NC_("dialogs-action", "Open the paths dialog"),
    "clay-path-list",
    CLAY_HELP_PATH_DIALOG },

  { "dialogs-indexed-palette", CLAY_ICON_COLORMAP,
    NC_("dialogs-action", "Color_map Dialog"),
    NC_("dialogs-action", "Color_map"),
    { NULL },
    NC_("dialogs-action", "Open the colormap dialog"),
    "clay-indexed-palette",
    CLAY_HELP_INDEXED_PALETTE_DIALOG },

  { "dialogs-histogram", CLAY_ICON_HISTOGRAM,
    NC_("dialogs-action", "Histogra_m Dialog"),
    NC_("dialogs-action", "Histogra_m"),
    { NULL },
    NC_("dialogs-action", "Open the histogram dialog"),
    "clay-histogram-editor",
    CLAY_HELP_HISTOGRAM_DIALOG },

  { "dialogs-selection-editor", CLAY_ICON_SELECTION,
    NC_("dialogs-action", "_Selection Editor"), NULL,
    { NULL },
    NC_("dialogs-action", "Open the selection editor"),
    "clay-selection-editor",
    CLAY_HELP_SELECTION_DIALOG },

  { "dialogs-navigation", CLAY_ICON_DIALOG_NAVIGATION,
    NC_("dialogs-action", "Na_vigation Dialog"),
    NC_("dialogs-action", "Na_vigation"),
    { NULL },
    NC_("dialogs-action", "Open the display navigation dialog"),
    "clay-navigation-view",
    CLAY_HELP_NAVIGATION_DIALOG },

  { "dialogs-undo-history", CLAY_ICON_DIALOG_UNDO_HISTORY,
    NC_("dialogs-action", "Undo _History Dialog"),
    NC_("dialogs-action", "Undo _History"),
    { NULL },
    NC_("dialogs-action", "Open the undo history dialog"),
    "clay-undo-history",
    CLAY_HELP_UNDO_DIALOG },

  { "dialogs-cursor", CLAY_ICON_CURSOR,
    NC_("dialogs-action", "_Pointer Dialog"),
    NC_("dialogs-action", "_Pointer"),
    { NULL },
    NC_("dialogs-action", "Open the pointer information dialog"),
    "clay-cursor-view",
    CLAY_HELP_POINTER_INFO_DIALOG },

  { "dialogs-sample-points", CLAY_ICON_SAMPLE_POINT,
    NC_("dialogs-action", "_Sample Points Dialog"),
    NC_("dialogs-action", "_Sample Points"),
    { NULL },
    NC_("dialogs-action", "Open the sample points dialog"),
    "clay-sample-point-editor",
    CLAY_HELP_SAMPLE_POINT_DIALOG },

  { "dialogs-colors", CLAY_ICON_COLORS_DEFAULT,
    NC_("dialogs-action", "Colo_rs Dialog"),
    NC_("dialogs-action", "Colo_rs"),
    { NULL },
    NC_("dialogs-action", "Open the FG/BG color dialog"),
    "clay-color-editor",
    CLAY_HELP_COLOR_DIALOG },

  { "dialogs-brushes", CLAY_ICON_BRUSH,
    NC_("dialogs-action", "_Brushes Dialog"),
    NC_("dialogs-action", "_Brushes"),
    { "<primary><shift>B", NULL },
    NC_("dialogs-action", "Open the brushes dialog"),
    "clay-brush-grid|clay-brush-list",
    CLAY_HELP_BRUSH_DIALOG },

  { "dialogs-brush-editor", CLAY_ICON_BRUSH,
    NC_("dialogs-action", "Brush Editor"), NULL,
    { NULL },
    NC_("dialogs-action", "Open the brush editor"),
    "clay-brush-editor",
    CLAY_HELP_BRUSH_EDIT },

  { "dialogs-dynamics", CLAY_ICON_DYNAMICS,
    NC_("dialogs-action", "Paint D_ynamics Dialog"),
    NC_("dialogs-action", "Paint D_ynamics"),
    { NULL },
    NC_("dialogs-action", "Open paint dynamics dialog"),
    "clay-dynamics-list|clay-dynamics-grid",
    CLAY_HELP_DYNAMICS_DIALOG },

  { "dialogs-dynamics-editor", CLAY_ICON_DYNAMICS,
    NC_("dialogs-action", "Paint Dynamics Editor"), NULL,
    { NULL },
    NC_("dialogs-action", "Open the paint dynamics editor"),
    "clay-dynamics-editor",
    CLAY_HELP_DYNAMICS_EDITOR_DIALOG },

  { "dialogs-mypaint-brushes", CLAY_ICON_MYPAINT_BRUSH,
    NC_("dialogs-action", "_MyPaint Brushes Dialog"),
    NC_("dialogs-action", "_MyPaint Brushes"),
    { NULL },
    NC_("dialogs-action", "Open the mypaint brushes dialog"),
    "clay-mypaint-brush-grid|clay-mapyint-brush-list",
    CLAY_HELP_MYPAINT_BRUSH_DIALOG },

  { "dialogs-patterns", CLAY_ICON_PATTERN,
    NC_("dialogs-action", "P_atterns Dialog"),
    NC_("dialogs-action", "P_atterns"),
    { "<primary><shift>P", NULL },
    NC_("dialogs-action", "Open the patterns dialog"),
    "clay-pattern-grid|clay-pattern-list",
    CLAY_HELP_PATTERN_DIALOG },

  { "dialogs-gradients", CLAY_ICON_GRADIENT,
    NC_("dialogs-action", "_Gradients Dialog"),
    NC_("dialogs-action", "_Gradients"),
    { "<primary>G", NULL },
    NC_("dialogs-action", "Open the gradients dialog"),
    "clay-gradient-list|clay-gradient-grid",
    CLAY_HELP_GRADIENT_DIALOG },

  { "dialogs-gradient-editor", CLAY_ICON_GRADIENT,
    NC_("dialogs-action", "Gradient Editor"), NULL,
    { NULL },
    NC_("dialogs-action", "Open the gradient editor"),
    "clay-gradient-editor",
    CLAY_HELP_GRADIENT_EDIT },

  { "dialogs-palettes", CLAY_ICON_PALETTE,
    NC_("dialogs-action", "Pal_ettes Dialog"),
    NC_("dialogs-action", "Pal_ettes"),
    { NULL },
    NC_("dialogs-action", "Open the palettes dialog"),
    "clay-palette-list|clay-palette-grid",
    CLAY_HELP_PALETTE_DIALOG },

  { "dialogs-palette-editor", CLAY_ICON_PALETTE,
    NC_("dialogs-action", "Palette _Editor"),
    NC_("dialogs-action", "Palette _Editor"),
    { NULL },
    NC_("dialogs-action", "Open the palette editor"),
    "clay-palette-editor",
    CLAY_HELP_PALETTE_EDIT },

  { "dialogs-tool-presets", CLAY_ICON_TOOL_PRESET,
    NC_("dialogs-action", "Tool Pre_sets Dialog"),
    NC_("dialogs-action", "Tool Pre_sets"),
    { NULL },
    NC_("dialogs-action", "Open tool presets dialog"),
    "clay-tool-preset-list|clay-tool-preset-grid",
    CLAY_HELP_TOOL_PRESET_DIALOG },

  { "dialogs-fonts", CLAY_ICON_FONT,
    NC_("dialogs-action", "_Fonts Dialog"),
    NC_("dialogs-action", "_Fonts"),
    { NULL },
    NC_("dialogs-action", "Open the fonts dialog"),
    "clay-font-list|clay-font-grid",
    CLAY_HELP_FONT_DIALOG },

  { "dialogs-buffers", CLAY_ICON_BUFFER,
    NC_("dialogs-action", "B_uffers Dialog"),
    NC_("dialogs-action", "B_uffers"),
    { NULL },
    NC_("dialogs-action", "Open the named buffers dialog"),
    "clay-buffer-list|clay-buffer-grid",
    CLAY_HELP_BUFFER_DIALOG },

  { "dialogs-images", CLAY_ICON_DIALOG_IMAGES,
    NC_("dialogs-action", "_Images Dialog"),
    NC_("dialogs-action", "_Images"),
    { NULL },
    NC_("dialogs-action", "Open the images dialog"),
    "clay-image-list|clay-image-grid",
    CLAY_HELP_IMAGE_DIALOG },

  { "dialogs-document-history", CLAY_ICON_DOCUMENT_OPEN_RECENT,
    NC_("dialogs-action", "Document Histor_y Dialog"),
    NC_("dialogs-action", "Document Histor_y"),
    { NULL },
    NC_("dialogs-action", "Open the document history dialog"),
    "clay-document-list|clay-document-grid",
    CLAY_HELP_DOCUMENT_DIALOG },

  { "dialogs-templates", CLAY_ICON_TEMPLATE,
    NC_("dialogs-action", "_Templates Dialog"),
    NC_("dialogs-action", "_Templates"),
    { NULL },
    NC_("dialogs-action", "Open the image templates dialog"),
    "clay-template-list|clay-template-grid",
    CLAY_HELP_TEMPLATE_DIALOG },

  { "dialogs-error-console", CLAY_ICON_DIALOG_WARNING,
    NC_("dialogs-action", "Error Co_nsole"),
    NC_("dialogs-action", "Error Co_nsole"),
    { NULL },
    NC_("dialogs-action", "Open the error console"),
    "clay-error-console",
    CLAY_HELP_ERRORS_DIALOG },

  { "dialogs-dashboard", CLAY_ICON_DIALOG_DASHBOARD,
    NC_("dialogs-action", "_Dashboard"),
    NC_("dialogs-action", "_Dashboard"),
    { NULL },
    NC_("dialogs-action", "Open the dashboard"),
    "clay-dashboard",
    CLAY_HELP_ERRORS_DIALOG }
};

gint n_dialogs_dockable_actions = G_N_ELEMENTS (dialogs_dockable_actions);

static const ClayStringActionEntry dialogs_toplevel_actions[] =
{
  { "dialogs-preferences", CLAY_ICON_PREFERENCES_SYSTEM,
    NC_("dialogs-action", "_Preferences"),
    NC_("dialogs-action", "_Preferences"),
    { NULL },
    NC_("dialogs-action", "Open the preferences dialog"),
    "clay-preferences-dialog",
    CLAY_HELP_PREFS_DIALOG },

  { "dialogs-input-devices", CLAY_ICON_INPUT_DEVICE,
    NC_("dialogs-action", "_Input Devices Editor"),
    NC_("dialogs-action", "_Input Devices"),
    { NULL },
    NC_("dialogs-action", "Open the input devices editor"),
    "clay-input-devices-dialog",
    CLAY_HELP_INPUT_DEVICES },

  { "dialogs-keyboard-shortcuts", CLAY_ICON_CHAR_PICKER,
    NC_("dialogs-action", "_Keyboard Shortcuts Editor"),
    NC_("dialogs-action", "_Keyboard Shortcuts"),
    { NULL },
    NC_("dialogs-action", "Open the keyboard shortcuts editor"),
    "clay-keyboard-shortcuts-dialog",
    CLAY_HELP_KEYBOARD_SHORTCUTS },

  { "dialogs-module-dialog", CLAY_ICON_SYSTEM_RUN,
    NC_("dialogs-action", "_Modules Dialog"),
    NC_("dialogs-action", "_Modules"),
    { NULL },
    NC_("dialogs-action", "Open the module manager dialog"),
    "clay-module-dialog",
    CLAY_HELP_MODULE_DIALOG },

  { "dialogs-tips", CLAY_ICON_DIALOG_INFORMATION,
    NC_("dialogs-action", "_Tip of the Day"), NULL,
    { NULL },
    NC_("dialogs-action", "Show some helpful tips on using Clay"),
    "clay-tips-dialog",
    CLAY_HELP_TIPS_DIALOG },

  { "dialogs-welcome", CLAY_ICON_DIALOG_INFORMATION,
    NC_("dialogs-action", "Welcome Dialog"), NULL,
    { NULL },
    NC_("dialogs-action", "Show information on running Clay release"),
    "clay-welcome-dialog",
    CLAY_HELP_WELCOME_DIALOG },

  { "dialogs-about", CLAY_ICON_HELP_ABOUT,
#if defined(G_OS_WIN32)
    NC_("dialogs-action", "About Clay"),
#else /* UNIX: use GNOME HIG */
    NC_("dialogs-action", "_About"),
#endif
    NULL, { NULL },
    NC_("dialogs-action", "About Clay"),
    "clay-about-dialog",
      CLAY_HELP_ABOUT_DIALOG },

  { "dialogs-action-search", CLAY_ICON_TOOL_ZOOM,
    NC_("dialogs-action", "_Search and Run a Command"), NULL,
    { "slash", "KP_Divide", NULL },
    NC_("dialogs-action", "Search commands by keyword, and run them"),
    "clay-action-search-dialog",
    CLAY_HELP_ACTION_SEARCH_DIALOG },

  { "dialogs-extensions", CLAY_ICON_PLUGIN,
    NC_("dialogs-action", "Clay App Store (_DLC Shop)"), NULL,
    { NULL },
    NC_("dialogs-action", "Browse, search, and install Clay extensions, AI tools, and DLC packs."),
    "clay-extensions-dialog",
    CLAY_HELP_EXTENSIONS_DIALOG }
};

const ClayStringActionEntry language_actions[] =
{
  { "language-system", "clay-lang-system",
    NC_("language-action", "System"), NULL, { NULL },
    NC_("language-action", "Use the language of this computer"),
    "system", NULL },
  { "language-de", "clay-lang-de",
    "Deutsch", NULL, { NULL }, "Deutsch", "de", NULL },
  { "language-en", "clay-lang-en",
    "English", NULL, { NULL }, "English", "en", NULL },
  { "language-fr", "clay-lang-fr",
    "Français", NULL, { NULL }, "Français", "fr", NULL },
  { "language-es", "clay-lang-es",
    "Español", NULL, { NULL }, "Español", "es", NULL },
  { "language-it", "clay-lang-it",
    "Italiano", NULL, { NULL }, "Italiano", "it", NULL },
  { "language-pt", "clay-lang-pt",
    "Português", NULL, { NULL }, "Português", "pt", NULL },
  { "language-nl", "clay-lang-nl",
    "Nederlands", NULL, { NULL }, "Nederlands", "nl", NULL },
  { "language-pl", "clay-lang-pl",
    "Polski", NULL, { NULL }, "Polski", "pl", NULL },
  { "language-ru", "clay-lang-ru",
    "Русский", NULL, { NULL }, "Русский", "ru", NULL },
  { "language-zh-cn", "clay-lang-zh",
    "中文", NULL, { NULL }, "中文", "zh_CN", NULL },
  { "language-ja", "clay-lang-ja",
    "日本語", NULL, { NULL }, "日本語", "ja", NULL },
  { "language-ko", "clay-lang-ko",
    "한국어", NULL, { NULL }, "한국어", "ko", NULL },
  { "language-tr", "clay-lang-tr",
    "Türkçe", NULL, { NULL }, "Türkçe", "tr", NULL },
  { "language-ar", "clay-lang-ar",
    "العربية", NULL, { NULL }, "العربية", "ar", NULL },
  { "language-sv", "clay-lang-sv",
    "Svenska", NULL, { NULL }, "Svenska", "sv", NULL },
};


gboolean
dialogs_actions_toolbox_exists (Clay *clay)
{
  ClayDialogFactory *factory       = clay_dialog_factory_get_singleton ();
  gboolean           toolbox_found = FALSE;
  GList             *iter;

  /* First look in session managed windows */
  toolbox_found =
    clay_dialog_factory_find_widget (factory, "clay-toolbox-window") != NULL;

  /* Then in image windows */
  if (! toolbox_found)
    {
      GList *windows = clay ? clay_get_image_windows (clay) : NULL;

      for (iter = windows; iter; iter = g_list_next (iter))
        {
          ClayImageWindow *window = CLAY_IMAGE_WINDOW (windows->data);

          if (clay_image_window_has_toolbox (window))
            {
              toolbox_found = TRUE;
              break;
            }
        }

      g_list_free (windows);
    }

  return toolbox_found;
}

void
dialogs_actions_setup (ClayActionGroup *group)
{
  clay_language_flags_register ();

  clay_action_group_add_string_actions (group, "dialogs-action",
                                        dialogs_dockable_actions,
                                        G_N_ELEMENTS (dialogs_dockable_actions),
                                        dialogs_create_dockable_cmd_callback);

  clay_action_group_add_string_actions (group, "dialogs-action",
                                        dialogs_toplevel_actions,
                                        G_N_ELEMENTS (dialogs_toplevel_actions),
                                        dialogs_create_toplevel_cmd_callback);

  clay_action_group_add_string_actions (group, "language-action",
                                        language_actions,
                                        G_N_ELEMENTS (language_actions),
                                        dialogs_language_cmd_callback);
}

void
dialogs_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
  Clay        *clay            = action_data_get_clay (data);
  const gchar *toolbox_label   = NULL;
  const gchar *toolbox_tooltip = NULL;

  if (dialogs_actions_toolbox_exists (clay))
    {
      toolbox_label   = _("Tool_box");
      toolbox_tooltip = _("Raise the toolbox");
    }
  else
    {
      toolbox_label   = _("New Tool_box");
      toolbox_tooltip = _("Create a new toolbox");
    }

  clay_action_group_set_action_label (group, "dialogs-toolbox", toolbox_label);
  clay_action_group_set_action_tooltip (group, "dialogs-toolbox", toolbox_tooltip);

#ifdef PLATFORM_OSX
  /* match the wording of the macOS HIG. See: #15695 and preferences-dialog.c */
  {
    gchar *settings_label = menus_quartz_get_settings_label ();

    if (settings_label != NULL)
      {
        clay_action_group_set_action_label (group, "dialogs-preferences",
                                            settings_label);
        g_free (settings_label);
      }
  }
#endif
}
