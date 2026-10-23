/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawabletreeview-filters.c
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

#include "clayhelp-ids.h"
#include "widgets-types.h"
#include "tools/tools-types.h" /* FIXME */

#include "actions/claygeglprocedure.h"
#include "actions/filters-commands.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydrawable.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/claydrawablefilterundo.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claylinklayer.h"
#include "core/claylist.h"
#include "core/clayrasterizable.h"
#include "core/claytoolinfo.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"

#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "tools/claygradienttool.h"
#include "tools/tool_manager.h" /* FIXME */

#include "claycontainerlistview.h"
#include "claycontainerview.h"
#include "claydrawabletreeview.h"
#include "claydrawabletreeview-filters.h"

#include "clay-intl.h"


struct _ClayDrawableTreeViewFiltersEditor
{
  ClayDrawable       *drawable;
  ClayDrawableFilter *filter;

  GtkWidget          *popover;
  GtkWidget          *vbox;
  GtkWidget          *view;
  GtkWidget          *mask_view;

  GtkWidget          *options;
  GtkWidget          *visible_button;
  GtkWidget          *edit_button;
  GtkWidget          *raise_button;
  GtkWidget          *lower_button;
  GtkWidget          *merge_button;
  GtkWidget          *remove_button;

  GtkWidget          *filter_label;
  GtkWidget          *mask_label;

  GQuark              notify_temporary_handler;
  GQuark              notify_temporary_mask_handler;
};

static void   clay_drawable_filters_editor_create_view
                                              (ClayDrawableTreeView  *view,
                                               ClayDrawable          *drawable,
                                               ClayContainer         *filters,
                                               GtkWidget             *filter_view);
static void   clay_drawable_filters_editor_set_sensitive
                                              (ClayDrawableTreeView  *view);

static void   clay_drawable_filters_editor_view_selection_changed
                                              (ClayContainerView     *view,
                                               ClayDrawableTreeView  *drawable_view);
static void   clay_drawable_filters_editor_view_item_activated
                                              (GtkWidget             *widget,
                                               ClayViewable          *viewable,
                                               ClayDrawableTreeView  *view);

