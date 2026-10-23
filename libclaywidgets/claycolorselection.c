/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselection.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "claycolorarea.h"
#include "claycolornotebook.h"
#include "claycolorscales.h"
#include "claycolorselect.h"
#include "claycolorselection.h"
#include "clayhelpui.h"
#include "clayicons.h"
#include "claywidgets.h"
#include "claywidgets-private.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claycolorselection
 * @title: ClayColorSelection
 * @short_description: Widget for doing a color selection.
 *
 * Widget for doing a color selection.
 **/


#define COLOR_AREA_SIZE  20


typedef enum
{
  UPDATE_NOTEBOOK  = 1 << 0,
  UPDATE_SCALES    = 1 << 1,
  UPDATE_ENTRY     = 1 << 2,
  UPDATE_COLOR     = 1 << 3
} UpdateType;

#define UPDATE_ALL (UPDATE_NOTEBOOK | \
                    UPDATE_SCALES   | \
                    UPDATE_ENTRY    | \
                    UPDATE_COLOR)

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONFIG
};


struct _ClayColorSelection
{
  GtkBox                    parent_instance;

  gboolean                  show_alpha;

  GeglColor                *color;
  ClayColorSelectorChannel  channel;

  GtkWidget                *left_vbox;
  GtkWidget                *right_vbox;

  GtkWidget                *notebook;
  GtkWidget                *scales;

  GtkWidget                *new_color;
  GtkWidget                *old_color;
};


static void   clay_color_selection_finalize          (GObject            *object);
static void   clay_color_selection_set_property      (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);

