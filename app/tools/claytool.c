/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#ifdef _WIN32
#include <stdio.h>
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/claylinklayer.h"
#include "core/clayprogress.h"
#include "core/claytoolinfo.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-cursor.h"
#include "display/claystatusbar.h"

#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "widgets/claywidgets-utils.h"

#include "claysourcetool.h"
#include "claytool.h"
#include "claytool-progress.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-log.h"
#include "clay-intl.h"


/* #define DEBUG_ACTIVE_STATE 1 */


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void            clay_tool_constructed         (GObject          *object);
static void            clay_tool_dispose             (GObject          *object);
static void            clay_tool_finalize            (GObject          *object);
static void            clay_tool_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void            clay_tool_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean        clay_tool_real_has_display    (ClayTool         *tool,
                                                      ClayDisplay      *display);
static ClayDisplay   * clay_tool_real_has_image      (ClayTool         *tool,
                                                      ClayImage        *image);
static gboolean        clay_tool_real_initialize     (ClayTool         *tool,
                                                      ClayDisplay      *display,
                                                      GError          **error);
static void            clay_tool_real_control        (ClayTool         *tool,
                                                      ClayToolAction    action,
                                                      ClayDisplay      *display);
static void            clay_tool_real_button_press   (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      ClayButtonPressType press_type,
                                                      ClayDisplay      *display);
static void            clay_tool_real_button_release (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      ClayButtonReleaseType release_type,
                                                      ClayDisplay      *display);
static void            clay_tool_real_motion         (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      ClayDisplay      *display);
static gboolean        clay_tool_real_key_press      (ClayTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      ClayDisplay      *display);
static gboolean        clay_tool_real_key_release    (ClayTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      ClayDisplay      *display);
static void            clay_tool_real_modifier_key   (ClayTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      ClayDisplay      *display);
static void       clay_tool_real_active_modifier_key (ClayTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      ClayDisplay      *display);
static void            clay_tool_real_oper_update    (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      GdkModifierType   state,
                                                      gboolean          proximity,
                                                      ClayDisplay      *display);
static void            clay_tool_real_cursor_update  (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      GdkModifierType   state,
                                                      ClayDisplay      *display);
static const gchar   * clay_tool_real_can_undo       (ClayTool         *tool,
                                                      ClayDisplay      *display);
static const gchar   * clay_tool_real_can_redo       (ClayTool         *tool,
                                                      ClayDisplay      *display);
static gboolean        clay_tool_real_undo           (ClayTool         *tool,
                                                      ClayDisplay      *display);
static gboolean        clay_tool_real_redo           (ClayTool         *tool,
                                                      ClayDisplay      *display);
static ClayUIManager * clay_tool_real_get_popup      (ClayTool         *tool,
                                                      const ClayCoords *coords,
                                                      GdkModifierType   state,
                                                      ClayDisplay      *display,
                                                      const gchar     **ui_path);
static void            clay_tool_real_options_notify (ClayTool         *tool,
                                                      ClayToolOptions  *options,
                                                      const GParamSpec *pspec);

static void            clay_tool_options_notify      (ClayToolOptions  *options,
                                                      const GParamSpec *pspec,
                                                      ClayTool         *tool);
static void            clay_tool_clear_status        (ClayTool         *tool);
static void            clay_tool_release             (ClayTool         *tool);


G_DEFINE_TYPE_WITH_CODE (ClayTool, clay_tool, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_tool_progress_iface_init))

#define parent_class clay_tool_parent_class

static gint global_tool_ID = 1;


