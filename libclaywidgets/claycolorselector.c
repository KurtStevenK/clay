/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselector.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "claywidgetstypes.h"

#include "claycolorselector.h"
#include "clayicons.h"
#include "claywidgetsmarshal.h"


/**
 * SECTION: claycolorselector
 * @title: ClayColorSelector
 * @short_description: Pluggable Clay color selector modules.
 * @see_also: #GModule, #GTypeModule, #ClayModule
 *
 * Functions and definitions for creating pluggable Clay color
 * selector modules.
 **/


enum
{
  COLOR_CHANGED,
  CHANNEL_CHANGED,
  MODEL_VISIBLE_CHANGED,
  SIMULATION,
  LAST_SIGNAL
};


typedef struct _ClayColorSelectorPrivate
{
  gboolean                  toggles_visible;
  gboolean                  toggles_sensitive;
  gboolean                  show_alpha;
  gboolean                  model_visible[3];

  ClayColorSelectorChannel  channel;

  GeglColor                *color;

  gboolean                  simulation;
  ClayColorProfile         *simulation_profile;
  ClayColorRenderingIntent  simulation_intent;
  gboolean                  simulation_bpc;
} ClayColorSelectorPrivate;


static void   clay_color_selector_dispose                    (GObject           *object);

static void   clay_color_selector_emit_color_changed         (ClayColorSelector *selector);
static void   clay_color_selector_emit_channel_changed       (ClayColorSelector *selector);
static void   clay_color_selector_emit_model_visible_changed (ClayColorSelector *selector,
                                                              ClayColorSelectorModel model);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayColorSelector, clay_color_selector, GTK_TYPE_BOX)

#define parent_class clay_color_selector_parent_class

static guint selector_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_selector_class_init (ClayColorSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_color_selector_dispose;

  selector_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorSelectorClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  selector_signals[CHANNEL_CHANGED] =
    g_signal_new ("channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorSelectorClass, channel_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_COLOR_SELECTOR_CHANNEL);

  selector_signals[MODEL_VISIBLE_CHANGED] =
    g_signal_new ("model-visible-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorSelectorClass, model_visible_changed),
                  NULL, NULL,
                  _clay_widgets_marshal_VOID__ENUM_BOOLEAN,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_COLOR_SELECTOR_MODEL,
                  G_TYPE_BOOLEAN);

  selector_signals[SIMULATION] =
    g_signal_new ("simulation",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorSelectorClass, simulation),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  klass->name                  = "Unnamed";
  klass->help_id               = NULL;
  klass->icon_name             = CLAY_ICON_PALETTE;

  klass->set_toggles_visible   = NULL;
  klass->set_toggles_sensitive = NULL;
  klass->set_show_alpha        = NULL;
  klass->set_color             = NULL;
  klass->set_channel           = NULL;
  klass->set_model_visible     = NULL;
  klass->color_changed         = NULL;
  klass->channel_changed       = NULL;
  klass->model_visible_changed = NULL;
  klass->set_config            = NULL;
}

static void
clay_color_selector_init (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  priv = clay_color_selector_get_instance_private (selector);

  priv->toggles_visible   = TRUE;
  priv->toggles_sensitive = TRUE;
  priv->show_alpha        = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selector),
                                  GTK_ORIENTATION_VERTICAL);

  priv->channel = CLAY_COLOR_SELECTOR_RED;

  priv->color = gegl_color_new ("black");
  priv->model_visible[CLAY_COLOR_SELECTOR_MODEL_RGB] = TRUE;
  priv->model_visible[CLAY_COLOR_SELECTOR_MODEL_LCH] = TRUE;
  priv->model_visible[CLAY_COLOR_SELECTOR_MODEL_HSV] = FALSE;

  priv->simulation         = FALSE;
  priv->simulation_profile = NULL;
  priv->simulation_intent  = CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  priv->simulation_bpc     = FALSE;
}