static void   clay_color_selection_switch_page       (GtkWidget          *widget,
                                                      gpointer            page,
                                                      guint               page_num,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_notebook_changed  (ClayColorSelector  *selector,
                                                      GeglColor          *color,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_scales_changed    (ClayColorSelector  *selector,
                                                      GeglColor          *color,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_color_picked      (GtkWidget          *widget,
                                                      const GeglColor    *rgb,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_entry_changed     (ClayColorHexEntry  *entry,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_channel_changed   (ClayColorSelector  *selector,
                                                      ClayColorSelectorChannel channel,
                                                      ClayColorSelection *selection);
static void   clay_color_selection_new_color_changed (GtkWidget          *widget,
                                                      ClayColorSelection *selection);

static void   clay_color_selection_update            (ClayColorSelection *selection,
                                                      UpdateType          update);


G_DEFINE_TYPE (ClayColorSelection, clay_color_selection, GTK_TYPE_BOX)

#define parent_class clay_color_selection_parent_class

static guint selection_signals[LAST_SIGNAL] = { 0, };


static void
clay_color_selection_class_init (ClayColorSelectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_color_selection_finalize;
  object_class->set_property = clay_color_selection_set_property;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The color config used by this color selection",
                                                        CLAY_TYPE_COLOR_CONFIG,
                                                        G_PARAM_WRITABLE));

  selection_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ClayColorSelection");
}

static void
clay_color_selection_init (ClayColorSelection *selection)
{
  GtkWidget    *main_hbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *frame;
  GtkWidget    *label;
  GtkWidget    *entry;
  GtkWidget    *button;
  GtkSizeGroup *new_group;
  GtkSizeGroup *old_group;

  selection->show_alpha = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selection),
                                  GTK_ORIENTATION_VERTICAL);

  selection->color   = gegl_color_new ("black");
  selection->channel = CLAY_COLOR_SELECTOR_HUE;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection), main_hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_hbox, TRUE);

  /*  The left vbox with the notebook  */
  selection->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), selection->left_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (selection->left_vbox, TRUE);

  if (_clay_ensure_modules_func)
    {
      g_type_class_ref (CLAY_TYPE_COLOR_SELECT);
      _clay_ensure_modules_func ();
    }

  selection->notebook = clay_color_selector_new (CLAY_TYPE_COLOR_NOTEBOOK, selection->color, selection->channel);

  if (_clay_ensure_modules_func)
    g_type_class_unref (g_type_class_peek (CLAY_TYPE_COLOR_SELECT));

  clay_color_selector_set_toggles_visible
    (CLAY_COLOR_SELECTOR (selection->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (selection->left_vbox), selection->notebook,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (selection->notebook, TRUE);

  g_signal_connect (selection->notebook, "color-changed",
                    G_CALLBACK (clay_color_selection_notebook_changed),
                    selection);
  g_signal_connect (clay_color_notebook_get_notebook (CLAY_COLOR_NOTEBOOK (selection->notebook)),
                    "switch-page",
                    G_CALLBACK (clay_color_selection_switch_page),
                    selection);

  /*  The hbox for the color_areas  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (selection->left_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The labels  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("Current:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_set_visible (label, TRUE);

  new_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (new_group, label);
  g_object_unref (new_group);

  label = gtk_label_new (_("Old:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_set_visible (label, TRUE);

  old_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (old_group, label);
  g_object_unref (old_group);

  /*  The color areas  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  selection->new_color = clay_color_area_new (selection->color,
                                              selection->show_alpha ?
                                              CLAY_COLOR_AREA_SMALL_CHECKS :
                                              CLAY_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_size_group_add_widget (new_group, selection->new_color);
  gtk_box_pack_start (GTK_BOX (vbox), selection->new_color, FALSE, FALSE, 0);
  gtk_widget_set_visible (selection->new_color, TRUE);

  g_signal_connect (selection->new_color, "color-changed",
                    G_CALLBACK (clay_color_selection_new_color_changed),
                    selection);

  selection->old_color = clay_color_area_new (selection->color,
                                              selection->show_alpha ?
                                              CLAY_COLOR_AREA_SMALL_CHECKS :
                                              CLAY_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (selection->old_color);
  gtk_size_group_add_widget (old_group, selection->old_color);
  gtk_box_pack_start (GTK_BOX (vbox), selection->old_color, FALSE, FALSE, 0);
  gtk_widget_set_visible (selection->old_color, TRUE);

  /*  The right vbox with color scales  */
  selection->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), selection->right_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (selection->right_vbox, TRUE);

  selection->scales = clay_color_selector_new (CLAY_TYPE_COLOR_SCALES, selection->color, selection->channel);

  clay_color_selector_set_toggles_visible
    (CLAY_COLOR_SELECTOR (selection->scales), TRUE);
  clay_color_selector_set_show_alpha (CLAY_COLOR_SELECTOR (selection->scales),
                                      selection->show_alpha);
  gtk_box_pack_start (GTK_BOX (selection->right_vbox), selection->scales,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (selection->scales, TRUE);

  g_signal_connect (selection->scales, "channel-changed",
                    G_CALLBACK (clay_color_selection_channel_changed),
                    selection);
  g_signal_connect (selection->scales, "color-changed",
                    G_CALLBACK (clay_color_selection_scales_changed),
                    selection);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection->right_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The color picker  */
  button = clay_pick_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (clay_color_selection_color_picked),
                    selection);

  /* The hex triplet entry */
  entry = clay_color_hex_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (entry, TRUE);

  label = gtk_label_new_with_mnemonic (_("HTML _notation:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  g_object_set_data (G_OBJECT (selection), "color-hex-entry", entry);

  g_signal_connect (entry, "color-changed",
                    G_CALLBACK (clay_color_selection_entry_changed),
                    selection);
}

static void
clay_color_selection_finalize (GObject *object)
{
  ClayColorSelection *selection = CLAY_COLOR_SELECTION (object);

  g_object_unref (selection->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_selection_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayColorSelection *selection = CLAY_COLOR_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      clay_color_selection_set_config (selection, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * clay_color_selection_new:
 *
 * Creates a new #ClayColorSelection widget.
 *
 * Returns: The new #ClayColorSelection widget.
 **/
GtkWidget *
clay_color_selection_new (void)
{
  return g_object_new (CLAY_TYPE_COLOR_SELECTION, NULL);
}

/**
 * clay_color_selection_set_show_alpha:
 * @selection:  A #ClayColorSelection widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selection widget.
 **/
void
clay_color_selection_set_show_alpha (ClayColorSelection *selection,
                                     gboolean            show_alpha)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));

  if (show_alpha != selection->show_alpha)
    {
      selection->show_alpha = show_alpha ? TRUE : FALSE;

      clay_color_selector_set_show_alpha
        (CLAY_COLOR_SELECTOR (selection->notebook), selection->show_alpha);
      clay_color_selector_set_show_alpha
        (CLAY_COLOR_SELECTOR (selection->scales), selection->show_alpha);

      clay_color_area_set_type (CLAY_COLOR_AREA (selection->new_color),
                                selection->show_alpha ?
                                CLAY_COLOR_AREA_SMALL_CHECKS :
                                CLAY_COLOR_AREA_FLAT);
      clay_color_area_set_type (CLAY_COLOR_AREA (selection->old_color),
                                selection->show_alpha ?
                                CLAY_COLOR_AREA_SMALL_CHECKS :
                                CLAY_COLOR_AREA_FLAT);
    }
}

/**
 * clay_color_selection_get_show_alpha:
 * @selection: A #ClayColorSelection widget.
 *
 * Returns the @selection's @show_alpha property.
 *
 * Returns: %TRUE if the #ClayColorSelection has alpha controls.
 **/
gboolean
clay_color_selection_get_show_alpha (ClayColorSelection *selection)
{
  g_return_val_if_fail (CLAY_IS_COLOR_SELECTION (selection), FALSE);

  return selection->show_alpha;
}

/**
 * clay_color_selection_set_color:
 * @selection: A #ClayColorSelection widget.
 * @color:     The @color to set as current color.
 *
 * Sets the #ClayColorSelection's current color to the new @color.
 **/
void
clay_color_selection_set_color (ClayColorSelection *selection,
                                GeglColor          *color)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));
  g_return_if_fail (GEGL_IS_COLOR (color));

  g_object_unref (selection->color);
  selection->color = gegl_color_duplicate (color);

  clay_color_selection_update (selection, UPDATE_ALL);
  clay_color_selection_color_changed (selection);
}

