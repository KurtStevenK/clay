/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include <string.h> /* strlen */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaybase/claybase-private.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claycoreapp.h"

#include "config/clayrc.h"

#include "gegl/clay-babl.h"

#include "pdb/claypdb.h"
#include "pdb/clay-pdb-compat.h"
#include "pdb/internal-procs.h"

#include "plug-in/claypluginmanager.h"
#include "plug-in/claypluginmanager-restore.h"

#include "paint/clay-paint.h"

#include "xcf/xcf.h"
#include "file-data/file-data.h"

#include "clay.h"
#include "clay-contexts.h"
#include "clay-data-factories.h"
#include "clay-filter-history.h"
#include "clay-memsize.h"
#include "clay-modules.h"
#include "clay-parasites.h"
#include "clay-templates.h"
#include "clay-units.h"
#include "clay-utils.h"
#include "claybrush.h"
#include "claybrushgenerated.h"
#include "claybuffer.h"
#include "claycontext.h"
#include "claydynamics.h"
#include "claydocumentlist.h"
#include "clayextensionmanager.h"
#include "claygradient.h"
#include "clayidtable.h"
#include "clayimage.h"
#include "clayimage-metadata.h"
#include "clayimagefile.h"
#include "claylist.h"
#include "claymarshal.h"
#include "claymybrush.h"
#include "claypalette.h"
#include "clayparasitelist.h"
#include "claypattern.h"
#include "claytemplate.h"
#include "claytoolinfo.h"
#include "claytoolpreset.h"
#include "claytreeproxy.h"

#include "text/clayfont.h"

#include "clay-intl.h"


/*  we need to register all enum types so they are known to the type
 *  system by name, re-use the files the pdb generated for libclay
 */
void           clay_enums_init           (void);
const gchar ** clay_enums_get_type_names (gint *n_type_names);
#include "libclay/clayenums.c.tail"


enum
{
  INITIALIZE,
  RESTORE,
  EXIT,
  CLIPBOARD_CHANGED,
  FILTER_HISTORY_CHANGED,
  IMAGE_OPENED,
  FOCUSED_ONCE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VERBOSE
};


static void      clay_constructed                    (GObject           *object);
static void      clay_set_property                   (GObject           *object,
                                                      guint              property_id,
                                                      const GValue      *value,
                                                      GParamSpec        *pspec);
static void      clay_get_property                   (GObject           *object,
                                                      guint              property_id,
                                                      GValue            *value,
                                                      GParamSpec        *pspec);
static void      clay_dispose                        (GObject           *object);
static void      clay_finalize                       (GObject           *object);

static gint64    clay_get_memsize                    (ClayObject        *object,
                                                      gint64            *gui_size);

static void      clay_real_initialize                (Clay              *clay,
                                                      ClayInitStatusFunc status_callback);
static void      clay_real_restore                   (Clay              *clay,
                                                      ClayInitStatusFunc status_callback);
static gboolean  clay_real_exit                      (Clay              *clay,
                                                      gboolean           force);

static void      clay_global_config_notify           (GObject           *global_config,
                                                      GParamSpec        *param_spec,
                                                      GObject           *edit_config);
static void      clay_edit_config_notify             (GObject           *edit_config,
                                                      GParamSpec        *param_spec,
                                                      GObject           *global_config);

static gboolean  clay_exit_idle_cleanup_stray_images (Clay              *clay);


G_DEFINE_TYPE (Clay, clay, CLAY_TYPE_OBJECT)

#define parent_class clay_parent_class

static guint clay_signals[LAST_SIGNAL] = { 0, };


