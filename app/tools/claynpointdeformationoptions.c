/* xyz.kainzmayer.ClayDesign
 *
 * claynpointdeformationoptions.c
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "widgets/claypropwidgets.h"

#include "claynpointdeformationoptions.h"
#include "claytooloptions-gui.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_SQUARE_SIZE,
  PROP_RIGIDITY,
  PROP_ASAP_DEFORMATION,
  PROP_MLS_WEIGHTS,
  PROP_MLS_WEIGHTS_ALPHA,
  PROP_MESH_VISIBLE
};


static void   clay_n_point_deformation_options_set_property (GObject      *object,
                                                             guint         property_id,
                                                             const GValue *value,
                                                             GParamSpec   *pspec);
static void   clay_n_point_deformation_options_get_property (GObject      *object,
                                                             guint         property_id,
                                                             GValue       *value,
                                                             GParamSpec   *pspec);


G_DEFINE_TYPE (ClayNPointDeformationOptions, clay_n_point_deformation_options,
               CLAY_TYPE_TOOL_OPTIONS)

#define parent_class clay_n_point_deformation_options_parent_class


static void
clay_n_point_deformation_options_class_init (ClayNPointDeformationOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_n_point_deformation_options_set_property;
  object_class->get_property = clay_n_point_deformation_options_get_property;

  CLAY_CONFIG_PROP_DOUBLE  (object_class, PROP_SQUARE_SIZE,
                            "square-size",
                            _("Density"),
                            _("Density"),
                            5.0, 1000.0, 20.0,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE  (object_class, PROP_RIGIDITY,
                            "rigidity",
                            _("Rigidity"),
                            _("Rigidity"),
                            1.0, 10000.0, 100.0,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ASAP_DEFORMATION,
                            "asap-deformation",
                            _("Deformation mode"),
                            _("Deformation mode"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_MLS_WEIGHTS,
                            "mls-weights",
                            _("Use weights"),
                            _("Use weights"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE  (object_class, PROP_MLS_WEIGHTS_ALPHA,
                            "mls-weights-alpha",
                            _("Control points influence"),
                            _("Amount of control points' influence"),
                            0.1, 2.0, 1.0,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_MESH_VISIBLE,
                            "mesh-visible",
                            _("Show lattice"),
                            _("Show lattice"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_n_point_deformation_options_init (ClayNPointDeformationOptions *options)
{
}

static void
clay_n_point_deformation_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  ClayNPointDeformationOptions *options;

  options = CLAY_N_POINT_DEFORMATION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SQUARE_SIZE:
      options->square_size = g_value_get_double (value);
      break;
    case PROP_RIGIDITY:
      options->rigidity = g_value_get_double (value);
      break;
    case PROP_ASAP_DEFORMATION:
      options->asap_deformation = g_value_get_boolean (value);
      break;
    case PROP_MLS_WEIGHTS:
      options->mls_weights = g_value_get_boolean (value);
      break;
    case PROP_MLS_WEIGHTS_ALPHA:
      options->mls_weights_alpha = g_value_get_double (value);
      break;
    case PROP_MESH_VISIBLE:
      options->mesh_visible = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_n_point_deformation_options_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  ClayNPointDeformationOptions *options;

  options = CLAY_N_POINT_DEFORMATION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SQUARE_SIZE:
      g_value_set_double (value, options->square_size);
      break;
    case PROP_RIGIDITY:
      g_value_set_double (value, options->rigidity);
      break;
    case PROP_ASAP_DEFORMATION:
      g_value_set_boolean (value, options->asap_deformation);
      break;
    case PROP_MLS_WEIGHTS:
      g_value_set_boolean (value, options->mls_weights);
      break;
    case PROP_MLS_WEIGHTS_ALPHA:
      g_value_set_double (value, options->mls_weights_alpha);
      break;
    case PROP_MESH_VISIBLE:
      g_value_set_boolean (value, options->mesh_visible);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_n_point_deformation_options_gui (ClayToolOptions *tool_options)
{
  ClayNPointDeformationOptions *npd_options;
  GObject                      *config = G_OBJECT (tool_options);
  GtkWidget                    *vbox   = clay_tool_options_gui (tool_options);
  GtkWidget                    *widget;

  npd_options = CLAY_N_POINT_DEFORMATION_OPTIONS (tool_options);

  widget = clay_prop_check_button_new (config, "mesh-visible", NULL);
  npd_options->check_mesh_visible = widget;
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  widget = clay_prop_spin_scale_new (config, "square-size",
                                     1.0, 10.0, 0);
  npd_options->scale_square_size = widget;
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (widget), 10.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  widget = clay_prop_spin_scale_new (config, "rigidity",
                                     1.0, 10.0, 0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (widget), 1.0, 2000.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  widget = clay_prop_boolean_radio_frame_new (config, "asap-deformation",
                                              NULL,
                                              _("Scale"),
                                              _("Rigid (Rubber)"));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  widget = clay_prop_check_button_new (config, "mls-weights", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  widget = clay_prop_spin_scale_new (config, "mls-weights-alpha",
                                     0.1, 0.1, 1);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (widget), 0.1, 2.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_can_focus (widget, FALSE);

  g_object_bind_property (config, "mls-weights",
                          widget, "sensitive",
                          G_BINDING_DEFAULT |
                          G_BINDING_SYNC_CREATE);

  clay_n_point_deformation_options_set_sensitivity (npd_options, FALSE);

  return vbox;
}

void
clay_n_point_deformation_options_set_sensitivity (ClayNPointDeformationOptions *npd_options,
                                                  gboolean                      tool_active)
{
  gtk_widget_set_sensitive (npd_options->scale_square_size, ! tool_active);
  gtk_widget_set_sensitive (npd_options->check_mesh_visible, tool_active);
}
