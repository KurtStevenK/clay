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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydisplay.h"

#include "clayaction.h"
#include "claycolordialog.h"
#include "claydialogfactory.h"
#include "clayfgbgeditor.h"
#include "clayhelp-ids.h"
#include "claysessioninfo.h"
#include "claytoolbox.h"
#include "claytoolbox-color-area.h"
#include "clayuimanager.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   color_area_foreground_changed (ClayContext          *context,
                                             GeglColor            *color,
                                             ClayColorDialog      *dialog);
static void   color_area_background_changed (ClayContext          *context,
                                             GeglColor            *color,
                                             ClayColorDialog      *dialog);

static void   color_area_dialog_update      (ClayColorDialog      *dialog,
                                             GeglColor            *color,
                                             ClayColorDialogState  state,
                                             ClayContext          *context);

static void   color_area_color_clicked      (ClayFgBgEditor       *editor,
                                             ClayActiveColor       active_color,
                                             ClayContext          *context);
static void   color_area_color_changed      (ClayContext          *context);
static void   color_area_tooltip            (ClayFgBgEditor       *editor,
                                             ClayFgBgTarget        target,
                                             GtkTooltip           *tooltip,
                                             ClayToolbox          *toolbox);


/*  local variables  */

static GtkWidget       *color_area          = NULL;
static GtkWidget       *color_dialog        = NULL;
static gboolean         color_dialog_active = FALSE;
static ClayActiveColor  edit_color          = CLAY_ACTIVE_COLOR_FOREGROUND;
static GeglColor       *revert_fg           = NULL;
static GeglColor       *revert_bg           = NULL;


/*  public functions  */

GtkWidget *
clay_toolbox_color_area_create (ClayToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  ClayContext *context;

  g_return_val_if_fail (CLAY_IS_TOOLBOX (toolbox), NULL);

  context = clay_toolbox_get_context (toolbox);

  color_area = clay_fg_bg_editor_new (context);
  gtk_widget_set_size_request (color_area, width, height);

  clay_help_set_help_data (color_area, NULL,
                           CLAY_HELP_TOOLBOX_COLOR_AREA);
  g_object_set (color_area, "has-tooltip", TRUE, NULL);

  g_signal_connect (color_area, "color-clicked",
                    G_CALLBACK (color_area_color_clicked),
                    context);
  g_signal_connect_swapped (color_area, "colors-swapped",
                            G_CALLBACK (color_area_color_changed),
                            context);
  g_signal_connect_swapped (color_area, "colors-default",
                            G_CALLBACK (color_area_color_changed),
                            context);
  g_signal_connect_swapped (color_area, "color-dropped",
                            G_CALLBACK (color_area_color_changed),
                            context);

  g_signal_connect (color_area, "tooltip",
                    G_CALLBACK (color_area_tooltip),
                    toolbox);

  return color_area;
}


/*  private functions  */