static void
clay_class_init (ClayClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  clay_signals[INITIALIZE] =
    g_signal_new ("initialize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass, initialize),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  clay_signals[RESTORE] =
    g_signal_new ("restore",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass, restore),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  clay_signals[EXIT] =
    g_signal_new ("exit",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass, exit),
                  g_signal_accumulator_true_handled, NULL,
                  clay_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);

  clay_signals[CLIPBOARD_CHANGED] =
    g_signal_new ("clipboard-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass, clipboard_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_signals[FILTER_HISTORY_CHANGED] =
    g_signal_new ("filter-history-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass,
                                   filter_history_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_signals[IMAGE_OPENED] =
    g_signal_new ("image-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayClass, image_opened),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_FILE);

  clay_signals[FOCUSED_ONCE] =
    g_signal_new ("focused-once",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed      = clay_constructed;
  object_class->set_property     = clay_set_property;
  object_class->get_property     = clay_get_property;
  object_class->dispose          = clay_dispose;
  object_class->finalize         = clay_finalize;

  clay_object_class->get_memsize = clay_get_memsize;

  klass->initialize              = clay_real_initialize;
  klass->restore                 = clay_real_restore;
  klass->exit                    = clay_real_exit;
  klass->clipboard_changed       = NULL;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_init (Clay *clay)
{
  clay->be_verbose       = FALSE;
  clay->no_data          = FALSE;
  clay->no_interface     = FALSE;
  clay->show_gui         = TRUE;
  clay->use_shm          = FALSE;
  clay->use_cpu_accel    = TRUE;
  clay->message_handler  = CLAY_CONSOLE;
  clay->show_playground  = FALSE;
  clay->stack_trace_mode = CLAY_STACK_TRACE_NEVER;
  clay->pdb_compat_mode  = CLAY_PDB_COMPAT_OFF;

  clay_gui_init (clay);

  clay->parasites = clay_parasite_list_new ();

  clay_enums_init ();

  clay_units_init (clay);

  clay->images = clay_list_new_weak (CLAY_TYPE_IMAGE, FALSE);
  clay_object_set_static_name (CLAY_OBJECT (clay->images), "images");

  clay->next_guide_id         = 1;
  clay->next_sample_point_id  = 1;
  clay->image_table           = clay_id_table_new ();
  clay->item_table            = clay_id_table_new ();
  clay->drawable_filter_table = clay_id_table_new ();

  clay->displays = g_object_new (CLAY_TYPE_LIST,
                                 "child-type", CLAY_TYPE_OBJECT,
                                 "policy",     CLAY_CONTAINER_POLICY_WEAK,
                                 "append",     TRUE,
                                 NULL);
  clay_object_set_static_name (CLAY_OBJECT (clay->displays), "displays");
  clay->next_display_id = 1;

  clay->named_buffers = clay_list_new (CLAY_TYPE_BUFFER, TRUE);
  clay_object_set_static_name (CLAY_OBJECT (clay->named_buffers),
                               "named buffers");

  clay_data_factories_init (clay);

  clay->tool_info_list = g_object_new (CLAY_TYPE_LIST,
                                       "child-type", CLAY_TYPE_TOOL_INFO,
                                       "append",     TRUE,
                                       NULL);
  clay_object_set_static_name (CLAY_OBJECT (clay->tool_info_list),
                               "tool infos");

  clay->tool_item_list = g_object_new (CLAY_TYPE_LIST,
                                       "child-type", CLAY_TYPE_TOOL_ITEM,
                                       "append",     TRUE,
                                       NULL);
  clay_object_set_static_name (CLAY_OBJECT (clay->tool_item_list),
                               "tool items");

  clay->tool_item_ui_list = clay_tree_proxy_new_for_container (
    clay->tool_item_list);
  clay_object_set_static_name (CLAY_OBJECT (clay->tool_item_ui_list),
                               "ui tool items");

  clay->documents = clay_document_list_new (clay);

  clay->templates = clay_list_new (CLAY_TYPE_TEMPLATE, TRUE);
  clay_object_set_static_name (CLAY_OBJECT (clay->templates), "templates");

  clay->focused_once = FALSE;
}

static void
clay_constructed (GObject *object)
{
  Clay *clay = Clay (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_modules_init (clay);

  clay_paint_init (clay);

  clay->extension_manager = clay_extension_manager_new (clay);
  clay->plug_in_manager   = clay_plug_in_manager_new (clay);
  clay->pdb               = clay_pdb_new (clay);

  xcf_init (clay);
  file_data_init (clay);

  /*  create user and default context  */
  clay_contexts_init (clay);

  /* Initialize the extension manager early as its contents may be used
   * at the very start (e.g. the splash image).
   */
  clay_extension_manager_initialize (clay->extension_manager);
}

static void
clay_set_property (GObject           *object,
                   guint              property_id,
                   const GValue      *value,
                   GParamSpec        *pspec)
{
  Clay *clay = Clay (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      clay->be_verbose = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_get_property (GObject    *object,
                   guint       property_id,
                   GValue     *value,
                   GParamSpec *pspec)
{
  Clay *clay = Clay (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, clay->be_verbose);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dispose (GObject *object)
{
  Clay *clay = Clay (object);

  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  clay_data_factories_clear (clay);

  clay_filter_history_clear (clay);

  g_clear_object (&clay->edit_config);
  g_clear_object (&clay->config);

  g_clear_object (&clay->image_new_last_template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_finalize (GObject *object)
{
  Clay  *clay      = Clay (object);
  GList *standards = NULL;

  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  standards = g_list_prepend (standards,
                              clay_brush_get_standard (clay->user_context));
  standards = g_list_prepend (standards,
                              clay_dynamics_get_standard (clay->user_context));
  standards = g_list_prepend (standards,
                              clay_mybrush_get_standard (clay->user_context));
  standards = g_list_prepend (standards,
                              clay_pattern_get_standard (clay->user_context));
  standards = g_list_prepend (standards,
                              clay_gradient_get_standard (clay->user_context));
  standards = g_list_prepend (standards,
                              clay_palette_get_standard (clay->user_context));

  g_clear_object (&clay->templates);
  g_clear_object (&clay->documents);

  clay_tool_info_set_standard (clay, NULL);

  g_clear_object (&clay->tool_item_list);
  g_clear_object (&clay->tool_item_ui_list);

  if (clay->tool_info_list)
    {
      clay_container_foreach (clay->tool_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_clear_object (&clay->tool_info_list);
    }

  file_data_exit (clay);
  xcf_exit (clay);

  g_clear_object (&clay->pdb);

  clay_data_factories_exit (clay);

  g_clear_object (&clay->named_buffers);
  g_clear_object (&clay->clipboard_buffer);
  g_clear_object (&clay->clipboard_image);
  g_clear_object (&clay->displays);
  g_clear_object (&clay->item_table);
  g_clear_object (&clay->drawable_filter_table);
  g_clear_object (&clay->image_table);
  g_clear_object (&clay->images);
  g_clear_object (&clay->plug_in_manager);
  g_clear_object (&clay->extension_manager);

  if (clay->module_db)
    clay_modules_exit (clay);

  clay_paint_exit (clay);

  g_clear_object (&clay->parasites);
  g_clear_object (&clay->default_folder);

  g_clear_pointer (&clay->session_name, g_free);

  if (clay->context_list)
    {
      GList *list;

      g_warning ("%s: list of contexts not empty upon exit (%d contexts left)\n",
                 G_STRFUNC, g_list_length (clay->context_list));

      for (list = clay->context_list; list; list = g_list_next (list))
        g_printerr ("stale context: %s (of type %s)\n",
                    clay_object_get_name (list->data),
                    g_type_name (G_TYPE_FROM_INSTANCE (list->data)));

      g_list_free (clay->context_list);
      clay->context_list = NULL;
    }

  g_list_foreach (standards, (GFunc) g_object_unref, NULL);
  g_list_free (standards);

  clay_units_exit (clay);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_get_memsize (ClayObject *object,
                  gint64     *gui_size)
{
  Clay   *clay    = Clay (object);
  gint64  memsize = 0;

  memsize += clay_g_list_get_memsize (clay->user_units, 0 /* FIXME */);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->parasites),
                                      gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->paint_info_list),
                                      gui_size);

  memsize += clay_g_object_get_memsize (G_OBJECT (clay->module_db));
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->plug_in_manager),
                                      gui_size);

  memsize += clay_g_list_get_memsize_foreach (clay->filter_history,
                                              (ClayMemsizeFunc)
                                              clay_object_get_memsize,
                                              gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->image_table), 0);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->item_table),  0);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->drawable_filter_table),  0);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->displays), gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->clipboard_image),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->clipboard_buffer),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->named_buffers),
                                      gui_size);

  memsize += clay_data_factories_get_memsize (clay, gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->pdb), gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->tool_info_list),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->standard_tool_info),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->documents),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->templates),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->image_new_last_template),
                                      gui_size);

  memsize += clay_g_list_get_memsize (clay->context_list, 0);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->default_context),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->user_context),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_real_initialize (Clay               *clay,
                      ClayInitStatusFunc  status_callback)
{
  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  status_callback (_("Initialization"), NULL, 0.0);

  /*  add data objects that need the user context  */
  clay_data_factories_add_builtin (clay);

  /*  register all internal procedures  */
  status_callback (NULL, _("Internal Procedures"), 0.2);
  internal_procs_init (clay->pdb);
  clay_pdb_compat_procs_register (clay->pdb, clay->pdb_compat_mode);

  clay_plug_in_manager_initialize (clay->plug_in_manager, status_callback);

  status_callback (NULL, "", 1.0);
}

