/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayplugin.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <errno.h>
#include <libintl.h>
#include <string.h>

#include "clay.h"

#include "libclaybase/clayprotocol-private.h"
#include "libclaybase/claywire-private.h"

#include "clay-private.h"
#include "clay-shm.h"
#include "claygpparams.h"
#include "clayplugin-private.h"
#include "clayplugin_pdb.h"


/**
 * ClayPlugIn:
 *
 * The base class for plug-ins to derive from.
 *
 * ClayPlugIn manages the plug-in's [class@Procedure] objects. The procedures a
 * plug-in implements are registered with Clay by returning a list of their
 * names from either [vfunc@ClayPlugIn.query_procedures] or
 * [vfunc@ClayPlugIn.init_procedures].
 *
 * Every Clay plug-in has to be implemented as a subclass and make it known to
 * the libclay infrastructure and the main Clay application by passing its
 * `GType` to [func@MAIN].
 *
 * [func@MAIN] passes the 'argc' and 'argv' of the platform's main() function,
 * along with the `GType`, to [func@main], which creates an instance of the
 * plug-in's `ClayPlugIn` subclass and calls its virtual functions, depending
 * on how the plug-in was called by Clay.
 *
 * There are 3 different ways Clay calls a plug-in: "query", "init" and "run".
 *
 * The plug-in is called in "query" mode once after it was installed, or when
 * the cached plug-in information in the config file "pluginrc" needs to be
 * recreated. In "query" mode, [vfunc@ClayPlugIn.query_procedures] is called
 * and returns a list of procedure names the plug-in implements. This is the
 * "normal" place to register procedures, because the existence of most
 * procedures doesn't depend on things that change between Clay sessions.
 *
 * The plug-in is called in "init" mode at each Clay startup, and
 * [vfunc@PlugIn.init_procedures] is called and returns a list of procedure
 * names this plug-in implements. This only happens if the plug-in actually
 * implements [vfunc@ClayPlugIn.init_procedures]. A plug-in only needs to
 * implement init_procedures if the existence of its procedures can change
 * between Clay sessions, for example if they depend on the presence of
 * external tools, or hardware like scanners, or online services, or whatever
 * variable circumstances.
 *
 * In order to register the plug-in's procedures with the main Clay application
 * in the plug-in's "query" and "init" modes, [class@PlugIn] calls
 * [vfunc@PlugIn.create_procedure] on all procedure names in the exact order of
 * the list returned by [vfunc@PlugIn.query_procedures] or
 * [vfunc@PlugIn.init_procedures] and then registers the returned
 * [class@Procedure].
 *
 * The plug-in is called in "run" mode whenever one of the procedures it
 * implements is called by either the main Clay application or any other
 * plug-in. In "run" mode, one of the procedure names returned by
 * [vfunc@PlugIn.query_procedures] or [vfunc@PlugIn.init_procedures] is passed
 * to [vfunc@PlugIn.create_procedure] which must return a [class@Procedure] for
 * the passed name. The procedure is then executed by calling
 * [method@Procedure.run].
 *
 * In any of the three modes, [vfunc@PlugIn.quit] is called before the plug-in
 * process exits, so the plug-in can perform whatever cleanup necessary.
 *
 * Since: 3.0
 */


#define WRITE_BUFFER_SIZE 1024

/**
 * clay_plug_in_error_quark:
 *
 * Generic #GQuark error domain for plug-ins. Plug-ins are welcome to
 * create their own domain when they want to handle advanced error
 * handling. Often, you just want to pass an error message to the core.
 * This domain can be used for such simple usage.
 *
 * See #GError for information on error domains.
 */
G_DEFINE_QUARK (clay-plug-in-error-quark, clay_plug_in_error)

enum
{
  PROP_0,
  PROP_PROGRAM_NAME,
  PROP_READ_CHANNEL,
  PROP_WRITE_CHANNEL,
  N_PROPS
};


typedef struct _ClayPlugInPrivate
{
  gchar      *program_name;

  GIOChannel *read_channel;
  GIOChannel *write_channel;

  gchar       write_buffer[WRITE_BUFFER_SIZE];
  gulong      write_buffer_index;

  guint       persistent_source_id;

  gchar      *translation_domain_name;
  GFile      *translation_domain_path;

  gchar      *help_domain_name;
  GFile      *help_domain_uri;

  GList      *temp_procedures;

  GList      *procedure_stack;
  GList      *ran_procedure_stack;
  GHashTable *displays;
  GHashTable *images;
  GHashTable *items;
  GHashTable *filters;
  GHashTable *resources;

  gboolean    returned;
} ClayPlugInPrivate;


