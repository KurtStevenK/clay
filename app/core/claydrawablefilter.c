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

/* This file contains the code necessary for generating on canvas
 * previews, by connecting a specified GEGL operation to do the
 * processing. It uses drawable filters that allow for non-destructive
 * manipulation of drawable data, with live preview on screen.
 *
 * To create a tool that uses this, see app/tools/clayfiltertool.c for
 * the interface and e.g. app/tools/claycolorbalancetool.c for an
 * example of using that interface.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <gegl-paramspecs.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "operations/clay-operation-config.h"

#include "gegl/clay-babl.h"
#include "gegl/clayapplicator.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "clay-utils.h"
#include "claychannel.h"
#include "claycontext.h"
#include "claydrawable-filters.h"
#include "claydrawablefilter.h"
#include "claydrawablefiltermask.h"
#include "clayerror.h"
#include "claygradient.h"
#include "clayidtable.h"
#include "clayimage.h"
#include "claylayer.h"
#include "claylist.h"
#include "clayprogress.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-intl.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_DRAWABLE,
  PROP_MASK,
  PROP_CUSTOM_NAME,
  PROP_TEMPORARY,
  PROP_TO_BE_MERGED,
  N_PROPS
};


struct _ClayDrawableFilter
{
  ClayFilter              parent_instance;

  gint                    ID;

  ClayDrawable           *drawable;
  ClayDrawableFilterMask *mask;
  GeglNode               *operation;

  gboolean                has_input;
  gboolean                has_custom_name;

  gboolean                clip;
  ClayFilterRegion        region;
  gboolean                crop_enabled;
  GeglRectangle           crop_rect;
  gboolean                preview_enabled;
  gboolean                preview_split_enabled;
  ClayAlignmentType       preview_split_alignment;
  gint                    preview_split_position;
  gdouble                 opacity;
  ClayLayerMode           paint_mode;
  ClayLayerColorSpace     blend_space;
  ClayLayerColorSpace     composite_space;
  ClayLayerCompositeMode  composite_mode;
  gboolean                add_alpha;
  gboolean                color_managed;

  gboolean                override_constraints;

  GeglRectangle           filter_area;
  gboolean                filter_clip;

  GeglNode               *translate;
  GeglNode               *crop_before;
  GeglNode               *crop_after;
  ClayApplicator         *applicator;

  gboolean                temporary;
  /* This is mirroring merge_filter option of ClayFilterOptions. */
  gboolean                to_be_merged;
};


static void       clay_savable_iface_init                    (ClaySavableInterface *iface);

static void       clay_drawable_filter_set_property          (GObject              *object,
                                                              guint                 property_id,
                                                              const GValue         *value,
                                                              GParamSpec           *pspec);

static void       clay_drawable_filter_get_property          (GObject              *object,
                                                              guint                 property_id,
                                                              GValue               *value,
                                                              GParamSpec           *pspec);
static void       clay_drawable_filter_dispose               (GObject              *object);
static void       clay_drawable_filter_finalize              (GObject              *object);

static void       clay_drawable_filter_save                  (ClaySavable          *savable,
                                                              ClaySaveState        *state);
static gboolean   clay_drawable_filter_load_enter            (ClayLoadState        *state,
                                                              const gchar         **attribute_names,
                                                              const gchar         **attribute_values,
                                                              gpointer              user_data,
                                                              GError              **error);
static gboolean   clay_drawable_filter_load_exit             (ClayLoadState        *state,
                                                              const gchar          *text,
                                                              gsize                 len,
                                                              gpointer              user_data,
                                                              GError              **error);

static void       clay_drawable_filter_set_operation         (ClayDrawableFilter   *filter,
                                                              GeglNode             *operation);

