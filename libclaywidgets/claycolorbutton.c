/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorbutton.c
 * Copyright (C) 1999-2001 Sven Neumann
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "claywidgetstypes.h"

#include "claycolorarea.h"
#include "claycolorbutton.h"
#include "claycolornotebook.h"
#include "claycolorselection.h"
#include "claydialog.h"
#include "clayhelpui.h"
#include "clayicons.h"
#include "claywidgets-private.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claycolorbutton
 * @title: ClayColorButton
 * @short_description: Widget for selecting a color from a simple button.
 * @see_also: #libclaycolor-claycolorspace
 *
 * This widget provides a simple button with a preview showing the
 * color.
 *
 * On click a color selection dialog is opened. Additionally the
 * button supports Drag and Drop and has a right-click menu that
 * allows one to choose the color from the current FG or BG color. If
 * the user changes the color, the "color-changed" signal is emitted.
 **/


#define COLOR_BUTTON_KEY "clay-color-button"
#define RESPONSE_RESET   1

#define TODOUBLE(i) (i / 65535.0)
#define TOUINT16(d) ((guint16) (d * 65535 + 0.5))


#define CLAY_COLOR_BUTTON_COLOR_FG     "use-foreground"
#define CLAY_COLOR_BUTTON_COLOR_BG     "use-background"
#define CLAY_COLOR_BUTTON_COLOR_BLACK  "use-black"
#define CLAY_COLOR_BUTTON_COLOR_WHITE  "use-white"

#define CLAY_COLOR_BUTTON_GROUP_PREFIX "clay-color-button"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_COLOR,
  PROP_TYPE,
  PROP_UPDATE,
  PROP_AREA_WIDTH,
  PROP_AREA_HEIGHT,
  PROP_COLOR_CONFIG
};


typedef struct _ClayColorButtonPrivate
{
  gchar              *title;
  gboolean            continuous_update;

  GtkWidget          *color_area;
  GtkWidget          *dialog;
  GtkWidget          *selection;

  GSimpleActionGroup *group;
  GMenu              *menu;

  ClayColorConfig    *config;
} ClayColorButtonPrivate;

#define GET_PRIVATE(obj) ((ClayColorButtonPrivate *) clay_color_button_get_instance_private ((ClayColorButton *) (obj)))


static void     clay_color_button_constructed         (GObject         *object);
static void     clay_color_button_finalize            (GObject         *object);
static void     clay_color_button_dispose             (GObject         *object);
static void     clay_color_button_get_property        (GObject         *object,
                                                       guint            property_id,
                                                       GValue          *value,
                                                       GParamSpec      *pspec);
static void     clay_color_button_set_property        (GObject         *object,
                                                       guint            property_id,
                                                       const GValue    *value,
                                                       GParamSpec      *pspec);

static gboolean clay_color_button_button_press        (GtkWidget       *widget,
                                                       GdkEventButton  *bevent);
static void     clay_color_button_state_flags_changed (GtkWidget       *widget,
                                                       GtkStateFlags    prev_state);
static void     clay_color_button_clicked             (GtkButton       *button);
static GType    clay_color_button_get_action_type     (ClayColorButton *button);

static void     clay_color_button_dialog_response     (GtkWidget       *dialog,
                                                       gint             response_id,
                                                       ClayColorButton *button);
static void     clay_color_button_use_color           (GAction         *action,
                                                       GVariant        *parameter,
                                                       ClayColorButton *button);
static void     clay_color_button_area_changed        (GtkWidget       *color_area,
                                                       ClayColorButton *button);
static void     clay_color_button_selection_changed   (GtkWidget       *selection,
                                                       ClayColorButton *button);
static void     clay_color_button_help_func           (const gchar     *help_id,
                                                       gpointer         help_data);


typedef void (* ClayColorButtonActionCallback)        (GAction         *action,
                                                       GVariant        *parameter,
                                                       ClayColorButton *button);
typedef struct _ClayColorButtonActionEntry
{
  const gchar                   *name;
  ClayColorButtonActionCallback  callback;
} ClayColorButtonActionEntry;


