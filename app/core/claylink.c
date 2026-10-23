/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayLink
 * Copyright (C) 2019 Jehan
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clayimage.h"
#include "claylink.h"
#include "claymarshal.h"
#include "claypickable.h"
#include "clayprojection.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "file/file-open.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_FILE,
  PROP_ABSOLUTE_PATH,
  N_PROPS
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

struct _ClayLinkPrivate
{
  Clay                *clay;
  GFile               *file;
  GFileMonitor        *monitor;
  gboolean             absolute_path;

  GeglBuffer          *buffer;
  gboolean             broken;
  GError              *error;
  guint                idle_changed_source;

  gboolean             is_vector;
  gint                 width;
  gint                 height;
  gboolean             keep_ratio;
  ClayImageBaseType    base_type;
  ClayPrecision        precision;
  ClayPlugInProcedure *load_proc;
  const gchar         *mime_type;
};

static void       clay_link_savable_iface_init (ClaySavableInterface  *iface);

static void       clay_link_finalize           (GObject               *object);
static void       clay_link_get_property       (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);
static void       clay_link_set_property       (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);

static void       clay_link_save               (ClaySavable           *savable,
                                                ClaySaveState         *state);
static gboolean   clay_link_load_enter         (ClayLoadState         *state,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static gboolean   clay_link_load_exit          (ClayLoadState         *state,
                                                const gchar           *text,
                                                gsize                  len,
                                                gpointer               user_data,
                                                GError               **error);

static void       clay_link_file_changed       (GFileMonitor          *monitor,
                                                GFile                 *file,
                                                GFile                 *other_file,
                                                GFileMonitorEvent      event_type,
                                                ClayLink              *link);
static gboolean   clay_link_emit_changed       (gpointer               data);

static void       clay_link_update_buffer      (ClayLink              *link,
                                                ClayProgress          *progress,
                                                GError               **error);
static void       clay_link_start_monitoring   (ClayLink              *link);
static gchar    * clay_link_get_relative_path  (ClayLink              *link,
                                                GFile                 *parent,
                                                gint                   n_back);



G_DEFINE_TYPE_WITH_CODE (ClayLink, clay_link, CLAY_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayLink)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_link_savable_iface_init))

#define parent_class clay_link_parent_class

static guint       link_signals[LAST_SIGNAL] = { 0 };

static GParamSpec *link_props[N_PROPS]       = { NULL, };

static void
clay_link_class_init (ClayLinkClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);

  link_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLinkClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize     = clay_link_finalize;
  object_class->get_property = clay_link_get_property;
  object_class->set_property = clay_link_set_property;

  link_props[PROP_CLAY]          = g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY);
  link_props[PROP_FILE]          = g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_EXPLICIT_NOTIFY);
  link_props[PROP_ABSOLUTE_PATH] = g_param_spec_boolean ("absolute-path", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, link_props);
}

static void
clay_link_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "link";
  iface->save       = clay_link_save;
  iface->load_enter = clay_link_load_enter;
  iface->load_exit  = clay_link_load_exit;
}

static void
clay_link_init (ClayLink *link)
{
  link->p            = clay_link_get_instance_private (link);
  link->p->clay      = NULL;
  link->p->file      = NULL;
  link->p->monitor   = NULL;
  link->p->buffer    = NULL;
  link->p->broken    = TRUE;
  link->p->error     = NULL;
  link->p->width     = 0;
  link->p->height    = 0;
  link->p->base_type = CLAY_RGB;
  link->p->precision = CLAY_PRECISION_U8_PERCEPTUAL;
  link->p->load_proc = NULL;

  link->p->idle_changed_source = 0;
}