static void
clay_tool_class_init (ClayToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_tool_constructed;
  object_class->dispose      = clay_tool_dispose;
  object_class->finalize     = clay_tool_finalize;
  object_class->set_property = clay_tool_set_property;
  object_class->get_property = clay_tool_get_property;

  klass->has_display         = clay_tool_real_has_display;
  klass->has_image           = clay_tool_real_has_image;
  klass->initialize          = clay_tool_real_initialize;
  klass->control             = clay_tool_real_control;
  klass->button_press        = clay_tool_real_button_press;
  klass->button_release      = clay_tool_real_button_release;
  klass->motion              = clay_tool_real_motion;
  klass->key_press           = clay_tool_real_key_press;
  klass->key_release         = clay_tool_real_key_release;
  klass->modifier_key        = clay_tool_real_modifier_key;
  klass->active_modifier_key = clay_tool_real_active_modifier_key;
  klass->oper_update         = clay_tool_real_oper_update;
  klass->cursor_update       = clay_tool_real_cursor_update;
  klass->can_undo            = clay_tool_real_can_undo;
  klass->can_redo            = clay_tool_real_can_redo;
  klass->undo                = clay_tool_real_undo;
  klass->redo                = clay_tool_real_redo;
  klass->get_popup           = clay_tool_real_get_popup;
  klass->options_notify      = clay_tool_real_options_notify;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        CLAY_TYPE_TOOL_INFO,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  klass->is_destructive = TRUE;
}

static void
clay_tool_init (ClayTool *tool)
{
  tool->tool_info             = NULL;
  tool->ID                    = global_tool_ID++;
  tool->control               = g_object_new (CLAY_TYPE_TOOL_CONTROL, NULL);
  tool->display               = NULL;
  tool->drawables             = NULL;
  tool->focus_display         = NULL;
  tool->modifier_state        = 0;
  tool->active_modifier_state = 0;
  tool->button_press_state    = 0;
}

static void
clay_tool_constructed (GObject *object)
{
  ClayTool *tool = CLAY_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_TOOL_INFO (tool->tool_info));

  g_signal_connect_object (clay_tool_get_options (tool), "notify",
                           G_CALLBACK (clay_tool_options_notify),
                           tool, 0);
}

static void
clay_tool_dispose (GObject *object)
{
  ClayTool *tool = CLAY_TOOL (object);

  clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_tool_finalize (GObject *object)
{
  ClayTool *tool = CLAY_TOOL (object);

  g_clear_object (&tool->tool_info);
  g_clear_object (&tool->control);
  g_clear_pointer (&tool->label,     g_free);
  g_clear_pointer (&tool->undo_desc, g_free);
  g_clear_pointer (&tool->icon_name, g_free);
  g_clear_pointer (&tool->help_id,   g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_tool_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayTool *tool = CLAY_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      tool->tool_info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  ClayTool *tool = CLAY_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      g_value_set_object (value, tool->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  standard member functions  */

static gboolean
clay_tool_real_has_display (ClayTool    *tool,
                            ClayDisplay *display)
{
  return (display == tool->display ||
          g_list_find (tool->status_displays, display));
}

static ClayDisplay *
clay_tool_real_has_image (ClayTool  *tool,
                          ClayImage *image)
{
  if (tool->display)
    {
      if (image && clay_display_get_image (tool->display) == image)
        return tool->display;

      /*  NULL image means any display  */
      if (! image)
        return tool->display;
    }

  return NULL;
}

static gboolean
clay_tool_real_initialize (ClayTool     *tool,
                           ClayDisplay  *display,
                           GError      **error)
{
  return TRUE;
}

static void
clay_tool_real_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      tool->display   = NULL;
      g_list_free (tool->drawables);
      tool->drawables = NULL;
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }
}

static void
clay_tool_real_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    {
      ClayImage *image = clay_display_get_image (display);

      tool->display   = display;
      g_list_free (tool->drawables);
      tool->drawables = clay_image_get_selected_drawables (image);

      clay_tool_control_activate (tool->control);
    }
}

static void
clay_tool_real_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  clay_tool_control_halt (tool->control);
}

static void
clay_tool_real_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
}

static gboolean
clay_tool_real_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  return FALSE;
}

static gboolean
clay_tool_real_key_release (ClayTool    *tool,
                            GdkEventKey *kevent,
                            ClayDisplay *display)
{
  return FALSE;
}

static void
clay_tool_real_modifier_key (ClayTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             ClayDisplay     *display)
{
}

static void
clay_tool_real_active_modifier_key (ClayTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    ClayDisplay     *display)
{
}

