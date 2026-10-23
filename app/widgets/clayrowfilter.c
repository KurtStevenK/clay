/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowfilter.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/clay-utils.h"
#include "core/clayfilter.h"

#include "clayrowfilter.h"


typedef struct _ClayRowFilterPrivate ClayRowFilterPrivate;

struct _ClayRowFilterPrivate
{
  GtkWidget *active_toggle;
  GtkWidget *active_icon;
};

#define GET_PRIVATE(obj) \
  ((ClayRowFilterPrivate *) \
   clay_row_filter_get_instance_private ((ClayRowFilter *) obj))


static void   clay_row_filter_set_viewable        (ClayRow          *row,
                                                   ClayViewable     *viewable);
static void   clay_row_filter_set_view_size       (ClayRow          *row);

static void   clay_row_filter_real_active_toggled (ClayRowFilter    *row,
                                                   gboolean          active);

static void   clay_row_filter_active_changed      (ClayFilter       *filter,
                                                   ClayRowFilter    *row);
static void   clay_row_filter_active_toggled      (GtkToggleButton  *button,
                                                   ClayRowFilter    *row);


G_DEFINE_TYPE_WITH_PRIVATE (ClayRowFilter,
                            clay_row_filter,
                            CLAY_TYPE_ROW)

#define parent_class clay_row_filter_parent_class


static void
clay_row_filter_class_init (ClayRowFilterClass *klass)
{
  ClayRowClass *row_class = CLAY_ROW_CLASS (klass);

  row_class->set_viewable      = clay_row_filter_set_viewable;
  row_class->set_view_size     = clay_row_filter_set_view_size;

  klass->active_property       = "active";
  klass->active_changed_signal = "active-changed";

  klass->active_toggled        = clay_row_filter_real_active_toggled;
}

static void
clay_row_filter_init (ClayRowFilter *row)
{
  ClayRowFilterPrivate *priv = GET_PRIVATE (row);

  priv->active_toggle = _clay_row_add_toggle (CLAY_ROW (row),
                                              CLAY_ICON_VISIBLE,
                                              &priv->active_icon);

  g_signal_connect (priv->active_toggle, "toggled",
                    G_CALLBACK (clay_row_filter_active_toggled),
                    row);
}

static void
clay_row_filter_set_viewable (ClayRow      *row,
                              ClayViewable *viewable)
{
  ClayViewable *old_viewable = clay_row_get_viewable (row);

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            clay_row_filter_active_changed,
                                            row);
    }

  CLAY_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      g_signal_connect (viewable,
                        CLAY_ROW_FILTER_GET_CLASS (row)->active_changed_signal,
                        G_CALLBACK (clay_row_filter_active_changed),
                        row);

      clay_row_filter_active_changed (CLAY_FILTER (viewable),
                                      CLAY_ROW_FILTER (row));
    }
}

static void
clay_row_filter_set_view_size (ClayRow *row)
{
  ClayRowFilterPrivate *priv = GET_PRIVATE (row);
  gint                  view_size;

  CLAY_ROW_CLASS (parent_class)->set_view_size (row);

  view_size = clay_row_get_view_size (row, NULL);
  view_size = clay_view_size_get_smaller (view_size);

  gtk_image_set_pixel_size (GTK_IMAGE (priv->active_icon),
                            view_size);
}

static void
clay_row_filter_real_active_toggled (ClayRowFilter *row,
                                     gboolean       active)
{
  ClayViewable *viewable = clay_row_get_viewable (CLAY_ROW (row));

  if (viewable)
    g_object_set (viewable,
                  CLAY_ROW_FILTER_GET_CLASS (row)->active_property, active,
                  NULL);
}

static void
clay_row_filter_active_changed (ClayFilter    *filter,
                                ClayRowFilter *row)
{
  ClayRowFilterPrivate *priv = GET_PRIVATE (row);
  gboolean              active;

  g_signal_handlers_block_by_func (priv->active_toggle,
                                   clay_row_filter_active_toggled,
                                   row);

  g_object_get (filter,
                CLAY_ROW_FILTER_GET_CLASS (row)->active_property, &active,
                NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->active_toggle),
                                active);
  gtk_widget_set_opacity (priv->active_icon, active ? 1.0 : 0.0);

  g_signal_handlers_unblock_by_func (priv->active_toggle,
                                     clay_row_filter_active_toggled,
                                     row);
}

static void
clay_row_filter_active_toggled (GtkToggleButton *button,
                                ClayRowFilter   *row)
{
  ClayRowFilterPrivate *priv = GET_PRIVATE (row);
  gboolean              active;

  active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->active_toggle));

  CLAY_ROW_FILTER_GET_CLASS  (row)->active_toggled (row, active);
}
