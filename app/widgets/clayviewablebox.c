/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayconfig-utils.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"

#include "claycontainerentry.h"
#include "claydialogfactory.h"
#include "claypropwidgets.h"
#include "clayview.h"
#include "clayviewablebutton.h"
#include "clayviewablebox.h"
#include "clayviewrenderergradient.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"


/*  local function prototypes  */

static GtkWidget * clay_viewable_box_new       (ClayContainer *container,
                                                ClayContext   *context,
                                                const gchar   *label,
                                                gint           spacing,
                                                ClayViewType   view_type,
                                                ClayViewSize   button_view_size,
                                                ClayViewSize   view_size,
                                                const gchar   *dialog_identifier,
                                                const gchar   *dialog_icon_name,
                                                const gchar   *dialog_tooltip,
                                                const gchar   *editor_id,
                                                const gchar   *editor_tooltip);
static GtkWidget * view_props_connect          (GtkWidget     *box,
                                                ClayContext   *context,
                                                const gchar   *view_type_prop,
                                                const gchar   *view_size_prop);
static void   clay_viewable_box_edit_clicked   (GtkWidget          *widget,
                                                ClayViewableButton *button);
static void   clay_gradient_box_reverse_notify (GObject       *object,
                                                GParamSpec    *pspec,
                                                ClayView      *view);
static void   clay_gradient_box_blend_notify   (GObject       *object,
                                                GParamSpec    *pspec,
                                                ClayView      *view);


/*  brush boxes  */