static void   clay_drawable_filters_editor_temporary_changed
                                              (ClayFilter            *filter,
                                               const GParamSpec      *pspec,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_filters_changed
                                              (ClayDrawable          *drawable,
                                               ClayDrawableTreeView  *view);

static void   clay_drawable_filters_editor_visible_all_toggled
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_edit_clicked
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_raise_clicked
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_lower_clicked
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_merge_clicked
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
static void   clay_drawable_filters_editor_remove_clicked
                                              (GtkWidget             *widget,
                                               ClayDrawableTreeView  *view);
/* Helper Methods */
static gint   clay_drawable_filters_editor_has_filters
                                              (ClayDrawable          *drawable,
                                               ClayDrawableTreeView  *view);

gboolean
_clay_drawable_tree_view_filter_editor_show (ClayDrawableTreeView *view,
                                             ClayDrawable         *drawable,
                                             GdkRectangle         *rect)
{
  ClayDrawableTreeViewFiltersEditor *editor          = view->editor;
  ClayContainerTreeView             *tree_view       = CLAY_CONTAINER_TREE_VIEW (view);
  ClayContainer                     *filters         = NULL;
  ClayContainer                     *mask_filters    = NULL;
  ClayDrawable                      *mask            = NULL;
  gint                               n_editable      = 0;
  gint                               n_mask_editable = 0;
  gboolean                           visible         = FALSE;
  gboolean                           mask_visible    = FALSE;

  if (drawable && CLAY_IS_LAYER (drawable))
    mask = CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (drawable)));

  /* Prevents "duplicate" filter views if you click the popover too fast */
  if (editor && (editor->view || editor->mask_view))
    _clay_drawable_tree_view_filter_editor_hide (view);

  if (! editor)
    {
      GtkWidget   *vbox;
      GtkWidget   *image;
      GtkWidget   *label;
      gchar       *text;
      GtkIconSize  button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
      gint         pixel_icon_size  = 16;
      gint         button_spacing;

      view->editor = editor = g_new0 (ClayDrawableTreeViewFiltersEditor, 1);

      gtk_widget_style_get (GTK_WIDGET (view),
                            "button-icon-size", &button_icon_size,
                            "button-spacing",   &button_spacing,
                            NULL);
      gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

      /*  filters popover  */
      editor->popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
      gtk_popover_set_modal (GTK_POPOVER (editor->popover), TRUE);

      /*  main vbox  */
      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      gtk_container_add (GTK_CONTAINER (editor->popover), vbox);
      gtk_widget_set_visible (vbox, TRUE);

      clay_help_connect (vbox, NULL, clay_standard_help_func,
                         CLAY_HELP_LAYER_EFFECTS, NULL, NULL);

      /*  top label  */
      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("Filters"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      /*  main vbox within  */
      editor->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      gtk_box_pack_end (GTK_BOX (vbox), editor->vbox, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->vbox, TRUE);

      /*  bottom buttons  */
      editor->options = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
      gtk_box_pack_end (GTK_BOX (editor->vbox), editor->options, FALSE, FALSE, 0);
      gtk_widget_set_visible (editor->options, TRUE);

      /*  "visible" button  */
      editor->visible_button = gtk_toggle_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->visible_button),
                                    TRUE);
      image = gtk_image_new_from_icon_name (CLAY_ICON_VISIBLE,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (editor->visible_button), image);
      gtk_widget_set_visible (image, TRUE);

      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->visible_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->visible_button, TRUE);

      clay_help_set_help_data (editor->visible_button,
                               _("Toggle the visibility of all filters."),
                               CLAY_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->visible_button, "toggled",
                        G_CALLBACK (clay_drawable_filters_editor_visible_all_toggled),
                        view);

      /*  "edit" button  */
      editor->edit_button =
        gtk_button_new_from_icon_name (CLAY_ICON_EDIT,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->edit_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->edit_button, TRUE);

      clay_help_set_help_data (editor->edit_button,
                               _("Edit the selected filter."),
                               CLAY_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->edit_button, "clicked",
                        G_CALLBACK (clay_drawable_filters_editor_edit_clicked),
                        view);

      /*  "raise" button  */
      editor->raise_button =
        gtk_button_new_from_icon_name (CLAY_ICON_GO_UP,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->raise_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->raise_button, TRUE);

      clay_help_set_help_data (editor->raise_button,
                               _("Raise filter one step up in the stack."),
                               CLAY_HELP_LAYER_EFFECTS);
      g_signal_connect (editor->raise_button, "clicked",
                        G_CALLBACK (clay_drawable_filters_editor_raise_clicked),
                        view);

      /*  "lower" button  */
      editor->lower_button =
        gtk_button_new_from_icon_name (CLAY_ICON_GO_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->lower_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->lower_button, TRUE);

      clay_help_set_help_data (editor->lower_button,
                               _("Lower filter one step down in the stack."),
                               CLAY_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->lower_button, "clicked",
                        G_CALLBACK (clay_drawable_filters_editor_lower_clicked),
                        view);

      /*  "merge" button  */
      editor->merge_button =
        gtk_button_new_from_icon_name (CLAY_ICON_LAYER_MERGE_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->merge_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->merge_button, TRUE);

      clay_help_set_help_data (editor->merge_button,
                               _("Merge all active filters down."),
                               CLAY_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->merge_button, "clicked",
                        G_CALLBACK (clay_drawable_filters_editor_merge_clicked),
                        view);

      /*  "remove" button  */
      editor->remove_button =
        gtk_button_new_from_icon_name (CLAY_ICON_EDIT_DELETE,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->remove_button, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->remove_button, TRUE);

      clay_help_set_help_data (editor->remove_button,
                               _("Remove the selected filter."),
                               CLAY_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->remove_button, "clicked",
                        G_CALLBACK (clay_drawable_filters_editor_remove_clicked),
                        view);

      editor->view      = NULL;
      editor->mask_view = NULL;
    }

  filters = clay_drawable_get_filters (drawable);
  visible = clay_drawable_has_visible_filters (drawable);

  if (mask)
    {
      mask_filters = clay_drawable_get_filters (mask);
      mask_visible = clay_drawable_has_visible_filters (mask);
    }

  /* Set the initial value for the effect visibility toggle */
  g_signal_handlers_block_by_func (editor->visible_button,
                                   clay_drawable_filters_editor_visible_all_toggled,
                                   view);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->visible_button),
                                visible || mask_visible);
  g_signal_handlers_unblock_by_func (editor->visible_button,
                                     clay_drawable_filters_editor_visible_all_toggled,
                                     view);

  clay_drawable_n_editable_filters (drawable, &n_editable, NULL, NULL);
  if (mask)
    clay_drawable_n_editable_filters (mask, &n_mask_editable, NULL, NULL);

  /*  only show if we have at least one editable filter  */
  if (n_editable > 0 || n_mask_editable > 0)
    {
      editor->drawable = drawable;

      editor->view =
        clay_container_list_view_new (filters,
                                      clay_container_view_get_context (CLAY_CONTAINER_VIEW (view)),
                                      CLAY_VIEW_SIZE_SMALL, 0);

      /* Create filter views */
      if (n_editable > 0)
        {
          if (CLAY_IS_LAYER (drawable))
            editor->filter_label = gtk_label_new ("Layer");
          else
            editor->filter_label = gtk_label_new ("Channel");
          gtk_box_pack_start (GTK_BOX (editor->vbox), editor->filter_label,
                                       FALSE, FALSE, 0);
          gtk_widget_set_visible (editor->filter_label, TRUE);

          clay_drawable_filters_editor_create_view (view, drawable, filters,
                                                    editor->view);
        }

      if (n_mask_editable > 0)
        {
          editor->mask_label = gtk_label_new ("Layer Mask");
          gtk_box_pack_start (GTK_BOX (editor->vbox), editor->mask_label,
                              FALSE, FALSE, 0);
          gtk_widget_set_visible (editor->mask_label, TRUE);

          editor->mask_view =
            clay_container_list_view_new (mask_filters,
                                          clay_container_view_get_context (CLAY_CONTAINER_VIEW (view)),
                                          CLAY_VIEW_SIZE_SMALL, 0);

          clay_drawable_filters_editor_create_view (view, mask, mask_filters,
                                                    editor->mask_view);
        }

      clay_drawable_filters_editor_set_sensitive (view);

      gtk_popover_set_pointing_to (GTK_POPOVER (editor->popover), rect);
      gtk_popover_popup (GTK_POPOVER (editor->popover));

      g_signal_connect_swapped (editor->popover, "closed",
                                G_CALLBACK (_clay_drawable_tree_view_filter_editor_hide),
                                view);

      return TRUE;
    }

  return FALSE;
}

