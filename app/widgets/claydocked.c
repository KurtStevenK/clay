/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydocked.c
 * Copyright (C) 2003  Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/claycontext.h"

#include "claydocked.h"
#include "claysessioninfo-aux.h"


enum
{
  TITLE_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void    clay_docked_iface_set_aux_info (ClayDocked *docked,
                                               GList      *aux_info);
static GList * clay_docked_iface_get_aux_info (ClayDocked *docked);


G_DEFINE_INTERFACE (ClayDocked, clay_docked, GTK_TYPE_WIDGET)


static guint docked_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
clay_docked_default_init (ClayDockedInterface *iface)
{
  docked_signals[TITLE_CHANGED] =
    g_signal_new ("title-changed",
                  CLAY_TYPE_DOCKED,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockedInterface, title_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  iface->get_aux_info = clay_docked_iface_get_aux_info;
  iface->set_aux_info = clay_docked_iface_set_aux_info;
}

#define AUX_INFO_SHOW_BUTTON_BAR "show-button-bar"

static void
clay_docked_iface_set_aux_info (ClayDocked *docked,
                                GList      *aux_info)
{
  GList *list;

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (strcmp (aux->name, AUX_INFO_SHOW_BUTTON_BAR) == 0)
        {
          gboolean show = g_ascii_strcasecmp (aux->value, "false");

          clay_docked_set_show_button_bar (docked, show);
        }
    }
}

static GList *
clay_docked_iface_get_aux_info (ClayDocked *docked)
{
  if (clay_docked_has_button_bar (docked))
    {
      gboolean show = clay_docked_get_show_button_bar (docked);

      return g_list_append (NULL,
                            clay_session_info_aux_new (AUX_INFO_SHOW_BUTTON_BAR,
                                                       show ? "true" : "false"));
    }

  return NULL;
}


/*  public functions  */


void
clay_docked_title_changed (ClayDocked *docked)
{
  g_return_if_fail (CLAY_IS_DOCKED (docked));

  g_signal_emit (docked, docked_signals[TITLE_CHANGED], 0);
}

void
clay_docked_set_aux_info (ClayDocked *docked,
                          GList      *aux_info)
{
  ClayDockedInterface *docked_iface;

  g_return_if_fail (CLAY_IS_DOCKED (docked));

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_aux_info)
    docked_iface->set_aux_info (docked, aux_info);
}

GList *
clay_docked_get_aux_info (ClayDocked *docked)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), NULL);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_aux_info)
    return docked_iface->get_aux_info (docked);

  return NULL;
}

GtkWidget *
clay_docked_get_preview (ClayDocked  *docked,
                         ClayContext *context,
                         GtkIconSize  size)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), NULL);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_preview)
    return docked_iface->get_preview (docked, context, size);

  return NULL;
}

gboolean
clay_docked_get_prefer_icon (ClayDocked *docked)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), FALSE);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_prefer_icon)
    return docked_iface->get_prefer_icon (docked);

  return FALSE;
}

ClayUIManager *
clay_docked_get_menu (ClayDocked     *docked,
                      const gchar   **ui_path,
                      gpointer       *popup_data)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_menu)
    return docked_iface->get_menu (docked, ui_path, popup_data);

  return NULL;
}

gchar *
clay_docked_get_title (ClayDocked *docked)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), NULL);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_title)
    return docked_iface->get_title (docked);

  return NULL;
}

void
clay_docked_set_context (ClayDocked  *docked,
                         ClayContext *context)
{
  ClayDockedInterface *docked_iface;

  g_return_if_fail (CLAY_IS_DOCKED (docked));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_context)
    docked_iface->set_context (docked, context);
}

gboolean
clay_docked_has_button_bar (ClayDocked *docked)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), FALSE);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->has_button_bar)
    return docked_iface->has_button_bar (docked);

  return FALSE;
}

void
clay_docked_set_show_button_bar (ClayDocked *docked,
                                 gboolean    show)
{
  ClayDockedInterface *docked_iface;

  g_return_if_fail (CLAY_IS_DOCKED (docked));

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->set_show_button_bar)
    docked_iface->set_show_button_bar (docked, show ? TRUE : FALSE);
}

gboolean
clay_docked_get_show_button_bar (ClayDocked *docked)
{
  ClayDockedInterface *docked_iface;

  g_return_val_if_fail (CLAY_IS_DOCKED (docked), FALSE);

  docked_iface = CLAY_DOCKED_GET_IFACE (docked);

  if (docked_iface->get_show_button_bar)
    return docked_iface->get_show_button_bar (docked);

  return FALSE;
}
