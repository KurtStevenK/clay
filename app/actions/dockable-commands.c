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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "widgets/claycontainerview.h"
#include "widgets/claycontainerview-utils.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockable.h"
#include "widgets/claydockbook.h"
#include "widgets/claydocked.h"
#include "widgets/claysessioninfo.h"

#include "dockable-commands.h"


static ClayDockable * dockable_get_current (ClayDockbook *dockbook);


/*  public functions  */

void
dockable_add_tab_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);

  clay_dockbook_add_from_dialog_factory (dockbook,
                                         g_variant_get_string (value, NULL));
}

void
dockable_close_tab_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    gtk_container_remove (GTK_CONTAINER (dockbook),
                          GTK_WIDGET (dockable));
}

void
dockable_detach_tab_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    clay_dockable_detach (dockable);
}

void
dockable_lock_tab_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      gboolean lock = g_variant_get_boolean (value);

      clay_dockable_set_locked (dockable, lock);
    }
}

void
dockable_toggle_view_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable;
  ClayViewType  view_type;
  gint          page_num;

  view_type = (ClayViewType) g_variant_get_int32 (value);

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (ClayDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      ClayDialogFactoryEntry *entry;

      clay_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

      if (entry)
        {
          gchar *identifier;
          gchar *substring = NULL;

          identifier = g_strdup (entry->identifier);

          substring = strstr (identifier, "grid");

          if (substring && view_type == CLAY_VIEW_TYPE_GRID)
            {
              g_free (identifier);
              return;
            }

          if (! substring)
            {
              substring = strstr (identifier, "list");

              if (substring && view_type == CLAY_VIEW_TYPE_LIST)
                {
                  g_free (identifier);
                  return;
                }
            }

          if (substring)
            {
              ClayContainerView *old_view;
              GtkWidget         *new_dockable;
              ClayDock          *dock;
              gint               view_size = -1;

              if (view_type == CLAY_VIEW_TYPE_LIST)
                memcpy (substring, "list", 4);
              else if (view_type == CLAY_VIEW_TYPE_GRID)
                memcpy (substring, "grid", 4);

              old_view = clay_container_view_get_by_dockable (dockable);

              if (old_view)
                view_size = clay_container_view_get_view_size (old_view, NULL);

              dock         = clay_dockbook_get_dock (dockbook);
              new_dockable = clay_dialog_factory_dockable_new (clay_dock_get_dialog_factory (dock),
                                                               dock,
                                                               identifier,
                                                               view_size);

              if (new_dockable)
                {
                  ClayDocked *old;
                  ClayDocked *new;
                  gboolean    show;

                  clay_dockable_set_locked (CLAY_DOCKABLE (new_dockable),
                                            clay_dockable_get_locked (dockable));

                  old = CLAY_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
                  new = CLAY_DOCKED (gtk_bin_get_child (GTK_BIN (new_dockable)));

                  show = clay_docked_get_show_button_bar (old);
                  clay_docked_set_show_button_bar (new, show);

                  /*  Maybe clay_dialog_factory_dockable_new() returned
                   *  an already existing singleton dockable, so check
                   *  if it already is attached to a dockbook.
                   */
                  if (! clay_dockable_get_dockbook (CLAY_DOCKABLE (new_dockable)))
                    {
                      gtk_notebook_insert_page (GTK_NOTEBOOK (dockbook),
                                                new_dockable, NULL,
                                                page_num);
                      gtk_widget_set_visible (new_dockable, TRUE);

                      gtk_container_remove (GTK_CONTAINER (dockbook),
                                            GTK_WIDGET (dockable));

                      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                                     page_num);
                   }
                }
            }

          g_free (identifier);
        }
    }
}

void
dockable_view_size_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);
  gint          view_size;

  view_size = g_variant_get_int32 (value);

  if (dockable)
    {
      ClayContainerView *view = clay_container_view_get_by_dockable (dockable);

      if (view)
        {
          gint old_size;
          gint border_width;

          old_size = clay_container_view_get_view_size (view, &border_width);

          if (old_size != view_size)
            clay_container_view_set_view_size (view, view_size, border_width);
        }
    }
}

void
dockable_tab_style_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);
  ClayTabStyle  tab_style;

  tab_style = (ClayTabStyle) g_variant_get_int32 (value);

  if (dockable && clay_dockable_get_tab_style (dockable) != tab_style)
    {
      GtkWidget *tab_widget;

      clay_dockable_set_tab_style (dockable, tab_style);

      tab_widget = clay_dockbook_create_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
}

void
dockable_show_button_bar_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (data);
  ClayDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      ClayDocked *docked;
      gboolean    show;

      docked = CLAY_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
      show   = g_variant_get_boolean (value);

      clay_docked_set_show_button_bar (docked, show);
    }
}


/*  private functions  */

static ClayDockable *
dockable_get_current (ClayDockbook *dockbook)
{
  gint page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  return (ClayDockable *) gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                     page_num);
}