void
_clay_drawable_tree_view_filter_editor_hide (ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;

  if (! editor)
    return;

  if (editor->notify_temporary_handler)
    {
      ClayContainer *filters = clay_drawable_get_filters (editor->drawable);

      g_signal_handlers_disconnect_by_func (editor->popover,
                                            _clay_drawable_tree_view_filter_editor_hide,
                                            view);

      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            clay_drawable_filters_editor_filters_changed,
                                            view);

      clay_container_remove_handler (filters,
                                     editor->notify_temporary_handler);
      editor->notify_temporary_handler = 0;
    }

  if (editor->notify_temporary_mask_handler)
    {
      ClayDrawable  *mask;
      ClayContainer *filters;

      mask = CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (editor->drawable)));

      filters = clay_drawable_get_filters (mask);

      g_signal_handlers_disconnect_by_func (mask,
                                            clay_drawable_filters_editor_filters_changed,
                                            view);

      clay_container_remove_handler (filters,
                                     editor->notify_temporary_mask_handler);
      editor->notify_temporary_mask_handler = 0;
    }

  gtk_popover_popdown (GTK_POPOVER (editor->popover));

  if (editor->view)
    {
      g_signal_handlers_disconnect_by_func (editor->view,
                                            clay_drawable_filters_editor_view_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (editor->view,
                                            clay_drawable_filters_editor_view_item_activated,
                                            view);

      g_clear_pointer (&editor->view, gtk_widget_destroy);
    }

  if (editor->mask_view)
    {
      g_signal_handlers_disconnect_by_func (editor->mask_view,
                                            clay_drawable_filters_editor_view_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (editor->mask_view,
                                            clay_drawable_filters_editor_view_item_activated,
                                            view);

      g_clear_pointer (&editor->mask_view, gtk_widget_destroy);
    }

  g_clear_pointer (&editor->filter_label, gtk_widget_destroy);
  if (editor->mask_label)
    g_clear_pointer (&editor->mask_label, gtk_widget_destroy);

  editor->drawable = NULL;
  editor->filter   = NULL;
}

void
_clay_drawable_tree_view_filter_editor_destroy (ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;

  if (! editor)
    return;

  _clay_drawable_tree_view_filter_editor_hide (view);

  g_clear_pointer (&editor->popover, gtk_widget_destroy);
  g_clear_pointer (&view->editor, g_free);
}

static void
clay_drawable_filters_editor_create_view (ClayDrawableTreeView *view,
                                          ClayDrawable         *drawable,
                                          ClayContainer        *filters,
                                          GtkWidget            *filter_view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  GtkWidget                         *scrolled_window;
  gboolean                           is_mask;

  is_mask = CLAY_IS_LAYER_MASK (drawable);

  if (! is_mask)
    editor->notify_temporary_handler =
      clay_container_add_handler (filters, "notify::temporary",
                                  G_CALLBACK (clay_drawable_filters_editor_temporary_changed),
                                  view);
  else
    editor->notify_temporary_mask_handler =
      clay_container_add_handler (filters, "notify::temporary",
                                  G_CALLBACK (clay_drawable_filters_editor_temporary_changed),
                                  view);

  g_signal_connect (drawable, "filters-changed",
                    G_CALLBACK (clay_drawable_filters_editor_filters_changed),
                    view);

  g_signal_connect (filter_view, "selection-changed",
                    G_CALLBACK (clay_drawable_filters_editor_view_selection_changed),
                    view);
  g_signal_connect_object (filter_view, "item-activated",
                           G_CALLBACK (clay_drawable_filters_editor_view_item_activated),
                           view, 0);

  gtk_box_pack_start (GTK_BOX (editor->vbox), filter_view,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (filter_view, TRUE);

  scrolled_window = CLAY_CONTAINER_BOX (filter_view)->scrolled_win;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
}

static void
clay_drawable_filters_editor_set_sensitive (ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayDrawable                      *drawable;
  gboolean                           editable;
  gint                               n_editable;
  gint                               first_editable;
  gint                               last_editable;

  drawable = editor->drawable;
  if (editor->filter &&
      CLAY_IS_DRAWABLE_FILTER (editor->filter))
    drawable = clay_drawable_filter_get_drawable (editor->filter);

  editable = clay_drawable_n_editable_filters (drawable,
                                               &n_editable,
                                               &first_editable,
                                               &last_editable);

  /*  lock everything if none is editable  */
  gtk_widget_set_sensitive (editor->vbox, editable);
  if (! editable)
    return;

  gtk_widget_set_sensitive (editor->options,
                            CLAY_IS_DRAWABLE_FILTER (editor->filter));

  if (CLAY_IS_DRAWABLE_FILTER (editor->filter))
    {
      ClayContainer *filters;
      gboolean       is_group    = FALSE;
      gboolean       is_editable = FALSE;
      gboolean       is_temporary;
      gboolean       has_visible_filters;
      gint           index;

      filters = clay_drawable_get_filters (drawable);

      index = clay_container_get_child_index (filters,
                                              CLAY_OBJECT (editor->filter));
      is_temporary = (clay_drawable_filter_get_temporary (editor->filter) ||
                      ! clay_drawable_filter_get_mask (editor->filter));

      /*  do not allow merging down effects on group layers  */
      if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
        is_group = TRUE;

      has_visible_filters =
        clay_drawable_has_visible_filters (drawable);

      is_editable = (index >= first_editable &&
                     index <= last_editable);

      g_signal_handlers_block_by_func (editor->visible_button,
                                       clay_drawable_filters_editor_visible_all_toggled,
                                       view);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->visible_button),
                                    has_visible_filters);
      g_signal_handlers_unblock_by_func (editor->visible_button,
                                         clay_drawable_filters_editor_visible_all_toggled,
                                         view);

      gtk_widget_set_sensitive (editor->visible_button,
                                TRUE);
      gtk_widget_set_sensitive (editor->edit_button,
                                is_editable && ! is_temporary);
      gtk_widget_set_sensitive (editor->raise_button,
                                index > first_editable);
      gtk_widget_set_sensitive (editor->lower_button,
                                index < last_editable);
      gtk_widget_set_sensitive (editor->merge_button,
                                ! is_group          &&
                                has_visible_filters &&
                                ! is_temporary      &&
                                (! CLAY_IS_RASTERIZABLE (drawable) ||
                                 clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (drawable))));
      gtk_widget_set_sensitive (editor->remove_button,
                                is_editable && ! is_temporary);

      if (is_group                                 ||
          (CLAY_IS_RASTERIZABLE (drawable) &&
          ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (drawable))))
        {
          gchar       *disabled_reason;
          const gchar *default_name;
          gchar       *tooltip;

          default_name =
            CLAY_VIEWABLE_GET_CLASS (CLAY_VIEWABLE (drawable))->default_name;

          if (is_group)
            disabled_reason = g_strdup (_("Disabled because filters cannot be "
                                          "merged on group layers."));
          else
            /* TRANSLATORS: The %s will be replaced by the name of the
             * layer type. */
            disabled_reason = g_strdup_printf (_("Disabled because filters "
                                                 "cannot be merged on: %s."),
                                               default_name);

          tooltip = g_strdup_printf ("%s\n<i>%s</i>",
                                     _("Merge all active filters down."),
                                     disabled_reason);
          g_free (disabled_reason);

          clay_help_set_help_data_with_markup (editor->merge_button, tooltip,
                                               CLAY_HELP_LAYER_EFFECTS);
          g_free (tooltip);
        }
      else
        {
          clay_help_set_help_data (editor->merge_button,
                                   _("Merge all active filters down."),
                                   CLAY_HELP_LAYER_EFFECTS);
        }
    }
}

