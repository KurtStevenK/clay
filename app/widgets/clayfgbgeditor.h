/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfgbgeditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


typedef enum
{
  CLAY_FG_BG_TARGET_INVALID,
  CLAY_FG_BG_TARGET_FOREGROUND,
  CLAY_FG_BG_TARGET_BACKGROUND,
  CLAY_FG_BG_TARGET_SWAP,
  CLAY_FG_BG_TARGET_DEFAULT
} ClayFgBgTarget;


#define CLAY_TYPE_FG_BG_EDITOR            (clay_fg_bg_editor_get_type ())
#define CLAY_FG_BG_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FG_BG_EDITOR, ClayFgBgEditor))
#define CLAY_FG_BG_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FG_BG_EDITOR, ClayFgBgEditorClass))
#define CLAY_IS_FG_BG_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FG_BG_EDITOR))
#define CLAY_IS_FG_BG_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FG_BG_EDITOR))
#define CLAY_FG_BG_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FG_BG_EDITOR, ClayFgBgEditorClass))


typedef struct _ClayFgBgEditorClass ClayFgBgEditorClass;

struct _ClayFgBgEditor
{
  GtkEventBox         parent_instance;

  ClayContext        *context;
  ClayColorConfig    *color_config;
  ClayColorTransform *transform;

  ClayActiveColor     active_color;

  ClayImage          *active_image;

  GdkPixbuf          *default_icon;
  GdkPixbuf          *swap_icon;

  gint                rect_width;
  gint                rect_height;
  gint                click_target;
};

struct _ClayFgBgEditorClass
{
  GtkEventBoxClass    parent_class;

  /*  signals  */

  void (* color_clicked)  (ClayFgBgEditor  *editor,
                           ClayActiveColor  color);
  void (* color_dropped)  (ClayFgBgEditor  *editor,
                           ClayActiveColor  color);
  void (* colors_swapped) (ClayFgBgEditor  *editor);
  void (* colors_default) (ClayFgBgEditor  *editor);

  void (* tooltip)        (ClayFgBgEditor *editor,
                           ClayFgBgTarget  target,
                           GtkTooltip      tooltip);
};


GType       clay_fg_bg_editor_get_type    (void);

GtkWidget * clay_fg_bg_editor_new         (ClayContext     *context);

void        clay_fg_bg_editor_set_context (ClayFgBgEditor  *editor,
                                           ClayContext     *context);
void        clay_fg_bg_editor_set_active  (ClayFgBgEditor  *editor,
                                           ClayActiveColor  active);