static void
clay_link_finalize (GObject *object)
{
  ClayLink *link = CLAY_LINK (object);

  g_clear_object (&link->p->file);
  g_clear_object (&link->p->monitor);
  g_clear_object (&link->p->buffer);
  g_clear_error  (&link->p->error);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_link_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  ClayLink *link = CLAY_LINK (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, link->p->clay);
      break;
    case PROP_FILE:
      g_value_set_object (value, link->p->file);
      break;
    case PROP_ABSOLUTE_PATH:
      g_value_set_boolean (value, link->p->absolute_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_link_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayLink *link = CLAY_LINK (object);

  switch (property_id)
    {
    case PROP_CLAY:
      link->p->clay = g_value_get_object (value);
      break;
    case PROP_FILE:
      clay_link_set_file (link, g_value_get_object (value), 0, 0, FALSE, NULL, NULL);
      break;
    case PROP_ABSOLUTE_PATH:
      link->p->absolute_path = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_link_save (ClaySavable   *savable,
                ClaySaveState *state)
{
  ClayLink *link = CLAY_LINK (savable);
  gchar    *path = NULL;

  if (link->p->is_vector)
    clay_savable_print_element_start (state, "link",
                                      "width",  "%u", link->p->width,
                                      "height", "%u", link->p->height,
                                      NULL);
  else
    clay_savable_print_element_start (state, "link", NULL);

  clay_link_get_file (link, state->xcf_file, &path);
  clay_savable_print_element (state, "file-path", "%s", path,
                              "absolute", "%b", link->p->absolute_path,
                              NULL);
  clay_savable_print_element_end (state, "link");

  g_free (path);
}

static gboolean
clay_link_load_enter (ClayLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError        **error)
{
  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "width") == 0 ||
          g_strcmp0 (*attribute_names, "height") == 0)
        clay_savable_load_store_from_string (state, error,
                                             *attribute_names, G_TYPE_UINT, *attribute_values,
                                             NULL);
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  clay_savable_load_add_simple_handler (state, "file-path", G_TYPE_STRING,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "absolute", G_TYPE_BOOLEAN,
                                        NULL);

  return TRUE;
}

static gboolean
clay_link_load_exit (ClayLoadState  *state,
                     const gchar    *text,
                     gsize           len,
                     gpointer        user_data,
                     GError        **error)
{
  const gchar *path = NULL;

  if (clay_savable_load_get_values (state,
                                    "file-path", &path,
                                    NULL))
    {
      ClayLink *link;
      GFile    *file       = NULL;
      guint     width      = 0;
      guint     height     = 0;
      gboolean  absolute   = FALSE;
      gboolean  keep_ratio = FALSE;

      /* TODO: this initial version does not handle circular links yet. */

      clay_savable_load_get_values (state,
                                    "width",         &width,
                                    "height",        &height,
                                    "path:absolute", &absolute,
                                    NULL);
      if (absolute && ! g_path_is_absolute (path))
        {
          /* When a path is marked as absolute, it must be actually
           * absolute. It's only relative paths which are allowed to be
           * absolute because it's not always possible to create a
           * relative path (e.g. on Windows with different partitions).
           */
          CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                   "%s: link path is not absolute, yet should be: %s",
                                   G_STRFUNC, path);
          return TRUE;
        }
      else if (! absolute)
        {
          GFile *xcf_file;

          xcf_file = clay_image_get_file (state->image);
          if (xcf_file)
            {
              GFile *folder = g_file_get_parent (xcf_file);
              file = g_file_resolve_relative_path (folder, path);
              g_object_unref (folder);
            }
          else
            {
              CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                       "%s: link relative path cannot be computed without source file: %s",
                                       G_STRFUNC, path);
              return TRUE;
            }
        }

      if (! file)
        file = g_file_new_for_path (path);

      link = clay_link_new (state->clay, file, width, height, keep_ratio, NULL, error);
      clay_savable_load_store_value (state, "link", link, g_object_unref);
      clay_savable_load_bubble_up (state, "link");
    }

  return TRUE;
}

static void
clay_link_file_changed (GFileMonitor      *monitor,
                        GFile             *file,
                        GFile             *other_file,
                        GFileMonitorEvent  event_type,
                        ClayLink          *link)
{
  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
      if (link->p->idle_changed_source == 0)
        link->p->idle_changed_source = g_idle_add_full (G_PRIORITY_LOW,
                                                        clay_link_emit_changed,
                                                        link, NULL);
      break;
    case G_FILE_MONITOR_EVENT_CREATED:
      g_signal_emit (link, link_signals[CHANGED], 0);
      break;
    case G_FILE_MONITOR_EVENT_DELETED:
      link->p->broken = TRUE;
      g_clear_error (&link->p->error);
      g_set_error_literal (&link->p->error,
                           G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("The file got deleted"));
      break;

    default:
      /* No need to signal for changes where nothing can be done anyway.
       * In particular a file deletion, the link is broken, yet we don't
       * want to re-render.
       * Don't emit either on G_FILE_MONITOR_EVENT_CHANGED because too
       * many such events may be emitted for a single file writing.
       */
      break;
    }

}

