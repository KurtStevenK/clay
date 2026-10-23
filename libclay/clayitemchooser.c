/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayitemchooser.h
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"

#include "clayuitypes.h"
#include "clayitemchooser.h"
#include "clayuimarshal.h"

#include "libclay-intl.h"


/**
 * SECTION: clayitemchooser
 * @title: ClayItemChooser
 * @short_description: A widget allowing to select an item.
 *
 * The chooser contains an optional label and a button which queries the core
 * process to pop up a item selection dialog.
 *
 * Since: 3.2
 **/

#define CELL_SIZE 40

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LABEL,
  PROP_ITEM,
  PROP_ITEM_TYPE,
  N_PROPS
};

struct _ClayItemChooser
{
  GtkBox        parent_instance;

  GType         item_type;
  ClayItem     *item;
  gchar        *title;
  gchar        *label;
  gchar        *callback;

  GBytes       *thumbnail;
  ClayItem     *thumbnail_item;
  gint          width;
  gint          height;
  gint          bpp;

  GtkWidget    *label_widget;
  GtkWidget    *preview_frame;
  GtkWidget    *preview;
  GtkWidget    *preview_title;
};


/*  local function prototypes  */

static void             clay_item_chooser_constructed       (GObject             *object);
static void             clay_item_chooser_dispose           (GObject             *object);
static void             clay_item_chooser_finalize          (GObject             *object);

static void             clay_item_chooser_set_property      (GObject             *object,
                                                             guint                property_id,
                                                             const GValue        *value,
                                                             GParamSpec          *pspec);
static void             clay_item_chooser_get_property      (GObject             *object,
                                                             guint                property_id,
                                                             GValue              *value,
                                                             GParamSpec          *pspec);

static void             clay_item_chooser_clicked           (ClayItemChooser     *chooser);

static ClayValueArray * clay_temp_callback_run              (ClayProcedure       *procedure,
                                                             ClayProcedureConfig *config,
                                                             ClayItemChooser     *chooser);
static gboolean         clay_item_select_remove_after_run   (const gchar         *procedure_name);

static void             clay_item_chooser_draw              (ClayItemChooser     *chooser);
static void             clay_item_chooser_get_thumbnail     (ClayItemChooser     *chooser,
                                                             gint                  width,
                                                             gint                  height);


static GParamSpec *item_button_props[N_PROPS] = { NULL, };

G_DEFINE_FINAL_TYPE (ClayItemChooser, clay_item_chooser, GTK_TYPE_BOX)


static void
clay_item_chooser_class_init (ClayItemChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_item_chooser_constructed;
  object_class->dispose      = clay_item_chooser_dispose;
  object_class->finalize     = clay_item_chooser_finalize;
  object_class->set_property = clay_item_chooser_set_property;
  object_class->get_property = clay_item_chooser_get_property;

  /**
   * ClayItemChooser:title:
   *
   * The title to be used for the item selection popup dialog.
   *
   * Since: 3.0
   */
  item_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the item selection popup dialog",
                         "Item Selection",
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayItemChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  item_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * ClayItemChooser:item:
   *
   * The currently selected item.
   *
   * Since: 3.0
   */
  item_button_props[PROP_ITEM] =
    clay_param_spec_item ("item",
                          "Item",
                          "The currently selected item",
                          TRUE,
                          CLAY_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClayItemChooser:item-type:
   *
   * Allowed item types, which must be either CLAY_TYPE_ITEM or a
   * subtype.
   *
   * Since: 3.0
   */
  item_button_props[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type",
                        "Allowed item Type",
                        "The GType of the item property",
                        CLAY_TYPE_ITEM,
                        G_PARAM_CONSTRUCT_ONLY |
                        CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS, item_button_props);
}

static void
clay_item_chooser_init (ClayItemChooser *chooser)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (chooser), 6);

  chooser->thumbnail_item = NULL;
  chooser->thumbnail      = NULL;
}

static void
clay_item_chooser_constructed (GObject *object)
{
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (object);
  GtkWidget       *button;
  GtkWidget       *box;
  gint             scale_factor;

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (chooser));

  chooser->label_widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (chooser), chooser->label_widget, FALSE, FALSE, 0);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (chooser->label_widget), chooser->label);
  gtk_widget_set_visible (GTK_WIDGET (chooser->label_widget), TRUE);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (chooser), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (chooser->label_widget), button);
  gtk_widget_set_visible (button, TRUE);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (button), box);
  gtk_widget_set_visible (box, TRUE);

  chooser->preview_frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (chooser->preview_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (chooser->preview_frame, TRUE);

  chooser->preview = clay_preview_area_new ();
  gtk_widget_set_size_request (chooser->preview, scale_factor * CELL_SIZE, scale_factor * CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (chooser->preview_frame), chooser->preview);
  gtk_widget_set_visible (chooser->preview, TRUE);

  chooser->preview_title = gtk_label_new (_("Browse..."));
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_title, FALSE, FALSE, 0);
  gtk_widget_set_visible (chooser->preview_title, TRUE);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (clay_item_chooser_clicked),
                            chooser);

  G_OBJECT_CLASS (clay_item_chooser_parent_class)->constructed (object);
}

