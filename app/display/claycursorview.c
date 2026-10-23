/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycursorview.c
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
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
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-color.h"
#include "core/clayitem.h"

#include "widgets/claycolorframe.h"
#include "widgets/claydocked.h"
#include "widgets/claymenufactory.h"
#include "widgets/claysessioninfo-aux.h"

#include "claycursorview.h"
#include "claydisplay.h"
#include "claydisplayshell.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_SAMPLE_MERGED
};


struct _ClayCursorViewPrivate
{
  ClayEditor        parent_instance;

  Clay             *clay;

  GtkWidget        *coord_hbox;
  GtkWidget        *selection_hbox;
  GtkWidget        *color_hbox;

  GtkWidget        *pixel_x_label;
  GtkWidget        *pixel_y_label;
  GtkWidget        *unit_x_label;
  GtkWidget        *unit_y_label;
  GtkWidget        *selection_x_label;
  GtkWidget        *selection_y_label;
  GtkWidget        *selection_width_label;
  GtkWidget        *selection_height_label;
  GtkWidget        *color_frame_1;
  GtkWidget        *color_frame_2;

  gboolean          sample_merged;

  ClayContext      *context;
  ClayDisplayShell *shell;
  ClayImage        *image;

  ClayUnit         *unit;
  guint             cursor_idle_id;
  ClayImage        *cursor_image;
  ClayUnit         *cursor_unit;
  gboolean          cursor_show_all;
  gdouble           cursor_x;
  gdouble           cursor_y;
};


static void       clay_cursor_view_docked_iface_init     (ClayDockedInterface *iface);

static void       clay_cursor_view_dispose               (GObject             *object);
static void       clay_cursor_view_constructed           (GObject             *object);
static void       clay_cursor_view_set_property          (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void       clay_cursor_view_get_property          (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void       clay_cursor_view_style_updated         (GtkWidget           *widget);

static void       clay_cursor_view_set_aux_info          (ClayDocked          *docked,
                                                          GList               *aux_info);
static GList    * clay_cursor_view_get_aux_info          (ClayDocked          *docked);

static void       clay_cursor_view_set_context           (ClayDocked          *docked,
                                                          ClayContext         *context);
static void       clay_cursor_view_image_changed         (ClayCursorView      *view,
                                                          ClayImage           *image,
                                                          ClayContext         *context);
static void       clay_cursor_view_mask_changed          (ClayCursorView      *view,
                                                          ClayImage           *image);
static void       clay_cursor_view_display_changed       (ClayCursorView      *view,
                                                          ClayDisplay         *display,
                                                          ClayContext         *context);
static void       clay_cursor_view_shell_unit_changed    (ClayCursorView      *view,
                                                          GParamSpec          *pspec,
                                                          ClayDisplayShell    *shell);
static void       clay_cursor_view_format_as_unit        (ClayUnit            *unit,
                                                          gchar               *output_buf,
                                                          gint                 output_buf_size,
                                                          gdouble              pixel_value,
                                                          gdouble              image_res);
static void       clay_cursor_view_set_label_italic      (GtkWidget           *label,
                                                          gboolean             italic);
static void       clay_cursor_view_update_selection_info (ClayCursorView      *view,
                                                          ClayImage           *image,
                                                          ClayUnit            *unit);
static gboolean   clay_cursor_view_cursor_idle           (ClayCursorView      *view);


G_DEFINE_TYPE_WITH_CODE (ClayCursorView, clay_cursor_view, CLAY_TYPE_EDITOR,
                         G_ADD_PRIVATE (ClayCursorView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_cursor_view_docked_iface_init))

#define parent_class clay_cursor_view_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_cursor_view_class_init (ClayCursorViewClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = clay_cursor_view_constructed;
  object_class->dispose       = clay_cursor_view_dispose;
  object_class->get_property  = clay_cursor_view_get_property;
  object_class->set_property  = clay_cursor_view_set_property;

  widget_class->style_updated = clay_cursor_view_style_updated;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SAMPLE_MERGED,
                                   g_param_spec_boolean ("sample-merged",
                                                         NULL, NULL,
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
clay_cursor_view_init (ClayCursorView *view)
{
  GtkWidget *frame;
  GtkWidget *grid;
  GtkWidget *toggle;
  gint       content_spacing;

  view->priv = clay_cursor_view_get_instance_private (view);

  view->priv->sample_merged  = TRUE;
  view->priv->context        = NULL;
  view->priv->shell          = NULL;
  view->priv->image          = NULL;
  view->priv->unit           = clay_unit_pixel ();
  view->priv->cursor_idle_id = 0;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);


  /* cursor information */

  view->priv->coord_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                        content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->coord_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->coord_hbox,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (view->priv->coord_hbox, TRUE);

  view->priv->selection_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                            content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->selection_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->selection_hbox,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (view->priv->selection_hbox, TRUE);


  /* Pixels */

  frame = clay_frame_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  view->priv->pixel_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_x_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->pixel_x_label, 1);

  view->priv->pixel_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->pixel_y_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->pixel_y_label, 1);


  /* Units */

  frame = clay_frame_new (_("Units"));
  gtk_box_pack_start (GTK_BOX (view->priv->coord_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  view->priv->unit_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_x_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->unit_x_label, 1);

  view->priv->unit_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->unit_y_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->unit_y_label, 1);


  /* Selection Bounding Box */

  frame = clay_frame_new (_("Selection"));
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  clay_help_set_help_data (frame, _("The selection's bounding box"), NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  view->priv->selection_x_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_x_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X"), 0.5, 0.5,
                            view->priv->selection_x_label, 1);

  view->priv->selection_y_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_y_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y"), 0.5, 0.5,
                            view->priv->selection_y_label, 1);

  frame = clay_frame_new ("");
  gtk_box_pack_start (GTK_BOX (view->priv->selection_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  view->priv->selection_width_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_width_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            /* Width */
                            _("W"), 0.5, 0.5,
                            view->priv->selection_width_label, 1);

  view->priv->selection_height_label = gtk_label_new (_("n/a"));
  gtk_label_set_xalign (GTK_LABEL (view->priv->selection_height_label), 1.0);
  clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            /* Height */
                            _("H"), 0.5, 0.5,
                            view->priv->selection_height_label, 1);

  /* sample merged toggle */

  toggle = clay_prop_check_button_new (G_OBJECT (view), "sample-merged",
                                       _("_Sample Merged"));
  gtk_box_pack_start (GTK_BOX (view), toggle, FALSE, FALSE, 0);
}

