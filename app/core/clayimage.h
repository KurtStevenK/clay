/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#pragma once

#include "clayviewable.h"


#define CLAY_IMAGE_ACTIVE_PARENT ((gpointer) 1)


#define CLAY_TYPE_IMAGE            (clay_image_get_type ())
#define CLAY_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE, ClayImage))
#define CLAY_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE, ClayImageClass))
#define CLAY_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE))
#define CLAY_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE))
#define CLAY_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE, ClayImageClass))


typedef struct _ClayImageClass   ClayImageClass;
typedef struct _ClayImagePrivate ClayImagePrivate;

struct _ClayImage
{
  ClayViewable      parent_instance;

  Clay             *clay;  /*  the Clay the image belongs to  */

  ClayImagePrivate *priv;
};

struct _ClayImageClass
{
  ClayViewableClass  parent_class;

  /*  signals  */
  void (* mode_changed)                 (ClayImage            *image);
  void (* precision_changed)            (ClayImage            *image);
  void (* alpha_changed)                (ClayImage            *image);
  void (* floating_selection_changed)   (ClayImage            *image);
  void (* selected_channels_changed)    (ClayImage            *image);
  void (* selected_paths_changed)       (ClayImage            *image);
  void (* selected_layers_changed)      (ClayImage            *image);
  void (* selected_drawables_changed)   (ClayImage            *image);
  void (* component_visibility_changed) (ClayImage            *image,
                                         ClayChannelType       channel);
  void (* component_active_changed)     (ClayImage            *image,
                                         ClayChannelType       channel);
  void (* mask_changed)                 (ClayImage            *image);
  void (* resolution_changed)           (ClayImage            *image);
  void (* size_changed_detailed)        (ClayImage            *image,
                                         gint                  previous_origin_x,
                                         gint                  previous_origin_y,
                                         gint                  previous_width,
                                         gint                  previous_height);
  void (* unit_changed)                 (ClayImage            *image);
  void (* quick_mask_changed)           (ClayImage            *image);
  void (* selection_invalidate)         (ClayImage            *image);

  void (* clean)                        (ClayImage            *image,
                                         ClayDirtyMask         dirty_mask);
  void (* dirty)                        (ClayImage            *image,
                                         ClayDirtyMask         dirty_mask);
  void (* saving)                       (ClayImage            *image);
  void (* saved)                        (ClayImage            *image,
                                         GFile                *file);
  void (* exported)                     (ClayImage            *image,
                                         GFile                *file);

  void (* guide_added)                  (ClayImage            *image,
                                         ClayGuide            *guide);
  void (* guide_removed)                (ClayImage            *image,
                                         ClayGuide            *guide);
  void (* guide_moved)                  (ClayImage            *image,
                                         ClayGuide            *guide);
  void (* sample_point_added)           (ClayImage            *image,
                                         ClaySamplePoint      *sample_point);
  void (* sample_point_removed)         (ClayImage            *image,
                                         ClaySamplePoint      *sample_point);
  void (* sample_point_moved)           (ClayImage            *image,
                                         ClaySamplePoint      *sample_point);
  void (* parasite_attached)            (ClayImage            *image,
                                         const gchar          *name);
  void (* parasite_detached)            (ClayImage            *image,
                                         const gchar          *name);
  void (* colormap_changed)             (ClayImage            *image,
                                         gint                  color_index);
  void (* undo_event)                   (ClayImage            *image,
                                         ClayUndoEvent         event,
                                         ClayUndo             *undo);
  void (* item_sets_changed)            (ClayImage            *image,
                                         GType                 item_type);
  void (* display_count_changed)        (ClayImage            *image,
                                         gint                  display_count);
};


GType                 clay_image_get_type                     (void);

ClayImage           * clay_image_new                          (Clay               *clay,
                                                               gint                width,
                                                               gint                height,
                                                               ClayImageBaseType   base_type,
                                                               ClayPrecision       precision);

gint64                clay_image_estimate_memsize             (ClayImage          *image,
                                                               ClayComponentType   component_type,
                                                               gint                width,
                                                               gint                height);

ClayImageBaseType     clay_image_get_base_type                (ClayImage          *image);
ClayComponentType     clay_image_get_component_type           (ClayImage          *image);
ClayPrecision         clay_image_get_precision                (ClayImage          *image);

