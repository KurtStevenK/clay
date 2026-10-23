/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claydevicestatus.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/claylist.h"
#include "core/claypattern.h"
#include "core/claytoolinfo.h"

#include "claydnd.h"
#include "claydeviceinfo.h"
#include "claydevicemanager.h"
#include "claydevices.h"
#include "claydevicestatus.h"
#include "claydialogfactory.h"
#include "claypropwidgets.h"
#include "clayview.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"


#define CELL_SIZE 20 /* The size of the view cells */


enum
{
  PROP_0,
  PROP_CLAY
};


struct _ClayDeviceStatusEntry
{
  ClayDeviceInfo  *device_info;
  ClayContext     *context;
  ClayToolOptions *tool_options;

  GtkWidget       *ebox;
  GtkWidget       *options_hbox;
  GtkWidget       *tool;
  GtkWidget       *foreground;
  GtkWidget       *foreground_none;
  GtkWidget       *background;
  GtkWidget       *background_none;
  GtkWidget       *brush;
  GtkWidget       *brush_none;
  GtkWidget       *pattern;
  GtkWidget       *pattern_none;
  GtkWidget       *gradient;
  GtkWidget       *gradient_none;
};


static void clay_device_status_constructed     (GObject               *object);
static void clay_device_status_dispose         (GObject               *object);
static void clay_device_status_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);

static void clay_device_status_device_add      (ClayContainer         *devices,
                                                ClayDeviceInfo        *device_info,
                                                ClayDeviceStatus      *status);
static void clay_device_status_device_remove   (ClayContainer         *devices,
                                                ClayDeviceInfo        *device_info,
                                                ClayDeviceStatus      *status);

static void clay_device_status_notify_device   (ClayDeviceManager     *manager,
                                                const GParamSpec      *pspec,
                                                ClayDeviceStatus      *status);
static void clay_device_status_config_notify   (ClayGuiConfig         *config,
                                                const GParamSpec      *pspec,
                                                ClayDeviceStatus      *status);
static void clay_device_status_image_changed   (ClayContext           *user_context,
                                                ClayImage             *image,
                                                ClayDeviceStatus      *status);
static void clay_device_status_notify_info     (ClayDeviceInfo        *device_info,
                                                const GParamSpec      *pspec,
                                                ClayDeviceStatusEntry *entry);
static void clay_device_status_save_clicked    (GtkWidget             *button,
                                                ClayDeviceStatus      *status);
static void clay_device_status_view_clicked    (GtkWidget             *widget,
                                                GdkModifierType        state,
                                                const gchar           *identifier);

static void
        clay_device_status_set_color_help_data (ClayDeviceStatusEntry *entry,
                                                ClayContext           *user_context);


G_DEFINE_TYPE (ClayDeviceStatus, clay_device_status, CLAY_TYPE_EDITOR)

#define parent_class clay_device_status_parent_class


static void
clay_device_status_class_init (ClayDeviceStatusClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = clay_device_status_constructed;
  object_class->dispose      = clay_device_status_dispose;
  object_class->set_property = clay_device_status_set_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  gtk_widget_class_set_css_name (widget_class, "ClayDeviceStatus");
}

static void
clay_device_status_init (ClayDeviceStatus *status)
{
  status->clay           = NULL;
  status->current_device = NULL;

  status->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (status->vbox), 2);
  gtk_box_pack_start (GTK_BOX (status), status->vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (status->vbox, TRUE);

  status->save_button =
    clay_editor_add_button (CLAY_EDITOR (status), CLAY_ICON_DOCUMENT_SAVE,
                            _("Save device status"), NULL,
                            G_CALLBACK (clay_device_status_save_clicked),
                            NULL,
                            G_OBJECT (status));
}

