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

#include "widgets-types.h"

#include "core/claycontext.h"
#include "core/clayimage.h"

#include "claydocked.h"
#include "clayimageeditor.h"
#include "clayuimanager.h"


static void   clay_image_editor_docked_iface_init (ClayDockedInterface *iface);

static void   clay_image_editor_set_context    (ClayDocked       *docked,
                                                ClayContext      *context);

static void   clay_image_editor_dispose        (GObject          *object);

static void   clay_image_editor_real_set_image (ClayImageEditor  *editor,
                                                ClayImage        *image);
static void   clay_image_editor_image_flush    (ClayImage        *image,
                                                gboolean          invalidate_preview,
                                                ClayImageEditor  *editor);
static gboolean
              clay_image_editor_image_flush_idle
                                               (gpointer user_data);


G_DEFINE_TYPE_WITH_CODE (ClayImageEditor, clay_image_editor, CLAY_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_image_editor_docked_iface_init))

#define parent_class clay_image_editor_parent_class


static void
clay_image_editor_class_init (ClayImageEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_image_editor_dispose;

  klass->set_image      = clay_image_editor_real_set_image;
}

static void
clay_image_editor_init (ClayImageEditor *editor)
{
  editor->image = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
clay_image_editor_docked_iface_init (ClayDockedInterface *iface)
{
  iface->set_context = clay_image_editor_set_context;
}

static void
clay_image_editor_set_context (ClayDocked  *docked,
                               ClayContext *context)
{
  ClayImageEditor *editor = CLAY_IMAGE_EDITOR (docked);
  ClayImage       *image  = NULL;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_image_editor_set_image,
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (context)
    {
      g_object_ref (editor->context);

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (clay_image_editor_set_image),
                                editor);

      image = clay_context_get_image (context);
    }

  clay_image_editor_set_image (editor, image);
}

static void
clay_image_editor_dispose (GObject *object)
{
  ClayImageEditor *editor = CLAY_IMAGE_EDITOR (object);

  if (editor->image)
    clay_image_editor_set_image (editor, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_image_editor_real_set_image (ClayImageEditor *editor,
                                  ClayImage       *image)
{
  if (editor->image)
    g_signal_handlers_disconnect_by_func (editor->image,
                                          clay_image_editor_image_flush,
                                          editor);

  editor->image = image;

  if (editor->image)
    g_signal_connect (editor->image, "flush",
                      G_CALLBACK (clay_image_editor_image_flush),
                      editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), image != NULL);
}


/*  public functions  */

void
clay_image_editor_set_image (ClayImageEditor *editor,
                             ClayImage       *image)
{
  g_return_if_fail (CLAY_IS_IMAGE_EDITOR (editor));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  if (image != editor->image)
    {
      CLAY_IMAGE_EDITOR_GET_CLASS (editor)->set_image (editor, image);

      if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
        clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                                clay_editor_get_popup_data (CLAY_EDITOR (editor)));
    }
}

ClayImage *
clay_image_editor_get_image (ClayImageEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_EDITOR (editor), NULL);

  return editor->image;
}


/*  private functions  */

static void
clay_image_editor_image_flush (ClayImage       *image,
                               gboolean         invalidate_preview,
                               ClayImageEditor *editor)
{
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) clay_image_editor_image_flush_idle,
                   g_object_ref (editor), g_object_unref);
}

static gboolean
clay_image_editor_image_flush_idle (gpointer user_data)
{
  ClayImageEditor *editor = user_data;

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));

  return G_SOURCE_REMOVE;
}
