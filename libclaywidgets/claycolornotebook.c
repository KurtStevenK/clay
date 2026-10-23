/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolornotebook.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claycolornotebook.h"
#include "claycolorscales.h"
#include "clayhelpui.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claycolornotebook
 * @title: ClayColorNotebook
 * @short_description: A #ClayColorSelector implementation.
 *
 * The #ClayColorNotebook widget is an implementation of a
 * #ClayColorSelector. It serves as a container for
 * #ClayColorSelectors.
 **/


#define DEFAULT_TAB_ICON_SIZE  GTK_ICON_SIZE_BUTTON


struct _ClayColorNotebook
{
  ClayColorSelector  parent_instance;

  GtkWidget         *notebook;

  GList             *selectors;
  ClayColorSelector *cur_page;
};


static void   clay_color_notebook_style_updated   (GtkWidget         *widget);

static void   clay_color_notebook_togg_visible    (ClayColorSelector *selector,
                                                   gboolean           visible);
static void   clay_color_notebook_togg_sensitive  (ClayColorSelector *selector,
                                                   gboolean           sensitive);
static void   clay_color_notebook_set_show_alpha  (ClayColorSelector *selector,
                                                   gboolean           show_alpha);
static void   clay_color_notebook_set_color       (ClayColorSelector *selector,
                                                   GeglColor         *color);
static void   clay_color_notebook_set_channel     (ClayColorSelector *selector,
                                                   ClayColorSelectorChannel channel);
static void   clay_color_notebook_set_model_visible
                                                  (ClayColorSelector *selector,
                                                   ClayColorSelectorModel model,
                                                   gboolean           gboolean);
static void   clay_color_notebook_set_config      (ClayColorSelector *selector,
                                                   ClayColorConfig   *config);


static void   clay_color_notebook_switch_page     (GtkNotebook       *gtk_notebook,
                                                   gpointer           page,
                                                   guint              page_num,
                                                   ClayColorNotebook *notebook);

static void   clay_color_notebook_color_changed   (ClayColorSelector *page,
                                                   GeglColor         *color,
                                                   ClayColorNotebook *notebook);
static void   clay_color_notebook_channel_changed (ClayColorSelector *page,
                                                   ClayColorSelectorChannel channel,
                                                   ClayColorNotebook *notebook);
static void   clay_color_notebook_model_visible_changed
                                                  (ClayColorSelector *page,
                                                   ClayColorSelectorModel model,
                                                   gboolean           visible,
                                                   ClayColorNotebook *notebook);

static GtkWidget * clay_color_notebook_add_page   (ClayColorNotebook *notebook,
                                                   GType              page_type);
static void   clay_color_notebook_remove_selector (GtkContainer      *container,
                                                   GtkWidget         *widget,
                                                   ClayColorNotebook *notebook);


G_DEFINE_TYPE (ClayColorNotebook, clay_color_notebook, CLAY_TYPE_COLOR_SELECTOR)

#define parent_class clay_color_notebook_parent_class


static void
clay_color_notebook_class_init (ClayColorNotebookClass *klass)
{
  GtkWidgetClass         *widget_class   = GTK_WIDGET_CLASS (klass);
  ClayColorSelectorClass *selector_class = CLAY_COLOR_SELECTOR_CLASS (klass);

  widget_class->style_updated           = clay_color_notebook_style_updated;

  selector_class->name                  = "Notebook";
  selector_class->help_id               = "clay-colorselector-notebook";
  selector_class->set_toggles_visible   = clay_color_notebook_togg_visible;
  selector_class->set_toggles_sensitive = clay_color_notebook_togg_sensitive;
  selector_class->set_show_alpha        = clay_color_notebook_set_show_alpha;
  selector_class->set_color             = clay_color_notebook_set_color;
  selector_class->set_channel           = clay_color_notebook_set_channel;
  selector_class->set_model_visible     = clay_color_notebook_set_model_visible;
  selector_class->set_config            = clay_color_notebook_set_config;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL,
                                                              "Size for icons displayed in the tab",
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              G_PARAM_READABLE));

  gtk_widget_class_set_css_name (widget_class, "ClayColorNotebook");
}