static void
clay_device_status_constructed (GObject *object)
{
  ClayDeviceStatus *status = CLAY_DEVICE_STATUS (object);
  ClayContainer    *devices;
  GList            *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (status->clay));

  devices = CLAY_CONTAINER (clay_devices_get_manager (status->clay));

  for (list = CLAY_LIST (devices)->queue->head; list; list = list->next)
    clay_device_status_device_add (devices, list->data, status);

  g_signal_connect_object (devices, "add",
                           G_CALLBACK (clay_device_status_device_add),
                           status, 0);
  g_signal_connect_object (devices, "remove",
                           G_CALLBACK (clay_device_status_device_remove),
                           status, 0);

  g_signal_connect (devices, "notify::current-device",
                    G_CALLBACK (clay_device_status_notify_device),
                    status);

  clay_device_status_notify_device (CLAY_DEVICE_MANAGER (devices), NULL, status);

  g_signal_connect_object (status->clay->config, "notify::devices-share-tool",
                           G_CALLBACK (clay_device_status_config_notify),
                           status, 0);

  clay_device_status_config_notify (CLAY_GUI_CONFIG (status->clay->config),
                                    NULL, status);

  g_signal_connect_object (clay_get_user_context (status->clay), "image-changed",
                           G_CALLBACK (clay_device_status_image_changed),
                           status, 0);
}

static void
clay_device_status_dispose (GObject *object)
{
  ClayDeviceStatus *status = CLAY_DEVICE_STATUS (object);

  if (status->devices)
    {
      GList *list;

      for (list = status->devices; list; list = list->next)
        {
          ClayDeviceStatusEntry *entry = list->data;

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                clay_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (ClayDeviceStatusEntry, entry);
        }

      g_list_free (status->devices);
      status->devices = NULL;

      g_signal_handlers_disconnect_by_func (clay_devices_get_manager (status->clay),
                                            clay_device_status_notify_device,
                                            status);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_device_status_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayDeviceStatus *status = CLAY_DEVICE_STATUS (object);

  switch (property_id)
    {
    case PROP_CLAY:
      status->clay = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
pack_prop_widget (GtkBox     *hbox,
                  GtkWidget  *widget,
                  GtkWidget **none_widget)
{
  GtkSizeGroup *size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, widget);
  gtk_widget_set_visible (widget, TRUE);

  *none_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), *none_widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, *none_widget);

  g_object_unref (size_group);
}

static void
clay_device_status_device_add (ClayContainer    *devices,
                               ClayDeviceInfo   *device_info,
                               ClayDeviceStatus *status)
{
  ClayDeviceStatusEntry *entry;
  GdkDisplay            *display;

  GClosure              *closure;
  GParamSpec            *pspec;
  GtkWidget             *vbox;
  GtkWidget             *hbox;
  GtkWidget             *label;
  gchar                 *name;

  entry = g_slice_new0 (ClayDeviceStatusEntry);

  status->devices = g_list_prepend (status->devices, entry);

  entry->device_info = device_info;
  entry->context     = clay_context_new (CLAY_TOOL_PRESET (device_info)->clay,
                                         clay_object_get_name (device_info),
                                         NULL);

  clay_context_define_properties (entry->context,
                                  CLAY_CONTEXT_PROP_MASK_TOOL       |
                                  CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                                  CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                                  CLAY_CONTEXT_PROP_MASK_BRUSH      |
                                  CLAY_CONTEXT_PROP_MASK_PATTERN    |
                                  CLAY_CONTEXT_PROP_MASK_GRADIENT,
                                  FALSE);

  closure = g_cclosure_new (G_CALLBACK (clay_device_status_notify_info),
                            entry, NULL);
  g_object_watch_closure (G_OBJECT (status), closure);
  g_signal_connect_closure (device_info, "notify", closure,
                            FALSE);

  entry->ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (status->vbox), entry->ebox,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (entry->ebox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (entry->ebox), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  /*  the device name  */

  clay_device_info_get_device (device_info, &display);
  if (display == NULL || display == gdk_display_get_default ())
    name = g_strdup (clay_object_get_name (device_info));
  else
    name = g_strdup_printf ("%s (%s)",
                            clay_object_get_name (device_info),
                            gdk_display_get_name (display));

  label = gtk_label_new (name);
  g_free (name);

  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  /*  the row of properties  */

  hbox = entry->options_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  the tool  */

  entry->tool = clay_prop_view_new (G_OBJECT (entry->context), "tool",
                                    entry->context, CELL_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry->tool, FALSE, FALSE, 0);

  /*  the foreground color  */

  entry->foreground = clay_prop_color_area_new (G_OBJECT (entry->context),
                                                "foreground",
                                                CELL_SIZE, CELL_SIZE,
                                                CLAY_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->foreground,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->foreground, &entry->foreground_none);

  /*  the background color  */

  entry->background = clay_prop_color_area_new (G_OBJECT (entry->context),
                                                "background",
                                                CELL_SIZE, CELL_SIZE,
                                                CLAY_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->background,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->background, &entry->background_none);

  /*  the brush  */

  entry->brush = clay_prop_view_new (G_OBJECT (entry->context), "brush",
                                     entry->context, CELL_SIZE);
  CLAY_VIEW (entry->brush)->clickable  = TRUE;
  CLAY_VIEW (entry->brush)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->brush, &entry->brush_none);

  g_signal_connect (entry->brush, "clicked",
                    G_CALLBACK (clay_device_status_view_clicked),
                    "clay-brush-grid|clay-brush-list");

  /*  the pattern  */

  entry->pattern = clay_prop_view_new (G_OBJECT (entry->context), "pattern",
                                       entry->context, CELL_SIZE);
  CLAY_VIEW (entry->pattern)->clickable  = TRUE;
  CLAY_VIEW (entry->pattern)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->pattern, &entry->pattern_none);

  g_signal_connect (entry->pattern, "clicked",
                    G_CALLBACK (clay_device_status_view_clicked),
                    "clay-pattern-grid|clay-pattern-list");

  /*  the gradient  */

  entry->gradient = clay_prop_view_new (G_OBJECT (entry->context), "gradient",
                                        entry->context, 2 * CELL_SIZE);
  CLAY_VIEW (entry->gradient)->clickable  = TRUE;
  CLAY_VIEW (entry->gradient)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->gradient, &entry->gradient_none);

  g_signal_connect (entry->gradient, "clicked",
                    G_CALLBACK (clay_device_status_view_clicked),
                    "clay-gradient-list|clay-gradient-grid");

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (device_info),
                                        "tool-options");
  clay_device_status_notify_info (device_info, pspec, entry);
}

