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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-edit.h"
#include "core/claybuffer.h"
#include "core/claycontainer.h"
#include "core/claydrawable-edit.h"
#include "core/clayfilloptions.h"
#include "core/clayimage.h"
#include "core/clayimage-new.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"
#include "core/claylayermask.h"
#include "core/claylinklayer.h"
#include "core/claylist.h"
#include "core/claypattern.h"
#include "core/clayprogress.h"

#include "file/file-open.h"

#include "path/claypath.h"
#include "path/claypath-import.h"
#include "path/clayvectorlayer.h"

#include "text/claytext.h"
#include "text/claytextlayer.h"

#include "tools/claytools-utils.h"

#include "widgets/claydnd.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-dnd.h"
#include "claydisplayshell-transform.h"

#include "clay-log.h"
#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_display_shell_drop_drawable  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 ClayViewable    *viewable,
                                                 gpointer         data);
static void   clay_display_shell_drop_path      (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 ClayViewable    *viewable,
                                                 gpointer         data);
static void   clay_display_shell_drop_svg       (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 const guchar    *svg_data,
                                                 gsize            svg_data_length,
                                                 gpointer         data);
static void   clay_display_shell_drop_pattern   (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 ClayViewable    *viewable,
                                                 gpointer         data);
static void   clay_display_shell_drop_color     (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GeglColor       *color,
                                                 gpointer         data);
static void   clay_display_shell_drop_buffer    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 ClayViewable    *viewable,
                                                 gpointer         data);
static void   clay_display_shell_drop_uri_list  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GList           *uri_list,
                                                 gpointer         data);
static void   clay_display_shell_drop_component (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 ClayImage       *image,
                                                 ClayChannelType  component,
                                                 gpointer         data);
static void   clay_display_shell_drop_pixbuf    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GdkPixbuf       *pixbuf,
                                                 gpointer         data);


/*  public functions  */

void
clay_display_shell_dnd_init (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_LAYER,
                               clay_display_shell_drop_drawable,
                               shell);
  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_LAYER_MASK,
                               clay_display_shell_drop_drawable,
                               shell);
  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_CHANNEL,
                               clay_display_shell_drop_drawable,
                               shell);
  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_PATH,
                               clay_display_shell_drop_path,
                               shell);
  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_PATTERN,
                               clay_display_shell_drop_pattern,
                               shell);
  clay_dnd_viewable_dest_add  (shell->canvas, CLAY_TYPE_BUFFER,
                               clay_display_shell_drop_buffer,
                               shell);
  clay_dnd_color_dest_add     (shell->canvas,
                               clay_display_shell_drop_color,
                               shell);
  clay_dnd_component_dest_add (shell->canvas,
                               clay_display_shell_drop_component,
                               shell);
  clay_dnd_uri_list_dest_add  (shell->canvas,
                               clay_display_shell_drop_uri_list,
                               shell);
  clay_dnd_svg_dest_add       (shell->canvas,
                               clay_display_shell_drop_svg,
                               shell);
  clay_dnd_pixbuf_dest_add    (shell->canvas,
                               clay_display_shell_drop_pixbuf,
                               shell);
}


/*  private functions  */

/*
 * Position the dropped item in the middle of the viewport.
 */
static void
clay_display_shell_dnd_position_item (ClayDisplayShell *shell,
                                      ClayImage        *image,
                                      ClayItem         *item)
{
  gint item_width  = clay_item_get_width  (item);
  gint item_height = clay_item_get_height (item);
  gint off_x, off_y;

  if (item_width  >= clay_image_get_width  (image) &&
      item_height >= clay_image_get_height (image))
    {
      off_x = (clay_image_get_width  (image) - item_width)  / 2;
      off_y = (clay_image_get_height (image) - item_height) / 2;
    }
  else
    {
      gint x, y;
      gint width, height;

      clay_display_shell_untransform_viewport (
        shell,
        ! clay_display_shell_get_infinite_canvas (shell),
        &x, &y, &width, &height);

      off_x = x + (width  - item_width)  / 2;
      off_y = y + (height - item_height) / 2;
    }

  clay_item_translate (item,
                       off_x - clay_item_get_offset_x (item),
                       off_y - clay_item_get_offset_y (item),
                       FALSE);
}

