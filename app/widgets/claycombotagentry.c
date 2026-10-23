/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycombotagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claytag.h"
#include "core/claytagged.h"
#include "core/claytaggedcontainer.h"
#include "core/clayviewable.h"

#include "claytagentry.h"
#include "claytagpopup.h"
#include "claycombotagentry.h"


static void     clay_combo_tag_entry_constructed       (GObject              *object);

static void     clay_combo_tag_entry_icon_press        (GtkWidget            *widget,
                                                        GtkEntryIconPosition  icon_pos,
                                                        GdkEvent             *event,
                                                        gpointer              user_data);

static void     clay_combo_tag_entry_popup_destroy     (GtkWidget            *widget,
                                                        ClayComboTagEntry    *entry);

static void     clay_combo_tag_entry_tag_count_changed (ClayTaggedContainer  *container,
                                                        gint                  tag_count,
                                                        ClayComboTagEntry    *entry);


G_DEFINE_TYPE (ClayComboTagEntry, clay_combo_tag_entry, CLAY_TYPE_TAG_ENTRY);

#define parent_class clay_combo_tag_entry_parent_class


static void
clay_combo_tag_entry_class_init (ClayComboTagEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = clay_combo_tag_entry_constructed;
}

static void
clay_combo_tag_entry_init (ClayComboTagEntry *entry)
{
  entry->popup = NULL;

  gtk_widget_add_events (GTK_WIDGET (entry),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_POINTER_MOTION_MASK);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     CLAY_ICON_GO_DOWN);

  g_signal_connect (entry, "icon-press",
                    G_CALLBACK (clay_combo_tag_entry_icon_press),
                    NULL);
}

static void
clay_combo_tag_entry_constructed (GObject *object)
{
  ClayComboTagEntry *entry = CLAY_COMBO_TAG_ENTRY (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (CLAY_TAG_ENTRY (entry)->container,
                           "tag-count-changed",
                           G_CALLBACK (clay_combo_tag_entry_tag_count_changed),
                           entry, 0);
}

/**
 * clay_combo_tag_entry_new:
 * @container: a tagged container to be used.
 * @mode:      tag entry mode to work in.
 *
 * Creates a new #ClayComboTagEntry widget which extends #ClayTagEntry by
 * adding ability to pick tags using popup window (similar to combo box).
 *
 * Returns: a new #ClayComboTagEntry widget.
 **/
GtkWidget *
clay_combo_tag_entry_new (ClayTaggedContainer *container,
                          ClayTagEntryMode     mode)
{
  g_return_val_if_fail (CLAY_IS_TAGGED_CONTAINER (container), NULL);

  return g_object_new (CLAY_TYPE_COMBO_TAG_ENTRY,
                       "container", container,
                       "mode",      mode,
                       NULL);
}

static void
clay_combo_tag_entry_icon_press (GtkWidget            *widget,
                                 GtkEntryIconPosition  icon_pos,
                                 GdkEvent             *event,
                                 gpointer              user_data)
{
  ClayComboTagEntry *entry = CLAY_COMBO_TAG_ENTRY (widget);

  if (! entry->popup)
    {
      ClayTaggedContainer *container = CLAY_TAG_ENTRY (entry)->container;
      gint                 tag_count;

      tag_count = clay_tagged_container_get_tag_count (container);

      if (tag_count > 0 && ! CLAY_TAG_ENTRY (entry)->has_invalid_tags)
        {
          entry->popup = clay_tag_popup_new (entry);
          g_signal_connect (entry->popup, "destroy",
                            G_CALLBACK (clay_combo_tag_entry_popup_destroy),
                            entry);
          clay_tag_popup_show (CLAY_TAG_POPUP (entry->popup), event);
        }
    }
  else
    {
      gtk_widget_destroy (entry->popup);
    }
}

static void
clay_combo_tag_entry_popup_destroy (GtkWidget         *widget,
                                    ClayComboTagEntry *entry)
{
  entry->popup = NULL;
  gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void
clay_combo_tag_entry_tag_count_changed (ClayTaggedContainer *container,
                                        gint                 tag_count,
                                        ClayComboTagEntry   *entry)
{
  gboolean sensitive;

  sensitive = tag_count > 0 && ! CLAY_TAG_ENTRY (entry)->has_invalid_tags;

  gtk_entry_set_icon_sensitive (GTK_ENTRY (entry),
                                GTK_ENTRY_ICON_SECONDARY,
                                sensitive);
}