static const ClayColorButtonActionEntry actions[] =
{
  {
    CLAY_COLOR_BUTTON_COLOR_FG,
    clay_color_button_use_color
  },
  {
    CLAY_COLOR_BUTTON_COLOR_BG,
    clay_color_button_use_color
  },
  {
    CLAY_COLOR_BUTTON_COLOR_BLACK,
    clay_color_button_use_color
  },
  {
    CLAY_COLOR_BUTTON_COLOR_WHITE,
    clay_color_button_use_color
  }
};


G_DEFINE_TYPE_WITH_CODE (ClayColorButton, clay_color_button, CLAY_TYPE_BUTTON,
                         G_ADD_PRIVATE (ClayColorButton))

#define parent_class clay_color_button_parent_class

static guint clay_color_button_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_button_class_init (ClayColorButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  clay_color_button_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorButtonClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed         = clay_color_button_constructed;
  object_class->finalize            = clay_color_button_finalize;
  object_class->dispose             = clay_color_button_dispose;
  object_class->get_property        = clay_color_button_get_property;
  object_class->set_property        = clay_color_button_set_property;

  widget_class->button_press_event  = clay_color_button_button_press;
  widget_class->state_flags_changed = clay_color_button_state_flags_changed;

  button_class->clicked             = clay_color_button_clicked;

  klass->color_changed              = NULL;
  klass->get_action_type            = clay_color_button_get_action_type;

  /**
   * ClayColorButton:title:
   *
   * The title to be used for the color selection dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the color selection dialog",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:color:
   *
   * The color displayed in the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_COLOR,
                                   clay_param_spec_color_from_string ("color",
                                                                      "Color",
                                                                      "The color displayed in the button's color area",
                                                                      TRUE, "black",
                                                                      CLAY_PARAM_READWRITE |
                                                                      G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:type:
   *
   * The type of the button's color area.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type",
                                                      "Type",
                                                      "The type of the button's color area",
                                                      CLAY_TYPE_COLOR_AREA_TYPE,
                                                      CLAY_COLOR_AREA_FLAT,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:continuous-update:
   *
   * The update policy of the color button.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_UPDATE,
                                   g_param_spec_boolean ("continuous-update",
                                                         "Contiguous Update",
                                                         "The update policy of the color button",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:area-width:
   *
   * The minimum width of the button's #ClayColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_WIDTH,
                                   g_param_spec_int ("area-width",
                                                     "Area Width",
                                                     "The minimum width of the button's ClayColorArea",
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:area-height:
   *
   * The minimum height of the button's #ClayColorArea.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class, PROP_AREA_HEIGHT,
                                   g_param_spec_int ("area-height",
                                                     "Area Height",
                                                     "The minimum height of the button's ClayColorArea",
                                                     1, G_MAXINT, 16,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
  /**
   * ClayColorButton:color-config:
   *
   * The #ClayColorConfig object used for the button's #ClayColorArea
   * and #ClayColorSelection.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        "Color Config",
                                                        "The color config object used",
                                                        CLAY_TYPE_COLOR_CONFIG,
                                                        G_PARAM_READWRITE));
}

static void
clay_color_button_init (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (button);

  priv->color_area = g_object_new (CLAY_TYPE_COLOR_AREA,
                                   "drag-mask", GDK_BUTTON1_MASK,
                                   NULL);

  g_signal_connect (priv->color_area, "color-changed",
                    G_CALLBACK (clay_color_button_area_changed),
                    button);

  gtk_container_add (GTK_CONTAINER (button), priv->color_area);
  gtk_widget_set_visible (priv->color_area, TRUE);
}

static void
clay_color_button_constructed (GObject *object)
{
  ClayColorButton        *button = CLAY_COLOR_BUTTON (object);
  ClayColorButtonClass   *klass  = CLAY_COLOR_BUTTON_GET_CLASS (object);
  ClayColorButtonPrivate *priv   = GET_PRIVATE (object);
  GMenu                  *section;
  gint                    i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->group = g_simple_action_group_new ();

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      GAction *action;

      action = g_object_new (klass->get_action_type (button),
                             "name", actions[i].name,
                             NULL);

      if (actions[i].callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (actions[i].callback),
                          button);

      g_action_map_add_action (G_ACTION_MAP (priv->group), action);

      g_object_unref (action);
    }

  gtk_widget_insert_action_group (GTK_WIDGET (button),
                                  CLAY_COLOR_BUTTON_GROUP_PREFIX,
                                  G_ACTION_GROUP (priv->group));

  /* right-click opens a popup */
  priv->menu = g_menu_new ();

  section = g_menu_new ();
  g_menu_append (section, _("_Foreground Color"), CLAY_COLOR_BUTTON_GROUP_PREFIX "." CLAY_COLOR_BUTTON_COLOR_FG);
  g_menu_append (section, _("_Background Color"), CLAY_COLOR_BUTTON_GROUP_PREFIX "." CLAY_COLOR_BUTTON_COLOR_BG);
  g_menu_append_section (priv->menu, NULL, G_MENU_MODEL (section));
  g_clear_object (&section);

  section = g_menu_new ();
  g_menu_append (section, _("Blac_k"), CLAY_COLOR_BUTTON_GROUP_PREFIX "." CLAY_COLOR_BUTTON_COLOR_BLACK);
  g_menu_append (section, _("_White"), CLAY_COLOR_BUTTON_GROUP_PREFIX "." CLAY_COLOR_BUTTON_COLOR_WHITE);
  g_menu_append_section (priv->menu, NULL, G_MENU_MODEL (section));
  g_clear_object (&section);
}

