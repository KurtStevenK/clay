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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/claypalette.h"

#include "claycolordialog.h"
#include "claydnd.h"
#include "claydocked.h"
#include "claydialogfactory.h"
#include "clayhelp-ids.h"
#include "claypaletteeditor.h"
#include "claypaletteview.h"
#include "claysessioninfo-aux.h"
#include "clayuimanager.h"
#include "clayviewrendererpalette.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH  + SPACING) * COLUMNS + 1)
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT + SPACING) * ROWS    + 1)


/*  local function prototypes  */

static void   clay_palette_editor_docked_iface_init (ClayDockedInterface *face);

static void   clay_palette_editor_constructed      (GObject           *object);
static void   clay_palette_editor_dispose          (GObject           *object);

static void   clay_palette_editor_unmap            (GtkWidget         *widget);

static void   clay_palette_editor_set_data         (ClayDataEditor    *editor,
                                                    ClayData          *data);

static void   clay_palette_editor_set_context      (ClayDocked        *docked,
                                                    ClayContext       *context);
static void   clay_palette_editor_set_aux_info     (ClayDocked        *docked,
                                                    GList             *aux_info);
static GList *clay_palette_editor_get_aux_info     (ClayDocked        *docked);

static void   palette_editor_invalidate_preview    (ClayPalette       *palette,
                                                    ClayPaletteEditor *editor);

static void   palette_editor_viewport_size_allocate(GtkWidget         *widget,
                                                    GtkAllocation     *allocation,
                                                    ClayPaletteEditor *editor);

static void   palette_editor_drop_palette          (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    ClayViewable      *viewable,
                                                    gpointer           data);
static void   palette_editor_drop_color            (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    GeglColor         *color,
                                                    gpointer           data);

static void   palette_editor_entry_clicked         (ClayPaletteView   *view,
                                                    ClayPaletteEntry  *entry,
                                                    GdkModifierType    state,
                                                    ClayPaletteEditor *editor);
static void   palette_editor_entry_selected        (ClayPaletteView   *view,
                                                    ClayPaletteEntry  *entry,
                                                    ClayPaletteEditor *editor);
static void   palette_editor_entry_activated       (ClayPaletteView   *view,
                                                    ClayPaletteEntry  *entry,
                                                    ClayPaletteEditor *editor);
static gboolean palette_editor_button_press_event  (GtkWidget *widget,
                                                    GdkEvent  *event,
                                                    gpointer   user_data);
static gboolean palette_editor_popup_menu          (GtkWidget *widget,
                                                    gpointer   user_data);
static void   palette_editor_color_dropped         (ClayPaletteView   *view,
                                                    ClayPaletteEntry  *entry,
                                                    GeglColor         *color,
                                                    ClayPaletteEditor *editor);

static void   palette_editor_color_name_changed    (GtkWidget         *widget,
                                                    ClayPaletteEditor *editor);
static void   palette_editor_columns_changed       (GtkAdjustment     *adj,
                                                    ClayPaletteEditor *editor);

static void   palette_editor_resize                (ClayPaletteEditor *editor,
                                                    gint               width,
                                                    gdouble            zoom_factor);
static void   palette_editor_scroll_top_left       (ClayPaletteEditor *editor);
static void   palette_editor_edit_color_update     (ClayColorDialog   *dialog,
                                                    GeglColor         *color,
                                                    ClayColorDialogState state,
                                                    ClayPaletteEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClayPaletteEditor, clay_palette_editor,
                         CLAY_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_palette_editor_docked_iface_init))

#define parent_class clay_palette_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_palette_editor_class_init (ClayPaletteEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);
  ClayDataEditorClass *editor_class = CLAY_DATA_EDITOR_CLASS (klass);

  object_class->constructed = clay_palette_editor_constructed;
  object_class->dispose     = clay_palette_editor_dispose;

  widget_class->unmap       = clay_palette_editor_unmap;

  editor_class->set_data    = clay_palette_editor_set_data;
  editor_class->title       = _("Palette Editor");
}

static void
clay_palette_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context  = clay_palette_editor_set_context;
  iface->set_aux_info = clay_palette_editor_set_aux_info;
  iface->get_aux_info = clay_palette_editor_get_aux_info;
}