static void
clay_cursor_view_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_aux_info = clay_cursor_view_set_aux_info;
  iface->get_aux_info = clay_cursor_view_get_aux_info;
  iface->set_context  = clay_cursor_view_set_context;
}

static void
clay_cursor_view_constructed (GObject             *object)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (object);
  gint            content_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  /* color information */
  view->priv->color_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                        content_spacing);
  gtk_box_set_homogeneous (GTK_BOX (view->priv->color_hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (view), view->priv->color_hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (view->priv->color_hbox, TRUE);

  view->priv->color_frame_1 = clay_color_frame_new (view->priv->clay);
  clay_color_frame_set_mode (CLAY_COLOR_FRAME (view->priv->color_frame_1),
                             CLAY_COLOR_PICK_MODE_PIXEL);
  clay_color_frame_set_ellipsize (CLAY_COLOR_FRAME (view->priv->color_frame_1),
                                  PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_1,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (view->priv->color_frame_1, TRUE);

  view->priv->color_frame_2 = clay_color_frame_new (view->priv->clay);
  clay_color_frame_set_mode (CLAY_COLOR_FRAME (view->priv->color_frame_2),
                             CLAY_COLOR_PICK_MODE_RGB_PERCENT);
  gtk_box_pack_start (GTK_BOX (view->priv->color_hbox), view->priv->color_frame_2,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (view->priv->color_frame_2, TRUE);

}

static void
clay_cursor_view_dispose (GObject *object)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (object);

  if (view->priv->context)
    clay_docked_set_context (CLAY_DOCKED (view), NULL);

  if (view->priv->cursor_idle_id)
    {
      g_source_remove (view->priv->cursor_idle_id);
      view->priv->cursor_idle_id = 0;
    }

  view->priv->color_frame_1 = NULL;
  view->priv->color_frame_2 = NULL;

  g_clear_object (&view->priv->cursor_image);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_cursor_view_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_CLAY:
      view->priv->clay = g_value_get_object (value);
      break;

    case PROP_SAMPLE_MERGED:
      view->priv->sample_merged = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_cursor_view_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, view->priv->clay);
      break;

    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, view->priv->sample_merged);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

#define AUX_INFO_FRAME_1_MODE "frame-1-mode"
#define AUX_INFO_FRAME_2_MODE "frame-2-mode"

static void
clay_cursor_view_set_aux_info (ClayDocked *docked,
                               GList      *aux_info)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (docked);
  GList          *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux   = list->data;
      GtkWidget          *frame = NULL;

      if (! strcmp (aux->name, AUX_INFO_FRAME_1_MODE))
        frame = view->priv->color_frame_1;
      else if (! strcmp (aux->name, AUX_INFO_FRAME_2_MODE))
        frame = view->priv->color_frame_2;

      if (frame)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          enum_class = g_type_class_peek (CLAY_TYPE_COLOR_PICK_MODE);
          enum_value = g_enum_get_value_by_nick (enum_class, aux->value);

          if (enum_value)
            clay_color_frame_set_mode (CLAY_COLOR_FRAME (frame),
                                       enum_value->value);
        }
    }
}

