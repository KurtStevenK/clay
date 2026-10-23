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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claytoolinfo.h"

#include "widgets/claypropwidgets.h"
#include "widgets/claywidgets-utils.h"

#include "clayperspectivetool.h"
#include "clayrotatetool.h"
#include "clayscaletool.h"
#include "clayunifiedtransformtool.h"
#include "claytooloptions-gui.h"
#include "claytransformgridoptions.h"
#include "claytransformgridtool.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_DIRECTION,
  PROP_DIRECTION_LINKED,
  PROP_SHOW_PREVIEW,
  PROP_COMPOSITED_PREVIEW,
  PROP_SYNCHRONOUS_PREVIEW,
  PROP_PREVIEW_OPACITY,
  PROP_GRID_TYPE,
  PROP_GRID_SIZE,
  PROP_CONSTRAIN_MOVE,
  PROP_CONSTRAIN_SCALE,
  PROP_CONSTRAIN_ROTATE,
  PROP_CONSTRAIN_SHEAR,
  PROP_CONSTRAIN_PERSPECTIVE,
  PROP_FROMPIVOT_SCALE,
  PROP_FROMPIVOT_SHEAR,
  PROP_FROMPIVOT_PERSPECTIVE,
  PROP_CORNERSNAP,
  PROP_FIXEDPIVOT,
};


static void       clay_transform_grid_options_set_property (GObject      *object,
                                                            guint         property_id,
                                                            const GValue *value,
                                                            GParamSpec   *pspec);
static void       clay_transform_grid_options_get_property (GObject      *object,
                                                            guint         property_id,
                                                            GValue       *value,
                                                            GParamSpec   *pspec);

static gboolean   clay_transform_grid_options_sync_grid    (GBinding     *binding,
                                                            const GValue *source_value,
                                                            GValue       *target_value,
                                                            gpointer      user_data);


G_DEFINE_TYPE (ClayTransformGridOptions, clay_transform_grid_options,
               CLAY_TYPE_TRANSFORM_OPTIONS)

#define parent_class clay_transform_grid_options_parent_class