static void
clay_real_restore (Clay               *clay,
                   ClayInitStatusFunc  status_callback)
{
  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  clay_plug_in_manager_restore (clay->plug_in_manager,
                                clay_get_user_context (clay), status_callback);

  /*  initialize babl fishes  */
  status_callback (_("Initialization"), "Babl Fishes", 0.0);
  clay_babl_init_fishes (status_callback);

  clay->restored = TRUE;
}

static gboolean clay_restart_pending = FALSE;

static void
clay_restart_spawn (void)
{
  gchar  *exe = NULL;
  gchar  *argv[2];
  GError *error = NULL;

#ifdef G_OS_WIN32
  {
    gchar *dir = g_win32_get_package_installation_directory_of_module (NULL);

    if (dir)
      {
        exe = g_build_filename (dir, "bin", g_get_prgname (), NULL);
        if (! g_file_test (exe, G_FILE_TEST_IS_REGULAR))
          {
            g_free (exe);
            exe = g_build_filename (dir, g_get_prgname (), NULL);
          }
        g_free (dir);
      }
  }
#else
  exe = g_file_read_link ("/proc/self/exe", NULL);
#endif

  if (! exe)
    return;

  argv[0] = exe;
  argv[1] = NULL;

  if (! g_spawn_async (NULL, argv, NULL, G_SPAWN_DEFAULT,
                       NULL, NULL, NULL, &error))
    {
      g_printerr ("Could not restart Clay: %s\n", error->message);
      g_clear_error (&error);
    }

  g_free (exe);
}

