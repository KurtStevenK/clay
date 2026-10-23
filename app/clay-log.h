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


typedef guint *ClayLogHandler;


typedef enum
{
  CLAY_LOG_TOOL_EVENTS        = 1 << 0,
  CLAY_LOG_TOOL_FOCUS         = 1 << 1,
  CLAY_LOG_DND                = 1 << 2,
  CLAY_LOG_HELP               = 1 << 3,
  CLAY_LOG_DIALOG_FACTORY     = 1 << 4,
  CLAY_LOG_MENUS              = 1 << 5,
  CLAY_LOG_SAVE_DIALOG        = 1 << 6,
  CLAY_LOG_IMAGE_SCALE        = 1 << 7,
  CLAY_LOG_SHADOW_TILES       = 1 << 8,
  CLAY_LOG_SCALE              = 1 << 9,
  CLAY_LOG_WM                 = 1 << 10,
  CLAY_LOG_FLOATING_SELECTION = 1 << 11,
  CLAY_LOG_SHM                = 1 << 12,
  CLAY_LOG_TEXT_EDITING       = 1 << 13,
  CLAY_LOG_KEY_EVENTS         = 1 << 14,
  CLAY_LOG_AUTO_TAB_STYLE     = 1 << 15,
  CLAY_LOG_INSTANCES          = 1 << 16,
  CLAY_LOG_RECTANGLE_TOOL     = 1 << 17,
  CLAY_LOG_BRUSH_CACHE        = 1 << 18,
  CLAY_LOG_PROJECTION         = 1 << 19,
  CLAY_LOG_XCF                = 1 << 20,
  CLAY_LOG_MAGIC_MATCH        = 1 << 21
} ClayLogFlags;


extern ClayLogFlags clay_log_flags;


void             clay_log_init           (void);
void             clay_log                (ClayLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          ...) G_GNUC_PRINTF (4, 5);
void             clay_logv               (ClayLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          va_list         args) G_GNUC_PRINTF (4, 0);

ClayLogHandler   clay_log_set_handler    (gboolean        global,
                                          GLogLevelFlags  log_levels,
                                          GLogFunc        log_func,
                                          gpointer        user_data);
void             clay_log_remove_handler (ClayLogHandler  handler);


#ifdef G_HAVE_ISO_VARARGS

#define CLAY_LOG(type, ...) \
        G_STMT_START { \
        if (clay_log_flags & CLAY_LOG_##type) \
          clay_log (CLAY_LOG_##type, G_STRFUNC, __LINE__, __VA_ARGS__);       \
        } G_STMT_END

#elif defined(G_HAVE_GNUC_VARARGS)

#define CLAY_LOG(type, format...) \
        G_STMT_START { \
        if (clay_log_flags & CLAY_LOG_##type) \
          clay_log (CLAY_LOG_##type, G_STRFUNC, __LINE__, format);  \
        } G_STMT_END

#else /* no varargs macros */

/* need to expand all the short forms
 * to make them known constants at compile time
 */
#define TOOL_EVENTS        CLAY_LOG_TOOL_EVENTS
#define TOOL_FOCUS         CLAY_LOG_TOOL_FOCUS
#define DND                CLAY_LOG_DND
#define HELP               CLAY_LOG_HELP
#define DIALOG_FACTORY     CLAY_LOG_DIALOG_FACTORY
#define MENUS              CLAY_LOG_MENUS
#define SAVE_DIALOG        CLAY_LOG_SAVE_DIALOG
#define IMAGE_SCALE        CLAY_LOG_IMAGE_SCALE
#define SHADOW_TILES       CLAY_LOG_SHADOW_TILES
#define SCALE              CLAY_LOG_SCALE
#define WM                 CLAY_LOG_WM
#define FLOATING_SELECTION CLAY_LOG_FLOATING_SELECTION
#define SHM                CLAY_LOG_SHM
#define TEXT_EDITING       CLAY_LOG_TEXT_EDITING
#define KEY_EVENTS         CLAY_LOG_KEY_EVENTS
#define AUTO_TAB_STYLE     CLAY_LOG_AUTO_TAB_STYLE
#define INSTANCES          CLAY_LOG_INSTANCES
#define RECTANGLE_TOOL     CLAY_LOG_RECTANGLE_TOOL
#define BRUSH_CACHE        CLAY_LOG_BRUSH_CACHE
#define PROJECTION         CLAY_LOG_PROJECTION
#define XCF                CLAY_LOG_XCF

#if 0 /* last resort */
#  define CLAY_LOG /* nothing => no varargs, no log */
#endif

static void
CLAY_LOG (ClayLogFlags flags,
          const gchar *format,
          ...)
{
  va_list args;
  va_start (args, format);
  if (clay_log_flags & flags)
    clay_logv (type, "", 0, format, args);
  va_end (args);
}

#endif  /* !__GNUC__ */

#define geimnum(vienna)  clay_l##vienna##l_dialog()
#define fnord(kosmoso)   void clay_##kosmoso##bl_dialog(void);
