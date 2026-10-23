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

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "config/clayguiconfig.h"

#include "menus/menus.h"

#include "widgets/claybrusheditor.h"
#include "widgets/claybrushfactoryview.h"
#include "widgets/claybufferview.h"
#include "widgets/claychanneltreeview.h"
#include "widgets/claycoloreditor.h"
#include "widgets/claycolormapeditor.h"
#include "widgets/claycriticaldialog.h"
#include "widgets/claydashboard.h"
#include "widgets/claydevicestatus.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydockwindow.h"
#include "widgets/claydocumentview.h"
#include "widgets/claydynamicseditor.h"
#include "widgets/claydynamicsfactoryview.h"
#include "widgets/clayerrorconsole.h"
#include "widgets/clayerrordialog.h"
#include "widgets/clayfontfactoryview.h"
#include "widgets/claygradienteditor.h"
#include "widgets/clayhistogrameditor.h"
#include "widgets/clayimageview.h"
#include "widgets/claylayertreeview.h"
#include "widgets/claymenudock.h"
#include "widgets/claypaletteeditor.h"
#include "widgets/claypatternfactoryview.h"
#include "widgets/claysamplepointeditor.h"
#include "widgets/clayselectioneditor.h"
#include "widgets/claysymmetryeditor.h"
#include "widgets/claytemplateview.h"
#include "widgets/claytoolbox.h"
#include "widgets/claytooloptionseditor.h"
#include "widgets/claytoolpresetfactoryview.h"
#include "widgets/claytoolpreseteditor.h"
#include "widgets/clayundoeditor.h"
#include "widgets/claypathtreeview.h"

#include "display/claycursorview.h"
#include "display/claynavigationeditor.h"

#include "about-dialog.h"
#include "action-search-dialog.h"
#include "dialogs.h"
#include "dialogs-constructors.h"
#include "extensions-dialog.h"
#include "file-open-dialog.h"
#include "file-open-location-dialog.h"
#include "file-save-dialog.h"
#include "image-new-dialog.h"
#include "input-devices-dialog.h"
#include "keyboard-shortcuts-dialog.h"
#include "module-dialog.h"
#include "palette-import-dialog.h"
#include "preferences-dialog.h"
#include "quit-dialog.h"
#include "tips-dialog.h"
#include "welcome-dialog.h"

#include "clay-intl.h"


/**********************/
/*  toplevel dialogs  */
/**********************/

GtkWidget *
dialogs_image_new_new (ClayDialogFactory *factory,
                       ClayContext       *context,
                       ClayUIManager     *ui_manager,
                       gint               view_size)
{
  return image_new_dialog_new (context);
}

GtkWidget *
dialogs_file_open_new (ClayDialogFactory *factory,
                       ClayContext       *context,
                       ClayUIManager     *ui_manager,
                       gint               view_size)
{
  return file_open_dialog_new (context->clay);
}

GtkWidget *
dialogs_file_open_location_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return file_open_location_dialog_new (context->clay);
}

GtkWidget *
dialogs_file_save_new (ClayDialogFactory *factory,
                       ClayContext       *context,
                       ClayUIManager     *ui_manager,
                       gint               view_size)
{
  return file_save_dialog_new (context->clay, FALSE);
}

GtkWidget *
dialogs_file_export_new (ClayDialogFactory *factory,
                         ClayContext       *context,
                         ClayUIManager     *ui_manager,
                         gint               view_size)
{
  return file_save_dialog_new (context->clay, TRUE);
}

GtkWidget *
dialogs_preferences_get (ClayDialogFactory *factory,
                         ClayContext       *context,
                         ClayUIManager     *ui_manager,
                         gint               view_size)
{
  return preferences_dialog_create (context->clay);
}

GtkWidget *
dialogs_extensions_get (ClayDialogFactory *factory,
                        ClayContext       *context,
                        ClayUIManager     *ui_manager,
                        gint               view_size)
{
  return extensions_dialog_new (context->clay);
}

GtkWidget *
dialogs_keyboard_shortcuts_get (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return keyboard_shortcuts_dialog_new (context->clay);
}

GtkWidget *
dialogs_input_devices_get (ClayDialogFactory *factory,
                           ClayContext       *context,
                           ClayUIManager     *ui_manager,
                           gint               view_size)
{
  return input_devices_dialog_new (context->clay);
}

GtkWidget *
dialogs_module_get (ClayDialogFactory *factory,
                    ClayContext       *context,
                    ClayUIManager     *ui_manager,
                    gint               view_size)
{
  return module_dialog_new (context->clay);
}

GtkWidget *
dialogs_palette_import_get (ClayDialogFactory *factory,
                            ClayContext       *context,
                            ClayUIManager     *ui_manager,
                            gint               view_size)
{
  return palette_import_dialog_new (context);
}