static GList *
clay_cursor_view_get_aux_info (ClayDocked *docked)
{
  ClayCursorView     *view = CLAY_CURSOR_VIEW (docked);
  GList              *aux_info;
  const gchar        *nick;
  ClaySessionInfoAux *aux;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (clay_enum_get_value (CLAY_TYPE_COLOR_PICK_MODE,
                           CLAY_COLOR_FRAME (view->priv->color_frame_1)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = clay_session_info_aux_new (AUX_INFO_FRAME_1_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  if (clay_enum_get_value (CLAY_TYPE_COLOR_PICK_MODE,
                           CLAY_COLOR_FRAME (view->priv->color_frame_2)->pick_mode,
                           NULL, &nick, NULL, NULL))
    {
      aux = clay_session_info_aux_new (AUX_INFO_FRAME_2_MODE, nick);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
clay_cursor_view_format_as_unit (ClayUnit *unit,
                                 gchar    *output_buf,
                                 gint      output_buf_size,
                                 gdouble   pixel_value,
                                 gdouble   image_res)
{
  gchar         format_buf[32];
  gdouble       value;
  gint          unit_digits = 0;
  const gchar  *unit_str = "";

  value = clay_pixels_to_units (pixel_value, unit, image_res);

  if (unit != clay_unit_pixel ())
    {
      unit_digits = clay_unit_get_scaled_digits (unit, image_res);
      unit_str    = clay_unit_get_abbreviation (unit);
    }

  g_snprintf (format_buf, sizeof (format_buf),
              "%%.%df %s", unit_digits, unit_str);

  g_snprintf (output_buf, output_buf_size, format_buf, value);
}

static void
clay_cursor_view_set_label_italic (GtkWidget *label,
                                   gboolean   italic)
{
  PangoStyle attribute = italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;

  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, attribute,
                             -1);
}

static void
clay_cursor_view_style_updated (GtkWidget *widget)
{
  ClayCursorView *view = CLAY_CURSOR_VIEW (widget);
  gint            content_spacing;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        NULL);

  gtk_box_set_spacing (GTK_BOX (view->priv->coord_hbox),     content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->selection_hbox), content_spacing);
  gtk_box_set_spacing (GTK_BOX (view->priv->color_hbox),     content_spacing);
}

static void
clay_cursor_view_set_context (ClayDocked  *docked,
                              ClayContext *context)
{
  ClayCursorView  *view    = CLAY_CURSOR_VIEW (docked);
  ClayColorConfig *config  = NULL;
  ClayDisplay     *display = NULL;
  ClayImage       *image   = NULL;

  if (context == view->priv->context)
    return;

  if (view->priv->context)
    {
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            clay_cursor_view_display_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->context,
                                            clay_cursor_view_image_changed,
                                            view);

      g_object_unref (view->priv->context);
    }

  view->priv->context = context;

  if (view->priv->context)
    {
      g_object_ref (view->priv->context);

      g_signal_connect_swapped (view->priv->context, "display-changed",
                                G_CALLBACK (clay_cursor_view_display_changed),
                                view);

      g_signal_connect_swapped (view->priv->context, "image-changed",
                                G_CALLBACK (clay_cursor_view_image_changed),
                                view);

      config  = context->clay->config->color_management;
      display = clay_context_get_display (context);
      image   = clay_context_get_image (context);
    }

  clay_color_frame_set_color_config (CLAY_COLOR_FRAME (view->priv->color_frame_1),
                                     config);
  clay_color_frame_set_color_config (CLAY_COLOR_FRAME (view->priv->color_frame_2),
                                     config);

  clay_cursor_view_display_changed (view, display, view->priv->context);
  clay_cursor_view_image_changed (view, image, view->priv->context);
}

static void
clay_cursor_view_image_changed (ClayCursorView *view,
                                ClayImage      *image,
                                ClayContext    *context)
{
  g_return_if_fail (CLAY_IS_CURSOR_VIEW (view));

  if (image == view->priv->image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            clay_cursor_view_mask_changed,
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      g_signal_connect_swapped (view->priv->image, "mask-changed",
                                G_CALLBACK (clay_cursor_view_mask_changed),
                                view);
    }

  clay_cursor_view_mask_changed (view, view->priv->image);
}

static void
clay_cursor_view_mask_changed (ClayCursorView *view,
                               ClayImage      *image)
{
  clay_cursor_view_update_selection_info (view,
                                          view->priv->image,
                                          view->priv->unit);
}

static void
clay_cursor_view_display_changed (ClayCursorView *view,
                                  ClayDisplay    *display,
                                  ClayContext    *context)
{
  ClayDisplayShell *shell = NULL;

  if (display)
    shell = clay_display_get_shell (display);

  if (view->priv->shell)
    {
      g_signal_handlers_disconnect_by_func (view->priv->shell,
                                            clay_cursor_view_shell_unit_changed,
                                            view);
    }

  view->priv->shell = shell;

  if (view->priv->shell)
    {
      g_signal_connect_swapped (view->priv->shell, "notify::unit",
                                G_CALLBACK (clay_cursor_view_shell_unit_changed),
                                view);
    }

  clay_cursor_view_shell_unit_changed (view,
                                       NULL,
                                       view->priv->shell);
}

static void
clay_cursor_view_shell_unit_changed (ClayCursorView   *view,
                                     GParamSpec       *pspec,
                                     ClayDisplayShell *shell)
{
  ClayUnit *new_unit = clay_unit_pixel ();

  if (shell)
    {
      new_unit = clay_display_shell_get_unit (shell);
    }

  if (view->priv->unit != new_unit)
    {
      clay_cursor_view_update_selection_info (view, view->priv->image, new_unit);
      view->priv->unit = new_unit;
    }
}

static void
clay_cursor_view_update_selection_info (ClayCursorView *view,
                                        ClayImage      *image,
                                        ClayUnit       *unit)
{
  gint x, y, width, height;

  if (image &&
      clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                        &x, &y, &width, &height))
    {
      gdouble xres, yres;
      gchar   buf[32];

      clay_image_get_resolution (image, &xres, &yres);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_x_label), buf);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_y_label), buf);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), width, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_width_label), buf);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), height, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->selection_height_label), buf);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->priv->selection_x_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_y_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_width_label),
                          _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->selection_height_label),
                          _("n/a"));
    }
}