static void
clay_display_shell_dnd_flush (ClayDisplayShell *shell,
                              ClayImage        *image)
{
  clay_display_shell_present (shell);

  clay_image_flush (image);

  clay_context_set_display (clay_get_user_context (shell->display->clay),
                            shell->display);
}

static void
clay_display_shell_drop_drawable (GtkWidget    *widget,
                                  gint          x,
                                  gint          y,
                                  ClayViewable *viewable,
                                  gpointer      data)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image     = clay_display_get_image (shell->display);
  GType             new_type;
  ClayItem         *new_item;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! image)
    {
      image = clay_image_new_from_drawable (shell->display->clay,
                                            CLAY_DRAWABLE (viewable));
      clay_create_display (shell->display->clay, image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  if (CLAY_IS_LAYER (viewable))
    new_type = G_TYPE_FROM_INSTANCE (viewable);
  else
    new_type = CLAY_TYPE_LAYER;

  new_item = clay_item_convert (CLAY_ITEM (viewable), image, new_type);

  if (new_item)
    {
      ClayLayer *new_layer = CLAY_LAYER (new_item);

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      clay_display_shell_dnd_position_item (shell, image, new_item);

      clay_item_set_visible (new_item, TRUE, FALSE);

      clay_image_add_layer (image, new_layer,
                            CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

      clay_image_undo_group_end (image);

      clay_display_shell_dnd_flush (shell, image);
    }
}

static void
clay_display_shell_drop_path (GtkWidget    *widget,
                              gint          x,
                              gint          y,
                              ClayViewable *viewable,
                              gpointer      data)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image = clay_display_get_image (shell->display);
  ClayItem         *new_item;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! image)
    return;

  new_item = clay_item_convert (CLAY_ITEM (viewable),
                                image, G_TYPE_FROM_INSTANCE (viewable));

  if (new_item)
    {
      ClayPath *new_path = CLAY_PATH (new_item);

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Path"));

      clay_image_add_path (image, new_path,
                           CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

      clay_image_undo_group_end (image);

      clay_display_shell_dnd_flush (shell, image);
    }
}

static void
clay_display_shell_drop_svg (GtkWidget     *widget,
                             gint           x,
                             gint           y,
                             const guchar  *svg_data,
                             gsize          svg_data_len,
                             gpointer       data)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image = clay_display_get_image (shell->display);
  GError           *error  = NULL;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! image)
    return;

  if (! clay_path_import_buffer (image,
                                 (const gchar *) svg_data, svg_data_len,
                                 TRUE, FALSE,
                                 CLAY_IMAGE_ACTIVE_PARENT, -1,
                                 NULL, &error))
    {
      clay_message_literal (shell->display->clay, G_OBJECT (shell->display),
                            CLAY_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }
  else
    {
      clay_display_shell_dnd_flush (shell, image);
    }
}