static gboolean
clay_real_exit (Clay     *clay,
                gboolean  force)
{
  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  if (clay_restart_pending)
    {
      clay_restart_pending = FALSE;
      clay_restart_spawn ();
    }

  clay_plug_in_manager_exit (clay->plug_in_manager);
  clay_extension_manager_exit (clay->extension_manager);
  clay_modules_unload (clay);

  clay_data_factories_save (clay);

  clay_templates_save (clay);
  clay_parasiterc_save (clay);
  clay_unitrc_save (clay);

  return FALSE; /* continue exiting */
}

Clay *
clay_new (const gchar       *name,
          const gchar       *session_name,
          GFile             *default_folder,
          gboolean           be_verbose,
          gboolean           no_data,
          gboolean           no_fonts,
          gboolean           no_interface,
          gboolean           use_shm,
          gboolean           use_cpu_accel,
          gboolean           console_messages,
          gboolean           show_playground,
          gboolean           show_debug_menu,
          ClayStackTraceMode stack_trace_mode,
          ClayPDBCompatMode  pdb_compat_mode)
{
  Clay *clay;

  g_return_val_if_fail (name != NULL, NULL);

  clay = g_object_new (CLAY_TYPE_CLAY,
                       "name",    name,
                       "verbose", be_verbose ? TRUE : FALSE,
                       NULL);

  if (default_folder)
    clay->default_folder = g_object_ref (default_folder);

  clay->session_name     = g_strdup (session_name);
  clay->no_data          = no_data          ? TRUE : FALSE;
  clay->no_fonts         = no_fonts         ? TRUE : FALSE;
  clay->no_interface     = no_interface     ? TRUE : FALSE;
  clay->use_shm          = use_shm          ? TRUE : FALSE;
  clay->use_cpu_accel    = use_cpu_accel    ? TRUE : FALSE;
  clay->console_messages = console_messages ? TRUE : FALSE;
  clay->show_playground  = show_playground  ? TRUE : FALSE;
  clay->show_debug_menu  = show_debug_menu  ? TRUE : FALSE;
  clay->stack_trace_mode = stack_trace_mode;
  clay->pdb_compat_mode  = pdb_compat_mode;

  return clay;
}

