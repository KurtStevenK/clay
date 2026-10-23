/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemtreeview-search.c
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

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/clayitemlist.h"

#include "clayitemtreeview.h"
#include "clayitemtreeview-search.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void     clay_item_search_style_updated       (ClayItemTreeView *view);
static gboolean clay_item_search_key_release         (GtkWidget        *widget,
                                                      GdkEventKey      *event,
                                                      ClayItemTreeView *view);
static void     clay_item_search_link_entry_activate (ClayItemTreeView *view);
static gboolean clay_item_search_new_link_clicked    (ClayItemTreeView *view);
static gboolean clay_item_search_link_clicked        (GtkWidget        *widget,
                                                      GdkEvent         *event,
                                                      ClayItemTreeView *view);
static gboolean clay_item_search_unlink_clicked      (GtkWidget        *widget,
                                                      GdkEvent         *event,
                                                      ClayItemTreeView *view);


struct _ClayItemTreeViewSearch
{
  GtkWidget    *popover;

  GtkWidget    *search_entry;

  GtkWidget    *link_list;
  GtkWidget    *link_entry;
  GtkWidget    *link_button;

  ClayItemList *link_pattern_set;
};


void
_clay_item_tree_view_search_create (ClayItemTreeView *view,
                                    GtkWidget        *parent)
{
  ClayItemTreeViewSearch *search = view->search;
  PangoAttrList          *attrs;
  GtkWidget              *grid;
  GtkWidget              *placeholder;
  GtkIconSize             button_size = GTK_ICON_SIZE_SMALL_TOOLBAR;

  if (search)
    return;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &button_size,
                        NULL);

  view->search = search = g_new0 (ClayItemTreeViewSearch, 1);

  search->popover = gtk_popover_new (parent);
  gtk_popover_set_modal (GTK_POPOVER (search->popover), TRUE);

  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (search->popover), grid);
  gtk_widget_set_visible (grid, TRUE);

  search->search_entry = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (search->search_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "system-search");
  gtk_grid_attach (GTK_GRID (grid), search->search_entry,
                   0, 0, 2, 1);
  gtk_widget_set_visible (search->search_entry, TRUE);

  g_signal_connect (search->search_entry, "key-release-event",
                    G_CALLBACK (clay_item_search_key_release),
                    view);

  search->link_list = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (search->link_list),
                                             TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (search->link_list)),
                               "view");
  gtk_grid_attach (GTK_GRID (grid), search->link_list,
                   0, 1, 2, 1);
  gtk_widget_set_visible (search->link_list, TRUE);

  placeholder = gtk_label_new (_("No layer set stored"));
  attrs = pango_attr_list_new ();
  gtk_label_set_attributes (GTK_LABEL (placeholder), attrs);
  pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC));
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_ULTRALIGHT));
  pango_attr_list_unref (attrs);
  gtk_list_box_set_placeholder (GTK_LIST_BOX (search->link_list),
                                placeholder);
  gtk_widget_set_visible (placeholder, TRUE);

  search->link_entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (search->link_entry),
                                  _("New layer set's name"));
  gtk_grid_attach (GTK_GRID (grid), search->link_entry,
                   0, 2, 1, 1);
  gtk_widget_set_visible (search->link_entry, TRUE);

  g_signal_connect_swapped (search->link_entry, "activate",
                            G_CALLBACK (clay_item_search_link_entry_activate),
                            view);

  search->link_button = gtk_button_new_from_icon_name (CLAY_ICON_LIST_ADD,
                                                       button_size);
  gtk_grid_attach (GTK_GRID (grid), search->link_button,
                   1, 2, 1, 1);
  gtk_widget_set_visible (search->link_button, TRUE);

  g_signal_connect_swapped (search->link_button, "style-updated",
                            G_CALLBACK (clay_item_search_style_updated),
                            view);
  g_signal_connect_swapped (search->link_button, "clicked",
                            G_CALLBACK (clay_item_search_new_link_clicked),
                            view);
}

void
_clay_item_tree_view_search_destroy (ClayItemTreeView *view)
{
  ClayItemTreeViewSearch *search = view->search;

  if (! search)
    return;

  _clay_item_tree_view_search_hide (view);

  g_clear_pointer (&search->popover, gtk_widget_destroy);
  g_clear_pointer (&view->search, g_free);
}