static void
clay_device_status_device_remove (ClayContainer    *devices,
                                  ClayDeviceInfo   *device_info,
                                  ClayDeviceStatus *status)
{
  GList *list;

  for (list = status->devices; list; list = list->next)
    {
      ClayDeviceStatusEntry *entry = list->data;

      if (entry->device_info == device_info)
        {
          status->devices = g_list_remove (status->devices, entry);

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                clay_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (ClayDeviceStatusEntry, entry);

          return;
        }
    }
}

GtkWidget *
clay_device_status_new (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_new (CLAY_TYPE_DEVICE_STATUS,
                       "clay", clay,
                       NULL);
}


/*  private functions  */

static void
clay_device_status_notify_device (ClayDeviceManager *manager,
                                  const GParamSpec  *pspec,
                                  ClayDeviceStatus  *status)
{
  GList *list;

  status->current_device = clay_device_manager_get_current_device (manager);

  for (list = status->devices; list; list = list->next)
    {
      ClayDeviceStatusEntry *entry  = list->data;
      GtkWidget             *widget = entry->ebox;
      GtkStyleContext       *style  = gtk_widget_get_style_context (widget);

      if (entry->device_info == status->current_device)
        {
          gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_SELECTED, TRUE);
          gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
        }
      else
        {
          gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_NORMAL, TRUE);
          gtk_style_context_remove_class (style, GTK_STYLE_CLASS_VIEW);
        }
    }
}

static void
clay_device_status_config_notify (ClayGuiConfig    *config,
                                  const GParamSpec *pspec,
                                  ClayDeviceStatus *status)
{
  gboolean  show_options;
  GList    *list;

  show_options = ! CLAY_GUI_CONFIG (status->clay->config)->devices_share_tool;

  for (list = status->devices; list; list = list->next)
    {
      ClayDeviceStatusEntry *entry = list->data;

      gtk_widget_set_visible (entry->options_hbox, show_options);
    }
}

static void
clay_device_status_image_changed (ClayContext      *user_context,
                                  ClayImage        *image,
                                  ClayDeviceStatus *status)
{
  GList *list;

  for (list = status->devices; list; list = list->next)
    {
      ClayDeviceStatusEntry *entry = list->data;

      clay_device_status_set_color_help_data (entry, user_context);
    }
}

static void
toggle_prop_visible (GtkWidget *widget,
                     GtkWidget *widget_none,
                     gboolean   available)
{
  gtk_widget_set_visible (widget, available);
  gtk_widget_set_visible (widget_none, ! available);
}