/**
 * clay_set_show_gui:
 * @clay:
 * @show:
 *
 * Test cases that tests the UI typically don't want any windows to be
 * presented during the test run. Allow them to set this.
 **/
void
clay_set_show_gui (Clay     *clay,
                   gboolean  show_gui)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay->show_gui = show_gui;
}

/**
 * clay_get_show_gui:
 * @clay:
 *
 * Returns: %TRUE if the GUI should be shown, %FALSE otherwise.
 **/
gboolean
clay_get_show_gui (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  return clay->show_gui;
}

static void
clay_global_config_notify (GObject    *global_config,
                           GParamSpec *param_spec,
                           GObject    *edit_config)
{
  GValue global_value = G_VALUE_INIT;
  GValue edit_value   = G_VALUE_INIT;

  g_value_init (&global_value, param_spec->value_type);
  g_value_init (&edit_value,   param_spec->value_type);

  g_object_get_property (global_config, param_spec->name, &global_value);
  g_object_get_property (edit_config,   param_spec->name, &edit_value);

  if (g_param_values_cmp (param_spec, &global_value, &edit_value))
    {
      g_signal_handlers_block_by_func (edit_config,
                                       clay_edit_config_notify,
                                       global_config);

      g_object_set_property (edit_config, param_spec->name, &global_value);

      g_signal_handlers_unblock_by_func (edit_config,
                                         clay_edit_config_notify,
                                         global_config);
    }

  g_value_unset (&global_value);
  g_value_unset (&edit_value);
}

static void
clay_edit_config_notify (GObject    *edit_config,
                         GParamSpec *param_spec,
                         GObject    *global_config)
{
  GValue edit_value   = G_VALUE_INIT;
  GValue global_value = G_VALUE_INIT;

  g_value_init (&edit_value,   param_spec->value_type);
  g_value_init (&global_value, param_spec->value_type);

  g_object_get_property (edit_config,   param_spec->name, &edit_value);
  g_object_get_property (global_config, param_spec->name, &global_value);

  if (g_param_values_cmp (param_spec, &edit_value, &global_value))
    {
      if (param_spec->flags & CLAY_CONFIG_PARAM_RESTART)
        {
#ifdef CLAY_CONFIG_DEBUG
          g_print ("NOT Applying edit_config change of '%s' to global_config "
                   "because it needs restart\n",
                   param_spec->name);
#endif
        }
      else
        {
#ifdef CLAY_CONFIG_DEBUG
          g_print ("Applying edit_config change of '%s' to global_config\n",
                   param_spec->name);
#endif
          g_signal_handlers_block_by_func (global_config,
                                           clay_global_config_notify,
                                           edit_config);

          g_object_set_property (global_config, param_spec->name, &edit_value);

          g_signal_handlers_unblock_by_func (global_config,
                                             clay_global_config_notify,
                                             edit_config);
        }
    }

  g_value_unset (&edit_value);
  g_value_unset (&global_value);
}

void
clay_load_config (Clay  *clay,
                  GFile *alternate_system_clayrc,
                  GFile *alternate_clayrc)
{
  ClayRc *clayrc;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (alternate_system_clayrc == NULL ||
                    G_IS_FILE (alternate_system_clayrc));
  g_return_if_fail (alternate_clayrc == NULL ||
                    G_IS_FILE (alternate_clayrc));
  g_return_if_fail (clay->config == NULL);
  g_return_if_fail (clay->edit_config == NULL);

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  this needs to be done before clayrc loading because clayrc can
   *  use user defined units
   */
  clay_unitrc_load (clay);

  clayrc = clay_rc_new (G_OBJECT (clay),
                        alternate_system_clayrc,
                        alternate_clayrc,
                        clay->be_verbose);

  clay->config = CLAY_CORE_CONFIG (clayrc);

  clay->edit_config = clay_config_duplicate (CLAY_CONFIG (clay->config));

  g_signal_connect_object (clay->config, "notify",
                           G_CALLBACK (clay_global_config_notify),
                           clay->edit_config, 0);
  g_signal_connect_object (clay->edit_config, "notify",
                           G_CALLBACK (clay_edit_config_notify),
                           clay->config, 0);

  if (! clay->show_playground)
    {
      gboolean use_opencl;
      gboolean use_npd_tool;
      gboolean use_seamless_clone_tool;
      gboolean use_paint_select_tool;
      gboolean use_list_box;

      /* Playground preferences is shown by default for unstable
       * versions and if the associated CLI option was set. Additionally
       * we want to show it if any of the playground options had been
       * enabled. Otherwise you might end up getting blocked with a
       * playground feature and forget where you can even disable it.
       *
       * Also we check this once at start when loading config, and not
       * inside preferences-dialog.c because we don't want to end up
       * with inconsistent behavior where you open once the Preferences,
       * deactivate features, then back to preferences and the tab is
       * gone.
       */

      g_object_get (clay->edit_config,
                    "use-opencl",                     &use_opencl,
                    "playground-npd-tool",            &use_npd_tool,
                    "playground-seamless-clone-tool", &use_seamless_clone_tool,
                    "playground-paint-select-tool",   &use_paint_select_tool,
                    "playground-use-list-box",        &use_list_box,
                    NULL);
      if (use_opencl || use_npd_tool || use_seamless_clone_tool ||
          use_paint_select_tool || use_list_box)
        clay->show_playground = TRUE;
    }
}