const Babl          * clay_image_get_format                   (ClayImage          *image,
                                                               ClayImageBaseType   base_type,
                                                               ClayPrecision       precision,
                                                               gboolean            with_alpha,
                                                               const Babl         *space);

const Babl          * clay_image_get_layer_space              (ClayImage          *image);
const Babl          * clay_image_get_layer_format             (ClayImage          *image,
                                                               gboolean            with_alpha);
const Babl          * clay_image_get_channel_format           (ClayImage          *image);
const Babl          * clay_image_get_mask_format              (ClayImage          *image);

ClayLayerMode         clay_image_get_default_new_layer_mode   (ClayImage          *image);
void                  clay_image_unset_default_new_layer_mode (ClayImage          *image);

gint                  clay_image_get_id                       (ClayImage          *image);
ClayImage           * clay_image_get_by_id                    (Clay               *clay,
                                                               gint                id);

GFile               * clay_image_get_file                     (ClayImage          *image);
GFile               * clay_image_get_untitled_file            (ClayImage          *image);
GFile               * clay_image_get_file_or_untitled         (ClayImage          *image);
GFile               * clay_image_get_imported_file            (ClayImage          *image);
GFile               * clay_image_get_exported_file            (ClayImage          *image);
GFile               * clay_image_get_save_a_copy_file         (ClayImage          *image);
GFile               * clay_image_get_any_file                 (ClayImage          *image);

void                  clay_image_set_file                     (ClayImage          *image,
                                                               GFile              *file);
void                  clay_image_set_imported_file            (ClayImage          *image,
                                                               GFile              *file);
void                  clay_image_set_exported_file            (ClayImage          *image,
                                                               GFile              *file);
void                  clay_image_set_save_a_copy_file         (ClayImage          *image,
                                                               GFile              *file);

const gchar         * clay_image_get_display_name             (ClayImage          *image);
const gchar         * clay_image_get_display_path             (ClayImage          *image);

void                  clay_image_set_load_proc                (ClayImage          *image,
                                                               ClayPlugInProcedure *proc);
ClayPlugInProcedure * clay_image_get_load_proc                (ClayImage          *image);
void                  clay_image_set_save_proc                (ClayImage          *image,
                                                               ClayPlugInProcedure *proc);
ClayPlugInProcedure * clay_image_get_save_proc                (ClayImage          *image);
void                  clay_image_saving                       (ClayImage          *image);
void                  clay_image_saved                        (ClayImage          *image,
                                                               GFile              *file);
void                  clay_image_set_export_proc              (ClayImage          *image,
                                                               ClayPlugInProcedure *proc);
ClayPlugInProcedure * clay_image_get_export_proc              (ClayImage          *image);
void                  clay_image_exported                     (ClayImage          *image,
                                                               GFile              *file);

const gchar         * clay_image_get_cache_folder             (ClayImage          *image);
GFile               * clay_image_get_cache_xml_file           (ClayImage          *image);
const gchar         * clay_image_get_buffers_folder           (ClayImage          *image);

gint                  clay_image_get_xcf_version              (ClayImage          *image,
                                                               gboolean            zlib_compression,
                                                               gint               *clay_version,
                                                               const gchar       **version_string,
                                                               gchar             **version_reason);

void                  clay_image_set_xcf_compression          (ClayImage          *image,
                                                               gboolean            compression);
gboolean              clay_image_get_xcf_compression          (ClayImage          *image);

void                  clay_image_set_resolution               (ClayImage          *image,
                                                               gdouble             xres,
                                                               gdouble             yres);
void                  clay_image_get_resolution               (ClayImage          *image,
                                                               gdouble            *xres,
                                                               gdouble            *yres);
void                  clay_image_resolution_changed           (ClayImage          *image);

void                  clay_image_set_unit                     (ClayImage          *image,
                                                               ClayUnit           *unit);
ClayUnit            * clay_image_get_unit                     (ClayImage          *image);
void                  clay_image_unit_changed                 (ClayImage          *image);

gint                  clay_image_get_width                    (ClayImage          *image);
gint                  clay_image_get_height                   (ClayImage          *image);

gboolean              clay_image_has_alpha                    (ClayImage          *image);
gboolean              clay_image_has_transparency             (ClayImage          *image);
gboolean              clay_image_is_empty                     (ClayImage          *image);

