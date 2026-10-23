/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysymmetryeditor.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "propgui/propgui-types.h" /* ugly, but what the heck */

#include "core/clayimage.h"
#include "core/clayimage-symmetry.h"
#include "core/claysymmetry.h"

#include "propgui/claypropgui.h"

#include "claymenufactory.h"
#include "claysymmetryeditor.h"

#include "clay-intl.h"


struct _ClaySymmetryEditorPrivate
{
  ClayContext *context;

  GtkWidget   *menu;
  GtkWidget   *options_vbox;
};


static void   clay_symmetry_editor_set_image        (ClayImageEditor    *editor,
                                                     ClayImage          *image);

/* Signal handlers on the contextual image. */
static void   clay_symmetry_editor_symmetry_notify  (ClayImage          *image,
                                                     GParamSpec         *pspec,
                                                     ClaySymmetryEditor *editor);

/* Signal handlers on the symmetry. */
static void   clay_symmetry_editor_symmetry_updated (ClaySymmetry       *symmetry,
                                                     ClayImage          *image,
                                                     ClaySymmetryEditor *editor);

/* Private functions. */
static void   clay_symmetry_editor_set_options      (ClaySymmetryEditor *editor,
                                                     ClaySymmetry       *symmetry);


G_DEFINE_TYPE_WITH_PRIVATE (ClaySymmetryEditor, clay_symmetry_editor,
                            CLAY_TYPE_IMAGE_EDITOR)

#define parent_class clay_symmetry_editor_parent_class


static void
clay_symmetry_editor_class_init (ClaySymmetryEditorClass *klass)
{
  ClayImageEditorClass *image_editor_class = CLAY_IMAGE_EDITOR_CLASS (klass);

  image_editor_class->set_image = clay_symmetry_editor_set_image;
}

static void
clay_symmetry_editor_init (ClaySymmetryEditor *editor)
{
  GtkWidget *scrolled_window;
  GtkWidget *viewport;

  editor->p = clay_symmetry_editor_get_instance_private (editor);

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_window, TRUE);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_set_visible (viewport, TRUE);

  editor->p->options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  g_object_set (editor->p->options_vbox, "valign", GTK_ALIGN_START, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (editor->p->options_vbox), 2);
  gtk_container_add (GTK_CONTAINER (viewport), editor->p->options_vbox);
  gtk_widget_set_visible (editor->p->options_vbox, TRUE);

  clay_symmetry_editor_set_image (CLAY_IMAGE_EDITOR (editor), NULL);
}