void
clay_initialize (Clay               *clay,
                 ClayInitStatusFunc  status_callback)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (status_callback != NULL);
  g_return_if_fail (CLAY_IS_CORE_CONFIG (clay->config));

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  g_signal_emit (clay, clay_signals[INITIALIZE], 0, status_callback);
}

/**
 * clay_restore:
 * @clay: a #Clay object
 * @error: a #GError for uncessful loading.
 *
 * This function always succeeds. If present, @error may be filled for
 * possible feedback on data which failed to load. It doesn't imply any
 * fatale error.
 **/
void
clay_restore (Clay                *clay,
              ClayInitStatusFunc   status_callback,
              GError             **error)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (status_callback != NULL);

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  initialize  the global parasite table  */
  status_callback (_("Looking for data files"), _("Parasites"), 0.0);
  clay_parasiterc_load (clay);

  /*  initialize the lists of clay brushes, dynamics, patterns etc.  */
  clay_data_factories_load (clay, status_callback);

  /*  initialize the template list  */
  status_callback (NULL, _("Templates"), 0.8);
  clay_templates_load (clay);

  /*  initialize the module list  */
  status_callback (NULL, _("Modules"), 0.9);
  clay_modules_load (clay);

  g_signal_emit (clay, clay_signals[RESTORE], 0, status_callback);

  /* when done, make sure everything is clean, to clean out dirty
   * states from data objects which reference each other and got
   * dirtied by loading the referenced object
   */
  clay_data_factories_data_clean (clay);
}

/**
 * clay_is_restored:
 * @clay: a #Clay object
 *
 * Returns: %TRUE if Clay is completely started, %FALSE otherwise.
 **/
gboolean
clay_is_restored (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  return clay->initialized && clay->restored && clay_data_factories_wait (clay);
}

/**
 * clay_exit:
 * @clay: a #Clay object
 * @force: whether to force the application to quit
 *
 * Exit this Clay session. Unless @force is %TRUE, the user is queried
 * whether unsaved images should be saved and can cancel the operation.
 **/
void
clay_restart (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_restart_pending = TRUE;
  clay_exit (clay, FALSE);
}

void
clay_restart_cancel (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_restart_pending = FALSE;
}

void
clay_exit (Clay     *clay,
           gboolean  force)
{
  gboolean handled;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_emit (clay, clay_signals[EXIT], 0,
                 force ? TRUE : FALSE,
                 &handled);

  if (handled)
    return;

  clay_contexts_exit (clay);

  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) clay_exit_idle_cleanup_stray_images,
                   clay, NULL);
}

void
clay_set_focused_once (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (! clay->focused_once)
    {
      clay->focused_once = TRUE;
      g_signal_emit (clay, clay_signals[FOCUSED_ONCE], 0);
    }
}

gboolean
clay_has_focused_once (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  return clay->focused_once;
}

GList *
clay_get_image_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->images)->queue->head;
}

GList *
clay_get_display_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->displays)->queue->head;
}

GList *
clay_get_image_windows (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_list_copy (clay->image_windows);
}

GList *
clay_get_paint_info_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->paint_info_list)->queue->head;
}