static void
clay_item_chooser_dispose (GObject *object)
{
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (object);

  if (chooser->callback)
    {
      clay_items_close_popup (chooser->callback);

      clay_plug_in_remove_temp_procedure (clay_get_plug_in (),
                                          chooser->callback);
      g_clear_pointer (&chooser->callback, g_free);
    }

  G_OBJECT_CLASS (clay_item_chooser_parent_class)->dispose (object);
}

static void
clay_item_chooser_finalize (GObject *object)
{
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (object);

  g_clear_pointer (&chooser->title, g_free);
  g_clear_pointer (&chooser->label, g_free);
  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);

  G_OBJECT_CLASS (clay_item_chooser_parent_class)->finalize (object);
}

static void
clay_item_chooser_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *gvalue,
                                GParamSpec   *pspec)
{
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      chooser->title = g_value_dup_string (gvalue);
      break;

    case PROP_LABEL:
      chooser->label = g_value_dup_string (gvalue);
      break;

    case PROP_ITEM:
      g_return_if_fail (g_value_get_object (gvalue) == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (g_value_get_object (gvalue)),
                                     chooser->item_type));
      clay_item_chooser_set_item (chooser, g_value_get_object (gvalue));
      break;

    case PROP_ITEM_TYPE:
      chooser->item_type = g_value_get_gtype (gvalue);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_item_chooser_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayItemChooser *chooser = CLAY_ITEM_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, chooser->title);
      break;

    case PROP_LABEL:
      g_value_set_string (value, chooser->label);
      break;

    case PROP_ITEM:
      g_value_set_object (value, chooser->item);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, chooser->item_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_item_chooser_new:
 * @title:    (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label:    (nullable): Button label or %NULL for no label.
 * @item_type: the acceptable subtype of choosable items.
 * @item: (nullable): Initial item.
 *
 * Creates a new #GtkWidget that lets a user choose a item which must be of
 * type @item_type. @item_type of values %G_TYPE_NONE and
 * %CLAY_TYPE_ITEM are equivalent. Otherwise it must be a subtype of
 * %CLAY_TYPE_ITEM.
 *
 * When @item is %NULL, initial choice is from context.
 *
 * Returns: A [class@ClayUi.ItemChooser.
 *
 * Since: 3.2
 */
GtkWidget *
clay_item_chooser_new (const gchar  *title,
                       const gchar  *label,
                       GType         item_type,
                       ClayItem     *item)
{
  GtkWidget *chooser;

  if (item_type == G_TYPE_NONE)
    item_type = CLAY_TYPE_ITEM;

  g_return_val_if_fail (g_type_is_a (item_type, CLAY_TYPE_ITEM), NULL);
  g_return_val_if_fail (item == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (item), item_type),
                        NULL);

  chooser = g_object_new (CLAY_TYPE_ITEM_CHOOSER,
                          "title",     title,
                          "label",     label,
                          "item",      item,
                          "item-type", item_type,
                          NULL);

  return chooser;
}

/**
 * clay_item_chooser_get_item:
 * @chooser: A #ClayItemChooser
 *
 * Gets the currently selected item.
 *
 * Returns: (transfer none): an internal copy of the item which must not be freed.
 *
 * Since: 3.2
 */
ClayItem *
clay_item_chooser_get_item (ClayItemChooser *chooser)
{
  g_return_val_if_fail (CLAY_IS_ITEM_CHOOSER (chooser), NULL);

  return chooser->item;
}

/**
 * clay_item_chooser_set_item:
 * @chooser: A #ClayItemChooser
 * @item: Item to set.
 *
 * Sets the currently selected item.
 * This will select the item in both the button and any chooser popup.
 *
 * Since: 3.2
 */
void
clay_item_chooser_set_item (ClayItemChooser *chooser,
                            ClayItem        *item)
{
  g_return_if_fail (CLAY_IS_ITEM_CHOOSER (chooser));
  g_return_if_fail (item == NULL || CLAY_IS_ITEM (item));

  chooser->item = item;

  if (chooser->callback)
    clay_items_set_popup (chooser->callback, chooser->item);

  g_object_notify_by_pspec (G_OBJECT (chooser), item_button_props[PROP_ITEM]);

  clay_item_chooser_draw (chooser);
}

/**
 * clay_item_chooser_get_label:
 * @widget: A [class@ItemChooser].
 *
 * Returns the label widget.
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 * Since: 3.2
 */
GtkWidget *
clay_item_chooser_get_label (ClayItemChooser *chooser)
{
  g_return_val_if_fail (CLAY_IS_ITEM_CHOOSER (chooser), NULL);

  return chooser->label_widget;
}


/*  private functions  */

static ClayValueArray *
clay_temp_callback_run (ClayProcedure       *procedure,
                        ClayProcedureConfig *config,
                        ClayItemChooser     *chooser)
{
  ClayItem *item;
  gboolean      closing;

  g_object_get (config,
                "item",    &item,
                "closing", &closing,
                NULL);

  g_object_set (chooser, "item", item, NULL);

  if (closing)
    {
      g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                       (GSourceFunc) clay_item_select_remove_after_run,
                       g_strdup (clay_procedure_get_name (procedure)),
                       g_free);
      g_clear_pointer (&chooser->callback, g_free);
    }

  g_clear_object (&item);

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}