static gboolean
clay_cursor_view_cursor_idle (ClayCursorView *view)
{
  if (view->priv->cursor_image)
    {
      ClayImage  *image    = view->priv->cursor_image;
      ClayUnit   *unit     = view->priv->cursor_unit;
      gboolean    show_all = view->priv->cursor_show_all;
      gdouble     x        = view->priv->cursor_x;
      gdouble     y        = view->priv->cursor_y;
      gboolean    in_image;
      gchar       buf[32];
      const Babl *sample_format;
      gdouble     pixel[4];
      GeglColor  *color;
      gdouble     xres;
      gdouble     yres;
      gint        int_x;
      gint        int_y;

      if (unit == clay_unit_pixel ())
        unit = clay_image_get_unit (image);

      clay_image_get_resolution (image, &xres, &yres);

      in_image = (x >= 0.0 && x < clay_image_get_width  (image) &&
                  y >= 0.0 && y < clay_image_get_height (image));

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (x));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_x_label), buf);
      clay_cursor_view_set_label_italic (view->priv->pixel_x_label, ! in_image);

      g_snprintf (buf, sizeof (buf), "%d", (gint) floor (y));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_y_label), buf);
      clay_cursor_view_set_label_italic (view->priv->pixel_y_label, ! in_image);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), x, xres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_x_label), buf);
      clay_cursor_view_set_label_italic (view->priv->unit_x_label, ! in_image);

      clay_cursor_view_format_as_unit (unit, buf, sizeof (buf), y, yres);
      gtk_label_set_text (GTK_LABEL (view->priv->unit_y_label), buf);
      clay_cursor_view_set_label_italic (view->priv->unit_y_label, ! in_image);

      int_x = (gint) floor (x);
      int_y = (gint) floor (y);

      color = gegl_color_new ("black");
      if (clay_image_pick_color (image, NULL,
                                 int_x, int_y,
                                 show_all,
                                 view->priv->sample_merged,
                                 FALSE, 0.0,
                                 &sample_format, pixel, &color))
        {
          clay_color_frame_set_color (CLAY_COLOR_FRAME (view->priv->color_frame_1),
                                      FALSE, color, int_x, int_y);
          clay_color_frame_set_color (CLAY_COLOR_FRAME (view->priv->color_frame_2),
                                      FALSE, color, int_x, int_y);
        }
      else
        {
          clay_color_frame_set_invalid (CLAY_COLOR_FRAME (view->priv->color_frame_1));
          clay_color_frame_set_invalid (CLAY_COLOR_FRAME (view->priv->color_frame_2));
        }

      /* Show the selection info from the image under the cursor if any */
      clay_cursor_view_update_selection_info (view,
                                              image,
                                              view->priv->cursor_unit);

      g_clear_object (&view->priv->cursor_image);
      g_object_unref (color);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_x_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->pixel_y_label), _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->unit_x_label),  _("n/a"));
      gtk_label_set_text (GTK_LABEL (view->priv->unit_y_label),  _("n/a"));

      clay_color_frame_set_invalid (CLAY_COLOR_FRAME (view->priv->color_frame_1));
      clay_color_frame_set_invalid (CLAY_COLOR_FRAME (view->priv->color_frame_2));

      /* Start showing selection info from the active image again */
      clay_cursor_view_update_selection_info (view,
                                              view->priv->image,
                                              view->priv->unit);
    }

  view->priv->cursor_idle_id = 0;

  return G_SOURCE_REMOVE;
}