static gboolean
clay_link_emit_changed (gpointer data)
{
  ClayLink *link = CLAY_LINK (data);

  clay_link_update_buffer (link, NULL, NULL);

  g_signal_emit (link, link_signals[CHANGED], 0);
  link->p->idle_changed_source = 0;

  return G_SOURCE_REMOVE;
}

static void
clay_link_update_buffer (ClayLink      *link,
                         ClayProgress  *progress,
                         GError       **error)
{
  GeglBuffer *buffer     = NULL;
  GError     *real_error = NULL;

  g_return_if_fail (CLAY_IS_LINK (link));
  g_return_if_fail (error == NULL || *error == NULL);

  link->p->is_vector = FALSE;
  g_clear_error (&link->p->error);

  if (link->p->file)
    {
      ClayImage         *image;
      ClayPDBStatusType  status;

      link->p->mime_type = NULL;
      image = file_open_image (link->p->clay,
                               clay_get_user_context (link->p->clay),
                               progress,
                               link->p->file,
                               link->p->width, link->p->height,
                               link->p->keep_ratio,
                               FALSE, NULL,
                               /* XXX We might want interactive opening
                                * for a first opening (when done through
                                * GUI), but not for every re-render.
                                */
                               CLAY_RUN_NONINTERACTIVE,
                               &link->p->is_vector,
                               &status, &link->p->mime_type,
                               &real_error);

      if (image && status == CLAY_PDB_SUCCESS)
        {
          /* If we don't flush the projection first, the buffer may be empty.
           * I do wonder if the flushing and updating of the link could
           * not be multi-threaded with clay_projection_flush() instead,
           * then notifying the update through signals. For very heavy
           * images, would it be a better UX? XXX
           */
          clay_projection_flush_now (clay_image_get_projection (image), TRUE);
          buffer = clay_pickable_get_buffer (CLAY_PICKABLE (image));
          g_object_ref (buffer);

          link->p->base_type = clay_image_get_base_type (image);
          link->p->precision = clay_image_get_precision (image);
          link->p->width     = clay_image_get_width (image);
          link->p->height    = clay_image_get_height (image);
          link->p->load_proc = clay_image_get_load_proc (image);
        }

      /* Only keep the buffer, free the rest. */
      g_clear_object (&image);
    }

  link->p->broken = (buffer == NULL);
  if (link->p->broken)
    {
      if (real_error)
        link->p->error = g_error_copy (real_error);
      else
        g_set_error_literal (&link->p->error,
                             G_FILE_ERROR, G_FILE_ERROR_FAILED,
                             _("No file was set"));
    }

  if (error)
    *error = real_error;
  else
    g_clear_error (&real_error);

  if (buffer)
    {
      /* Keep the old buffer if the link is broken (outdated image is
       * better than none).
       */
      g_clear_object (&link->p->buffer);
      link->p->buffer = buffer;
    }
}

static void
clay_link_start_monitoring (ClayLink *link)
{
  /* GFileMonitors created with the G_FILE_MONITOR_WATCH_HARD_LINKS flag
   * are not monitored on Windows. This was resolved in MR glib!4901.
   * TODO: remove the top #ifdef when we bump the GLib requirement.
   */
  link->p->monitor = g_file_monitor (link->p->file,
#if defined(G_OS_WIN32) && ! GLIB_CHECK_VERSION(2, 86, 3)
                                     G_FILE_MONITOR_NONE,
#else
                                     G_FILE_MONITOR_WATCH_HARD_LINKS,
#endif
                                     NULL, NULL);
  g_signal_connect (link->p->monitor, "changed",
                    G_CALLBACK (clay_link_file_changed),
                    link);
}

/**
 * clay_link_get_relative_path:
 * @link: the image this link is associated with.
 * @parent: (transfer full): a #GFile object.
 * @n_back: set it to 0 when calling it initially.
 *
 * This is a variant of g_file_get_relative_path() which will work even
 * when the @link file is not a child of @parent. In this case, the
 * relative link will use "../" as many times as necessary until a
 * common parent folder is found.
 *
 * In case no parent is found (it may happen for instance on Windows,
 * with various file system roots), an absolute path is returned
 * instead, ensuring that we never return %NULL.
 *
 * Note that this function takes ownership of @parent and will take care
 * of freeing it. This allows for tail recursion.
 *
 * Returns: a path from @link relatively to @parent, falling back
 *          to an absolute path if a relative path cannot be constructed.
 **/