static void
clay_drawable_filters_editor_view_selection_changed (ClayContainerView    *view,
                                                     ClayDrawableTreeView *drawable_view)
{
  ClayDrawableTreeViewFiltersEditor *editor = drawable_view->editor;
  ClayViewable                      *viewable;

  viewable = clay_container_view_get_1_selected (view);

  editor->filter = NULL;

  if (viewable)
    {
      /* Don't set floating selection as active filter */
      if (CLAY_IS_DRAWABLE_FILTER (viewable))
        editor->filter = CLAY_DRAWABLE_FILTER (viewable);
    }

  clay_drawable_filters_editor_set_sensitive (drawable_view);
}

static void
clay_drawable_filters_editor_view_item_activated (GtkWidget            *widget,
                                                  ClayViewable         *viewable,
                                                  ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;

  if (gtk_widget_is_sensitive (editor->edit_button))
    clay_drawable_filters_editor_edit_clicked (widget, view);
}

static void
clay_drawable_filters_editor_temporary_changed (ClayFilter           *filter,
                                                const GParamSpec     *pspec,
                                                ClayDrawableTreeView *view)
{
  clay_drawable_filters_editor_set_sensitive (view);
}

static void
clay_drawable_filters_editor_filters_changed (ClayDrawable         *drawable,
                                              ClayDrawableTreeView *view)
{
  ClayContainer *filters;
  GtkWidget     *filter_view;
  gint           n_filters;
  gint           height;

  if (! CLAY_IS_LAYER_MASK (drawable))
    filter_view = view->editor->view;
  else
    filter_view = view->editor->mask_view;

  filters   = clay_drawable_get_filters (drawable);
  n_filters = clay_container_get_n_children (filters);

  height =
    clay_container_view_get_view_size (CLAY_CONTAINER_VIEW (filter_view),
                                       NULL);

  /*  it doesn't get any more hackish  */
  gtk_widget_set_size_request (filter_view, -1, (height + 6) * n_filters);

  clay_drawable_filters_editor_set_sensitive (view);
}

