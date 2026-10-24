/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include "clayobject.h"
#include "clay-gui.h"


#define CLAY_TYPE_CLAY            (clay_get_type ())
#define Clay(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CLAY, Clay))
#define CLAY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CLAY, ClayClass))
#define CLAY_IS_CLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CLAY))
#define CLAY_IS_CLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CLAY))


typedef struct _ClayClass ClayClass;

struct _Clay
{
  ClayObject              parent_instance;

  GApplication           *app;

  ClayCoreConfig         *config;
  ClayCoreConfig         *edit_config; /* don't use this one, it's just
                                        * for the preferences dialog
                                        */
  gchar                  *session_name;
  GFile                  *default_folder;

  gboolean                be_verbose;
  gboolean                no_data;
  gboolean                no_fonts;
  gboolean                no_interface;
  gboolean                show_gui;
  gboolean                use_shm;
  gboolean                use_cpu_accel;
  ClayMessageHandlerType  message_handler;
  gboolean                console_messages;
  gboolean                show_playground;
  gboolean                show_debug_menu;
  ClayStackTraceMode      stack_trace_mode;
  ClayPDBCompatMode       pdb_compat_mode;

  ClayGui                 gui;         /* gui vtable */

  gboolean                restored;    /* becomes TRUE in clay_restore() */
  gboolean                initialized; /* Fully initialized (only set once at start). */
  gboolean                query_all;   /* Force query all plug-ins. */

  gint                    busy;
  guint                   busy_idle_id;

  GList                  *user_units;

  ClayParasiteList       *parasites;

  ClayContainer          *paint_info_list;
  ClayPaintInfo          *standard_paint_info;

  ClayModuleDB           *module_db;
  gboolean                write_modulerc;

  ClayExtensionManager   *extension_manager;
  ClayPlugInManager      *plug_in_manager;

  GList                  *filter_history;

  ClayContainer          *images;
  guint32                 next_guide_id;
  guint32                 next_sample_point_id;
  ClayIdTable            *image_table;
  ClayIdTable            *item_table;
  ClayIdTable            *drawable_filter_table;

  ClayContainer          *displays;
  gint                    next_display_id;

  GList                  *image_windows;

  GObject                *controller_manager;

  ClayImage              *clipboard_image;
  ClayBuffer             *clipboard_buffer;
  ClayContainer          *named_buffers;

  ClayDataFactory        *brush_factory;
  ClayDataFactory        *dynamics_factory;
  ClayDataFactory        *mybrush_factory;
  ClayDataFactory        *pattern_factory;
  ClayDataFactory        *gradient_factory;
  ClayDataFactory        *palette_factory;
  ClayDataFactory        *font_factory;
  ClayDataFactory        *tool_preset_factory;

  ClayTagCache           *tag_cache;

  ClayPDB                *pdb;

  ClayContainer          *tool_info_list;
  ClayToolInfo           *standard_tool_info;

  ClayContainer          *tool_item_list;
  ClayContainer          *tool_item_ui_list;

  /*  the opened and saved images in MRU order  */
  ClayContainer          *documents;

  /*  image_new values  */
  ClayContainer          *templates;
  ClayTemplate           *image_new_last_template;

  /*  the list of all contexts  */
  GList                  *context_list;

  /*  the default context which is initialized from clayrc  */
  ClayContext            *default_context;

  /*  the context used by the interface  */
  ClayContext            *user_context;

  /*  GUI focus occured. See gtk#7534.  */
  gboolean                focused_once;
};

struct _ClayClass
{
  ClayObjectClass  parent_class;

  void     (* initialize)             (Clay               *clay,
                                       ClayInitStatusFunc  status_callback);
  void     (* restore)                (Clay               *clay,
                                       ClayInitStatusFunc  status_callback);
  gboolean (* exit)                   (Clay               *clay,
                                       gboolean            force);

  void     (* clipboard_changed)      (Clay               *clay);

  void     (* filter_history_changed) (Clay               *clay);

  /*  emitted if an image is loaded and opened with a display  */
  void     (* image_opened)           (Clay               *clay,
                                       GFile              *file);
};


GType          clay_get_type               (void);