GtkWidget *
dialogs_tips_get (ClayDialogFactory *factory,
                  ClayContext       *context,
                  ClayUIManager     *ui_manager,
                  gint               view_size)
{
  return tips_dialog_create (context->clay);
}

GtkWidget *
dialogs_welcome_get (ClayDialogFactory *factory,
                     ClayContext       *context,
                     ClayUIManager     *ui_manager,
                     gint               view_size)
{
  return welcome_dialog_create (context->clay, TRUE);
}

GtkWidget *
dialogs_about_get (ClayDialogFactory *factory,
                   ClayContext       *context,
                   ClayUIManager     *ui_manager,
                   gint               view_size)
{
  return about_dialog_create (context->clay, context->clay->edit_config);
}

GtkWidget *
dialogs_action_search_get (ClayDialogFactory *factory,
                           ClayContext       *context,
                           ClayUIManager     *ui_manager,
                           gint               view_size)
{
  return action_search_dialog_create (context->clay);
}

GtkWidget *
dialogs_error_get (ClayDialogFactory *factory,
                   ClayContext       *context,
                   ClayUIManager     *ui_manager,
                   gint               view_size)
{
  return clay_error_dialog_new (_("Clay Message"));
}

GtkWidget *
dialogs_critical_get (ClayDialogFactory *factory,
                      ClayContext       *context,
                      ClayUIManager     *ui_manager,
                      gint               view_size)
{
  return clay_critical_dialog_new (_("Clay Debug"),
                                   context->clay->config->last_known_release,
                                   context->clay->config->last_release_timestamp);
}

GtkWidget *
dialogs_close_all_get (ClayDialogFactory *factory,
                       ClayContext       *context,
                       ClayUIManager     *ui_manager,
                       gint               view_size)
{
  return close_all_dialog_new (context->clay);
}

GtkWidget *
dialogs_quit_get (ClayDialogFactory *factory,
                  ClayContext       *context,
                  ClayUIManager     *ui_manager,
                  gint               view_size)
{
  return quit_dialog_new (context->clay);
}


/***********/
/*  docks  */
/***********/

GtkWidget *
dialogs_toolbox_new (ClayDialogFactory *factory,
                     ClayContext       *context,
                     ClayUIManager     *ui_manager,
                     gint               view_size)
{
  return clay_toolbox_new (factory,
                           context,
                           ui_manager);
}

GtkWidget *
dialogs_toolbox_dock_window_new (ClayDialogFactory *factory,
                                 ClayContext       *context,
                                 ClayUIManager     *ui_manager,
                                 gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("clay-toolbox-%d", role_serial++);
  dock = clay_dock_window_new (role,
                               "<Toolbox>",
                               TRUE,
                               factory,
                               context);
  g_free (role);

  return dock;
}

GtkWidget *
dialogs_dock_new (ClayDialogFactory *factory,
                  ClayContext       *context,
                  ClayUIManager     *ui_manager,
                  gint               view_size)
{
  return clay_menu_dock_new ();
}

GtkWidget *
dialogs_dock_window_new (ClayDialogFactory *factory,
                         ClayContext       *context,
                         ClayUIManager     *ui_manager,
                         gint               view_size)
{
  static gint  role_serial = 1;
  GtkWidget   *dock;
  gchar       *role;

  role = g_strdup_printf ("clay-dock-%d", role_serial++);
  dock = clay_dock_window_new (role,
                               "<Dock>",
                               FALSE,
                               factory,
                               context);
  g_free (role);

  return dock;
}


/***************/
/*  dockables  */
/***************/

/*****  singleton dialogs  *****/