void                  clay_image_set_floating_selection       (ClayImage          *image,
                                                               ClayLayer          *floating_sel);
ClayLayer           * clay_image_get_floating_selection       (ClayImage          *image);
void                  clay_image_floating_selection_changed   (ClayImage          *image);

ClayChannel         * clay_image_get_mask                     (ClayImage          *image);
void                  clay_image_mask_changed                 (ClayImage          *image);
gboolean              clay_image_mask_intersect               (ClayImage          *image,
                                                               GList              *items,
                                                               gint               *x,
                                                               gint               *y,
                                                               gint               *width,
                                                               gint               *height);


/*  image components  */

const Babl          * clay_image_get_component_format         (ClayImage          *image,
                                                               ClayChannelType     channel);
gint                  clay_image_get_component_index          (ClayImage          *image,
                                                               ClayChannelType     channel);

void                  clay_image_set_component_active         (ClayImage          *image,
                                                               ClayChannelType     type,
                                                               gboolean            active);
gboolean              clay_image_get_component_active         (ClayImage          *image,
                                                               ClayChannelType     type);
ClayComponentMask     clay_image_get_active_mask              (ClayImage          *image);

void                  clay_image_set_component_visible        (ClayImage          *image,
                                                               ClayChannelType     type,
                                                               gboolean            visible);
gboolean              clay_image_get_component_visible        (ClayImage          *image,
                                                               ClayChannelType     type);
ClayComponentMask     clay_image_get_visible_mask             (ClayImage          *image);


/*  emitting image signals  */

void                  clay_image_mode_changed                 (ClayImage          *image);
void                  clay_image_precision_changed            (ClayImage          *image);
void                  clay_image_alpha_changed                (ClayImage          *image);
void                  clay_image_selected_drawables_changed   (ClayImage          *image);
void                  clay_image_invalidate                   (ClayImage          *image,
                                                               gint                x,
                                                               gint                y,
                                                               gint                width,
                                                               gint                height);
void                  clay_image_invalidate_all               (ClayImage          *image);
void                  clay_image_guide_added                  (ClayImage          *image,
                                                               ClayGuide          *guide);
void                  clay_image_guide_removed                (ClayImage          *image,
                                                               ClayGuide          *guide);
void                  clay_image_guide_moved                  (ClayImage          *image,
                                                               ClayGuide          *guide);

void                  clay_image_sample_point_added           (ClayImage          *image,
                                                               ClaySamplePoint    *sample_point);
void                  clay_image_sample_point_removed         (ClayImage          *image,
                                                               ClaySamplePoint    *sample_point);
void                  clay_image_sample_point_moved           (ClayImage          *image,
                                                               ClaySamplePoint    *sample_point);
void                  clay_image_colormap_changed             (ClayImage          *image,
                                                               gint                col);
void                  clay_image_selection_invalidate         (ClayImage          *image);
void                  clay_image_quick_mask_changed           (ClayImage          *image);
void                  clay_image_size_changed_detailed        (ClayImage          *image,
                                                               gint                previous_origin_x,
                                                               gint                previous_origin_y,
                                                               gint                previous_width,
                                                               gint                previous_height);
void                  clay_image_undo_event                   (ClayImage          *image,
                                                               ClayUndoEvent       event,
                                                               ClayUndo           *undo);


/*  dirty counters  */

gint                  clay_image_dirty                        (ClayImage          *image,
                                                               ClayDirtyMask       dirty_mask);
gint                  clay_image_clean                        (ClayImage          *image,
                                                               ClayDirtyMask       dirty_mask);
void                  clay_image_clean_all                    (ClayImage          *image);
void                  clay_image_export_clean_all             (ClayImage          *image);
gint                  clay_image_is_dirty                     (ClayImage          *image);
gboolean              clay_image_is_export_dirty              (ClayImage          *image);
gint64                clay_image_get_dirty_time               (ClayImage          *image);


/*  flush this image's displays  */

void                  clay_image_flush                        (ClayImage          *image);


/*  display / instance counters  */

gint                  clay_image_get_display_count            (ClayImage          *image);
void                  clay_image_inc_display_count            (ClayImage          *image);
void                  clay_image_dec_display_count            (ClayImage          *image);

