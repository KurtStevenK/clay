/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageProfileView
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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

#include "libclayconfig/clayconfig.h"
#include "libclaycolor/claycolor.h"
#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"

#include "clayimageprofileview.h"

#include "clay-intl.h"


static void   clay_image_profile_view_update (ClayImageParasiteView *view);


G_DEFINE_TYPE (ClayImageProfileView,
               clay_image_profile_view, CLAY_TYPE_IMAGE_PARASITE_VIEW)

#define parent_class clay_image_profile_view_parent_class


static void
clay_image_profile_view_class_init (ClayImageProfileViewClass *klass)
{
  ClayImageParasiteViewClass *view_class;

  view_class = CLAY_IMAGE_PARASITE_VIEW_CLASS (klass);

  view_class->update = clay_image_profile_view_update;
}

static void
clay_image_profile_view_init (ClayImageProfileView *view)
{
  GtkWidget *scrolled_window;
  GtkWidget *profile_view;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (view), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_window, TRUE);

  profile_view = clay_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled_window), profile_view);
  gtk_widget_set_visible (profile_view, TRUE);

  view->profile_view = CLAY_COLOR_PROFILE_VIEW (profile_view);
}


/*  public functions  */

GtkWidget *
clay_image_profile_view_new (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return g_object_new (CLAY_TYPE_IMAGE_PROFILE_VIEW,
                       "image",    image,
                       "parasite", CLAY_ICC_PROFILE_PARASITE_NAME,
                       NULL);
}


/*  private functions  */

static void
clay_image_profile_view_update (ClayImageParasiteView *view)
{
  ClayImageProfileView *profile_view = CLAY_IMAGE_PROFILE_VIEW (view);
  ClayImage            *image;
  ClayColorManaged     *managed;
  ClayColorProfile     *profile;

  image   = clay_image_parasite_view_get_image (view);
  managed = CLAY_COLOR_MANAGED (image);

  profile = clay_color_managed_get_color_profile (managed);

  clay_color_profile_view_set_profile (profile_view->profile_view, profile);
}
