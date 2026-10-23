/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageParasiteView
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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/clayimage.h"

#include "clayimageparasiteview.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_PARASITE
};

enum
{
  UPDATE,
  LAST_SIGNAL
};


static void   clay_image_parasite_view_constructed  (GObject     *object);
static void   clay_image_parasite_view_finalize     (GObject     *object);
static void   clay_image_parasite_view_set_property (GObject     *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   clay_image_parasite_view_get_property (GObject     *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void   clay_image_parasite_view_parasite_changed (ClayImageParasiteView *view,
                                                         const gchar          *name);
static void   clay_image_parasite_view_update           (ClayImageParasiteView *view);


G_DEFINE_TYPE (ClayImageParasiteView, clay_image_parasite_view, GTK_TYPE_BOX)

#define parent_class clay_image_parasite_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
clay_image_parasite_view_class_init (ClayImageParasiteViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  view_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayImageParasiteViewClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = clay_image_parasite_view_constructed;
  object_class->finalize     = clay_image_parasite_view_finalize;
  object_class->set_property = clay_image_parasite_view_set_property;
  object_class->get_property = clay_image_parasite_view_get_property;

  klass->update              = NULL;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_PARASITE,
                                   g_param_spec_string ("parasite", NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_image_parasite_view_init (ClayImageParasiteView *view)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (view),
                                  GTK_ORIENTATION_VERTICAL);

  view->parasite = NULL;
}

static void
clay_image_parasite_view_constructed (GObject *object)
{
  ClayImageParasiteView *view = CLAY_IMAGE_PARASITE_VIEW (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (view->parasite != NULL);
  clay_assert (view->image != NULL);

  g_signal_connect_object (view->image, "parasite-attached",
                           G_CALLBACK (clay_image_parasite_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (view->image, "parasite-detached",
                           G_CALLBACK (clay_image_parasite_view_parasite_changed),
                           G_OBJECT (view),
                           G_CONNECT_SWAPPED);

  clay_image_parasite_view_update (view);
}

static void
clay_image_parasite_view_finalize (GObject *object)
{
  ClayImageParasiteView *view = CLAY_IMAGE_PARASITE_VIEW (object);

  g_clear_pointer (&view->parasite, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_image_parasite_view_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ClayImageParasiteView *view = CLAY_IMAGE_PARASITE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      view->image = g_value_get_object (value);
      break;
    case PROP_PARASITE:
      view->parasite = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_parasite_view_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ClayImageParasiteView *view = CLAY_IMAGE_PARASITE_VIEW (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, view->image);
      break;
    case PROP_PARASITE:
      g_value_set_string (value, view->parasite);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_image_parasite_view_new (ClayImage   *image,
                              const gchar *parasite)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return g_object_new (CLAY_TYPE_IMAGE_PARASITE_VIEW,
                       "image",    image,
                       "parasite", parasite,
                       NULL);
}


ClayImage *
clay_image_parasite_view_get_image (ClayImageParasiteView *view)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_PARASITE_VIEW (view), NULL);

  return view->image;
}

const ClayParasite *
clay_image_parasite_view_get_parasite (ClayImageParasiteView *view)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_PARASITE_VIEW (view), NULL);

  return clay_image_parasite_find (view->image, view->parasite);
}


/*  private functions  */

static void
clay_image_parasite_view_parasite_changed (ClayImageParasiteView *view,
                                           const gchar           *name)
{
  if (name && view->parasite && strcmp (name, view->parasite) == 0)
    clay_image_parasite_view_update (view);
}

static void
clay_image_parasite_view_update (ClayImageParasiteView *view)
{
  g_signal_emit (view, view_signals[UPDATE], 0);
}