/*  public functions  */

GtkWidget *
clay_cursor_view_new (Clay            *clay,
                      ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_CURSOR_VIEW,
                       "clay",            clay,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<CursorInfo>",
                       "ui-path",         "/cursor-info-popup",
                       NULL);
}

void
clay_cursor_view_set_sample_merged (ClayCursorView *view,
                                    gboolean        sample_merged)
{
  g_return_if_fail (CLAY_IS_CURSOR_VIEW (view));

  sample_merged = sample_merged ? TRUE : FALSE;

  if (view->priv->sample_merged != sample_merged)
    {
      view->priv->sample_merged = sample_merged;

      g_object_notify (G_OBJECT (view), "sample-merged");
    }
}

gboolean
clay_cursor_view_get_sample_merged (ClayCursorView *view)
{
  g_return_val_if_fail (CLAY_IS_CURSOR_VIEW (view), FALSE);

  return view->priv->sample_merged;
}

void
clay_cursor_view_update_cursor (ClayCursorView   *view,
                                ClayImage        *image,
                                ClayUnit         *shell_unit,
                                gboolean          shell_show_all,
                                gdouble           x,
                                gdouble           y)
{
  g_return_if_fail (CLAY_IS_CURSOR_VIEW (view));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  g_clear_object (&view->priv->cursor_image);

  view->priv->cursor_image    = g_object_ref (image);
  view->priv->cursor_unit     = shell_unit;
  view->priv->cursor_show_all = shell_show_all;
  view->priv->cursor_x        = x;
  view->priv->cursor_y        = y;

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) clay_cursor_view_cursor_idle, view);
    }
}

void
clay_cursor_view_clear_cursor (ClayCursorView *view)
{
  g_return_if_fail (CLAY_IS_CURSOR_VIEW (view));

  g_clear_object (&view->priv->cursor_image);

  if (view->priv->cursor_idle_id == 0)
    {
      view->priv->cursor_idle_id =
        g_idle_add ((GSourceFunc) clay_cursor_view_cursor_idle, view);
    }
}