static void
clay_tool_real_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
}

static void
clay_tool_real_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  clay_tool_set_cursor (tool, display,
                        clay_tool_control_get_cursor (tool->control),
                        clay_tool_control_get_tool_cursor (tool->control),
                        clay_tool_control_get_cursor_modifier (tool->control));
}

static const gchar *
clay_tool_real_can_undo (ClayTool    *tool,
                         ClayDisplay *display)
{
  return NULL;
}

static const gchar *
clay_tool_real_can_redo (ClayTool    *tool,
                         ClayDisplay *display)
{
  return NULL;
}

static gboolean
clay_tool_real_undo (ClayTool    *tool,
                     ClayDisplay *display)
{
  return FALSE;
}

static gboolean
clay_tool_real_redo (ClayTool    *tool,
                     ClayDisplay *display)
{
  return FALSE;
}

static ClayUIManager *
clay_tool_real_get_popup (ClayTool         *tool,
                          const ClayCoords *coords,
                          GdkModifierType   state,
                          ClayDisplay      *display,
                          const gchar     **ui_path)
{
  *ui_path = NULL;

  return NULL;
}

static void
clay_tool_real_options_notify (ClayTool         *tool,
                               ClayToolOptions  *options,
                               const GParamSpec *pspec)
{
}


/*  public functions  */

ClayToolOptions *
clay_tool_get_options (ClayTool *tool)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);
  g_return_val_if_fail (CLAY_IS_TOOL_INFO (tool->tool_info), NULL);

  return tool->tool_info->tool_options;
}

void
clay_tool_set_label (ClayTool    *tool,
                     const gchar *label)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  g_free (tool->label);
  tool->label = g_strdup (label);
}

const gchar *
clay_tool_get_label (ClayTool *tool)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);

  if (tool->label)
    return tool->label;

  return tool->tool_info->label;
}

void
clay_tool_set_undo_desc (ClayTool    *tool,
                         const gchar *undo_desc)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  g_free (tool->undo_desc);
  tool->undo_desc = g_strdup (undo_desc);
}

const gchar *
clay_tool_get_undo_desc (ClayTool *tool)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);

  if (tool->undo_desc)
    return tool->undo_desc;

  return tool->tool_info->label;
}

void
clay_tool_set_icon_name (ClayTool    *tool,
                         const gchar *icon_name)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  g_free (tool->icon_name);
  tool->icon_name = g_strdup (icon_name);
}

const gchar *
clay_tool_get_icon_name (ClayTool *tool)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);

  if (tool->icon_name)
    return tool->icon_name;

  return clay_viewable_get_icon_name (CLAY_VIEWABLE (tool->tool_info));
}

void
clay_tool_set_help_id (ClayTool    *tool,
                       const gchar *help_id)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  g_free (tool->help_id);
  tool->help_id = g_strdup (help_id);
}

const gchar *
clay_tool_get_help_id (ClayTool *tool)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);

  if (tool->help_id)
    return tool->help_id;

  return tool->tool_info->help_id;
}

gboolean
clay_tool_has_display (ClayTool    *tool,
                       ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  return CLAY_TOOL_GET_CLASS (tool)->has_display (tool, display);
}

ClayDisplay *
clay_tool_has_image (ClayTool  *tool,
                     ClayImage *image)
{
  ClayDisplay *display;

  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), NULL);

  display = CLAY_TOOL_GET_CLASS (tool)->has_image (tool, image);

  /*  check status displays last because they don't affect the tool
   *  itself (unlike tool->display or draw_tool->display)
   */
  if (! display && tool->status_displays)
    {
      GList *list;

      for (list = tool->status_displays; list; list = g_list_next (list))
        {
          ClayDisplay *status_display = list->data;

          if (clay_display_get_image (status_display) == image)
            return status_display;
        }

      /*  NULL image means any display  */
      if (! image)
        return tool->status_displays->data;
    }

  return display;
}