void
_clay_item_tree_view_search_show (ClayItemTreeView *view)
{
  ClayItemTreeViewSearch *search = view->search;
  ClayImage              *image;
  ClaySelectMethod        pattern_syntax;
  const gchar            *tooltip     = NULL;
  const gchar            *placeholder = NULL;

  if (! search)
    return;

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

  if (! image)
    return;

  g_object_get (image->clay->config,
                "items-select-method", &pattern_syntax,
                NULL);

  switch (pattern_syntax)
    {
    case CLAY_SELECT_PLAIN_TEXT:
      tooltip     = _("Select layers by text search");
      placeholder = _("Text search");
      break;

    case CLAY_SELECT_GLOB_PATTERN:
      tooltip     = _("Select layers by glob patterns");
      placeholder = _("Glob pattern search");
      break;

    case CLAY_SELECT_REGEX_PATTERN:
      tooltip     = _("Select layers by regular expressions");
      placeholder = _("Regular Expression search");
      break;
    }

  gtk_widget_set_tooltip_text (search->search_entry, tooltip);
  gtk_entry_set_placeholder_text (GTK_ENTRY (search->search_entry),
                                  placeholder);

  gtk_popover_popup (GTK_POPOVER (search->popover));
  gtk_widget_grab_focus (search->search_entry);
}

void
_clay_item_tree_view_search_hide (ClayItemTreeView *view)
{
  ClayItemTreeViewSearch *search = view->search;

  if (! search)
    return;

  gtk_popover_popdown (GTK_POPOVER (search->popover));
}

void
_clay_item_tree_view_search_update_links (ClayItemTreeView *view,
                                          ClayImage        *image,
                                          GType             item_type)
{
  ClayItemTreeViewSearch *search = view->search;
  ClayItemTreeViewClass  *item_view_class;
  GtkSizeGroup           *label_size;
  GList                  *iter;

  if (! search)
    return;

  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  if (item_type != item_view_class->item_type)
    return;

  gtk_container_foreach (GTK_CONTAINER (search->link_list),
                         (GtkCallback) gtk_widget_destroy, NULL);

  if (! image)
    {
      _clay_item_tree_view_search_hide (view);
      return;
    }

  label_size = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  for (iter = clay_image_get_stored_item_sets (image, item_type);
       iter;
       iter = iter->next)
    {
      GtkWidget        *grid;
      GtkWidget        *label;
      GtkWidget        *event_box;
      GtkWidget        *icon;
      ClaySelectMethod  method;

      grid = gtk_grid_new ();

      label = gtk_label_new (clay_object_get_name (iter->data));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      if (clay_item_list_is_pattern (iter->data, &method))
        {
          gchar         *display_name;
          PangoAttrList *attrs;

          display_name =
            g_strdup_printf ("<small>[%s]</small> %s",
                             method == CLAY_SELECT_PLAIN_TEXT ? _("search") :
                             (method == CLAY_SELECT_GLOB_PATTERN ? _("glob") : _("regexp")),
                             clay_object_get_name (iter->data));
          gtk_label_set_markup (GTK_LABEL (label), display_name);
          g_free (display_name);

          attrs = pango_attr_list_new ();
          pango_attr_list_insert (attrs,
                                  pango_attr_style_new (PANGO_STYLE_OBLIQUE));
          gtk_label_set_attributes (GTK_LABEL (label), attrs);
          pango_attr_list_unref (attrs);
        }
      gtk_widget_set_hexpand (GTK_WIDGET (label), TRUE);
      gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
      gtk_size_group_add_widget (label_size, label);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
      gtk_widget_set_visible (label, TRUE);

      /* I don't use a GtkButton because the minimum size is 16 which is
       * weird and ugly here. And somehow if I force smaller GtkImage
       * size then add it to the GtkButton, I still get a giant button
       * with a small image in it, which is even worse. XXX
       */
      event_box = gtk_event_box_new ();
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), TRUE);
      gtk_widget_add_events (event_box, GDK_BUTTON_RELEASE_MASK);
      g_object_set_data (G_OBJECT (event_box), "link-set", iter->data);
      gtk_grid_attach (GTK_GRID (grid), event_box, 2, 0, 1, 1);
      gtk_widget_set_visible (event_box, TRUE);

      g_signal_connect (event_box, "button-release-event",
                        G_CALLBACK (clay_item_search_unlink_clicked),
                        view);

      icon = gtk_image_new_from_icon_name (CLAY_ICON_EDIT_DELETE,
                                           GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (icon), 10);
      gtk_container_add (GTK_CONTAINER (event_box), icon);
      gtk_widget_set_visible (icon, TRUE);

      /* Now using again an event box on the whole grid, but behind its
       * child (so that the delete button is processed first. I do it
       * this way instead of using the "row-activated" of GtkListBox
       * because this signal does not give us event info, and in
       * particular modifier state. Yet I want to be able to process
       * Shift/Ctrl state for logical operations on layer sets.
       */
      event_box = gtk_event_box_new ();
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), FALSE);
      gtk_widget_add_events (event_box, GDK_BUTTON_RELEASE_MASK);
      g_object_set_data (G_OBJECT (event_box), "link-set", iter->data);
      gtk_container_add (GTK_CONTAINER (event_box), grid);
      gtk_list_box_prepend (GTK_LIST_BOX (search->link_list), event_box);
      gtk_widget_set_visible (event_box, TRUE);

      g_signal_connect (event_box, "button-release-event",
                        G_CALLBACK (clay_item_search_link_clicked),
                        view);

      gtk_widget_set_visible (grid, TRUE);
    }

  g_object_unref (label_size);
  gtk_list_box_unselect_all (GTK_LIST_BOX (search->link_list));
}