static void
clay_color_selector_dispose (GObject *object)
{
  ClayColorSelector        *selector = CLAY_COLOR_SELECTOR (object);
  ClayColorSelectorPrivate *priv;

  priv = clay_color_selector_get_instance_private (selector);

  clay_color_selector_set_config (selector, NULL);
  g_clear_object (&priv->color);
  g_clear_object (&priv->simulation_profile);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * clay_color_selector_new: (skip) (attributes skip-reason=internal_only_since_9ff663cc)
 * @selector_type: The #GType of the selector to create.
 * @color:         The initial color to be edited.
 * @channel:       The selector's initial channel.
 *
 * Creates a new #ClayColorSelector widget of type @selector_type.
 *
 * Note that this is mostly internal API to be used by other widgets.
 *
 * Please use clay_color_selection_new() for the "Clay-typical" color
 * selection widget. Also see clay_color_button_new().
 *
 * Returns: the new #ClayColorSelector widget.
 **/
GtkWidget *
clay_color_selector_new (GType                     selector_type,
                         GeglColor                *color,
                         ClayColorSelectorChannel  channel)
{
  ClayColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, CLAY_TYPE_COLOR_SELECTOR), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  selector = g_object_new (selector_type, NULL);

  clay_color_selector_set_color (selector, color);
  clay_color_selector_set_channel (selector, channel);

  return GTK_WIDGET (selector);
}

/**
 * clay_color_selector_set_toggles_visible:
 * @selector: A #ClayColorSelector widget.
 * @visible:  The new @visible setting.
 *
 * Sets the @visible property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
clay_color_selector_set_toggles_visible (ClayColorSelector *selector,
                                         gboolean           visible)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  if (priv->toggles_visible != visible)
    {
      ClayColorSelectorClass *selector_class;

      priv->toggles_visible = visible ? TRUE : FALSE;

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_visible)
        selector_class->set_toggles_visible (selector, visible);
    }
}

/**
 * clay_color_selector_get_toggles_visible:
 * @selector: A #ClayColorSelector widget.
 *
 * Returns the @visible property of the @selector's toggles.
 *
 * Returns: %TRUE if the #ClayColorSelector's toggles are visible.
 *
 * Since: 2.10
 **/
gboolean
clay_color_selector_get_toggles_visible (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);

  return priv->toggles_visible;
}

/**
 * clay_color_selector_set_toggles_sensitive:
 * @selector:  A #ClayColorSelector widget.
 * @sensitive: The new @sensitive setting.
 *
 * Sets the @sensitive property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
clay_color_selector_set_toggles_sensitive (ClayColorSelector *selector,
                                           gboolean           sensitive)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  if (priv->toggles_sensitive != sensitive)
    {
      ClayColorSelectorClass *selector_class;

      priv->toggles_sensitive = sensitive ? TRUE : FALSE;

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_sensitive)
        selector_class->set_toggles_sensitive (selector, sensitive);
    }
}

/**
 * clay_color_selector_get_toggles_sensitive:
 * @selector: A #ClayColorSelector widget.
 *
 * Returns the @sensitive property of the @selector's toggles.
 *
 * Returns: %TRUE if the #ClayColorSelector's toggles are sensitive.
 *
 * Since: 2.10
 **/
gboolean
clay_color_selector_get_toggles_sensitive (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);

  return priv->toggles_sensitive;
}

/**
 * clay_color_selector_set_show_alpha:
 * @selector:   A #ClayColorSelector widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selector widget.
 **/
void
clay_color_selector_set_show_alpha (ClayColorSelector *selector,
                                    gboolean           show_alpha)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  if (show_alpha != priv->show_alpha)
    {
      ClayColorSelectorClass *selector_class;

      priv->show_alpha = show_alpha ? TRUE : FALSE;

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_show_alpha)
        selector_class->set_show_alpha (selector, show_alpha);
    }
}

/**
 * clay_color_selector_get_show_alpha:
 * @selector: A #ClayColorSelector widget.
 *
 * Returns the @selector's @show_alpha property.
 *
 * Returns: %TRUE if the #ClayColorSelector has alpha controls.
 *
 * Since: 2.10
 **/
gboolean
clay_color_selector_get_show_alpha (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);

  return priv->show_alpha;
}

/**
 * clay_color_selector_set_color:
 * @selector: A #ClayColorSelector widget.
 * @color:    The new color.
 *
 * Sets the color shown in the @selector widget.
 **/
void
clay_color_selector_set_color (ClayColorSelector *selector,
                               GeglColor         *color)
{
  ClayColorSelectorClass   *selector_class;
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (GEGL_IS_COLOR (color));

  priv = clay_color_selector_get_instance_private (selector);

  g_object_unref (priv->color);
  priv->color = gegl_color_duplicate (color);

  selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_color)
    selector_class->set_color (selector, priv->color);

  clay_color_selector_emit_color_changed (selector);
}

/**
 * clay_color_selector_get_color:
 * @selector: A #ClayColorSelector widget.
 *
 * Retrieves the color shown in the @selector widget.
 *
 * Returns: (transfer full): a copy of the selected color.
 * Since: 2.10
 **/
