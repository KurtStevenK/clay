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

#include <stdlib.h>
#include <string.h> /* memcmp */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claydata.h"
#include "claypadactions.h"
#include "clayparamspecs.h"

#include "clay-intl.h"


typedef struct _ClayPadAction ClayPadAction;

struct _ClayPadAction
{
  ClayPadActionType  type;
  gint               number;
  gint               mode;
  gchar             *action_name;
};

struct _ClayPadActionsPrivate
{
  GArray *actions;
};


/*  local function prototypes  */

static void     clay_pad_actions_config_iface_init (ClayConfigInterface *iface);

static void     clay_pad_actions_finalize          (GObject             *object);

static gboolean clay_pad_actions_serialize         (ClayConfig          *config,
                                                    ClayConfigWriter    *writer,
                                                    gpointer             data);
static gboolean clay_pad_actions_deserialize       (ClayConfig          *config,
                                                    GScanner            *scanner,
                                                    gint                 nest_level,
                                                    gpointer             data);

static gboolean clay_pad_actions_equal             (ClayConfig          *a,
                                                    ClayConfig          *b);
static void     clay_pad_actions_reset             (ClayConfig          *config);
static gboolean clay_pad_actions_config_copy       (ClayConfig          *src,
                                                    ClayConfig          *dest,
                                                    GParamFlags          flags);


G_DEFINE_TYPE_WITH_CODE (ClayPadActions, clay_pad_actions, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayPadActions)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_pad_actions_config_iface_init))

#define parent_class clay_pad_actions_parent_class


static void
clay_pad_actions_class_init (ClayPadActionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_pad_actions_finalize;
}

static void
clay_pad_actions_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_pad_actions_serialize;
  iface->deserialize = clay_pad_actions_deserialize;
  iface->equal       = clay_pad_actions_equal;
  iface->reset       = clay_pad_actions_reset;
  iface->copy        = clay_pad_actions_config_copy;
}

static void
clear_action (gpointer user_data)
{
  ClayPadAction *pad_action = user_data;

  g_free (pad_action->action_name);
}

static void
clay_pad_actions_init (ClayPadActions *pad_actions)
{
  pad_actions->priv = clay_pad_actions_get_instance_private (pad_actions);

  pad_actions->priv->actions =
    g_array_new (FALSE, FALSE, sizeof (ClayPadAction));
  g_array_set_clear_func (pad_actions->priv->actions, clear_action);
}

static void
clay_pad_actions_finalize (GObject *object)
{
  ClayPadActions *pad_actions = CLAY_PAD_ACTIONS (object);

  g_clear_pointer (&pad_actions->priv->actions, g_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
clay_pad_actions_serialize (ClayConfig       *config,
                            ClayConfigWriter *writer,
                            gpointer          data)
{
  ClayPadActions *pad_actions = CLAY_PAD_ACTIONS (config);
  guint           i;

  for (i = 0; i < pad_actions->priv->actions->len; i++)
    {
      ClayPadAction *pad_action;
      const gchar   *name        = NULL;

      pad_action = &g_array_index (pad_actions->priv->actions,
                                   ClayPadAction, i);

      switch (pad_action->type)
        {
        case CLAY_PAD_ACTION_BUTTON:
          name = "button";
          break;
        case CLAY_PAD_ACTION_RING:
          name = "ring";
          break;
        case CLAY_PAD_ACTION_STRIP:
          name = "strip";
          break;
        }

      clay_config_writer_open (writer, name);
      clay_config_writer_printf (writer, "%d", pad_action->number);
      clay_config_writer_printf (writer, "%d", pad_action->mode);
      clay_config_writer_string (writer, pad_action->action_name);
      clay_config_writer_close (writer);
    }

  return TRUE;
}

static gboolean
clay_pad_actions_deserialize (ClayConfig *config,
                              GScanner   *scanner,
                              gint        nest_level,
                              gpointer    data)
{
  ClayPadActions *pad_actions = CLAY_PAD_ACTIONS (config);
  GTokenType      token;
  guint           scope_id;
  guint           old_scope_id;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id, "button",
                              GINT_TO_POINTER (CLAY_PAD_ACTION_BUTTON));
  g_scanner_scope_add_symbol (scanner, scope_id, "ring",
                              GINT_TO_POINTER (CLAY_PAD_ACTION_RING));
  g_scanner_scope_add_symbol (scanner, scope_id, "strip",
                              GINT_TO_POINTER (CLAY_PAD_ACTION_STRIP));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      ClayPadActionType  type;
      gint               number;
      gint               mode;
      gchar             *action_name = NULL;

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GUINT_TO_POINTER (CLAY_PAD_ACTION_BUTTON) ||
              scanner->value.v_symbol == GUINT_TO_POINTER (CLAY_PAD_ACTION_RING) ||
              scanner->value.v_symbol == GUINT_TO_POINTER (CLAY_PAD_ACTION_STRIP))
            type = GPOINTER_TO_UINT (scanner->value.v_symbol);
          else
            goto error;

          if (! clay_scanner_parse_int (scanner, &number))
            goto error;
          if (! clay_scanner_parse_int (scanner, &mode))
            goto error;
          if (! clay_scanner_parse_string (scanner, &action_name))
            goto error;

          clay_pad_actions_set_action (pad_actions, type, number, mode,
                                       action_name);
          g_clear_pointer (&action_name, g_free);
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

 error:

  g_scanner_scope_remove_symbol (scanner, scope_id, "button");
  g_scanner_scope_remove_symbol (scanner, scope_id, "ring");
  g_scanner_scope_remove_symbol (scanner, scope_id, "strip");

  g_scanner_set_scope (scanner, old_scope_id);

  return G_TOKEN_LEFT_PAREN;
}

