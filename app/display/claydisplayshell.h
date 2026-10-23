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


/* Apply to a float the same rounding mode used in the renderer */
#define  PROJ_ROUND(coord)   ((gint) RINT (coord))
#define  PROJ_ROUND64(coord) ((gint64) RINT (coord))

/* scale values */
#define  SCALEX(s,x)      PROJ_ROUND ((x) * (s)->scale_x)
#define  SCALEY(s,y)      PROJ_ROUND ((y) * (s)->scale_y)

/* unscale values */
#define  UNSCALEX(s,x)    ((gint) ((x) / (s)->scale_x))
#define  UNSCALEY(s,y)    ((gint) ((y) / (s)->scale_y))
/* (and float-returning versions) */
#define  FUNSCALEX(s,x)   ((x) / (s)->scale_x)
#define  FUNSCALEY(s,y)   ((y) / (s)->scale_y)


#define CLAY_TYPE_DISPLAY_SHELL            (clay_display_shell_get_type ())
#define CLAY_DISPLAY_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DISPLAY_SHELL, ClayDisplayShell))
#define CLAY_DISPLAY_SHELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DISPLAY_SHELL, ClayDisplayShellClass))
#define CLAY_IS_DISPLAY_SHELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DISPLAY_SHELL))
#define CLAY_IS_DISPLAY_SHELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DISPLAY_SHELL))
#define CLAY_DISPLAY_SHELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DISPLAY_SHELL, ClayDisplayShellClass))


typedef struct _ClayDisplayShellClass   ClayDisplayShellClass;
typedef struct _ClayDisplayShellPrivate ClayDisplayShellPrivate;

struct _ClayDisplayShell
{
  GtkEventBox        parent_instance;

  ClayDisplay       *display;

  GBytes            *window_handle;

  ClayUIManager     *popup_manager;

  ClayDisplayOptions *options;
  ClayDisplayOptions *fullscreen_options;
  ClayDisplayOptions *no_image_options;

  ClayUnit           *unit;

  gint               offset_x;         /*  offset of display image            */
  gint               offset_y;

  gdouble            scale_x;          /*  horizontal scale factor            */
  gdouble            scale_y;          /*  vertical scale factor              */

  gboolean           flip_horizontally;
  gboolean           flip_vertically;
  gdouble            rotate_angle;
  cairo_matrix_t    *rotate_transform;
  cairo_matrix_t    *rotate_untransform;

  gdouble            monitor_xres;
  gdouble            monitor_yres;
  gboolean           dot_for_dot;      /*  ignore monitor resolution          */

  ClayZoomModel     *zoom;

  gdouble            last_scale;       /*  scale used when reverting zoom     */
  guint              last_scale_time;  /*  time when last_scale was set       */
  gint               last_offset_x;    /*  offsets used when reverting zoom   */
  gint               last_offset_y;

  gint               disp_width;       /*  width of drawing area              */
  gint               disp_height;      /*  height of drawing area             */

  gboolean           proximity;        /*  is a device in proximity           */

  gboolean           show_image;       /*  whether to show the image          */

  gboolean           show_all;         /*  show the entire image              */

  Selection         *selection;        /*  Selection (marching ants)          */
  gint64             selection_update; /*  Last selection index update        */

  GList             *children;

  GtkWidget         *canvas;           /*  ClayCanvas widget                  */
  GtkGesture        *zoom_gesture;     /*  Zoom gesture handler for the canvas*/
  GtkGesture        *rotate_gesture;   /*  Rotation gesture handler           */

  GtkAdjustment     *hsbdata;          /*  adjustments                        */
  GtkAdjustment     *vsbdata;
  GtkWidget         *hsb;              /*  scroll bars                        */
  GtkWidget         *vsb;

  GtkWidget         *hrule;            /*  rulers                             */
  GtkWidget         *vrule;

  GtkWidget         *origin;           /*  NW: origin                         */
  GtkWidget         *quick_mask_button;/*  SW: quick mask button              */
  GtkWidget         *zoom_button;      /*  NE: zoom toggle button             */
  GtkWidget         *nav_ebox;         /*  SE: navigation event box           */

  GtkWidget         *statusbar;        /*  statusbar                          */

  ClayCanvasItem    *canvas_item;      /*  items drawn on the canvas          */
  ClayCanvasItem    *unrotated_item;   /*  unrotated items for e.g. cursor    */
  ClayCanvasItem    *passe_partout;    /*  item for the highlight             */
  ClayCanvasItem    *preview_items;    /*  item for previews                  */
  ClayCanvasItem    *paths;            /*  item proxy of paths                */
  ClayCanvasItem    *grid;             /*  item proxy of the grid             */
  ClayCanvasItem    *guides;           /*  item proxies of guides             */
  ClayCanvasItem    *sample_points;    /*  item proxies of sample points      */
  ClayCanvasItem    *canvas_boundary;  /*  item for the cabvas boundary       */
  ClayCanvasItem    *layer_boundary;   /*  item for the layer boundary        */
  ClayCanvasItem    *tool_items;       /*  tools items, below the cursor      */
  ClayCanvasItem    *cursor;           /*  item for the software cursor       */

