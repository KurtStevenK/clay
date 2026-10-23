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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "claydnd.h"
#include "clayhelp-ids.h"
#include "claytoolbox.h"
#include "claytoolbox-image-area.h"
#include "clayview.h"
#include "claywindowstrategy.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void
image_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       ClayToolbox     *toolbox)
{
  ClayContext *context = clay_toolbox_get_context (toolbox);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (context->clay)),
                                             context->clay,
                                             clay_dock_get_dialog_factory (CLAY_DOCK (toolbox)),
                                             clay_widget_get_monitor (widget),
                                             "clay-image-list|clay-image-grid");
}

static void
image_preview_drop_image (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          ClayViewable *viewable,
                          gpointer      data)
{
  ClayContext *context = CLAY_CONTEXT (data);

  clay_context_set_image (context, CLAY_IMAGE (viewable));
}

static void
image_preview_set_viewable (ClayView     *view,
                            ClayViewable *old_viewable,
                            ClayViewable *new_viewable)
{
  if (! old_viewable && new_viewable)
    {
      clay_dnd_xds_source_add (GTK_WIDGET (view),
                               (ClayDndDragViewableFunc) clay_view_get_viewable,
                               NULL);
    }
  else if (old_viewable && ! new_viewable)
    {
      clay_dnd_xds_source_remove (GTK_WIDGET (view));
    }
}


/*  public functions  */

GtkWidget *
clay_toolbox_image_area_create (ClayToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  ClayContext *context;
  GtkWidget   *image_view;
  gchar       *tooltip;

  g_return_val_if_fail (CLAY_IS_TOOLBOX (toolbox), NULL);

  context = clay_toolbox_get_context (toolbox);

  image_view = clay_view_new_full_by_types (context,
                                            CLAY_TYPE_VIEW, CLAY_TYPE_IMAGE,
                                            width, height, 0,
                                            FALSE, TRUE, TRUE);

  g_signal_connect (image_view, "set-viewable",
                    G_CALLBACK (image_preview_set_viewable),
                    NULL);

  clay_view_set_viewable (CLAY_VIEW (image_view),
                          CLAY_VIEWABLE (clay_context_get_image (context)));

  gtk_widget_set_visible (image_view, TRUE);

#ifdef GDK_WINDOWING_X11
  tooltip = g_strdup_printf ("%s\n%s",
                             _("The active image.\n"
                               "Click to open the Image Dialog."),
                             _("Drag to an XDS enabled file-manager to "
                               "save the image."));
#else
  tooltip = g_strdup (_("The active image.\n"
                        "Click to open the Image Dialog."));
#endif

  clay_help_set_help_data (image_view, tooltip,
                           CLAY_HELP_TOOLBOX_IMAGE_AREA);
  g_free (tooltip);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (clay_view_set_viewable),
                           image_view, G_CONNECT_SWAPPED);

  g_signal_connect (image_view, "clicked",
                    G_CALLBACK (image_preview_clicked),
                    toolbox);

  clay_dnd_viewable_dest_add (image_view,
                              CLAY_TYPE_IMAGE,
                              image_preview_drop_image,
                              context);

  return image_view;
}