static void       clay_drawable_filter_sync_active           (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_clip             (ClayDrawableFilter   *filter,
                                                              gboolean              sync_region);
static void       clay_drawable_filter_sync_region           (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_crop             (ClayDrawableFilter   *filter,
                                                              gboolean              old_crop_enabled,
                                                              const GeglRectangle  *old_crop_rect,
                                                              gboolean              old_preview_split_enabled,
                                                              ClayAlignmentType     old_preview_split_alignment,
                                                              gint                  old_preview_split_position,
                                                              gboolean              update);
static void       clay_drawable_filter_sync_opacity          (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_mode             (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_affect           (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_format           (ClayDrawableFilter   *filter);
static void       clay_drawable_filter_sync_mask             (ClayDrawableFilter   *filter);

static gboolean   clay_drawable_filter_is_added              (ClayDrawableFilter   *filter);
static gboolean   clay_drawable_filter_is_active             (ClayDrawableFilter   *filter);
static gboolean   clay_drawable_filter_add_filter            (ClayDrawableFilter   *filter);
static gboolean   clay_drawable_filter_remove_filter         (ClayDrawableFilter   *filter);

static void       clay_drawable_filter_update_drawable       (ClayDrawableFilter   *filter,
                                                              const GeglRectangle  *area);

static void       clay_drawable_filter_affect_changed        (ClayImage            *image,
                                                              ClayChannelType       channel,
                                                              ClayDrawableFilter   *filter);
static void       clay_drawable_filter_mask_changed          (ClayImage            *image,
                                                              ClayDrawableFilter   *filter);
static void       clay_drawable_filter_lock_position_changed (ClayDrawable         *drawable,
                                                              ClayDrawableFilter   *filter);
static void       clay_drawable_filter_format_changed        (ClayDrawable         *drawable,
                                                              ClayDrawableFilter   *filter);
static void       clay_drawable_filter_drawable_removed      (ClayDrawable         *drawable,
                                                              ClayDrawableFilter   *filter);
static void       clay_drawable_filter_lock_alpha_changed    (ClayLayer            *layer,
                                                              ClayDrawableFilter   *filter);

static void       clay_drawable_filter_reorder               (ClayFilterStack      *stack,
                                                              ClayDrawableFilter   *reordered_filter,
                                                              gint                  old_index,
                                                              gint                  new_index,
                                                              ClayDrawableFilter   *filter);

static gboolean   clay_drawable_filter_enter_operation       (ClayLoadState        *state,
                                                              const gchar         **attribute_names,
                                                              const gchar         **attribute_values,
                                                              gpointer              user_data,
                                                              GError              **error);
static gboolean   clay_drawable_filter_exit_operation        (ClayLoadState        *state,
                                                              const gchar          *text,
                                                              gsize                 len,
                                                              gpointer              user_data,
                                                              GError              **error);
static gboolean   clay_drawable_filter_enter_mask            (ClayLoadState        *state,
                                                              const gchar         **attribute_names,
                                                              const gchar         **attribute_values,
                                                              gpointer              user_data,
                                                              GError              **error);
static gboolean   clay_drawable_filter_enter_argument        (ClayLoadState        *state,
                                                              const gchar         **attribute_names,
                                                              const gchar         **attribute_values,
                                                              gpointer              user_data,
                                                              GError              **error);
static gboolean   clay_drawable_filter_exit_argument         (ClayLoadState        *state,
                                                              const gchar          *text,
                                                              gsize                 len,
                                                              gpointer              user_data,
                                                              GError              **error);


G_DEFINE_TYPE_WITH_CODE (ClayDrawableFilter, clay_drawable_filter, CLAY_TYPE_FILTER,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_savable_iface_init))

#define parent_class clay_drawable_filter_parent_class

static guint       drawable_filter_signals[LAST_SIGNAL] = { 0, };
static GParamSpec *drawable_filter_props[N_PROPS]       = { NULL, };


static void
clay_drawable_filter_class_init (ClayDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  drawable_filter_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableFilterClass, flush),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property = clay_drawable_filter_set_property;
  object_class->get_property = clay_drawable_filter_get_property;
  object_class->dispose      = clay_drawable_filter_dispose;
  object_class->finalize     = clay_drawable_filter_finalize;

  drawable_filter_props[PROP_ID]       = g_param_spec_int ("id", NULL, NULL,
                                                           0, G_MAXINT, 0,
                                                           CLAY_PARAM_READABLE);

  drawable_filter_props[PROP_DRAWABLE] = g_param_spec_object ("drawable", NULL, NULL,
                                                              CLAY_TYPE_DRAWABLE,
                                                              CLAY_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
  /* The mask is in fact a ClayDrawableFilterMask but the property is a
   * ClayDrawable to allow setting any drawable. The set_property() code
   * will take care of creating a new object of the proper type.
   */
  drawable_filter_props[PROP_MASK]     = g_param_spec_object ("mask",
                                                              NULL, NULL,
                                                              CLAY_TYPE_DRAWABLE,
                                                              CLAY_PARAM_READWRITE);

  drawable_filter_props[PROP_CUSTOM_NAME] = g_param_spec_boolean ("custom-name",
                                                                  NULL, NULL,
                                                                  FALSE,
                                                                  CLAY_PARAM_READWRITE);

  drawable_filter_props[PROP_TEMPORARY] = g_param_spec_boolean ("temporary",
                                                                NULL, NULL,
                                                                FALSE,
                                                                CLAY_PARAM_READWRITE);

  drawable_filter_props[PROP_TO_BE_MERGED] = g_param_spec_boolean ("to-be-merged",
                                                                   NULL, NULL,
                                                                   FALSE,
                                                                   CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, drawable_filter_props);
}

static void
clay_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "filter";
  iface->save       = clay_drawable_filter_save;
  iface->load_enter = clay_drawable_filter_load_enter;
  iface->load_exit  = clay_drawable_filter_load_exit;
}

static void
clay_drawable_filter_init (ClayDrawableFilter *drawable_filter)
{
  drawable_filter->clip                    = TRUE;
  drawable_filter->region                  = CLAY_FILTER_REGION_SELECTION;
  drawable_filter->preview_enabled         = TRUE;
  drawable_filter->preview_split_enabled   = FALSE;
  drawable_filter->preview_split_alignment = CLAY_ALIGN_LEFT;
  drawable_filter->preview_split_position  = 0;
  drawable_filter->opacity                 = CLAY_OPACITY_OPAQUE;
  drawable_filter->paint_mode              = CLAY_LAYER_MODE_REPLACE;
  drawable_filter->blend_space             = CLAY_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_space         = CLAY_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode          = CLAY_LAYER_COMPOSITE_AUTO;
}

static void
clay_drawable_filter_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayDrawableFilter     *filter = CLAY_DRAWABLE_FILTER (object);
  ClayDrawableFilterMask *mask   = NULL;
  ClayImage              *image;

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_set_object (&filter->drawable, g_value_get_object (value));
      break;

    case PROP_MASK:
      image = clay_item_get_image (CLAY_ITEM (filter->drawable));

      if (g_value_get_object (value) != NULL)
        mask = CLAY_DRAWABLE_FILTER_MASK (clay_item_convert (CLAY_ITEM (g_value_get_object (value)),
                                                             image, CLAY_TYPE_DRAWABLE_FILTER_MASK));
      g_set_object (&filter->mask, mask);
      g_clear_object (&mask);

      if (filter->mask)
        {
          clay_drawable_filter_mask_set_filter (filter->mask, filter);
          clay_drawable_filter_sync_mask (filter);
        }
      break;

    case PROP_CUSTOM_NAME:
      filter->has_custom_name = g_value_get_boolean (value);
      break;

    case PROP_TEMPORARY:
      filter->temporary = g_value_get_boolean (value);
      break;

    case PROP_TO_BE_MERGED:
      filter->to_be_merged = g_value_get_boolean (value);
      clay_drawable_filter_sync_format (filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_filter_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayDrawableFilter *filter = CLAY_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, filter->ID);
      break;
    case PROP_MASK:
      g_value_set_object (value, clay_drawable_filter_get_mask (filter));
      break;
    case PROP_CUSTOM_NAME:
      g_value_set_boolean (value, filter->has_custom_name);
      break;
    case PROP_TEMPORARY:
      g_value_set_boolean (value, filter->temporary);
      break;
    case PROP_TO_BE_MERGED:
      g_value_set_boolean (value, filter->to_be_merged);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_filter_dispose (GObject *object)
{
  ClayDrawableFilter *drawable_filter = CLAY_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    clay_drawable_filter_remove_filter (drawable_filter);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_drawable_filter_finalize (GObject *object)
{
  ClayDrawableFilter *drawable_filter = CLAY_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (drawable_filter->drawable));

      clay_id_table_remove (image->clay->drawable_filter_table, drawable_filter->ID);
    }

  g_clear_object (&drawable_filter->applicator);
  g_clear_object (&drawable_filter->drawable);
  g_clear_object (&drawable_filter->operation);
  g_clear_object (&drawable_filter->mask);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_drawable_filter_save (ClaySavable   *savable,
                           ClaySaveState *state)
{
  ClayDrawableFilter      *filter = CLAY_DRAWABLE_FILTER (savable);
  ClayDrawableFilterMask  *mask;
  GeglNode                *node;
  gchar                   *operation;
  const gchar             *op_version;
  GParamSpec             **pspecs;
  guint                    n_pspecs;
  gchar                   *name;
  gchar                   *icon;
  GEnumClass              *enum_class;
  GEnumValue              *enum_value;
  gboolean                 has_custom_name;
  ClayLayerMode            mode;
  ClayLayerColorSpace      space;
  ClayLayerCompositeMode   composite_mode;
  ClayFilterRegion         region;

  node = clay_drawable_filter_get_operation (filter);

  gegl_node_get (node,
                 "operation", &operation,
                 NULL);

  g_object_get (filter,
                "name",        &name,
                "icon-name",   &icon,
                "custom-name", &has_custom_name,
                NULL);

  clay_savable_print_element_start (state, "filter", has_custom_name ? "name" : NULL, "%s", name, NULL);

  if (icon && *icon)
    clay_savable_print_element (state, "icon", "%s", icon, NULL);

  op_version = gegl_operation_get_op_version (operation);
  clay_savable_print_element_start (state, "operation",
                                    "name",    "%s", operation,
                                    "version", "%s", op_version,
                                    NULL);
  pspecs = gegl_operation_list_properties (operation, &n_pspecs);
  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec  = pspecs[i];
      GValue      value  = G_VALUE_INIT;
      gchar      *strval = NULL;

      g_value_init (&value, pspec->value_type);
      gegl_node_get_property (node, pspec->name, &value);

      switch (G_VALUE_TYPE (&value))
        {
          case G_TYPE_INT:
            strval = g_strdup_printf ("%d", g_value_get_int (&value));
            break;

          case G_TYPE_UINT:
            strval = g_strdup_printf ("%u", g_value_get_uint (&value));
            break;

          case G_TYPE_BOOLEAN:
            strval = g_strdup_printf ("%s", g_value_get_boolean (&value) ? "true" : "false");
            break;

          case G_TYPE_FLOAT:
          case G_TYPE_DOUBLE:
            strval = g_strdup_printf ("%f", g_value_get_double (&value));
            break;

          case G_TYPE_STRING:
            strval = g_strdup (g_value_get_string (&value));
            break;

          default:
            if (g_type_is_a (G_VALUE_TYPE (&value), CLAY_TYPE_CONTEXT))
              {
                /* No need to save the context - we'll set it on load */
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), CLAY_TYPE_GRADIENT))
              {
                clay_savable_print_element_start (state, "argument",
                                                  "name", "%s", pspec->name,
                                                  "type", "%s", g_type_name (G_VALUE_TYPE (&value)),
                                                  NULL);
                clay_savable_save (g_value_get_object (&value), state);
                clay_savable_print_element_end (state, "argument");
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), CLAY_TYPE_CONFIG))
              {
                clay_savable_print_element_start (state, "argument",
                                                  "name", "%s", pspec->name,
                                                  "type", "%s", g_type_name (G_VALUE_TYPE (&value)),
                                                  NULL);
                clay_savable_config_save (g_value_get_object (&value),
                                          g_type_name (G_VALUE_TYPE (&value)),
                                          state);
                clay_savable_print_element_end (state, "argument");
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), G_TYPE_ENUM))
              {
                enum_class = g_type_class_ref (G_VALUE_TYPE (&value));
                if (enum_class)
                  {
                    enum_value = g_enum_get_value (enum_class, g_value_get_enum (&value));
                    strval = g_strdup (enum_value->value_nick);
                    g_type_class_unref (enum_class);
                  }
                else
                  {
                    strval = g_strdup_printf ("%d", g_value_get_enum (&value));
                  }
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), GEGL_TYPE_COLOR))
              {
                clay_savable_print_element_start (state, "argument",
                                                  "name", "%s", pspec->name,
                                                  "type", "%s", g_type_name (G_VALUE_TYPE (&value)),
                                                  NULL);
                clay_savable_color_save (g_value_get_object (&value), NULL, NULL, state);
                clay_savable_print_element_end (state, "argument");
              }
            else
              {
                ClayImage *image = clay_item_get_image (CLAY_ITEM (filter->drawable));

                clay_message (image->clay, NULL,
                              CLAY_MESSAGE_WARNING,
                              "XCF Warning: argument \"%s\" of filter %s has "
                              "unsupported type %s. It was discarded.",
                              pspec->name, operation,
                              g_type_name (G_VALUE_TYPE (&value)));
              }
            break;
        }
      if (strval)
        clay_savable_print_element (state, "argument", "%s", strval,
                                    "name", "%s", pspec->name,
                                    "type", "%s", g_type_name (G_VALUE_TYPE (&value)),
                                    NULL);

      g_value_unset (&value);
      g_free (strval);
    }
  clay_savable_print_element_end (state, "operation");

  if ((mask = clay_drawable_filter_get_mask (filter)))
    {
      clay_savable_print_element_start (state, "mask", NULL);
      clay_savable_save (CLAY_SAVABLE (mask), state);
      clay_savable_print_element_end (state, "mask");
    }

  clay_savable_print_element (state, "visible", "%b",
                              clay_filter_get_active (CLAY_FILTER (filter)),
                              NULL);
  clay_savable_print_element (state, "opacity", "%f",
                              clay_drawable_filter_get_opacity (CLAY_DRAWABLE_FILTER (filter)),
                              NULL);

  mode = clay_drawable_filter_get_paint_mode (CLAY_DRAWABLE_FILTER (filter));
  clay_savable_print_element (state, "mode", "%[ClayLayerMode]", mode, NULL);

  space = clay_drawable_filter_get_blend_space (CLAY_DRAWABLE_FILTER (filter));
  clay_savable_blend_space_save (space, mode, state);

  space = clay_drawable_filter_get_composite_space (CLAY_DRAWABLE_FILTER (filter));
  clay_savable_composite_space_save (space, mode, state);

  composite_mode = clay_drawable_filter_get_composite_mode (filter);
  clay_savable_composite_mode_save (composite_mode, mode, state);

  clay_savable_print_element (state, "clip", "%b",
                              clay_drawable_filter_get_clip (filter),
                              NULL);

  region = clay_drawable_filter_get_region (filter);
  clay_savable_print_element (state, "region", "%[ClayFilterRegion]", region, NULL);

  clay_savable_print_element_end (state, "filter");

  g_free (name);
  g_free (icon);
  g_free (operation);
  g_free (pspecs);
}