static gchar *
clay_link_get_relative_path (ClayLink *link,
                             GFile    *parent,
                             gint      n_back)
{
  gchar *relative_path;

  g_return_val_if_fail (CLAY_IS_LINK (link), NULL);
  g_return_val_if_fail (parent != NULL && n_back >= 0, NULL);

  relative_path = g_file_get_relative_path (parent, link->p->file);

  if (relative_path == NULL)
    {
      GFile *grand_parent = g_file_get_parent (parent);

      g_object_unref (parent);

      if (grand_parent == NULL)
        /* This may happen e.g. on Windows where there are several roots
         * so it is not always possible to make a relative path.
         */
        return g_file_get_path (link->p->file);
      else
        return clay_link_get_relative_path (link, grand_parent, n_back + 1);
    }
  else
    {
      g_object_unref (parent);

      if (n_back > 0)
        {
          GStrvBuilder  *builder;
          gchar        **array;
          gchar         *dots;
          gchar         *relpath;

          builder = g_strv_builder_new ();
          for (gint i = 0; i < n_back; i++)
            g_strv_builder_add (builder, "..");

          array   = g_strv_builder_end (builder);
          dots    = g_strjoinv (G_DIR_SEPARATOR_S, array);
          relpath = g_build_filename (dots, relative_path, NULL);

          g_free (relative_path);
          g_free (dots);
          g_strfreev (array);
          g_strv_builder_unref (builder);

          relative_path = relpath;
        }

      return relative_path;
    }
}


/*  public functions  */

/**
 * clay_link_new:
 * @clay: #Clay object.
 * @file: a #GFile object.
 *
 * Creates a new link object. By default, all link objects are created
 * as being relative to the path of the image they will be associated
 * with.
 *
 * Return value: a new #ClayLink or %NULL in case of a problem
 **/
ClayLink *
clay_link_new (Clay          *clay,
               GFile         *file,
               gint           vector_width,
               gint           vector_height,
               gboolean       keep_ratio,
               ClayProgress  *progress,
               GError       **error)
{
  ClayLink *link;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  link = g_object_new (CLAY_TYPE_LINK,
                       "clay",          clay,
                       "absolute-path", FALSE,
                       NULL);

  clay_link_set_file (link, file, vector_width, vector_height, keep_ratio, progress, error);

  return CLAY_LINK (link);
}

ClayLink *
clay_link_duplicate (ClayLink *link)
{
  ClayLink *new_link;

  g_return_val_if_fail (CLAY_IS_LINK (link), NULL);

  new_link = g_object_new (CLAY_TYPE_LINK,
                           "clay",          link->p->clay,
                           "absolute-path", clay_link_get_absolute_path (link),
                           NULL);

  /* Copy things manually as we do not need to trigger a load. */
  new_link->p->file      = link->p->file ? g_object_ref (link->p->file) : NULL;

  new_link->p->buffer    = link->p->buffer ? gegl_buffer_dup (link->p->buffer) : NULL;
  new_link->p->broken    = link->p->broken;
  new_link->p->error     = link->p->error ? g_error_copy (link->p->error) : NULL;

  new_link->p->is_vector = link->p->is_vector;
  new_link->p->width     = link->p->width;
  new_link->p->height    = link->p->height;
  new_link->p->base_type = link->p->base_type;
  new_link->p->precision = link->p->precision;
  new_link->p->load_proc = link->p->load_proc;

  if (new_link->p->file)
    {
      gchar *basename;

      basename = g_file_get_basename (new_link->p->file);
      clay_object_set_name_safe (CLAY_OBJECT (new_link), basename);
      g_free (basename);

      if (clay_link_is_monitored (link))
        clay_link_start_monitoring (new_link);
    }

  return new_link;
}

/*
 * clay_link_get_file:
 * @link: the #ClayLink object.
 * @xcf_file: optional XCF file from which @path will be relative to.
 * @path: optional returned path of the returned file.
 *
 * If @path is non-%NULL, it will be set to the file system path for the
 * returned %GFile, either as an absolute or relative path, depending on
 * how @link was set.
 * Note that it is possible for @path to be absolute even when it is set
 * to be a relative path, in cases where no relative path can be
 * constructed from @xcf_file.
 *
 * Returns: the %GFile which %link is syncing too.
 */