gboolean
clay_tool_initialize (ClayTool    *tool,
                      ClayDisplay *display)
{
  GError *error = NULL;

  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  if (! CLAY_TOOL_GET_CLASS (tool)->initialize (tool, display, &error))
    {
      if (error)
        {
          clay_tool_message_literal (tool, display, error->message);
          g_clear_error (&error);
        }

      return FALSE;
    }

  return TRUE;
}

void
clay_tool_control (ClayTool       *tool,
                   ClayToolAction  action,
                   ClayDisplay    *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  g_object_ref (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
      if (! clay_tool_control_is_paused (tool->control))
        CLAY_TOOL_GET_CLASS (tool)->control (tool, action, display);

      clay_tool_control_pause (tool->control);
      break;

    case CLAY_TOOL_ACTION_RESUME:
      if (clay_tool_control_is_paused (tool->control))
        {
          clay_tool_control_resume (tool->control);

          if (! clay_tool_control_is_paused (tool->control))
            CLAY_TOOL_GET_CLASS (tool)->control (tool, action, display);
        }
      else
        {
          g_warning ("clay_tool_control: unable to RESUME tool with "
                     "tool->control->paused_count == 0");
        }
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_tool_release (tool);

      CLAY_TOOL_GET_CLASS (tool)->control (tool, action, display);

      /*  always HALT after COMMIT here and not in each tool individually;
       *  some tools interact with their subclasses (e.g. filter tool
       *  and operation tool), and it's essential that COMMIT runs
       *  through the entire class hierarchy before HALT
       */
      action = CLAY_TOOL_ACTION_HALT;

      /* pass through */
    case CLAY_TOOL_ACTION_HALT:
      clay_tool_release (tool);

      CLAY_TOOL_GET_CLASS (tool)->control (tool, action, display);

      if (clay_tool_control_is_active (tool->control))
        clay_tool_control_halt (tool->control);

      clay_tool_clear_status (tool);
      break;
    }

  g_object_unref (tool);
}

void
clay_tool_button_press (ClayTool            *tool,
                        const ClayCoords    *coords,
                        guint32              time,
                        GdkModifierType      state,
                        ClayButtonPressType  press_type,
                        ClayDisplay         *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  if (CLAY_TOOL_GET_CLASS (tool)->is_destructive)
    {
      ClayImage *image;
      GList     *drawables;

      image     = clay_display_get_image (display);
      drawables = clay_image_get_selected_drawables (image);
      for (GList *iter = drawables; iter; iter = iter->next)
        {
          ClayDrawable *drawable    = iter->data;
          ClayItem     *locked_item = NULL;

          if (clay_item_is_vector_layer (CLAY_ITEM (drawable)) ||
              clay_item_is_link_layer (CLAY_ITEM (drawable))   ||
              clay_item_is_text_layer (CLAY_ITEM (drawable))   ||
              clay_item_is_content_locked (CLAY_ITEM (drawable), &locked_item))
            {
              gboolean constrain_only;

              /* Allow vector/link or pixel-locked layers to be set as sources */
              constrain_only = (state & clay_get_constrain_behavior_mask () &&
                                ! (state & clay_get_extend_selection_mask ()));
              if (! CLAY_IS_SOURCE_TOOL (tool) || ! constrain_only)
                {
                  /* TRANSLATORS: this is a menu path. Make sure you use
                   * the same translations you used for the "Rasterize"
                   * action under the "Layer" menu.
                   */
                  gchar *menu_path = _("Layer > Rasterize");

                  if (clay_item_is_text_layer (CLAY_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      clay_tool_message (tool, display, _("Text layers must be rasterized (%s)."), menu_path);
                      clay_tools_blink_item (display->clay, CLAY_ITEM (drawable));
                    }
                  else if (clay_item_is_link_layer (CLAY_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      clay_tool_message (tool, display, _("Link layers must be rasterized (%s)."), menu_path);
                      clay_tools_blink_item (display->clay, CLAY_ITEM (drawable));
                    }
                  else if (clay_item_is_vector_layer (CLAY_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      clay_tool_message (tool, display, _("Vector layers must be rasterized (%s)."), menu_path);
                      clay_tools_blink_item (display->clay, CLAY_ITEM (drawable));
                    }
                  else
                    {
                      clay_tool_message_literal (tool, display,
                                                 _("The selected item's pixels are locked."));
                      clay_tools_blink_lock_box (display->clay, locked_item);
                    }

                  g_list_free (drawables);
                  return;
                }
            }
        }
      g_list_free (drawables);
    }

  CLAY_TOOL_GET_CLASS (tool)->button_press (tool, coords, time, state,
                                            press_type, display);

  if (press_type == CLAY_BUTTON_PRESS_NORMAL &&
      clay_tool_control_is_active (tool->control))
    {
      tool->button_press_state    = state;
      tool->active_modifier_state = state;

      tool->last_pointer_coords = *coords;
      tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
      tool->last_pointer_state  = state;

      if (clay_tool_control_get_wants_click (tool->control))
        {
          tool->in_click_distance   = TRUE;
          tool->got_motion_event    = FALSE;
          tool->button_press_coords = *coords;
          tool->button_press_time   = time;
        }
      else
        {
          tool->in_click_distance   = FALSE;
        }
    }
}

static gboolean
clay_tool_check_click_distance (ClayTool         *tool,
                                const ClayCoords *coords,
                                guint32           time,
                                ClayDisplay      *display)
{
  ClayDisplayShell *shell;
  gint              double_click_time;
  gint              double_click_distance;

  if (! tool->in_click_distance)
    return FALSE;

  shell = clay_display_get_shell (display);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (shell)),
                "gtk-double-click-time",     &double_click_time,
                "gtk-double-click-distance", &double_click_distance,
                NULL);

  if ((time - tool->button_press_time) > double_click_time)
    {
      tool->in_click_distance = FALSE;
    }
  else
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);
      gdouble           dx;
      gdouble           dy;

      dx = SCALEX (shell, tool->button_press_coords.x - coords->x);
      dy = SCALEY (shell, tool->button_press_coords.y - coords->y);

      if ((SQR (dx) + SQR (dy)) > SQR (double_click_distance))
        {
          tool->in_click_distance = FALSE;
        }
    }

  return tool->in_click_distance;
}

