/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenushell.c
 * Copyright (C) 2023 Jehan
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

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "claymenu.h"
#include "claymenumodel.h"
#include "claymenushell.h"
#include "clayuimanager.h"


enum
{
  MODEL_DELETED,
  LAST_SIGNAL
};

#define GET_PRIVATE(obj) (clay_menu_shell_get_private ((ClayMenuShell *) (obj)))

typedef struct _ClayMenuShellPrivate ClayMenuShellPrivate;

struct _ClayMenuShellPrivate
{
  ClayUIManager  *manager;
  ClayMenuModel  *model;
  gchar          *path_prefix;

  GRegex         *path_regex;
};


static ClayMenuShellPrivate *
                clay_menu_shell_get_private             (ClayMenuShell        *menu_shell);
static void     clay_menu_shell_private_finalize        (ClayMenuShellPrivate *priv);

static void     clay_menu_shell_model_changed           (GMenuModel           *self,
                                                         gint                  position,
                                                         gint                  removed,
                                                         gint                  added,
                                                         ClayMenuShell        *shell);

static void     clay_menu_shell_append_model_drop_top   (ClayMenuShell        *shell,
                                                         ClayMenuModel        *model);


G_DEFINE_INTERFACE (ClayMenuShell, clay_menu_shell, GTK_TYPE_CONTAINER)

static guint signals[LAST_SIGNAL];

static void
clay_menu_shell_default_init (ClayMenuShellInterface *iface)
{
  signals[MODEL_DELETED] =
    g_signal_new ("model-deleted",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayMenuShellInterface, model_deleted),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("manager",
                                                            NULL, NULL,
                                                            CLAY_TYPE_UI_MANAGER,
                                                            CLAY_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("model",
                                                            NULL, NULL,
                                                            CLAY_TYPE_MENU_MODEL,
                                                            CLAY_PARAM_READWRITE |
                                                            G_PARAM_READWRITE));
}


/* Public functions. */

void
clay_menu_shell_fill (ClayMenuShell *shell,
                      ClayMenuModel *model,
                      gboolean       drop_top_submenu)
{
  g_return_if_fail (GTK_IS_CONTAINER (shell));

  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  if (drop_top_submenu)
    clay_menu_shell_append_model_drop_top (shell, model);
  else
    clay_menu_shell_append (shell, model);
}


/* Protected functions. */

void
clay_menu_shell_append (ClayMenuShell *shell,
                        ClayMenuModel *model)
{
  ClayMenuShellPrivate *priv = GET_PRIVATE (shell);

  g_free (priv->path_prefix);
  priv->path_prefix = g_strdup (clay_menu_model_get_path (CLAY_MENU_MODEL (model)));

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            G_CALLBACK (clay_menu_shell_model_changed),
                                            shell);
      g_signal_emit (shell, signals[MODEL_DELETED], 0);
    }

  if (priv->model != model)
    {
      g_clear_object (&priv->model);
      priv->model = g_object_ref (model);
    }

  if (model)
    {
      CLAY_MENU_SHELL_GET_INTERFACE (shell)->append (shell, model);

      g_signal_connect_object (priv->model, "items-changed",
                               G_CALLBACK (clay_menu_shell_model_changed),
                               shell, 0);
    }
}

void
clay_menu_shell_init (ClayMenuShell *shell)
{
  ClayMenuShellPrivate *priv;

  g_return_if_fail (CLAY_IS_MENU_SHELL (shell));

  priv = GET_PRIVATE (shell);

  priv->path_prefix   = NULL;
  priv->model         = NULL;
  priv->path_regex    = g_regex_new ("/+", 0, 0, NULL);
}

/**
 * clay_menu_shell_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #ClayMenuShell. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
clay_menu_shell_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, CLAY_MENU_SHELL_PROP_MANAGER, "manager");
  g_object_class_override_property (klass, CLAY_MENU_SHELL_PROP_MODEL,   "model");
}

void
clay_menu_shell_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayMenuShellPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case CLAY_MENU_SHELL_PROP_MANAGER:
      g_value_set_object (value, priv->manager);
      break;
    case CLAY_MENU_SHELL_PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
clay_menu_shell_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayMenuShellPrivate *priv  = GET_PRIVATE (object);
  ClayMenuShell        *shell = CLAY_MENU_SHELL (object);


  switch (property_id)
    {
    case CLAY_MENU_SHELL_PROP_MANAGER:
      g_set_weak_pointer (&priv->manager, g_value_get_object (value));
      break;
    case CLAY_MENU_SHELL_PROP_MODEL:
      clay_menu_shell_append (shell, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

ClayUIManager *
clay_menu_shell_get_manager (ClayMenuShell *shell)
{
  return GET_PRIVATE (shell)->manager;
}


/*  Private functions  */

static ClayMenuShellPrivate *
clay_menu_shell_get_private (ClayMenuShell *menu_shell)
{
  ClayMenuShellPrivate *priv;
  static GQuark         private_key = 0;

  g_return_val_if_fail (CLAY_IS_MENU_SHELL (menu_shell), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("clay-menu_shell-priv");

  priv = g_object_get_qdata ((GObject *) menu_shell, private_key);

  if (! priv)
    {
      priv = g_slice_new0 (ClayMenuShellPrivate);

      g_object_set_qdata_full ((GObject *) menu_shell, private_key, priv,
                               (GDestroyNotify) clay_menu_shell_private_finalize);
    }

  return priv;
}

static void
clay_menu_shell_private_finalize (ClayMenuShellPrivate *priv)
{
  g_clear_weak_pointer (&priv->manager);
  g_free (priv->path_prefix);
  g_clear_pointer (&priv->path_regex, g_regex_unref);
  g_clear_object (&priv->model);

  g_slice_free (ClayMenuShellPrivate, priv);
}

static void
clay_menu_shell_model_changed (GMenuModel    *model,
                               gint           position,
                               gint           removed,
                               gint           added,
                               ClayMenuShell *shell)
{
  ClayMenuShellPrivate *priv = GET_PRIVATE (shell);

  /* Lazy approach to handle model changes. A better implementation would only
   * remove or delete the changed items using:
   * CLAY_MENU_SHELL_GET_INTERFACE (shell)->add_ui()
   * CLAY_MENU_SHELL_GET_INTERFACE (shell)->remove_ui
   * TODO
   */
  gtk_container_foreach (GTK_CONTAINER (shell),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  CLAY_MENU_SHELL_GET_INTERFACE (shell)->append (shell, priv->model);
}

static void
clay_menu_shell_append_model_drop_top (ClayMenuShell *shell,
                                       ClayMenuModel *model)
{
  ClayMenuShellPrivate *priv    = GET_PRIVATE (shell);
  GMenuModel           *submenu = NULL;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  if (g_menu_model_get_n_items (G_MENU_MODEL (model)) == 1)
    {
      gchar *label = NULL;

      submenu = g_menu_model_get_item_link (G_MENU_MODEL (model), 0, G_MENU_LINK_SUBMENU);

      if (submenu)
        {
          gchar *path_prefix;

          g_menu_model_get_item_attribute (submenu, 0, G_MENU_ATTRIBUTE_LABEL, "s", &label);

          path_prefix = g_strdup_printf ("%s/%s", priv->path_prefix, label);
          g_free (priv->path_prefix);
          priv->path_prefix = path_prefix;
        }

      g_free (label);
    }

  clay_menu_shell_append (shell, submenu != NULL ? CLAY_MENU_MODEL (submenu) : CLAY_MENU_MODEL (model));

  g_clear_object (&submenu);
}