static gboolean
clay_drawable_filter_load_enter (ClayLoadState  *state,
                                 const gchar   **attribute_names,
                                 const gchar   **attribute_values,
                                 gpointer        user_data,
                                 GError        **error)
{
  ClayDrawableFilter *filter;
  ClayDrawable       *drawable;
  const gchar        *name = NULL;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (clay_savable_load_peek_active_object (state)), FALSE);

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        name = *attribute_values;
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  drawable = CLAY_DRAWABLE (clay_savable_load_peek_active_object (state));
  filter   = g_object_new (CLAY_TYPE_DRAWABLE_FILTER,
                           "name",     name,
                           "drawable", drawable,
                           "mask",     NULL,
                           NULL);
  filter->applicator = clay_applicator_new (clay_filter_get_node (CLAY_FILTER (filter)));
  clay_savable_load_push_active_object (state, G_OBJECT (filter));

  clay_savable_load_add_simple_handler (state, "icon", G_TYPE_STRING,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);
  clay_savable_load_add_handlers (state, "operation",
                                  clay_drawable_filter_enter_operation,
                                  clay_drawable_filter_exit_operation,
                                  NULL, NULL);
  clay_savable_load_add_handlers (state, "mask",
                                  clay_drawable_filter_enter_mask,
                                  NULL, NULL, NULL);
  clay_savable_load_add_simple_handler (state, "visible", G_TYPE_BOOLEAN,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "opacity", G_TYPE_DOUBLE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "mode", CLAY_TYPE_LAYER_MODE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "blend-space", CLAY_TYPE_LAYER_COLOR_SPACE,
                                        NULL, NULL, NULL, FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN, NULL);
  clay_savable_load_add_simple_handler (state, "composite-space", CLAY_TYPE_LAYER_COLOR_SPACE,
                                        NULL, NULL, NULL, FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN, NULL);
  clay_savable_load_add_simple_handler (state, "composite-mode", CLAY_TYPE_LAYER_COMPOSITE_MODE,
                                        NULL, NULL, NULL, FALSE, FALSE,
                                        "auto", G_TYPE_BOOLEAN, NULL);
  clay_savable_load_add_simple_handler (state, "clip", G_TYPE_BOOLEAN,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  CLAY_TYPE_FILTER_REGION;
  clay_savable_load_add_simple_handler (state, "region", CLAY_TYPE_FILTER_REGION,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);

  return TRUE;
}

static gboolean
clay_drawable_filter_load_exit (ClayLoadState  *state,
                                const gchar    *text,
                                gsize           len,
                                gpointer        user_data,
                                GError        **error)
{
  ClayDrawableFilter     *filter;
  GeglNode               *operation       = NULL;
  const gchar            *icon            = NULL;
  ClayLayerMode           mode;
  ClayLayerColorSpace     blend_space     = CLAY_LAYER_COLOR_SPACE_AUTO;
  ClayLayerColorSpace     composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
  ClayLayerCompositeMode  composite_mode  = CLAY_LAYER_COMPOSITE_AUTO;
  gdouble                 opacity         = 1.0;
  gboolean                visible         = TRUE;
  gboolean                clip            = TRUE;
  ClayFilterRegion        region          = CLAY_FILTER_REGION_SELECTION;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (clay_savable_load_peek_active_object (state)), FALSE);

  filter = CLAY_DRAWABLE_FILTER (clay_savable_load_pop_active_object (state));

  if (! clay_savable_load_get_values (state,
                                      "operation", &operation,
                                      NULL))
    {
      g_object_unref (filter);
      return TRUE;
    }

  clay_drawable_filter_set_operation (filter, operation);

  CLAY_TYPE_FILTER_REGION;
  clay_savable_load_get_values (state,
                                "icon",    &icon,
                                "visible", &visible,
                                "opacity", &opacity,
                                "clip",    &clip,
                                "region",  &region,
                                NULL);

  clay_viewable_set_icon_name (CLAY_VIEWABLE (filter), icon);
  clay_drawable_filter_set_opacity (filter, opacity);
  clay_filter_set_active (CLAY_FILTER (filter), visible);
  clay_drawable_filter_set_clip (filter, clip);
  clay_drawable_filter_set_region (filter, region);

  if (clay_savable_load_get_values (state,
                                    "mode",            &mode,
                                    "blend-space",     &blend_space,
                                    "composite-space", &composite_space,
                                    "composite-mode",  &composite_mode,
                                    NULL))
    clay_drawable_filter_set_mode (filter, mode, blend_space,
                                   composite_space, composite_mode);
  else
    g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                 "%s: blend and composite information missing.",
                 G_STRFUNC);

  clay_drawable_filter_apply_with_mask (filter, CLAY_CHANNEL (filter->mask), NULL);
  clay_drawable_filter_commit (filter, TRUE, NULL, FALSE);
  clay_drawable_filter_layer_mask_freeze (filter);

  g_object_unref (filter);

  return TRUE;
}

ClayDrawableFilter *
clay_drawable_filter_new (ClayDrawable *drawable,
                          const gchar  *undo_desc,
                          GeglNode     *operation,
                          const gchar  *icon_name)
{
  ClayDrawableFilter *filter;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (gegl_node_has_pad (operation, "output"), NULL);

  filter = g_object_new (CLAY_TYPE_DRAWABLE_FILTER,
                         "name",        undo_desc,
                         "icon-name",   icon_name,
                         "drawable",    drawable,
                         "mask",        NULL,
                         NULL);

  clay_drawable_filter_set_operation (filter, operation);

  return filter;
}

ClayDrawableFilter *
clay_drawable_filter_duplicate (ClayDrawable       *drawable,
                                ClayDrawableFilter *prior_filter)
{
  ClayImage          *image;
  ClayDrawableFilter *filter;
  GeglNode           *prior_node;
  GeglNode           *node = gegl_node_new ();
  gchar              *operation;
  const gchar        *undo_desc;
  const gchar        *icon_name;
  GParamSpec        **pspecs;
  guint               n_pspecs;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (prior_filter), NULL);

  prior_node = clay_drawable_filter_get_operation (prior_filter);

  /* Don't duplicate tool-based filters (for now) or temporary filters */
  if (clay_drawable_filter_get_mask (prior_filter) == NULL ||
      prior_node                                   == NULL ||
      ! strcmp (gegl_node_get_operation (prior_node), "GraphNode"))
    return NULL;

  g_object_get (prior_filter,
                "name",      &undo_desc,
                "icon-name", &icon_name,
                NULL);

  gegl_node_get (prior_node,
                 "operation", &operation,
                 NULL);

  gegl_node_set (node,
                 "operation", operation,
                 NULL);

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);

  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      gegl_node_get_property (prior_node, pspec->name,
                              &value);

      gegl_node_set_property (node, pspec->name,
                              &value);
      g_value_unset (&value);
    }
  g_free (pspecs);

  filter = clay_drawable_filter_new (drawable, undo_desc, node, icon_name);
  g_object_unref (node);

  clay_drawable_filter_set_clip (filter, prior_filter->clip);
  clay_drawable_filter_set_opacity (filter, prior_filter->opacity);
  clay_drawable_filter_set_mode (filter,
                                 prior_filter->paint_mode,
                                 prior_filter->blend_space,
                                 prior_filter->composite_space,
                                 prior_filter->composite_mode);
  clay_drawable_filter_set_region (filter,
                                   prior_filter->region);
  clay_drawable_filter_set_add_alpha (filter,
                                      prior_filter->add_alpha);
  clay_filter_set_active (CLAY_FILTER (filter),
                          clay_filter_get_active (CLAY_FILTER (prior_filter)));
  clay_filter_set_is_last_node (CLAY_FILTER (filter),
                                clay_filter_get_is_last_node (CLAY_FILTER (prior_filter)));

  image = clay_item_get_image (CLAY_ITEM (drawable));
  if (image != NULL)
    g_object_set (filter,
                  "mask", prior_filter->mask,
                  NULL);

  g_free (operation);

  return filter;
}

gint
clay_drawable_filter_get_id (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), -1);

  return filter->ID;
}

ClayDrawableFilter *
clay_drawable_filter_get_by_id (Clay *clay,
                                gint  filter_id)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->drawable_filter_table == NULL)
    return NULL;

  return (ClayDrawableFilter *) clay_id_table_lookup (clay->drawable_filter_table, filter_id);
}