gint                  clay_image_get_instance_count           (ClayImage          *image);
void                  clay_image_inc_instance_count           (ClayImage          *image);

void                  clay_image_inc_show_all_count           (ClayImage          *image);
void                  clay_image_dec_show_all_count           (ClayImage          *image);


/*  parasites  */

const ClayParasite  * clay_image_parasite_find                (ClayImage          *image,
                                                               const gchar        *name);
gchar              ** clay_image_parasite_list                (ClayImage          *image);
gboolean              clay_image_parasite_validate            (ClayImage          *image,
                                                               const ClayParasite *parasite,
                                                               GError            **error);
void                  clay_image_parasite_attach              (ClayImage          *image,
                                                               const ClayParasite *parasite,
                                                               gboolean            push_undo);
void                  clay_image_parasite_detach              (ClayImage          *image,
                                                               const gchar        *name,
                                                               gboolean            push_undo);


/*  tattoos  */

ClayTattoo            clay_image_get_new_tattoo               (ClayImage          *image);
gboolean              clay_image_set_tattoo_state             (ClayImage          *image,
                                                               ClayTattoo          val);
ClayTattoo            clay_image_get_tattoo_state             (ClayImage          *image);


/*  projection  */

ClayProjection      * clay_image_get_projection               (ClayImage          *image);


/*  layers / channels / paths  */

ClayItemTree        * clay_image_get_layer_tree               (ClayImage          *image);
ClayItemTree        * clay_image_get_channel_tree             (ClayImage          *image);
ClayItemTree        * clay_image_get_path_tree                (ClayImage          *image);

ClayContainer       * clay_image_get_items                    (ClayImage          *image,
                                                               GType               item_type);
ClayContainer       * clay_image_get_layers                   (ClayImage          *image);
ClayContainer       * clay_image_get_channels                 (ClayImage          *image);
ClayContainer       * clay_image_get_paths                    (ClayImage          *image);

gint                  clay_image_get_n_layers                 (ClayImage          *image);
gint                  clay_image_get_n_channels               (ClayImage          *image);
gint                  clay_image_get_n_paths                  (ClayImage          *image);

GList               * clay_image_get_layer_iter               (ClayImage          *image);
GList               * clay_image_get_channel_iter             (ClayImage          *image);
GList               * clay_image_get_path_iter                (ClayImage          *image);

GList               * clay_image_get_layer_list               (ClayImage          *image);
GList               * clay_image_get_channel_list             (ClayImage          *image);
GList               * clay_image_get_path_list                (ClayImage          *image);

void                  clay_image_unset_selected_channels      (ClayImage          *image);

gboolean              clay_image_is_selected_drawable         (ClayImage          *image,
                                                               ClayDrawable       *drawable);
gboolean              clay_image_equal_selected_drawables     (ClayImage          *image,
                                                               GList              *drawables);

GList               * clay_image_get_selected_drawables       (ClayImage          *image);

GList               * clay_image_get_selected_items           (ClayImage          *image,
                                                               GType               item_type);
GList               * clay_image_get_selected_layers          (ClayImage          *image);
GList               * clay_image_get_selected_channels        (ClayImage          *image);
GList               * clay_image_get_selected_paths           (ClayImage          *image);

void                  clay_image_set_selected_items           (ClayImage          *image,
                                                               GType               item_type,
                                                               GList              *items);
void                  clay_image_set_selected_layers          (ClayImage          *image,
                                                               GList              *layers);
void                  clay_image_set_selected_channels        (ClayImage          *image,
                                                               GList              *channels);
void                  clay_image_set_selected_paths           (ClayImage          *image,
                                                               GList              *paths);

ClayLayer           * clay_image_get_layer_by_tattoo          (ClayImage          *image,
                                                               ClayTattoo          tattoo);
ClayChannel         * clay_image_get_channel_by_tattoo        (ClayImage          *image,
                                                               ClayTattoo          tattoo);
ClayPath            * clay_image_get_path_by_tattoo           (ClayImage          *image,
                                                               ClayTattoo          tattoo);

ClayLayer           * clay_image_get_layer_by_name            (ClayImage          *image,
                                                               const gchar        *name);
ClayChannel         * clay_image_get_channel_by_name          (ClayImage          *image,
                                                               const gchar        *name);
ClayPath            * clay_image_get_path_by_name             (ClayImage          *image,
                                                               const gchar        *name);