/**
 * clay_color_selection_get_color:
 * @selection: A #ClayColorSelection widget.
 *
 * This function returns the #ClayColorSelection's current color.
 *
 * Returns: (transfer full): the currently selected color.
 **/
GeglColor *
clay_color_selection_get_color (ClayColorSelection *selection)
{
  g_return_val_if_fail (CLAY_IS_COLOR_SELECTION (selection), NULL);

  return gegl_color_duplicate (selection->color);
}

/**
 * clay_color_selection_set_old_color:
 * @selection: A #ClayColorSelection widget.
 * @color:     The @color to set as old color.
 *
 * Sets the #ClayColorSelection's old color.
 **/
void
clay_color_selection_set_old_color (ClayColorSelection *selection,
                                    GeglColor          *color)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));
  g_return_if_fail (GEGL_IS_COLOR (color));

  clay_color_area_set_color (CLAY_COLOR_AREA (selection->old_color), color);
}

/**
 * clay_color_selection_get_old_color:
 * @selection: A #ClayColorSelection widget.
 *
 * Returns: (transfer full): the old color.
 **/
GeglColor *
clay_color_selection_get_old_color (ClayColorSelection *selection)
{
  g_return_val_if_fail (CLAY_IS_COLOR_SELECTION (selection), NULL);

  return clay_color_area_get_color (CLAY_COLOR_AREA (selection->old_color));
}

/**
 * clay_color_selection_reset:
 * @selection: A #ClayColorSelection widget.
 *
 * Sets the #ClayColorSelection's current color to its old color.
 **/
void
clay_color_selection_reset (ClayColorSelection *selection)
{
  GeglColor *color;

  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));

  color = clay_color_area_get_color (CLAY_COLOR_AREA (selection->old_color));
  clay_color_selection_set_color (selection, color);

  g_object_unref (color);
}

