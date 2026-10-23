/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#include "claydrawtool.h"


#define CLAY_TYPE_TEXT_TOOL            (clay_text_tool_get_type ())
#define CLAY_TEXT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_TOOL, ClayTextTool))
#define CLAY_IS_TEXT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_TOOL))
#define CLAY_TEXT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_TOOL, ClayTextToolClass))
#define CLAY_IS_TEXT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_TOOL))

#define CLAY_TEXT_TOOL_GET_OPTIONS(t)  (CLAY_TEXT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayTextTool       ClayTextTool;
typedef struct _ClayTextToolClass  ClayTextToolClass;

typedef enum
{
  CLAY_TEXT_SELECT_DEFAULT,
  CLAY_TEXT_SELECT_WORDS,
  CLAY_TEXT_SELECT_LINES
} ClayTextSelectMode;


struct _ClayTextTool
{
  ClayDrawTool        parent_instance;

  ClayText           *proxy;
  GList              *pending;
  guint               idle_id;

  gboolean            moving;
  gint                move_start_x;
  gint                move_start_y;

  ClayTextBuffer     *buffer;

  ClayText           *text;
  ClayTextLayer      *layer;
  ClayImage          *image;

  GtkWidget          *confirm_dialog;

  gboolean            handle_rectangle_change_complete;
  gboolean            text_box_fixed;

  ClayTextLayout     *layout;
  gint                drawing_blocked;

  ClayToolWidget     *widget;
  ClayToolWidget     *grab_widget;

  /* text editor state: */

  GtkWidget          *style_overlay;
  GtkWidget          *style_editor;

  /* style overlay dragging: */
  gboolean            overlay_dragging;

  /* Whether a click-and-drag selection uses whole words or lines */
  ClayTextSelectMode  select_by;

  gboolean            button_held;
  GtkIMContext       *im_context;
  gboolean            needs_im_reset;

  gboolean            preedit_active;
  gchar              *preedit_string;
  gint                preedit_cursor;
  GtkTextMark        *preedit_start;
  GtkTextMark        *preedit_end;

  gboolean            overwrite_mode;
  gint                x_pos;

  GtkWidget          *offscreen_window;
  GtkWidget          *proxy_text_view;

  GtkWidget          *editor_dialog;
};

struct _ClayTextToolClass
{
  ClayDrawToolClass  parent_class;
};


void       clay_text_tool_register               (ClayToolRegisterCallback  callback,
                                                  gpointer                  data);

GType      clay_text_tool_get_type               (void);

gboolean   clay_text_tool_set_layer              (ClayTextTool  *text_tool,
                                                  ClayLayer     *layer);

gboolean   clay_text_tool_get_has_text_selection (ClayTextTool  *text_tool);

void       clay_text_tool_delete_selection       (ClayTextTool  *text_tool);
void       clay_text_tool_cut_clipboard          (ClayTextTool  *text_tool);
void       clay_text_tool_copy_clipboard         (ClayTextTool  *text_tool);
void       clay_text_tool_paste_clipboard        (ClayTextTool  *text_tool);
void       clay_text_tool_paste_clipboard_unformatted
                                                 (ClayTextTool  *text_tool);
void       clay_text_tool_toggle_tag             (ClayTextTool  *text_tool,
                                                  GtkTextTag    *tag);

void       clay_text_tool_create_path            (ClayTextTool  *text_tool);
gboolean   clay_text_tool_create_path_warped     (ClayTextTool  *text_tool,
                                                  GError       **error);

ClayTextDirection
           clay_text_tool_get_direction          (ClayTextTool  *text_tool);

/*  only for the text editor  */
void       clay_text_tool_clear_layout           (ClayTextTool  *text_tool);
gboolean   clay_text_tool_ensure_layout          (ClayTextTool  *text_tool);
void       clay_text_tool_apply                  (ClayTextTool  *text_tool,
                                                  gboolean       push_undo);

void       clay_text_tool_restore_on_canvas_editor_position
                                                 (ClayTextTool  *text_tool);