ClayDrawable *
clay_drawable_filter_get_drawable (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->drawable;
}

GeglNode *
clay_drawable_filter_get_operation (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->operation;
}

ClayDrawableFilterMask *
clay_drawable_filter_get_mask (ClayDrawableFilter  *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->mask;
}

void
clay_drawable_filter_set_temporary (ClayDrawableFilter *filter,
                                    gboolean            temporary)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  temporary = temporary ? TRUE : FALSE;

  if (temporary != filter->temporary)
    {
      g_object_set (filter,
                    "temporary", temporary,
                    NULL);
    }
}

gboolean
clay_drawable_filter_get_temporary (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->temporary;
}

void
clay_drawable_filter_set_opacity (ClayDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      clay_drawable_filter_sync_opacity (filter);

      if (clay_drawable_filter_is_active (filter))
        clay_drawable_filter_update_drawable (filter, NULL);
    }
}

gdouble
clay_drawable_filter_get_opacity (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0.0f);

  return filter->opacity;
}

void
clay_drawable_filter_set_mode (ClayDrawableFilter     *filter,
                               ClayLayerMode           paint_mode,
                               ClayLayerColorSpace     blend_space,
                               ClayLayerColorSpace     composite_space,
                               ClayLayerCompositeMode  composite_mode)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      clay_drawable_filter_sync_mode (filter);

      if (clay_drawable_filter_is_active (filter))
        clay_drawable_filter_update_drawable (filter, NULL);
    }
}

ClayLayerMode
clay_drawable_filter_get_paint_mode (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0);

  return filter->paint_mode;
}

ClayLayerColorSpace
clay_drawable_filter_get_blend_space (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0);

  return filter->blend_space;
}

ClayLayerColorSpace
clay_drawable_filter_get_composite_space (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0);

  return filter->composite_space;
}

ClayLayerCompositeMode
clay_drawable_filter_get_composite_mode (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0);

  return filter->composite_mode;
}

void
clay_drawable_filter_set_clip (ClayDrawableFilter *filter,
                               gboolean            clip)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (clip != filter->clip)
    {
      filter->clip = clip;

      clay_drawable_filter_sync_region (filter);
      clay_drawable_filter_sync_clip (filter, TRUE);
    }
}

gboolean
clay_drawable_filter_get_clip (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->clip;
}

void
clay_drawable_filter_set_region (ClayDrawableFilter *filter,
                                 ClayFilterRegion    region)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (region != filter->region)
    {
      filter->region = region;

      clay_drawable_filter_sync_region (filter);

      if (clay_drawable_filter_is_active (filter))
        clay_drawable_filter_update_drawable (filter, NULL);
    }
}

ClayFilterRegion
clay_drawable_filter_get_region (ClayDrawableFilter  *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), 0);

  return filter->region;
}

void
clay_drawable_filter_set_crop (ClayDrawableFilter  *filter,
                               const GeglRectangle *rect,
                               gboolean             update)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if ((rect != NULL) != filter->crop_enabled ||
      (rect && ! gegl_rectangle_equal (rect, &filter->crop_rect)))
    {
      gboolean      old_enabled = filter->crop_enabled;
      GeglRectangle old_rect    = filter->crop_rect;

      if (rect)
        {
          filter->crop_enabled = TRUE;
          filter->crop_rect    = *rect;
        }
      else
        {
          filter->crop_enabled = FALSE;
        }

      clay_drawable_filter_sync_crop (filter,
                                      old_enabled,
                                      &old_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      update);
    }
}

void
clay_drawable_filter_set_preview (ClayDrawableFilter *filter,
                                  gboolean            enabled)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (enabled != filter->preview_enabled)
    {
      filter->preview_enabled = enabled;

      clay_drawable_filter_sync_active (filter);

      if (clay_drawable_filter_is_added (filter))
        {
          clay_drawable_update_bounding_box (filter->drawable);

          clay_drawable_filter_update_drawable (filter, NULL);
        }
    }
}

void
clay_drawable_filter_set_preview_split (ClayDrawableFilter  *filter,
                                        gboolean             enabled,
                                        ClayAlignmentType    alignment,
                                        gint                 position)
{
  ClayItem *item;

  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (alignment == CLAY_ALIGN_LEFT  ||
                    alignment == CLAY_ALIGN_RIGHT ||
                    alignment == CLAY_ALIGN_TOP   ||
                    alignment == CLAY_ALIGN_BOTTOM);

  item = CLAY_ITEM (filter->drawable);

  switch (alignment)
    {
    case CLAY_ALIGN_LEFT:
    case CLAY_ALIGN_RIGHT:
      position = CLAMP (position, 0, clay_item_get_width (item));
      break;

    case CLAY_ALIGN_TOP:
    case CLAY_ALIGN_BOTTOM:
      position = CLAMP (position, 0, clay_item_get_height (item));
      break;

    default:
      g_return_if_reached ();
    }

  if (enabled   != filter->preview_split_enabled   ||
      alignment != filter->preview_split_alignment ||
      position  != filter->preview_split_position)
    {
      gboolean          old_enabled   = filter->preview_split_enabled;
      ClayAlignmentType old_alignment = filter->preview_split_alignment;
      gint              old_position  = filter->preview_split_position;

      filter->preview_split_enabled   = enabled;
      filter->preview_split_alignment = alignment;
      filter->preview_split_position  = position;

      clay_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      old_enabled,
                                      old_alignment,
                                      old_position,
                                      TRUE);
    }
}

/* This function is **ONLY** for usage by libclay API. The idea is to have
 * a single function which updates a bunch of settings in a single call
 * and in particular a single rendering update.
 *
 * Also it does some funky config object switch for custom operations
 * which is only needed libclay-side.
 */
