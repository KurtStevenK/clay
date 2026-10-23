/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo-dock.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"

#include "widgets-types.h"

#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockbook.h"
#include "claydockcontainer.h"
#include "claydockwindow.h"
#include "claysessioninfo.h"
#include "claysessioninfo-aux.h"
#include "claysessioninfo-book.h"
#include "claysessioninfo-dock.h"
#include "claysessioninfo-private.h"
#include "claytoolbox.h"


enum
{
  SESSION_INFO_SIDE,
  SESSION_INFO_POSITION,
  SESSION_INFO_BOOK
};


static ClayAlignmentType clay_session_info_dock_get_side (ClayDock *dock);


static ClayAlignmentType
clay_session_info_dock_get_side (ClayDock *dock)
{
  ClayAlignmentType result   = -1;
  GtkWidget        *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dock));

  if (CLAY_IS_DOCK_CONTAINER (toplevel))
    {
      ClayDockContainer *container = CLAY_DOCK_CONTAINER (toplevel);

      result = clay_dock_container_get_dock_side (container, dock);
    }

  return result;
}


/*  public functions  */

ClaySessionInfoDock *
clay_session_info_dock_new (const gchar *dock_type)
{
  ClaySessionInfoDock *dock_info = NULL;

  dock_info = g_slice_new0 (ClaySessionInfoDock);
  dock_info->dock_type = g_strdup (dock_type);
  dock_info->side      = -1;

  return dock_info;
}

void
clay_session_info_dock_free (ClaySessionInfoDock *dock_info)
{
  g_return_if_fail (dock_info != NULL);

  g_clear_pointer (&dock_info->dock_type, g_free);

  if (dock_info->books)
    {
      g_list_free_full (dock_info->books,
                        (GDestroyNotify) clay_session_info_book_free);
      dock_info->books = NULL;
    }

  g_slice_free (ClaySessionInfoDock, dock_info);
}

void
clay_session_info_dock_serialize (ClayConfigWriter    *writer,
                                  ClaySessionInfoDock *dock_info)
{
  GList *list;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (dock_info != NULL);

  clay_config_writer_open (writer, dock_info->dock_type);

  if (dock_info->side != -1)
    {
      const char *side_text =
        dock_info->side == CLAY_ALIGN_LEFT ? "left" : "right";

      clay_config_writer_open (writer, "side");
      clay_config_writer_print (writer, side_text, strlen (side_text));
      clay_config_writer_close (writer);
    }

  if (dock_info->position != 0)
    {
      gint position;

      position = clay_session_info_apply_position_accuracy (dock_info->position);

      clay_config_writer_open (writer, "position");
      clay_config_writer_printf (writer, "%d", position);
      clay_config_writer_close (writer);
    }

  for (list = dock_info->books; list; list = g_list_next (list))
    clay_session_info_book_serialize (writer, list->data);

  clay_config_writer_close (writer);
}

GTokenType
clay_session_info_dock_deserialize (GScanner             *scanner,
                                    gint                  scope,
                                    ClaySessionInfoDock **dock_info,
                                    const gchar          *dock_type)
{
  GTokenType token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (dock_info != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "side",
                              GINT_TO_POINTER (SESSION_INFO_SIDE));
  g_scanner_scope_add_symbol (scanner, scope, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, scope, "book",
                              GINT_TO_POINTER (SESSION_INFO_BOOK));

  *dock_info = clay_session_info_dock_new (dock_type);

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
              ClaySessionInfoBook *book;

            case SESSION_INFO_SIDE:
              token = G_TOKEN_IDENTIFIER;
              if (g_scanner_peek_next_token (scanner) != token)
                break;

              g_scanner_get_next_token (scanner);

              if (strcmp ("left", scanner->value.v_identifier) == 0)
                (*dock_info)->side = CLAY_ALIGN_LEFT;
              else
                (*dock_info)->side = CLAY_ALIGN_RIGHT;
              break;

            case SESSION_INFO_POSITION:
              token = G_TOKEN_INT;
              if (! clay_scanner_parse_int (scanner, &((*dock_info)->position)))
                (*dock_info)->position = 0;
              break;

            case SESSION_INFO_BOOK:
              g_scanner_set_scope (scanner, scope + 1);
              token = clay_session_info_book_deserialize (scanner, scope + 1,
                                                          &book);

              if (token == G_TOKEN_LEFT_PAREN)
                {
                  (*dock_info)->books = g_list_append ((*dock_info)->books, book);
                  g_scanner_set_scope (scanner, scope);
                }
              else
                return token;

              break;

            default:
              return token;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  g_scanner_scope_remove_symbol (scanner, scope, "book");
  g_scanner_scope_remove_symbol (scanner, scope, "position");
  g_scanner_scope_remove_symbol (scanner, scope, "side");

  return token;
}