/**
 * clay_color_selection_color_changed:
 * @selection: A #ClayColorSelection widget.
 *
 * Emits the "color-changed" signal.
 **/
void
clay_color_selection_color_changed (ClayColorSelection *selection)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * clay_color_selection_set_format:
 * @selection: A #ClayColorSelection widget.
 * @format:    A Babl format, with space.
 *
 * Updates all selectors with the current format.
 *
 * Since: 3.0
 */
void
clay_color_selection_set_format (ClayColorSelection *selection,
                                 const Babl         *format)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));

  clay_color_notebook_set_format (CLAY_COLOR_NOTEBOOK (selection->notebook),
                                  format);
  clay_color_selector_set_format (CLAY_COLOR_SELECTOR (selection->scales),
                                  format);

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * clay_color_selection_set_simulation:
 * @selection: A #ClayColorSelection widget.
 * @profile:   A #ClayColorProfile object.
 * @intent:    A #ClayColorRenderingIntent enum.
 * @bpc:       A gboolean.
 *
 * Sets the simulation options to use with this color selection.
 *
 * Since: 3.0
 */
void
clay_color_selection_set_simulation (ClayColorSelection *selection,
                                     ClayColorProfile   *profile,
                                     ClayColorRenderingIntent intent,
                                     gboolean            bpc)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));

  clay_color_notebook_set_simulation (CLAY_COLOR_NOTEBOOK (selection->notebook),
                                      profile,
                                      intent,
                                      bpc);

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * clay_color_selection_set_config:
 * @selection: A #ClayColorSelection widget.
 * @config:    A #ClayColorConfig object.
 *
 * Sets the color management configuration to use with this color selection.
 *
 * Since: 2.4
 */
void
clay_color_selection_set_config (ClayColorSelection *selection,
                                 ClayColorConfig    *config)
{
  g_return_if_fail (CLAY_IS_COLOR_SELECTION (selection));
  g_return_if_fail (config == NULL || CLAY_IS_COLOR_CONFIG (config));

  clay_color_selector_set_config (CLAY_COLOR_SELECTOR (selection->notebook),
                                  config);
  clay_color_selector_set_config (CLAY_COLOR_SELECTOR (selection->scales),
                                  config);
  clay_color_area_set_color_config (CLAY_COLOR_AREA (selection->old_color),
                                    config);
  clay_color_area_set_color_config (CLAY_COLOR_AREA (selection->new_color),
                                    config);
}

/**
 * clay_color_selection_get_notebook:
 * @selection: A #ClayColorSelection widget.
 *
 * Returns: (transfer none): The selection's #ClayColorNotebook.
 *
 * Since: 3.0
 */
GtkWidget *
clay_color_selection_get_notebook (ClayColorSelection *selection)
{
  g_return_val_if_fail (CLAY_IS_COLOR_SELECTION (selection), NULL);

  return selection->notebook;
}

/**
 * clay_color_selection_get_right_vbox:
 * @selection: A #ClayColorSelection widget.
 *
 * Returns: (transfer none) (type GtkBox): The selection's right #GtkBox which
 *          contains the color scales.
 *
 * Since: 3.0
 */
GtkWidget *
clay_color_selection_get_right_vbox (ClayColorSelection *selection)
{
  g_return_val_if_fail (CLAY_IS_COLOR_SELECTION (selection), NULL);

  return selection->right_vbox;
}


/*  private functions  */

static void
clay_color_selection_switch_page (GtkWidget          *widget,
                                  gpointer            page,
                                  guint               page_num,
                                  ClayColorSelection *selection)
{
  ClayColorNotebook *notebook;
  ClayColorSelector *current;
  gboolean           sensitive;

  notebook = CLAY_COLOR_NOTEBOOK (selection->notebook);

  current = clay_color_notebook_get_current_selector (notebook);

  sensitive = (CLAY_COLOR_SELECTOR_GET_CLASS (current)->set_channel != NULL);

  clay_color_selector_set_toggles_sensitive
    (CLAY_COLOR_SELECTOR (selection->scales), sensitive);
}