static void
clay_item_search_style_updated (ClayItemTreeView *view)
{
  ClayItemTreeViewSearch *search           = view->search;
  GtkWidget              *image;
  GtkIconSize             button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  GtkIconSize             old_size;
  const gchar            *old_icon_name;

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &button_icon_size,
                        NULL);

  image = gtk_button_get_image (GTK_BUTTON (search->link_button));

  gtk_image_get_icon_name (GTK_IMAGE (image),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      gchar *icon_name;

      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (image), icon_name,
                                    button_icon_size);
      g_free (icon_name);
    }
}

static gboolean
clay_item_search_key_release (GtkWidget        *widget,
                              GdkEventKey      *event,
                              ClayItemTreeView *view)
{
  ClayItemTreeViewClass  *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  ClayItemTreeViewSearch *search          = view->search;
  ClayImage              *image;
  const gchar            *pattern;
  ClaySelectMethod        pattern_syntax;

  if (event->keyval == GDK_KEY_Escape)
    {
      _clay_item_tree_view_search_hide (view);

      return TRUE;
    }

  if (event->keyval == GDK_KEY_Return   ||
      event->keyval == GDK_KEY_KP_Enter ||
      event->keyval == GDK_KEY_ISO_Enter)
    {
      if (event->state & GDK_SHIFT_MASK)
        {
          if (clay_item_search_new_link_clicked (view))
            _clay_item_tree_view_search_hide (view);
        }
      else
        {
          _clay_item_tree_view_search_hide (view);
        }

      return TRUE;
    }

  gtk_entry_set_attributes (GTK_ENTRY (search->search_entry), NULL);

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));
  g_clear_object (&search->link_pattern_set);

  if (! image)
    return TRUE;

  g_object_get (image->clay->config,
                "items-select-method", &pattern_syntax,
                NULL);

  pattern = gtk_entry_get_text (GTK_ENTRY (search->search_entry));
  if (pattern && strlen (pattern) > 0)
    {
      GList  *items;
      GError *error = NULL;

      gtk_entry_set_text (GTK_ENTRY (search->link_entry), "");
      gtk_widget_set_sensitive (search->link_entry, FALSE);

      search->link_pattern_set = clay_item_list_pattern_new (image,
                                                             item_view_class->item_type,
                                                             pattern_syntax,
                                                             pattern);

      items = clay_item_list_get_items (search->link_pattern_set, &error);
      if (error)
        {
          /* Invalid regular expression. */
          PangoAttrList *attrs = pango_attr_list_new ();
          gchar         *tooltip;

          pango_attr_list_insert (attrs, pango_attr_strikethrough_new (TRUE));
          tooltip = g_strdup_printf (_("Invalid regular expression: %s\n"),
                                     error->message);
          gtk_widget_set_tooltip_text (search->search_entry, tooltip);
          g_free (tooltip);

          clay_image_set_selected_items (image,
                                         item_view_class->item_type,
                                         NULL);

          gtk_entry_set_attributes (GTK_ENTRY (search->search_entry), attrs);
          pango_attr_list_unref (attrs);

          g_clear_error (&error);
          g_clear_object (&search->link_pattern_set);
        }
      else if (items == NULL)
        {
          /* Pattern does not match any results. */
          clay_image_set_selected_items (image,
                                         item_view_class->item_type,
                                         NULL);
          clay_widget_blink (search->search_entry);
        }
      else
        {
          clay_image_set_selected_items (image,
                                         item_view_class->item_type,
                                         items);
          g_list_free (items);
        }
    }
  else
    {
      gtk_widget_set_sensitive (search->link_entry, TRUE);
    }

  return TRUE;
}