GList *
clay_get_tool_info_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->tool_info_list)->queue->head;
}

GList *
clay_get_tool_item_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->tool_item_list)->queue->head;
}

GList *
clay_get_tool_item_ui_iter (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return CLAY_LIST (clay->tool_item_ui_list)->queue->head;
}

ClayObject *
clay_get_clipboard_object (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->clipboard_image)
    return CLAY_OBJECT (clay->clipboard_image);

  return CLAY_OBJECT (clay->clipboard_buffer);
}

void
clay_set_clipboard_image (Clay      *clay,
                          ClayImage *image)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  g_clear_object (&clay->clipboard_buffer);
  g_set_object (&clay->clipboard_image, image);

  /* we want the signal emission */
  g_signal_emit (clay, clay_signals[CLIPBOARD_CHANGED], 0);
}

ClayImage *
clay_get_clipboard_image (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return clay->clipboard_image;
}

void
clay_set_clipboard_buffer (Clay       *clay,
                           ClayBuffer *buffer)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (buffer == NULL || CLAY_IS_BUFFER (buffer));

  g_clear_object (&clay->clipboard_image);
  g_set_object (&clay->clipboard_buffer, buffer);

  /* we want the signal emission */
  g_signal_emit (clay, clay_signals[CLIPBOARD_CHANGED], 0);
}

ClayBuffer *
clay_get_clipboard_buffer (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return clay->clipboard_buffer;
}

ClayImage *
clay_create_image (Clay              *clay,
                   gint               width,
                   gint               height,
                   ClayImageBaseType  type,
                   ClayPrecision      precision,
                   gboolean           attach_comment)
{
  ClayImage    *image;
  ClayMetadata *metadata;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  image = clay_image_new (clay, width, height, type, precision);

  metadata = clay_metadata_new ();
  clay_image_set_metadata (image, metadata, FALSE);
  g_object_unref (metadata);

  if (attach_comment)
    {
      const gchar *comment;

      comment = clay_template_get_comment (clay->config->default_image);

      if (comment)
        {
          ClayParasite *parasite = clay_parasite_new ("clay-comment",
                                                      CLAY_PARASITE_PERSISTENT,
                                                      strlen (comment) + 1,
                                                      comment);
          clay_image_parasite_attach (image, parasite, FALSE);
          clay_parasite_free (parasite);
        }
    }

  return image;
}

void
clay_set_default_context (Clay        *clay,
                          ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  g_set_object (&clay->default_context, context);
}

ClayContext *
clay_get_default_context (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return clay->default_context;
}

void
clay_set_user_context (Clay        *clay,
                       ClayContext *context)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  g_set_object (&clay->user_context, context);
}

ClayContext *
clay_get_user_context (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return clay->user_context;
}

ClayToolInfo *
clay_get_tool_info (Clay        *clay,
                    const gchar *tool_id)
{
  gpointer info;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (tool_id != NULL, NULL);

  info = clay_container_get_child_by_name (clay->tool_info_list, tool_id);

  return (ClayToolInfo *) info;
}

void
clay_set_last_template (Clay         *clay,
                        ClayTemplate *template)
{
  ClayTemplate *last_template;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_TEMPLATE (template));

  last_template = clay_get_last_template (clay);

  clay_config_sync (G_OBJECT (template),
                    G_OBJECT (last_template), 0);
}

ClayTemplate *
clay_get_last_template (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (! clay->image_new_last_template)
    {
      /*  set the last values used to default values  */
      clay->image_new_last_template =
        clay_config_duplicate (CLAY_CONFIG (clay->config->default_image));
    }

  return clay->image_new_last_template;
}


/**
 * clay_message:
 * @clay:     a pointer to the %Clay object
 * @handler:  either a %ClayProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @...:      arguments to use with @format
 *
 * Present a message to the user. How exactly the message is displayed
 * depends on the @severity, the @handler object and user preferences.
 **/
void
clay_message (Clay                *clay,
              GObject             *handler,
              ClayMessageSeverity  severity,
              const gchar         *format,
              ...)
{
  va_list args;

  va_start (args, format);

  clay_message_valist (clay, handler, severity, format, args);

  va_end (args);
}