static void
clay_palette_editor_init (ClayPaletteEditor *editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);
  GtkWidget      *viewport;
  GtkWidget      *hbox;
  GtkWidget      *icon;
  GtkWidget      *spinbutton;

  editor->zoom_factor = 1.0;
  editor->col_width   = 0;
  editor->last_width  = 0;
  editor->columns     = COLUMNS;

  data_editor->view = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (data_editor->view, -1, PREVIEW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (data_editor->view),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (editor), data_editor->view, TRUE, TRUE, 0);
  gtk_widget_set_visible (data_editor->view, TRUE);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (data_editor->view), viewport);
  gtk_widget_set_visible (viewport, TRUE);

  editor->view = clay_view_new_full_by_types (NULL,
                                              CLAY_TYPE_PALETTE_VIEW,
                                              CLAY_TYPE_PALETTE,
                                              PREVIEW_WIDTH, PREVIEW_HEIGHT, 0,
                                              FALSE, TRUE, FALSE);
  clay_view_renderer_palette_set_cell_size
    (CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (editor->view)->renderer), -1);
  clay_view_renderer_palette_set_draw_grid
    (CLAY_VIEW_RENDERER_PALETTE (CLAY_VIEW (editor->view)->renderer), TRUE);
  gtk_container_add (GTK_CONTAINER (viewport), editor->view);
  gtk_widget_set_visible (editor->view, TRUE);

  g_signal_connect (gtk_widget_get_parent (editor->view), "size-allocate",
                    G_CALLBACK (palette_editor_viewport_size_allocate),
                    editor);

  g_signal_connect (editor->view, "entry-clicked",
                    G_CALLBACK (palette_editor_entry_clicked),
                    editor);
  g_signal_connect (editor->view, "entry-selected",
                    G_CALLBACK (palette_editor_entry_selected),
                    editor);
  g_signal_connect (editor->view, "entry-activated",
                    G_CALLBACK (palette_editor_entry_activated),
                    editor);
  g_signal_connect (editor->view, "color-dropped",
                    G_CALLBACK (palette_editor_color_dropped),
                    editor);
  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (palette_editor_button_press_event),
                    editor);
  g_signal_connect (editor->view, "popup-menu",
                    G_CALLBACK (palette_editor_popup_menu),
                    editor);

  clay_dnd_viewable_dest_add (editor->view,
                              CLAY_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);
  clay_dnd_viewable_dest_add (gtk_widget_get_parent (editor->view),
                              CLAY_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);

  clay_dnd_color_dest_add (gtk_widget_get_parent (editor->view),
                           palette_editor_drop_color,
                           editor);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  The color index number  */
  editor->index_label = gtk_label_new ("####");
  gtk_box_pack_start (GTK_BOX (hbox), editor->index_label, FALSE, FALSE, 0);
  clay_label_set_attributes (GTK_LABEL (editor->index_label),
                             PANGO_ATTR_FAMILY, "Monospace", -1);
  gtk_widget_set_visible (editor->index_label, TRUE);

  /*  The color name entry  */
  editor->color_name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->color_name, TRUE, TRUE, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_name), 1);
  gtk_entry_set_text (GTK_ENTRY (editor->color_name), _("Undefined"));
  gtk_editable_set_editable (GTK_EDITABLE (editor->color_name), FALSE);
  gtk_widget_set_visible (editor->color_name, TRUE);

  g_signal_connect (editor->color_name, "changed",
                    G_CALLBACK (palette_editor_color_name_changed),
                    editor);

  icon = gtk_image_new_from_icon_name (CLAY_ICON_GRID, GTK_ICON_SIZE_MENU);
  gtk_widget_set_margin_start (icon, 2);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
  gtk_widget_set_visible (icon, TRUE);

  editor->columns_adj = gtk_adjustment_new (0, 0, 64, 1, 4, 0);
  spinbutton = clay_spin_button_new (editor->columns_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_set_visible (spinbutton, TRUE);

  clay_help_set_help_data (spinbutton, _("Set the number of columns"), NULL);

  g_signal_connect (editor->columns_adj, "value-changed",
                    G_CALLBACK (palette_editor_columns_changed),
                    editor);
}

static void
clay_palette_editor_constructed (GObject *object)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-edit-color", NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-new-color-fg",
                                 "palette-editor-new-color-bg",
                                 clay_get_toggle_behavior_mask (),
                                 NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-delete-color", NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-out", NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-in", NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-all", NULL);
}