static GtkWidget *
brush_box_new (ClayContainer *container,
               ClayContext   *context,
               const gchar   *label,
               gint           spacing,
               ClayViewType   view_type,
               ClayViewSize   view_size,
               const gchar   *editor_id,
               const gchar   *editor_tooltip)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->brush_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-brush-grid|clay-brush-list",
                                CLAY_ICON_BRUSH,
                                _("Open the brush selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
clay_brush_box_new (ClayContainer *container,
                    ClayContext   *context,
                    const gchar   *label,
                    gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return brush_box_new (container, context, label, spacing,
                        CLAY_VIEW_TYPE_GRID, CLAY_VIEW_SIZE_MEDIUM,
                        NULL, NULL);
}

GtkWidget *
clay_prop_brush_box_new (ClayContainer *container,
                         ClayContext   *context,
                         const gchar   *label,
                         gint           spacing,
                         const gchar   *view_type_prop,
                         const gchar   *view_size_prop,
                         const gchar   *editor_id,
                         const gchar   *editor_tooltip)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (brush_box_new (container, context, label, spacing,
                                            view_type, view_size,
                                            editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  dynamics boxes  */

static GtkWidget *
dynamics_box_new (ClayContainer *container,
                  ClayContext   *context,
                  const gchar   *label,
                  gint           spacing,
                  ClayViewType   view_type,
                  ClayViewSize   view_size,
                  const gchar   *editor_id,
                  const gchar   *editor_tooltip)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->dynamics_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-dynamics-list|clay-dynamics-grid",
                                CLAY_ICON_DYNAMICS,
                                _("Open the dynamics selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
clay_dynamics_box_new (ClayContainer *container,
                       ClayContext   *context,
                       const gchar   *label,
                       gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return dynamics_box_new (container, context, label, spacing,
                           CLAY_VIEW_TYPE_LIST, CLAY_VIEW_SIZE_MEDIUM,
                           NULL, NULL);
}

GtkWidget *
clay_prop_dynamics_box_new (ClayContainer *container,
                            ClayContext   *context,
                            const gchar   *label,
                            gint           spacing,
                            const gchar   *view_type_prop,
                            const gchar   *view_size_prop,
                            const gchar   *editor_id,
                            const gchar   *editor_tooltip)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (dynamics_box_new (container, context, label,
                                               spacing,
                                               view_type, view_size,
                                               editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  brush boxes  */

static GtkWidget *
mybrush_box_new (ClayContainer *container,
                 ClayContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 ClayViewType   view_type,
                 ClayViewSize   view_size)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->mybrush_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_LARGE, view_size,
                                "clay-mypaint-brush-grid|clay-mypaint-brush-list",
                                CLAY_ICON_BRUSH,
                                _("Open the MyPaint brush selection dialog"),
                                NULL, NULL);
}

GtkWidget *
clay_mybrush_box_new (ClayContainer *container,
                      ClayContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return mybrush_box_new (container, context, label, spacing,
                          CLAY_VIEW_TYPE_GRID, CLAY_VIEW_SIZE_LARGE);
}

GtkWidget *
clay_prop_mybrush_box_new (ClayContainer *container,
                           ClayContext   *context,
                           const gchar   *label,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop)
{
  ClayViewType view_type = CLAY_VIEW_TYPE_GRID;
  ClayViewSize view_size = CLAY_VIEW_SIZE_LARGE;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  if (view_type_prop && view_size_prop)
    g_object_get (context,
                  view_type_prop, &view_type,
                  view_size_prop, &view_size,
                  NULL);

  return view_props_connect (mybrush_box_new (container, context, label, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  pattern boxes  */

static GtkWidget *
pattern_box_new (ClayContainer *container,
                 ClayContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 ClayViewType   view_type,
                 ClayViewSize   view_size)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->pattern_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-pattern-grid|clay-pattern-list",
                                CLAY_ICON_PATTERN,
                                _("Open the pattern selection dialog"),
                                NULL, NULL);
}

GtkWidget *
clay_pattern_box_new (ClayContainer *container,
                      ClayContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return pattern_box_new (container, context, label, spacing,
                          CLAY_VIEW_TYPE_GRID, CLAY_VIEW_SIZE_MEDIUM);
}

GtkWidget *
clay_prop_pattern_box_new (ClayContainer *container,
                           ClayContext   *context,
                           const gchar   *label,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (pattern_box_new (container, context, label, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  gradient boxes  */

static GtkWidget *
gradient_box_new (ClayContainer *container,
                  ClayContext   *context,
                  const gchar   *label,
                  gint           spacing,
                  ClayViewType   view_type,
                  ClayViewSize   view_size,
                  const gchar   *reverse_prop,
                  const gchar   *blend_color_space_prop,
                  const gchar   *editor_id,
                  const gchar   *editor_tooltip)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GList     *children;

  if (! container)
    container = clay_data_factory_get_container (context->clay->gradient_factory);

  hbox = clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-gradient-list|clay-gradient-grid",
                                CLAY_ICON_GRADIENT,
                                _("Open the gradient selection dialog"),
                                editor_id, editor_tooltip);

  children = gtk_container_get_children (GTK_CONTAINER (hbox));
  button = children->data;
  g_list_free (children);

  CLAY_VIEWABLE_BUTTON (button)->button_view_size = CLAY_VIEW_SIZE_MEDIUM;

  if (reverse_prop)
    {
      GtkWidget *toggle;
      GtkWidget *view;
      GtkWidget *image;
      gchar     *signal_name;

      toggle = clay_prop_check_button_new (G_OBJECT (context), reverse_prop,
                                           NULL);
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (toggle), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (hbox), toggle, 1);
      gtk_widget_set_visible (toggle, TRUE);

      clay_help_set_help_data (toggle, _("Reverse"), NULL);

      image = gtk_image_new_from_icon_name (CLAY_ICON_OBJECT_FLIP_HORIZONTAL,
                                            GTK_ICON_SIZE_MENU);
      /* clay_prop_check_button_new() adds the property nick as label of
       * the button by default. */
      gtk_container_remove (GTK_CONTAINER (toggle),
                            gtk_bin_get_child (GTK_BIN (toggle)));
      gtk_container_add (GTK_CONTAINER (toggle), image);
      gtk_widget_set_visible (image, TRUE);

      view = gtk_bin_get_child (GTK_BIN (button));

      signal_name = g_strconcat ("notify::", reverse_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (clay_gradient_box_reverse_notify),
                               G_OBJECT (view), 0);
      g_free (signal_name);

      clay_gradient_box_reverse_notify (G_OBJECT (context),
                                        NULL,
                                        CLAY_VIEW (view));
    }

  if (blend_color_space_prop)
    {
      GtkWidget *view;
      gchar     *signal_name;

      view = gtk_bin_get_child (GTK_BIN (button));

      signal_name = g_strconcat ("notify::", blend_color_space_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (clay_gradient_box_blend_notify),
                               G_OBJECT (view), 0);
      g_free (signal_name);

      clay_gradient_box_blend_notify (G_OBJECT (context),
                                      NULL,
                                      CLAY_VIEW (view));
    }

  return hbox;
}

GtkWidget *
clay_gradient_box_new (ClayContainer *container,
                       ClayContext   *context,
                       const gchar   *label,
                       gint           spacing,
                       const gchar   *reverse_prop,
                       const gchar   *blend_color_space_prop)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return gradient_box_new (container, context, label, spacing,
                           CLAY_VIEW_TYPE_LIST, CLAY_VIEW_SIZE_LARGE,
                           reverse_prop, blend_color_space_prop,
                           NULL, NULL);
}

GtkWidget *
clay_prop_gradient_box_new (ClayContainer *container,
                            ClayContext   *context,
                            const gchar   *label,
                            gint           spacing,
                            const gchar   *view_type_prop,
                            const gchar   *view_size_prop,
                            const gchar   *reverse_prop,
                            const gchar   *blend_color_space_prop,
                            const gchar   *editor_id,
                            const gchar   *editor_tooltip)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (gradient_box_new (container, context, label, spacing,
                                               view_type, view_size,
                                               reverse_prop,
                                               blend_color_space_prop,
                                               editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  palette boxes  */

static GtkWidget *
palette_box_new (ClayContainer *container,
                 ClayContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 ClayViewType   view_type,
                 ClayViewSize   view_size,
                 const gchar   *editor_id,
                 const gchar   *editor_tooltip)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->palette_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-palette-list|clay-palette-grid",
                                CLAY_ICON_PALETTE,
                                _("Open the palette selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
clay_palette_box_new (ClayContainer *container,
                      ClayContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return palette_box_new (container, context, label, spacing,
                          CLAY_VIEW_TYPE_LIST, CLAY_VIEW_SIZE_MEDIUM,
                          NULL, NULL);
}

GtkWidget *
clay_prop_palette_box_new (ClayContainer *container,
                           ClayContext   *context,
                           const gchar   *label,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop,
                           const gchar   *editor_id,
                           const gchar   *editor_tooltip)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (palette_box_new (container, context, label, spacing,
                                              view_type, view_size,
                                              editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  font boxes  */

static GtkWidget *
font_box_new (ClayContainer *container,
              ClayContext   *context,
              const gchar   *label,
              gint           spacing,
              ClayViewType   view_type,
              ClayViewSize   view_size)
{
  if (! container)
    container = clay_data_factory_get_container (context->clay->font_factory);

  return clay_viewable_box_new (container, context, label, spacing,
                                view_type, CLAY_VIEW_SIZE_MEDIUM, view_size,
                                "clay-font-list|clay-font-grid",
                                CLAY_ICON_FONT,
                                _("Open the font selection dialog"),
                                NULL, NULL);
}

GtkWidget *
clay_font_box_new (ClayContainer *container,
                   ClayContext   *context,
                   const gchar   *label,
                   gint           spacing)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return font_box_new (container, context, label, spacing,
                       CLAY_VIEW_TYPE_LIST, CLAY_VIEW_SIZE_MEDIUM);
}

GtkWidget *
clay_prop_font_box_new (ClayContainer *container,
                        ClayContext   *context,
                        const gchar   *label,
                        gint           spacing,
                        const gchar   *view_type_prop,
                        const gchar   *view_size_prop)
{
  ClayViewType view_type;
  ClayViewSize view_size;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (font_box_new (container, context, label, spacing,
                                           view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  private functions  */

static GtkWidget *
clay_viewable_box_new (ClayContainer *container,
                       ClayContext   *context,
                       const gchar   *label,
                       gint           spacing,
                       ClayViewType   view_type,
                       ClayViewSize   button_view_size,
                       ClayViewSize   view_size,
                       const gchar   *dialog_identifier,
                       const gchar   *dialog_icon_name,
                       const gchar   *dialog_tooltip,
                       const gchar   *editor_id,
                       const gchar   *editor_tooltip)
{
  GtkWidget *viewable_box;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *l;
  GtkWidget *entry;

  viewable_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, spacing);

  button = clay_viewable_button_new (container, context,
                                     view_type, button_view_size, view_size, 1,
                                     clay_dialog_factory_get_singleton (),
                                     dialog_identifier,
                                     dialog_icon_name,
                                     dialog_tooltip);

  clay_view_renderer_set_size_full (CLAY_VIEW (CLAY_VIEWABLE_BUTTON (button)->view)->renderer,
                                    button_view_size, button_view_size, 1);

  g_object_set_data (G_OBJECT (viewable_box), "viewable-button", button);

  gtk_box_pack_start (GTK_BOX (viewable_box), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (viewable_box), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  if (label)
    {
      l = gtk_label_new_with_mnemonic (label);
      gtk_label_set_xalign (GTK_LABEL (l), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), l, FALSE, FALSE, 0);
      gtk_widget_set_visible (l, TRUE);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  entry = clay_container_entry_new (container, context, view_size, 1);

  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 4);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (entry, TRUE);

  if (editor_id)
    {
      GtkWidget *edit_button;
      GtkWidget *image;

      edit_button = gtk_button_new ();
      gtk_button_set_relief (GTK_BUTTON (edit_button), GTK_RELIEF_NONE);
      gtk_box_pack_start (GTK_BOX (hbox), edit_button, FALSE, FALSE, 0);
      gtk_widget_set_visible (edit_button, TRUE);

      if (editor_tooltip)
        clay_help_set_help_data (edit_button, editor_tooltip, NULL);

      image = gtk_image_new_from_icon_name (CLAY_ICON_EDIT,
                                            GTK_ICON_SIZE_BUTTON);
      gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
      gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
      gtk_container_add (GTK_CONTAINER (edit_button), image);
      gtk_widget_set_visible (image, TRUE);

      g_object_set_data_full (G_OBJECT (button),
                              "clay-viewable-box-editor",
                              g_strdup (editor_id),
                              (GDestroyNotify) g_free);

      g_signal_connect (edit_button, "clicked",
                        G_CALLBACK (clay_viewable_box_edit_clicked),
                        button);
    }

  return viewable_box;
}

static GtkWidget *
view_props_connect (GtkWidget   *box,
                    ClayContext *context,
                    const gchar *view_type_prop,
                    const gchar *view_size_prop)
{
  GtkWidget *button = g_object_get_data (G_OBJECT (box), "viewable-button");

  if (view_type_prop)
    clay_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                              view_type_prop, "popup-view-type");

  if (view_size_prop)
    clay_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                              view_size_prop, "popup-view-size");

  gtk_widget_set_visible (box, TRUE);

  return box;
}

static void
clay_viewable_box_edit_clicked (GtkWidget          *widget,
                                ClayViewableButton *button)
{
  const gchar *editor_id = g_object_get_data (G_OBJECT (button),
                                              "clay-viewable-box-editor");

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (button->context->clay)),
                                             button->context->clay,
                                             clay_dialog_factory_get_singleton (),
                                             clay_widget_get_monitor (widget),
                                             editor_id);
}

static void
clay_gradient_box_reverse_notify (GObject    *object,
                                  GParamSpec *pspec,
                                  ClayView   *view)
{
  ClayViewRendererGradient *rendergrad;
  gboolean                  reverse;

  rendergrad = CLAY_VIEW_RENDERER_GRADIENT (view->renderer);

  g_object_get (object, "gradient-reverse", &reverse, NULL);

  clay_view_renderer_gradient_set_reverse (rendergrad, reverse);
}

static void
clay_gradient_box_blend_notify (GObject    *object,
                                GParamSpec *pspec,
                                ClayView   *view)
{
  ClayViewRendererGradient    *rendergrad;
  ClayGradientBlendColorSpace  blend_color_space;

  rendergrad = CLAY_VIEW_RENDERER_GRADIENT (view->renderer);

  g_object_get (object,
                "gradient-blend-color-space", &blend_color_space,
                NULL);

  clay_view_renderer_gradient_set_blend_color_space (rendergrad,
                                                     blend_color_space);
}