static void
clay_display_shell_dnd_fill (ClayDisplayShell *shell,
                             ClayFillOptions  *options,
                             const gchar      *undo_desc)
{
  ClayImage     *image  = clay_display_get_image (shell->display);
  ClayGuiConfig *config = CLAY_GUI_CONFIG (shell->display->clay->config);
  GList         *drawables;
  GList         *iter;

  if (shell->display->clay->busy)
    return;

  if (! image)
    return;

  drawables = clay_image_get_selected_drawables (image);

  if (! drawables)
    return;

  for (iter = drawables; iter; iter = iter->next)
    {
      if (clay_viewable_get_children (iter->data))
        {
          clay_message_literal (shell->display->clay, G_OBJECT (shell->display),
                                CLAY_MESSAGE_ERROR,
                                _("Cannot modify the pixels of layer groups."));
          g_list_free (drawables);
          return;
        }

      if (clay_item_is_content_locked (iter->data, NULL))
        {
          clay_message_literal (shell->display->clay, G_OBJECT (shell->display),
                                CLAY_MESSAGE_ERROR,
                                _("A selected layer's pixels are locked."));
          g_list_free (drawables);
          return;
        }

      if (! clay_item_is_visible (CLAY_ITEM (iter->data)) &&
          ! config->edit_non_visible)
        {
          clay_message_literal (shell->display->clay, G_OBJECT (shell->display),
                                CLAY_MESSAGE_ERROR,
                                _("A selected layer is not visible."));
          g_list_free (drawables);
          return;
        }

      /* We can drop colors on text layers, and colors and patterns on
       * vector layers to change their fill. Otherwise, we want to prevent
       * destructive fills on non-rasterized layers */
      if (clay_item_is_rasterizable (CLAY_ITEM (iter->data)) &&
          ! clay_item_is_rasterized (CLAY_ITEM (iter->data)))
        {
          gchar *menu_path = _("Layer > Rasterize");
          gchar *message   = NULL;

          if (clay_item_is_link_layer (CLAY_ITEM (iter->data)))
            message = g_strdup_printf (_("Link layers must be rasterized (%s)."),
                                       menu_path);
          else if (clay_item_is_text_layer (CLAY_ITEM (iter->data)) &&
                   clay_fill_options_get_style (options) == CLAY_FILL_STYLE_PATTERN)
            message = g_strdup_printf (_("Text layers must be rasterized (%s)."),
                                       menu_path);

          if (message)
            {
              clay_message_literal (shell->display->clay,
                                    G_OBJECT (shell->display),
                                    CLAY_MESSAGE_ERROR,
                                    message);
              g_free (message);
              clay_tools_blink_item (shell->display->clay,
                                     CLAY_ITEM (iter->data));
              g_list_free (drawables);
              return;
            }
        }
    }

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT, undo_desc);

  for (iter = drawables; iter; iter = iter->next)
    {
      /* FIXME: there should be a virtual method for this that the
       *        ClayTextLayer can override.
       */
      if (clay_item_is_text_layer (iter->data) ||
          clay_item_is_vector_layer (iter->data))
        {
          ClayPattern *pattern = NULL;
          GeglColor   *color   = NULL;

          if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_FG_COLOR)
            color = clay_context_get_foreground (CLAY_CONTEXT (options));
          else if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_BG_COLOR)
            color = clay_context_get_background (CLAY_CONTEXT (options));
          else
            pattern = clay_context_get_pattern (CLAY_CONTEXT (options));

          if (color)
            {
              if (clay_item_is_vector_layer (iter->data))
                {
                  clay_vector_layer_set (CLAY_VECTOR_LAYER (iter->data), NULL,
                                         "enable-fill",
                                         TRUE,
                                         "fill-style",
                                         CLAY_CUSTOM_STYLE_SOLID_COLOR,
                                         "fill-color",
                                         color, NULL);
                  clay_vector_layer_refresh (iter->data);
                }
              else
                {
                  clay_text_layer_set (iter->data, NULL, "color", color, NULL);
                }
            }
          else if (pattern &&
                   clay_item_is_vector_layer (iter->data))
            {
              clay_vector_layer_set (CLAY_VECTOR_LAYER (iter->data), NULL,
                                     "enable-fill",  TRUE,
                                     "fill-style",   CLAY_CUSTOM_STYLE_PATTERN,
                                     "fill-pattern", pattern,
                                     NULL);
              clay_vector_layer_refresh (iter->data);
            }
        }
      else
        {
          clay_drawable_edit_fill (iter->data, options, undo_desc);
        }
    }

  g_list_free (drawables);
  clay_image_undo_group_end (image);
  clay_display_shell_dnd_flush (shell, image);
}