static void
clay_color_button_finalize (GObject *object)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->title, g_free);
  g_clear_object (&priv->menu);
  g_clear_object (&priv->group);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_button_dispose (GObject *object)
{
  ClayColorButton        *button = CLAY_COLOR_BUTTON (object);
  ClayColorButtonPrivate *priv   = GET_PRIVATE (button);

  g_clear_pointer (&priv->dialog, gtk_widget_destroy);
  priv->selection = NULL;

  g_clear_pointer (&priv->color_area, gtk_widget_destroy);

  clay_color_button_set_color_config (button, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_color_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_COLOR:
      g_object_get_property (G_OBJECT (priv->color_area), "color", value);
      break;

    case PROP_TYPE:
      g_object_get_property (G_OBJECT (priv->color_area), "type", value);
      break;

    case PROP_UPDATE:
      g_value_set_boolean (value, priv->continuous_update);
      break;

    case PROP_COLOR_CONFIG:
      g_value_set_object (value, priv->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayColorButton        *button = CLAY_COLOR_BUTTON (object);
  ClayColorButtonPrivate *priv   = GET_PRIVATE (object);
  gint                    other;

  switch (property_id)
    {
    case PROP_TITLE:
      clay_color_button_set_title (button, g_value_get_string (value));
      break;

    case PROP_COLOR:
      g_object_set_property (G_OBJECT (priv->color_area), "color", value);
      break;

    case PROP_TYPE:
      g_object_set_property (G_OBJECT (priv->color_area), "type", value);
      break;

    case PROP_UPDATE:
      clay_color_button_set_update (button, g_value_get_boolean (value));
      break;

    case PROP_AREA_WIDTH:
      gtk_widget_get_size_request (priv->color_area, NULL, &other);
      gtk_widget_set_size_request (priv->color_area,
                                   g_value_get_int (value), other);
      break;

    case PROP_AREA_HEIGHT:
      gtk_widget_get_size_request (priv->color_area, &other, NULL);
      gtk_widget_set_size_request (priv->color_area,
                                   other, g_value_get_int (value));
      break;

    case PROP_COLOR_CONFIG:
      clay_color_button_set_color_config (button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_color_button_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      GtkWidget *menu;

      menu = gtk_menu_new_from_model (G_MENU_MODEL (priv->menu));

      /*gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));*/
      gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);
      gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) bevent);
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);
}

static void
clay_color_button_state_flags_changed (GtkWidget     *widget,
                                       GtkStateFlags  previous_state)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (widget);

  if (! gtk_widget_is_sensitive (widget) && priv->dialog)
    gtk_widget_set_visible (priv->dialog, FALSE);

  if (GTK_WIDGET_CLASS (parent_class)->state_flags_changed)
    GTK_WIDGET_CLASS (parent_class)->state_flags_changed (widget,
                                                          previous_state);
}

