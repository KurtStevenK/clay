/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayfontchooser.c
 * Copyright (C) 2003  Sven Neumann  <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"

#include "clayuitypes.h"
#include "clayfontchooser.h"
#include "clayuimarshal.h"

#include "libclay-intl.h"


/**
 * SECTION: clayfontchooser
 * @title: ClayFontChooser
 * @short_description: A button which pops up a font selection dialog.
 *
 * A button which pops up a font selection dialog.
 **/

struct _ClayFontChooser
{
  ClayResourceChooser  parent_instance;

  GtkWidget           *label;
};


static void clay_font_chooser_draw_interior (ClayResourceChooser *self);


static const GtkTargetEntry drag_target = { "application/x-clay-font-name", 0, 0 };


G_DEFINE_FINAL_TYPE (ClayFontChooser, clay_font_chooser, CLAY_TYPE_RESOURCE_CHOOSER)


static void
clay_font_chooser_class_init (ClayFontChooserClass *klass)
{
  ClayResourceChooserClass *superclass = CLAY_RESOURCE_CHOOSER_CLASS (klass);

  superclass->draw_interior = clay_font_chooser_draw_interior;
  superclass->resource_type = CLAY_TYPE_FONT;
}

static void
clay_font_chooser_init (ClayFontChooser *self)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (self), button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  image = gtk_image_new_from_icon_name (CLAY_ICON_FONT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  self->label = gtk_label_new ("unknown");
  gtk_box_pack_start (GTK_BOX (hbox), self->label, TRUE, TRUE, 4);

  gtk_widget_show_all (GTK_WIDGET (self));

  _clay_resource_chooser_set_drag_target (CLAY_RESOURCE_CHOOSER (self),
                                         hbox, &drag_target);

  _clay_resource_chooser_set_clickable (CLAY_RESOURCE_CHOOSER (self), button);
}

static void
clay_font_chooser_draw_interior (ClayResourceChooser *self)
{
  ClayFontChooser *font_select= CLAY_FONT_CHOOSER (self);
  ClayResource    *resource;
  gchar           *name = NULL;

  resource = clay_resource_chooser_get_resource (self);

  if (resource)
    name = clay_resource_get_name (resource);

  gtk_label_set_text (GTK_LABEL (font_select->label), name);
}


/**
 * clay_font_chooser_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label: (nullable): Button label or %NULL for no label.
 * @font:  (nullable): Initial font.
 *
 * Creates a new #GtkWidget that lets a user choose a font.
 * You can put this widget in a plug-in dialog.
 *
 * When font is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
clay_font_chooser_new (const gchar  *title,
                       const gchar  *label,
                       ClayFont     *font)
{
  GtkWidget *chooser;

  g_return_val_if_fail (font == NULL || CLAY_IS_FONT (font), NULL);

  if (font == NULL)
    font = clay_context_get_font ();

   if (title)
     chooser = g_object_new (CLAY_TYPE_FONT_CHOOSER,
                             "title",     title,
                             "label",     label,
                             "resource",  font,
                             NULL);
   else
     chooser = g_object_new (CLAY_TYPE_FONT_CHOOSER,
                             "label",     label,
                             "resource",  font,
                             NULL);

  clay_font_chooser_draw_interior (CLAY_RESOURCE_CHOOSER (chooser));

  return chooser;
}