gboolean              clay_image_reorder_item                 (ClayImage          *image,
                                                               ClayItem           *item,
                                                               ClayItem           *new_parent,
                                                               gint                new_index,
                                                               gboolean            push_undo,
                                                               const gchar        *undo_desc);
gboolean              clay_image_raise_item                   (ClayImage          *image,
                                                               ClayItem           *item,
                                                               GError            **error);
gboolean              clay_image_raise_item_to_top            (ClayImage          *image,
                                                               ClayItem           *item);
gboolean              clay_image_lower_item                   (ClayImage          *image,
                                                               ClayItem           *item,
                                                               GError            **error);
gboolean              clay_image_lower_item_to_bottom         (ClayImage          *image,
                                                               ClayItem           *item);

gboolean              clay_image_add_item                     (ClayImage          *image,
                                                               ClayItem           *item,
                                                               ClayItem          *parent,
                                                               gint                position,
                                                               gboolean            push_undo);
void                  clay_image_remove_item                  (ClayImage          *image,
                                                               ClayItem           *item,
                                                               gboolean            push_undo,
                                                               GList              *new_selected);

gboolean              clay_image_add_layer                    (ClayImage          *image,
                                                               ClayLayer          *layer,
                                                               ClayLayer          *parent,
                                                               gint                position,
                                                               gboolean            push_undo);
void                  clay_image_remove_layer                 (ClayImage          *image,
                                                               ClayLayer          *layer,
                                                               gboolean            push_undo,
                                                               GList              *new_selected);

void                  clay_image_add_layers                   (ClayImage          *image,
                                                               GList              *layers,
                                                               ClayLayer          *parent,
                                                               gint                position,
                                                               gint                x,
                                                               gint                y,
                                                               gint                width,
                                                               gint                height,
                                                               const gchar        *undo_desc);

gboolean              clay_image_add_channel                  (ClayImage          *image,
                                                               ClayChannel        *channel,
                                                               ClayChannel        *parent,
                                                               gint                position,
                                                               gboolean            push_undo);
void                  clay_image_remove_channel               (ClayImage          *image,
                                                               ClayChannel        *channel,
                                                               gboolean            push_undo,
                                                               GList              *new_selected);

gboolean              clay_image_add_path                     (ClayImage          *image,
                                                               ClayPath           *path,
                                                               ClayPath           *parent,
                                                               gint                position,
                                                               gboolean            push_undo);
void                  clay_image_remove_path                  (ClayImage          *image,
                                                               ClayPath           *path,
                                                               gboolean            push_undo,
                                                               GList              *new_selected);

void                  clay_image_store_item_set               (ClayImage          *image,
                                                               ClayItemList       *set);
gboolean              clay_image_unlink_item_set              (ClayImage          *image,
                                                               ClayItemList       *set);
GList               * clay_image_get_stored_item_sets         (ClayImage          *image,
                                                               GType               item_type);
void                  clay_image_select_item_set              (ClayImage          *image,
                                                               ClayItemList       *set);
void                  clay_image_add_item_set                 (ClayImage          *image,
                                                               ClayItemList       *set);
void                  clay_image_remove_item_set              (ClayImage          *image,
                                                               ClayItemList       *set);
void                  clay_image_intersect_item_set           (ClayImage          *image,
                                                               ClayItemList       *set);

gboolean              clay_image_add_hidden_item              (ClayImage          *image,
                                                               ClayItem           *item);
void                  clay_image_remove_hidden_item           (ClayImage          *image,
                                                               ClayItem           *item);
gboolean              clay_image_is_hidden_item               (ClayImage          *image,
                                                               ClayItem           *item);

gboolean              clay_image_coords_in_active_pickable    (ClayImage          *image,
                                                               const ClayCoords   *coords,
                                                               gboolean            show_all,
                                                               gboolean            sample_merged,
                                                               gboolean            selected_only);

void                  clay_image_invalidate_previews          (ClayImage          *image);

void                  clay_image_set_converting               (ClayImage          *image,
                                                               gboolean            converting);
gboolean              clay_image_get_converting               (ClayImage          *image);

void                  clay_image_set_from_command_line        (ClayImage          *image,
                                                               gboolean            from_command_line);
gboolean              clay_image_get_from_command_line        (ClayImage          *image);