void
clay_tool_button_release (ClayTool         *tool,
                          const ClayCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          ClayDisplay      *display)
{
  ClayButtonReleaseType release_type = CLAY_BUTTON_RELEASE_NORMAL;
  ClayCoords            my_coords;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == TRUE);

  g_object_ref (tool);

  tool->last_pointer_state = 0;

  my_coords = *coords;

  if (state & GDK_BUTTON3_MASK)
    {
      release_type = CLAY_BUTTON_RELEASE_CANCEL;
    }
  else if (clay_tool_control_get_wants_click (tool->control))
    {
      if (clay_tool_check_click_distance (tool, coords, time, display))
        {
          release_type = CLAY_BUTTON_RELEASE_CLICK;
          my_coords    = tool->button_press_coords;

          if (tool->got_motion_event)
            {
              /*  if there has been a motion() since button_press(),
               *  synthesize a motion() back to the recorded press
               *  coordinates
               */
              CLAY_TOOL_GET_CLASS (tool)->motion (tool, &my_coords, time,
                                                  state & GDK_BUTTON1_MASK,
                                                  display);
            }
        }
      else if (! tool->got_motion_event)
        {
          release_type = CLAY_BUTTON_RELEASE_NO_MOTION;
        }
    }

  CLAY_TOOL_GET_CLASS (tool)->button_release (tool, &my_coords, time, state,
                                              release_type, display);

  g_warn_if_fail (clay_tool_control_is_active (tool->control) == FALSE);

  if (tool->active_modifier_state                            != 0 &&
      clay_tool_control_get_active_modifiers (tool->control) !=
      CLAY_TOOL_ACTIVE_MODIFIERS_SAME)
    {
      clay_tool_control_activate (tool->control);

      clay_tool_set_active_modifier_state (tool, 0, display);

      clay_tool_control_halt (tool->control);
    }

  tool->button_press_state    = 0;
  tool->active_modifier_state = 0;

  g_object_unref (tool);
}