static void
clay_item_search_link_entry_activate (ClayItemTreeView *view)
{
  if (clay_item_search_new_link_clicked (view))
    _clay_item_tree_view_search_hide (view);
}

static gboolean
clay_item_search_new_link_clicked (ClayItemTreeView *view)
{
  ClayItemTreeViewClass  *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  ClayItemTreeViewSearch *search          = view->search;
  ClayImage              *image;
  const gchar            *name;

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));

  if (! image)
    return TRUE;

  name = gtk_entry_get_text (GTK_ENTRY (search->link_entry));
  if (name && strlen (name) > 0)
    {
      ClayItemList *set;

      set = clay_item_list_named_new (image,
                                      item_view_class->item_type,
                                      name, NULL);
      if (set)
        {
          clay_image_store_item_set (image, set);
          gtk_entry_set_text (GTK_ENTRY (search->link_entry), "");
        }
      else
        {
          /* No existing selection. */
          return FALSE;
        }
    }
  else if (search->link_pattern_set != NULL)
    {
      clay_image_store_item_set (image, search->link_pattern_set);
      search->link_pattern_set = NULL;
      gtk_entry_set_text (GTK_ENTRY (search->search_entry), "");
    }
  else
    {
      clay_widget_blink (search->link_entry);
      clay_widget_blink (search->search_entry);

      return FALSE;
    }

  return TRUE;
}

static gboolean
clay_item_search_link_clicked (GtkWidget        *widget,
                               GdkEvent         *event,
                               ClayItemTreeView *view)
{
  ClayItemTreeViewSearch *search = view->search;
  ClayImage              *image;
  GdkEventButton         *bevent = (GdkEventButton *) event;
  GdkModifierType         modifiers;
  ClayItemList           *item_set;

  image = clay_item_tree_view_get_image (view);

  modifiers = bevent->state & clay_get_all_modifiers_mask ();

  item_set = g_object_get_data (G_OBJECT (widget), "link-set");

  if (modifiers == GDK_SHIFT_MASK)
    {
      clay_image_add_item_set (image, item_set);
    }
  else if (modifiers == GDK_CONTROL_MASK)
    {
      clay_image_remove_item_set (image, item_set);
    }
  else if (modifiers == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    {
      clay_image_intersect_item_set (image, item_set);
    }
  else
    {
      clay_image_select_item_set (image, item_set);
    }

  gtk_entry_set_text (GTK_ENTRY (search->search_entry), "");
  /* TODO: if clicking on pattern link, fill in the pattern field? */

  return FALSE;
}

static gboolean
clay_item_search_unlink_clicked (GtkWidget        *widget,
                                 GdkEvent         *event,
                                 ClayItemTreeView *view)
{
  ClayImage *image = clay_item_tree_view_get_image (view);

  clay_image_unlink_item_set (image, g_object_get_data (G_OBJECT (widget),
                                                        "link-set"));

  return TRUE;
}