GFile *
clay_link_get_file (ClayLink  *link,
                    GFile     *xcf_file,
                    gchar    **path)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), NULL);
  g_return_val_if_fail ((path == NULL && xcf_file == NULL) ||
                        (*path == NULL && xcf_file != NULL &&
                         g_file_has_parent (xcf_file, NULL)), NULL);

  if (path != NULL)
    {
      if (link->p->absolute_path || xcf_file == NULL)
        *path = g_file_get_path (link->p->file);
      else
        *path = clay_link_get_relative_path (link,
                                             g_file_get_parent (xcf_file),
                                             0);
    }

  return link->p->file;
}

void
clay_link_set_file (ClayLink      *link,
                    GFile         *file,
                    gint           vector_width,
                    gint           vector_height,
                    gboolean       keep_ratio,
                    ClayProgress  *progress,
                    GError       **error)
{
  g_return_if_fail (CLAY_IS_LINK (link));
  g_return_if_fail (G_IS_FILE (file) || file == NULL);
  g_return_if_fail (error == NULL || *error == NULL);

  if (file == link->p->file ||
      (file && link->p->file && g_file_equal (file, link->p->file)))
    {
      if (link->p->width != vector_width   ||
          link->p->height != vector_height ||
          link->p->keep_ratio != keep_ratio)
        clay_link_set_size (link, vector_width, vector_height, keep_ratio);

      return;
    }

  link->p->width      = vector_width;
  link->p->height     = vector_height;
  link->p->keep_ratio = keep_ratio;

  g_clear_object (&link->p->monitor);

  g_set_object (&link->p->file, file);

  clay_link_update_buffer (link, progress, error);

  if (link->p->file)
    {
      gchar *basename;

      basename = g_file_get_basename (link->p->file);
      clay_object_set_name_safe (CLAY_OBJECT (link), basename);
      g_free (basename);

      clay_link_start_monitoring (link);
    }

  g_object_notify_by_pspec (G_OBJECT (link), link_props[PROP_FILE]);
}

gboolean
clay_link_get_absolute_path (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), FALSE);

  return link->p->absolute_path;
}

void
clay_link_set_absolute_path (ClayLink *link,
                             gboolean  absolute_path)
{
  g_return_if_fail (CLAY_IS_LINK (link));

  link->p->absolute_path = absolute_path;
}

const gchar *
clay_link_get_mime_type (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), NULL);

  return link->p->mime_type;
}

void
clay_link_freeze (ClayLink *link)
{
  g_return_if_fail (CLAY_IS_LINK (link));
  g_return_if_fail (link->p->monitor != NULL);

  g_clear_object (&link->p->monitor);
}

void
clay_link_thaw (ClayLink *link)
{
  g_return_if_fail (CLAY_IS_LINK (link));
  g_return_if_fail (G_IS_FILE (link->p->file) && link->p->monitor == NULL);

  clay_link_update_buffer (link, NULL, NULL);
  clay_link_start_monitoring (link);
}

gboolean
clay_link_is_monitored (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), FALSE);

  return (link->p->monitor != NULL);
}

gboolean
clay_link_is_broken (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), TRUE);

  return link->p->broken;
}

void
clay_link_set_size (ClayLink *link,
                    gint      width,
                    gint      height,
                    gboolean  keep_ratio)
{
  g_return_if_fail (CLAY_IS_LINK (link));

  link->p->width      = width;
  link->p->height     = height;
  link->p->keep_ratio = keep_ratio;

  if (link->p->monitor && link->p->is_vector)
    clay_link_update_buffer (link, NULL, NULL);
}

void
clay_link_get_size (ClayLink *link,
                    gint     *width,
                    gint     *height)
{
  g_return_if_fail (CLAY_IS_LINK (link));

  *width  = link->p->width;
  *height = link->p->height;
}

ClayImageBaseType
clay_link_get_base_type (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link) && ! link->p->broken, CLAY_RGB);

  return link->p->base_type;
}

ClayPrecision
clay_link_get_precision (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link) && ! link->p->broken, CLAY_PRECISION_U8_PERCEPTUAL);

  return link->p->precision;
}

ClayPlugInProcedure *
clay_link_get_load_proc (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link) && ! link->p->broken, NULL);

  return link->p->load_proc;
}

gboolean
clay_link_is_vector (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), FALSE);

  return link->p->is_vector;
}

GeglBuffer *
clay_link_get_buffer (ClayLink *link)
{
  g_return_val_if_fail (CLAY_IS_LINK (link), NULL);

  return link->p->buffer;
}