static void
clay_color_notebook_init (ClayColorNotebook *notebook)
{
  GType *selector_types;
  guint  n_selector_types;
  guint  i;

  notebook->notebook = gtk_notebook_new ();
  gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook->notebook));
  gtk_box_pack_start (GTK_BOX (notebook), notebook->notebook, TRUE, TRUE, 0);
  gtk_widget_set_visible (notebook->notebook, TRUE);

  g_signal_connect (notebook->notebook, "switch-page",
                    G_CALLBACK (clay_color_notebook_switch_page),
                    notebook);
  g_signal_connect (notebook->notebook, "remove",
                    G_CALLBACK (clay_color_notebook_remove_selector),
                    notebook);

  selector_types = g_type_children (CLAY_TYPE_COLOR_SELECTOR,
                                    &n_selector_types);

  if (n_selector_types == 2)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook->notebook), FALSE);
    }

  for (i = 0; i < n_selector_types; i++)
    {
      /*  skip ourselves  */
      if (g_type_is_a (selector_types[i], CLAY_TYPE_COLOR_NOTEBOOK))
        continue;

      /*  skip the "Scales" color selector  */
      if (g_type_is_a (selector_types[i], CLAY_TYPE_COLOR_SCALES))
        continue;

      clay_color_notebook_add_page (notebook, selector_types[i]);
    }

  g_free (selector_types);
}

static void
clay_color_notebook_style_updated (GtkWidget *widget)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (widget);
  GList             *list;
  GtkIconSize        icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "tab-icon-size", &icon_size,
                        NULL);

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelectorClass *selector_class;
      GtkWidget              *image;

      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (list->data);

      image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                            icon_size);
      clay_help_set_help_data (image, gettext (selector_class->name), NULL);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook->notebook),
                                  GTK_WIDGET (list->data),
                                  image);
    }
}

static void
clay_color_notebook_togg_visible (ClayColorSelector *selector,
                                  gboolean           visible)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);
  GList             *list;

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *child = list->data;

      clay_color_selector_set_toggles_visible (child, visible);
    }
}

static void
clay_color_notebook_togg_sensitive (ClayColorSelector *selector,
                                    gboolean           sensitive)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);
  GList             *list;

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *child = list->data;

      clay_color_selector_set_toggles_sensitive (child, sensitive);
    }
}

static void
clay_color_notebook_set_show_alpha (ClayColorSelector *selector,
                                    gboolean           show_alpha)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);
  GList             *list;

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *child = list->data;

      clay_color_selector_set_show_alpha (child, show_alpha);
    }
}

static void
clay_color_notebook_set_color (ClayColorSelector *selector,
                               GeglColor         *color)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);

  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_color_changed,
                                   selector);

  clay_color_selector_set_color (notebook->cur_page, color);

  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_color_changed,
                                     selector);
}

static void
clay_color_notebook_set_channel (ClayColorSelector        *selector,
                                 ClayColorSelectorChannel  channel)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);

  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_channel_changed,
                                   selector);

  clay_color_selector_set_channel (notebook->cur_page, channel);

  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_channel_changed,
                                     selector);
}

static void
clay_color_notebook_set_model_visible (ClayColorSelector      *selector,
                                       ClayColorSelectorModel  model,
                                       gboolean                visible)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);

  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_model_visible_changed,
                                   selector);

  clay_color_selector_set_model_visible (notebook->cur_page, model, visible);

  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_model_visible_changed,
                                     selector);
}

static void
clay_color_notebook_set_config (ClayColorSelector *selector,
                                ClayColorConfig   *config)
{
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (selector);
  GList             *list;

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *child = list->data;

      clay_color_selector_set_config (child, config);
    }
}

