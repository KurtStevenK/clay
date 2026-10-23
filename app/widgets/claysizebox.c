/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysizebox.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#include <stdio.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "claysizebox.h"

#include "clay-intl.h"


#define SB_WIDTH 8

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_UNIT,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION,
  PROP_RESOLUTION_UNIT,
  PROP_KEEP_ASPECT,
  PROP_EDIT_RESOLUTION
};


#define CLAY_SIZE_BOX_GET_PRIVATE(obj) ((ClaySizeBoxPrivate *) clay_size_box_get_instance_private ((ClaySizeBox *) (obj)))

typedef struct _ClaySizeBoxPrivate ClaySizeBoxPrivate;

struct _ClaySizeBoxPrivate
{
  ClaySizeEntry   *size_entry;
  ClayChainButton *size_chain;
  GtkWidget       *pixel_label;
  GtkWidget       *res_label;
};


static void   clay_size_box_constructed       (GObject         *object);
static void   clay_size_box_dispose           (GObject         *object);
static void   clay_size_box_set_property      (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   clay_size_box_get_property      (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);

static void   clay_size_box_update_size       (ClaySizeBox     *box);
static void   clay_size_box_update_resolution (ClaySizeBox     *box);
static void   clay_size_box_chain_toggled     (ClayChainButton *button,
                                               ClaySizeBox     *box);


G_DEFINE_TYPE_WITH_PRIVATE (ClaySizeBox, clay_size_box, GTK_TYPE_BOX)

#define parent_class clay_size_box_parent_class


static void
clay_size_box_class_init (ClaySizeBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_size_box_constructed;
  object_class->dispose      = clay_size_box_dispose;
  object_class->set_property = clay_size_box_set_property;
  object_class->get_property = clay_size_box_get_property;

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     CLAY_MIN_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     256,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     CLAY_MIN_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE,
                                                     256,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_UNIT,
                                   clay_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, TRUE,
                                                         clay_unit_pixel (),
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_XRESOLUTION,
                                   g_param_spec_double ("xresolution",
                                                        NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        72.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_YRESOLUTION,
                                   g_param_spec_double ("yresolution",
                                                        NULL, NULL,
                                                        CLAY_MIN_RESOLUTION,
                                                        CLAY_MAX_RESOLUTION,
                                                        72.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_RESOLUTION_UNIT,
                                   clay_param_spec_unit ("resolution-unit",
                                                         NULL, NULL,
                                                         FALSE, FALSE,
                                                         clay_unit_inch (),
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_KEEP_ASPECT,
                                   g_param_spec_boolean ("keep-aspect",
                                                         NULL, NULL,
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_EDIT_RESOLUTION,
                                   g_param_spec_boolean ("edit-resolution",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_size_box_init (ClaySizeBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (box), 6);

  box->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
}

static void
clay_size_box_constructed (GObject *object)
{
  ClaySizeBox        *box  = CLAY_SIZE_BOX (object);
  ClaySizeBoxPrivate *priv = CLAY_SIZE_BOX_GET_PRIVATE (box);
  GtkWidget          *vbox;
  GtkWidget          *entry;
  GtkWidget          *hbox;
  GtkWidget          *label;
  GList              *children;
  GList              *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  entry = clay_coordinates_new (box->unit, "%n",
                                TRUE, TRUE, SB_WIDTH,
                                CLAY_SIZE_ENTRY_UPDATE_SIZE,
                                TRUE, TRUE,
                                _("_Width:"),
                                box->width, box->xresolution,
                                CLAY_MIN_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE,
                                0, box->width,
                                _("H_eight:"),
                                box->height, box->yresolution,
                                CLAY_MIN_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE,
                                0, box->height);

  priv->size_entry = CLAY_SIZE_ENTRY (entry);
  priv->size_chain = CLAY_COORDINATES_CHAINBUTTON (CLAY_SIZE_ENTRY (entry));

  /*
   * let clay_prop_coordinates_callback know how to interpret the chainbutton
   */
  g_object_set_data (G_OBJECT (priv->size_chain),
                     "constrains-ratio", GINT_TO_POINTER (TRUE));

  clay_prop_coordinates_connect (G_OBJECT (box),
                                 "width", "height",
                                 "unit",
                                 entry, NULL,
                                 box->xresolution,
                                 box->yresolution);

  g_signal_connect (priv->size_chain, "toggled",
                    G_CALLBACK (clay_size_box_chain_toggled),
                    box);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry, TRUE);

  children = gtk_container_get_children (GTK_CONTAINER (entry));
  for (list = children; list; list = g_list_next (list))
    if (GTK_IS_LABEL (list->data))
      gtk_size_group_add_widget (box->size_group, list->data);
  g_list_free (children);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_grid_attach (GTK_GRID (entry), vbox, 1, 2, 2, 1);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (NULL);
  clay_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  priv->pixel_label = label;

  if (box->edit_resolution)
    {
      gboolean chain_active;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      chain_active = ABS (box->xresolution -
                          box->yresolution) < CLAY_MIN_RESOLUTION;

      entry = clay_coordinates_new (box->resolution_unit, _("pixels/%a"),
                                    FALSE, FALSE, SB_WIDTH,
                                    CLAY_SIZE_ENTRY_UPDATE_RESOLUTION,
                                    chain_active, FALSE,
                                    _("_X resolution:"),
                                    box->xresolution, 1.0,
                                    1, 1, 1, 10,
                                    _("_Y resolution:"),
                                    box->yresolution, 1.0,
                                    1, 1, 1, 10);


      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_widget_set_visible (entry, TRUE);

      children = gtk_container_get_children (GTK_CONTAINER (entry));
      for (list = children; list; list = g_list_next (list))
        if (GTK_IS_LABEL (list->data))
          gtk_size_group_add_widget (box->size_group, list->data);
      g_list_free (children);

      clay_prop_coordinates_connect (G_OBJECT (box),
                                     "xresolution", "yresolution",
                                     "resolution-unit",
                                     entry, NULL,
                                     1.0, 1.0);
    }
  else
    {
      label = gtk_label_new (NULL);
      clay_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                                 -1);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      priv->res_label = label;
    }

  clay_size_box_update_size (box);
  clay_size_box_update_resolution (box);
}

static void
clay_size_box_dispose (GObject *object)
{
  ClaySizeBox *box = CLAY_SIZE_BOX (object);

  if (box->size_group)
    {
      g_object_unref (box->size_group);
      box->size_group = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_size_box_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClaySizeBox        *box  = CLAY_SIZE_BOX (object);
  ClaySizeBoxPrivate *priv = CLAY_SIZE_BOX_GET_PRIVATE (box);

  switch (property_id)
    {
    case PROP_WIDTH:
      box->width = g_value_get_int (value);
      clay_size_box_update_size (box);
      break;

    case PROP_HEIGHT:
      box->height = g_value_get_int (value);
      clay_size_box_update_size (box);
      break;

    case PROP_UNIT:
      box->unit = g_value_get_object (value);
      break;

    case PROP_XRESOLUTION:
      box->xresolution = g_value_get_double (value);
      if (priv->size_entry)
        clay_size_entry_set_resolution (priv->size_entry, 0,
                                        box->xresolution, TRUE);
      clay_size_box_update_resolution (box);
      break;

    case PROP_YRESOLUTION:
      box->yresolution = g_value_get_double (value);
      if (priv->size_entry)
        clay_size_entry_set_resolution (priv->size_entry, 1,
                                        box->yresolution, TRUE);
      clay_size_box_update_resolution (box);
      break;

    case PROP_RESOLUTION_UNIT:
      box->resolution_unit = g_value_get_object (value);
      break;

    case PROP_KEEP_ASPECT:
      if (priv->size_chain)
        clay_chain_button_set_active (priv->size_chain,
                                      g_value_get_boolean (value));
      break;

    case PROP_EDIT_RESOLUTION:
      box->edit_resolution = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_size_box_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClaySizeBox        *box  = CLAY_SIZE_BOX (object);
  ClaySizeBoxPrivate *priv = CLAY_SIZE_BOX_GET_PRIVATE (box);

  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, box->width);
      break;

    case PROP_HEIGHT:
      g_value_set_int (value, box->height);
      break;

    case PROP_UNIT:
      g_value_set_object (value, box->unit);
      break;

    case PROP_XRESOLUTION:
      g_value_set_double (value, box->xresolution);
      break;

    case PROP_YRESOLUTION:
      g_value_set_double (value, box->yresolution);
      break;

    case PROP_RESOLUTION_UNIT:
      g_value_set_object (value, box->resolution_unit);
      break;

    case PROP_KEEP_ASPECT:
      g_value_set_boolean (value,
                           clay_chain_button_get_active (priv->size_chain));
      break;

    case PROP_EDIT_RESOLUTION:
      g_value_set_boolean (value, box->edit_resolution);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_size_box_update_size (ClaySizeBox *box)
{
  ClaySizeBoxPrivate *priv = CLAY_SIZE_BOX_GET_PRIVATE (box);

  if (priv->pixel_label)
    {
      gchar *text = g_strdup_printf (ngettext ("%d × %d pixel",
                                               "%d × %d pixels", box->height),
                                     box->width, box->height);
      gtk_label_set_text (GTK_LABEL (priv->pixel_label), text);
      g_free (text);
    }
}

static void
clay_size_box_update_resolution (ClaySizeBox *box)
{
  ClaySizeBoxPrivate *priv = CLAY_SIZE_BOX_GET_PRIVATE (box);

  if (priv->size_entry)
    {
      clay_size_entry_set_refval (priv->size_entry, 0, box->width);
      clay_size_entry_set_refval (priv->size_entry, 1, box->height);
    }

  if (priv->res_label)
    {
      gchar *text;
      gint   xres = ROUND (box->xresolution);
      gint   yres = ROUND (box->yresolution);

      if (xres != yres)
        text = g_strdup_printf (_("%d × %d ppi"), xres, yres);
      else
        text = g_strdup_printf (_("%d ppi"), yres);

      gtk_label_set_text (GTK_LABEL (priv->res_label), text);
      g_free (text);
    }
}

static void
clay_size_box_chain_toggled (ClayChainButton *button,
                             ClaySizeBox     *box)
{
  g_object_set (box,
                "keep-aspect", clay_chain_button_get_active (button),
                NULL);
}

ClaySizeEntry *
clay_size_box_get_size_entry (ClaySizeBox *size_box)
{
  ClaySizeBoxPrivate *priv;

  g_return_val_if_fail (CLAY_IS_SIZE_BOX (size_box), NULL);

  priv = CLAY_SIZE_BOX_GET_PRIVATE (size_box);

  return priv->size_entry;
}
