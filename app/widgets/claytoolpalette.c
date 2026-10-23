/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolpalette.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claycontainer.h"
#include "core/claytoolitem.h"

#include "claytoolbox.h"
#include "claytoolbutton.h"
#include "claytoolpalette.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE


typedef struct _ClayToolPalettePrivate ClayToolPalettePrivate;

struct _ClayToolPalettePrivate
{
  ClayToolbox *toolbox;

  GtkWidget   *group;
  GHashTable  *buttons;
};

#define GET_PRIVATE(p) ((ClayToolPalettePrivate *) clay_tool_palette_get_instance_private ((ClayToolPalette *) (p)))


static void     clay_tool_palette_finalize            (GObject         *object);

static GtkSizeRequestMode
                clay_tool_palette_get_request_mode    (GtkWidget       *widget);
static void     clay_tool_palette_get_preferred_width (GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     clay_tool_palette_get_preferred_height(GtkWidget       *widget,
                                                       gint            *min_width,
                                                       gint            *pref_width);
static void     clay_tool_palette_height_for_width    (GtkWidget       *widget,
                                                       gint             width,
                                                       gint            *min_height,
                                                       gint            *pref_height);
static void     clay_tool_palette_style_updated       (GtkWidget       *widget);

static void     clay_tool_palette_tool_add            (ClayContainer   *container,
                                                       ClayToolItem    *tool_item,
                                                       ClayToolPalette *palette);
static void     clay_tool_palette_tool_remove         (ClayContainer   *container,
                                                       ClayToolItem    *tool_item,
                                                       ClayToolPalette *palette);
static void     clay_tool_palette_tool_reorder        (ClayContainer   *container,
                                                       ClayToolItem    *tool_item,
                                                       gint             old_index,
                                                       gint             new_index,
                                                       ClayToolPalette *palette);

static void     clay_tool_palette_add_button          (ClayToolPalette *palette,
                                                       ClayToolItem    *tool_item,
                                                       gint             index);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolPalette, clay_tool_palette,
                            GTK_TYPE_TOOL_PALETTE)

#define parent_class clay_tool_palette_parent_class


static void
clay_tool_palette_class_init (ClayToolPaletteClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize                       = clay_tool_palette_finalize;

  widget_class->get_request_mode               = clay_tool_palette_get_request_mode;
  widget_class->get_preferred_width            = clay_tool_palette_get_preferred_width;
  widget_class->get_preferred_height           = clay_tool_palette_get_preferred_height;
  widget_class->get_preferred_height_for_width = clay_tool_palette_height_for_width;
  widget_class->style_updated                  = clay_tool_palette_style_updated;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TOOL_ICON_SIZE,
                                                              CLAY_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-relief",
                                                              NULL, NULL,
                                                              GTK_TYPE_RELIEF_STYLE,
                                                              DEFAULT_BUTTON_RELIEF,
                                                              CLAY_PARAM_READABLE));
}

static void
clay_tool_palette_init (ClayToolPalette *palette)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (palette);

  private->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_tool_palette_set_style (GTK_TOOL_PALETTE (palette), GTK_TOOLBAR_ICONS);
}

static void
clay_tool_palette_finalize (GObject *object)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->buttons, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GtkSizeRequestMode
clay_tool_palette_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static gint
clay_tool_palette_get_n_tools (ClayToolPalette *palette,
                               gint            *button_width,
                               gint            *button_height,
                               gint            *min_columns,
                               gint            *min_rows)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (palette);
  ClayToolItem           *tool_item;
  GHashTableIter          iter;
  GdkMonitor             *monitor;
  GdkRectangle            workarea;
  gint                    n_tools;
  gint                    max_rows;
  gint                    max_columns;

  if (! clay_tool_palette_get_button_size (palette,
                                           button_width, button_height))
    {
      /* arbitrary values, just to simplify our callers */
      *button_width  = 24;
      *button_height = 24;
    }

  n_tools = 0;

  g_hash_table_iter_init (&iter, private->buttons);

  while (g_hash_table_iter_next (&iter, (gpointer *) &tool_item, NULL))
    {
      if (clay_tool_item_get_visible (tool_item))
        n_tools++;
    }

  monitor = clay_widget_get_monitor (GTK_WIDGET (palette));
  gdk_monitor_get_workarea (monitor, &workarea);

  max_columns = MAX (1, (workarea.width  * 0.9) / *button_width);
  max_rows    = MAX (1, (workarea.height * 0.7) / *button_height);

  *min_columns = MAX (1, n_tools / max_rows);
  *min_rows    = MAX (1, n_tools / max_columns);

  return n_tools;
}