static void
clay_color_selection_notebook_changed (ClayColorSelector  *selector,
                                       GeglColor          *color,
                                       ClayColorSelection *selection)
{
  UpdateType update;

  update = UPDATE_SCALES | UPDATE_ENTRY | UPDATE_COLOR;

  g_object_unref (selection->color);
  selection->color = gegl_color_duplicate (color);

  clay_color_selection_update (selection, update);
  clay_color_selection_color_changed (selection);
}

static void
clay_color_selection_scales_changed (ClayColorSelector  *selector,
                                     GeglColor          *color,
                                     ClayColorSelection *selection)
{
  UpdateType update;

  update = UPDATE_ENTRY | UPDATE_NOTEBOOK | UPDATE_COLOR;

  g_object_unref (selection->color);
  selection->color = gegl_color_duplicate (color);

  clay_color_selection_update (selection, update);
  clay_color_selection_color_changed (selection);
}

static void
clay_color_selection_color_picked (GtkWidget          *widget,
                                   const GeglColor    *rgb,
                                   ClayColorSelection *selection)
{
  if (rgb)
    clay_color_selection_set_color (selection, (GeglColor *) rgb);
}

static void
clay_color_selection_entry_changed (ClayColorHexEntry  *entry,
                                    ClayColorSelection *selection)
{
  g_object_unref (selection->color);
  selection->color = clay_color_hex_entry_get_color (entry);

  clay_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_COLOR);
  clay_color_selection_color_changed (selection);
}

static void
clay_color_selection_channel_changed (ClayColorSelector        *selector,
                                      ClayColorSelectorChannel  channel,
                                      ClayColorSelection       *selection)
{
  selection->channel = channel;

  clay_color_selector_set_channel (CLAY_COLOR_SELECTOR (selection->notebook),
                                   selection->channel);
}

static void
clay_color_selection_new_color_changed (GtkWidget          *widget,
                                        ClayColorSelection *selection)
{
  g_object_unref (selection->color);
  selection->color = clay_color_area_get_color (CLAY_COLOR_AREA (widget));

  clay_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_ENTRY);
  clay_color_selection_color_changed (selection);
}

static void
clay_color_selection_update (ClayColorSelection *selection,
                             UpdateType          update)
{
  if (update & UPDATE_NOTEBOOK)
    {
      g_signal_handlers_block_by_func (selection->notebook,
                                       clay_color_selection_notebook_changed,
                                       selection);

      clay_color_selector_set_color (CLAY_COLOR_SELECTOR (selection->notebook), selection->color);

      g_signal_handlers_unblock_by_func (selection->notebook,
                                         clay_color_selection_notebook_changed,
                                         selection);
    }

  if (update & UPDATE_SCALES)
    {
      g_signal_handlers_block_by_func (selection->scales,
                                       clay_color_selection_scales_changed,
                                       selection);

      clay_color_selector_set_color (CLAY_COLOR_SELECTOR (selection->scales), selection->color);

      g_signal_handlers_unblock_by_func (selection->scales,
                                         clay_color_selection_scales_changed,
                                         selection);
    }

  if (update & UPDATE_ENTRY)
    {
      ClayColorHexEntry *entry;

      entry = g_object_get_data (G_OBJECT (selection), "color-hex-entry");

      g_signal_handlers_block_by_func (entry,
                                       clay_color_selection_entry_changed,
                                       selection);

      clay_color_hex_entry_set_color (entry, selection->color);

      g_signal_handlers_unblock_by_func (entry,
                                         clay_color_selection_entry_changed,
                                         selection);
    }

  if (update & UPDATE_COLOR)
    {
      g_signal_handlers_block_by_func (selection->new_color,
                                       clay_color_selection_new_color_changed,
                                       selection);

      clay_color_area_set_color (CLAY_COLOR_AREA (selection->new_color), selection->color);

      g_signal_handlers_unblock_by_func (selection->new_color,
                                         clay_color_selection_new_color_changed,
                                         selection);
    }
}