static void
clay_palette_editor_dispose (GObject *object)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (object);

  g_clear_pointer (&editor->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_palette_editor_unmap (GtkWidget *widget)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_set_visible (editor->color_dialog, FALSE);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
clay_palette_editor_set_data (ClayDataEditor *editor,
                              ClayData       *data)
{
  ClayPaletteEditor *palette_editor = CLAY_PALETTE_EDITOR (editor);

  g_signal_handlers_block_by_func (palette_editor->columns_adj,
                                   palette_editor_columns_changed,
                                   editor);

  if (editor->data)
    {
      if (palette_editor->color_dialog)
        {
          gtk_widget_destroy (palette_editor->color_dialog);
          palette_editor->color_dialog = NULL;
        }

      g_signal_handlers_disconnect_by_func (editor->data,
                                            palette_editor_invalidate_preview,
                                            editor);

      gtk_adjustment_set_value (palette_editor->columns_adj, 0);
    }

  CLAY_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  clay_view_set_viewable (CLAY_VIEW (palette_editor->view),
                          CLAY_VIEWABLE (data));

  if (editor->data)
    {
      ClayPalette *palette = CLAY_PALETTE (editor->data);

      g_signal_connect (editor->data, "invalidate-preview",
                        G_CALLBACK (palette_editor_invalidate_preview),
                        editor);

      gtk_adjustment_set_value (palette_editor->columns_adj,
                                clay_palette_get_columns (palette));

      palette_editor_scroll_top_left (palette_editor);

      palette_editor_invalidate_preview (CLAY_PALETTE (editor->data),
                                         palette_editor);
    }

  g_signal_handlers_unblock_by_func (palette_editor->columns_adj,
                                     palette_editor_columns_changed,
                                     editor);
}

static void
clay_palette_editor_set_context (ClayDocked  *docked,
                                 ClayContext *context)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  clay_view_renderer_set_context (CLAY_VIEW (editor->view)->renderer,
                                  context);
}

#define AUX_INFO_ZOOM_FACTOR "zoom-factor"

static void
clay_palette_editor_set_aux_info (ClayDocked *docked,
                                  GList      *aux_info)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (docked);
  GList             *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_ZOOM_FACTOR))
        {
          gdouble zoom_factor;

          zoom_factor = g_ascii_strtod (aux->value, NULL);

          editor->zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);
        }
    }
}

static GList *
clay_palette_editor_get_aux_info (ClayDocked *docked)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (docked);
  GList             *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (editor->zoom_factor != 1.0)
    {
      ClaySessionInfoAux *aux;
      gchar               value[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (value, sizeof (value), "%.2f", editor->zoom_factor);

      aux = clay_session_info_aux_new (AUX_INFO_ZOOM_FACTOR, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}


/*  public functions  */

GtkWidget *
clay_palette_editor_new (ClayContext     *context,
                         ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_PALETTE_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<PaletteEditor>",
                       "ui-path",         "/palette-editor-popup",
                       "data-factory",    context->clay->palette_factory,
                       "context",         context,
                       "data",            clay_context_get_palette (context),
                       NULL);
}

void
clay_palette_editor_edit_color (ClayPaletteEditor *editor)
{
  ClayDataEditor *data_editor;
  ClayPalette    *palette;

  g_return_if_fail (CLAY_IS_PALETTE_EDITOR (editor));

  data_editor = CLAY_DATA_EDITOR (editor);

  if (! (data_editor->data_editable && editor->color))
    return;

  palette = CLAY_PALETTE (clay_data_editor_get_data (data_editor));

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        clay_color_dialog_new (CLAY_VIEWABLE (palette),
                               data_editor->context,
                               FALSE,
                               _("Edit Palette Color"),
                               CLAY_ICON_PALETTE,
                               _("Edit Color Palette Entry"),
                               GTK_WIDGET (editor),
                               clay_dialog_factory_get_singleton (),
                               "clay-palette-editor-color-dialog",
                               editor->color->color,
                               FALSE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (palette_editor_edit_color_update),
                        editor);
    }
  else
    {
      clay_viewable_dialog_set_viewables (CLAY_VIEWABLE_DIALOG (editor->color_dialog),
                                          g_list_prepend (NULL, palette),
                                          data_editor->context);
      clay_color_dialog_set_color (CLAY_COLOR_DIALOG (editor->color_dialog),
                                   editor->color->color);

      if (! gtk_widget_get_visible (editor->color_dialog))
        clay_dialog_factory_position_dialog (clay_dialog_factory_get_singleton (),
                                             "clay-palette-editor-color-dialog",
                                             editor->color_dialog,
                                             clay_widget_get_monitor (GTK_WIDGET (editor)));
    }

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
clay_palette_editor_pick_color (ClayPaletteEditor  *editor,
                                GeglColor          *color,
                                ClayColorPickState  pick_state)
{
  g_return_if_fail (CLAY_IS_PALETTE_EDITOR (editor));
  g_return_if_fail (GEGL_IS_COLOR (color));

  if (CLAY_DATA_EDITOR (editor)->data_editable)
    {
      ClayPaletteEntry *entry;
      ClayData         *data;
      gint              index = -1;

      data = clay_data_editor_get_data (CLAY_DATA_EDITOR (editor));
      if (editor->color)
        index = clay_palette_get_entry_position (CLAY_PALETTE (data),
                                                 editor->color);

      switch (pick_state)
        {
        case CLAY_COLOR_PICK_STATE_START:
          if (editor->color)
            index += 1;

          entry = clay_palette_add_entry (CLAY_PALETTE (data), index,
                                          NULL, color);
          clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view),
                                          entry);
          break;

        case CLAY_COLOR_PICK_STATE_UPDATE:
        case CLAY_COLOR_PICK_STATE_END:
          clay_palette_set_entry_color (CLAY_PALETTE (data),
                                        index, color, FALSE);
          break;
        }
    }
}