GtkWidget *
dialogs_tool_options_new (ClayDialogFactory *factory,
                          ClayContext       *context,
                          ClayUIManager     *ui_manager,
                          gint               view_size)
{
  return clay_tool_options_editor_new (context->clay,
                                       menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_device_status_new (ClayDialogFactory *factory,
                           ClayContext       *context,
                           ClayUIManager     *ui_manager,
                           gint               view_size)
{
  return clay_device_status_new (context->clay);
}

GtkWidget *
dialogs_error_console_new (ClayDialogFactory *factory,
                           ClayContext       *context,
                           ClayUIManager     *ui_manager,
                           gint               view_size)
{
  return clay_error_console_new (context->clay,
                                 menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_cursor_view_new (ClayDialogFactory *factory,
                         ClayContext       *context,
                         ClayUIManager     *ui_manager,
                         gint               view_size)
{
  return clay_cursor_view_new (context->clay,
                               menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_dashboard_new (ClayDialogFactory *factory,
                       ClayContext       *context,
                       ClayUIManager     *ui_manager,
                       gint               view_size)
{
  return clay_dashboard_new (context->clay,
                             menus_get_global_menu_factory (context->clay));
}


/*****  list views  *****/

GtkWidget *
dialogs_image_list_view_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_image_view_new (CLAY_VIEW_TYPE_LIST,
                              context->clay->images,
                              context,
                              view_size, 1,
                              menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_brush_list_view_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_brush_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                      context->clay->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_dynamics_list_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_dynamics_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                         context->clay->dynamics_factory,
                                         context,
                                         view_size, 0,
                                         menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_mypaint_brush_list_view_new (ClayDialogFactory *factory,
                                     ClayContext       *context,
                                     ClayUIManager     *ui_manager,
                                     gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                     context->clay->mybrush_factory,
                                     context,
                                     view_size, 0,
                                     menus_get_global_menu_factory (context->clay),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_list_view_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  return clay_pattern_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                        context->clay->pattern_factory,
                                        context,
                                        view_size, 1,
                                        menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_gradient_list_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                     context->clay->gradient_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_list_view_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                     context->clay->palette_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_list_view_new (ClayDialogFactory *factory,
                            ClayContext       *context,
                            ClayUIManager     *ui_manager,
                            gint               view_size)
{
  return clay_font_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                     context->clay->font_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_buffer_list_view_new (ClayDialogFactory *factory,
                              ClayContext       *context,
                              ClayUIManager     *ui_manager,
                              gint               view_size)
{
  return clay_buffer_view_new (CLAY_VIEW_TYPE_LIST,
                               context->clay->named_buffers,
                               context,
                               view_size, 1,
                               menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_tool_preset_list_view_new (ClayDialogFactory *factory,
                                   ClayContext       *context,
                                   ClayUIManager     *ui_manager,
                                   gint               view_size)
{
  return clay_tool_preset_factory_view_new (CLAY_VIEW_TYPE_LIST,
                                            context->clay->tool_preset_factory,
                                            context,
                                            view_size, 0,
                                            menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_document_list_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_document_view_new (CLAY_VIEW_TYPE_LIST,
                                 context->clay->documents,
                                 context,
                                 view_size, 0,
                                 menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_template_list_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_template_view_new (CLAY_VIEW_TYPE_LIST,
                                 context->clay->templates,
                                 context,
                                 view_size, 0,
                                 menus_get_global_menu_factory (context->clay));
}


/*****  grid views  *****/

GtkWidget *
dialogs_image_grid_view_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_image_view_new (CLAY_VIEW_TYPE_GRID,
                              context->clay->images,
                              context,
                              view_size, 1,
                              menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_brush_grid_view_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_brush_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                      context->clay->brush_factory,
                                      context,
                                      TRUE,
                                      view_size, 1,
                                      menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_dynamics_grid_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_dynamics_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                         context->clay->dynamics_factory,
                                         context,
                                         view_size, 1,
                                         menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_mypaint_brush_grid_view_new (ClayDialogFactory *factory,
                                     ClayContext       *context,
                                     ClayUIManager     *ui_manager,
                                     gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                     context->clay->mybrush_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay),
                                     "<MyPaintBrushes>",
                                     "/mypaint-brushes-popup",
                                     "mypaint-brushes");
}

GtkWidget *
dialogs_pattern_grid_view_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  return clay_pattern_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                        context->clay->pattern_factory,
                                        context,
                                        view_size, 1,
                                        menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_gradient_grid_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                     context->clay->gradient_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay),
                                     "<Gradients>",
                                     "/gradients-popup",
                                     "gradients");
}

GtkWidget *
dialogs_palette_grid_view_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  return clay_data_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                     context->clay->palette_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay),
                                     "<Palettes>",
                                     "/palettes-popup",
                                     "palettes");
}

GtkWidget *
dialogs_font_grid_view_new (ClayDialogFactory *factory,
                            ClayContext       *context,
                            ClayUIManager     *ui_manager,
                            gint               view_size)
{
  return clay_font_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                     context->clay->font_factory,
                                     context,
                                     view_size, 1,
                                     menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_buffer_grid_view_new (ClayDialogFactory *factory,
                              ClayContext       *context,
                              ClayUIManager     *ui_manager,
                              gint               view_size)
{
  return clay_buffer_view_new (CLAY_VIEW_TYPE_GRID,
                               context->clay->named_buffers,
                               context,
                               view_size, 1,
                               menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_tool_preset_grid_view_new (ClayDialogFactory *factory,
                                   ClayContext       *context,
                                   ClayUIManager     *ui_manager,
                                   gint               view_size)
{
  return clay_tool_preset_factory_view_new (CLAY_VIEW_TYPE_GRID,
                                            context->clay->tool_preset_factory,
                                            context,
                                            view_size, 1,
                                            menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_document_grid_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_document_view_new (CLAY_VIEW_TYPE_GRID,
                                 context->clay->documents,
                                 context,
                                 view_size, 0,
                                 menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_template_grid_view_new (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_template_view_new (CLAY_VIEW_TYPE_GRID,
                                 context->clay->templates,
                                 context,
                                 view_size, 0,
                                 menus_get_global_menu_factory (context->clay));
}


/*****  image related dialogs  *****/

GtkWidget *
dialogs_layer_list_view_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  if (view_size < 1)
    view_size = context->clay->config->layer_preview_size;

  return clay_item_tree_view_new (CLAY_TYPE_LAYER_TREE_VIEW,
                                  view_size, 2, TRUE,
                                  clay_context_get_image (context),
                                  menus_get_global_menu_factory (context->clay),
                                  "<Layers>",
                                  "/layers-popup");
}

GtkWidget *
dialogs_channel_list_view_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  if (view_size < 1)
    view_size = context->clay->config->layer_preview_size;

  return clay_item_tree_view_new (CLAY_TYPE_CHANNEL_TREE_VIEW,
                                  view_size, 1, TRUE,
                                  clay_context_get_image (context),
                                  menus_get_global_menu_factory (context->clay),
                                  "<Channels>",
                                  "/channels-popup");
}

GtkWidget *
dialogs_path_list_view_new (ClayDialogFactory *factory,
                            ClayContext       *context,
                            ClayUIManager     *ui_manager,
                            gint               view_size)
{
  if (view_size < 1)
    view_size = context->clay->config->layer_preview_size;

  return clay_item_tree_view_new (CLAY_TYPE_PATH_TREE_VIEW,
                                  view_size, 1, TRUE,
                                  clay_context_get_image (context),
                                  menus_get_global_menu_factory (context->clay),
                                  "<Paths>",
                                  "/paths-popup");
}

GtkWidget *
dialogs_colormap_editor_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_colormap_editor_new (menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_histogram_editor_new (ClayDialogFactory *factory,
                              ClayContext       *context,
                              ClayUIManager     *ui_manager,
                              gint               view_size)
{
  return clay_histogram_editor_new ();
}

GtkWidget *
dialogs_selection_editor_new (ClayDialogFactory *factory,
                              ClayContext       *context,
                              ClayUIManager     *ui_manager,
                              gint               view_size)
{
  return clay_selection_editor_new (menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_symmetry_editor_new (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_symmetry_editor_new (menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_undo_editor_new (ClayDialogFactory *factory,
                         ClayContext       *context,
                         ClayUIManager     *ui_manager,
                         gint               view_size)
{
  return clay_undo_editor_new (context->clay->config,
                               menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_sample_point_editor_new (ClayDialogFactory *factory,
                                 ClayContext       *context,
                                 ClayUIManager     *ui_manager,
                                 gint               view_size)
{
  return clay_sample_point_editor_new (menus_get_global_menu_factory (context->clay));
}


/*****  display related dialogs  *****/

GtkWidget *
dialogs_navigation_editor_new (ClayDialogFactory *factory,
                               ClayContext       *context,
                               ClayUIManager     *ui_manager,
                               gint               view_size)
{
  return clay_navigation_editor_new (menus_get_global_menu_factory (context->clay));
}


/*****  misc dockables  *****/

GtkWidget *
dialogs_color_editor_new (ClayDialogFactory *factory,
                          ClayContext       *context,
                          ClayUIManager     *ui_manager,
                          gint               view_size)
{
  return clay_color_editor_new (context);
}


/*************/
/*  editors  */
/*************/

GtkWidget *
dialogs_brush_editor_get (ClayDialogFactory *factory,
                          ClayContext       *context,
                          ClayUIManager     *ui_manager,
                          gint               view_size)
{
  return clay_brush_editor_new (context,
                                menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_dynamics_editor_get (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_dynamics_editor_new (context,
                                   menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_gradient_editor_get (ClayDialogFactory *factory,
                             ClayContext       *context,
                             ClayUIManager     *ui_manager,
                             gint               view_size)
{
  return clay_gradient_editor_new (context,
                                   menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_palette_editor_get (ClayDialogFactory *factory,
                            ClayContext       *context,
                            ClayUIManager     *ui_manager,
                            gint               view_size)
{
  return clay_palette_editor_new (context,
                                  menus_get_global_menu_factory (context->clay));
}

GtkWidget *
dialogs_tool_preset_editor_get (ClayDialogFactory *factory,
                                ClayContext       *context,
                                ClayUIManager     *ui_manager,
                                gint               view_size)
{
  return clay_tool_preset_editor_new (context,
                                      menus_get_global_menu_factory (context->clay));
}