ClaySessionInfoDock *
clay_session_info_dock_from_widget (ClayDock *dock)
{
  ClaySessionInfoDock *dock_info;
  GList               *list;
  GtkWidget           *parent;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  dock_info = clay_session_info_dock_new (CLAY_IS_TOOLBOX (dock) ?
                                          "clay-toolbox" :
                                          "clay-dock");

  for (list = clay_dock_get_dockbooks (dock); list; list = g_list_next (list))
    {
      ClaySessionInfoBook *book;

      book = clay_session_info_book_from_widget (list->data);

      dock_info->books = g_list_prepend (dock_info->books, book);
    }

  dock_info->books = g_list_reverse (dock_info->books);
  dock_info->side  = clay_session_info_dock_get_side (dock);

  parent = gtk_widget_get_parent (GTK_WIDGET (dock));

  if (GTK_IS_PANED (parent))
    {
      GtkPaned *paned = GTK_PANED (parent);

      if (GTK_WIDGET (dock) == gtk_paned_get_child2 (paned))
        dock_info->position = gtk_paned_get_position (paned);
    }

  return dock_info;
}

ClayDock *
clay_session_info_dock_restore (ClaySessionInfoDock *dock_info,
                                ClayDialogFactory   *factory,
                                GdkMonitor          *monitor,
                                ClayDockContainer   *dock_container)
{
  gint           n_books = 0;
  GtkWidget     *dock;
  GList         *iter;
  ClayUIManager *ui_manager;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  ui_manager = clay_dock_container_get_ui_manager (dock_container);
  dock       = clay_dialog_factory_dialog_new (factory, monitor,
                                               ui_manager,
                                               NULL,
                                               dock_info->dock_type,
                                               -1 /*view_size*/,
                                               FALSE /*present*/);

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  /* Add the dock to the dock window immediately so the stuff in the
   * dock has access to e.g. a dialog factory
   */
  clay_dock_container_add_dock (dock_container,
                                CLAY_DOCK (dock),
                                dock_info);

  /* Note that if it is a toolbox, we will get here even though we
   * don't have any books
   */
  for (iter = dock_info->books;
       iter;
       iter = g_list_next (iter))
    {
      ClaySessionInfoBook *book_info = iter->data;
      GtkWidget           *dockbook;

      dockbook = GTK_WIDGET (clay_session_info_book_restore (book_info,
                                                             CLAY_DOCK (dock)));

      if (dockbook)
        {
          GtkWidget *parent = gtk_widget_get_parent (dockbook);

          n_books++;

          if (GTK_IS_PANED (parent))
            {
              GtkPaned *paned = GTK_PANED (parent);

              if (dockbook == gtk_paned_get_child2 (paned))
                gtk_paned_set_position (paned, book_info->position);
            }
        }
    }

  /* Now remove empty dockbooks from the list, check the comment in
   * clay_session_info_book_restore() which explains why the dock
   * can contain empty dockbooks at all
   */
  if (dock_info->books)
    {
      GList *books;

      books = g_list_copy (clay_dock_get_dockbooks (CLAY_DOCK (dock)));

      while (books)
        {
          GtkContainer *dockbook = books->data;
          GList        *children = gtk_container_get_children (dockbook);

          if (children)
            {
              g_list_free (children);
            }
          else
            {
              g_object_ref (dockbook);
              clay_dock_remove_book (CLAY_DOCK (dock), CLAY_DOCKBOOK (dockbook));
              gtk_widget_destroy (GTK_WIDGET (dockbook));
              g_object_unref (dockbook);

              n_books--;
            }

          books = g_list_remove (books, dockbook);
        }
    }

  /*  if we removed all books again, the dock was destroyed, so bail out  */
  if (dock_info->books && n_books == 0)
    {
      return NULL;
    }

  gtk_widget_set_visible (dock, TRUE);

  return CLAY_DOCK (dock);
}