gboolean
clay_drawable_filter_update (ClayDrawableFilter      *filter,
                             const gchar            **propnames,
                             const ClayValueArray    *values,
                             gdouble                  opacity,
                             ClayLayerMode            paint_mode,
                             ClayLayerColorSpace      blend_space,
                             ClayLayerColorSpace      composite_space,
                             ClayLayerCompositeMode   composite_mode,
                             const gchar            **auxinputnames,
                             ClayDrawable           **auxinputs,
                             GError                 **error)
{
  ClayImage    *image;
  ClayObject   *settings        = NULL;
  GeglNode     *node            = NULL;
  GParamSpec  **pspecs;
  gchar        *opname;
  guint         n_parent_pspecs = 0;
  guint         n_pspecs;
  gint          n_values;
  gint          n_auxinputs;
  gboolean      changed = FALSE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);

  n_values = clay_value_array_length (values);

  if (n_values != g_strv_length ((gchar **) propnames))
    {
      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   "%s: the number of property names and values differ.",
                   G_STRFUNC);
      return FALSE;
    }

  n_auxinputs = 0;
  while (auxinputs[n_auxinputs] != NULL)
    n_auxinputs++;

  if (n_auxinputs != g_strv_length ((gchar **) auxinputnames))
    {
      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   "%s: the number of aux input names and aux inputs differ.",
                   G_STRFUNC);
      return FALSE;
    }

  g_object_freeze_notify (G_OBJECT (filter));

  gegl_node_get (filter->operation, "operation", &opname, NULL);

  image = clay_item_get_image (CLAY_ITEM (filter->drawable));
  node  = clay_drawable_filter_get_operation (filter);
  if (clay_operation_config_is_custom (image->clay, opname))
    {
      GObjectClass *klass;
      GObjectClass *parent_klass;

      gegl_node_get (node,
                     "config", &settings,
                     NULL);
      klass        = G_OBJECT_GET_CLASS (settings);
      parent_klass = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
      g_free (g_object_class_list_properties (parent_klass, &n_parent_pspecs));
      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), &n_pspecs);
    }
  else
    {
      pspecs = gegl_operation_list_properties (opname, &n_pspecs);
    }

  for (gint i = n_parent_pspecs; i < n_pspecs; i++)
    {
      GParamSpec *target_pspec;
      GParamSpec *pspec     = pspecs[i];
      GValue      old_value = G_VALUE_INIT;
      gint        j;

      if (settings)
        target_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), pspec->name);
      else
        target_pspec = gegl_node_find_property (node, pspec->name);
      if (! target_pspec)
        {
          /* If this ever happens, this is more likely a bug in our
           * PDB code, unless someone tried to call the PDB procedure
           * directly with bad data.
           */
          g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                       _("GEGL operation '%s' has been called with a "
                         "non-existent argument name '%s' (#%d)."),
                       opname, pspec->name, i);
          break;
        }

      if (settings)
        g_object_get_property (G_OBJECT (settings), pspec->name, &old_value);
      else
        gegl_node_get_property (node, pspec->name, &old_value);

      for (j = 0; j < n_values; j++)
        if (g_strcmp0 (pspec->name, propnames[j]) == 0)
          break;

      if (j < n_values)
        {
          GValue   *new_value;
          GValue    replaced_value        = G_VALUE_INIT;
          gboolean  new_value_initialized = FALSE;

          new_value = clay_value_array_index (values, j);

          if (GEGL_IS_PARAM_SPEC_ENUM (pspec) && G_VALUE_HOLDS_STRING (new_value))
            {
              /* GEGL enum types are special-cased to be passed as
               * ClayChoice (string) param specs on libclay.
               */
              GeglParamSpecEnum *gespec = GEGL_PARAM_SPEC_ENUM (pspec);
              GEnumClass        *enum_class;
              GEnumValue        *enum_value;
              const gchar       *new_str;

              new_str    = g_value_get_string (new_value);
              enum_class = g_type_class_ref (G_PARAM_SPEC_VALUE_TYPE (pspec));
              for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
                {
                  GSList *iter;

                  if (enum_value->value < enum_class->minimum || enum_value->value > enum_class->maximum)
                    continue;

                  for (iter = gespec->excluded_values; iter; iter = iter->next)
                    if (GPOINTER_TO_INT (iter->data) == enum_value->value)
                      break;

                  if (iter != NULL)
                    /* Excluded value. */
                    continue;

                  if (g_strcmp0 (enum_value->value_nick, new_str) == 0)
                    {
                      g_value_init (&replaced_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
                      g_value_set_enum (&replaced_value, enum_value->value);
                      new_value = &replaced_value;
                      new_value_initialized = TRUE;
                    }
                }

              g_type_class_unref (enum_class);
            }
          else if (! G_TYPE_CHECK_VALUE_TYPE (new_value, G_PARAM_SPEC_VALUE_TYPE (pspec)))
            {
              g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                           _("GEGL operation '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           opname, pspec->name, i,
                           g_type_name (pspec->value_type),
                           g_type_name (G_VALUE_TYPE (new_value)));

              break;
            }

          if (g_param_values_cmp (pspec, new_value, &old_value) != 0)
            {
              if (settings)
                g_object_set_property (G_OBJECT (settings), pspec->name, new_value);
              else
                gegl_node_set_property (node, pspec->name, new_value);
              changed = TRUE;
            }

          if (new_value_initialized)
            g_value_unset (new_value);

          continue;
        }

      if (! g_param_value_defaults (pspec, &old_value))
        {
          GValue default_value = G_VALUE_INIT;

          g_value_init (&default_value, pspec->value_type);
          g_param_value_set_default (pspec, &default_value);
          if (settings)
            g_object_set_property (G_OBJECT (settings), pspec->name, &default_value);
          else
            gegl_node_set_property (node, pspec->name, &default_value);
          changed = TRUE;

          g_value_unset (&default_value);
        }

      g_value_unset (&old_value);
    }

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      clay_drawable_filter_sync_opacity (filter);
      changed = TRUE;
    }

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      clay_drawable_filter_sync_mode (filter);
      changed = TRUE;
    }

  if (*error == NULL)
    {
      for (gint i = 0; auxinputnames[i]; i++)
        {
          GeglNode   *src_node;
          GeglBuffer *buffer;

          if (! gegl_node_has_pad (node, auxinputnames[i]))
            {
              g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                           _("GEGL operation '%s' has been called with an "
                             "invalid aux input name '%s'."),
                           opname, auxinputnames[i]);
              break;
            }


          buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (auxinputs[i]));
          g_object_ref (buffer);
          src_node = gegl_node_new_child (gegl_node_get_parent (node),
                                          "operation", "gegl:buffer-source",
                                          "buffer",    buffer,
                                          NULL);
          g_object_unref (buffer);

          gegl_node_connect (src_node, "output", node, auxinputnames[i]);
        }
    }

  if (settings)
    gegl_node_set (node, "config", settings, NULL);

  g_object_thaw_notify (G_OBJECT (filter));

  g_clear_object (&settings);
  g_free (pspecs);
  g_free (opname);

  if (changed && clay_drawable_filter_is_active (filter))
    clay_drawable_filter_update_drawable (filter, NULL);

  return (*error != NULL);
}

gboolean
clay_drawable_filter_get_add_alpha (ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->add_alpha;
}

void
clay_drawable_filter_set_add_alpha (ClayDrawableFilter *filter,
                                    gboolean            add_alpha)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (add_alpha != filter->add_alpha)
    {
      filter->add_alpha = add_alpha;
      clay_drawable_filter_sync_format (filter);
    }
}

void
clay_drawable_filter_set_override_constraints (ClayDrawableFilter *filter,
                                               gboolean            override_constraints)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (override_constraints != filter->override_constraints)
    {
      filter->override_constraints = override_constraints;

      clay_drawable_filter_sync_affect (filter);
      clay_drawable_filter_sync_format (filter);
      clay_drawable_filter_sync_clip   (filter, TRUE);

      if (clay_drawable_filter_is_active (filter))
        clay_drawable_filter_update_drawable (filter, NULL);
    }
}

const Babl *
clay_drawable_filter_get_format (ClayDrawableFilter *filter)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  format = clay_applicator_get_output_format (filter->applicator);

  if (! format)
    format = clay_drawable_get_format (filter->drawable);

  return format;
}

void
clay_drawable_filter_apply (ClayDrawableFilter  *filter,
                            const GeglRectangle *area)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  clay_drawable_filter_add_filter (filter);

  clay_drawable_filter_sync_clip (filter, TRUE);

  if (clay_drawable_filter_is_active (filter))
    {
      clay_drawable_update_bounding_box (filter->drawable);

      clay_drawable_filter_update_drawable (filter, area);
    }
}

void
clay_drawable_filter_apply_with_mask (ClayDrawableFilter  *filter,
                                      ClayChannel         *mask,
                                      const GeglRectangle *area)
{
  ClayImage   *image     = NULL;
  ClayChannel *selection = NULL;
  GeglBuffer  *buffer    = NULL;

  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (mask)
    {
      image = clay_item_get_image (CLAY_ITEM (filter->drawable));
      selection =
        CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (clay_image_get_mask (image)),
                                           CLAY_TYPE_CHANNEL));
      clay_channel_clear (clay_image_get_mask (image), NULL, FALSE);

      buffer =
        clay_gegl_buffer_dup (clay_drawable_get_buffer (CLAY_DRAWABLE (mask)),
                              CLAY_DRAWABLE (clay_image_get_mask (image)));
      clay_drawable_set_buffer (CLAY_DRAWABLE (clay_image_get_mask (image)),
                                FALSE, NULL, buffer);

      g_object_unref (buffer);
    }

  clay_drawable_filter_apply (filter, area);

  if (mask)
    {
      buffer =
        clay_gegl_buffer_dup (clay_drawable_get_buffer (CLAY_DRAWABLE (selection)),
                              CLAY_DRAWABLE (clay_image_get_mask (image)));
      clay_drawable_set_buffer (CLAY_DRAWABLE (clay_image_get_mask (image)),
                                FALSE, NULL, buffer);
      g_object_unref (buffer);
      g_object_unref (selection);
    }
}

gboolean
clay_drawable_filter_commit (ClayDrawableFilter *filter,
                             gboolean            non_destructive,
                             ClayProgress       *progress,
                             gboolean            cancellable)
{
  gboolean success = TRUE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);

  if (clay_drawable_filter_is_added (filter))
    {
      const Babl *format;

      g_object_add_weak_pointer (G_OBJECT (filter), (gpointer) &filter);
      format = clay_drawable_filter_get_format (filter);

      clay_drawable_filter_set_preview_split (filter, FALSE,
                                              filter->preview_split_alignment,
                                              filter->preview_split_position);
      clay_drawable_filter_set_preview (filter, TRUE);

      /* Only commit if filter is applied destructively */
      if (! non_destructive)
        {
          success = clay_drawable_merge_filter (filter->drawable,
                                                CLAY_FILTER (filter),
                                                progress,
                                                clay_object_get_name (filter),
                                                format,
                                                filter->filter_clip,
                                                cancellable,
                                                FALSE);

          clay_drawable_filter_remove_filter (filter);
        }
      else
        {
          GeglRectangle rect;

          if (clay_viewable_preview_is_frozen (CLAY_VIEWABLE (filter->drawable)))
            clay_viewable_preview_thaw (CLAY_VIEWABLE (filter->drawable));

          /* Update layer tree preview */
          rect = clay_drawable_get_bounding_box (filter->drawable);
          clay_drawable_update (filter->drawable, rect.x, rect.y,
                                rect.width, rect.height);
        }

      if (filter)
        {
          if (! success)
            clay_drawable_filter_update_drawable (filter, NULL);

          g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
          g_object_remove_weak_pointer (G_OBJECT (filter), (gpointer) &filter);
        }
    }

  return success;
}

void
clay_drawable_filter_abort (ClayDrawableFilter *filter)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (clay_drawable_filter_remove_filter (filter))
    {
      clay_drawable_filter_update_drawable (filter, NULL);
    }
}

void
clay_drawable_filter_layer_mask_freeze (ClayDrawableFilter *filter)
{
  ClayImage *image = clay_item_get_image (CLAY_ITEM (filter->drawable));

  if (! filter->mask)
    g_object_set (filter,
                  "mask", CLAY_DRAWABLE (clay_image_get_mask (image)),
                  NULL);

  g_signal_handlers_disconnect_by_func (image,
                                        clay_drawable_filter_mask_changed,
                                        filter);
}