/**
 * clay_message_valist:
 * @clay:     a pointer to the %Clay object
 * @handler:  either a %ClayProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @args:     arguments to use with @format
 *
 * See documentation for clay_message().
 **/
void
clay_message_valist (Clay                *clay,
                     GObject             *handler,
                     ClayMessageSeverity  severity,
                     const gchar         *format,
                     va_list              args)
{
  gchar *message;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (format != NULL);

  message = g_strdup_vprintf (format, args);

  clay_show_message (clay, handler, severity, NULL, message);

  g_free (message);
}

void
clay_message_literal (Clay                *clay,
                      GObject             *handler,
                      ClayMessageSeverity  severity,
                      const gchar         *message)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (message != NULL);

  clay_show_message (clay, handler, severity, NULL, message);
}

void
clay_filter_history_changed (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_signal_emit (clay, clay_signals[FILTER_HISTORY_CHANGED], 0);
}

void
clay_image_opened (Clay  *clay,
                   GFile *file)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (clay, clay_signals[IMAGE_OPENED], 0, file);
}

GFile *
clay_get_temp_file (Clay         *clay,
                    const gchar  *extension,
                    GError      **error)
{
  static gint  id = 0;
  static gint  pid;
  gchar       *basename;
  GFile       *dir;
  GFile       *file;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (id == 0)
    pid = clay_get_pid ();

  if (extension)
    basename = g_strdup_printf ("clay-temp-%d%d.%s", pid, id++, extension);
  else
    basename = g_strdup_printf ("clay-temp-%d%d", pid, id++);

  dir = clay_file_new_for_config_path (CLAY_GEGL_CONFIG (clay->config)->temp_path,
                                       error);
  if (dir == NULL)
    {
      g_free (basename);
      return NULL;
    }

  if (! g_file_query_exists (dir, NULL))
    {
      /* Try to make the temp directory if it doesn't exist.
       * Ignore any error.
       */
      if (! g_file_make_directory_with_parents (dir, NULL, error))
        {
          g_free (basename);
          g_object_unref (dir);
          return NULL;
        }
    }
  file = g_file_get_child (dir, basename);
  g_free (basename);
  g_object_unref (dir);

  return file;
}

ClayDataFactory *
clay_get_data_factory (Clay  *clay,
                       GType  data_type)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, CLAY_TYPE_DATA), NULL);

  if (g_type_is_a (data_type, CLAY_TYPE_BRUSH_GENERATED))
    return clay->brush_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_BRUSH))
    return clay->brush_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_PATTERN))
    return clay->pattern_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_GRADIENT))
    return clay->gradient_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_PALETTE))
    return clay->palette_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_FONT))
    return clay->font_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_DYNAMICS))
    return clay->dynamics_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_MYBRUSH))
    return clay->mybrush_factory;
  else if (g_type_is_a (data_type, CLAY_TYPE_TOOL_PRESET))
    return clay->tool_preset_factory;

  /* If we reach this, it means we forgot a data factory in our list! */
  g_return_val_if_reached (NULL);
}

static gboolean
clay_exit_idle_cleanup_stray_images (Clay *clay)
{
  GList *image_iter;

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);

  /* Get rid of images without display. We do this *after* handling the
   * usual exit callbacks and any other pending event, because the
   * things that are torn down there might have references to these
   * images, for instance ClayActions in the UI manager, or some plug-in
   * which was still running and had to get killed in clay_exit() (cf. #11922).
   */
  while ((image_iter = clay_get_image_iter (clay)))
    {
      ClayImage *image = image_iter->data;

      /* Plug-in developers are expected to free the images they
       * created, unless they add a display (while in GUI mode), in
       * which case the user gets ownership of the image and it will be
       * freed as any other image by the GUI.
       * On the other hand, images created from command lines are very
       * likely to still exist when the software exits, if it was closed
       * by the --quit argument, or run without an interface. This is
       * normal use case.
       */
      if (! clay_image_get_from_command_line (image) ||
          (! clay->no_interface && ! clay_core_app_get_quit (CLAY_CORE_APP (clay->app))))
        /* TODO: localize after string freeze. */
        g_printerr ("INFO: a stray image seems to have been left around by a plug-in: \"%s\"\n",
                    clay_image_get_display_name (image));

      g_object_unref (image);
    }

  return G_SOURCE_REMOVE ;
}