GeglColor *
clay_color_selector_get_color (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), NULL);

  priv = clay_color_selector_get_instance_private (selector);

  return gegl_color_duplicate (priv->color);
}

/**
 * clay_color_selector_set_channel:
 * @selector: A #ClayColorSelector widget.
 * @channel:  The new @channel setting.
 *
 * Sets the @channel property of the @selector widget.
 *
 * Changes between displayed channels if this @selector instance has
 * the ability to show different channels.
 * This will also update the color model if needed.
 **/
void
clay_color_selector_set_channel (ClayColorSelector        *selector,
                                 ClayColorSelectorChannel  channel)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  if (channel != priv->channel)
    {
      ClayColorSelectorClass *selector_class;
      ClayColorSelectorModel  model = -1;

      priv->channel = channel;

      switch (channel)
        {
        case CLAY_COLOR_SELECTOR_RED:
        case CLAY_COLOR_SELECTOR_GREEN:
        case CLAY_COLOR_SELECTOR_BLUE:
          model = CLAY_COLOR_SELECTOR_MODEL_RGB;
          break;

        case CLAY_COLOR_SELECTOR_HUE:
        case CLAY_COLOR_SELECTOR_SATURATION:
        case CLAY_COLOR_SELECTOR_VALUE:
          model = CLAY_COLOR_SELECTOR_MODEL_HSV;
          break;

        case CLAY_COLOR_SELECTOR_LCH_LIGHTNESS:
        case CLAY_COLOR_SELECTOR_LCH_CHROMA:
        case CLAY_COLOR_SELECTOR_LCH_HUE:
          model = CLAY_COLOR_SELECTOR_MODEL_LCH;
          break;

        case CLAY_COLOR_SELECTOR_ALPHA:
          /* Alpha channel does not change the color model. */
          break;

        default:
          /* Should not happen. */
          g_return_if_reached ();
          break;
        }

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_channel)
        selector_class->set_channel (selector, channel);

      clay_color_selector_emit_channel_changed (selector);

      if (model != -1)
        {
          /*  make visibility of LCH and HSV mutuallky exclusive  */
          if (model == CLAY_COLOR_SELECTOR_MODEL_HSV)
            {
              clay_color_selector_set_model_visible (selector,
                                                     CLAY_COLOR_SELECTOR_MODEL_LCH,
                                                     FALSE);
            }
          else if (model == CLAY_COLOR_SELECTOR_MODEL_LCH)
            {
              clay_color_selector_set_model_visible (selector,
                                                     CLAY_COLOR_SELECTOR_MODEL_HSV,
                                                     FALSE);
            }

          clay_color_selector_set_model_visible (selector, model, TRUE);
        }
    }
}

/**
 * clay_color_selector_get_channel:
 * @selector: A #ClayColorSelector widget.
 *
 * Returns the @selector's current channel.
 *
 * Returns: The #ClayColorSelectorChannel currently shown by the
 * @selector.
 *
 * Since: 2.10
 **/
ClayColorSelectorChannel
clay_color_selector_get_channel (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector),
                        CLAY_COLOR_SELECTOR_RED);

  priv = clay_color_selector_get_instance_private (selector);

  return priv->channel;
}

/**
 * clay_color_selector_set_model_visible:
 * @selector: A #ClayColorSelector widget.
 * @model:    The affected #ClayColorSelectorModel.
 * @visible:  The new visible setting.
 *
 * Sets the @model visible/invisible on the @selector widget.
 *
 * Toggles visibility of displayed models if this @selector instance
 * has the ability to show different color models.
 *
 * Since: 2.10
 **/
void
clay_color_selector_set_model_visible (ClayColorSelector      *selector,
                                       ClayColorSelectorModel  model,
                                       gboolean                visible)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  visible = visible ? TRUE : FALSE;

  if (visible != priv->model_visible[model])
    {
      ClayColorSelectorClass *selector_class;

      priv->model_visible[model] = visible;

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_model_visible)
        selector_class->set_model_visible (selector, model, visible);

      clay_color_selector_emit_model_visible_changed (selector, model);
    }
}

/**
 * clay_color_selector_get_model_visible:
 * @selector: A #ClayColorSelector widget.
 * @model:    The #ClayColorSelectorModel.
 *
 * Returns: whether @model is visible in @selector.
 *
 * Since: 2.10
 **/
gboolean
clay_color_selector_get_model_visible (ClayColorSelector      *selector,
                                       ClayColorSelectorModel  model)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);

  return priv->model_visible[model];
}