void
clay_drawable_filter_refresh_crop (ClayDrawableFilter *filter,
                                   GeglRectangle      *rect)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  if (rect)
    {
      /* Some filters have built-in width/height properties that limit
       * the buffer size. If they have one in those roles, we can update
       * their size here. */
      GParamSpec *gegl_pspec_width  = NULL;
      GParamSpec *gegl_pspec_height = NULL;

      gegl_pspec_width  = gegl_node_find_property (filter->operation, "width");
      gegl_pspec_height = gegl_node_find_property (filter->operation, "height");

      if (gegl_pspec_width != NULL)
        {
          if (clay_gegl_param_spec_has_key (gegl_pspec_width,
                                            "role",
                                            "output-extent"))
            {
              gegl_node_set (filter->operation, "width", rect->width, NULL);
            }
        }
      if (gegl_pspec_height != NULL)
        {
          if (clay_gegl_param_spec_has_key (gegl_pspec_height,
                                            "role",
                                            "output-extent"))
            {
              gegl_node_set (filter->operation, "height", rect->height, NULL);
            }
        }
      filter->filter_area.width  = rect->width;
      filter->filter_area.height = rect->height;

      clay_drawable_filter_set_clip (filter, TRUE);
      clay_drawable_filter_set_clip (filter, FALSE);
      clay_drawable_filter_set_region (filter, CLAY_FILTER_REGION_SELECTION);
      clay_drawable_filter_set_region (filter, CLAY_FILTER_REGION_DRAWABLE);

      if (filter->crop_enabled)
        {
          clay_drawable_filter_set_crop (filter, NULL, FALSE);
          clay_drawable_filter_set_crop (filter, rect, FALSE);
        }
    }
}

/*  private functions  */

/* All generic code should use clay_drawable_filter_new() only.
 * This separate function exists solely for the purpose of WLBR file
 * loading as we are creating a filter in pieces.
 */
static void
clay_drawable_filter_set_operation (ClayDrawableFilter *filter,
                                    GeglNode           *operation)
{
  ClayImage     *image;
  ClayDrawable  *drawable;
  GeglNode      *node;
  GeglOperation *op;

  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (filter->operation == NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));
  g_return_if_fail (gegl_node_has_pad (operation, "output"));

  op = gegl_node_get_gegl_operation (operation);
  if (op != NULL)
    {
      GeglOperationClass *opclass;
      const gchar        *opname;
      gchar              *name = NULL;

      opclass = GEGL_OPERATION_GET_CLASS (op);
      opname  = gegl_operation_class_get_key (opclass, "title");

      g_object_get (filter, "name", &name, NULL);
      if (name == NULL || strlen (name) == 0)
        g_object_set (filter,
                      "name",        opname,
                      "custom-name", FALSE,
                      NULL);
      else if (g_strcmp0 (name, opname) == 0)
        g_object_set (filter,
                      "custom-name", FALSE,
                      NULL);
      else
        g_object_set (filter,
                      "custom-name", TRUE,
                      NULL);

      g_free (name);
    }

  filter->operation = g_object_ref (operation);

  drawable   = clay_drawable_filter_get_drawable (filter);
  image      = clay_item_get_image (CLAY_ITEM (drawable));
  filter->ID = clay_id_table_insert (image->clay->drawable_filter_table, filter);
  g_object_notify_by_pspec (G_OBJECT (filter), drawable_filter_props[PROP_ID]);

  node = clay_filter_get_node (CLAY_FILTER (filter));

  if (! gegl_node_get_parent (operation))
    {
      gegl_node_add_child (node, operation);
      clay_gegl_node_set_underlying_operation (node, operation);
    }

  if (! filter->applicator)
    filter->applicator = clay_applicator_new (node);

  clay_filter_set_applicator (CLAY_FILTER (filter), filter->applicator);

  clay_applicator_set_cache (filter->applicator, TRUE);

  filter->has_input = gegl_node_has_pad (filter->operation, "input");

  if (filter->has_input)
    {
      GeglNode *input;

      input = gegl_node_get_input_proxy (node, "input");

      filter->translate = gegl_node_new_child (node,
                                               "operation", "gegl:translate",
                                               NULL);

      filter->crop_before = gegl_node_new_child (node,
                                                 "operation", "gegl:crop",
                                                 NULL);

      gegl_node_link_many (input,
                           filter->translate,
                           filter->crop_before,
                           filter->operation,
                           NULL);
    }

  filter->crop_after = gegl_node_new_child (node,
                                            "operation", "gegl:crop",
                                            NULL);

  gegl_node_link_many (filter->operation,
                       filter->crop_after,
                       NULL);

  gegl_node_connect (filter->crop_after, "output", node, "aux");
}

static void
clay_drawable_filter_sync_active (ClayDrawableFilter *filter)
{
  clay_applicator_set_active (filter->applicator, filter->preview_enabled);
}

static void
clay_drawable_filter_sync_clip (ClayDrawableFilter *filter,
                                gboolean            sync_region)
{
  gboolean clip;

  if (filter->override_constraints)
    clip = filter->clip;
  else
    clip = clay_item_get_clip (CLAY_ITEM (filter->drawable), filter->clip);

  if (! clip)
    {
      GeglRectangle bounding_box;

      bounding_box = gegl_node_get_bounding_box (filter->operation);

      if (gegl_rectangle_is_infinite_plane (&bounding_box))
        clip = TRUE;
    }

  if (clip != filter->filter_clip)
    {
      filter->filter_clip = clip;

      if (sync_region)
        clay_drawable_filter_sync_region (filter);
    }
}

