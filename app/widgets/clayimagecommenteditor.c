/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageCommentEditor
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/claytemplate.h"

#include "clayimagecommenteditor.h"

#include "clay-intl.h"

#define CLAY_IMAGE_COMMENT_PARASITE "clay-comment"


static void  clay_image_comment_editor_update              (ClayImageParasiteView  *view);

static void  clay_image_comment_editor_buffer_changed      (GtkTextBuffer          *buffer,
                                                            ClayImageCommentEditor *editor);
static void  clay_image_comment_editor_use_default_comment (GtkWidget              *button,
                                                            ClayImageCommentEditor *editor);


G_DEFINE_TYPE (ClayImageCommentEditor,
               clay_image_comment_editor, CLAY_TYPE_IMAGE_PARASITE_VIEW)

static void
clay_image_comment_editor_class_init (ClayImageCommentEditorClass *klass)
{
  ClayImageParasiteViewClass *view_class;

  view_class = CLAY_IMAGE_PARASITE_VIEW_CLASS (klass);

  view_class->update = clay_image_comment_editor_update;
}

static void
clay_image_comment_editor_init (ClayImageCommentEditor *editor)
{
  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *text_view;
  GtkWidget *button;

  /* Init */
  editor->recoursing = FALSE;

  /* Vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (editor), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  /* Scrolled winow */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_window, TRUE);

  /* Text view */
  text_view = gtk_text_view_new ();

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 6);

  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_set_visible (text_view, TRUE);

  /* Button */
  button = gtk_button_new_with_mnemonic (_("Use _default comment"));
  clay_help_set_help_data (GTK_WIDGET (button),
                           _("Replace the current image comment with the "
                             "default comment set in "
                             "Edit→Preferences→Default Image."),
                           NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_image_comment_editor_use_default_comment),
                    editor);

  /* Buffer */
  editor->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  g_signal_connect (editor->buffer, "changed",
                    G_CALLBACK (clay_image_comment_editor_buffer_changed),
                    editor);
}


/*  public functions  */

GtkWidget *
clay_image_comment_editor_new (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return g_object_new (CLAY_TYPE_IMAGE_COMMENT_EDITOR,
                       "image",    image,
                       "parasite", CLAY_IMAGE_COMMENT_PARASITE,
                       NULL);
}


/*  private functions  */

static void
clay_image_comment_editor_update (ClayImageParasiteView *view)
{
  ClayImageCommentEditor *editor = CLAY_IMAGE_COMMENT_EDITOR (view);
  const ClayParasite     *parasite;

  if (editor->recoursing)
    return;

  g_signal_handlers_block_by_func (editor->buffer,
                                   clay_image_comment_editor_buffer_changed,
                                   editor);

  parasite = clay_image_parasite_view_get_parasite (view);

  if (parasite)
    {
      gchar   *text;
      guint32  text_size;

      text = (gchar *) clay_parasite_get_data (parasite, &text_size);
      text = g_strndup (text, text_size);

      if (! g_utf8_validate (text, -1, NULL))
        {
          gchar *tmp = clay_any_to_utf8 (text, -1, NULL);

          g_free (text);
          text = tmp;
        }

      gtk_text_buffer_set_text (editor->buffer, text, -1);
      g_free (text);
    }
  else
    {
      gtk_text_buffer_set_text (editor->buffer, "", 0);
    }

  g_signal_handlers_unblock_by_func (editor->buffer,
                                     clay_image_comment_editor_buffer_changed,
                                     editor);
}

static void
clay_image_comment_editor_buffer_changed (GtkTextBuffer          *buffer,
                                          ClayImageCommentEditor *editor)
{
  ClayImage   *image;
  gchar       *text;
  gint         len;
  GtkTextIter  start;
  GtkTextIter  end;

  image =
    clay_image_parasite_view_get_image (CLAY_IMAGE_PARASITE_VIEW (editor));

  gtk_text_buffer_get_bounds (buffer, &start, &end);

  text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  len = text ? strlen (text) : 0;

  editor->recoursing = TRUE;

  if (len > 0)
    {
      ClayParasite *parasite;

      parasite = clay_parasite_new (CLAY_IMAGE_COMMENT_PARASITE,
                                    CLAY_PARASITE_PERSISTENT,
                                    len + 1, text);

      clay_image_parasite_attach (image, parasite, TRUE);
      clay_parasite_free (parasite);
    }
  else
    {
      clay_image_parasite_detach (image, CLAY_IMAGE_COMMENT_PARASITE, TRUE);
    }

  editor->recoursing = FALSE;

  g_free (text);
}

static void
clay_image_comment_editor_use_default_comment (GtkWidget              *button,
                                               ClayImageCommentEditor *editor)
{
  ClayImage   *image;
  const gchar *comment = NULL;

  image = clay_image_parasite_view_get_image (CLAY_IMAGE_PARASITE_VIEW (editor));

  if (image)
    {
      ClayTemplate *template = image->clay->config->default_image;

      comment = clay_template_get_comment (template);
    }

  if (comment)
    gtk_text_buffer_set_text (editor->buffer, comment, -1);
  else
    gtk_text_buffer_set_text (editor->buffer, "", -1);
}