/**
 * clay_color_selector_set_config:
 * @selector: a #ClayColorSelector widget.
 * @config:   a #ClayColorConfig object.
 *
 * Sets the color management configuration to use with this color selector.
 *
 * Since: 2.4
 */
void
clay_color_selector_set_config (ClayColorSelector *selector,
                                ClayColorConfig   *config)
{
  ClayColorSelectorClass *selector_class;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (config == NULL || CLAY_IS_COLOR_CONFIG (config));

  selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_config)
    selector_class->set_config (selector, config);
}

/**
 * clay_color_selector_set_format
 * @selector: a #ClayColorSelector widget.
 * @format:   a Babl format, with space.
 *
 * Sets the babl format representing the color model and the space this
 * @selector is supposed to display values for. Depending on the type of color
 * selector, it may trigger various UX changes, or none at all.
 *
 * Since: 3.0
 */
void
clay_color_selector_set_format (ClayColorSelector *selector,
                                const Babl        *format)
{
  ClayColorSelectorClass *selector_class;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_format)
    selector_class->set_format (selector, format);
}

/**
 * clay_color_selector_set_simulation
 * @selector: a #ClayColorSelector widget.
 * @profile:  a #ClayColorProfile object.
 * @intent:   a #ClayColorRenderingIntent enum.
 * @bpc:      a gboolean.
 *
 * Sets the simulation options to use with this color selector.
 *
 * Since: 3.0
 */
void
clay_color_selector_set_simulation (ClayColorSelector        *selector,
                                    ClayColorProfile         *profile,
                                    ClayColorRenderingIntent  intent,
                                    gboolean                  bpc)
{
  ClayColorSelectorClass   *selector_class;
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (profile == NULL || CLAY_IS_COLOR_PROFILE (profile));

  selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);
  priv           = clay_color_selector_get_instance_private (selector);

  if ((profile && ! priv->simulation_profile)                                        ||
      (! profile && priv->simulation_profile)                                        ||
      (profile && ! clay_color_profile_is_equal (profile, priv->simulation_profile)) ||
       intent != priv->simulation_intent                                             ||
       bpc    != priv->simulation_bpc)
    {
      g_set_object (&priv->simulation_profile, profile);
      priv->simulation_intent = intent;
      priv->simulation_bpc    = bpc;

      if (selector_class->set_simulation)
        selector_class->set_simulation (selector, profile, intent, bpc);
    }
}

gboolean
clay_color_selector_get_simulation (ClayColorSelector         *selector,
                                    ClayColorProfile         **profile,
                                    ClayColorRenderingIntent *intent,
                                    gboolean                 *bpc)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);

  if (profile)
    *profile = priv->simulation_profile;
  if (intent)
    *intent = priv->simulation_intent;
  if (bpc)
    *bpc = priv->simulation_bpc;

  return priv->simulation;
}

gboolean
clay_color_selector_enable_simulation (ClayColorSelector *selector,
                                       gboolean           enabled)
{
  ClayColorSelectorPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_SELECTOR (selector), FALSE);

  priv = clay_color_selector_get_instance_private (selector);
  if (priv->simulation != enabled)
    {
      if (! enabled || priv->simulation_profile)
        {
          priv->simulation = enabled;
          g_signal_emit (selector, selector_signals[SIMULATION], 0, enabled);
        }
    }

  return priv->simulation;
}


/* Private functions. */

/**
 * clay_color_selector_emit_color_changed:
 * @selector: A #ClayColorSelector widget.
 *
 * Emits the "color-changed" signal.
 */
static void
clay_color_selector_emit_color_changed (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[COLOR_CHANGED], 0, priv->color);
}

/**
 * clay_color_selector_emit_channel_changed:
 * @selector: A #ClayColorSelector widget.
 *
 * Emits the "channel-changed" signal.
 */
static void
clay_color_selector_emit_channel_changed (ClayColorSelector *selector)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[CHANNEL_CHANGED], 0,
                 priv->channel);
}

/**
 * clay_color_selector_emit_model_visible_changed:
 * @selector: A #ClayColorSelector widget.
 * @model:    The #ClayColorSelectorModel where visibility changed.
 *
 * Emits the "model-visible-changed" signal.
 *
 * Since: 2.10
 */
static void
clay_color_selector_emit_model_visible_changed (ClayColorSelector      *selector,
                                                ClayColorSelectorModel  model)
{
  ClayColorSelectorPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_SELECTOR (selector));

  priv = clay_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[MODEL_VISIBLE_CHANGED], 0,
                 model, priv->model_visible[model]);
}