static void
clay_drawable_filters_editor_visible_all_toggled (GtkWidget            *widget,
                                                  ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayImage                         *image;
  ClayContainer                     *filters;
  ClayDrawable                      *drawable;
  GList                             *list;
  GList                             *iter;
  gint                               n_filters = 0;
  gboolean                           visible;
  ClayUndo                          *undo;

  visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  image   = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

  if (editor->filter)
    drawable = clay_drawable_filter_get_drawable (editor->filter);
  else
    drawable = editor->drawable;

  filters = clay_drawable_get_filters (drawable);
  list    = CLAY_LIST (filters)->queue->head;

  undo = clay_drawable_filter_undo_can_compress_visibility (image, list);

  for (iter = list;
       iter;
       iter = g_list_next (iter))
    {
      if (CLAY_IS_DRAWABLE_FILTER (iter->data) &&
          visible != clay_filter_get_active (CLAY_FILTER (iter->data)))
        {
          n_filters++;
        }
    }

  if (n_filters <= 0)
    return;

  if (undo == NULL)
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_FILTER_VISIBILITY,
                                   NULL);
    }

  for (iter = list;
       iter;
       iter = g_list_next (iter))
    {
      if (CLAY_IS_DRAWABLE_FILTER (iter->data))
        {
          ClayFilter         *filter  = iter->data;
          ClayDrawableFilter *dfilter = iter->data;

          /**
           * Undos are pushed for every filters, even if they weren't
           * actually toggled, so that the undo group can be compressed with
           * subsequent toggles.
           */
          if (undo == NULL)
            {
              clay_image_undo_push_filter_visibility (image,
                                                      _("Filter visibility"),
                                                      editor->drawable,
                                                      dfilter);
            }

          clay_filter_set_active (filter, visible);
        }
    }

  if (undo == NULL)
    {
      clay_image_undo_group_end (image);
    }
  else
    {
      ClayContext *context;

      context = clay_container_view_get_context (CLAY_CONTAINER_VIEW (view));
      clay_undo_refresh_preview (undo, context);
    }

  clay_drawable_update (drawable, 0, 0, -1, -1);
  clay_image_flush (clay_item_get_image (CLAY_ITEM (drawable)));
}