static void
clay_color_button_clicked (GtkButton *button)
{
  ClayColorButton        *color_button = CLAY_COLOR_BUTTON (button);
  ClayColorButtonPrivate *priv         = GET_PRIVATE (button);
  GeglColor              *color;

  if (! priv->dialog)
    {
      GtkWidget *dialog;

      dialog = priv->dialog =
        clay_dialog_new (priv->title, "clay-color-button",
                         gtk_widget_get_toplevel (GTK_WIDGET (button)), 0,
                         clay_color_button_help_func, NULL,

                         _("_Reset"),  RESPONSE_RESET,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

      g_object_set_data (G_OBJECT (dialog), COLOR_BUTTON_KEY, button);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               RESPONSE_RESET,
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (clay_color_button_dialog_response),
                        color_button);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &priv->dialog);

      priv->selection = clay_color_selection_new ();
      gtk_container_set_border_width (GTK_CONTAINER (priv->selection), 6);
      clay_color_selection_set_show_alpha (CLAY_COLOR_SELECTION (priv->selection),
                                           clay_color_button_has_alpha (color_button));
      clay_color_selection_set_config (CLAY_COLOR_SELECTION (priv->selection),
                                       priv->config);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          priv->selection, TRUE, TRUE, 0);
      gtk_widget_set_visible (priv->selection, TRUE);

      g_signal_connect (priv->selection, "color-changed",
                        G_CALLBACK (clay_color_button_selection_changed),
                        button);
    }

  color = clay_color_button_get_color (color_button);

  g_signal_handlers_block_by_func (priv->selection,
                                   clay_color_button_selection_changed,
                                   button);

  clay_color_selection_set_color (CLAY_COLOR_SELECTION (priv->selection), color);
  clay_color_selection_set_old_color (CLAY_COLOR_SELECTION (priv->selection), color);

  g_signal_handlers_unblock_by_func (priv->selection,
                                     clay_color_button_selection_changed,
                                     button);

  gtk_window_present (GTK_WINDOW (priv->dialog));

  g_object_unref (color);
}

static GType
clay_color_button_get_action_type (ClayColorButton *button)
{
  return G_TYPE_SIMPLE_ACTION;
}


/*  public functions  */

/**
 * clay_color_button_new:
 * @title:  String that will be used as title for the color_selector.
 * @width:  Width of the colorpreview in pixels.
 * @height: Height of the colorpreview in pixels.
 * @color:  A [class@Gegl.Color].
 * @type:   The type of transparency to be displayed.
 *
 * Creates a new #ClayColorButton widget.
 *
 * This returns a button with a preview showing the color.
 * When the button is clicked a GtkColorSelectionDialog is opened.
 * If the user changes the color the new color is written into the
 * array that was used to pass the initial color and the "color-changed"
 * signal is emitted.
 *
 * Returns: Pointer to the new #ClayColorButton widget.
 **/
GtkWidget *
clay_color_button_new (const gchar       *title,
                       gint               width,
                       gint               height,
                       GeglColor         *color,
                       ClayColorAreaType  type)
{
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  return g_object_new (CLAY_TYPE_COLOR_BUTTON,
                       "title",       title,
                       "type",        type,
                       "color",       color,
                       "area-width",  width,
                       "area-height", height,
                       NULL);
}

/**
 * clay_color_button_set_title:
 * @button: a #ClayColorButton.
 * @title:  the new title.
 *
 * Sets the @button dialog's title.
 *
 * Since: 2.10
 **/
void
clay_color_button_set_title (ClayColorButton *button,
                             const gchar     *title)
{
  ClayColorButtonPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_BUTTON (button));
  g_return_if_fail (title != NULL);

  priv = GET_PRIVATE (button);

  g_free (priv->title);
  priv->title = g_strdup (title);

  if (priv->dialog)
    gtk_window_set_title (GTK_WINDOW (priv->dialog), title);

  g_object_notify (G_OBJECT (button), "title");
}

/**
 * clay_color_button_get_title:
 * @button: a #ClayColorButton.
 *
 * Returns: The @button dialog's title.
 *
 * Since: 2.10
 **/
const gchar *
clay_color_button_get_title (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->title;
}

/**
 * clay_color_button_set_color:
 * @button: Pointer to a #ClayColorButton.
 * @color:  A new [class@Gegl.Color].
 *
 * Sets the @button to the given @color.
 **/