void
clay_tool_motion (ClayTool         *tool,
                  const ClayCoords *coords,
                  guint32           time,
                  GdkModifierType   state,
                  ClayDisplay      *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == TRUE);

  tool->got_motion_event = TRUE;

  tool->last_pointer_coords = *coords;
  tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
  tool->last_pointer_state  = state;

  CLAY_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, display);
}

void
clay_tool_set_focus_display (ClayTool    *tool,
                             ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (display == NULL || CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == FALSE);

  CLAY_LOG (TOOL_FOCUS, "tool: %p  focus_display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  if (display != tool->focus_display)
    {
      if (tool->focus_display)
        {
          if (tool->active_modifier_state != 0)
            {
              clay_tool_control_activate (tool->control);

              clay_tool_set_active_modifier_state (tool, 0, tool->focus_display);

              clay_tool_control_halt (tool->control);
            }

          if (tool->modifier_state != 0)
            clay_tool_set_modifier_state (tool, 0, tool->focus_display);
        }

      tool->focus_display = display;
    }
}

gboolean
clay_tool_key_press (ClayTool    *tool,
                     GdkEventKey *kevent,
                     ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (clay_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return CLAY_TOOL_GET_CLASS (tool)->key_press (tool, kevent, display);
}

gboolean
clay_tool_key_release (ClayTool    *tool,
                       GdkEventKey *kevent,
                       ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (clay_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return CLAY_TOOL_GET_CLASS (tool)->key_release (tool, kevent, display);
}

static void
clay_tool_modifier_key (ClayTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
                        GdkModifierType  state,
                        ClayDisplay     *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  CLAY_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, display);
}

static void
clay_tool_active_modifier_key (ClayTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               ClayDisplay     *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  CLAY_TOOL_GET_CLASS (tool)->active_modifier_key (tool, key, press, state,
                                                   display);
}

static gboolean
state_changed (GdkModifierType  old_state,
               GdkModifierType  new_state,
               GdkModifierType  modifier,
               gboolean        *press)
{
  if ((old_state & modifier) != (new_state & modifier))
    {
      *press = (new_state & modifier) ? TRUE : FALSE;

      return TRUE;
    }

  return FALSE;
}

void
clay_tool_set_modifier_state (ClayTool        *tool,
                              GdkModifierType  state,
                              ClayDisplay     *display)
{
  gboolean press;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == FALSE);

  CLAY_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  if (state_changed (tool->modifier_state, state, GDK_SHIFT_MASK, &press))
    {
      clay_tool_modifier_key (tool, GDK_SHIFT_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_CONTROL_MASK, &press))
    {
      clay_tool_modifier_key (tool, GDK_CONTROL_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD1_MASK, &press))
    {
      clay_tool_modifier_key (tool, GDK_MOD1_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD2_MASK, &press))
    {
      clay_tool_modifier_key (tool, GDK_MOD2_MASK,
                              press, state,
                              display);
    }

  tool->modifier_state = state;
}

void
clay_tool_set_active_modifier_state (ClayTool        *tool,
                                     GdkModifierType  state,
                                     ClayDisplay     *display)
{
  ClayToolActiveModifiers active_modifiers;
  gboolean                press;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == TRUE);

  CLAY_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  active_modifiers = clay_tool_control_get_active_modifiers (tool->control);

  if (state_changed (tool->active_modifier_state, state, GDK_SHIFT_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: SHIFT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case CLAY_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SAME:
          clay_tool_modifier_key (tool, GDK_SHIFT_MASK,
                                  press, state,
                                  display);
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_SHIFT_MASK))
            {
              tool->button_press_state &= ~GDK_SHIFT_MASK;
            }
          else
            {
              clay_tool_active_modifier_key (tool, GDK_SHIFT_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_CONTROL_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: CONTROL %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case CLAY_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SAME:
          clay_tool_modifier_key (tool, GDK_CONTROL_MASK,
                                  press, state,
                                  display);
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_CONTROL_MASK))
            {
              tool->button_press_state &= ~GDK_CONTROL_MASK;
            }
          else
            {
              clay_tool_active_modifier_key (tool, GDK_CONTROL_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_MOD1_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: ALT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case CLAY_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SAME:
          clay_tool_modifier_key (tool, GDK_MOD1_MASK,
                                  press, state,
                                  display);
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD1_MASK))
            {
              tool->button_press_state &= ~GDK_MOD1_MASK;
            }
          else
            {
              clay_tool_active_modifier_key (tool, GDK_MOD1_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_MOD2_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: MOD2 %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case CLAY_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SAME:
          clay_tool_modifier_key (tool, GDK_MOD2_MASK,
                                  press, state,
                                  display);
          break;

        case CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD2_MASK))
            {
              tool->button_press_state &= ~GDK_MOD2_MASK;
            }
          else
            {
              clay_tool_active_modifier_key (tool, GDK_MOD2_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  tool->active_modifier_state = state;

  if (active_modifiers == CLAY_TOOL_ACTIVE_MODIFIERS_SAME)
    tool->modifier_state = state;
}

void
clay_tool_oper_update (ClayTool         *tool,
                       const ClayCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity,
                       ClayDisplay      *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == FALSE);

  CLAY_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, proximity,
                                           display);

  if (G_UNLIKELY (clay_image_is_empty (clay_display_get_image (display)) &&
                  ! clay_tool_control_get_handle_empty_image (tool->control)))
    {
      clay_tool_replace_status (tool, display,
                                "%s",
                                _("Can't work on an empty image, "
                                  "add a layer first"));
    }
}

