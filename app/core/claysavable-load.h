/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claysavable.h
 * Copyright (C) 2026 Jehan
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


#define CLAY_WLBR_ERROR (clay_wlbr_error_quark ())

typedef enum
{
  CLAY_WLBR_ERROR_OPEN,     /*  Opening file failed   */
  CLAY_WLBR_ERROR_READ,     /*  Reading file failed   */
  CLAY_WLBR_ERROR_WRITE,    /*  Writing file failed   */
  CLAY_WLBR_ERROR_DELETE,   /*  Deleting file failed  */
  CLAY_WLBR_ERROR_FORMAT,   /*  Invalid format        */
  CLAY_WLBR_ERROR_DATA,     /*  Invalid data          */
  CLAY_WLBR_ERROR_INTERNAL, /*  Internal code error   */
} ClayWlbrError;


/* Macro to set @error.
 * If @error is already set, just ignore further errors in the same call.
 * When error is NULL, we are in a case where we control the input data
 * in a way that we are not supposed to ever reach this point. So we
 * trigger a CRITICAL and return FALSE because it's a bug.
 */
#define CLAY_SAVABLE_LOAD_ERROR(error, error_code, cleanup, message, ...) \
  G_STMT_START {                                                          \
      if (error)                                                          \
        {                                                                 \
          if (*((GError**) error) == NULL)                                \
            g_set_error (error, CLAY_WLBR_ERROR, error_code,              \
                         message, __VA_ARGS__);                           \
        }                                                                 \
      else                                                                \
        {                                                                 \
          g_critical ("%s (%s - line %d): internal error.\n",             \
                      G_STRFUNC, __FILE__, __LINE__);                     \
          cleanup;                                                        \
          return FALSE;                                                   \
        }                                                                 \
  } G_STMT_END


void         clay_savable_load                   (GType                    savable_type,
                                                  ClayLoadState           *state);
void         clay_savable_config_load            (GType                    config_type,
                                                  const gchar             *element_name,
                                                  ClayLoadState           *state,
                                                  ClayExitElementhandler   secondary_exit_handler,
                                                  ...) G_GNUC_NULL_TERMINATED;
void         clay_savable_parasite_load          (ClayLoadState           *state,
                                                  GObject                 *image_or_item);


/* Functions to be used from inside ClaySavable's load() implementation */

void         clay_savable_load_push_active_object (ClayLoadState            *state,
                                                   GObject                  *object);
GObject    * clay_savable_load_pop_active_object  (ClayLoadState            *state);
GObject    * clay_savable_load_peek_active_object (ClayLoadState            *state);

void         clay_savable_load_store_from_string (ClayLoadState            *state,
                                                  GError                  **error,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         clay_savable_load_store_value        (ClayLoadState            *state,
                                                   const gchar              *key,
                                                   gpointer                  data,
                                                   GDestroyNotify            free_data);

gboolean     clay_savable_load_get_values         (ClayLoadState            *state,
                                                   ...) G_GNUC_NULL_TERMINATED;
GValue     * clay_savable_load_get_gvalue         (ClayLoadState            *state,
                                                   const gchar              *value_name);
gboolean     clay_savable_load_get_parent_values  (ClayLoadState            *state,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         clay_savable_load_bubble_up          (ClayLoadState            *state,
                                                   const gchar              *key);

void         clay_savable_load_add_handlers       (ClayLoadState            *state,
                                                   const gchar              *element_name,
                                                   ClayEnterElementHandler   enter_callback,
                                                   ClayExitElementhandler    exit_callback,
                                                   gpointer                  user_data,
                                                   GDestroyNotify            free_data);

void         clay_savable_load_add_simple_handler (ClayLoadState            *state,
                                                   const gchar              *element_name,
                                                   GType                     text_value_type,
                                                   ClayExitElementhandler    secondary_exit_handler,
                                                   gpointer                  user_data,
                                                   GDestroyNotify            free_data,
                                                   gboolean                  all_attributes_needed,
                                                   gboolean                  fatal_on_missing,
                                                   ...) G_GNUC_NULL_TERMINATED;


/* Generic element handlers as args to clay_savable_load_add_handlers(). */

gboolean     clay_savable_enter_unit             (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_unit              (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_enter_color            (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_color             (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_enter_format           (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_format            (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_enter_space            (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_space             (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_enter_value_array      (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_value_array       (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_enter_matrix           (ClayLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     clay_savable_exit_matrix            (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);


/* Friend functions used only in clayimage-savable.c */

gboolean      clay_savable_load_parse            (ClayLoadState            *state,
                                                  Clay                     *clay,
                                                  GFile                    *backup_dir,
                                                  gint                      ID,
                                                  GError                  **error);
void          clay_savable_load_free_state       (ClayLoadState            *state);
void          clay_savable_load_append_text      (ClayLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     text_len);
const gchar * clay_savable_load_get_text         (ClayLoadState            *state,
                                                  gsize                    *text_len);

gboolean      clay_savable_enter_element         (ClayLoadState            *state,
                                                  const gchar              *element_name,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  GError                  **error);
gboolean      clay_savable_exit_element          (ClayLoadState            *state,
                                                  const gchar              *element_name,
                                                  const gchar              *text,
                                                  gsize                     text_len,
                                                  GError                  **error);


/*  The error domain associated with format loading. */

GQuark       clay_wlbr_error_quark               (void) G_GNUC_CONST;