static void
clay_symmetry_editor_set_image (ClayImageEditor *image_editor,
                                ClayImage       *image)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  GList        *syms;
  GList        *sym_iter;
  ClaySymmetry *symmetry;

  ClaySymmetryEditor *editor = CLAY_SYMMETRY_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            G_CALLBACK (clay_symmetry_editor_symmetry_notify),
                                            editor);
    }

  CLAY_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  /* Destroy the previous menu. */
  if (editor->p->menu)
    {
      gtk_widget_destroy (editor->p->menu);
      editor->p->menu = NULL;
    }

  store = g_object_new (CLAY_TYPE_INT_STORE, NULL);

  /* The menu of available symmetries. */
  syms = clay_image_symmetry_list ();
  for (sym_iter = syms; sym_iter; sym_iter = g_list_next (sym_iter))
    {
      ClaySymmetryClass *klass;
      GType              type;

      type = (GType) sym_iter->data;
      klass = g_type_class_ref (type);

      gtk_list_store_prepend (store, &iter);
      gtk_list_store_set (store, &iter,
                          CLAY_INT_STORE_LABEL,
                          klass->label,
                          CLAY_INT_STORE_USER_DATA,
                          sym_iter->data,
                          -1);
      g_type_class_unref (klass);
    }
  g_list_free (syms);

  gtk_list_store_prepend (store, &iter);
  gtk_list_store_set (store, &iter,
                      CLAY_INT_STORE_LABEL, _("None"),
                      CLAY_INT_STORE_USER_DATA, CLAY_TYPE_SYMMETRY,
                      -1);
  if (image_editor->image)
    editor->p->menu =
      clay_prop_pointer_combo_box_new (G_OBJECT (image_editor->image),
                                       "symmetry",
                                       CLAY_INT_STORE (store));
  else
    editor->p->menu =
      clay_int_combo_box_new (_("None"), 0,
                              NULL);

  g_object_unref (store);

  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (editor->p->menu),
                                _("Symmetry"));
  g_object_set (editor->p->menu, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  gtk_box_pack_start (GTK_BOX (editor), editor->p->menu,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor), editor->p->menu, 0);

  if (image_editor->image)
    {
      /* Connect to symmetry change. */
      g_signal_connect (image_editor->image, "notify::symmetry",
                        G_CALLBACK (clay_symmetry_editor_symmetry_notify),
                        editor);

      /* Update the symmetry options. */
      symmetry = clay_image_get_active_symmetry (image_editor->image);
      clay_symmetry_editor_set_options (editor, symmetry);
    }
  else
    {
      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (editor->p->menu), 0);
      gtk_widget_set_sensitive (editor->p->menu, FALSE);
      gtk_widget_set_visible (editor->p->menu, TRUE);
    }
}

static void
clay_symmetry_editor_symmetry_notify (ClayImage          *image,
                                      GParamSpec         *pspec,
                                      ClaySymmetryEditor *editor)
{
  ClaySymmetry *symmetry = NULL;

  if (image)
    {
      symmetry = clay_image_get_active_symmetry (image);

      if (symmetry)
        g_signal_connect (symmetry, "gui-param-changed",
                          G_CALLBACK (clay_symmetry_editor_symmetry_updated),
                          editor);
    }

  clay_symmetry_editor_set_options (editor, symmetry);
}

static void
clay_symmetry_editor_symmetry_updated (ClaySymmetry       *symmetry,
                                       ClayImage          *image,
                                       ClaySymmetryEditor *editor)
{
  if (image    != clay_image_editor_get_image (CLAY_IMAGE_EDITOR (editor)) ||
      symmetry != clay_image_get_active_symmetry (image))
    {
      g_signal_handlers_disconnect_by_func (symmetry,
                                            clay_symmetry_editor_symmetry_updated,
                                            editor);
      return;
    }

  clay_symmetry_editor_set_options (editor, symmetry);
}

static void
clay_symmetry_editor_set_options (ClaySymmetryEditor *editor,
                                  ClaySymmetry       *symmetry)
{
  gtk_container_foreach (GTK_CONTAINER (editor->p->options_vbox),
                         (GtkCallback) gtk_widget_destroy, NULL);

  if (symmetry && G_TYPE_FROM_INSTANCE (symmetry) != CLAY_TYPE_SYMMETRY)
    {
      ClayImageEditor *image_editor = CLAY_IMAGE_EDITOR (editor);
      ClayImage       *image        = image_editor->image;
      GtkWidget       *gui;

      gui = clay_prop_gui_new (G_OBJECT (symmetry),
                               CLAY_TYPE_SYMMETRY,
                               CLAY_SYMMETRY_PARAM_GUI,
                               GEGL_RECTANGLE (0, 0,
                                               clay_image_get_width  (image),
                                               clay_image_get_height (image)),
                               CLAY_IMAGE_EDITOR (editor)->context,
                               NULL, NULL, NULL);
      gtk_box_pack_start (GTK_BOX (editor->p->options_vbox), gui,
                          FALSE, FALSE, 0);
    }
}


/*  public functions  */

GtkWidget *
clay_symmetry_editor_new (ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_SYMMETRY_EDITOR,
                       "menu-factory",    menu_factory,
                       NULL);
}
