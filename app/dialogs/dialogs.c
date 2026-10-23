/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * dialogs.c
 * Copyright (C) 2010 Martin Nordholts <martinn@src.gnome.org>
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
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "config/clayguiconfig.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claylist.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydockwindow.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymenufactory.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claysessioninfo-aux.h"
#include "widgets/claysessionmanaged.h"
#include "widgets/claytoolbox.h"

#include "dialogs.h"
#include "dialogs-constructors.h"

#include "clay-log.h"

#include "clay-intl.h"


ClayContainer *global_recent_docks = NULL;


#define FOREIGN(id, singleton, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    NULL                   /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    TRUE                   /* session_managed  */, \
    remember_size          /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define IMAGE_WINDOW(id, singleton, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    NULL                   /* new_func         */, \
    dialogs_restore_window /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    TRUE                   /* session_managed  */, \
    remember_size          /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    FALSE                  /* hideable         */, \
    TRUE                   /* image_window     */, \
    FALSE                  /* dockable         */}

#define TOPLEVEL(id, new_func, singleton, session_managed, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    session_managed        /* session_managed  */, \
    remember_size          /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define DOCKABLE(id, name, blurb, icon_name, help_id, new_func, view_size, singleton) \
  { id                     /* identifier       */, \
    name                   /* name             */, \
    blurb                  /* blurb            */, \
    icon_name              /* icon_name        */, \
    help_id                /* help_id          */, \
    new_func               /* new_func         */, \
    NULL                   /* restore_func     */, \
    view_size              /* view_size        */, \
    singleton              /* singleton        */, \
    FALSE                  /* session_managed  */, \
    FALSE                  /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    TRUE                   /* dockable         */}

#define DOCK(id, new_func) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    FALSE                  /* singleton        */, \
    FALSE                  /* session_managed  */, \
    FALSE                  /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define DOCK_WINDOW(id, new_func) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    FALSE                  /* singleton        */, \
    TRUE                   /* session_managed  */, \
    TRUE                   /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define LISTGRID(id, new_func, name, blurb, icon_name, help_id, view_size) \
  { "clay-"#id"-list"             /* identifier       */,  \
    name                          /* name             */,  \
    blurb                         /* blurb            */,  \
    icon_name                     /* icon_name        */,  \
    help_id                       /* help_id          */,  \
    dialogs_##new_func##_list_view_new /* new_func         */,  \
    NULL                          /* restore_func     */,  \
    view_size                     /* view_size        */,  \
    FALSE                         /* singleton        */,  \
    FALSE                         /* session_managed  */,  \
    FALSE                         /* remember_size    */,  \
    TRUE                          /* remember_if_open */,  \
    TRUE                          /* hideable         */,  \
    FALSE                         /* image_window     */,  \
    TRUE                          /* dockable         */}, \
  { "clay-"#id"-grid"             /* identifier       */,  \
    name                          /* name             */,  \
    blurb                         /* blurb            */,  \
    icon_name                     /* icon_name        */,  \
    help_id                       /* help_id          */,  \
    dialogs_##new_func##_grid_view_new /* new_func         */,  \
    NULL                          /* restore_func     */,  \
    view_size                     /* view_size        */,  \
    FALSE                         /* singleton        */,  \
    FALSE                         /* session_managed  */,  \
    FALSE                         /* remember_size    */,  \
    TRUE                          /* remember_if_open */,  \
    TRUE                          /* hideable         */,  \
    FALSE                         /* image_window     */,  \
    TRUE                          /* dockable         */}

#define LIST(id, new_func, name, blurb, icon_name, help_id, view_size) \
  { "clay-"#id"-list"                   /* identifier       */, \
    name                                /* name             */, \
    blurb                               /* blurb            */, \
    icon_name                            /* icon_name         */, \
    help_id                             /* help_id          */, \
    dialogs_##new_func##_list_view_new  /* new_func         */, \
    NULL                                /* restore_func     */, \
    view_size                           /* view_size        */, \
    FALSE                               /* singleton        */, \
    FALSE                               /* session_managed  */, \
    FALSE                               /* remember_size    */, \
    TRUE                                /* remember_if_open */, \
    TRUE                                /* hideable         */, \
    FALSE                               /* image_window     */, \
    TRUE                                /* dockable         */}


static GtkWidget * dialogs_restore_dialog (ClayDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           ClaySessionInfo   *info);
static GtkWidget * dialogs_restore_window (ClayDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           ClaySessionInfo   *info);