void
clay_palette_editor_zoom (ClayPaletteEditor  *editor,
                          ClayZoomType        zoom_type)
{
  ClayPalette *palette;
  gdouble      zoom_factor;

  g_return_if_fail (CLAY_IS_PALETTE_EDITOR (editor));

  palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  zoom_factor = editor->zoom_factor;

  switch (zoom_type)
    {
    case CLAY_ZOOM_IN_MAX:
    case CLAY_ZOOM_IN_MORE:
    case CLAY_ZOOM_IN:
      zoom_factor += 0.1;
      break;

    case CLAY_ZOOM_OUT_MORE:
    case CLAY_ZOOM_OUT:
      zoom_factor -= 0.1;
      break;

    case CLAY_ZOOM_OUT_MAX:
    case CLAY_ZOOM_TO: /* abused as ZOOM_ALL */
      {
        GtkWidget     *scrolled_win = CLAY_DATA_EDITOR (editor)->view;
        GtkWidget     *viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));
        GtkAllocation  allocation;
        gint           columns;
        gint           rows;

        gtk_widget_get_allocation (viewport, &allocation);

        columns = clay_palette_get_columns (palette);
        if (columns == 0)
          columns = COLUMNS;

        rows = clay_palette_get_n_colors (palette) / columns;
        if (clay_palette_get_n_colors (palette) % columns)
          rows += 1;

        rows = MAX (1, rows);

        zoom_factor = (((gdouble) allocation.height - 2 * SPACING) /
                       (gdouble) rows - SPACING) / ENTRY_HEIGHT;
      }
      break;

    case CLAY_ZOOM_SMOOTH:
    case CLAY_ZOOM_PINCH: /* can't happen */
      g_return_if_reached ();
    }

  zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);

  editor->columns = clay_palette_get_columns (palette);
  if (editor->columns == 0)
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, zoom_factor);

  palette_editor_scroll_top_left (editor);
}

gint
clay_palette_editor_get_index (ClayPaletteEditor *editor,
                               GeglColor         *search)
{
  ClayPalette *palette;

  g_return_val_if_fail (CLAY_IS_PALETTE_EDITOR (editor), -1);
  g_return_val_if_fail (GEGL_IS_COLOR (search), -1);

  palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  if (palette && clay_palette_get_n_colors (palette) > 0)
    {
      ClayPaletteEntry *entry;

      entry = clay_palette_find_entry (palette, search, editor->color);

      if (entry)
        return clay_palette_get_entry_position (palette, entry);
    }

  return -1;
}

gboolean
clay_palette_editor_set_index (ClayPaletteEditor *editor,
                               gint               index,
                               GeglColor         *color)
{
  ClayPalette *palette;

  g_return_val_if_fail (CLAY_IS_PALETTE_EDITOR (editor), FALSE);

  palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  if (palette && clay_palette_get_n_colors (palette) > 0)
    {
      ClayPaletteEntry *entry;

      index = CLAMP (index, 0, clay_palette_get_n_colors (palette) - 1);

      entry = clay_palette_get_entry (palette, index);

      clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view),
                                      entry);

      if (color)
        {
          const Babl *format;
          guchar      pixel[40];

          format = gegl_color_get_format (editor->color->color);
          gegl_color_get_pixel (editor->color->color, format, pixel);
          gegl_color_set_pixel (color, format, pixel);
        }

      return TRUE;
    }

  return FALSE;
}