static void
clay_drawable_filters_editor_edit_clicked (GtkWidget            *widget,
                                           ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayImage                         *image;
  ClayDrawable                      *drawable;
  GeglNode                          *op;

  if (! CLAY_IS_DRAWABLE_FILTER (editor->filter))
    return;

  drawable = clay_drawable_filter_get_drawable (editor->filter);

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! CLAY_GUI_CONFIG (image->clay->config)->edit_non_visible)
    {
      clay_message_literal (image->clay, G_OBJECT (view),
                            CLAY_MESSAGE_ERROR,
                            _("A selected layer is not visible."));
      return;
    }
  else if (clay_item_get_lock_content (CLAY_ITEM (drawable)))
    {
      clay_message_literal (image->clay, G_OBJECT (view),
                            CLAY_MESSAGE_WARNING,
                            _("A selected layer's pixels are locked."));
      return;
    }

  op = clay_drawable_filter_get_operation (editor->filter);
  if (op)
    {
      ClayProcedure *procedure;
      GVariant      *variant;
      gchar         *operation;
      gchar         *name;

      g_object_get (editor->filter,
                    "name", &name,
                    NULL);
      g_object_get (op,
                    "operation", &operation,
                    NULL);

      if (operation)
        {
          if (! strcmp (operation, "clay:gradient"))
            {
              ClayTool      *active_tool = tool_manager_get_active (image->clay);
              ClayContext   *context     = clay_get_user_context (image->clay);
              ClayContainer *filters;
              gint           drawable_id;
              gint           filter_index;

              /* Since switching the active tool hides the popover, we need to
               * get and store the filter index first, then pass it to the tool
               * so it can retrieve the filter for editing */
              drawable_id  = clay_item_get_id (CLAY_ITEM (drawable));
              filters      = clay_drawable_get_filters (drawable);
              filter_index =
                clay_container_get_child_index (filters,
                                                CLAY_OBJECT (editor->filter));

              if (! CLAY_IS_GRADIENT_TOOL (active_tool))
                {
                  ClayToolInfo *tool_info =
                    clay_get_tool_info (image->clay, "clay-gradient-tool");

                  if (CLAY_IS_TOOL_INFO (tool_info))
                    {
                      clay_context_set_tool (context, tool_info);
                      active_tool = tool_manager_get_active (image->clay);
                    }
                }

                if (filter_index >= 0)
                  {
                    clay_gradient_tool_set_existing_filter (CLAY_GRADIENT_TOOL (active_tool),
                                                            clay_context_get_display (context),
                                                            drawable_id, filter_index);
                    _clay_drawable_tree_view_filter_editor_hide (view);
                  }
            }
          else
            {
              procedure = clay_gegl_procedure_new (image->clay,
                                                   editor->filter,
                                                   CLAY_RUN_INTERACTIVE, NULL,
                                                   operation,
                                                   name,
                                                   name,
                                                   NULL, NULL, NULL);

              variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
              g_variant_take_ref (variant);
              filters_run_procedure (image->clay,
                                     clay_context_get_display (clay_get_user_context (image->clay)),
                                     procedure, CLAY_RUN_INTERACTIVE);

              g_variant_unref (variant);
              g_object_unref (procedure);
            }
        }

      g_free (name);
      g_free (operation);
    }
}

