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

#pragma once


/*  maximal length of the format string for the cursor-coordinates  */
#define CURSOR_FORMAT_LENGTH 32


#define CLAY_TYPE_STATUSBAR            (clay_statusbar_get_type ())
#define CLAY_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_STATUSBAR, ClayStatusbar))
#define CLAY_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_STATUSBAR, ClayStatusbarClass))
#define CLAY_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_STATUSBAR))
#define CLAY_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_STATUSBAR))
#define CLAY_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_STATUSBAR, ClayStatusbarClass))

typedef struct _ClayStatusbarClass ClayStatusbarClass;

struct _ClayStatusbar
{
  GtkFrame             parent_instance;

  Clay                *clay;
  ClayDisplayShell    *shell;
  ClayImage           *image;

  GSList              *messages;
  GHashTable          *context_ids;
  guint                seq_context_id;

  guint                statusbar_pos_redraw_idle_id;
  gchar               *cursor_string_todraw;
  gchar               *cursor_string_last;

  GdkPixbuf           *icon;
  GHashTable          *icon_hash;
  gint                 icon_space_width;
  GtkIconSize          icon_size;

  guint                temp_context_id;
  guint                temp_timeout_id;
  ClayMessageSeverity  temp_severity;

  gchar                cursor_format_str[CURSOR_FORMAT_LENGTH];
  gchar                cursor_format_str_f[CURSOR_FORMAT_LENGTH];
  gchar                length_format_str[CURSOR_FORMAT_LENGTH];
  ClayCursorPrecision  cursor_precision;
  gint                 cursor_w_digits;
  gint                 cursor_h_digits;


  GtkWidget           *cursor_label;
  GtkWidget           *unit_combo;
  GtkWidget           *scale_combo;
  GtkWidget           *rotate_widget;
  GtkWidget           *rotate_label;
  GtkWidget           *horizontal_flip_icon;
  GtkWidget           *vertical_flip_icon;
  GtkWidget           *label; /* same as GtkStatusbar->label */

  GtkWidget           *progressbar;
  GtkWidget           *cancel_button;
  gboolean             progress_active;
  gboolean             progress_shown;
  gdouble              progress_value;
  guint64              progress_last_update_time;

  GtkWidget           *soft_proof_button;
  GtkWidget           *soft_proof_container;
  GtkWidget           *soft_proof_popover;
  GtkWidget           *proof_colors_toggle;
  GtkWidget           *profile_label;
  GtkWidget           *profile_combo;
  GtkWidget           *rendering_intent_combo;
  GtkWidget           *bpc_toggle;
  GtkWidget           *optimize_combo;
  GtkWidget           *out_of_gamut_toggle;

  GSList              *size_widgets;
};

struct _ClayStatusbarClass
{
  GtkFrameClass parent_class;
};


GType       clay_statusbar_get_type              (void);
GtkWidget * clay_statusbar_new                   (void);

void        clay_statusbar_set_shell             (ClayStatusbar       *statusbar,
                                                  ClayDisplayShell    *shell);

gboolean    clay_statusbar_get_visible           (ClayStatusbar       *statusbar);
void        clay_statusbar_set_visible           (ClayStatusbar       *statusbar,
                                                  gboolean             visible);
void        clay_statusbar_empty                 (ClayStatusbar       *statusbar);
void        clay_statusbar_fill                  (ClayStatusbar       *statusbar);

void        clay_statusbar_override_window_title (ClayStatusbar       *statusbar);
void        clay_statusbar_restore_window_title  (ClayStatusbar       *statusbar);

void        clay_statusbar_push                  (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        clay_statusbar_push_valist           (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        clay_statusbar_push_coords           (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  ClayCursorPrecision  precision,
                                                  const gchar         *title,
                                                  gdouble              x,
                                                  const gchar         *separator,
                                                  gdouble              y,
                                                  const gchar         *help);
void        clay_statusbar_push_length           (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *title,
                                                  ClayOrientationType  axis,
                                                  gdouble              value,
                                                  const gchar         *help);
void        clay_statusbar_replace               (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        clay_statusbar_replace_valist        (ClayStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
const gchar * clay_statusbar_peek                (ClayStatusbar       *statusbar,
                                                  const gchar         *context);
void        clay_statusbar_pop                   (ClayStatusbar       *statusbar,
                                                  const gchar         *context);

void        clay_statusbar_push_temp             (ClayStatusbar       *statusbar,
                                                  ClayMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        clay_statusbar_push_temp_valist      (ClayStatusbar       *statusbar,
                                                  ClayMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        clay_statusbar_pop_temp              (ClayStatusbar       *statusbar);

void        clay_statusbar_update_cursor         (ClayStatusbar       *statusbar,
                                                  ClayCursorPrecision  precision,
                                                  gdouble              x,
                                                  gdouble              y);
void        clay_statusbar_clear_cursor          (ClayStatusbar       *statusbar);