static void
clay_tool_palette_get_preferred_width (GtkWidget *widget,
                                       gint      *min_width,
                                       gint      *pref_width)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  clay_tool_palette_get_n_tools (CLAY_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_width  = min_columns * button_width;
  *pref_width = min_columns * button_width;
}

static void
clay_tool_palette_get_preferred_height (GtkWidget *widget,
                                        gint      *min_height,
                                        gint      *pref_height)
{
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;

  clay_tool_palette_get_n_tools (CLAY_TOOL_PALETTE (widget),
                                 &button_width, &button_height,
                                 &min_columns, &min_rows);

  *min_height  = min_rows * button_height;
  *pref_height = min_rows * button_height;
}

static void
clay_tool_palette_height_for_width (GtkWidget *widget,
                                    gint       width,
                                    gint      *min_height,
                                    gint      *pref_height)
{
  gint n_tools;
  gint button_width;
  gint button_height;
  gint min_columns;
  gint min_rows;
  gint tool_columns;
  gint tool_rows;

  n_tools = clay_tool_palette_get_n_tools (CLAY_TOOL_PALETTE (widget),
                                           &button_width, &button_height,
                                           &min_columns, &min_rows);

  tool_columns = MAX (min_columns, width / button_width);
  tool_rows    = n_tools / tool_columns;

  if (n_tools % tool_columns)
    tool_rows++;

  /* see: #7454 */
  *min_height = tool_rows * button_height / 2;
  *pref_height = tool_rows * button_height;
}

static void
clay_tool_palette_style_updated (GtkWidget *widget)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (widget);
  GtkWidget              *tool_button;
  GHashTableIter          iter;
  GtkReliefStyle          relief;
  GtkIconSize             tool_icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! clay_toolbox_get_context (private->toolbox))
    return;

  gtk_widget_style_get (widget,
                        "button-relief",  &relief,
                        "tool-icon-size", &tool_icon_size,
                        NULL);

  gtk_tool_palette_set_icon_size (GTK_TOOL_PALETTE (widget),
                                  tool_icon_size);

  g_hash_table_iter_init (&iter, private->buttons);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
    {
      GtkWidget *button = gtk_bin_get_child (GTK_BIN (tool_button));

      gtk_button_set_relief (GTK_BUTTON (button), relief);
    }

  clay_dock_invalidate_geometry (CLAY_DOCK (private->toolbox));
}


/*  public functions  */

GtkWidget *
clay_tool_palette_new (void)
{
  return g_object_new (CLAY_TYPE_TOOL_PALETTE, NULL);
}

