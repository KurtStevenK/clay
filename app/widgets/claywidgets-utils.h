/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claywidgets-utils.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __APP_CLAY_WIDGETS_UTILS_H__
#define __APP_CLAY_WIDGETS_UTILS_H__


void              clay_language_flags_register       (void);

GtkWidget       * clay_menu_item_get_image         (GtkMenuItem          *item);
void              clay_menu_item_set_image         (GtkMenuItem          *item,
                                                    GtkWidget            *image,
                                                    ClayAction           *action);

void              clay_menu_position               (GtkMenu              *menu,
                                                    gint                 *x,
                                                    gint                 *y);
void              clay_grid_attach_icon            (GtkGrid              *grid,
                                                    gint                  row,
                                                    const gchar          *icon_name,
                                                    GtkWidget            *widget,
                                                    gint                  columns);
void              clay_enum_radio_box_add          (GtkBox               *box,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
void              clay_enum_radio_frame_add        (GtkFrame             *frame,
                                                    GtkWidget            *widget,
                                                    gint                  enum_value,
                                                    gboolean              below);
GdkPixbuf       * clay_widget_load_icon            (GtkWidget            *widget,
                                                    const gchar          *icon_name,
                                                    gint                  size);
ClayTabStyle      clay_preview_tab_style_to_icon   (ClayTabStyle          tab_style);

const gchar     * clay_get_mod_string              (GdkModifierType       modifiers);
gchar           * clay_suggest_modifiers           (const gchar          *message,
                                                    GdkModifierType       modifiers,
                                                    const gchar          *extend_selection_format,
                                                    const gchar          *toggle_behavior_format,
                                                    const gchar          *alt_format);
ClayChannelOps    clay_modifiers_to_channel_op     (GdkModifierType       modifiers);
GdkModifierType   clay_replace_virtual_modifiers   (GdkModifierType       modifiers);
GdkModifierType   clay_get_primary_accelerator_mask(void);
GdkModifierType   clay_get_extend_selection_mask   (void);
GdkModifierType   clay_get_modify_selection_mask   (void);
GdkModifierType   clay_get_toggle_behavior_mask    (void);
GdkModifierType   clay_get_constrain_behavior_mask (void);
GdkModifierType   clay_get_all_modifiers_mask      (void);

void              clay_get_monitor_resolution      (GdkMonitor           *monitor,
                                                    gdouble              *xres,
                                                    gdouble              *yres);
GeglColor       * clay_get_style_color             (GtkWidget            *widget,
                                                    const gchar          *color_name);
void              clay_window_set_hint             (GtkWindow            *window,
                                                    ClayWindowHint        hint);
void              clay_window_set_transient_for    (GtkWindow            *window,
                                                    ClayProgress         *progress);
void          clay_window_set_transient_for_handle (GtkWindow            *window,
                                                    GBytes               *handle);
void              clay_widget_set_accel_help       (GtkWidget            *widget,
                                                    ClayAction           *action);

const gchar     * clay_get_message_icon_name       (ClayMessageSeverity   severity);
gboolean          clay_get_color_tag_color         (ClayColorTag          color_tag,
                                                    GeglColor            *color,
                                                    gboolean              inherited);

void              clay_pango_layout_set_scale      (PangoLayout          *layout,
                                                    double                scale);
void              clay_pango_layout_set_weight     (PangoLayout          *layout,
                                                    PangoWeight           weight);

void              clay_highlight_widget            (GtkWidget            *widget,
                                                    gboolean              highlight,
                                                    GdkRectangle         *rect);
void              clay_widget_blink_rect           (GtkWidget            *widget,
                                                    GdkRectangle         *rect);
void              clay_widget_blink                (GtkWidget             *widget);
void              clay_widget_blink_cancel         (GtkWidget             *widget);

void              clay_blink_toolbox               (Clay                  *clay,
                                                    const gchar           *action_name,
                                                    GList               **blink_script);
void              clay_blink_dockable              (Clay                  *clay,
                                                    const gchar           *dockable_identifier,
                                                    const gchar           *widget_identifier,
                                                    const gchar           *settings_value,
                                                    GList               **blink_script);

void              clay_widget_script_blink         (GtkWidget            *widget,
                                                    const gchar          *widget_identifier,
                                                    GList               **blink_script);
void              clay_blink_play_script           (GList                *blink_script);


GtkWidget       * clay_dock_with_window_new        (ClayDialogFactory    *factory,
                                                    GdkMonitor           *monitor,
                                                    gboolean              toolbox);
GtkWidget       * clay_tools_get_tool_options_gui  (ClayToolOptions      *tool_options);
void              clay_tools_set_tool_options_gui  (ClayToolOptions      *tool_options,
                                                    GtkWidget            *widget);
void              clay_tools_set_tool_options_gui_func
                                                   (ClayToolOptions      *tool_options,
                                                    ClayToolOptionsGUIFunc func);

gboolean          clay_widget_get_fully_opaque     (GtkWidget            *widget);
void              clay_widget_set_fully_opaque     (GtkWidget            *widget,
                                                    gboolean              fully_opaque);

void              clay_gtk_container_clear         (GtkContainer         *container);

void              clay_button_set_suggested        (GtkWidget            *button,
                                                    gboolean              suggested,
                                                    GtkReliefStyle        default_relief);
void              clay_button_set_destructive      (GtkWidget            *button,
                                                    gboolean              destructive,
                                                    GtkReliefStyle        default_relief);

void              clay_gtk_adjustment_chain        (GtkAdjustment        *adjustment1,
                                                    GtkAdjustment        *adjustment2);

const gchar     * clay_print_event                 (const GdkEvent       *event);

gboolean          clay_color_profile_store_add_defaults
                                                   (ClayColorProfileStore *store,
                                                    ClayColorConfig       *config,
                                                    ClayImageBaseType      base_type,
                                                    ClayPrecision          precision,
                                                    GError               **error);

void              clay_color_profile_chooser_dialog_connect_path
                                                   (GtkWidget             *dialog,
                                                    GObject               *config,
                                                    const gchar           *property_name);

void              clay_widget_flush_expose         (void);

void              clay_make_valid_action_name      (gchar                 *action_name);

gchar           * clay_utils_make_canonical_menu_label (const gchar       *path);
gchar          ** clay_utils_break_menu_path           (const gchar       *path,
                                                        gchar            **mnemonic_path1,
                                                        gchar           ***section_names);
gboolean          clay_utils_are_menu_path_identical   (const gchar       *path1,
                                                        const gchar       *path2,
                                                        gchar            **canonical_path1,
                                                        gchar            **mnemonic_path1,
                                                        gchar            **path1_section_name);
#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
void              clay_window_set_title_bar_theme      (Clay              *clay,
                                                        GtkWidget         *dialog);
#endif
#ifdef G_OS_WIN32
gboolean          clay_is_win32_system_theme_dark      (void);
#endif
#ifdef PLATFORM_OSX
gboolean          themes_macos_is_dark_mode_active     (void);
#endif
#if defined(G_OS_WIN32) || defined(PLATFORM_OSX)
gchar           * themes_get_system_accent_color       (gboolean           prefer_dark);
#endif

#endif /* __APP_CLAY_WIDGETS_UTILS_H__ */
