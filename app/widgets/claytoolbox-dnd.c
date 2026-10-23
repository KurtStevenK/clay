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

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claybuffer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-new.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claytoolinfo.h"

#include "file/file-open.h"

#include "claydnd.h"
#include "claytoolbox.h"
#include "claytoolbox-dnd.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_toolbox_drop_uri_list  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GList           *uri_list,
                                           gpointer         data);
static void   clay_toolbox_drop_drawable  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           ClayViewable    *viewable,
                                           gpointer         data);
static void   clay_toolbox_drop_tool      (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           ClayViewable    *viewable,
                                           gpointer         data);
static void   clay_toolbox_drop_buffer    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           ClayViewable    *viewable,
                                           gpointer         data);
static void   clay_toolbox_drop_component (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           ClayImage       *image,
                                           ClayChannelType  component,
                                           gpointer         data);
static void   clay_toolbox_drop_pixbuf    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GdkPixbuf       *pixbuf,
                                           gpointer         data);


/*  public functions  */

void
clay_toolbox_dnd_init (ClayToolbox *toolbox,
                       GtkWidget   *vbox)
{
  ClayContext *context = NULL;

  g_return_if_fail (CLAY_IS_TOOLBOX (toolbox));
  g_return_if_fail (GTK_IS_BOX (vbox));

  context = clay_toolbox_get_context (toolbox);

  /* Before calling any dnd helper functions, setup the drag
   * destination manually since we want to handle all drag events
   * manually, otherwise we would not be able to give the drag handler
   * a chance to handle drag events
   */
  gtk_drag_dest_set (vbox,
                     0, NULL, 0,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  clay_dnd_viewable_dest_add  (vbox,
                               CLAY_TYPE_LAYER,
                               clay_toolbox_drop_drawable,
                               context);
  clay_dnd_viewable_dest_add  (vbox,
                               CLAY_TYPE_LAYER_MASK,
                               clay_toolbox_drop_drawable,
                               context);
  clay_dnd_viewable_dest_add  (vbox,
                               CLAY_TYPE_CHANNEL,
                               clay_toolbox_drop_drawable,
                               context);
  clay_dnd_viewable_dest_add  (vbox,
                               CLAY_TYPE_TOOL_INFO,
                               clay_toolbox_drop_tool,
                               context);
  clay_dnd_viewable_dest_add  (vbox,
                               CLAY_TYPE_BUFFER,
                               clay_toolbox_drop_buffer,
                               context);
  clay_dnd_component_dest_add (vbox,
                               clay_toolbox_drop_component,
                               context);
  clay_dnd_uri_list_dest_add  (vbox,
                               clay_toolbox_drop_uri_list,
                               context);
  clay_dnd_pixbuf_dest_add    (vbox,
                               clay_toolbox_drop_pixbuf,
                               context);
}


/*  private functions  */

static void
clay_toolbox_drop_uri_list (GtkWidget *widget,
                            gint       x,
                            gint       y,
                            GList     *uri_list,
                            gpointer   data)
{
  ClayContext *context = CLAY_CONTEXT (data);
  GList       *list;

  if (context->clay->busy)
    return;

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file = g_file_new_for_uri (list->data);
      ClayPDBStatusType  status;
      GError            *error = NULL;

      file_open_with_display (context->clay, context, NULL,
                              file, FALSE,
                              G_OBJECT (clay_widget_get_monitor (widget)),
                              &status, &error);

      if (status != CLAY_PDB_CANCEL && status != CLAY_PDB_SUCCESS)
        {
          /* file_open_image() took care of always having a filled error when
           * the status is neither CANCEL nor SUCCESS (and to transform a
           * wrongful success without a returned image into an EXECUTION_ERROR).
           *
           * In some case, we may also have a SUCCESS without an image, when the
           * file procedure is a `generic_file_proc` (e.g. for a .gex extension
           * file). So we should not rely on having a returned image or not.
           * Once again, sanitizing the returned status is handled by
           * file_open_image().
           */
          clay_message (context->clay, G_OBJECT (widget), CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

static void
clay_toolbox_drop_drawable (GtkWidget    *widget,
                            gint          x,
                            gint          y,
                            ClayViewable *viewable,
                            gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);
  ClayImage   *new_image;

  if (context->clay->busy)
    return;

  new_image = clay_image_new_from_drawable (context->clay,
                                            CLAY_DRAWABLE (viewable));
  clay_create_display (context->clay, new_image, clay_unit_pixel (), 1.0,
                       G_OBJECT (clay_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
clay_toolbox_drop_tool (GtkWidget    *widget,
                        gint          x,
                        gint          y,
                        ClayViewable *viewable,
                        gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);

  if (context->clay->busy)
    return;

  clay_context_set_tool (context, CLAY_TOOL_INFO (viewable));
}

static void
clay_toolbox_drop_buffer (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          ClayViewable *viewable,
                          gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);
  ClayImage   *image;

  if (context->clay->busy)
    return;

  image = clay_image_new_from_buffer (context->clay,
                                      CLAY_BUFFER (viewable));
  clay_create_display (image->clay, image, clay_unit_pixel (), 1.0,
                       G_OBJECT (clay_widget_get_monitor (widget)));
  g_object_unref (image);
}

static void
clay_toolbox_drop_component (GtkWidget       *widget,
                             gint             x,
                             gint             y,
                             ClayImage       *image,
                             ClayChannelType  component,
                             gpointer         data)
{
  ClayContext *context = CLAY_CONTEXT (data);
  ClayImage   *new_image;

  if (context->clay->busy)
    return;

  new_image = clay_image_new_from_component (context->clay,
                                             image, component);
  clay_create_display (new_image->clay, new_image, clay_unit_pixel (), 1.0,
                       G_OBJECT (clay_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
clay_toolbox_drop_pixbuf (GtkWidget *widget,
                          gint       x,
                          gint       y,
                          GdkPixbuf *pixbuf,
                          gpointer   data)
{
  ClayContext   *context = CLAY_CONTEXT (data);
  ClayImage     *new_image;

  if (context->clay->busy)
    return;

  new_image = clay_image_new_from_pixbuf (context->clay, pixbuf,
                                          _("Dropped Buffer"));
  clay_create_display (new_image->clay, new_image, clay_unit_pixel (), 1.0,
                       G_OBJECT (clay_widget_get_monitor (widget)));
  g_object_unref (new_image);
}
