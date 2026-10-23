/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo-dockable.c
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

#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"

#include "widgets-types.h"

#include "claycontainerview-utils.h"
#include "claycontainerview.h"
#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockable.h"
#include "claysessioninfo-aux.h"
#include "claysessioninfo-dockable.h"
#include "claysessionmanaged.h"
#include "claytoolbox.h"


enum
{
  SESSION_INFO_DOCKABLE_LOCKED,
  SESSION_INFO_DOCKABLE_TAB_STYLE,
  SESSION_INFO_DOCKABLE_VIEW_SIZE,
  SESSION_INFO_DOCKABLE_AUX
};


/*  public functions  */

ClaySessionInfoDockable *
clay_session_info_dockable_new (void)
{
  return g_slice_new0 (ClaySessionInfoDockable);
}

void
clay_session_info_dockable_free (ClaySessionInfoDockable *info)
{
  g_return_if_fail (info != NULL);

  g_clear_pointer (&info->identifier, g_free);

  if (info->aux_info)
    {
      g_list_free_full (info->aux_info,
                        (GDestroyNotify) clay_session_info_aux_free);
      info->aux_info = NULL;
    }

  g_slice_free (ClaySessionInfoDockable, info);
}

void
clay_session_info_dockable_serialize (ClayConfigWriter        *writer,
                                      ClaySessionInfoDockable *info)
{
  GEnumClass  *enum_class;
  GEnumValue  *enum_value;
  const gchar *tab_style = "icon";

  g_return_if_fail (writer != NULL);
  g_return_if_fail (info != NULL);

  enum_class = g_type_class_ref (CLAY_TYPE_TAB_STYLE);

  clay_config_writer_open (writer, "dockable");
  clay_config_writer_string (writer, info->identifier);

  if (info->locked)
    {
      clay_config_writer_open (writer, "locked");
      clay_config_writer_close (writer);
    }

  enum_value = g_enum_get_value (enum_class, info->tab_style);

  if (enum_value)
    tab_style = enum_value->value_nick;

  clay_config_writer_open (writer, "tab-style");
  clay_config_writer_print (writer, tab_style, -1);
  clay_config_writer_close (writer);

  if (info->view_size > 0)
    {
      clay_config_writer_open (writer, "preview-size");
      clay_config_writer_printf (writer, "%d", info->view_size);
      clay_config_writer_close (writer);
    }

  if (info->aux_info)
    clay_session_info_aux_serialize (writer, info->aux_info);

  clay_config_writer_close (writer);

  g_type_class_unref (enum_class);
}

GTokenType
clay_session_info_dockable_deserialize (GScanner                 *scanner,
                                        gint                      scope,
                                        ClaySessionInfoDockable **dockable)
{
  ClaySessionInfoDockable *info;
  GEnumClass              *enum_class;
  GEnumValue              *enum_value;
  GTokenType               token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (dockable != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "locked",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_LOCKED));
  g_scanner_scope_add_symbol (scanner, scope, "tab-style",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_TAB_STYLE));
  g_scanner_scope_add_symbol (scanner, scope, "preview-size",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_VIEW_SIZE));
  g_scanner_scope_add_symbol (scanner, scope, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_AUX));

  info = clay_session_info_dockable_new ();

  enum_class = g_type_class_ref (CLAY_TYPE_TAB_STYLE);

  token = G_TOKEN_STRING;
  if (! clay_scanner_parse_string (scanner, &info->identifier))
    goto error;

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
            case SESSION_INFO_DOCKABLE_LOCKED:
              info->locked = TRUE;
              break;

            case SESSION_INFO_DOCKABLE_TAB_STYLE:
              token = G_TOKEN_IDENTIFIER;
              if (g_scanner_peek_next_token (scanner) != token)
                goto error;

              g_scanner_get_next_token (scanner);

              enum_value = g_enum_get_value_by_nick (enum_class,
                                                     scanner->value.v_identifier);

              if (! enum_value)
                enum_value = g_enum_get_value_by_name (enum_class,
                                                       scanner->value.v_identifier);

              if (enum_value)
                info->tab_style = enum_value->value;
              break;

            case SESSION_INFO_DOCKABLE_VIEW_SIZE:
              token = G_TOKEN_INT;
              if (! clay_scanner_parse_int (scanner, &info->view_size))
                goto error;
              break;

            case SESSION_INFO_DOCKABLE_AUX:
              token = clay_session_info_aux_deserialize (scanner,
                                                         &info->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            default:
              goto error;
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

  *dockable = info;

  g_type_class_unref (enum_class);

  g_scanner_scope_remove_symbol (scanner, scope, "locked");
  g_scanner_scope_remove_symbol (scanner, scope, "tab-style");
  g_scanner_scope_remove_symbol (scanner, scope, "preview-size");
  g_scanner_scope_remove_symbol (scanner, scope, "aux-info");

  return token;

 error:
  *dockable = NULL;

  clay_session_info_dockable_free (info);
  g_type_class_unref (enum_class);

  return token;
}

ClaySessionInfoDockable *
clay_session_info_dockable_from_widget (ClayDockable *dockable)
{
  ClaySessionInfoDockable *info;
  ClayDialogFactoryEntry  *entry;
  ClayContainerView       *view;
  gint                     view_size = -1;

  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  clay_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

  g_return_val_if_fail (entry != NULL, NULL);

  info = clay_session_info_dockable_new ();

  info->locked     = clay_dockable_get_locked (dockable);
  info->identifier = g_strdup (entry->identifier);
  info->tab_style  = clay_dockable_get_tab_style (dockable);
  info->view_size  = -1;

  view = clay_container_view_get_by_dockable (dockable);

  if (view)
    view_size = clay_container_view_get_view_size (view, NULL);

  if (view_size > 0 && view_size != entry->view_size)
    info->view_size = view_size;

  if (CLAY_IS_SESSION_MANAGED (dockable))
    info->aux_info =
      clay_session_managed_get_aux_info (CLAY_SESSION_MANAGED (dockable));

  return info;
}

ClayDockable *
clay_session_info_dockable_restore (ClaySessionInfoDockable *info,
                                    ClayDock                *dock)
{
  GtkWidget *dockable;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  if (info->view_size < CLAY_VIEW_SIZE_TINY ||
      info->view_size > CLAY_VIEW_SIZE_GIGANTIC)
    info->view_size = -1;

  dockable =
    clay_dialog_factory_dockable_new (clay_dock_get_dialog_factory (dock),
                                      dock,
                                      info->identifier,
                                      info->view_size);

  if (dockable)
    {
      /*  clay_dialog_factory_dockable_new() might return an already
       *  existing singleton dockable, return NULL so our caller won't
       *  try to add it to another dockbook
       */
      if (clay_dockable_get_dockbook (CLAY_DOCKABLE (dockable)))
        return NULL;

      clay_dockable_set_locked    (CLAY_DOCKABLE (dockable), info->locked);
      clay_dockable_set_tab_style (CLAY_DOCKABLE (dockable), info->tab_style);

      if (info->aux_info)
        clay_session_managed_set_aux_info (CLAY_SESSION_MANAGED (dockable),
                                           info->aux_info);
    }

  return CLAY_DOCKABLE (dockable);
}