gint
clay_palette_editor_max_index (ClayPaletteEditor *editor)
{
  ClayPalette *palette;

  g_return_val_if_fail (CLAY_IS_PALETTE_EDITOR (editor), -1);

  palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  if (palette && clay_palette_get_n_colors (palette) > 0)
    {
      return clay_palette_get_n_colors (palette) - 1;
    }

  return -1;
}


/*  private functions  */

static void
palette_editor_invalidate_preview (ClayPalette       *palette,
                                   ClayPaletteEditor *editor)
{
  editor->columns = clay_palette_get_columns (palette);
  if (editor->columns == 0)
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, editor->zoom_factor);
}

static void
palette_editor_viewport_size_allocate (GtkWidget         *widget,
                                       GtkAllocation     *allocation,
                                       ClayPaletteEditor *editor)
{
  if (allocation->width != editor->last_width)
    {
      palette_editor_resize (editor, allocation->width,
                             editor->zoom_factor);
    }
}

static void
palette_editor_drop_palette (GtkWidget    *widget,
                             gint          x,
                             gint          y,
                             ClayViewable *viewable,
                             gpointer      data)
{
  clay_data_editor_set_data (CLAY_DATA_EDITOR (data), CLAY_DATA (viewable));
}

static void
palette_editor_drop_color (GtkWidget *widget,
                           gint       x,
                           gint       y,
                           GeglColor *color,
                           gpointer   data)
{
  ClayPaletteEditor *editor = data;

  if (CLAY_DATA_EDITOR (editor)->data_editable)
    {
      ClayPalette      *palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);
      ClayPaletteEntry *entry;

      entry = clay_palette_add_entry (palette, -1, NULL, color);
      clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view), entry);
    }
}


/*  palette view callbacks  */

static void
palette_editor_entry_clicked (ClayPaletteView   *view,
                              ClayPaletteEntry  *entry,
                              GdkModifierType    state,
                              ClayPaletteEditor *editor)
{
  if (entry)
    {
      ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);

      if (state & clay_get_toggle_behavior_mask ())
        clay_context_set_background (data_editor->context, entry->color);
      else
        clay_context_set_foreground (data_editor->context, entry->color);
    }
}

static void
palette_editor_entry_selected (ClayPaletteView   *view,
                               ClayPaletteEntry  *entry,
                               ClayPaletteEditor *editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (editor);

  if (editor->color != entry)
    {
      gchar index[8];

      editor->color = entry;

      if (entry)
        {
          ClayPalette *palette = CLAY_PALETTE (data_editor->data);
          gint         pos;

          pos = clay_palette_get_entry_position (palette, entry);
          g_snprintf (index, sizeof (index), "%04i", pos);
        }
      else
        {
          g_snprintf (index, sizeof (index), "####");
        }

      gtk_label_set_text (GTK_LABEL (editor->index_label), index);

      g_signal_handlers_block_by_func (editor->color_name,
                                       palette_editor_color_name_changed,
                                       editor);

      gtk_entry_set_text (GTK_ENTRY (editor->color_name),
                          entry ? entry->name : _("Undefined"));

      g_signal_handlers_unblock_by_func (editor->color_name,
                                         palette_editor_color_name_changed,
                                         editor);

      gtk_editable_set_editable (GTK_EDITABLE (editor->color_name),
                                 entry && data_editor->data_editable);

      clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                              clay_editor_get_popup_data (CLAY_EDITOR (editor)));
    }
}

static void
palette_editor_entry_activated (ClayPaletteView   *view,
                                ClayPaletteEntry  *entry,
                                ClayPaletteEditor *editor)
{
  if (CLAY_DATA_EDITOR (editor)->data_editable && entry == editor->color)
    {
      clay_ui_manager_activate_action (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                                       "palette-editor",
                                       "palette-editor-edit-color");
    }
}