static void
clay_display_shell_drop_pattern (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 ClayViewable *viewable,
                                 gpointer      data)
{
  ClayDisplayShell *shell   = CLAY_DISPLAY_SHELL (data);
  ClayFillOptions  *options = clay_fill_options_new (shell->display->clay,
                                                     NULL, FALSE);

  CLAY_LOG (DND, NULL);

  clay_fill_options_set_style (options, CLAY_FILL_STYLE_PATTERN);
  clay_context_set_pattern (CLAY_CONTEXT (options), CLAY_PATTERN (viewable));

  clay_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
clay_display_shell_drop_color (GtkWidget     *widget,
                               gint           x,
                               gint           y,
                               GeglColor     *color,
                               gpointer       data)
{
  ClayDisplayShell *shell   = CLAY_DISPLAY_SHELL (data);
  ClayFillOptions  *options = clay_fill_options_new (shell->display->clay,
                                                     NULL, FALSE);

  CLAY_LOG (DND, NULL);

  clay_fill_options_set_style (options, CLAY_FILL_STYLE_FG_COLOR);
  clay_context_set_foreground (CLAY_CONTEXT (options), color);

  clay_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}

static void
clay_display_shell_drop_buffer (GtkWidget    *widget,
                                gint          drop_x,
                                gint          drop_y,
                                ClayViewable *viewable,
                                gpointer      data)
{
  ClayDisplayShell *shell    = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image    = clay_display_get_image (shell->display);
  ClayContext      *context;
  GList            *drawables;
  ClayBuffer       *buffer;
  ClayPasteType     paste_type;
  gint              x, y, width, height;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! image)
    {
      image = clay_image_new_from_buffer (shell->display->clay,
                                          CLAY_BUFFER (viewable));
      clay_create_display (image->clay, image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  paste_type = CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING;
  drawables  = clay_image_get_selected_drawables (image);
  context    = clay_get_user_context (shell->display->clay);
  buffer     = CLAY_BUFFER (viewable);

  clay_display_shell_untransform_viewport (
    shell,
    ! clay_display_shell_get_infinite_canvas (shell),
    &x, &y, &width, &height);

  /* FIXME: popup a menu for selecting "Paste Into" */

  g_list_free (clay_edit_paste (image, drawables, CLAY_OBJECT (buffer),
                                paste_type, context, FALSE,
                                x, y, width, height));

  g_list_free (drawables);
  clay_display_shell_dnd_flush (shell, image);
}

static void
clay_display_shell_drop_uri_list (GtkWidget *widget,
                                  gint       x,
                                  gint       y,
                                  GList     *uri_list,
                                  gpointer   data)
{
  ClayDisplayShell *shell   = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image;
  ClayContext      *context;
  GList            *list;
  gboolean          open_as_layers;

  /* If the app is already being torn down, shell->display might be
   * NULL here.  Play it safe.
   */
  if (! shell->display)
    return;

  image = clay_display_get_image (shell->display);
  context = clay_get_user_context (shell->display->clay);

  CLAY_LOG (DND, NULL);

  open_as_layers = (image != NULL);

  if (image)
    g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file  = g_file_new_for_uri (list->data);
      ClayPDBStatusType  status;
      GError            *error = NULL;
      gboolean           warn  = FALSE;

      if (! shell->display)
        {
          /* It seems as if Clay is being torn down for quitting. Bail out. */
          g_object_unref (file);
          g_clear_object (&image);
          return;
        }

      if (open_as_layers)
        {
          GList *new_layers;

          new_layers = file_open_layers (shell->display->clay, context,
                                         CLAY_PROGRESS (shell->display),
                                         image, FALSE, FALSE,
                                         file, CLAY_RUN_INTERACTIVE, NULL,
                                         &status, &error);

          if (new_layers)
            {
              gint x      = 0;
              gint y      = 0;
              gint width  = clay_image_get_width  (image);
              gint height = clay_image_get_height (image);

              if (clay_display_get_image (shell->display))
                {
                  clay_display_shell_untransform_viewport (
                    shell,
                    ! clay_display_shell_get_infinite_canvas (shell),
                    &x, &y, &width, &height);
                }

              clay_image_add_layers (image, new_layers,
                                     CLAY_IMAGE_ACTIVE_PARENT, -1,
                                     x, y, width, height,
                                     _("Drop layers"));

              g_list_free (new_layers);
            }
          else if (status != CLAY_PDB_CANCEL && status != CLAY_PDB_SUCCESS)
            {
              warn = TRUE;
            }
        }
      else if (clay_display_get_image (shell->display))
        {
          /*  open any subsequent images in a new display  */
          ClayImage *new_image;

          new_image = file_open_with_display (shell->display->clay, context,
                                              NULL,
                                              file, FALSE,
                                              G_OBJECT (clay_widget_get_monitor (widget)),
                                              &status, &error);

          if (! new_image && status != CLAY_PDB_CANCEL && status != CLAY_PDB_SUCCESS)
            warn = TRUE;
        }
      else
        {
          /*  open the first image in the empty display  */
          image = file_open_with_display (shell->display->clay, context,
                                          CLAY_PROGRESS (shell->display),
                                          file, FALSE,
                                          G_OBJECT (clay_widget_get_monitor (widget)),
                                          &status, &error);

          if (image)
            {
              g_object_ref (image);
            }
          else if (status != CLAY_PDB_CANCEL && status != CLAY_PDB_SUCCESS)
            {
              warn = TRUE;
            }
        }

      /* Something above might have run a few rounds of the main loop. Check
       * that shell->display is still there, otherwise ignore this as the app
       * is being torn down for quitting.
       */
      if (warn && shell->display)
        {
          clay_message (shell->display->clay, G_OBJECT (shell->display),
                        CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  if (image)
    clay_display_shell_dnd_flush (shell, image);

  g_clear_object (&image);
}

static void
clay_display_shell_drop_component (GtkWidget       *widget,
                                   gint             x,
                                   gint             y,
                                   ClayImage       *image,
                                   ClayChannelType  component,
                                   gpointer         data)
{
  ClayDisplayShell *shell      = CLAY_DISPLAY_SHELL (data);
  ClayImage        *dest_image = clay_display_get_image (shell->display);
  ClayChannel      *channel;
  ClayItem         *new_item;
  const gchar      *desc;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! dest_image)
    {
      dest_image = clay_image_new_from_component (image->clay,
                                                  image, component);
      clay_create_display (dest_image->clay, dest_image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (dest_image);

      return;
    }

  channel = clay_channel_new_from_component (image, component, NULL, NULL);

  new_item = clay_item_convert (CLAY_ITEM (channel),
                                dest_image, CLAY_TYPE_LAYER);
  g_object_unref (channel);

  if (new_item)
    {
      ClayLayer *new_layer = CLAY_LAYER (new_item);

      clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);
      clay_object_take_name (CLAY_OBJECT (new_layer),
                             g_strdup_printf (_("%s Channel Copy"), desc));

      clay_image_undo_group_start (dest_image, CLAY_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      clay_display_shell_dnd_position_item (shell, image, new_item);

      clay_image_add_layer (dest_image, new_layer,
                            CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

      clay_image_undo_group_end (dest_image);

      clay_display_shell_dnd_flush (shell, dest_image);
    }
}

static void
clay_display_shell_drop_pixbuf (GtkWidget *widget,
                                gint       x,
                                gint       y,
                                GdkPixbuf *pixbuf,
                                gpointer   data)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (data);
  ClayImage        *image = clay_display_get_image (shell->display);
  ClayLayer        *new_layer;
  gboolean          has_alpha = FALSE;

  CLAY_LOG (DND, NULL);

  if (shell->display->clay->busy)
    return;

  if (! image)
    {
      image = clay_image_new_from_pixbuf (shell->display->clay, pixbuf,
                                          _("Dropped Buffer"));
      clay_create_display (image->clay, image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  if (gdk_pixbuf_get_n_channels (pixbuf) == 2 ||
      gdk_pixbuf_get_n_channels (pixbuf) == 4)
    {
      has_alpha = TRUE;
    }

  new_layer =
    clay_layer_new_from_pixbuf (pixbuf, image,
                                clay_image_get_layer_format (image, has_alpha),
                                _("Dropped Buffer"),
                                CLAY_OPACITY_OPAQUE,
                                clay_image_get_default_new_layer_mode (image));

  if (new_layer)
    {
      ClayItem *new_item = CLAY_ITEM (new_layer);

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      clay_display_shell_dnd_position_item (shell, image, new_item);

      clay_image_add_layer (image, new_layer,
                            CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

      clay_image_undo_group_end (image);

      clay_display_shell_dnd_flush (shell, image);
    }
}