static void
clay_transform_grid_options_class_init (ClayTransformGridOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_transform_grid_options_set_property;
  object_class->get_property = clay_transform_grid_options_get_property;

  g_object_class_override_property (object_class, PROP_DIRECTION,
                                    "direction");

  g_object_class_install_property (object_class, PROP_DIRECTION_LINKED,
                                   g_param_spec_boolean ("direction-linked",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_PREVIEW,
                            "show-preview",
                            _("Show image preview"),
                            _("Show a preview of the transformed image"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_COMPOSITED_PREVIEW,
                            "composited-preview",
                            _("Composited preview"),
                            _("Show preview as part of the image composition"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SYNCHRONOUS_PREVIEW,
                            "synchronous-preview",
                            _("Synchronous preview"),
                            _("Render the preview synchronously"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_PREVIEW_OPACITY,
                           "preview-opacity",
                           _("Image opacity"),
                           _("Opacity of the preview image"),
                           0.0, 1.0, 1.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_GRID_TYPE,
                         "grid-type",
                         _("Guides"),
                         _("Composition guides such as rule of thirds"),
                         CLAY_TYPE_GUIDES_TYPE,
                         CLAY_GUIDES_NONE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_INT (object_class, PROP_GRID_SIZE,
                        "grid-size",
                        NULL,
                        _("Size of a grid cell for variable number "
                          "of composition guides"),
                        1, 128, 15,
                        CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_MOVE,
                            "constrain-move",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_SCALE,
                            "constrain-scale",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_ROTATE,
                            "constrain-rotate",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_SHEAR,
                            "constrain-shear",
                            NULL, NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_PERSPECTIVE,
                            "constrain-perspective",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FROMPIVOT_SCALE,
                            "frompivot-scale",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FROMPIVOT_SHEAR,
                            "frompivot-shear",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FROMPIVOT_PERSPECTIVE,
                            "frompivot-perspective",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CORNERSNAP,
                            "cornersnap",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FIXEDPIVOT,
                            "fixedpivot",
                            NULL, NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_transform_grid_options_init (ClayTransformGridOptions *options)
{
}

static void
clay_transform_grid_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayTransformGridOptions *options           = CLAY_TRANSFORM_GRID_OPTIONS (object);
  ClayTransformOptions     *transform_options = CLAY_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_DIRECTION:
      transform_options->direction = g_value_get_enum (value);

      /* Expected default for corrective transform_grid is to see the
       * original image only.
       */
      g_object_set (options,
                    "show-preview",
                    transform_options->direction != CLAY_TRANSFORM_BACKWARD,
                    NULL);
      break;
    case PROP_DIRECTION_LINKED:
      options->direction_linked = g_value_get_boolean (value);
      break;
    case PROP_SHOW_PREVIEW:
      options->show_preview = g_value_get_boolean (value);
      break;
    case PROP_COMPOSITED_PREVIEW:
      options->composited_preview = g_value_get_boolean (value);
      break;
    case PROP_SYNCHRONOUS_PREVIEW:
      options->synchronous_preview = g_value_get_boolean (value);
      break;
    case PROP_PREVIEW_OPACITY:
      options->preview_opacity = g_value_get_double (value);
      break;
    case PROP_GRID_TYPE:
      options->grid_type = g_value_get_enum (value);
      break;
    case PROP_GRID_SIZE:
      options->grid_size = g_value_get_int (value);
      break;
    case PROP_CONSTRAIN_MOVE:
      options->constrain_move = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_SCALE:
      options->constrain_scale = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_ROTATE:
      options->constrain_rotate = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_SHEAR:
      options->constrain_shear = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_PERSPECTIVE:
      options->constrain_perspective = g_value_get_boolean (value);
      break;
    case PROP_FROMPIVOT_SCALE:
      options->frompivot_scale = g_value_get_boolean (value);
      break;
    case PROP_FROMPIVOT_SHEAR:
      options->frompivot_shear = g_value_get_boolean (value);
      break;
    case PROP_FROMPIVOT_PERSPECTIVE:
      options->frompivot_perspective = g_value_get_boolean (value);
      break;
    case PROP_CORNERSNAP:
      options->cornersnap = g_value_get_boolean (value);
      break;
    case PROP_FIXEDPIVOT:
      options->fixedpivot = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_transform_grid_options_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ClayTransformGridOptions *options           = CLAY_TRANSFORM_GRID_OPTIONS (object);
  ClayTransformOptions     *transform_options = CLAY_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_DIRECTION:
      g_value_set_enum (value, transform_options->direction);
      break;
    case PROP_DIRECTION_LINKED:
      g_value_set_boolean (value, options->direction_linked);
      break;
    case PROP_SHOW_PREVIEW:
      g_value_set_boolean (value, options->show_preview);
      break;
    case PROP_COMPOSITED_PREVIEW:
      g_value_set_boolean (value, options->composited_preview);
      break;
    case PROP_SYNCHRONOUS_PREVIEW:
      g_value_set_boolean (value, options->synchronous_preview);
      break;
    case PROP_PREVIEW_OPACITY:
      g_value_set_double (value, options->preview_opacity);
      break;
    case PROP_GRID_TYPE:
      g_value_set_enum (value, options->grid_type);
      break;
    case PROP_GRID_SIZE:
      g_value_set_int (value, options->grid_size);
      break;
    case PROP_CONSTRAIN_MOVE:
      g_value_set_boolean (value, options->constrain_move);
      break;
    case PROP_CONSTRAIN_SCALE:
      g_value_set_boolean (value, options->constrain_scale);
      break;
    case PROP_CONSTRAIN_ROTATE:
      g_value_set_boolean (value, options->constrain_rotate);
      break;
    case PROP_CONSTRAIN_SHEAR:
      g_value_set_boolean (value, options->constrain_shear);
      break;
    case PROP_CONSTRAIN_PERSPECTIVE:
      g_value_set_boolean (value, options->constrain_perspective);
      break;
    case PROP_FROMPIVOT_SCALE:
      g_value_set_boolean (value, options->frompivot_scale);
      break;
    case PROP_FROMPIVOT_SHEAR:
      g_value_set_boolean (value, options->frompivot_shear);
      break;
    case PROP_FROMPIVOT_PERSPECTIVE:
      g_value_set_boolean (value, options->frompivot_perspective);
      break;
    case PROP_CORNERSNAP:
      g_value_set_boolean (value, options->cornersnap);
      break;
    case PROP_FIXEDPIVOT:
      g_value_set_boolean (value, options->fixedpivot);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_transform_grid_options_gui:
 * @tool_options: a #ClayToolOptions
 *
 * Build the TransformGrid Tool Options.
 *
 * Returns: a container holding the transform_grid tool options
 **/
GtkWidget *
clay_transform_grid_options_gui (ClayToolOptions *tool_options)
{
  GObject                    *config  = G_OBJECT (tool_options);
  ClayTransformGridOptions   *options = CLAY_TRANSFORM_GRID_OPTIONS (tool_options);
  ClayTransformGridToolClass *tg_class;
  GtkWidget                  *vbox;
  GtkWidget                  *vbox2;
  GtkWidget                  *vbox3;
  GtkWidget                  *button;
  GtkWidget                  *frame;
  GtkWidget                  *combo;
  GtkWidget                  *scale;
  GtkWidget                  *grid_box;
  GdkModifierType             extend_mask    = clay_get_extend_selection_mask ();
  GdkModifierType             constrain_mask = clay_get_constrain_behavior_mask ();

  vbox = clay_transform_options_gui (tool_options, TRUE, TRUE, TRUE);

  tg_class  = g_type_class_ref (tool_options->tool_info->tool_type);

  /* the direction-link button */
  if (tg_class->matrix_to_info)
    {
      ClayTransformOptions *tr_options = CLAY_TRANSFORM_OPTIONS (tool_options);
      GtkWidget            *hbox;

      vbox2 = gtk_bin_get_child (GTK_BIN (tr_options->direction_frame));
      g_object_ref (vbox2);
      gtk_container_remove (GTK_CONTAINER (tr_options->direction_frame), vbox2);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
      gtk_container_add (GTK_CONTAINER (tr_options->direction_frame), hbox);
      gtk_widget_set_visible (hbox, TRUE);

      gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
      g_object_unref (vbox2);

      button = clay_chain_button_new (CLAY_CHAIN_RIGHT);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_set_sensitive (button, FALSE);
      clay_chain_button_set_icon_size (CLAY_CHAIN_BUTTON (button),
                                       GTK_ICON_SIZE_MENU);
      gtk_widget_set_visible (button, TRUE);

      clay_help_set_help_data (button,
                               _("If connected, you can reposition the "
                                 "handles without altering the "
                                 "transformation."),
                               NULL);

      g_object_bind_property (config, "direction-linked",
                              button, "active",
                              G_BINDING_BIDIRECTIONAL |
                              G_BINDING_SYNC_CREATE);

      options->direction_chain_button = button;
    }

  g_type_class_unref (tg_class);

  /*  the preview frame  */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  button = clay_prop_check_button_new (config, "synchronous-preview", NULL);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  frame = clay_prop_expanding_frame_new (config, "composited-preview", NULL,
                                         vbox3, NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  scale = clay_prop_spin_scale_new (config, "preview-opacity",
                                    0.01, 0.1, 0);
  clay_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_object_bind_property (config, "composited-preview",
                          scale,  "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  frame = clay_prop_expanding_frame_new (config, "show-preview", NULL,
                                         vbox2, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  /*  the guides frame  */
  frame = clay_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  /*  the guides type menu  */
  combo = clay_prop_enum_combo_box_new (config, "grid-type", 0, 0);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Guides"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);

  /*  the grid density scale  */
  scale = clay_prop_spin_scale_new (config, "grid-size",
                                    1.8, 8.0, 0);
  clay_spin_scale_set_label (CLAY_SPIN_SCALE (scale), NULL);
  gtk_container_add (GTK_CONTAINER (frame), scale);

  g_object_bind_property_full (config, "grid-type",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               clay_transform_grid_options_sync_grid,
                               NULL,
                               NULL, NULL);

  if (tool_options->tool_info->tool_type == CLAY_TYPE_ROTATE_TOOL)
    {
      gchar *label;

      label = g_strdup_printf (_("15 degrees (%s)"),
                               clay_get_mod_string (extend_mask));

      button = clay_prop_check_button_new (config, "constrain-rotate", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      clay_help_set_help_data (button, _("Limit rotation steps to 15 degrees"),
                               NULL);

      g_free (label);
    }
  else if (tool_options->tool_info->tool_type == CLAY_TYPE_SCALE_TOOL)
    {
      gchar *label;

      label = g_strdup_printf (_("Keep aspect (%s)"),
                               clay_get_mod_string (extend_mask));

      button = clay_prop_check_button_new (config, "constrain-scale", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      clay_help_set_help_data (button, _("Keep the original aspect ratio"),
                               NULL);

      g_free (label);

      label = g_strdup_printf (_("Around center (%s)"),
                               clay_get_mod_string (constrain_mask));

      button = clay_prop_check_button_new (config, "frompivot-scale", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      clay_help_set_help_data (button, _("Scale around the center point"),
                               NULL);

      g_free (label);
    }
  else if (tool_options->tool_info->tool_type == CLAY_TYPE_PERSPECTIVE_TOOL)
    {
      gchar *label;

      label = g_strdup_printf (_("Constrain handles (%s)"),
                               clay_get_mod_string (extend_mask));

      button = clay_prop_check_button_new (config, "constrain-perspective", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      clay_help_set_help_data (
        button, _("Constrain handles to move along edges and diagonal (%s)"),
        NULL);

      g_free (label);

      label = g_strdup_printf (_("Around center (%s)"),
                               clay_get_mod_string (constrain_mask));

      button = clay_prop_check_button_new (config, "frompivot-perspective", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      clay_help_set_help_data (
        button, _("Transform around the center point"),
        NULL);

      g_free (label);
    }
  else if (tool_options->tool_info->tool_type == CLAY_TYPE_UNIFIED_TRANSFORM_TOOL)
    {
      struct
      {
        GdkModifierType  mod;
        gchar           *name;
        gchar           *desc;
        gchar           *tip;
      }
      opt_list[] =
      {
        { extend_mask, NULL, N_("Constrain (%s)") },
        { extend_mask, "constrain-move", N_("Move"),
          N_("Constrain movement to 45 degree angles from center (%s)") },
        { extend_mask, "constrain-scale", N_("Scale"),
          N_("Maintain aspect ratio when scaling (%s)") },
        { extend_mask, "constrain-rotate", N_("Rotate"),
          N_("Constrain rotation to 15 degree increments (%s)") },
        { extend_mask, "constrain-shear", N_("Shear"),
          N_("Shear along edge direction only (%s)") },
        { extend_mask, "constrain-perspective", N_("Perspective"),
          N_("Constrain perspective handles to move along edges and diagonal (%s)") },

        { constrain_mask, NULL,
          N_("From pivot  (%s)") },
        { constrain_mask, "frompivot-scale", N_("Scale"),
          N_("Scale from pivot point (%s)") },
        { constrain_mask, "frompivot-shear", N_("Shear"),
          N_("Shear opposite edge by same amount (%s)") },
        { constrain_mask, "frompivot-perspective", N_("Perspective"),
          N_("Maintain position of pivot while changing perspective (%s)") },

        { 0, NULL,
          N_("Pivot") },
        { extend_mask, "cornersnap", N_("Snap (%s)"),
          N_("Snap pivot to corners and center (%s)") },
        { 0, "fixedpivot", N_("Lock"),
          N_("Lock pivot position to canvas") },
      };

      gchar *label;
      gint   i;

      frame = NULL;

      for (i = 0; i < G_N_ELEMENTS (opt_list); i++)
        {
          if (! opt_list[i].name && ! opt_list[i].desc)
            {
              frame = NULL;
              continue;
            }

          label = g_strdup_printf (gettext (opt_list[i].desc),
                                   clay_get_mod_string (opt_list[i].mod));

          if (opt_list[i].name)
            {
              button = clay_prop_check_button_new (config, opt_list[i].name,
                                                   label);
              gtk_box_pack_start (GTK_BOX (frame ? grid_box : vbox),
                                  button, FALSE, FALSE, 0);

              g_free (label);
              label = g_strdup_printf (gettext (opt_list[i].tip),
                                       clay_get_mod_string (opt_list[i].mod));

              clay_help_set_help_data (button, label, NULL);
            }
          else
            {
              frame = clay_frame_new (label);
              gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
              gtk_widget_set_visible (frame, TRUE);

              grid_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
              gtk_container_add (GTK_CONTAINER (frame), grid_box);
              gtk_widget_set_visible (grid_box, TRUE);
            }

          g_free (label);
        }
    }

  return vbox;
}

gboolean
clay_transform_grid_options_show_preview (ClayTransformGridOptions *options)
{
  ClayTransformOptions *transform_options;

  g_return_val_if_fail (CLAY_IS_TRANSFORM_GRID_OPTIONS (options), FALSE);

  transform_options = CLAY_TRANSFORM_OPTIONS (options);

  if (options->show_preview)
    {
      switch (transform_options->type)
        {
        case CLAY_TRANSFORM_TYPE_LAYER:
        case CLAY_TRANSFORM_TYPE_IMAGE:
          return TRUE;

        case CLAY_TRANSFORM_TYPE_SELECTION:
        case CLAY_TRANSFORM_TYPE_PATH:
          return FALSE;
        }
    }

  return FALSE;
}


/*  private functions  */

static gboolean
clay_transform_grid_options_sync_grid (GBinding     *binding,
                                       const GValue *source_value,
                                       GValue       *target_value,
                                       gpointer      user_data)
{
  ClayGuidesType type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == CLAY_GUIDES_N_LINES ||
                       type == CLAY_GUIDES_SPACING);

  return TRUE;
}