void
clay_color_button_set_color (ClayColorButton *button,
                             GeglColor       *color)
{
  ClayColorButtonPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_BUTTON (button));
  g_return_if_fail (GEGL_IS_COLOR (color));

  priv = GET_PRIVATE (button);

  clay_color_area_set_color (CLAY_COLOR_AREA (priv->color_area), color);

  g_object_notify (G_OBJECT (button), "color");
}

/**
 * clay_color_button_get_color:
 * @button: Pointer to a #ClayColorButton.
 *
 * Retrieves the currently set color from the @button.
 *
 * Returns: (transfer full): a copy of @button's [class@Gegl.Color].
 **/
GeglColor *
clay_color_button_get_color (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return clay_color_area_get_color (CLAY_COLOR_AREA (priv->color_area));
}

/**
 * clay_color_button_has_alpha:
 * @button: Pointer to a #ClayColorButton.
 *
 * Checks whether the @buttons shows transparency information.
 *
 * Returns: %TRUE if the @button shows transparency information, %FALSE
 * otherwise.
 **/
gboolean
clay_color_button_has_alpha (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return clay_color_area_has_alpha (CLAY_COLOR_AREA (priv->color_area));
}

/**
 * clay_color_button_set_type:
 * @button: Pointer to a #ClayColorButton.
 * @type: the new #ClayColorAreaType
 *
 * Sets the @button to the given @type. See also clay_color_area_set_type().
 **/
void
clay_color_button_set_type (ClayColorButton   *button,
                            ClayColorAreaType  type)
{
  ClayColorButtonPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  clay_color_area_set_type (CLAY_COLOR_AREA (priv->color_area), type);

  g_object_notify (G_OBJECT (button), "type");
}

/**
 * clay_color_button_get_update:
 * @button: A #ClayColorButton widget.
 *
 * Returns the color button's @continuous_update property.
 *
 * Returns: the @continuous_update property.
 **/
gboolean
clay_color_button_get_update (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_BUTTON (button), FALSE);

  priv = GET_PRIVATE (button);

  return priv->continuous_update;
}

/**
 * clay_color_button_set_update:
 * @button:     A #ClayColorButton widget.
 * @continuous: The new setting of the @continuous_update property.
 *
 * When set to %TRUE, the @button will emit the "color-changed"
 * continuously while the color is changed in the color selection
 * dialog.
 **/
void
clay_color_button_set_update (ClayColorButton *button,
                              gboolean         continuous)
{
  ClayColorButtonPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_BUTTON (button));

  priv = GET_PRIVATE (button);

  if (continuous != priv->continuous_update)
    {
      priv->continuous_update = continuous ? TRUE : FALSE;

      if (priv->selection)
        {
          GeglColor *color;

          if (priv->continuous_update)
            color = clay_color_selection_get_color (CLAY_COLOR_SELECTION (priv->selection));
          else
            color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (priv->selection));

          clay_color_button_set_color (button, color);
          g_object_unref (color);
        }

      g_object_notify (G_OBJECT (button), "continuous-update");
    }
}

/**
 * clay_color_button_set_color_config:
 * @button: a #ClayColorButton widget.
 * @config: a #ClayColorConfig object.
 *
 * Sets the color management configuration to use with this color button's
 * #ClayColorArea.
 *
 * Since: 2.10
 */
void
clay_color_button_set_color_config (ClayColorButton *button,
                                    ClayColorConfig *config)
{
  ClayColorButtonPrivate *priv;

  g_return_if_fail (CLAY_IS_COLOR_BUTTON (button));
  g_return_if_fail (config == NULL || CLAY_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (button);

  if (g_set_object (&priv->config, config))
    {
      if (priv->color_area)
        clay_color_area_set_color_config (CLAY_COLOR_AREA (priv->color_area),
                                          priv->config);

      if (priv->selection)
        clay_color_selection_set_config (CLAY_COLOR_SELECTION (priv->selection),
                                         priv->config);
    }
}

/**
 * clay_color_button_get_action_group:
 * @button: a #ClayColorButton.
 *
 * Returns: (transfer none): The @button's #GSimpleActionGroup.
 *
 * Since: 3.0
 **/
GSimpleActionGroup *
clay_color_button_get_action_group (ClayColorButton *button)
{
  ClayColorButtonPrivate *priv;

  g_return_val_if_fail (CLAY_IS_COLOR_BUTTON (button), NULL);

  priv = GET_PRIVATE (button);

  return priv->group;
}


/*  private functions  */

static void
clay_color_button_dialog_response (GtkWidget       *dialog,
                                   gint             response_id,
                                   ClayColorButton *button)
{
  ClayColorButtonPrivate *priv  = GET_PRIVATE (button);
  GeglColor              *color = NULL;

  switch (response_id)
    {
    case RESPONSE_RESET:
      clay_color_selection_reset (CLAY_COLOR_SELECTION (priv->selection));
      break;

    case GTK_RESPONSE_OK:
      if (! priv->continuous_update)
        {
          color = clay_color_selection_get_color (CLAY_COLOR_SELECTION (priv->selection));
          clay_color_button_set_color (button, color);
        }

      gtk_widget_set_visible (dialog, FALSE);
      break;

    default:
      if (priv->continuous_update)
        {
          color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (priv->selection)),
          clay_color_button_set_color (button, color);
        }

      gtk_widget_set_visible (dialog, FALSE);
      break;
    }

  g_clear_object (&color);
}