static void
clay_drawable_filter_sync_region (ClayDrawableFilter *filter)
{
  ClayContainer *filters;
  gboolean       first_filter = FALSE;

  filters = clay_drawable_get_filters (filter->drawable);

  /* The first test is because the filter might not be added yet. */
  if (CLAY_LIST (filters)->queue->tail != NULL &&
      filter == CLAY_LIST (filters)->queue->tail->data)
    {
      ClayDrawableFilter *next_filter = NULL;

      if (CLAY_LIST (filters)->queue->head->next)
        next_filter = CLAY_LIST (filters)->queue->tail->prev->data;

      if (next_filter)
        /* If the current filter became the first after a reorder, we
         * want to re-sync the next filter which was the first filter
         * just before.
         */
        clay_drawable_filter_sync_region (next_filter);

      first_filter = TRUE;
    }

  if (filter->region == CLAY_FILTER_REGION_SELECTION)
    {
      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) -filter->filter_area.x,
                         "y", (gdouble) -filter->filter_area.y,
                         NULL);

          if (first_filter)
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:crop",
                           "width",     (gdouble) filter->filter_area.width,
                           "height",    (gdouble) filter->filter_area.height,
                           NULL);
          else
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:nop",
                           NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         0.0,
                         "y",         0.0,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      clay_applicator_set_apply_offset (filter->applicator,
                                        filter->filter_area.x,
                                        filter->filter_area.y);
    }
  else
    {
      ClayItem *item   = CLAY_ITEM (filter->drawable);
      gdouble   width  = clay_item_get_width (item);
      gdouble   height = clay_item_get_height (item);

      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) 0.0,
                         "y", (gdouble) 0.0,
                         NULL);

          if (first_filter)
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:crop",
                           "width",     width,
                           "height",    height,
                           NULL);
          else
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:nop",
                           NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         (gdouble) filter->filter_area.x,
                         "y",         (gdouble) filter->filter_area.y,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      clay_applicator_set_apply_offset (filter->applicator, 0, 0);
    }

  if (clay_drawable_filter_is_active (filter))
    {
      if (clay_drawable_update_bounding_box (filter->drawable))
        g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static gboolean
clay_drawable_filter_get_crop_rect (ClayDrawableFilter  *filter,
                                    gboolean             crop_enabled,
                                    const GeglRectangle *crop_rect,
                                    gboolean             preview_split_enabled,
                                    ClayAlignmentType    preview_split_alignment,
                                    gint                 preview_split_position,
                                    GeglRectangle       *rect)
{
  GeglRectangle bounds;
  gint          x1, x2;
  gint          y1, y2;

  bounds = gegl_rectangle_infinite_plane ();

  x1 = bounds.x;
  x2 = bounds.x + bounds.width;

  y1 = bounds.y;
  y2 = bounds.y + bounds.height;

  if (preview_split_enabled)
    {
      switch (preview_split_alignment)
        {
        case CLAY_ALIGN_LEFT:
          x2 = preview_split_position;
          break;

        case CLAY_ALIGN_RIGHT:
          x1 = preview_split_position;
          break;

        case CLAY_ALIGN_TOP:
          y2 = preview_split_position;
          break;

        case CLAY_ALIGN_BOTTOM:
          y1 = preview_split_position;
          break;

        default:
          g_return_val_if_reached (FALSE);
        }
    }

  gegl_rectangle_set (rect, x1, y1, x2 - x1, y2 - y1);

  if (crop_enabled)
    gegl_rectangle_intersect (rect, rect, crop_rect);

  return ! gegl_rectangle_equal (rect, &bounds);
}

static void
clay_drawable_filter_sync_crop (ClayDrawableFilter  *filter,
                                gboolean             old_crop_enabled,
                                const GeglRectangle *old_crop_rect,
                                gboolean             old_preview_split_enabled,
                                ClayAlignmentType    old_preview_split_alignment,
                                gint                 old_preview_split_position,
                                gboolean             update)
{
  GeglRectangle old_rect;
  GeglRectangle new_rect;
  gboolean      enabled;

  clay_drawable_filter_get_crop_rect (filter,
                                      old_crop_enabled,
                                      old_crop_rect,
                                      old_preview_split_enabled,
                                      old_preview_split_alignment,
                                      old_preview_split_position,
                                      &old_rect);

  enabled = clay_drawable_filter_get_crop_rect (filter,
                                                filter->crop_enabled,
                                                &filter->crop_rect,
                                                filter->preview_split_enabled,
                                                filter->preview_split_alignment,
                                                filter->preview_split_position,
                                                &new_rect);

  clay_applicator_set_crop (filter->applicator, enabled ? &new_rect : NULL);

  if (update                                  &&
      clay_drawable_filter_is_active (filter) &&
      ! gegl_rectangle_equal (&old_rect, &new_rect))
    {
      GeglRectangle diff_rects[4];
      gint          n_diff_rects;
      gint          i;

      clay_drawable_update_bounding_box (filter->drawable);

      n_diff_rects = gegl_rectangle_xor (diff_rects, &old_rect, &new_rect);

      for (i = 0; i < n_diff_rects; i++)
        clay_drawable_filter_update_drawable (filter, &diff_rects[i]);
    }
}

static void
clay_drawable_filter_sync_opacity (ClayDrawableFilter *filter)
{
  clay_applicator_set_opacity (filter->applicator,
                               filter->opacity);
}

static void
clay_drawable_filter_sync_mode (ClayDrawableFilter *filter)
{
  ClayLayerMode paint_mode = filter->paint_mode;

  if (! filter->has_input && paint_mode == CLAY_LAYER_MODE_REPLACE)
    {
      /* if the filter's op has no input, use NORMAL instead of REPLACE, so
       * that we composite the op's output on top of the input, instead of
       * completely replacing it.
       */
      paint_mode = CLAY_LAYER_MODE_NORMAL;
    }

  clay_applicator_set_mode (filter->applicator,
                            paint_mode,
                            filter->blend_space,
                            filter->composite_space,
                            filter->composite_mode);
}

static void
clay_drawable_filter_sync_affect (ClayDrawableFilter *filter)
{
  clay_applicator_set_affect (
    filter->applicator,
    filter->override_constraints ?

      CLAY_COMPONENT_MASK_RED   |
      CLAY_COMPONENT_MASK_GREEN |
      CLAY_COMPONENT_MASK_BLUE  |
      CLAY_COMPONENT_MASK_ALPHA :

      clay_drawable_get_active_mask (filter->drawable));
}

static void
clay_drawable_filter_sync_format (ClayDrawableFilter *filter)
{
  const Babl *format = NULL;
  gboolean    changed;

  /* We only convert back to drawable format when the filter is planned
   * to be merged, to simulate how it would look like once it happens.
   *
   * On the other hand, when a filter is meant to stay on the stack,
   * non-destructively, the output might be higher bit depth and there
   * is no reason to demote it back.
   *
   * XXX: actually we might want to do this after the last layer mode
   * node, no? Otherwise the display render might be better in some case
   * than when the whole image is actually flattened into a single
   * buffer. But maybe that's what some people would want?
   */
  if (filter->to_be_merged)
    {
      if (filter->add_alpha                                &&
          (clay_drawable_supports_alpha (filter->drawable) ||
           filter->override_constraints))
        format = clay_drawable_get_format_with_alpha (filter->drawable);
      else
        format = clay_drawable_get_format (filter->drawable);
    }

  changed = clay_applicator_set_output_format (filter->applicator, format);

  if (changed && clay_drawable_filter_is_active (filter))
    clay_drawable_filter_update_drawable (filter, NULL);
}

static void
clay_drawable_filter_sync_mask (ClayDrawableFilter *filter)
{
  ClayImage   *image = clay_item_get_image (CLAY_ITEM (filter->drawable));
  ClayChannel *mask  = NULL;

  if (! filter->mask)
    mask = clay_image_get_mask (image);
  else
    mask = CLAY_CHANNEL (filter->mask);

  if (! mask || clay_channel_is_empty (mask))
    {
      clay_applicator_set_mask_buffer (filter->applicator, NULL);

      clay_item_mask_intersect (CLAY_ITEM (filter->drawable),
                                &filter->filter_area.x,
                                &filter->filter_area.y,
                                &filter->filter_area.width,
                                &filter->filter_area.height);
    }
  else
    {
      GeglBuffer *mask_buffer;
      gint        offset_x, offset_y;

      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));
      clay_item_get_offset (CLAY_ITEM (filter->drawable),
                            &offset_x, &offset_y);

      clay_applicator_set_mask_buffer (filter->applicator, mask_buffer);
      clay_applicator_set_mask_offset (filter->applicator,
                                       -offset_x, -offset_y);

      /* Update filter crop */
      filter->filter_area.x      = mask->x1;
      filter->filter_area.y      = mask->y1;
      filter->filter_area.width  = mask->x2 - mask->x1;
      filter->filter_area.height = mask->y2 - mask->y1;

      clay_item_mask_intersect (CLAY_ITEM (filter->drawable),
                                &filter->filter_area.x,
                                &filter->filter_area.y,
                                &filter->filter_area.width,
                                &filter->filter_area.height);

      clay_drawable_filter_sync_region (filter);
    }
}

static gboolean
clay_drawable_filter_is_added (ClayDrawableFilter *filter)
{
  return clay_drawable_has_filter (filter->drawable,
                                   CLAY_FILTER (filter));
}

static gboolean
clay_drawable_filter_is_active (ClayDrawableFilter *filter)
{
  return clay_drawable_filter_is_added (filter) &&
         filter->preview_enabled;
}

static gboolean
clay_drawable_filter_add_filter (ClayDrawableFilter *filter)
{
  if (! clay_drawable_filter_is_added (filter))
    {
      ClayImage     *image = clay_item_get_image (CLAY_ITEM (filter->drawable));
      ClayContainer *filters;

      clay_viewable_preview_freeze (CLAY_VIEWABLE (filter->drawable));

      clay_drawable_filter_sync_active (filter);
      clay_drawable_filter_sync_mask (filter);
      clay_drawable_filter_sync_clip (filter, FALSE);
      clay_drawable_filter_sync_region (filter);
      clay_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      TRUE);
      clay_drawable_filter_sync_opacity (filter);
      clay_drawable_filter_sync_mode (filter);
      clay_drawable_filter_sync_affect (filter);

      clay_drawable_add_filter (filter->drawable,
                                CLAY_FILTER (filter));
      clay_drawable_filter_sync_format (filter);

      clay_drawable_update_bounding_box (filter->drawable);

      g_signal_connect_object (image, "component-active-changed",
                               G_CALLBACK (clay_drawable_filter_affect_changed),
                               filter, 0);
      if (! filter->mask)
        g_signal_connect_object (image, "mask-changed",
                                 G_CALLBACK (clay_drawable_filter_mask_changed),
                                 filter, 0);
      g_signal_connect_object (filter->drawable, "lock-position-changed",
                               G_CALLBACK (clay_drawable_filter_lock_position_changed),
                               filter, 0);
      g_signal_connect_object (filter->drawable, "format-changed",
                               G_CALLBACK (clay_drawable_filter_format_changed),
                               filter, 0);
      g_signal_connect_object (filter->drawable, "removed",
                               G_CALLBACK (clay_drawable_filter_drawable_removed),
                               filter, 0);

      if (CLAY_IS_LAYER (filter->drawable))
        {
          g_signal_connect_object (filter->drawable, "lock-alpha-changed",
                                   G_CALLBACK (clay_drawable_filter_lock_alpha_changed),
                                   filter, 0);
        }

      filters = clay_drawable_get_filters (filter->drawable);
      g_signal_connect_object (G_OBJECT (filters), "reorder",
                               G_CALLBACK (clay_drawable_filter_reorder),
                               filter, 0);

      g_signal_connect_object (G_OBJECT (filter->operation), "notify",
                               G_CALLBACK (clay_drawable_filters_changed),
                               filter->drawable, G_CONNECT_SWAPPED);
      g_signal_connect_object (G_OBJECT (filter), "active-changed",
                               G_CALLBACK (clay_drawable_filters_changed),
                               filter->drawable, G_CONNECT_SWAPPED);

      return TRUE;
    }

  return FALSE;
}