static void
clay_drawable_filters_editor_raise_clicked (GtkWidget            *widget,
                                            ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayImage                         *image;
  ClayDrawable                      *drawable;

  if (! CLAY_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image    = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));
  drawable = clay_drawable_filter_get_drawable (editor->filter);

  if (clay_drawable_filter_get_mask (editor->filter) == NULL)
    {
      clay_message_literal (image->clay, G_OBJECT (view), CLAY_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (clay_drawable_raise_filter (drawable,
                                  CLAY_FILTER (editor->filter)))
    {
      clay_image_flush (image);
    }
}

static void
clay_drawable_filters_editor_lower_clicked (GtkWidget            *widget,
                                            ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayImage                         *image;
  ClayDrawable                      *drawable;

  if (! CLAY_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image    = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));
  drawable = clay_drawable_filter_get_drawable (editor->filter);

  if (clay_drawable_filter_get_mask (editor->filter) == NULL)
    {
      clay_message_literal (image->clay, G_OBJECT (view), CLAY_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (clay_drawable_lower_filter (drawable,
                                  CLAY_FILTER (editor->filter)))
    {
      clay_image_flush (image);
    }
}

static void
clay_drawable_filters_editor_merge_clicked (GtkWidget            *widget,
                                            ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayContext                       *context;
  ClayToolInfo                      *active_tool;
  ClayDrawable                      *drawable;

  if (! CLAY_IS_DRAWABLE_FILTER (editor->filter))
    return;

  drawable = clay_drawable_filter_get_drawable (editor->filter);

  /* Commit GEGL-based tools before trying to merge filters */
  context     = clay_container_view_get_context (CLAY_CONTAINER_VIEW (view));
  active_tool = clay_context_get_tool (context);

  if (! strcmp (clay_object_get_name (active_tool), "clay-cage-tool")     ||
      ! strcmp (clay_object_get_name (active_tool), "clay-gradient-tool") ||
      ! strcmp (clay_object_get_name (active_tool), "clay-warp-tool"))
    {
      tool_manager_control_active (context->clay, CLAY_TOOL_ACTION_COMMIT,
                                   clay_context_get_display (context));
    }

  if (! clay_viewable_get_children (CLAY_VIEWABLE (drawable)) &&
      clay_drawable_has_visible_filters (drawable))
    {
      ClayImage *image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

      /* Don't merge if the layer is currently locked */
      if (clay_item_get_lock_content (CLAY_ITEM (editor->drawable)))
        {
          clay_message_literal (image->clay, G_OBJECT (view),
                                CLAY_MESSAGE_WARNING,
                                _("The layer to merge down to is locked."));
          return;
        }

      clay_drawable_merge_filters (CLAY_DRAWABLE (drawable));

      /* If we still have inactive filters, keep pop-up open */
      if (clay_drawable_filters_editor_has_filters (drawable, view) == 0)
        _clay_drawable_tree_view_filter_editor_hide (view);

      clay_image_flush (image);
    }
}

static void
clay_drawable_filters_editor_remove_clicked (GtkWidget            *widget,
                                             ClayDrawableTreeView *view)
{
  ClayDrawableTreeViewFiltersEditor *editor = view->editor;
  ClayImage                         *image;

  if (! CLAY_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

  clay_image_undo_push_filter_remove (image, _("Remove filter"),
                                      editor->drawable,
                                      editor->filter);

  clay_drawable_filter_abort (editor->filter);

  /*  close the popover if all effects are deleted  */
  if (clay_drawable_filters_editor_has_filters (editor->drawable, view) == 0)
    _clay_drawable_tree_view_filter_editor_hide (view);

  clay_image_flush (image);
}

static gint
clay_drawable_filters_editor_has_filters (ClayDrawable          *drawable,
                                          ClayDrawableTreeView  *view)
{
  ClayContainer *filters;
  gint           number_of_filters = 0;

  filters = clay_drawable_get_filters (drawable);
  number_of_filters += clay_container_get_n_children (filters);

  if (CLAY_IS_LAYER (drawable))
    {
      ClayDrawable *mask = NULL;

      mask = CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (drawable)));
      if (mask)
        {
          filters = clay_drawable_get_filters (mask);
          number_of_filters += clay_container_get_n_children (filters);
        }
    }

  return number_of_filters;
}
