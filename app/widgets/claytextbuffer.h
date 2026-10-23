/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextBuffer
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_TEXT_BUFFER            (clay_text_buffer_get_type ())
#define CLAY_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_BUFFER, ClayTextBuffer))
#define CLAY_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_BUFFER))
#define CLAY_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_BUFFER, ClayTextBufferClass))
#define CLAY_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_BUFFER))


typedef struct _ClayTextBufferClass  ClayTextBufferClass;

struct _ClayTextBuffer
{
  GtkTextBuffer  parent_instance;

  GtkTextTag    *bold_tag;
  GtkTextTag    *italic_tag;
  GtkTextTag    *underline_tag;
  GtkTextTag    *strikethrough_tag;

  GList         *size_tags;
  GList         *baseline_tags;
  GList         *kerning_tags;
  GList         *font_tags;
  GList         *color_tags;

  GtkTextTag    *preedit_underline_tag;
  GList         *preedit_color_tags;
  GList         *preedit_bg_color_tags;

  gboolean       insert_tags_set;
  GList         *insert_tags;
  GList         *remove_tags;

  GdkAtom        markup_atom;
};

struct _ClayTextBufferClass
{
  GtkTextBufferClass  parent_class;

  void (* color_applied) (ClayTextBuffer *buffer,
                          GeglColor      *color);
};


GType            clay_text_buffer_get_type             (void);

ClayTextBuffer * clay_text_buffer_new                  (void);

void             clay_text_buffer_set_text             (ClayTextBuffer     *buffer,
                                                        const gchar        *text);
gchar          * clay_text_buffer_get_text             (ClayTextBuffer     *buffer);

void             clay_text_buffer_set_markup           (ClayTextBuffer     *buffer,
                                                        const gchar        *markup);
gchar          * clay_text_buffer_get_markup           (ClayTextBuffer     *buffer);

gboolean         clay_text_buffer_has_markup           (ClayTextBuffer     *buffer);

GtkTextTag     * clay_text_buffer_get_iter_size        (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *size);
GtkTextTag     * clay_text_buffer_get_size_tag         (ClayTextBuffer     *buffer,
                                                        gint                size);
void             clay_text_buffer_set_size             (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                size);
void             clay_text_buffer_change_size          (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                amount,
                                                        gint                default_size);

GtkTextTag     * clay_text_buffer_get_iter_baseline    (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *baseline);
void             clay_text_buffer_set_baseline         (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);
void             clay_text_buffer_change_baseline      (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);

GtkTextTag     * clay_text_buffer_get_iter_kerning     (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *kerning);
void             clay_text_buffer_set_kerning          (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);
void             clay_text_buffer_change_kerning       (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);

GtkTextTag     * clay_text_buffer_get_iter_font        (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gchar             **font);
GtkTextTag     * clay_text_buffer_get_font_tag         (ClayTextBuffer     *buffer,
                                                        const gchar        *font);
void             clay_text_buffer_set_font             (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        const gchar        *font);

GtkTextTag     * clay_text_buffer_get_iter_color       (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        GeglColor         **color);
GtkTextTag     * clay_text_buffer_get_color_tag        (ClayTextBuffer     *buffer,
                                                        GeglColor          *color);
void             clay_text_buffer_set_color            (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);

GtkTextTag * clay_text_buffer_get_preedit_color_tag    (ClayTextBuffer     *buffer,
                                                        GeglColor          *color);
void         clay_text_buffer_set_preedit_color        (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);
GtkTextTag * clay_text_buffer_get_preedit_bg_color_tag (ClayTextBuffer     *buffer,
                                                        GeglColor          *color);
void         clay_text_buffer_set_preedit_bg_color     (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);

const gchar    * clay_text_buffer_tag_to_name          (ClayTextBuffer     *buffer,
                                                        GtkTextTag         *tag,
                                                        const gchar       **attribute,
                                                        gchar             **value);
GtkTextTag     * clay_text_buffer_name_to_tag          (ClayTextBuffer     *buffer,
                                                        const gchar        *name,
                                                        const gchar        *attribute,
                                                        const gchar        *value);

void             clay_text_buffer_set_insert_tags      (ClayTextBuffer     *buffer,
                                                        GList              *insert_tags,
                                                        GList              *remove_tags);
void             clay_text_buffer_clear_insert_tags    (ClayTextBuffer     *buffer);
void             clay_text_buffer_insert               (ClayTextBuffer     *buffer,
                                                        const gchar        *text);

gint             clay_text_buffer_get_iter_index       (ClayTextBuffer     *buffer,
                                                        GtkTextIter        *iter,
                                                        gboolean            layout_index);
void             clay_text_buffer_get_iter_at_index    (ClayTextBuffer     *buffer,
                                                        GtkTextIter        *iter,
                                                        gint                index,
                                                        gboolean            layout_index);

gboolean         clay_text_buffer_load                 (ClayTextBuffer     *buffer,
                                                        GFile              *file,
                                                        GError            **error);
gboolean         clay_text_buffer_save                 (ClayTextBuffer     *buffer,
                                                        GFile              *file,
                                                        gboolean            selection_only,
                                                        GError            **error);

GList          * clay_text_buffer_get_tags_on_iter     (ClayTextBuffer     *buffer,
                                                        const GtkTextIter  *iter);
GList          * clay_text_buffer_get_all_tags         (ClayTextBuffer     *buffer);