static gboolean
clay_pad_actions_equal (ClayConfig *a,
                        ClayConfig *b)
{
  ClayPadActions *a_pad_actions = CLAY_PAD_ACTIONS (a);
  ClayPadActions *b_pad_actions = CLAY_PAD_ACTIONS (b);

  if (a_pad_actions->priv->actions->len != b_pad_actions->priv->actions->len ||
      memcmp (a_pad_actions->priv->actions->data,
              b_pad_actions->priv->actions->data,
              sizeof (ClayPadAction) * a_pad_actions->priv->actions->len))
    {
      return FALSE;
    }

  return TRUE;
}

static void
clay_pad_actions_reset (ClayConfig *config)
{
  ClayPadActions *pad_actions = CLAY_PAD_ACTIONS (config);

  g_array_set_size (pad_actions->priv->actions, 0);
}

static gboolean
clay_pad_actions_config_copy (ClayConfig  *src,
                              ClayConfig  *dest,
                              GParamFlags  flags)
{
  clay_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);
  clay_data_dirty (CLAY_DATA (dest));

  return TRUE;
}


static gint
clay_pad_actions_find_action (GArray            *array,
                              ClayPadActionType  type,
                              guint              number,
                              guint              mode)
{
  ClayPadAction *pad_action;
  guint          i;

  for (i = 0; i < array->len; i++)
    {
      pad_action = &g_array_index (array, ClayPadAction, i);
      if (pad_action->type == type && pad_action->number == number &&
          pad_action->mode == mode)
        return i;
    }

  return -1;
}


/*  public functions  */

ClayPadActions *
clay_pad_actions_new (void)
{
  return g_object_new (CLAY_TYPE_PAD_ACTIONS, NULL);
}

void
clay_pad_actions_foreach (ClayPadActions       *pad_actions,
                          ClayPadActionForeach  func,
                          gpointer              data)
{
  ClayPadActionsPrivate *private;
  ClayPadAction         *pad_action;
  guint                  i;

  private = pad_actions->priv;

  for (i = 0; i < private->actions->len; i++)
    {
      pad_action = &g_array_index (private->actions, ClayPadAction, i);

      func (pad_actions, pad_action->type, pad_action->number, pad_action->mode,
            pad_action->action_name, data);
    }
}

gint
clay_pad_actions_set_action (ClayPadActions    *pad_actions,
                             ClayPadActionType  type,
                             guint              number,
                             guint              mode,
                             const gchar       *action_name)
{
  ClayPadActionsPrivate *private;
  ClayPadAction          new_action = { 0, };
  guint                  i;

  g_return_val_if_fail (CLAY_IS_PAD_ACTIONS (pad_actions), -1);
  g_return_val_if_fail (type <= CLAY_PAD_ACTION_STRIP, -1);
  g_return_val_if_fail (action_name, -1);
  g_return_val_if_fail (*action_name, -1);

  new_action.type = type;
  new_action.number = number;
  new_action.mode = mode;
  new_action.action_name = g_strdup (action_name);

  private = pad_actions->priv;

  /* Find insertion point */
  for (i = 0; i < private->actions->len; i++)
    {
      ClayPadAction *pad_action;

      pad_action = &g_array_index (private->actions,
                                   ClayPadAction, i);
      /* If the number/mode is already there, this is an existing item */
      if (pad_action->type == type && pad_action->number == number &&
          pad_action->mode == mode)
        {
          g_array_remove_index (private->actions, i);
          break;
        }

      if (pad_action->type > type ||
          (pad_action->type == type && pad_action->mode > mode) ||
          (pad_action->type == type && pad_action->mode == mode &&
           pad_action->number > number))
        break;
    }

  if (i == pad_actions->priv->actions->len)
    g_array_append_val (private->actions, new_action);
  else
    g_array_insert_val (private->actions, i, new_action);

  return i;
}

void
clay_pad_actions_clear_action (ClayPadActions    *pad_actions,
                               ClayPadActionType  type,
                               guint              number,
                               guint              mode)
{
  ClayPadActionsPrivate *private;
  gint                   pos;

  g_return_if_fail (CLAY_IS_PAD_ACTIONS (pad_actions));
  g_return_if_fail (type <= CLAY_PAD_ACTION_STRIP);

  private = pad_actions->priv;
  pos = clay_pad_actions_find_action (private->actions, type,
                                      number, mode);
  if (pos >= 0)
    g_array_remove_index (private->actions, pos);
}

gint
clay_pad_actions_get_n_actions (ClayPadActions *pad_actions)
{
  ClayPadActionsPrivate *private;

  g_return_val_if_fail (CLAY_IS_PAD_ACTIONS (pad_actions), -1);

  private = pad_actions->priv;

  return private->actions->len;
}

const gchar *
clay_pad_actions_get_action (ClayPadActions    *pad_actions,
                             ClayPadActionType  type,
                             guint              number,
                             guint              mode)
{
  ClayPadActionsPrivate *private;
  ClayPadAction         *pad_action;
  gint                   pos;

  g_return_val_if_fail (CLAY_IS_PAD_ACTIONS (pad_actions), NULL);
  g_return_val_if_fail (type <= CLAY_PAD_ACTION_STRIP, NULL);

  private = pad_actions->priv;

  pos = clay_pad_actions_find_action (private->actions, type,
                                      number, mode);
  if (pos >= 0)
    {
      pad_action = &g_array_index (private->actions,
                                   ClayPadAction, pos);
      return pad_action->action_name;
    }
  else
    {
      return NULL;
    }
}