Clay         * clay_new                    (const gchar         *name,
                                            const gchar         *session_name,
                                            GFile               *default_folder,
                                            gboolean             be_verbose,
                                            gboolean             no_data,
                                            gboolean             no_fonts,
                                            gboolean             no_interface,
                                            gboolean             use_shm,
                                            gboolean             use_cpu_accel,
                                            gboolean             console_messages,
                                            gboolean             show_playground,
                                            gboolean             show_debug_menu,
                                            ClayStackTraceMode   stack_trace_mode,
                                            ClayPDBCompatMode    pdb_compat_mode);
void           clay_set_show_gui           (Clay                *clay,
                                            gboolean             show_gui);
gboolean       clay_get_show_gui           (Clay                *clay);

void           clay_load_config            (Clay                *clay,
                                            GFile               *alternate_system_clayrc,
                                            GFile               *alternate_clayrc);
void           clay_initialize             (Clay                *clay,
                                            ClayInitStatusFunc   status_callback);
void           clay_restore                (Clay                *clay,
                                            ClayInitStatusFunc   status_callback,
                                            GError             **error);
gboolean       clay_is_restored            (Clay                *clay);

void           clay_exit                   (Clay                *clay,
                                            gboolean             force);
void           clay_restart                  (Clay                *clay);
void           clay_restart_cancel           (Clay                *clay);

void           clay_set_focused_once       (Clay                *clay);
gboolean       clay_has_focused_once       (Clay                *clay);

GList        * clay_get_image_iter         (Clay                *clay);
GList        * clay_get_display_iter       (Clay                *clay);
GList        * clay_get_image_windows      (Clay                *clay);
GList        * clay_get_paint_info_iter    (Clay                *clay);
GList        * clay_get_tool_info_iter     (Clay                *clay);
GList        * clay_get_tool_item_iter     (Clay                *clay);
GList        * clay_get_tool_item_ui_iter  (Clay                *clay);

ClayObject   * clay_get_clipboard_object   (Clay                *clay);

void           clay_set_clipboard_image    (Clay                *clay,
                                            ClayImage           *image);
ClayImage    * clay_get_clipboard_image    (Clay                *clay);

void           clay_set_clipboard_buffer   (Clay                *clay,
                                            ClayBuffer          *buffer);
ClayBuffer   * clay_get_clipboard_buffer   (Clay                *clay);

ClayImage    * clay_create_image           (Clay                *clay,
                                            gint                 width,
                                            gint                 height,
                                            ClayImageBaseType    type,
                                            ClayPrecision        precision,
                                            gboolean             attach_comment);

void           clay_set_default_context    (Clay                *clay,
                                            ClayContext         *context);
ClayContext  * clay_get_default_context    (Clay                *clay);

void           clay_set_user_context       (Clay                *clay,
                                            ClayContext         *context);
ClayContext  * clay_get_user_context       (Clay                *clay);

ClayToolInfo * clay_get_tool_info          (Clay                *clay,
                                            const gchar         *tool_name);

void           clay_set_last_template      (Clay                *clay,
                                            ClayTemplate        *_template);
ClayTemplate * clay_get_last_template      (Clay                *clay);

void           clay_message                (Clay                *clay,
                                            GObject             *handler,
                                            ClayMessageSeverity  severity,
                                            const gchar         *format,
                                            ...) G_GNUC_PRINTF (4, 5);
void           clay_message_valist         (Clay                *clay,
                                            GObject             *handler,
                                            ClayMessageSeverity  severity,
                                            const gchar         *format,
                                            va_list              args) G_GNUC_PRINTF (4, 0);
void           clay_message_literal        (Clay                *clay,
                                            GObject             *handler,
                                            ClayMessageSeverity  severity,
                                            const gchar         *message);

void           clay_filter_history_changed (Clay                *clay);

void           clay_image_opened           (Clay                *clay,
                                            GFile               *file);

GFile        * clay_get_temp_file          (Clay                *clay,
                                            const gchar         *extension,
                                            GError             **error);

ClayDataFactory *
               clay_get_data_factory       (Clay                *clay,
                                            GType                data_type);