static void
clay_color_notebook_switch_page (GtkNotebook       *gtk_notebook,
                                 gpointer           page,
                                 guint              page_num,
                                 ClayColorNotebook *notebook)
{
  ClayColorSelector      *selector = CLAY_COLOR_SELECTOR (notebook);
  GtkWidget              *page_widget;
  GeglColor              *color;
  ClayColorSelectorModel  model;

  page_widget = gtk_notebook_get_nth_page (gtk_notebook, page_num);

  notebook->cur_page = CLAY_COLOR_SELECTOR (page_widget);

  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_color_changed,
                                   notebook);
  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_channel_changed,
                                   notebook);
  g_signal_handlers_block_by_func (notebook->cur_page,
                                   clay_color_notebook_model_visible_changed,
                                   notebook);

  color = clay_color_selector_get_color (selector);
  clay_color_selector_set_color (notebook->cur_page, color);
  g_object_unref (color);
  clay_color_selector_set_channel (notebook->cur_page,
                                   clay_color_selector_get_channel (selector));

  for (model = CLAY_COLOR_SELECTOR_MODEL_RGB;
       model <= CLAY_COLOR_SELECTOR_MODEL_HSV;
       model++)
    {
      gboolean visible = clay_color_selector_get_model_visible (selector, model);

      clay_color_selector_set_model_visible (notebook->cur_page, model,
                                             visible);
    }

  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_color_changed,
                                     notebook);
  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_channel_changed,
                                     notebook);
  g_signal_handlers_unblock_by_func (notebook->cur_page,
                                     clay_color_notebook_model_visible_changed,
                                     notebook);
}

static void
clay_color_notebook_color_changed (ClayColorSelector *page,
                                   GeglColor         *color,
                                   ClayColorNotebook *notebook)
{
  ClayColorSelector *selector = CLAY_COLOR_SELECTOR (notebook);

  clay_color_selector_set_color (selector, color);
}

static void
clay_color_notebook_channel_changed (ClayColorSelector        *page,
                                     ClayColorSelectorChannel  channel,
                                     ClayColorNotebook        *notebook)
{
  ClayColorSelector *selector = CLAY_COLOR_SELECTOR (notebook);

  clay_color_selector_set_channel (selector, channel);
}

static void
clay_color_notebook_model_visible_changed (ClayColorSelector      *page,
                                           ClayColorSelectorModel  model,
                                           gboolean                visible,
                                           ClayColorNotebook      *notebook)
{
  ClayColorSelector *selector = CLAY_COLOR_SELECTOR (notebook);

  clay_color_selector_set_model_visible (selector, model, visible);
}

static GtkWidget *
clay_color_notebook_add_page (ClayColorNotebook *notebook,
                              GType              page_type)
{
  ClayColorSelector        *selector = CLAY_COLOR_SELECTOR (notebook);
  ClayColorSelectorClass   *selector_class;
  GtkWidget                *page;
  GtkWidget                *menu_widget;
  GtkWidget                *image;
  GtkWidget                *label;
  GeglColor                *color;
  gboolean                  show_alpha;

  color = clay_color_selector_get_color (selector);
  page = clay_color_selector_new (page_type, color,
                                  clay_color_selector_get_channel (selector));
  g_object_unref (color);

  if (! page)
    return NULL;

  selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (page);

  show_alpha = clay_color_selector_get_show_alpha (CLAY_COLOR_SELECTOR (notebook));
  clay_color_selector_set_show_alpha (CLAY_COLOR_SELECTOR (page), show_alpha);

  menu_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                        GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (menu_widget), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  label = gtk_label_new (gettext (selector_class->name));
  gtk_box_pack_start (GTK_BOX (menu_widget), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                        DEFAULT_TAB_ICON_SIZE);
  clay_help_set_help_data (image, gettext (selector_class->name), NULL);

  gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook->notebook),
                                 page, image, menu_widget);

  if (! notebook->cur_page)
    notebook->cur_page = CLAY_COLOR_SELECTOR (page);

  notebook->selectors = g_list_append (notebook->selectors, page);

  gtk_widget_set_visible (page, TRUE);

  g_signal_connect (page, "color-changed",
                    G_CALLBACK (clay_color_notebook_color_changed),
                    notebook);
  g_signal_connect (page, "channel-changed",
                    G_CALLBACK (clay_color_notebook_channel_changed),
                    notebook);
  g_signal_connect (page, "model-visible-changed",
                    G_CALLBACK (clay_color_notebook_model_visible_changed),
                    notebook);

  return page;
}

static void
clay_color_notebook_remove_selector (GtkContainer      *container,
                                     GtkWidget         *widget,
                                     ClayColorNotebook *notebook)
{
  notebook->selectors = g_list_remove (notebook->selectors, widget);

  if (! notebook->selectors)
    notebook->cur_page = NULL;
}