void
clay_tool_palette_set_toolbox (ClayToolPalette *palette,
                               ClayToolbox     *toolbox)
{
  ClayToolPalettePrivate *private;
  ClayContext            *context;
  GList                  *list;

  g_return_if_fail (CLAY_IS_TOOL_PALETTE (palette));
  g_return_if_fail (CLAY_IS_TOOLBOX (toolbox));

  private = GET_PRIVATE (palette);

  private->toolbox = toolbox;

  context = clay_toolbox_get_context (toolbox);

  private->group = gtk_tool_item_group_new (_("Tools"));
  gtk_tool_item_group_set_label_widget (GTK_TOOL_ITEM_GROUP (private->group),
                                        NULL);
  gtk_container_add (GTK_CONTAINER (palette), private->group);
  gtk_widget_set_visible (private->group, TRUE);

  for (list = clay_get_tool_item_ui_iter (context->clay);
       list;
       list = g_list_next (list))
    {
      ClayToolItem *tool_item = list->data;

      clay_tool_palette_add_button (palette, tool_item, -1);
    }

  g_signal_connect_object (context->clay->tool_item_ui_list, "add",
                           G_CALLBACK (clay_tool_palette_tool_add),
                           palette, 0);
  g_signal_connect_object (context->clay->tool_item_ui_list, "remove",
                           G_CALLBACK (clay_tool_palette_tool_remove),
                           palette, 0);
  g_signal_connect_object (context->clay->tool_item_ui_list, "reorder",
                           G_CALLBACK (clay_tool_palette_tool_reorder),
                           palette, 0);

  g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                           "notify::theme",
                           G_CALLBACK (clay_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                           "notify::override-theme-icon-size",
                           G_CALLBACK (clay_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                           "notify::custom-icon-size",
                           G_CALLBACK (clay_tool_palette_style_updated),
                           palette, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
}

gboolean
clay_tool_palette_get_button_size (ClayToolPalette *palette,
                                   gint            *width,
                                   gint            *height)
{
  ClayToolPalettePrivate *private;
  GHashTableIter          iter;
  GtkWidget              *tool_button;

  g_return_val_if_fail (CLAY_IS_TOOL_PALETTE (palette), FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  private = GET_PRIVATE (palette);

  g_hash_table_iter_init (&iter, private->buttons);

  if (g_hash_table_iter_next (&iter, NULL, (gpointer *) &tool_button))
    {
      GtkRequisition button_requisition;

      gtk_widget_get_preferred_size (tool_button, &button_requisition, NULL);

      *width  = button_requisition.width;
      *height = button_requisition.height;

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
clay_tool_palette_tool_add (ClayContainer   *container,
                            ClayToolItem    *tool_item,
                            ClayToolPalette *palette)
{
  clay_tool_palette_add_button (
    palette,
    tool_item,
    clay_container_get_child_index (container, CLAY_OBJECT (tool_item)));
}

static void
clay_tool_palette_tool_remove (ClayContainer   *container,
                               ClayToolItem    *tool_item,
                               ClayToolPalette *palette)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkWidget              *tool_button;

  tool_button = g_hash_table_lookup (private->buttons, tool_item);

  if (tool_button)
    {
      g_hash_table_remove (private->buttons, tool_item);

      gtk_container_remove (GTK_CONTAINER (private->group), tool_button);
    }
}

static void
clay_tool_palette_tool_reorder (ClayContainer   *container,
                                ClayToolItem    *tool_item,
                                gint             old_index,
                                gint             new_index,
                                ClayToolPalette *palette)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkWidget              *tool_button;

  tool_button = g_hash_table_lookup (private->buttons, tool_item);

  if (tool_button)
    {
      gtk_tool_item_group_set_item_position
        (GTK_TOOL_ITEM_GROUP (private->group),
         GTK_TOOL_ITEM (tool_button), new_index);
    }
}

static void
clay_tool_palette_add_button (ClayToolPalette *palette,
                              ClayToolItem    *tool_item,
                              gint             index)
{
  ClayToolPalettePrivate *private = GET_PRIVATE (palette);
  GtkToolItem            *tool_button;
  GtkWidget              *button;
  GtkReliefStyle          relief;

  tool_button = clay_tool_button_new (private->toolbox, tool_item);
  gtk_tool_item_group_insert (GTK_TOOL_ITEM_GROUP (private->group),
                              tool_button, index);
  gtk_widget_set_visible (GTK_WIDGET (tool_button), TRUE);

  g_object_bind_property (tool_item,   "shown",
                          tool_button, "visible-horizontal",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (tool_item,   "shown",
                          tool_button, "visible-vertical",
                          G_BINDING_SYNC_CREATE);

  /* Workaround for a GTK bug (GNOME/gtk!9120). Without it, revealing
   * (toggling eye icon) a previuosly hidden item does not show it again
   * This ensures that the group gets gets resized/redrawn.
   */
  g_signal_connect_object (tool_item, "shown-changed",
                           G_CALLBACK (gtk_widget_queue_resize),
                           private->group, G_CONNECT_SWAPPED);

  button = gtk_bin_get_child (GTK_BIN (tool_button));

  gtk_widget_style_get (GTK_WIDGET (palette),
                        "button-relief", &relief,
                        NULL);

  gtk_button_set_relief (GTK_BUTTON (button), relief);

  g_hash_table_insert (private->buttons, tool_item, tool_button);
}