static const ClayDialogFactoryEntry entries[] =
{
  /*  foreign toplevels without constructor  */
  FOREIGN ("clay-brightness-contrast-tool-dialog", TRUE,  FALSE),
  FOREIGN ("clay-color-balance-tool-dialog",       TRUE,  FALSE),
  FOREIGN ("clay-color-picker-tool-dialog",        TRUE,  TRUE),
  FOREIGN ("clay-colorize-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("clay-crop-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("clay-curves-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("clay-desaturate-tool-dialog",          TRUE,  FALSE),
  FOREIGN ("clay-foreground-select-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("clay-gegl-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("clay-gradient-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("clay-hue-saturation-tool-dialog",      TRUE,  FALSE),
  FOREIGN ("clay-levels-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("clay-measure-tool-dialog",             TRUE,  FALSE),
  FOREIGN ("clay-offset-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("clay-operation-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("clay-posterize-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("clay-rotate-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("clay-scale-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("clay-shear-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("clay-text-tool-dialog",                TRUE,  TRUE),
  FOREIGN ("clay-threshold-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("clay-transform-3d-tool-dialog",        TRUE,  FALSE),
  FOREIGN ("clay-perspective-tool-dialog",         TRUE,  FALSE),
  FOREIGN ("clay-unified-transform-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("clay-handle-transform-tool-dialog",    TRUE,  FALSE),

  FOREIGN ("clay-toolbox-color-dialog",            TRUE,  FALSE),
  FOREIGN ("clay-gradient-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("clay-palette-editor-color-dialog",     TRUE,  FALSE),
  FOREIGN ("clay-colormap-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("clay-colormap-selection-color-dialog", TRUE,  FALSE),

  FOREIGN ("clay-controller-editor-dialog",        FALSE, TRUE),
  FOREIGN ("clay-controller-action-dialog",        FALSE, TRUE),
  FOREIGN ("clay-pad-action-dialog",               FALSE, TRUE),

  /*  ordinary toplevels  */
  TOPLEVEL ("clay-image-new-dialog",
            dialogs_image_new_new,          FALSE, TRUE, FALSE),
  TOPLEVEL ("clay-file-open-dialog",
            dialogs_file_open_new,          TRUE,  TRUE, TRUE),
  TOPLEVEL ("clay-file-open-location-dialog",
            dialogs_file_open_location_new, FALSE, TRUE, FALSE),
  TOPLEVEL ("clay-file-save-dialog",
            dialogs_file_save_new,          FALSE, TRUE, TRUE),
  TOPLEVEL ("clay-file-export-dialog",
            dialogs_file_export_new,        FALSE, TRUE, TRUE),

  /*  singleton toplevels  */
  TOPLEVEL ("clay-preferences-dialog",
            dialogs_preferences_get,        TRUE, TRUE,  TRUE),
  TOPLEVEL ("clay-input-devices-dialog",
            dialogs_input_devices_get,      TRUE, TRUE,  FALSE),
  TOPLEVEL ("clay-keyboard-shortcuts-dialog",
            dialogs_keyboard_shortcuts_get, TRUE, TRUE,  TRUE),
  TOPLEVEL ("clay-module-dialog",
            dialogs_module_get,             TRUE, TRUE,  TRUE),
  TOPLEVEL ("clay-palette-import-dialog",
            dialogs_palette_import_get,     TRUE, TRUE,  TRUE),
  TOPLEVEL ("clay-tips-dialog",
            dialogs_tips_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-welcome-dialog",
            dialogs_welcome_get,            TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-about-dialog",
            dialogs_about_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-action-search-dialog",
            dialogs_action_search_get,      TRUE, TRUE,  TRUE),
  TOPLEVEL ("clay-error-dialog",
            dialogs_error_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-critical-dialog",
            dialogs_critical_get,           TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-close-all-dialog",
            dialogs_close_all_get,          TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-quit-dialog",
            dialogs_quit_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("clay-extensions-dialog",
            dialogs_extensions_get,         TRUE, TRUE,  TRUE),

  /*  docks  */
  DOCK ("clay-dock",
        dialogs_dock_new),
  DOCK ("clay-toolbox",
        dialogs_toolbox_new),

  /*  dock windows  */
  DOCK_WINDOW ("clay-dock-window",
               dialogs_dock_window_new),
  DOCK_WINDOW ("clay-toolbox-window",
               dialogs_toolbox_dock_window_new),

  /*  singleton dockables  */
  DOCKABLE ("clay-tool-options",
            N_("Tool Options"), NULL, CLAY_ICON_DIALOG_TOOL_OPTIONS,
            CLAY_HELP_TOOL_OPTIONS_DIALOG,
            dialogs_tool_options_new, 0, TRUE),
  DOCKABLE ("clay-device-status",
            N_("Devices"), N_("Device Status"), CLAY_ICON_DIALOG_DEVICE_STATUS,
            CLAY_HELP_DEVICE_STATUS_DIALOG,
            dialogs_device_status_new, 0, TRUE),
  DOCKABLE ("clay-error-console",
            N_("Errors"), N_("Error Console"), CLAY_ICON_DIALOG_WARNING,
            CLAY_HELP_ERRORS_DIALOG,
            dialogs_error_console_new, 0, TRUE),
  DOCKABLE ("clay-cursor-view",
            N_("Pointer"), N_("Pointer Information"), CLAY_ICON_CURSOR,
            CLAY_HELP_POINTER_INFO_DIALOG,
            dialogs_cursor_view_new, 0, TRUE),
  DOCKABLE ("clay-dashboard",
            N_("Dashboard"), N_("Dashboard"), CLAY_ICON_DIALOG_DASHBOARD,
            CLAY_HELP_DASHBOARD_DIALOG,
            dialogs_dashboard_new, 0, TRUE),

  /*  list & grid views  */
  LISTGRID (image, image,
            N_("Images"), NULL, CLAY_ICON_DIALOG_IMAGES,
            CLAY_HELP_IMAGE_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (brush, brush,
            N_("Brushes"), NULL, CLAY_ICON_BRUSH,
            CLAY_HELP_BRUSH_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (dynamics, dynamics,
            N_("Paint Dynamics"), NULL, CLAY_ICON_DYNAMICS,
            CLAY_HELP_DYNAMICS_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (mypaint-brush, mypaint_brush,
            N_("MyPaint Brushes"), NULL, CLAY_ICON_MYPAINT_BRUSH,
            CLAY_HELP_MYPAINT_BRUSH_DIALOG, CLAY_VIEW_SIZE_LARGE),
  LISTGRID (pattern, pattern,
            N_("Patterns"), NULL, CLAY_ICON_PATTERN,
            CLAY_HELP_PATTERN_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (gradient, gradient,
            N_("Gradients"), NULL, CLAY_ICON_GRADIENT,
            CLAY_HELP_GRADIENT_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (palette, palette,
            N_("Palettes"), NULL, CLAY_ICON_PALETTE,
            CLAY_HELP_PALETTE_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (font, font,
            N_("Fonts"), NULL, CLAY_ICON_FONT,
            CLAY_HELP_FONT_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (buffer, buffer,
            N_("Buffers"), NULL, CLAY_ICON_BUFFER,
            CLAY_HELP_BUFFER_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (tool-preset, tool_preset,
            N_("Tool Presets"), NULL, CLAY_ICON_TOOL_PRESET,
            CLAY_HELP_TOOL_PRESET_DIALOG, CLAY_VIEW_SIZE_MEDIUM),
  LISTGRID (document, document,
            N_("History"), N_("Document History"), CLAY_ICON_DOCUMENT_OPEN_RECENT,
            CLAY_HELP_DOCUMENT_DIALOG, CLAY_VIEW_SIZE_LARGE),
  LISTGRID (template, template,
            N_("Templates"), N_("Image Templates"), CLAY_ICON_TEMPLATE,
            CLAY_HELP_TEMPLATE_DIALOG, CLAY_VIEW_SIZE_SMALL),

  /*  image related  */
  DOCKABLE ("clay-layer-list",
            N_("Layers"), NULL, CLAY_ICON_DIALOG_LAYERS,
            CLAY_HELP_LAYER_DIALOG,
            dialogs_layer_list_view_new, 0, FALSE),
  DOCKABLE ("clay-channel-list",
            N_("Channels"), NULL, CLAY_ICON_DIALOG_CHANNELS,
            CLAY_HELP_CHANNEL_DIALOG,
            dialogs_channel_list_view_new, 0, FALSE),
  DOCKABLE ("clay-path-list",
            N_("Paths"), NULL, CLAY_ICON_DIALOG_PATHS,
            CLAY_HELP_PATH_DIALOG,
            dialogs_path_list_view_new, 0, FALSE),
  DOCKABLE ("clay-indexed-palette",
            N_("Colormap"), NULL, CLAY_ICON_COLORMAP,
            CLAY_HELP_INDEXED_PALETTE_DIALOG,
            dialogs_colormap_editor_new, 0, FALSE),
  DOCKABLE ("clay-histogram-editor",
            N_("Histogram"), NULL, CLAY_ICON_HISTOGRAM,
            CLAY_HELP_HISTOGRAM_DIALOG,
            dialogs_histogram_editor_new, 0, FALSE),
  DOCKABLE ("clay-selection-editor",
            N_("Selection"), N_("Selection Editor"), CLAY_ICON_SELECTION,
            CLAY_HELP_SELECTION_DIALOG,
            dialogs_selection_editor_new, 0, FALSE),
  DOCKABLE ("clay-symmetry-editor",
            N_("Symmetry Painting"), NULL, CLAY_ICON_SYMMETRY,
            CLAY_HELP_SYMMETRY_DIALOG,
            dialogs_symmetry_editor_new, 0, FALSE),
  DOCKABLE ("clay-undo-history",
            N_("Undo"), N_("Undo History"), CLAY_ICON_DIALOG_UNDO_HISTORY,
            CLAY_HELP_UNDO_DIALOG,
            dialogs_undo_editor_new, 0, FALSE),
  DOCKABLE ("clay-sample-point-editor",
            N_("Sample Points"), N_("Sample Points"), CLAY_ICON_SAMPLE_POINT,
            CLAY_HELP_SAMPLE_POINT_DIALOG,
            dialogs_sample_point_editor_new, 0, FALSE),

  /*  display related  */
  DOCKABLE ("clay-navigation-view",
            N_("Navigation"), N_("Display Navigation"), CLAY_ICON_DIALOG_NAVIGATION,
            CLAY_HELP_NAVIGATION_DIALOG,
            dialogs_navigation_editor_new, 0, FALSE),

  /*  editors  */
  DOCKABLE ("clay-color-editor",
            N_("FG/BG"), N_("FG/BG Color"), CLAY_ICON_COLORS_DEFAULT,
            CLAY_HELP_COLOR_DIALOG,
            dialogs_color_editor_new, 0, FALSE),

  /*  singleton editors  */
  DOCKABLE ("clay-brush-editor",
            N_("Brush Editor"), NULL, CLAY_ICON_BRUSH,
            CLAY_HELP_BRUSH_EDITOR_DIALOG,
            dialogs_brush_editor_get, 0, TRUE),
  DOCKABLE ("clay-dynamics-editor",
            N_("Paint Dynamics Editor"), NULL, CLAY_ICON_DYNAMICS,
            CLAY_HELP_DYNAMICS_EDITOR_DIALOG,
            dialogs_dynamics_editor_get, 0, TRUE),
  DOCKABLE ("clay-gradient-editor",
            N_("Gradient Editor"), NULL, CLAY_ICON_GRADIENT,
            CLAY_HELP_GRADIENT_EDITOR_DIALOG,
            dialogs_gradient_editor_get, 0, TRUE),
  DOCKABLE ("clay-palette-editor",
            N_("Palette Editor"), NULL, CLAY_ICON_PALETTE,
            CLAY_HELP_PALETTE_EDITOR_DIALOG,
            dialogs_palette_editor_get, 0, TRUE),
  DOCKABLE ("clay-tool-preset-editor",
            N_("Tool Preset Editor"), NULL, CLAY_ICON_TOOL_PRESET,
            CLAY_HELP_TOOL_PRESET_EDITOR_DIALOG,
            dialogs_tool_preset_editor_get, 0, TRUE),

  /*  image windows  */
  IMAGE_WINDOW ("clay-empty-image-window",
                TRUE, TRUE),
  IMAGE_WINDOW ("clay-single-image-window",
                TRUE, TRUE)
};

/**
 * dialogs_restore_dialog:
 * @factory:
 * @screen:
 * @info:
 *
 * Creates a top level widget based on the given session info object
 * in which other widgets later can be be put, typically also restored
 * from the same session info object.
 *
 * Returns:
 **/
static GtkWidget *
dialogs_restore_dialog (ClayDialogFactory *factory,
                        GdkMonitor        *monitor,
                        ClaySessionInfo   *info)
{
  GtkWidget        *dialog;
  Clay             *clay    = clay_dialog_factory_get_context (factory)->clay;
  ClayCoreConfig   *config  = clay->config;
  ClayDisplay      *display = clay_context_get_display (clay_get_user_context (clay));
  ClayDisplayShell *shell   = clay_display_get_shell (display);

  CLAY_LOG (DIALOG_FACTORY, "restoring toplevel \"%s\" (info %p)",
            clay_session_info_get_factory_entry (info)->identifier,
            info);

  dialog =
    clay_dialog_factory_dialog_new (factory, monitor,
                                    NULL /*ui_manager*/,
                                    GTK_WIDGET (clay_display_shell_get_window (shell)),
                                    clay_session_info_get_factory_entry (info)->identifier,
                                    clay_session_info_get_factory_entry (info)->view_size,
                                    ! CLAY_GUI_CONFIG (config)->hide_docks);

  g_object_set_data (G_OBJECT (dialog), CLAY_DIALOG_VISIBILITY_KEY,
                     GINT_TO_POINTER (CLAY_GUI_CONFIG (config)->hide_docks ?
                                      CLAY_DIALOG_VISIBILITY_HIDDEN :
                                      CLAY_DIALOG_VISIBILITY_VISIBLE));

  return dialog;
}

/**
 * dialogs_restore_window:
 * @factory:
 * @monitor:
 * @info:
 *
 * "restores" the image window. We don't really restore anything since
 * the image window is created earlier, so we just look for and return
 * the already-created image window.
 *
 * Returns:
 **/
static GtkWidget *
dialogs_restore_window (ClayDialogFactory *factory,
                        GdkMonitor        *monitor,
                        ClaySessionInfo   *info)
{
  Clay             *clay    = clay_dialog_factory_get_context (factory)->clay;
  ClayDisplay      *display = CLAY_DISPLAY (clay_get_empty_display (clay));
  ClayDisplayShell *shell   = clay_display_get_shell (display);
  GtkWidget        *dialog;

  dialog = GTK_WIDGET (clay_display_shell_get_window (shell));

  return dialog;
}


/*  public functions  */

void
dialogs_init (Clay *clay)
{
  ClayDialogFactory *factory = NULL;
  gint               i       = 0;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  factory = clay_dialog_factory_new ("toplevel", clay_get_user_context (clay));
  clay_dialog_factory_set_singleton (factory);

  for (i = 0; i < G_N_ELEMENTS (entries); i++)
    clay_dialog_factory_register_entry (factory,
                                        entries[i].identifier,
                                        entries[i].name ? gettext(entries[i].name) : NULL,
                                        entries[i].blurb ? gettext(entries[i].blurb) : NULL,
                                        entries[i].icon_name,
                                        entries[i].help_id,
                                        entries[i].new_func,
                                        entries[i].restore_func,
                                        entries[i].view_size,
                                        entries[i].singleton,
                                        entries[i].session_managed,
                                        entries[i].remember_size,
                                        entries[i].remember_if_open,
                                        entries[i].hideable,
                                        entries[i].image_window,
                                        entries[i].dockable);

  global_recent_docks = clay_list_new (CLAY_TYPE_SESSION_INFO, FALSE);
}

void
dialogs_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay_dialog_factory_get_singleton ())
    {
      /* run dispose manually so the factory destroys its dialogs, which
       * might in turn directly or indirectly ref the factory
       */
      g_object_run_dispose (G_OBJECT (clay_dialog_factory_get_singleton ()));

      g_object_unref (clay_dialog_factory_get_singleton ());
      clay_dialog_factory_set_singleton (NULL);
    }

  g_clear_object (&global_recent_docks);
}

static void
dialogs_ensure_factory_entry_on_recent_dock (ClaySessionInfo *info)
{
  if (! clay_session_info_get_factory_entry (info))
    {
      ClayDialogFactoryEntry *entry = NULL;

      /* The recent docks container only contains session infos for
       * dock windows
       */
      entry = clay_dialog_factory_find_entry (clay_dialog_factory_get_singleton (),
                                              "clay-dock-window");

      clay_session_info_set_factory_entry (info, entry);
    }
}

static GFile *
dialogs_get_dockrc_file (void)
{
  const gchar *basename;

  basename = g_getenv ("CLAY_TESTING_DOCKRC_NAME");
  if (! basename)
    basename = "dockrc";

  return clay_directory_file (basename, NULL);
}

void
dialogs_load_recent_docks (Clay *clay)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = dialogs_get_dockrc_file ();

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_deserialize_file (CLAY_CONFIG (global_recent_docks),
                                      file,
                                      NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);

  /* In Clay 2.6 dockrc did not contain the factory entries for the
   * session infos, so set that up manually if needed
   */
  clay_container_foreach (global_recent_docks,
                          (GFunc) dialogs_ensure_factory_entry_on_recent_dock,
                          NULL);

  clay_list_reverse (CLAY_LIST (global_recent_docks));
}

void
dialogs_save_recent_docks (Clay *clay)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = dialogs_get_dockrc_file ();

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (global_recent_docks),
                                       file,
                                       "recently closed docks",
                                       "end of recently closed docks",
                                       NULL, &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

GtkWidget *
dialogs_get_toolbox (void)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (clay_dialog_factory_get_singleton ()), NULL);

  for (list = clay_dialog_factory_get_open_dialogs (clay_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      if (CLAY_IS_DOCK_WINDOW (list->data) &&
          clay_dock_window_has_toolbox (list->data))
        return list->data;
    }

  return NULL;
}

GtkWidget *
dialogs_get_dialog (GObject     *attach_object,
                    const gchar *attach_key)
{
  g_return_val_if_fail (G_IS_OBJECT (attach_object), NULL);
  g_return_val_if_fail (attach_key != NULL, NULL);

  return g_object_get_data (attach_object, attach_key);
}

void
dialogs_attach_dialog (GObject     *attach_object,
                       const gchar *attach_key,
                       GtkWidget   *dialog)
{
  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  g_object_set_data (attach_object, attach_key, dialog);
  g_object_set_data (G_OBJECT (dialog), "clay-dialogs-attach-key",
                     (gpointer) attach_key);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (dialogs_detach_dialog),
                           attach_object,
                           G_CONNECT_SWAPPED);
}

void
dialogs_detach_dialog (GObject   *attach_object,
                       GtkWidget *dialog)
{
  const gchar *attach_key;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  attach_key = g_object_get_data (G_OBJECT (dialog),
                                  "clay-dialogs-attach-key");

  g_return_if_fail (attach_key != NULL);

  g_object_set_data (attach_object, attach_key, NULL);

  g_signal_handlers_disconnect_by_func (dialog,
                                        dialogs_detach_dialog,
                                        attach_object);
}

void
dialogs_destroy_dialog (GObject     *attach_object,
                        const gchar *attach_key)
{
  GtkWidget *dialog;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);

  dialog = g_object_get_data (attach_object, attach_key);

  if (dialog)
    gtk_widget_destroy (dialog);
}

GtkNativeDialog *
dialogs_get_native_dialog (GObject     *attach_object,
                           const gchar *attach_key)
{
  g_return_val_if_fail (G_IS_OBJECT (attach_object), NULL);
  g_return_val_if_fail (attach_key != NULL, NULL);

  return g_object_get_data (attach_object, attach_key);
}

void
dialogs_attach_native_dialog (GObject         *attach_object,
                              const gchar     *attach_key,
                              GtkNativeDialog *dialog)
{
  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);
  g_return_if_fail (GTK_IS_NATIVE_DIALOG (dialog));

  g_object_set_data (attach_object, attach_key, dialog);
  g_object_set_data (G_OBJECT (dialog), "clay-dialogs-attach-key",
                     (gpointer) attach_key);
}

void
dialogs_detach_native_dialog (GObject         *attach_object,
                              GtkNativeDialog *dialog)
{
  const gchar *attach_key;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (GTK_IS_NATIVE_DIALOG (dialog));

  attach_key = g_object_get_data (G_OBJECT (dialog),
                                  "clay-dialogs-attach-key");

  g_return_if_fail (attach_key != NULL);

  g_object_set_data (attach_object, attach_key, NULL);

  g_signal_handlers_disconnect_by_func (dialog,
                                        dialogs_detach_dialog,
                                        attach_object);
}

void
dialogs_destroy_native_dialog (GObject     *attach_object,
                               const gchar *attach_key)
{
  GtkNativeDialog *dialog;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);

  dialog = g_object_get_data (attach_object, attach_key);

  if (dialog)
    gtk_native_dialog_destroy (dialog);
}