  guint              title_idle_id;    /*  title update idle ID               */
  gchar             *title;            /*  current title                      */
  gchar             *status;           /*  current default statusbar content  */

  guint              fill_idle_id;     /*  display_shell_fill() idle ID       */

  ClayHandedness     cursor_handedness;/*  Handedness for cursor display      */
  ClayCursorType     current_cursor;   /*  Currently installed main cursor    */
  ClayToolCursorType tool_cursor;      /*  Current Tool cursor                */
  ClayCursorModifier cursor_modifier;  /*  Cursor modifier (plus, minus, ...) */

  ClayCursorType     override_cursor;  /*  Overriding cursor                  */
  gboolean           using_override_cursor;
  gboolean           draw_cursor;      /* should we draw software cursor ?    */

  GtkWidget         *close_dialog;     /*  close dialog                       */
  GtkWidget         *scale_dialog;     /*  scale (zoom) dialog                */
  GtkWidget         *rotate_dialog;    /*  rotate dialog                      */
  GtkWidget         *nav_popup;        /*  navigation popup                   */

  ClayColorConfig   *color_config;     /*  color management settings          */
  gboolean           color_config_set; /*  settings changed from defaults     */

  ClayColorTransform *profile_transform;
  GeglBuffer         *profile_buffer;  /*  buffer for profile transform       */
  guchar             *profile_data;    /*  profile_buffer's pixels            */
  gint                profile_stride;  /*  profile_buffer's stride            */

  ClayColorDisplayStack *filter_stack; /*  color display conversion stuff     */
  guint                  filter_idle_id;

  ClayColorTransform *filter_transform;
  const Babl         *filter_format;   /*  filter_buffer's format             */
  ClayColorProfile   *filter_profile;  /*  filter_format's profile            */
  GeglBuffer         *filter_buffer;   /*  buffer for display filters         */
  guchar             *filter_data;     /*  filter_buffer's pixels             */
  gint                filter_stride;   /*  filter_buffer's stride             */

  gint               render_scale;

  cairo_surface_t   *render_cache;
  cairo_region_t    *render_cache_valid;

  gint               render_buf_width;
  gint               render_buf_height;

  cairo_surface_t   *render_surface;   /*  buffer for rendering the mask      */
  cairo_surface_t   *mask_surface;     /*  buffer for rendering the mask      */
  cairo_pattern_t   *checkerboard;     /*  checkerboard pattern               */

  gint               paused_count;

  ClayTreeHandler   *path_freeze_handler;
  ClayTreeHandler   *path_thaw_handler;
  ClayTreeHandler   *path_visible_handler;

  gboolean           zoom_on_resize;

  gboolean           size_allocate_from_configure_event;
  gboolean           size_allocate_center_image;

  /*  the state of clay_display_shell_tool_events()  */
  GdkSeat           *grab_seat;
  GdkDevice         *grab_pointer;
  GdkDevice         *grab_pointer_source;

  /*  the state of clay_display_shell_zoom_gesture_*() */
  gdouble            last_zoom_scale;
  gboolean           zoom_gesture_active;

  /*  the state of clay_display_shell_rotate_gesture_*() */
  guint              last_gesture_rotate_state;
  gdouble            initial_gesture_rotate_angle;
  gboolean           rotate_gesture_active;

  /* Two states are possible when the shell is grabbed: it can be
   * grabbed with space (or space+button1 which is the same state),
   * then if space is released but button1 was still pressed, we wait
   * for button1 to be released as well.
   */
  gboolean           space_release_pending;
  gboolean           button1_release_pending;
  const gchar       *space_shaded_tool;

  /* Modifier action currently ON. */
  ClayModifierAction mod_action;
  gchar             *mod_action_desc;

  gint               scroll_start_x;
  gint               scroll_start_y;
  gint               scroll_last_x;
  gint               scroll_last_y;
  gdouble            rotate_drag_angle;
  gpointer           scroll_info;
  ClayLayer         *picked_layer;

  GeglBuffer        *mask;
  gint               mask_offset_x;
  gint               mask_offset_y;
  GeglColor         *mask_color;
  gboolean           mask_inverted;

  ClayMotionBuffer  *motion_buffer;

  GdkPoint          *zoom_focus_point;

  gboolean           blink;
  guint              blink_timeout_id;

  ClayAlignmentType  snapped_side_horizontal; /* if CLAY_ARRANGE_HFILL then no snapping happen */
  ClayLayer         *snapped_layer_horizontal;