static void       clay_plug_in_constructed       (GObject         *object);
static void       clay_plug_in_dispose           (GObject         *object);
static void       clay_plug_in_finalize          (GObject         *object);
static void       clay_plug_in_set_property      (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void       clay_plug_in_get_property      (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);

static gboolean   clay_plug_in_real_set_i18n     (ClayPlugIn      *plug_in,
                                                  const gchar     *procedure_name,
                                                  gchar          **gettext_domain,
                                                  gchar          **catalog_dir);

static void       clay_plug_in_register          (ClayPlugIn      *plug_in,
                                                  GList           *procedures);

static gboolean   clay_plug_in_write             (GIOChannel      *channel,
                                                  const guint8    *buf,
                                                  gulong           count,
                                                  gpointer         user_data);
static gboolean   clay_plug_in_flush             (GIOChannel      *channel,
                                                  gpointer         user_data);
static gboolean   clay_plug_in_io_error_handler  (GIOChannel      *channel,
                                                  GIOCondition     cond,
                                                  gpointer         data);

static void       clay_plug_in_loop              (ClayPlugIn      *plug_in);
static void       clay_plug_in_single_message    (ClayPlugIn      *plug_in);
static void       clay_plug_in_process_message   (ClayPlugIn      *plug_in,
                                                  ClayWireMessage *msg);
static void       clay_plug_in_main_proc_run     (ClayPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       clay_plug_in_temp_proc_run     (ClayPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void       clay_plug_in_proc_run          (GPProcRun       *proc_run,
                                                  ClayProcedure   *procedure,
                                                  GPProcReturn    *proc_return);
static void       clay_plug_in_proc_run_internal (ClayPlugIn      *plug_in,
                                                  GPProcRun       *proc_run,
                                                  ClayProcedure   *procedure,
                                                  GPProcReturn    *proc_return);
static gboolean   clay_plug_in_persistent_read   (GIOChannel      *channel,
                                                  GIOCondition     condition,
                                                  gpointer         data);

static void       clay_plug_in_push_procedure    (ClayPlugIn      *plug_in,
                                                  ClayProcedure   *procedure);
static void       clay_plug_in_pop_procedure     (ClayPlugIn      *plug_in,
                                                  ClayProcedure   *procedure);
static gboolean   clay_plug_in_is_procedure_stack_empty (
                                                  ClayPlugIn      *plug_in);

static void       clay_plug_in_main_run_cleanup  (ClayPlugIn      *plug_in);
static void       clay_plug_in_temp_run_cleanup  (ClayPlugIn      *plug_in);
static void       clay_plug_in_destroy_hashes    (ClayPlugIn      *plug_in);
static void       clay_plug_in_destroy_all_proxies
                                                 (ClayPlugIn      *plug_in);
static void       clay_plug_in_destroy_proxies   (ClayPlugIn      *plug_in,
                                                  GHashTable      *hash_table,
                                                  const gchar     *type,
                                                  gboolean         destroy_all);

static void       clay_plug_in_init_i18n         (ClayPlugIn      *plug_in);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPlugIn, clay_plug_in, G_TYPE_OBJECT)

#define parent_class clay_plug_in_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_plug_in_class_init (ClayPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_plug_in_constructed;
  object_class->dispose      = clay_plug_in_dispose;
  object_class->finalize     = clay_plug_in_finalize;
  object_class->set_property = clay_plug_in_set_property;
  object_class->get_property = clay_plug_in_get_property;

  klass->set_i18n            = clay_plug_in_real_set_i18n;

  /**
   * ClayPlugIn:program-name:
   *
   * The program name as usually found on argv[0]
   */
  props[PROP_PROGRAM_NAME] =
    g_param_spec_string ("program-name",
                         "The plug-in executable",
                         "The executable name as usually found on argv[0]",
                         NULL,
                         CLAY_PARAM_WRITABLE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayPlugIn:read-channel:
   *
   * The [struct@GLib.IOChannel] to read from Clay
   */
  props[PROP_READ_CHANNEL] =
    g_param_spec_boxed ("read-channel",
                        "Read channel",
                        "The GIOChanel to read from Clay",
                        G_TYPE_IO_CHANNEL,
                        CLAY_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayPlugIn:write-channel:
   *
   * The [struct@GLib.IOChannel] to write to Clay
   */
  props[PROP_WRITE_CHANNEL] =
    g_param_spec_boxed ("write-channel",
                        "Write channel",
                        "The GIOChanel to write to Clay",
                        G_TYPE_IO_CHANNEL,
                        CLAY_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);

  /* libclay creates GeglColor-s in _clay_config(), which requires at
   * least babl to be initialized.
   * We don't call gegl_init() because gegl_config() does not appreciate
   * being called after, which generates a bunch of CRITICALs (see
   * commit b06fe36970c and gegl#427).
   * Therefore gegl_init() still needs to be called manually by plug-ins
   * if necessary (it is called by clay_ui_init() too, but this only
   * applies to GUI plug-ins).
   */
  babl_init ();
}

static void
clay_plug_in_init (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  priv->procedure_stack     = NULL;
  priv->ran_procedure_stack = NULL;
  priv->temp_procedures     = NULL;
  priv->returned            = FALSE;
}

static void
clay_plug_in_constructed (GObject *object)
{
  ClayPlugIn        *plug_in = CLAY_PLUG_IN (object);
  ClayPlugInPrivate *priv;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv = clay_plug_in_get_instance_private (plug_in);

  g_assert (priv->read_channel != NULL);
  g_assert (priv->write_channel != NULL);

  gp_init ();

  clay_wire_set_writer (clay_plug_in_write);
  clay_wire_set_flusher (clay_plug_in_flush);
}

static void
clay_plug_in_dispose (GObject *object)
{
  ClayPlugIn        *plug_in = CLAY_PLUG_IN (object);
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  if (priv->persistent_source_id)
    {
      g_source_remove (priv->persistent_source_id);
      priv->persistent_source_id = 0;
    }

  if (priv->temp_procedures)
    {
      g_list_free_full (priv->temp_procedures, g_object_unref);
      priv->temp_procedures = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_plug_in_finalize (GObject *object)
{
  ClayPlugIn        *plug_in = CLAY_PLUG_IN (object);
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  g_clear_pointer (&priv->program_name, g_free);
  g_clear_pointer (&priv->translation_domain_name, g_free);
  g_clear_object  (&priv->translation_domain_path);

  g_clear_pointer (&priv->help_domain_name, g_free);
  g_clear_object  (&priv->help_domain_uri);

  clay_plug_in_destroy_proxies (plug_in, priv->displays,  "display",  TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->images,    "image",    TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->items,     "item",     TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->filters,   "filters",  TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->resources, "resource", TRUE);

  clay_plug_in_destroy_hashes (plug_in);
  g_clear_list (&priv->ran_procedure_stack, g_object_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_plug_in_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClayPlugIn        *plug_in = CLAY_PLUG_IN (object);
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  switch (property_id)
    {
    case PROP_PROGRAM_NAME:
      priv->program_name = g_value_dup_string (value);
      break;

    case PROP_READ_CHANNEL:
      priv->read_channel = g_value_get_boxed (value);
      break;

    case PROP_WRITE_CHANNEL:
      priv->write_channel = g_value_get_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_plug_in_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClayPlugIn        *plug_in = CLAY_PLUG_IN (object);
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  switch (property_id)
    {
    case PROP_READ_CHANNEL:
      g_value_set_boxed (value, priv->read_channel);
      break;

    case PROP_WRITE_CHANNEL:
      g_value_set_boxed (value, priv->write_channel);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_plug_in_real_set_i18n (ClayPlugIn   *plug_in,
                            const gchar  *procedure_name,
                            gchar       **gettext_domain,
                            gchar       **catalog_dir)
{
  /* Default to enabling localization by gettext. It will have the good
   * side-effect of warning plug-in developers of the existence of the
   * ability through stderr if the catalog directory is missing.
   */
  return TRUE;
}


/*  public functions  */

/**
 * clay_plug_in_set_help_domain:
 * @plug_in:     A #ClayPlugIn.
 * @domain_name: The XML namespace of the plug-in's help pages.
 * @domain_uri:  The root URI of the plug-in's help pages.
 *
 * Set a help domain and path for the @plug_in.
 *
 * This function registers user documentation for the calling plug-in
 * with the Clay help system. The @domain_uri parameter points to the
 * root directory where the plug-in help is installed. For each
 * supported language there should be a file called 'clay-help.xml'
 * that maps the help IDs to the actual help files.
 *
 * This function can only be called in the
 * [vfunc@PlugIn.query_procedures] function of a plug-in.
 *
 * Since: 3.0
 **/
void
clay_plug_in_set_help_domain (ClayPlugIn  *plug_in,
                              const gchar *domain_name,
                              GFile       *domain_uri)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (G_IS_FILE (domain_uri));

  priv = clay_plug_in_get_instance_private (plug_in);

  g_free (priv->help_domain_name);
  priv->help_domain_name = g_strdup (domain_name);

  g_set_object (&priv->help_domain_uri, domain_uri);
}

/**
 * clay_plug_in_add_menu_branch:
 * @plug_in:    A #ClayPlugIn
 * @menu_path:  The sub-menu's menu path.
 * @menu_label: The menu label of the sub-menu.
 *
 * This function is a no-op and should not be used anymore.
 *
 * Since: 3.0
 *
 * Deprecated: 3.2.8: Use [method@Clay.Procedure.add_menu_path].
 **/
void
clay_plug_in_add_menu_branch (ClayPlugIn  *plug_in,
                              const gchar *menu_path,
                              const gchar *menu_label)
{
}

/**
 * clay_plug_in_add_temp_procedure:
 * @plug_in:   A #ClayPlugIn
 * @procedure: A #ClayProcedure of type %CLAY_PDB_PROC_TYPE_TEMPORARY.
 *
 * This function adds a temporary procedure to @plug_in. It is usually
 * called from a %CLAY_PDB_PROC_TYPE_PERSISTENT procedure's
 * [vfunc@Procedure.run].
 *
 * A temporary procedure is a procedure which is only available while
 * one of your plug-in's "real" procedures is running.
 *
 * The procedure's type _must_ be
 * %CLAY_PDB_PROC_TYPE_TEMPORARY or the function will fail.
 *
 * NOTE: Normally, plug-in communication is triggered by the plug-in
 * and the Clay core only responds to the plug-in's requests. You must
 * explicitly enable receiving of temporary procedure run requests
 * using either [method@PlugIn.persistent_enable] or
 * [method@PlugIn.persistent_process]. See their respective
 * documentation for details.
 *
 * Since: 3.0
 **/
void
clay_plug_in_add_temp_procedure (ClayPlugIn    *plug_in,
                                 ClayProcedure *procedure)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (CLAY_IS_PROCEDURE (procedure));
  g_return_if_fail (clay_procedure_get_proc_type (procedure) ==
                    CLAY_PDB_PROC_TYPE_TEMPORARY);

  priv = clay_plug_in_get_instance_private (plug_in);

  priv->temp_procedures =
    g_list_prepend (priv->temp_procedures,
                    g_object_ref (procedure));

  CLAY_PROCEDURE_GET_CLASS (procedure)->install (procedure);
}

/**
 * clay_plug_in_remove_temp_procedure:
 * @plug_in:        A #ClayPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function removes a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Since: 3.0
 **/
void
clay_plug_in_remove_temp_procedure (ClayPlugIn  *plug_in,
                                    const gchar *procedure_name)
{
  ClayProcedure *procedure;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (clay_is_canonical_identifier (procedure_name));

  procedure = clay_plug_in_get_temp_procedure (plug_in, procedure_name);

  if (procedure)
    {
      ClayPlugInPrivate *priv;

      CLAY_PROCEDURE_GET_CLASS (procedure)->uninstall (procedure);

      priv = clay_plug_in_get_instance_private (plug_in);

      priv->temp_procedures = g_list_remove (priv->temp_procedures, procedure);
      if (! g_list_find (priv->ran_procedure_stack, procedure))
        g_object_unref (procedure);
    }
}

/**
 * clay_plug_in_get_temp_procedures:
 * @plug_in: A plug-in
 *
 * This function retrieves the list of temporary procedure of @plug_in as
 * added with [method@PlugIn.add_temp_procedure].
 *
 * Returns: (transfer none) (element-type ClayProcedure): The list of
 *          procedures.
 *
 * Since: 3.0
 **/
GList *
clay_plug_in_get_temp_procedures (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  return priv->temp_procedures;
}

/**
 * clay_plug_in_get_temp_procedure:
 * @plug_in:        A #ClayPlugIn
 * @procedure_name: The name of a [class@Procedure] added to @plug_in.
 *
 * This function retrieves a temporary procedure from @plug_in by the
 * procedure's @procedure_name.
 *
 * Returns: (nullable) (transfer none): The procedure if registered, or %NULL.
 *
 * Since: 3.0
 **/
ClayProcedure *
clay_plug_in_get_temp_procedure (ClayPlugIn  *plug_in,
                                 const gchar *procedure_name)
{
  ClayPlugInPrivate *priv;
  GList             *list;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (procedure_name), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  for (list = priv->temp_procedures; list; list = g_list_next (list))
    {
      ClayProcedure *procedure = list->data;

      if (! strcmp (procedure_name, clay_procedure_get_name (procedure)))
        return procedure;
    }

  return NULL;
}

/**
 * clay_plug_in_persistent_enable:
 * @plug_in: A plug-in
 *
 * Enables asynchronous processing of messages from the main Clay
 * application.
 *
 * Normally, a plug-in is not called by Clay except for the call to
 * the procedure it implements. All subsequent communication is
 * triggered by the plug-in and all messages sent from Clay to the
 * plug-in are just answers to requests the plug-in made.
 *
 * If the plug-in however registered temporary procedures using
 * [method@PlugIn.add_temp_procedure], it needs to be able to receive
 * requests to execute them. Usually this will be done by running
 * [method@PlugIn.persistent_process] in an endless loop.
 *
 * If the plug-in cannot use [method@PlugIn.persistent_process], i.e. if
 * it has a GUI and is hanging around in a [struct@GLib.MainLoop], it
 * must call [method@PlugIn.persistent_enable].
 *
 * Note that the plug-in does not need to be a
 * [enum@Clay.PDBProcType.PERSISTENT] to register temporary procedures.
 *
 * See also: [method@PlugIn.add_temp_procedure].
 *
 * Since: 3.0
 **/
void
clay_plug_in_persistent_enable (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  if (! priv->persistent_source_id)
    {
      priv->persistent_source_id =
        g_io_add_watch (priv->read_channel, G_IO_IN | G_IO_PRI,
                        clay_plug_in_persistent_read,
                        plug_in);

      /* #12631
       * Similar code in app/plug-in/clayplugin.c.
       *
       * The sequence of events and calls which requires set can_recurse
       * on the IO source of events:
       *
       * - A user choosing a menu item implemented by persistent plugin script-fu
       *   writes a "run temp proc" msg to the pipe to the plugin.
       * - On the plugin side, Glib generates an event which invokes the
       *   handler plug_in_persistent_read, of type GIOFunc .
       * - The handler ultimately shows a dialog having its own event loop.
       * - When the dialog has a resource select widget and the user clicks one,
       *   the plugin creates a temporary PDB procedure for a callback,
       *   and calls a PDB procedure to open a remote resource chooser widget
       *   in the app, passing the name of the callback.
       * - The user choosing a resource in the remote chooser widget
       *   invokes the callback by writing a second "run temp proc" msg
       *   to the pipe to the plugin.
       * - Unless can_recurse is set, the second write (the callback) by the app
       *   is blocked and does not generate an event in the plugin.
       * - The dialog event loop receives no event, doesn't read the pipe,
       *   and fails to update its resource select widget with the user's choice.
       *   The message in the pipe is then unexpected by the plugin
       *   for example when the user OKs the dialog.
       */
      g_source_set_can_recurse (
        g_main_context_find_source_by_id (NULL, priv->persistent_source_id),
        TRUE);
    }
}

/**
 * clay_plug_in_persistent_process:
 * @plug_in: A plug-in.
 * @timeout: The timeout (in ms) to use for the select() call.
 *
 * Processes one message sent by Clay and returns.
 *
 * Call this function in an endless loop after calling
 * [method@Clay.Procedure.persistent_ready] to process requests for
 * running temporary procedures.
 *
 * See [method@PlugIn.persistent_enable] for an asynchronous way of
 * doing the same if running an endless loop is not an option.
 *
 * See also: [method@PlugIn.add_temp_procedure].
 *
 * Since: 3.0
 **/
void
clay_plug_in_persistent_process (ClayPlugIn *plug_in,
                                 guint       timeout)
{
  ClayPlugInPrivate *priv;
#ifndef G_OS_WIN32
  gint               select_val;
#else
  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD            pollfd;
#endif

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

#ifndef G_OS_WIN32
  do
    {
      fd_set readfds;
      struct timeval  tv;
      struct timeval *tvp;

      if (timeout)
        {
          tv.tv_sec  = timeout / 1000;
          tv.tv_usec = (timeout % 1000) * 1000;
          tvp = &tv;
        }
      else
        tvp = NULL;

      FD_ZERO (&readfds);
      FD_SET (g_io_channel_unix_get_fd (priv->read_channel),
              &readfds);

      if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
        {
          clay_plug_in_single_message (plug_in);
        }
      else if (select_val == -1 && errno != EINTR)
        {
          perror ("clay_plug_in_persistent_process");
          _clay_quit ();
        }
    }
  while (select_val == -1 && errno == EINTR);

#else
  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (priv->read_channel, G_IO_IN,
                                  &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    {
      clay_plug_in_single_message (plug_in);
    }

#endif
}

/**
 * clay_plug_in_set_pdb_error_handler:
 * @plug_in: A plug-in
 * @handler: Who is responsible for handling procedure call errors.
 *
 * Sets an error handler for procedure calls.
 *
 * This procedure changes the way that errors in procedure calls are
 * handled. By default Clay will raise an error dialog if a procedure
 * call made by a plug-in fails. Using this procedure the plug-in can
 * change this behavior. If the error handler is set to
 * %CLAY_PDB_ERROR_HANDLER_PLUGIN, then the plug-in is responsible for
 * calling clay_pdb_get_last_error() and handling the error whenever
 * one if its procedure calls fails. It can do this by displaying the
 * error message or by forwarding it in its own return values.
 *
 * Since: 3.0
 **/
void
clay_plug_in_set_pdb_error_handler (ClayPlugIn          *plug_in,
                                    ClayPDBErrorHandler  handler)
{
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  _clay_plug_in_set_pdb_error_handler (handler);
}

/**
 * clay_plug_in_get_pdb_error_handler:
 * @plug_in: A plug-in
 *
 * Retrieves the active error handler for procedure calls.
 *
 * This procedure retrieves the currently active error handler for
 * procedure calls made by the calling plug-in. See
 * clay_plugin_set_pdb_error_handler() for details.
 *
 * Returns: Who is responsible for handling procedure call errors.
 *
 * Since: 3.0
 **/
ClayPDBErrorHandler
clay_plug_in_get_pdb_error_handler (ClayPlugIn *plug_in)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in),
                        CLAY_PDB_ERROR_HANDLER_INTERNAL);

  return _clay_plug_in_get_pdb_error_handler ();
}


/*  internal functions  */

void
_clay_plug_in_query (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  if (CLAY_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    gp_has_init_write (priv->write_channel, plug_in);

  if (CLAY_PLUG_IN_GET_CLASS (plug_in)->query_procedures)
    {
      GList *procedures =
        CLAY_PLUG_IN_GET_CLASS (plug_in)->query_procedures (plug_in);

      clay_plug_in_register (plug_in, procedures);
    }
}

void
_clay_plug_in_init (ClayPlugIn *plug_in)
{
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  if (CLAY_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    {
      GList *procedures =
        CLAY_PLUG_IN_GET_CLASS (plug_in)->init_procedures (plug_in);

      clay_plug_in_register (plug_in, procedures);
    }
}

void
_clay_plug_in_run (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  g_io_add_watch (priv->read_channel,
                  G_IO_ERR | G_IO_HUP,
                  clay_plug_in_io_error_handler,
                  NULL);

  clay_plug_in_loop (plug_in);
}

void
_clay_plug_in_quit (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  if (CLAY_PLUG_IN_GET_CLASS (plug_in)->quit)
    CLAY_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);

  _clay_shm_close ();

  /* A successful GP_PROC_RETURN message would have already triggered
   * the main process to close the plug-in's IO channels. Sending a
   * GP_QUIT now would at best be useless, or at worst lead to a channel
   * write "Broken pipe" error.
   */
  if (! priv->returned)
    gp_quit_write (priv->write_channel, plug_in);
}

GIOChannel *
_clay_plug_in_get_read_channel (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  return priv->read_channel;
}

GIOChannel *
_clay_plug_in_get_write_channel (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  return priv->write_channel;
}

void
_clay_plug_in_read_expect_msg (ClayPlugIn      *plug_in,
                               ClayWireMessage *msg,
                               gint             type)
{
  ClayPlugInPrivate *priv;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  while (TRUE)
    {
      if (! clay_wire_read_msg (priv->read_channel, msg, NULL))
        _clay_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          clay_plug_in_process_message (plug_in, msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      clay_wire_destroy (msg);
    }
}

gboolean
_clay_plug_in_set_i18n (ClayPlugIn   *plug_in,
                        const gchar  *procedure_name,
                        gchar       **gettext_domain,
                        gchar       **catalog_dir)
{
  ClayPlugInPrivate *priv;
  gboolean           use_gettext;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (gettext_domain && *gettext_domain == NULL, FALSE);
  g_return_val_if_fail (catalog_dir && *catalog_dir == NULL, FALSE);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (! priv->translation_domain_path ||
      ! priv->translation_domain_name)
    clay_plug_in_init_i18n (plug_in);

  if (! CLAY_PLUG_IN_GET_CLASS (plug_in)->set_i18n)
    {
      use_gettext = FALSE;
    }
  else
    {
      gchar *utf8_catalog_dir = NULL;

      use_gettext = CLAY_PLUG_IN_GET_CLASS (plug_in)->set_i18n (plug_in,
                                                                procedure_name,
                                                                gettext_domain,
                                                                &utf8_catalog_dir);
      if (use_gettext)
        {
          gboolean reserved = FALSE;

          if (*gettext_domain == NULL)
            {
              *gettext_domain = g_strdup (priv->translation_domain_name);
            }
          else if (g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-std-plug-ins") == 0 ||
                   g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-script-fu") == 0 ||
                   g_strcmp0 (*gettext_domain, GETTEXT_PACKAGE "-python") == 0)
            {
              /* We special-case these 3 domains as the only ones where
               * it is allowed to set an absolute system dir (actually
               * set by the lib itself; devs must set NULL). See docs of
               * set_i18n() method.
               */
              if (utf8_catalog_dir != NULL)
                g_printerr ("[%s] Do not set a catalog directory with set_i18n() with reserved domain: %s\n",
                            procedure_name, *gettext_domain);

              *catalog_dir = g_strdup (clay_locale_directory ());
              reserved = TRUE;
            }

          if (utf8_catalog_dir != NULL && *catalog_dir == NULL)
            {
              GError *error = NULL;

              /* The passed-on catalog directory is in UTF-8 because this is
               * usually hardcoded (in which case it's easier to request a
               * specific encoding, chosen at development time, rather than the
               * "OS encoding", depending on runtime).
               * But now we want to transform it to the encoding used for
               * filenames by GLib.
               */
              *catalog_dir = g_filename_from_utf8 (utf8_catalog_dir, -1, NULL, NULL, &error);

              if (*catalog_dir == NULL)
                g_printerr ("[%s] provided catalog directory is not proper UTF-8: %s\n",
                            procedure_name, error ? error->message : "(N/A)");

              g_clear_error (&error);
            }

          if (*catalog_dir && ! reserved)
            {
              if (g_path_is_absolute (*catalog_dir))
                {
                  g_printerr ("[%s] The catalog directory set by set_i18n() is not relative: %s\n",
                              procedure_name, *catalog_dir);
                  g_printerr ("[%s] Localization disabled\n", procedure_name);

                  use_gettext = FALSE;
                }
              else
                {
                  gchar *rootdir   = g_path_get_dirname (clay_get_progname ());
                  GFile *root_file = g_file_new_for_path (rootdir);
                  GFile *catalog_file;
                  GFile *parent_p  = NULL;
                  GFile *parent;

                  catalog_file = g_file_resolve_relative_path (root_file, *catalog_dir);

                  /* Verify that the catalog is a subdir of the plug-in folder.
                   * We do not want to allow plug-ins to look outside their own
                   * realm.
                   */
                  parent = g_file_dup (catalog_file);
                  do
                    {
                      g_clear_object (&parent_p);
                      if (g_file_equal (parent, root_file))
                        break;
                      parent_p = parent;
                    }
                  while ((parent = g_file_get_parent (parent)));

                  if (parent == NULL)
                    {
                      g_printerr ("[%s] The catalog directory set by set_i18n() is not a subdirectory: %s\n",
                                  procedure_name, *catalog_dir);
                      g_printerr ("[%s] Localization disabled\n", procedure_name);

                      use_gettext = FALSE;
                    }

                  g_free (*catalog_dir);
                  *catalog_dir = g_file_get_path (catalog_file);

                  g_free (rootdir);
                  g_object_unref (root_file);
                  g_clear_object (&parent);
                  g_clear_object (&parent_p);
                  g_object_unref (catalog_file);
                }
            }
          else if (! *catalog_dir)
            {
              *catalog_dir = g_file_get_path (priv->translation_domain_path);
            }
        }

      g_clear_pointer (&utf8_catalog_dir, g_free);
    }

  if (use_gettext && ! g_file_test (*catalog_dir, G_FILE_TEST_IS_DIR))
    {
      g_printerr ("[%s] The catalog directory does not exist: %s\n",
                  procedure_name, *catalog_dir);
      g_printerr ("[%s] Override method set_i18n() for the plug-in to customize or disable localization.\n",
                  procedure_name);
      g_printerr ("[%s] Localization disabled\n", procedure_name);

      use_gettext = FALSE;
    }

  if (! use_gettext)
    {
      g_clear_pointer (gettext_domain, g_free);
      g_clear_pointer (catalog_dir, g_free);
    }

  return use_gettext;
}

ClayProcedure *
_clay_plug_in_create_procedure (ClayPlugIn  *plug_in,
                                const gchar *procedure_name)
{
  gchar *gettext_domain = NULL;
  gchar *catalog_dir    = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (procedure_name), NULL);

  if (_clay_plug_in_set_i18n (plug_in, procedure_name, &gettext_domain, &catalog_dir))
    {
      clay_bind_text_domain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  if (CLAY_PLUG_IN_GET_CLASS (plug_in)->create_procedure)
    return CLAY_PLUG_IN_GET_CLASS (plug_in)->create_procedure (plug_in,
                                                               procedure_name);

  return NULL;
}


/*  private functions  */

static void
clay_plug_in_register (ClayPlugIn *plug_in,
                       GList      *procedures)
{
  ClayPlugInPrivate *priv;
  GList             *list;

  for (list = procedures; list; list = g_list_next (list))
    {
      const gchar   *name = list->data;
      ClayProcedure *procedure;

      procedure = _clay_plug_in_create_procedure (plug_in, name);
      if (procedure)
        {
          CLAY_PROCEDURE_GET_CLASS (procedure)->install (procedure);
          g_object_unref (procedure);
        }
      else
        {
          g_warning ("Plug-in failed to create procedure '%s'\n", name);
        }
    }

  g_list_free_full (procedures, g_free);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (priv->help_domain_name)
    {
      _clay_plug_in_help_register (priv->help_domain_name,
                                   priv->help_domain_uri);
    }
}

static gboolean
clay_plug_in_write (GIOChannel   *channel,
                    const guint8 *buf,
                    gulong        count,
                    gpointer      user_data)
{
  ClayPlugIn        *plug_in = user_data;
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  g_return_val_if_fail (priv->returned == FALSE, FALSE);

  while (count > 0)
    {
      gulong bytes;

      if ((priv->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - priv->write_buffer_index;
          memcpy (&priv->write_buffer[priv->write_buffer_index],
                  buf, bytes);
          priv->write_buffer_index += bytes;

          if (! clay_wire_flush (channel, plug_in))
            return FALSE;
        }
      else
        {
          bytes = count;
          memcpy (&priv->write_buffer[priv->write_buffer_index],
                  buf, bytes);
          priv->write_buffer_index += bytes;
        }

      buf   += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
clay_plug_in_flush (GIOChannel *channel,
                    gpointer    user_data)
{
  ClayPlugIn        *plug_in = user_data;
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  if (priv->write_buffer_index > 0)
    {
      gsize count = 0;

      while (count != priv->write_buffer_index)
        {
          GIOStatus status;
          gsize     bytes;
          GError   *error = NULL;

          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 &priv->write_buffer[count],
                                                 (priv->write_buffer_index - count),
                                                 &bytes,
                                                 &error);
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (error)
                {
                  g_warning ("%s: clay_flush(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: clay_flush(): error", g_get_prgname ());
                }

              return FALSE;
            }

          count += bytes;
        }

      priv->write_buffer_index = 0;
    }

  return TRUE;
}

static gboolean
clay_plug_in_io_error_handler (GIOChannel   *channel,
                               GIOCondition  cond,
                               gpointer      data)
{
  g_printerr ("%s: fatal error: Clay crashed\n", clay_get_progname ());
  _clay_quit ();

  /* never reached */
  return TRUE;
}

static void
clay_plug_in_loop (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  priv = clay_plug_in_get_instance_private (plug_in);

  while (TRUE)
    {
      ClayWireMessage msg;

      if (! clay_wire_read_msg (priv->read_channel, &msg, NULL))
        return;

      switch (msg.type)
        {
        case GP_QUIT:
          clay_wire_destroy (&msg);
          return;

        case GP_CONFIG:
          _clay_config (msg.data);
          break;

        case GP_TILE_REQ:
        case GP_TILE_ACK:
        case GP_TILE_DATA:
          g_warning ("unexpected tile message received (should not happen)");
          break;

        case GP_PROC_RUN:
          clay_plug_in_main_proc_run (plug_in, msg.data);
          clay_wire_destroy (&msg);
          return;

        case GP_PROC_RETURN:
          g_warning ("unexpected proc return message received (should not happen)");
          break;

        case GP_TEMP_PROC_RUN:
          g_warning ("unexpected temp proc run message received (should not happen");
          break;

        case GP_TEMP_PROC_RETURN:
          g_warning ("unexpected temp proc return message received (should not happen");
          break;

        case GP_PROC_INSTALL:
          g_warning ("unexpected proc install message received (should not happen)");
          break;

        case GP_HAS_INIT:
          g_warning ("unexpected has init message received (should not happen)");
          break;
        }

      clay_wire_destroy (&msg);
    }
}

static void
clay_plug_in_single_message (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;
  ClayWireMessage    msg;

  priv = clay_plug_in_get_instance_private (plug_in);

  /* Run a temp function */
  if (! clay_wire_read_msg (priv->read_channel, &msg, NULL))
    _clay_quit ();

  clay_plug_in_process_message (plug_in, &msg);

  clay_wire_destroy (&msg);
}

static void
clay_plug_in_process_message (ClayPlugIn      *plug_in,
                              ClayWireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      _clay_quit ();
      break;
    case GP_CONFIG:
      _clay_config (msg->data);
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received (should not happen)");
      break;
    case GP_PROC_RUN:
      g_warning ("unexpected proc run message received (should not happen)");
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received (should not happen)");
      break;
    case GP_TEMP_PROC_RUN:
      clay_plug_in_temp_proc_run (plug_in, msg->data);
      break;
    case GP_TEMP_PROC_RETURN:
      g_warning ("unexpected temp proc return message received (should not happen)");
      break;
    case GP_PROC_INSTALL:
      g_warning ("unexpected proc install message received (should not happen)");
      break;
    case GP_HAS_INIT:
      g_warning ("unexpected has init message received (should not happen)");
      break;
    }
}

/* Run a proc that is main, i.e. root of a plugin call stack. */
static void
clay_plug_in_main_proc_run (ClayPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  ClayPlugInPrivate *priv;
  GPProcReturn       proc_return;
  ClayProcedure     *procedure;

  procedure = _clay_plug_in_create_procedure (plug_in, proc_run->name);
  priv      = clay_plug_in_get_instance_private (plug_in);

  if (procedure)
    clay_plug_in_proc_run_internal (plug_in,
                                    proc_run, procedure,
                                    &proc_return);

  clay_plug_in_main_run_cleanup (plug_in);

  if (! gp_proc_return_write (priv->write_channel,
                              &proc_return, plug_in))
    _clay_quit ();
  else
    priv->returned = TRUE;

  _clay_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

static void
clay_plug_in_temp_proc_run (ClayPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  ClayPlugInPrivate *priv;
  GPProcReturn       proc_return;
  ClayProcedure     *procedure;

  procedure = clay_plug_in_get_temp_procedure (plug_in, proc_run->name);
  priv      = clay_plug_in_get_instance_private (plug_in);

  if (procedure)
    clay_plug_in_proc_run_internal (plug_in,
                                    proc_run, procedure,
                                    &proc_return);

  clay_plug_in_temp_run_cleanup (plug_in);

  if (! gp_temp_proc_return_write (priv->write_channel,
                                   &proc_return, plug_in))
    _clay_quit ();

  _clay_gp_params_free (proc_return.params, proc_return.n_params, TRUE);
}

/* Run the proc, passing args from proc_run
 * and returning values in proc_return.
 * This does not alter the state of the ClayPlugin.
 */
static void
clay_plug_in_proc_run (GPProcRun     *proc_run,
                       ClayProcedure *procedure,
                       GPProcReturn  *proc_return)
{
  ClayValueArray *arguments;
  ClayValueArray *return_values  = NULL;

  arguments = _clay_gp_params_to_value_array (NULL,
                                              NULL, 0,
                                              proc_run->params,
                                              proc_run->n_params,
                                              FALSE);

  return_values = _clay_procedure_run_array (procedure, arguments);

  clay_value_array_unref (arguments);

  proc_return->name     = proc_run->name;
  proc_return->n_params = clay_value_array_length (return_values);
  proc_return->params   = _clay_value_array_to_gp_params (return_values, TRUE);

  clay_value_array_unref (return_values);
}


/* Setup translation, maintain proc stack, and run proc.
 * Proc is a main or temp proc.
 * "internal" means private, not that the proc is type INTERNAL.
 */
static void
clay_plug_in_proc_run_internal (ClayPlugIn    *plug_in,
                                GPProcRun     *proc_run,
                                ClayProcedure *procedure,
                                GPProcReturn  *proc_return)
{
  gchar *gettext_domain = NULL;
  gchar *catalog_dir    = NULL;

  if (_clay_plug_in_set_i18n (plug_in, clay_procedure_get_name (procedure),
                              &gettext_domain, &catalog_dir))
    {
      clay_bind_text_domain (gettext_domain, catalog_dir);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (gettext_domain, "UTF-8");
#endif
      textdomain (gettext_domain);

      g_free (gettext_domain);
      g_free (catalog_dir);
    }

  clay_plug_in_push_procedure (plug_in, procedure);

  clay_plug_in_proc_run (proc_run, procedure, proc_return);

  clay_plug_in_pop_procedure (plug_in, procedure);
}

static gboolean
clay_plug_in_persistent_read (GIOChannel  *channel,
                              GIOCondition condition,
                              gpointer     data)
{
  ClayPlugIn *plug_in = data;

  clay_plug_in_single_message (plug_in);

  return G_SOURCE_CONTINUE;
}


/*  procedure stack / display-, image-, item-cache  */


ClayProcedure *
_clay_plug_in_get_procedure (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  g_return_val_if_fail (priv->procedure_stack != NULL, NULL);

  return priv->procedure_stack->data;
}

static gboolean
clay_plug_in_is_procedure_stack_empty (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  return (priv->procedure_stack == NULL);
}

static void
clay_plug_in_push_procedure (ClayPlugIn    *plug_in,
                             ClayProcedure *procedure)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  priv->procedure_stack =
    g_list_prepend (priv->procedure_stack, procedure);
}

/* After a run of a main proc, cleanup.
 * We are about to return to Clay or another calling plugin process.
 * This plugin process will soon terminate.
 *
 * Expect the proc stack is empty: don't destroy proxies
 * when there are still calling procs that might have a reference.
 */
static void
clay_plug_in_main_run_cleanup (ClayPlugIn *plug_in)
{
  if (clay_plug_in_is_procedure_stack_empty (plug_in))
    {
      g_debug ("%s proc stack empty, destroy proxies.", G_STRFUNC);
      clay_plug_in_destroy_all_proxies (plug_in);
      clay_plug_in_destroy_hashes (plug_in);
    }
  else
    {
      /* Unexpected. */
      g_warning ("%s proc stack not empty when main proc returns.", G_STRFUNC);
    }
}

/* After a run of a temp proc, cleanup.
 * We are about to return to another calling proc.
 *
 * When the just-run temp proc is returning to top proc on stack and it
 * is a persistent plug-in, cleanup is destroy the plugin's proxies.
 * The proc stack is never empty for a persistent plug-in: the top is
 * e.g. extension-script-fu, which must not reference proxies.
 */
static void
clay_plug_in_temp_run_cleanup (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  /* When at top of proc stack and top is a persistent plug-in, destroy proxies. */
  if ((g_list_length (priv->procedure_stack) == 1) &&
      (clay_procedure_get_proc_type (priv->procedure_stack->data) == CLAY_PDB_PROC_TYPE_PERSISTENT))
    {
      g_debug ("%s top of proc stack is a persistent plug-in, destroy proxies.", G_STRFUNC);
      clay_plug_in_destroy_all_proxies (plug_in);
      clay_plug_in_destroy_hashes (plug_in);
    }
  else
    {
      /* Normal.  The temp proc just run was called by a calling proc
       * which is not a main proc of a persistent plug-in.
       * We can't destroy proxies because the calling proc may retain a reference.
       */
      g_debug ("%s Not destroy proxies for temp proc.", G_STRFUNC);
    }
}

static void
clay_plug_in_pop_procedure (ClayPlugIn    *plug_in,
                            ClayProcedure *procedure)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  priv->procedure_stack     = g_list_remove (priv->procedure_stack, procedure);
  if (! g_list_find (priv->ran_procedure_stack, procedure))
    priv->ran_procedure_stack = g_list_prepend (priv->ran_procedure_stack, procedure);

  /* Don't destroy proxies now because any proc, especially temporary procs,
   * may have passed a reference to a proc higher in the stack e.g. the main procedure.
   * We don't have separate proxy hashes for each pushed procedure,
   * only a hash table for the run.
   */
}

ClayDisplay *
_clay_plug_in_get_display (ClayPlugIn *plug_in,
                           gint32      display_id)
{
  ClayPlugInPrivate *priv;

  ClayDisplay *display = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->displays))
    priv->displays =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  display = g_hash_table_lookup (priv->displays,
                                 GINT_TO_POINTER (display_id));

  if (! display)
    {
      display = g_object_new (CLAY_TYPE_DISPLAY,
                              "id", display_id,
                              NULL);

      g_hash_table_insert (priv->displays,
                           GINT_TO_POINTER (display_id),
                           display);
    }

  return display;
}

ClayImage *
_clay_plug_in_get_image (ClayPlugIn *plug_in,
                         gint32      image_id)
{
  ClayPlugInPrivate *priv;
  ClayImage         *image = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->images))
    priv->images =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  image = g_hash_table_lookup (priv->images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    {
      image = g_object_new (CLAY_TYPE_IMAGE,
                            "id", image_id,
                            NULL);

      g_hash_table_insert (priv->images,
                           GINT_TO_POINTER (image_id),
                           image);
    }

  return image;
}

ClayItem *
_clay_plug_in_get_item (ClayPlugIn *plug_in,
                        gint32      item_id)
{
  ClayPlugInPrivate *priv;
  ClayItem          *item = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->items))
    priv->items =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  item = g_hash_table_lookup (priv->items,
                              GINT_TO_POINTER (item_id));

  if (! item)
    {
      if (clay_item_id_is_text_layer (item_id))
        {
          item = g_object_new (CLAY_TYPE_TEXT_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_vector_layer (item_id))
        {
          item = g_object_new (CLAY_TYPE_VECTOR_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_link_layer (item_id))
        {
          item = g_object_new (CLAY_TYPE_LINK_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_group_layer (item_id))
        {
          item = g_object_new (CLAY_TYPE_GROUP_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_layer (item_id))
        {
          item = g_object_new (CLAY_TYPE_LAYER,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_layer_mask (item_id))
        {
          item = g_object_new (CLAY_TYPE_LAYER_MASK,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_selection (item_id))
        {
          item = g_object_new (CLAY_TYPE_SELECTION,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_channel (item_id))
        {
          item = g_object_new (CLAY_TYPE_CHANNEL,
                               "id", item_id,
                               NULL);
        }
      else if (clay_item_id_is_path (item_id))
        {
          item = g_object_new (CLAY_TYPE_PATH,
                               "id", item_id,
                               NULL);
        }

      if (item)
        g_hash_table_insert (priv->items,
                             GINT_TO_POINTER (item_id),
                             item);
    }

  return item;
}

ClayDrawableFilter *
_clay_plug_in_get_filter (ClayPlugIn *plug_in,
                          gint32      filter_id)
{
  ClayPlugInPrivate  *priv;
  ClayDrawableFilter *filter = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->filters))
    priv->filters =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  filter = g_hash_table_lookup (priv->filters, GINT_TO_POINTER (filter_id));

  if (! filter)
    {
      filter = g_object_new (CLAY_TYPE_DRAWABLE_FILTER,
                             "id", filter_id,
                             NULL);

      g_hash_table_insert (priv->filters,
                           GINT_TO_POINTER (filter_id),
                           filter);
    }

  return filter;
}

ClayResource *
_clay_plug_in_get_resource (ClayPlugIn *plug_in,
                            gint32      resource_id)
{
  ClayPlugInPrivate *priv;
  ClayResource      *resource = NULL;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  priv = clay_plug_in_get_instance_private (plug_in);

  if (G_UNLIKELY (! priv->resources))
    priv->resources =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  resource = g_hash_table_lookup (priv->resources,
                                  GINT_TO_POINTER (resource_id));

  if (! resource)
    {
      if (clay_resource_id_is_brush (resource_id))
        {
          resource = g_object_new (CLAY_TYPE_BRUSH,
                                   "id", resource_id,
                                   NULL);
        }
      else if (clay_resource_id_is_pattern (resource_id))
        {
          resource = g_object_new (CLAY_TYPE_PATTERN,
                                   "id", resource_id,
                                   NULL);
        }
      else if (clay_resource_id_is_gradient (resource_id))
        {
          resource = g_object_new (CLAY_TYPE_GRADIENT,
                                   "id", resource_id,
                                   NULL);
        }
      else if (clay_resource_id_is_palette (resource_id))
        {
          resource = g_object_new (CLAY_TYPE_PALETTE,
                                   "id", resource_id,
                                   NULL);
        }
      else if (clay_resource_id_is_font (resource_id))
        {
          resource = g_object_new (CLAY_TYPE_FONT,
                                   "id", resource_id,
                                   NULL);
        }

      if (resource)
        g_hash_table_insert (priv->resources,
                             GINT_TO_POINTER (resource_id),
                             resource);
    }

  return resource;
}

gboolean
_clay_plug_in_manage_memory_manually (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;
  gboolean           manual_management = TRUE;

  priv = clay_plug_in_get_instance_private (plug_in);

  if (priv->program_name)
    {
      GFile *file = g_file_new_for_path (priv->program_name);

      /* Limitations:
       * 1. Checking a file extension (and trusting argv[0] in general) is not
       *    the best idea. I thought about the possibility of passing through
       *    the name of the used interpreter from core to plug-in, but even this
       *    should not be trusted because it is configurable.
       * 2. An alternative could be to pass the information if a plug-in is
       *    interpreted (i.e. it has an interp file). Though I'm unsure if there
       *    might be some interpreted language out there with manual memory
       *    management.
       * 3. This list only contains the bindings we actually tested with (and
       *    have demo plug-ins for). Much more bindings are possible.
       *
       * So in the end, this test is just worth so far, but it's
       * acceptable for the officially supported bindings at least.
       */
      if (clay_file_has_extension (file, ".lua") ||
          clay_file_has_extension (file, ".py")  ||
          clay_file_has_extension (file, ".js")  ||
          clay_file_has_extension (file, ".gjs"))
        manual_management = FALSE;

      g_object_unref (file);
    }

  return manual_management;
}

static void
clay_plug_in_destroy_hashes (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  g_clear_pointer (&priv->displays,  g_hash_table_unref);
  g_clear_pointer (&priv->images,    g_hash_table_unref);
  g_clear_pointer (&priv->items,     g_hash_table_unref);
  g_clear_pointer (&priv->resources, g_hash_table_unref);
}

/* Destroy proxies of all kinds.
 * This destroys with prejudice, i.e. destroy_all==TRUE.
 * All procedures, main and temporary, of the plugin must not be run subsequently,
 * especially to reference a proxy we are destroying.
 */
static void
clay_plug_in_destroy_all_proxies (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv = clay_plug_in_get_instance_private (plug_in);

  clay_plug_in_destroy_proxies (plug_in, priv->displays,  "display",  TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->images,    "image",    TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->items,     "item",     TRUE);
  clay_plug_in_destroy_proxies (plug_in, priv->resources, "resource", TRUE);
}

static void
clay_plug_in_destroy_proxies (ClayPlugIn  *plug_in,
                              GHashTable  *hash_table,
                              const gchar *type,
                              gboolean     destroy_all)
{
  ClayPlugInPrivate *priv;
  GHashTableIter     iter;
  gpointer           key, value;

  if (! hash_table)
    return;

  priv = clay_plug_in_get_instance_private (plug_in);

  g_hash_table_iter_init (&iter, hash_table);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GObject *object = value;
      gint     ref_count = 0;

      for (GList *list = priv->ran_procedure_stack; list; list = list->next)
        ref_count += _clay_procedure_get_ref_count (list->data, object);

      if (object->ref_count == 1 + ref_count * 2)
        {
          /* There may be additional references as argument defaults (we
           * multiply by 2, because there would be one copy of the spec
           * in the config type too).
           */
          g_hash_table_iter_remove (&iter);
        }
      else if (! G_IS_OBJECT (object) || object->ref_count < 1 + ref_count * 2)
        {
          /* this is debug code, a plug-in MUST NOT unref a proxy. To be nice,
           * we steal the object from the table, as removing it normally would
           * crash, since the object is not valid anymore.
           */
          g_printerr ("%s: ERROR: %s proxy was unrefed "
                      "by plug-in, it MUST NOT do that!\n",
                      G_STRFUNC, type);

          g_hash_table_iter_steal (&iter);
        }
      else if (destroy_all)
        {
          /* this is debug code, a plug-in MUST NOT ref a proxy */
          gint id;

          g_object_get (object, "id", &id, NULL);

          /* Some bindings are always keeping a reference to objects, in
           * particular this is the behavior of lua and gjs bindings where
           * object->ref_count == 2 at this point.
           * As for the Python binding, it is only true for global
           * variables (scoped variables are properly freed before the
           * plug-in ends), which anyway is not the best practice.
           * In any case, it's not something which the plug-in developer can do
           * anything about so this error is only confusing.
           * See #3912.
           */
          if (_clay_plug_in_manage_memory_manually (plug_in))
            /* This only MIGHT be a programming error.
             * Because a plugin keeps temporary procedure instances,
             * which keep formal args with defaults that can be proxy objects,
             * and persistent plugins don't destroy their temporary procedures,
             * such proxy objects can have any refcount, often two.
             */
            g_debug ("%s: %s proxy with ID %d has refcount %d.",
                     G_STRFUNC, G_OBJECT_TYPE_NAME (object), id, object->ref_count);

#if 0
          /* The code used to do this, which is only meaningful when the bug is
           * really in the plug-in code. If it's one of the cases where the
           * reference is actually held by a bindings, it's unwise and will
           * likely end up in double-free crashes.
           */
          while (object->ref_count > 1)
            g_object_unref (object);
#endif
          g_hash_table_iter_remove (&iter);
        }
    }
}

static void
clay_plug_in_init_i18n (ClayPlugIn *plug_in)
{
  ClayPlugInPrivate *priv;
  gchar             *rootdir      = g_path_get_dirname (clay_get_progname ());
  GFile             *root_file    = g_file_new_for_path (rootdir);
  GFile             *catalog_file = NULL;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  priv = clay_plug_in_get_instance_private (plug_in);

  /* Default domain name is the program directory name. */
  g_free (priv->translation_domain_name);
  priv->translation_domain_name = g_path_get_basename (rootdir);

  /* Default catalog path is the locale/ directory under the root
   * directory.
   */
  catalog_file = g_file_resolve_relative_path (root_file, "locale");
  g_set_object (&priv->translation_domain_path, catalog_file);

  g_free (rootdir);
  g_object_unref (root_file);
  g_object_unref (catalog_file);
}