/**
 * clay_color_notebook_set_has_page:
 * @notebook:  A #ClayColorNotebook widget.
 * @page_type: The #GType of the notebook page to add or remove.
 * @has_page:  Whether the page should be added or removed.
 *
 * This function adds and removed pages to / from a #ClayColorNotebook.
 * The @page_type passed must be a #ClayColorSelector subtype.
 *
 * Returns: (transfer none): The new page widget, if @has_page was
 *               %TRUE, or %NULL if @has_page was %FALSE.
 **/
GtkWidget *
clay_color_notebook_set_has_page (ClayColorNotebook *notebook,
                                  GType              page_type,
                                  gboolean           has_page)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (g_type_is_a (page_type, CLAY_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (! g_type_is_a (page_type, CLAY_TYPE_COLOR_NOTEBOOK),
                        NULL);

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *page = list->data;

      if (G_TYPE_FROM_INSTANCE (page) == page_type)
        {
          if (has_page)
            return GTK_WIDGET (page);

          gtk_container_remove (GTK_CONTAINER (notebook->notebook),
                                GTK_WIDGET (page));

          return NULL;
        }
    }

  if (! has_page)
    return NULL;

  return clay_color_notebook_add_page (notebook, page_type);
}

/**
 * clay_color_notebook_get_notebook:
 * @notebook:  A #ClayColorNotebook widget.
 *
 * Returns: (transfer none) (type GtkNotebook): The #GtkNotebook inside.
 *
 * Since: 3.0
 **/
GtkWidget *
clay_color_notebook_get_notebook (ClayColorNotebook *notebook)
{
  g_return_val_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->notebook;
}

/**
 * clay_color_notebook_get_selectors:
 * @notebook:  A #ClayColorNotebook widget.
 *
 * Returns: (element-type ClayColorSelector) (transfer none): The
 *               notebook's list of #ClayColorSelector's.
 *
 * Since: 3.0
 **/
GList *
clay_color_notebook_get_selectors (ClayColorNotebook *notebook)
{
  g_return_val_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->selectors;
}

/**
 * clay_color_notebook_get_current_selector:
 * @notebook:  A #ClayColorNotebook widget.
 *
 * Returns: (transfer none): The active page's #ClayColorSelector.
 *
 * Since: 3.0
 **/
ClayColorSelector *
clay_color_notebook_get_current_selector (ClayColorNotebook *notebook)
{
  g_return_val_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->cur_page;
}

/**
 * clay_color_notebook_set_format:
 * @notebook:  A #ClayColorNotebook widget.
 * @format:    A Babl format, with space.
 *
 * Updates all selectors with the current format.
 *
 * Since: 3.0
 **/
void
clay_color_notebook_set_format (ClayColorNotebook *notebook,
                                const Babl        *format)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook));

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *selector = list->data;

      if (selector)
        clay_color_selector_set_format (selector, format);
    }
}

/**
 * clay_color_notebook_set_simulation:
 * @notebook:  A #ClayColorNotebook widget.
 * @profile:   A #ClayColorProfile object.
 * @intent:    A #ClayColorRenderingIntent enum.
 * @bpc:       A gboolean.
 *
 * Updates all selectors with the current simulation settings.
 *
 * Since: 3.0
 **/
void
clay_color_notebook_set_simulation (ClayColorNotebook *notebook,
                                    ClayColorProfile  *profile,
                                    ClayColorRenderingIntent intent,
                                    gboolean           bpc)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook));
  g_return_if_fail (profile == NULL || CLAY_IS_COLOR_PROFILE (profile));

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *selector = list->data;

      if (selector)
        clay_color_selector_set_simulation (selector, profile, intent, bpc);
    }
}

void
clay_color_notebook_enable_simulation (ClayColorNotebook *notebook,
                                       gboolean           enabled)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_NOTEBOOK (notebook));

  for (list = notebook->selectors; list; list = g_list_next (list))
    {
      ClayColorSelector *selector = list->data;

      if (selector)
        clay_color_selector_enable_simulation (selector, enabled);
    }
}