static void
clay_color_button_use_color (GAction         *action,
                             GVariant        *parameter,
                             ClayColorButton *button)
{
  const gchar *name;
  GeglColor   *color;

  name = g_action_get_name (action);

  if (! strcmp (name, CLAY_COLOR_BUTTON_COLOR_FG))
    {
      if (_clay_get_foreground_func)
        color = _clay_get_foreground_func ();
      else
        color = gegl_color_new ("black");
    }
  else if (! strcmp (name, CLAY_COLOR_BUTTON_COLOR_BG))
    {
      if (_clay_get_background_func)
        color = _clay_get_background_func ();
      else
        color = gegl_color_new ("white");
    }
  else if (! strcmp (name, CLAY_COLOR_BUTTON_COLOR_BLACK))
    {
      color = gegl_color_new ("black");
    }
  else if (! strcmp (name, CLAY_COLOR_BUTTON_COLOR_WHITE))
    {
      color = gegl_color_new ("white");
    }
  else
    {
      color = clay_color_button_get_color (button);
    }

  clay_color_button_set_color (button, color);

  g_clear_object (&color);
}

static void
clay_color_button_area_changed (GtkWidget       *color_area,
                                ClayColorButton *button)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->selection)
    {
      GeglColor *color;

      color = clay_color_button_get_color (button);

      g_signal_handlers_block_by_func (priv->selection,
                                       clay_color_button_selection_changed,
                                       button);

      clay_color_selection_set_color (CLAY_COLOR_SELECTION (priv->selection), color);

      g_signal_handlers_unblock_by_func (priv->selection,
                                         clay_color_button_selection_changed,
                                         button);

      g_object_unref (color);
    }

  g_signal_emit (button, clay_color_button_signals[COLOR_CHANGED], 0);
}

static void
clay_color_button_selection_changed (GtkWidget       *selection,
                                     ClayColorButton *button)
{
  ClayColorButtonPrivate *priv = GET_PRIVATE (button);

  if (priv->continuous_update)
    {
      GeglColor *color;

      color = clay_color_selection_get_color (CLAY_COLOR_SELECTION (selection));

      g_signal_handlers_block_by_func (priv->color_area,
                                       clay_color_button_area_changed,
                                       button);

      clay_color_area_set_color (CLAY_COLOR_AREA (priv->color_area), color);

      g_signal_handlers_unblock_by_func (priv->color_area,
                                         clay_color_button_area_changed,
                                         button);

      g_signal_emit (button, clay_color_button_signals[COLOR_CHANGED], 0);

      g_object_unref (color);
    }
}

static void
clay_color_button_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  ClayColorButton        *button;
  ClayColorButtonPrivate *priv;
  ClayColorSelection     *selection;
  ClayColorNotebook      *notebook;
  ClayColorSelector      *current;

  button = g_object_get_data (G_OBJECT (help_data), COLOR_BUTTON_KEY);
  priv   = GET_PRIVATE (button);

  selection = CLAY_COLOR_SELECTION (priv->selection);
  notebook = CLAY_COLOR_NOTEBOOK (clay_color_selection_get_notebook (selection));

  current = clay_color_notebook_get_current_selector (notebook);

  help_id = CLAY_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  clay_standard_help_func (help_id, NULL);
}