static gboolean
clay_drawable_filter_remove_filter (ClayDrawableFilter *filter)
{
  if (clay_drawable_filter_is_added (filter))
    {
      ClayImage     *image    = clay_item_get_image (CLAY_ITEM (filter->drawable));
      ClayDrawable  *drawable = filter->drawable;
      ClayContainer *filters;

      g_signal_handlers_disconnect_by_func (G_OBJECT (filter),
                                            G_CALLBACK (clay_drawable_filters_changed),
                                            filter->drawable);

      filters = clay_drawable_get_filters (filter->drawable);
      g_signal_handlers_disconnect_by_func (filters,
                                            G_CALLBACK (clay_drawable_filter_reorder),
                                            filter);

      if (CLAY_IS_LAYER (drawable))
        g_signal_handlers_disconnect_by_func (drawable,
                                              clay_drawable_filter_lock_alpha_changed,
                                              filter);

      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_filter_drawable_removed,
                                            filter);
      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_filter_format_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (drawable,
                                            clay_drawable_filter_lock_position_changed,
                                            filter);

      g_signal_handlers_disconnect_by_func (image,
                                            clay_drawable_filter_affect_changed,
                                            filter);

      clay_drawable_remove_filter (drawable, CLAY_FILTER (filter));

      clay_drawable_update_bounding_box (drawable);

      if (clay_viewable_preview_is_frozen (CLAY_VIEWABLE (drawable)))
        clay_viewable_preview_thaw (CLAY_VIEWABLE (drawable));

      return TRUE;
    }

  return FALSE;
}

static void
clay_drawable_filter_update_drawable (ClayDrawableFilter  *filter,
                                      const GeglRectangle *area)
{
  GeglRectangle bounding_box;
  GeglRectangle update_area;

  bounding_box = clay_drawable_get_bounding_box (filter->drawable);

  if (area)
    {
      if (! gegl_rectangle_intersect (&update_area,
                                      area, &bounding_box))
        {
          return;
        }
    }
  else
    {
      clay_drawable_filter_get_crop_rect (filter,
                                          filter->crop_enabled,
                                          &filter->crop_rect,
                                          filter->preview_split_enabled,
                                          filter->preview_split_alignment,
                                          filter->preview_split_position,
                                          &update_area);

      if (! gegl_rectangle_intersect (&update_area,
                                      &update_area, &bounding_box))
        {
          return;
        }
    }

  if (update_area.width  > 0 &&
      update_area.height > 0)
    {
      clay_drawable_update (filter->drawable,
                            update_area.x,
                            update_area.y,
                            update_area.width,
                            update_area.height);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static void
clay_drawable_filter_affect_changed (ClayImage          *image,
                                     ClayChannelType     channel,
                                     ClayDrawableFilter *filter)
{
  clay_drawable_filter_sync_affect (filter);
  clay_drawable_filter_update_drawable (filter, NULL);
}

static void
clay_drawable_filter_mask_changed (ClayImage          *image,
                                   ClayDrawableFilter *filter)
{
  if (! filter->mask)
    {
      clay_drawable_filter_update_drawable (filter, NULL);

      clay_drawable_filter_sync_mask (filter);
      clay_drawable_filter_sync_clip (filter, FALSE);
      clay_drawable_filter_sync_region (filter);

      clay_drawable_filter_update_drawable (filter, NULL);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (image,
                                            clay_drawable_filter_mask_changed,
                                            filter);
    }
}

static void
clay_drawable_filter_lock_position_changed (ClayDrawable       *drawable,
                                            ClayDrawableFilter *filter)
{
  clay_drawable_filter_sync_clip (filter, TRUE);
  clay_drawable_filter_update_drawable (filter, NULL);
}

static void
clay_drawable_filter_format_changed (ClayDrawable       *drawable,
                                     ClayDrawableFilter *filter)
{
  clay_drawable_filter_sync_format (filter);
  clay_drawable_filter_update_drawable (filter, NULL);
}

static void
clay_drawable_filter_drawable_removed (ClayDrawable       *drawable,
                                       ClayDrawableFilter *filter)
{
  if (filter)
    clay_drawable_filter_remove_filter (filter);
}

static void
clay_drawable_filter_lock_alpha_changed (ClayLayer          *layer,
                                         ClayDrawableFilter *filter)
{
  clay_drawable_filter_sync_affect (filter);
  clay_drawable_filter_update_drawable (filter, NULL);
}

static void
clay_drawable_filter_reorder (ClayFilterStack    *stack,
                              ClayDrawableFilter *reordered_filter,
                              gint                old_index,
                              gint                new_index,
                              ClayDrawableFilter *filter)
{
  if (reordered_filter == filter)
    {
      clay_drawable_filter_sync_format (filter);

      g_return_if_fail (CLAY_LIST (stack)->queue->head != NULL);

      if (CLAY_LIST (stack)->queue->head->data != filter &&
          /* When there is a floating selection, there will be a
           * ClayFilter (but not a drawable filter) in the stack.
           * XXX For now, let's fix the crash (#12851) but eventually we
           * really want to clean up this list and understand better how
           * it's organized.
           */
          CLAY_IS_DRAWABLE_FILTER (CLAY_LIST (stack)->queue->head->data))
        {
          clay_drawable_filter_sync_format (CLAY_LIST (stack)->queue->head->data);
        }
    }
}

static gboolean
clay_drawable_filter_enter_operation (ClayLoadState  *state,
                                      const gchar   **attribute_names,
                                      const gchar   **attribute_values,
                                      gpointer        user_data,
                                      GError        **error)
{
  GeglNode    *operation;
  const gchar *name    = NULL;
  const gchar *version = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        {
          name = *attribute_values;
        }
      else if (g_strcmp0 (*attribute_names, "version") == 0)
        {
          version = *attribute_values;
        }
      else
        {
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  if (name == NULL)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: GEGL operation name is missing.",
                   G_STRFUNC);
    }
  else if (! clay_gegl_op_nde_allowed (name, error))
    {
      g_prefix_error (error,
                      "%s: a filter was discarded: ",
                      G_STRFUNC);
    }
  else if (version &&
           g_strcmp0 (gegl_operation_get_op_version (name), version) != 0)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: version '%s' of filter '%s' is unsupported. "
                   "The filter was discarded.\n"
                   "It either means that you are using an old version of the filter or "
                   "that it was updated without proper version management. "
                   "In the latter case, you should report the issue to the filter's developers.",
                    G_STRFUNC, version, name);
    }
  else
    {
      operation = gegl_node_new ();
      gegl_node_set (operation, "operation", name, NULL);
      clay_savable_load_store_value (state, "operation", operation, g_object_unref);

      clay_savable_load_add_handlers (state, "argument",
                                      clay_drawable_filter_enter_argument,
                                      clay_drawable_filter_exit_argument,
                                      NULL, NULL);
    }

  return TRUE;
}

static gboolean
clay_drawable_filter_exit_operation (ClayLoadState  *state,
                                     const gchar    *text,
                                     gsize           len,
                                     gpointer        user_data,
                                     GError        **error)
{
  clay_savable_load_bubble_up (state, "operation");

  return TRUE;
}

static gboolean
clay_drawable_filter_enter_mask (ClayLoadState  *state,
                                 const gchar   **attribute_names,
                                 const gchar   **attribute_values,
                                 gpointer        user_data,
                                 GError        **error)
{
  clay_savable_load (CLAY_TYPE_CHANNEL, state);

  return TRUE;
}

static gboolean
clay_drawable_filter_enter_argument (ClayLoadState  *state,
                                     const gchar   **attribute_names,
                                     const gchar   **attribute_values,
                                     gpointer        user_data,
                                     GError        **error)
{
  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        clay_savable_load_store_from_string (state, error,
                                             "name", G_TYPE_STRING, *attribute_values,
                                             NULL);
      else if (g_strcmp0 (*attribute_names, "type") == 0)
        clay_savable_load_store_from_string (state, error,
                                             "type", G_TYPE_GTYPE, *attribute_values,
                                             NULL);
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  return TRUE;
}

static gboolean
clay_drawable_filter_exit_argument (ClayLoadState  *state,
                                    const gchar    *text,
                                    gsize           len,
                                    gpointer        user_data,
                                    GError        **error)
{
  const gchar *name     = NULL;
  GType        arg_type = G_TYPE_NONE;
  GParamSpec  *pspec;
  GeglNode    *operation;
  const gchar *op_name;

  clay_savable_load_get_parent_values (state, "operation", &operation, NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), FALSE);

  op_name = gegl_node_get_operation (operation);

  clay_savable_load_get_values (state,
                                "name", &name,
                                "type", &arg_type,
                                NULL);

  if (name == NULL)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                   "%s: skipping unnamed argument.",
                   G_STRFUNC);
    }
  else if (! (pspec = gegl_node_find_property (operation, name)))
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                   "%s: version %s of filter \"%s\" does not "
                   "have the %s property. The property was ignored and "
                   "the filter may not render properly.\n"
                   "This should not happen. "
                   "You should report the issue to the filter's developers.",
                   G_STRFUNC, gegl_operation_get_op_version (op_name),
                   op_name, name);
    }
  else if (arg_type == G_TYPE_NONE ||
           ! g_type_is_a (arg_type, pspec->value_type))
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                   "%s: skipping argument \"%s\" of filter \"%s\" with invalid type.",
                   G_STRFUNC, name, op_name);
    }
  else
    {
      clay_savable_load_store_from_string (state, error, "arg", arg_type, text, NULL);
      if (*error == NULL)
        {
          GValue *value = clay_savable_load_get_gvalue (state, "arg");
          gegl_node_set_property (operation, name, value);
        }
    }

  return TRUE;
}