void
clay_tool_cursor_update (ClayTool         *tool,
                         const ClayCoords *coords,
                         GdkModifierType   state,
                         ClayDisplay      *display)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_tool_control_is_active (tool->control) == FALSE);

  CLAY_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, display);
}

const gchar *
clay_tool_can_undo (ClayTool    *tool,
                    ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return CLAY_TOOL_GET_CLASS (tool)->can_undo (tool, display);

  return NULL;
}

const gchar *
clay_tool_can_redo (ClayTool    *tool,
                    ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return CLAY_TOOL_GET_CLASS (tool)->can_redo (tool, display);

  return NULL;
}

gboolean
clay_tool_undo (ClayTool    *tool,
                ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  if (clay_tool_can_undo (tool, display))
    return CLAY_TOOL_GET_CLASS (tool)->undo (tool, display);

  return FALSE;
}

gboolean
clay_tool_redo (ClayTool    *tool,
                ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  if (clay_tool_can_redo (tool, display))
    return CLAY_TOOL_GET_CLASS (tool)->redo (tool, display);

  return FALSE;
}

ClayUIManager *
clay_tool_get_popup (ClayTool         *tool,
                     const ClayCoords *coords,
                     GdkModifierType   state,
                     ClayDisplay      *display,
                     const gchar     **ui_path)
{
  g_return_val_if_fail (CLAY_IS_TOOL (tool), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  return CLAY_TOOL_GET_CLASS (tool)->get_popup (tool, coords, state, display,
                                                ui_path);
}

/*
 * clay_tool_push_status:
 * @tool:
 * @display:
 * @format:
 *
 * Push a new status message for the context of the @tool, in the shell
 * associated to @display. Note that the message will replace any
 * message for the same context (i.e. same tool) while also ensuring it
 * is in the front of the message list (except for any progress or
 * temporary messages which might be going on and are always front).
 * This is the main difference with clay_tool_replace_status() which
 * does not necessarily try to make it a front message.
 *
 * In particular, it means you don't have to call clay_tool_pop_status()
 * first before calling clay_tool_push_status(). Even more, you should
 * not pop the status if you were planning to push a new status for the
 * same context because you are triggering non necessary redraws of the
 * status bar.
 */
void
clay_tool_push_status (ClayTool    *tool,
                       ClayDisplay *display,
                       const gchar *format,
                       ...)
{
  ClayDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = clay_display_get_shell (display);

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool->tool_info));

  va_start (args, format);

  clay_statusbar_push_valist (clay_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
clay_tool_push_status_coords (ClayTool            *tool,
                              ClayDisplay         *display,
                              ClayCursorPrecision  precision,
                              const gchar         *title,
                              gdouble              x,
                              const gchar         *separator,
                              gdouble              y,
                              const gchar         *help)
{
  ClayDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  shell = clay_display_get_shell (display);

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool->tool_info));

  clay_statusbar_push_coords (clay_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              precision, title, x, separator, y,
                              help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
clay_tool_push_status_length (ClayTool            *tool,
                              ClayDisplay         *display,
                              const gchar         *title,
                              ClayOrientationType  axis,
                              gdouble              value,
                              const gchar         *help)
{
  ClayDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  shell = clay_display_get_shell (display);

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool->tool_info));

  clay_statusbar_push_length (clay_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              title, axis, value, help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

/*
 * clay_tool_replace_status:
 * @tool:
 * @display:
 * @format:
 *
 * Push a new status message for the context of the @tool, in the shell
 * associated to @display. If any message for the same context (i.e.
 * same tool) is already lined up, it will be replaced without changing
 * the appearance order. In other words, it doesn't try to prioritize
 * this new status message. Yet if no message for the same context
 * existed already, the new status would end up front (except for any
 * progress or temporary messages which might be going on and are always
 * front).
 * See also clay_tool_push_status().
 *
 * Therefore you should not call clay_tool_pop_status() first before
 * calling clay_tool_replace_status() as it would not be the same
 * behavior and could trigger unnecessary redraws of the status bar.
 */
void
clay_tool_replace_status (ClayTool    *tool,
                          ClayDisplay *display,
                          const gchar *format,
                          ...)
{
  ClayDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = clay_display_get_shell (display);

  icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool->tool_info));

  va_start (args, format);

  clay_statusbar_replace_valist (clay_display_shell_get_statusbar (shell),
                                 G_OBJECT_TYPE_NAME (tool), icon_name,
                                 format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
clay_tool_pop_status (ClayTool    *tool,
                      ClayDisplay *display)
{
  ClayDisplayShell *shell;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  shell = clay_display_get_shell (display);

  clay_statusbar_pop (clay_display_shell_get_statusbar (shell),
                      G_OBJECT_TYPE_NAME (tool));

  tool->status_displays = g_list_remove (tool->status_displays, display);
}

void
clay_tool_message (ClayTool    *tool,
                   ClayDisplay *display,
                   const gchar *format,
                   ...)
{
  va_list args;

  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  clay_message_valist (display->clay, G_OBJECT (display),
                       CLAY_MESSAGE_WARNING, format, args);

  va_end (args);
}

void
clay_tool_message_literal (ClayTool    *tool,
                           ClayDisplay *display,
                           const gchar *message)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (message != NULL);

  clay_message_literal (display->clay, G_OBJECT (display),
                        CLAY_MESSAGE_WARNING, message);
}

void
clay_tool_set_cursor (ClayTool           *tool,
                      ClayDisplay        *display,
                      ClayCursorType      cursor,
                      ClayToolCursorType  tool_cursor,
                      ClayCursorModifier  modifier)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  clay_display_shell_set_cursor (clay_display_get_shell (display),
                                 cursor, tool_cursor, modifier);
}


/*  private functions  */

static void
clay_tool_options_notify (ClayToolOptions  *options,
                          const GParamSpec *pspec,
                          ClayTool         *tool)
{
  CLAY_TOOL_GET_CLASS (tool)->options_notify (tool, options, pspec);
}

static void
clay_tool_clear_status (ClayTool *tool)
{
  g_return_if_fail (CLAY_IS_TOOL (tool));

  while (tool->status_displays)
    clay_tool_pop_status (tool, tool->status_displays->data);
}

static void
clay_tool_release (ClayTool *tool)
{
  if (tool->last_pointer_state &&
      clay_tool_control_is_active (tool->control))
    {
      clay_tool_button_release (
        tool,
        &tool->last_pointer_coords,
        tool->last_pointer_time + g_get_monotonic_time () / 1000,
        tool->last_pointer_state,
        tool->display);
    }
}
