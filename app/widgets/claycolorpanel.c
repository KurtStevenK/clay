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

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "clayaction.h"
#include "clayactiongroup.h"
#include "clayactionimpl.h"
#include "claycolordialog.h"
#include "claycolorpanel.h"


#define RGBA_EPSILON 1e-6

enum
{
  RESPONSE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void       clay_color_panel_dispose         (GObject            *object);

static gboolean   clay_color_panel_button_press    (GtkWidget          *widget,
                                                    GdkEventButton     *bevent);

static void       clay_color_panel_clicked         (GtkButton          *button);

static void       clay_color_panel_color_changed   (ClayColorButton    *button);
static GType      clay_color_panel_get_action_type (ClayColorButton    *button);

static void       clay_color_panel_dialog_update   (ClayColorDialog    *dialog,
                                                    GeglColor          *color,
                                                    ClayColorDialogState state,
                                                    ClayColorPanel     *panel);


G_DEFINE_TYPE (ClayColorPanel, clay_color_panel, CLAY_TYPE_COLOR_BUTTON)

#define parent_class clay_color_panel_parent_class

static guint color_panel_signals[LAST_SIGNAL] = { 0, };


static void
clay_color_panel_class_init (ClayColorPanelClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GtkButtonClass       *button_class       = GTK_BUTTON_CLASS (klass);
  ClayColorButtonClass *color_button_class = CLAY_COLOR_BUTTON_CLASS (klass);

  object_class->dispose               = clay_color_panel_dispose;

  widget_class->button_press_event    = clay_color_panel_button_press;

  button_class->clicked               = clay_color_panel_clicked;

  color_button_class->color_changed   = clay_color_panel_color_changed;
  color_button_class->get_action_type = clay_color_panel_get_action_type;

  color_panel_signals[RESPONSE] =
    g_signal_new ("response",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayColorPanelClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_COLOR_DIALOG_STATE);
}

static void
clay_color_panel_init (ClayColorPanel *panel)
{
  panel->context            = NULL;
  panel->color_dialog       = NULL;
  panel->user_context_aware = TRUE;
}

static void
clay_color_panel_dispose (GObject *object)
{
  ClayColorPanel *panel = CLAY_COLOR_PANEL (object);

  g_clear_pointer (&panel->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
clay_color_panel_button_press (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      ClayColorButton    *color_button;
      ClayColorPanel     *color_panel;
      GSimpleActionGroup *group;
      ClayAction         *action;
      GeglColor          *color;

      color_button = CLAY_COLOR_BUTTON (widget);
      color_panel  = CLAY_COLOR_PANEL (widget);

      group = clay_color_button_get_action_group (color_button);

      action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-foreground"));
      clay_action_set_visible (action, color_panel->context != NULL);

      action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-background"));
      clay_action_set_visible (action, color_panel->context != NULL);

      if (color_panel->context)
        {
          action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-foreground"));
          color = clay_context_get_foreground (color_panel->context);
          g_object_set (action, "color", color, NULL);

          action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-background"));
          color = clay_context_get_background (color_panel->context);
          g_object_set (action, "color", color, NULL);
        }

      action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-black"));
      color = gegl_color_new ("black");
      g_object_set (action, "color", color, NULL);
      g_object_unref (color);

      action = CLAY_ACTION (g_action_map_lookup_action (G_ACTION_MAP (group), "use-white"));
      color = gegl_color_new ("white");
      g_object_set (action, "color", color, NULL);
      g_object_unref (color);
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return FALSE;
}

static void
clay_color_panel_clicked (GtkButton *button)
{
  ClayColorPanel *panel = CLAY_COLOR_PANEL (button);
  GeglColor      *color;

  color = clay_color_button_get_color (CLAY_COLOR_BUTTON (button));

  if (! panel->color_dialog)
    {
      ClayColorButton *color_button = CLAY_COLOR_BUTTON (button);

      panel->color_dialog =
        clay_color_dialog_new (NULL, panel->context,
                               panel->user_context_aware,
                               clay_color_button_get_title (color_button),
                               NULL, NULL,
                               GTK_WIDGET (button),
                               NULL, NULL, color,
                               clay_color_button_get_update (color_button),
                               clay_color_button_has_alpha (color_button));

      g_signal_connect (panel->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &panel->color_dialog);

      g_signal_connect (panel->color_dialog, "update",
                        G_CALLBACK (clay_color_panel_dialog_update),
                        panel);
    }
  else
    {
      clay_color_dialog_set_color (CLAY_COLOR_DIALOG (panel->color_dialog), color);
    }

  gtk_window_present (GTK_WINDOW (panel->color_dialog));

  g_object_unref (color);
}

static GType
clay_color_panel_get_action_type (ClayColorButton *button)
{
  return CLAY_TYPE_ACTION_IMPL;
}


/*  public functions  */

GtkWidget *
clay_color_panel_new (const gchar       *title,
                      GeglColor         *color,
                      ClayColorAreaType  type,
                      gint               width,
                      gint               height)
{
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (color != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  return g_object_new (CLAY_TYPE_COLOR_PANEL,
                       "title",       title,
                       "type",        type,
                       "color",       color,
                       "area-width",  width,
                       "area-height", height,
                       NULL);
}

static void
clay_color_panel_color_changed (ClayColorButton *button)
{
  ClayColorPanel *panel = CLAY_COLOR_PANEL (button);

  if (panel->color_dialog)
    {
      GeglColor *color;
      GeglColor *dialog_color;

      color = clay_color_button_get_color (CLAY_COLOR_BUTTON (button));

      dialog_color = clay_color_dialog_get_color (CLAY_COLOR_DIALOG (panel->color_dialog));

      if (! clay_color_is_perceptually_identical (color, dialog_color))
        clay_color_dialog_set_color (CLAY_COLOR_DIALOG (panel->color_dialog), color);

      g_object_unref (color);
      g_object_unref (dialog_color);
    }
}

void
clay_color_panel_set_context (ClayColorPanel *panel,
                              ClayContext    *context)
{
  g_return_if_fail (CLAY_IS_COLOR_PANEL (panel));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  panel->context = context;

  if (context)
    clay_color_button_set_color_config (CLAY_COLOR_BUTTON (panel),
                                        context->clay->config->color_management);
}

void
clay_color_panel_set_user_context_aware (ClayColorPanel *panel,
                                         gboolean        user_context_aware)
{
  g_return_if_fail (CLAY_IS_COLOR_PANEL (panel));

  panel->user_context_aware = user_context_aware;
}

void
clay_color_panel_dialog_response (ClayColorPanel       *panel,
                                  ClayColorDialogState  state)
{
  g_return_if_fail (CLAY_IS_COLOR_PANEL (panel));
  g_return_if_fail (state == CLAY_COLOR_DIALOG_OK ||
                    state == CLAY_COLOR_DIALOG_CANCEL);

  if (panel->color_dialog && gtk_widget_get_visible (panel->color_dialog))
    clay_color_panel_dialog_update (NULL, NULL, state, panel);
}


/*  private functions  */

static void
clay_color_panel_dialog_update (ClayColorDialog      *dialog,
                                GeglColor            *color,
                                ClayColorDialogState  state,
                                ClayColorPanel       *panel)
{
  switch (state)
    {
    case CLAY_COLOR_DIALOG_UPDATE:
      if (clay_color_button_get_update (CLAY_COLOR_BUTTON (panel)) && color)
        clay_color_button_set_color (CLAY_COLOR_BUTTON (panel), color);
      break;

    case CLAY_COLOR_DIALOG_OK:
    case CLAY_COLOR_DIALOG_CANCEL:
      /* ClayColorDialog returns the appropriate color (new one or
       * original one if process cancelled.
       */
      if (color)
        clay_color_button_set_color (CLAY_COLOR_BUTTON (panel), color);
      gtk_widget_set_visible (panel->color_dialog, FALSE);

      g_signal_emit (panel, color_panel_signals[RESPONSE], 0,
                     state);
      break;
    }
}