static gboolean
clay_item_select_remove_after_run (const gchar *procedure_name)
{
  clay_plug_in_remove_temp_procedure (clay_get_plug_in (), procedure_name);

  return G_SOURCE_REMOVE;
}

static void
clay_item_chooser_clicked (ClayItemChooser *chooser)
{
  if (chooser->callback)
    {
      /* Popup already created.  Calling setter raises the popup. */
      clay_items_set_popup (chooser->callback, chooser->item);
    }
  else
    {
      ClayPlugIn    *plug_in  = clay_get_plug_in ();
      GtkWidget     *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
      GBytes        *handle   = NULL;
      gchar         *callback_name;
      ClayProcedure *callback_procedure;

      if (CLAY_IS_DIALOG (toplevel))
        handle = clay_dialog_get_native_handle (CLAY_DIALOG (toplevel));

      callback_name = clay_pdb_temp_procedure_name (clay_get_pdb ());
      callback_procedure = clay_procedure_new (plug_in,
                                               callback_name,
                                               CLAY_PDB_PROC_TYPE_TEMPORARY,
                                               (ClayRunFunc) clay_temp_callback_run,
                                               g_object_ref (chooser),
                                               (GDestroyNotify) g_object_unref);
      clay_procedure_add_item_argument (callback_procedure, "item",
                                        "Item", "The selected item",
                                        TRUE, G_PARAM_READWRITE);
      clay_procedure_add_boolean_argument (callback_procedure, "closing",
                                           "Closing", "If the dialog was closing",
                                           FALSE, G_PARAM_READWRITE);

      clay_plug_in_add_temp_procedure (plug_in, callback_procedure);
      g_object_unref (callback_procedure);
      g_free (callback_name);

      if (clay_items_popup (clay_procedure_get_name (callback_procedure), chooser->title,
                            g_type_name (chooser->item_type), chooser->item, handle))
        {
          /* Allow callbacks to be watched */
          clay_plug_in_persistent_enable (plug_in);

          chooser->callback = g_strdup (clay_procedure_get_name (callback_procedure));
        }
      else
        {
          g_warning ("%s: failed to open remote item select dialog.", G_STRFUNC);
          clay_plug_in_remove_temp_procedure (plug_in, clay_procedure_get_name (callback_procedure));
          return;
        }
      clay_items_set_popup (chooser->callback, chooser->item);
    }
}

static void
clay_item_chooser_draw (ClayItemChooser *chooser)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (chooser->preview);
  GtkAllocation    allocation;
  gint             x = 0;
  gint             y = 0;

  gtk_widget_get_allocation (chooser->preview, &allocation);

  clay_item_chooser_get_thumbnail (chooser, allocation.width, allocation.height);

  if (chooser->width < allocation.width ||
      chooser->height < allocation.height)
    {
      x = ((allocation.width  - chooser->width)  / 2);
      y = ((allocation.height - chooser->height) / 2);
    }
  clay_preview_area_reset (area);

  if (chooser->thumbnail)
    {
      ClayImageType type;
      gint          rowstride;

      rowstride = chooser->width * chooser->bpp;
      switch (chooser->bpp)
        {
        case 1:
          type = CLAY_GRAY_IMAGE;
          break;
        case 2:
          type = CLAY_GRAYA_IMAGE;
          break;
        case 3:
          type = CLAY_RGB_IMAGE;
          break;
        case 4:
          type = CLAY_RGBA_IMAGE;
          break;
        default:
          g_return_if_reached ();
        }

      clay_preview_area_draw (area, x, y, chooser->width, chooser->height, type,
                              g_bytes_get_data (chooser->thumbnail, NULL), rowstride);
    }
  else
    {
      /* TODO: ClayItems like ClayPath do not have a built-in drawable,
       * but it is possible to get a preview from it. We should add a
       * standard way to generate this preview. */
      if (chooser->item)
        gtk_label_set_text (GTK_LABEL (chooser->preview_title),
                            clay_item_get_name (chooser->item));
    }
}

static void
clay_item_chooser_get_thumbnail (ClayItemChooser *chooser,
                                 gint             width,
                                 gint             height)
{
  if (chooser->item   == chooser->thumbnail_item &&
      chooser->width  == width                   &&
      chooser->height == height)
    /* Let's assume item contents is not changing in a single run. */
    return;

  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);
  chooser->width = chooser->height = 0;

  chooser->thumbnail_item = chooser->item;
  if (chooser->item)
    {
      if (CLAY_IS_DRAWABLE (chooser->item))
        {
          chooser->thumbnail =
           clay_drawable_get_thumbnail_data (CLAY_DRAWABLE (chooser->item),
                                             width, height,
                                             &chooser->width,
                                             &chooser->height,
                                             &chooser->bpp);
        }
    }
}