static void
clay_device_status_notify_info (ClayDeviceInfo        *device_info,
                                const GParamSpec      *pspec,
                                ClayDeviceStatusEntry *entry)
{
  ClayToolOptions *tool_options = CLAY_TOOL_PRESET (device_info)->tool_options;

  if (tool_options != entry->tool_options)
    {
      ClayContextPropMask serialize_props;

      entry->tool_options = tool_options;
      clay_context_set_parent (entry->context, CLAY_CONTEXT (tool_options));

      serialize_props =
        clay_context_get_serialize_properties (CLAY_CONTEXT (tool_options));

      toggle_prop_visible (entry->foreground,
                           entry->foreground_none,
                           serialize_props & CLAY_CONTEXT_PROP_MASK_FOREGROUND);

      toggle_prop_visible (entry->background,
                           entry->background_none,
                           serialize_props & CLAY_CONTEXT_PROP_MASK_BACKGROUND);

      toggle_prop_visible (entry->brush,
                           entry->brush_none,
                           serialize_props & CLAY_CONTEXT_PROP_MASK_BRUSH);

      toggle_prop_visible (entry->pattern,
                           entry->pattern_none,
                           serialize_props & CLAY_CONTEXT_PROP_MASK_PATTERN);

      toggle_prop_visible (entry->gradient,
                           entry->gradient_none,
                           serialize_props & CLAY_CONTEXT_PROP_MASK_GRADIENT);
    }

  if (! clay_device_info_get_device (device_info, NULL) ||
      clay_device_info_get_mode (device_info) == GDK_MODE_DISABLED)
    {
      gtk_widget_set_visible (entry->ebox, FALSE);
    }
  else
    {
      gtk_widget_set_visible (entry->ebox, TRUE);
    }

  if (! strcmp (pspec->name, "tool-options"))
    clay_device_status_set_color_help_data (entry,
                                            clay_get_user_context (entry->context->clay));
}

static void
clay_device_status_save_clicked (GtkWidget        *button,
                                 ClayDeviceStatus *status)
{
  clay_devices_save (status->clay, TRUE);
}

static void
clay_device_status_view_clicked (GtkWidget       *widget,
                                 GdkModifierType  state,
                                 const gchar     *identifier)
{
  ClayDeviceStatus  *status;
  ClayDialogFactory *dialog_factory;

  status = CLAY_DEVICE_STATUS (gtk_widget_get_ancestor (widget,
                                                        CLAY_TYPE_DEVICE_STATUS));
  dialog_factory = clay_dialog_factory_get_singleton ();

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (status->clay)),
                                             status->clay,
                                             dialog_factory,
                                             clay_widget_get_monitor (widget),
                                             identifier);
}

static void
clay_device_status_set_color_help_data (ClayDeviceStatusEntry *entry,
                                        ClayContext           *user_context)
{
  const Babl *format;
  ClayImage  *image = NULL;
  GeglColor  *color;
  guchar      rgb[4];
  gchar      *buf;

  /* Note: we don't set the GeglColor on purpose. Whatever is the specific
   * color space is too disconnected to this GUI usage and showing info in
   * this space would only be confusing.
   */
  format = clay_context_get_rgba_format (user_context, NULL, "u8", &image);

  color = clay_context_get_foreground (entry->context);
  gegl_color_get_pixel (color, format, rgb);
  if (image != NULL)
    buf = g_strdup_printf (_("Foreground: %d, %d, %d (in color space of \"%s\")"),
                           rgb[0], rgb[1], rgb[2],
                           clay_image_get_display_name (image));
  else
    buf = g_strdup_printf (_("Foreground: %d, %d, %d (in sRGB)"), rgb[0], rgb[1], rgb[2]);
  clay_help_set_help_data (entry->foreground, buf, NULL);
  g_free (buf);

  color = clay_context_get_background (entry->context);
  gegl_color_get_pixel (color, format, rgb);
  if (image != NULL)
    buf = g_strdup_printf (_("Background: %d, %d, %d (in color space of \"%s\")"),
                           rgb[0], rgb[1], rgb[2],
                           clay_image_get_display_name (image));
  else
    buf = g_strdup_printf (_("Background: %d, %d, %d (in sRGB)"), rgb[0], rgb[1], rgb[2]);
  clay_help_set_help_data (entry->background, buf, NULL);
  g_free (buf);
}