static void
color_area_foreground_changed (ClayContext     *context,
                               GeglColor       *color,
                               ClayColorDialog *dialog)
{
  if (edit_color == CLAY_ACTIVE_COLOR_FOREGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      clay_color_dialog_set_color (dialog, color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_background_changed (ClayContext     *context,
                               GeglColor       *color,
                               ClayColorDialog *dialog)
{
  if (edit_color == CLAY_ACTIVE_COLOR_BACKGROUND)
    {
      g_signal_handlers_block_by_func (dialog,
                                       color_area_dialog_update,
                                       context);

      clay_color_dialog_set_color (dialog, color);

      g_signal_handlers_unblock_by_func (dialog,
                                         color_area_dialog_update,
                                         context);
    }
}

static void
color_area_dialog_update (ClayColorDialog      *dialog,
                          GeglColor            *color,
                          ClayColorDialogState  state,
                          ClayContext          *context)
{
  switch (state)
    {
    case CLAY_COLOR_DIALOG_OK:
      gtk_widget_set_visible (color_dialog, FALSE);
      color_dialog_active = FALSE;
      /* Fallthrough */

    case CLAY_COLOR_DIALOG_UPDATE:
      if (edit_color == CLAY_ACTIVE_COLOR_FOREGROUND)
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_foreground_changed,
                                           dialog);

          clay_context_set_foreground (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_foreground_changed,
                                             dialog);
        }
      else
        {
          g_signal_handlers_block_by_func (context,
                                           color_area_background_changed,
                                           dialog);

          clay_context_set_background (context, color);

          g_signal_handlers_unblock_by_func (context,
                                             color_area_background_changed,
                                             dialog);
        }
      break;

    case CLAY_COLOR_DIALOG_CANCEL:
      gtk_widget_set_visible (color_dialog, FALSE);
      color_dialog_active = FALSE;
      clay_context_set_foreground (context, revert_fg);
      clay_context_set_background (context, revert_bg);
      break;
    }

  if (clay_context_get_display (context) && state != CLAY_COLOR_DIALOG_UPDATE)
    clay_display_grab_focus (clay_context_get_display (context));
}

static void
color_area_color_clicked (ClayFgBgEditor  *editor,
                          ClayActiveColor  active_color,
                          ClayContext     *context)
{
  GeglColor   *color;
  const gchar *title;

  if (! color_dialog_active)
    {
      g_clear_object (&revert_fg);
      revert_fg = gegl_color_duplicate (clay_context_get_foreground (context));

      g_clear_object (&revert_bg);
      revert_bg = gegl_color_duplicate (clay_context_get_background (context));
    }

  if (active_color == CLAY_ACTIVE_COLOR_FOREGROUND)
    {
      color = clay_context_get_foreground (context);
      title = _("Change Foreground Color");
    }
  else
    {
      color = clay_context_get_background (context);
      title = _("Change Background Color");
    }

  edit_color = active_color;

  if (! color_dialog)
    {
      color_dialog = clay_color_dialog_new (NULL, context, TRUE,
                                            NULL, NULL, NULL,
                                            GTK_WIDGET (editor),
                                            clay_dialog_factory_get_singleton (),
                                            "clay-toolbox-color-dialog",
                                            color, TRUE, FALSE);

      g_signal_connect_object (color_dialog, "update",
                               G_CALLBACK (color_area_dialog_update),
                               G_OBJECT (context), 0);

      g_signal_connect_object (context, "foreground-changed",
                               G_CALLBACK (color_area_foreground_changed),
                               G_OBJECT (color_dialog), 0);
      g_signal_connect_object (context, "background-changed",
                               G_CALLBACK (color_area_background_changed),
                               G_OBJECT (color_dialog), 0);
    }
  else if (! gtk_widget_get_visible (color_dialog))
    {
      clay_dialog_factory_position_dialog (clay_dialog_factory_get_singleton (),
                                           "clay-toolbox-color-dialog",
                                           color_dialog,
                                           clay_widget_get_monitor (GTK_WIDGET (editor)));
    }

  gtk_window_set_title (GTK_WINDOW (color_dialog), title);
  clay_color_dialog_set_color (CLAY_COLOR_DIALOG (color_dialog), color);

  gtk_window_present (GTK_WINDOW (color_dialog));
  color_dialog_active = TRUE;
}

static void
color_area_color_changed (ClayContext *context)
{
  if (clay_context_get_display (context))
    clay_display_grab_focus (clay_context_get_display (context));
}

static void
color_area_tooltip (ClayFgBgEditor *editor,
                    ClayFgBgTarget  target,
                    GtkTooltip     *tooltip,
                    ClayToolbox    *toolbox)
{
  ClayUIManager *manager = clay_dock_get_ui_manager (CLAY_DOCK (toolbox));
  ClayAction    *action  = NULL;
  const gchar   *text    = NULL;

  switch (target)
    {
    case CLAY_FG_BG_TARGET_FOREGROUND:
      text = _("The active foreground color.\n"
               "Click to open the color selection dialog.");
      break;

    case CLAY_FG_BG_TARGET_BACKGROUND:
      text = _("The active background color.\n"
               "Click to open the color selection dialog.");
      break;

    case CLAY_FG_BG_TARGET_SWAP:
      action = clay_ui_manager_find_action (manager, "context",
                                            "context-colors-swap");
      text = clay_action_get_tooltip (action);
      break;

    case CLAY_FG_BG_TARGET_DEFAULT:
      action = clay_ui_manager_find_action (manager, "context",
                                            "context-colors-default");
      text = clay_action_get_tooltip (action);
      break;

    default:
      break;
    }

  if (text)
    {
      gchar *markup = NULL;

      if (action)
        {
          gchar **accels = clay_action_get_display_accels (action);

          if (accels && accels[0])
            {
              gchar *escaped = g_markup_escape_text (text, -1);

              markup = g_strdup_printf ("%s  <b>%s</b>", escaped, accels[0]);
              g_free (escaped);
            }

          g_strfreev (accels);
        }

      if (markup)
        {
          gtk_tooltip_set_markup (tooltip, markup);
          g_free (markup);
        }
      else
        {
          gtk_tooltip_set_text (tooltip, text);
        }
    }
}