  ClayAlignmentType  snapped_side_vertical; /* if CLAY_ARRANGE_HFILL then no snapping happen */
  ClayLayer         *snapped_layer_vertical;

  ClayAlignmentType  equidistance_side_horizontal;
  ClayLayer         *near_layer_horizontal1;
  ClayLayer         *near_layer_horizontal2;

  ClayAlignmentType  equidistance_side_vertical;
  ClayLayer         *near_layer_vertical1;
  ClayLayer         *near_layer_vertical2;

  gboolean           drawn;

  ClayDisplayShellPrivate *priv;
};

struct _ClayDisplayShellClass
{
  GtkEventBoxClass  parent_class;

  void (* scaled)    (ClayDisplayShell *shell);
  void (* scrolled)  (ClayDisplayShell *shell);
  void (* rotated)   (ClayDisplayShell *shell);
  void (* reconnect) (ClayDisplayShell *shell);
};


GType             clay_display_shell_get_type      (void);

GtkWidget       * clay_display_shell_new           (ClayDisplay        *display,
                                                    ClayUnit           *unit,
                                                    gdouble             scale,
                                                    ClayUIManager      *popup_manager,
                                                    GdkMonitor         *monitor);

void              clay_display_shell_add_overlay   (ClayDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    ClayHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);
void              clay_display_shell_move_overlay  (ClayDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    ClayHandleAnchor    anchor,
                                                    gint                spacing_x,
                                                    gint                spacing_y);

ClayImageWindow * clay_display_shell_get_window    (ClayDisplayShell   *shell);
ClayStatusbar   * clay_display_shell_get_statusbar (ClayDisplayShell   *shell);

ClayColorConfig * clay_display_shell_get_color_config
                                                   (ClayDisplayShell   *shell);

void              clay_display_shell_present       (ClayDisplayShell   *shell);

void              clay_display_shell_reconnect     (ClayDisplayShell   *shell);

void              clay_display_shell_empty         (ClayDisplayShell   *shell);
void              clay_display_shell_fill          (ClayDisplayShell   *shell,
                                                    ClayImage          *image,
                                                    ClayUnit           *unit,
                                                    gdouble             scale);

void              clay_display_shell_scaled        (ClayDisplayShell   *shell);
void              clay_display_shell_scrolled      (ClayDisplayShell   *shell);
void              clay_display_shell_rotated       (ClayDisplayShell   *shell);

void              clay_display_shell_set_unit      (ClayDisplayShell   *shell,
                                                    ClayUnit           *unit);
ClayUnit        * clay_display_shell_get_unit      (ClayDisplayShell   *shell);

gboolean          clay_display_shell_snap_coords   (ClayDisplayShell   *shell,
                                                    ClayCoords         *coords,
                                                    gint                snap_offset_x,
                                                    gint                snap_offset_y,
                                                    gint                snap_width,
                                                    gint                snap_height);

gboolean          clay_display_shell_mask_bounds   (ClayDisplayShell   *shell,
                                                    gint               *x,
                                                    gint               *y,
                                                    gint               *width,
                                                    gint               *height);

void              clay_display_shell_set_show_image
                                                   (ClayDisplayShell   *shell,
                                                    gboolean            show_image);

void              clay_display_shell_set_show_all  (ClayDisplayShell   *shell,
                                                    gboolean            show_all);

ClayPickable    * clay_display_shell_get_pickable  (ClayDisplayShell   *shell);
ClayPickable    * clay_display_shell_get_canvas_pickable
                                                   (ClayDisplayShell   *shell);
GeglRectangle     clay_display_shell_get_bounding_box
                                                   (ClayDisplayShell   *shell);
gboolean          clay_display_shell_get_infinite_canvas
                                                   (ClayDisplayShell   *shell);

void              clay_display_shell_update_priority_rect
                                                   (ClayDisplayShell *shell);

void              clay_display_shell_flush         (ClayDisplayShell   *shell);

void              clay_display_shell_pause         (ClayDisplayShell   *shell);
void              clay_display_shell_resume        (ClayDisplayShell   *shell);

void              clay_display_shell_set_highlight (ClayDisplayShell   *shell,
                                                    const GdkRectangle *highlight,
                                                    double              opacity);
void              clay_display_shell_set_mask      (ClayDisplayShell   *shell,
                                                    GeglBuffer         *mask,
                                                    gint                offset_x,
                                                    gint                offset_y,
                                                    GeglColor          *color,
                                                    gboolean            inverted);

gboolean          clay_display_shell_is_drawn      (ClayDisplayShell   *shell);

void              clay_display_shell_get_overlay_corners
                                                   (ClayDisplayShell   *shell,
                                                    GtkWidget          *child,
                                                    gdouble             image_x,
                                                    gdouble             image_y,
                                                    gdouble            *top_left_x,
                                                    gdouble            *top_left_y,
                                                    gdouble            *bottom_right_x,
                                                    gdouble            *bottom_right_y);