static gboolean
palette_editor_button_press_event (GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   user_data)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (user_data);

  if (gdk_event_triggers_context_menu (event))
    {
      clay_editor_popup_menu_at_pointer (CLAY_EDITOR (editor), event);
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
palette_editor_popup_menu (GtkWidget *widget,
                           gpointer   user_data)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (user_data);
  ClayPaletteEntry *selected;
  GdkRectangle rect;

  selected = clay_palette_view_get_selected_entry (CLAY_PALETTE_VIEW (editor->view));
  if (!selected)
    return GDK_EVENT_PROPAGATE;

  clay_palette_view_get_entry_rect (CLAY_PALETTE_VIEW (editor->view), selected, &rect);
  return clay_editor_popup_menu_at_rect (CLAY_EDITOR (editor),
                                         gtk_widget_get_window (GTK_WIDGET (editor->view)),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

static void
palette_editor_color_dropped (ClayPaletteView   *view,
                              ClayPaletteEntry  *entry,
                              GeglColor         *color,
                              ClayPaletteEditor *editor)
{
  if (CLAY_DATA_EDITOR (editor)->data_editable)
    {
      ClayPalette *palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);
      gint         pos     = -1;

      if (entry)
        pos = clay_palette_get_entry_position (palette, entry);

      entry = clay_palette_add_entry (palette, pos, NULL, color);
      clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view), entry);
    }
}


/*  color name and columns callbacks  */

static void
palette_editor_color_name_changed (GtkWidget         *widget,
                                   ClayPaletteEditor *editor)
{
  if (CLAY_DATA_EDITOR (editor)->data)
    {
      ClayPalette *palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);
      const gchar *name;
      gint         pos;

      name = gtk_entry_get_text (GTK_ENTRY (editor->color_name));

      pos = clay_palette_get_entry_position (palette, editor->color);
      clay_palette_set_entry_name (palette, pos, name);
    }
}

static void
palette_editor_columns_changed (GtkAdjustment     *adj,
                                ClayPaletteEditor *editor)
{
  if (CLAY_DATA_EDITOR (editor)->data)
    {
      ClayPalette *palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

      clay_palette_set_columns (palette,
                                ROUND (gtk_adjustment_get_value (adj)));
    }
}


/*  misc utils  */

static void
palette_editor_resize (ClayPaletteEditor *editor,
                       gint               width,
                       gdouble            zoom_factor)
{
  ClayPalette *palette;
  gint         rows;
  gint         preview_width;
  gint         preview_height;

  palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  width = MIN (width, CLAY_VIEWABLE_MAX_PREVIEW_SIZE);

  editor->zoom_factor = zoom_factor;
  editor->last_width  = width;
  editor->col_width   = width / (editor->columns + 1) - SPACING;

  if (editor->col_width < 0)
    editor->col_width = 0;

  rows = clay_palette_get_n_colors (palette) / editor->columns;
  if (clay_palette_get_n_colors (palette) % editor->columns)
    rows += 1;

  preview_width  = (editor->col_width + SPACING) * editor->columns;
  preview_height = (rows *
                    (SPACING + (gint) (ENTRY_HEIGHT * editor->zoom_factor)));

  if (preview_height > CLAY_VIEWABLE_MAX_PREVIEW_SIZE)
    preview_height = ((CLAY_VIEWABLE_MAX_PREVIEW_SIZE - SPACING) / rows) * rows;

  clay_view_renderer_set_size_full (CLAY_VIEW (editor->view)->renderer,
                                    preview_width  + SPACING,
                                    preview_height + SPACING, 0);
}

static void
palette_editor_scroll_top_left (ClayPaletteEditor *palette_editor)
{
  ClayDataEditor *data_editor = CLAY_DATA_EDITOR (palette_editor);
  GtkAdjustment  *hadj;
  GtkAdjustment  *vadj;

  if (! data_editor->view)
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (data_editor->view));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (data_editor->view));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_editor_edit_color_update (ClayColorDialog      *dialog,
                                  GeglColor            *color,
                                  ClayColorDialogState  state,
                                  ClayPaletteEditor    *editor)
{
  ClayPalette *palette = CLAY_PALETTE (CLAY_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case CLAY_COLOR_DIALOG_UPDATE:
      break;

    case CLAY_COLOR_DIALOG_OK:
      if (editor->color)
        {
          g_clear_object (&editor->color->color);
          editor->color->color = gegl_color_duplicate (color);
          clay_data_dirty (CLAY_DATA (palette));
        }
      /* Fallthrough */

    case CLAY_COLOR_DIALOG_CANCEL:
      gtk_widget_set_visible (editor->color_dialog, FALSE);
      break;
    }
}
