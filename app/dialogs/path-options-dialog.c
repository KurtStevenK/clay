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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "core/claycontext.h"
#include "core/clayimage.h"

#include "path/claypath.h"

#include "item-options-dialog.h"
#include "path-options-dialog.h"

#include "clay-intl.h"


typedef struct _PathOptionsDialog PathOptionsDialog;

struct _PathOptionsDialog
{
  ClayPathOptionsCallback  callback;
  gpointer                 user_data;
};


/*  local function prototypes  */

static void  path_options_dialog_free     (PathOptionsDialog *private);
static void  path_options_dialog_callback (GtkWidget         *dialog,
                                           ClayImage         *image,
                                           ClayItem          *item,
                                           ClayContext       *context,
                                           const gchar       *item_name,
                                           gboolean           item_visible,
                                           ClayColorTag       item_color_tag,
                                           gboolean           item_lock_content,
                                           gboolean           item_lock_position,
                                           gboolean           item_lock_visibility,
                                           gpointer           user_data);


/*  public functions  */

GtkWidget *
path_options_dialog_new (ClayImage               *image,
                         ClayPath                *path,
                         ClayContext             *context,
                         GtkWidget               *parent,
                         const gchar             *title,
                         const gchar             *role,
                         const gchar             *icon_name,
                         const gchar             *desc,
                         const gchar             *help_id,
                         const gchar             *path_name,
                         gboolean                 path_visible,
                         ClayColorTag             path_color_tag,
                         gboolean                 path_lock_content,
                         gboolean                 path_lock_position,
                         gboolean                 path_lock_visibility,
                         ClayPathOptionsCallback  callback,
                         gpointer                 user_data)
{
  PathOptionsDialog *private;
  GtkWidget         *dialog;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (path == NULL || CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (PathOptionsDialog);

  private->callback  = callback;
  private->user_data = user_data;

  dialog = item_options_dialog_new (image, CLAY_ITEM (path), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    _("Path _name:"),
                                    CLAY_ICON_LOCK_PATH,
                                    _("Lock p_ath"),
                                    _("Lock path _position"),
                                    _("Lock path _visibility"),
                                    path_name,
                                    path_visible,
                                    path_color_tag,
                                    path_lock_content,
                                    path_lock_position,
                                    path_lock_visibility,
                                    path_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) path_options_dialog_free, private);

  return dialog;
}


/*  private functions  */

static void
path_options_dialog_free (PathOptionsDialog *private)
{
  g_slice_free (PathOptionsDialog, private);
}

static void
path_options_dialog_callback (GtkWidget    *dialog,
                              ClayImage    *image,
                              ClayItem     *item,
                              ClayContext  *context,
                              const gchar  *item_name,
                              gboolean      item_visible,
                              ClayColorTag  item_color_tag,
                              gboolean      item_lock_content,
                              gboolean      item_lock_position,
                              gboolean      item_lock_visibility,
                              gpointer      user_data)
{
  PathOptionsDialog *private = user_data;

  private->callback (dialog,
                     image,
                     CLAY_PATH (item),
                     context,
                     item_name,
                     item_visible,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     item_lock_visibility,
                     private->user_data);
}
