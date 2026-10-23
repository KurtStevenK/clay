/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Clay Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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


#include <config.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ani.h"
#include "ico-dialog.h"
#include "ico-export.h"

#include "libclay/stdplugins-intl.h"

static void   ico_dialog_bpp_changed     (GtkWidget   *combo,
                                          GObject     *hbox);
static void   ico_dialog_toggle_compress (GtkWidget   *checkbox,
                                          GObject     *hbox);
static void   ico_dialog_check_compat    (GtkWidget   *dialog,
                                          IcoSaveInfo *info);
static void   ico_dialog_ani_update_inam (GtkEntry    *entry,
                                          gpointer     data);
static void   ico_dialog_ani_update_iart (GtkEntry    *entry,
                                          gpointer     data);


GtkWidget *
ico_dialog_new (ClayImage           *image,
                ClayProcedure       *procedure,
                ClayProcedureConfig *config,
                IcoSaveInfo         *info,
                AniFileHeader       *ani_header,
                AniSaveInfo         *ani_info)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *scrolled_window;
  GtkWidget     *viewport;
  GtkWidget     *warning;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             config, image);

  /* We store an array that holds each icon's requested bit depth
     with the dialog. It's queried when the dialog is closed so the
     save routine knows what colormaps etc to generate in the saved
     file. We store twice the number necessary because in the second
     set, the color depths that are automatically suggested are stored
     for later comparison.
  */

  g_object_set_data (G_OBJECT (dialog), "save_info", info);
  if (ani_header)
    {
      g_object_set_data (G_OBJECT (dialog), "save_ani_header", ani_header);
      g_object_set_data (G_OBJECT (dialog), "save_ani_info", ani_info);
    }

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);

  /*Animated Cursor */
  if (ani_header)
    {
      GtkWidget     *grid;
      GtkAdjustment *adjustment;
      GtkWidget     *spin;
      GtkWidget     *label;
      GtkWidget     *hbox;
      GtkWidget     *entry;

      frame = clay_frame_new (_("Animated Cursor Settings"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_set_visible (grid, TRUE);

      /* Cursor Name */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("_Cursor Name (Optional)"),
                                0.0, 0.5,
                                hbox, 1);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry),
                          ani_info->inam ? ani_info->inam : "");
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_widget_set_visible (entry, TRUE);

      g_signal_connect (GTK_ENTRY (entry), "focus-out-event",
                        G_CALLBACK (ico_dialog_ani_update_inam),
                        NULL);

      /* Author Name */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                                _("_Author Name (Optional)"),
                                0.0, 0.5,
                                hbox, 1);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry),
                          ani_info->iart ? ani_info->iart : "");
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_widget_set_visible (entry, TRUE);

      g_signal_connect (GTK_ENTRY (entry), "focus-out-event",
                        G_CALLBACK (ico_dialog_ani_update_iart),
                        NULL);

      /* Default delay spin */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                                _("_Delay between frames:"),
                                0.0, 0.5,
                                hbox, 1);

      adjustment = gtk_adjustment_new (ani_header->jif_rate, 1, G_MAXINT,
                                       1, 10, 0);
      spin = clay_spin_button_new (adjustment, 1, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
      gtk_widget_set_visible (spin, TRUE);

      label = gtk_label_new (_(" jiffies (16.66 ms)"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (clay_int_adjustment_update),
                        &ani_header->jif_rate);
    }

  /* Cursor */
  frame = clay_frame_new (_("Icon Details"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
  gtk_widget_set_visible (frame, TRUE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_set_visible (scrolled_window, TRUE);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_set_visible (viewport, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  g_object_set_data (G_OBJECT (dialog), "icons_vbox", vbox);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  warning = g_object_new (CLAY_TYPE_HINT_BOX,
                          "icon-name", CLAY_ICON_DIALOG_WARNING,
                          "hint",
                          _("Large icons and compression are not supported "
                            "by all programs. Older applications may not "
                            "open this file correctly."),
                          NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), warning, FALSE, FALSE, 12);
  /* don't show the warning here */

  g_object_set_data (G_OBJECT (dialog), "warning", warning);

  return dialog;
}

static GtkWidget *
ico_preview_new (ClayDrawable *layer)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gint       width  = clay_drawable_get_width (layer);
  gint       height = clay_drawable_get_height (layer);

  pixbuf = clay_drawable_get_thumbnail (layer,
                                        MIN (width, 128), MIN (height, 128),
                                        CLAY_PIXBUF_SMALL_CHECKS);
  image = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);

  return image;
}

/* This function creates and returns an hbox for an icon,
   which then gets added to the dialog's main vbox. */
static GtkWidget *
ico_create_icon_hbox (GtkWidget    *icon_preview,
                      ClayDrawable *layer,
                      gint          layer_num,
                      IcoSaveInfo  *info)
{
  static GtkSizeGroup *size = NULL;

  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *combo;
  GtkWidget *checkbox;
  gchar     *frame_header;
  gint       width  = clay_drawable_get_width (layer);
  gint       height = clay_drawable_get_height (layer);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  frame_header = g_strdup_printf ("%dx%d", width, height);

  frame = clay_frame_new (frame_header);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);
  g_free (frame_header);

  /* To make life easier for the callbacks, we store the
     layer's ID and stacking number with the hbox. */

  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer", layer);
  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer_num", GINT_TO_POINTER (layer_num));

  g_object_set_data (G_OBJECT (hbox), "icon_preview", icon_preview);
  gtk_widget_set_halign (icon_preview, GTK_ALIGN_END);
  gtk_widget_set_valign (icon_preview, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (frame), icon_preview);
  gtk_widget_set_visible (icon_preview, TRUE);

  if (! size)
    size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_size_group_add_widget (size, icon_preview);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  combo = clay_int_combo_box_new (_("1 bpp, 1-bit alpha, 2-slot palette"),   1,
                                  _("4 bpp, 1-bit alpha, 16-slot palette"),  4,
                                  _("8 bpp, 1-bit alpha, 256-slot palette"), 8,
                                  _("24 bpp, 1-bit alpha, no palette"),     24,
                                  _("32 bpp, 8-bit alpha, no palette"),     32,
                                  NULL);
  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo),
                                 info->depths[layer_num]);
  gtk_widget_set_margin_top (GTK_WIDGET (combo), 6);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ico_dialog_bpp_changed),
                    hbox);

  g_object_set_data (G_OBJECT (hbox), "icon_menu", combo);

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (combo, TRUE);

  checkbox = gtk_check_button_new_with_label (_("Compressed (PNG)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox),
                                info->compress[layer_num]);
  g_signal_connect (checkbox, "toggled",
                    G_CALLBACK (ico_dialog_toggle_compress), hbox);
  gtk_box_pack_start (GTK_BOX (vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (checkbox, TRUE);

  return hbox;
}

static GtkWidget *
ico_dialog_get_layer_preview (GtkWidget    *dialog,
                              ClayDrawable *layer)
{
  GtkWidget *preview;
  GtkWidget *icon_hbox;
  gchar      key[ICO_MAXBUF];

  g_snprintf (key, sizeof (key), "layer_%i_hbox",
              clay_item_get_id (CLAY_ITEM (layer)));
  icon_hbox = g_object_get_data (G_OBJECT (dialog), key);

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up hbox by layer ID\n"));
      return NULL;
    }

  preview = g_object_get_data (G_OBJECT (icon_hbox), "icon_preview");

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up preview from hbox\n"));
      return NULL;
    }

  return preview;
}

static void
ico_dialog_update_icon_preview (GtkWidget    *dialog,
                                ClayDrawable *layer,
                                gint          bpp)
{
  GtkWidget  *preview = ico_dialog_get_layer_preview (dialog, layer);
  GdkPixbuf  *pixbuf;
  const Babl *format;
  gint        w       = clay_drawable_get_width (layer);
  gint        h       = clay_drawable_get_height (layer);

  if (! preview)
    return;

  switch (clay_drawable_type (layer))
    {
    case CLAY_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case CLAY_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case CLAY_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case CLAY_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case CLAY_INDEXED_IMAGE:
    case CLAY_INDEXEDA_IMAGE:
      format = clay_drawable_get_format (layer);
      break;

    default:
      g_return_if_reached ();
    }

  if (bpp <= 8)
    {
      GeglBuffer  *buffer;
      GeglBuffer  *tmp;
      ClayImage   *image;
      ClayImage   *tmp_image;
      ClayLayer   *tmp_layer;
      guchar      *buf;
      ClayPalette *palette;

      image = clay_item_get_image (CLAY_ITEM (layer));

      tmp_image = clay_image_new (w, h, clay_image_get_base_type (image));
      clay_image_undo_disable (tmp_image);

      if (clay_drawable_is_indexed (layer))
        clay_image_set_palette (tmp_image, clay_image_get_palette (image));

      tmp_layer = clay_layer_new (tmp_image, "temporary", w, h,
                                  clay_drawable_type (layer),
                                  100,
                                  clay_image_get_default_new_layer_mode (tmp_image));
      clay_image_insert_layer (tmp_image, tmp_layer, NULL, 0);

      buffer = clay_drawable_get_buffer (layer);
      tmp    = clay_drawable_get_buffer (CLAY_DRAWABLE (tmp_layer));

      buf = g_malloc (w * h * 4);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, tmp, NULL);

      g_object_unref (tmp);
      g_object_unref (buffer);

      if (clay_drawable_is_indexed (layer))
        clay_image_convert_rgb (tmp_image);

      clay_image_convert_indexed (tmp_image,
                                  CLAY_CONVERT_DITHER_FS,
                                  CLAY_CONVERT_PALETTE_GENERATE,
                                  1 << bpp, TRUE, FALSE, "dummy");

      palette = clay_image_get_palette (tmp_image);

      if (clay_palette_get_color_count (palette) == (1 << bpp) &&
          ! ico_cmap_contains_black (palette))
        {
          /* Windows icons with color maps need the color black.
           * We need to eliminate one more color to make room for black.
           */
          if (clay_drawable_is_indexed (layer))
            {
              clay_image_set_palette (tmp_image, clay_image_get_palette (image));
            }
          else if (clay_drawable_is_gray (layer))
            {
              clay_image_convert_grayscale (tmp_image);
            }
          else
            {
              clay_image_convert_rgb (tmp_image);
            }

          tmp = clay_drawable_get_buffer (CLAY_DRAWABLE (tmp_layer));

          gegl_buffer_set (tmp, GEGL_RECTANGLE (0, 0, w, h), 0,
                           format, buf, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (tmp);

          if (!clay_drawable_is_rgb (layer))
            clay_image_convert_rgb (tmp_image);

          clay_image_convert_indexed (tmp_image,
                                      CLAY_CONVERT_DITHER_FS,
                                      CLAY_CONVERT_PALETTE_GENERATE,
                                      (1 << bpp) - 1, TRUE, FALSE, "dummy");
        }

      g_free (buf);

      pixbuf = clay_drawable_get_thumbnail (CLAY_DRAWABLE (tmp_layer),
                                            MIN (w, 128), MIN (h, 128),
                                            CLAY_PIXBUF_SMALL_CHECKS);

      clay_image_delete (tmp_image);
    }
  else if (bpp == 24)
    {
      GeglBuffer     *buffer;
      GeglBuffer     *tmp;
      ClayImage      *image;
      ClayImage      *tmp_image;
      ClayLayer      *tmp_layer;

      image = clay_item_get_image (CLAY_ITEM (layer));

      tmp_image = clay_image_new (w, h, clay_image_get_base_type (image));
      clay_image_undo_disable (tmp_image);

      if (clay_drawable_is_indexed (layer))
        clay_image_set_palette (tmp_image, clay_image_get_palette (image));

      tmp_layer = clay_layer_new (tmp_image, "temporary", w, h,
                                  clay_drawable_type (layer),
                                  100,
                                  clay_image_get_default_new_layer_mode (tmp_image));
      clay_image_insert_layer (tmp_image, tmp_layer, NULL, 0);

      buffer = clay_drawable_get_buffer (layer);
      tmp    = clay_drawable_get_buffer (CLAY_DRAWABLE (tmp_layer));

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, tmp, NULL);

      g_object_unref (tmp);
      g_object_unref (buffer);

      if (clay_drawable_is_indexed (layer))
        clay_image_convert_rgb (tmp_image);

      clay_drawable_merge_new_filter (CLAY_DRAWABLE (tmp_layer),
                                      "clay:threshold-alpha", NULL,
                                      CLAY_LAYER_MODE_REPLACE, 1.0,
                                      "value", ICO_ALPHA_THRESHOLD / 255.0,
                                      NULL);

      pixbuf = clay_drawable_get_thumbnail (CLAY_DRAWABLE (tmp_layer),
                                            MIN (w, 128), MIN (h, 128),
                                            CLAY_PIXBUF_SMALL_CHECKS);

      clay_image_delete (tmp_image);
    }
  else
    {
      pixbuf = clay_drawable_get_thumbnail (layer,
                                            MIN (w, 128), MIN (h, 128),
                                            CLAY_PIXBUF_SMALL_CHECKS);
    }

  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
  g_object_unref (pixbuf);
}

void
ico_dialog_add_icon (GtkWidget    *dialog,
                     ClayDrawable *layer,
                     gint          layer_num)
{
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *preview;
  gchar        key[ICO_MAXBUF];
  IcoSaveInfo *info;

  vbox = g_object_get_data (G_OBJECT (dialog), "icons_vbox");
  info = g_object_get_data (G_OBJECT (dialog), "save_info");

  preview = ico_preview_new (layer);
  hbox = ico_create_icon_hbox (preview, layer, layer_num, info);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /* Let's make the hbox accessible through the layer ID */
  g_snprintf (key, sizeof (key), "layer_%i_hbox",
              clay_item_get_id (CLAY_ITEM (layer)));
  g_object_set_data (G_OBJECT (dialog), key, hbox);

  ico_dialog_update_icon_preview (dialog, layer, info->depths[layer_num]);

  ico_dialog_check_compat (dialog, info);

  /* Cursor */
  if (info->is_cursor)
    {
      GtkWidget     *grid;
      GtkAdjustment *adj;
      GtkWidget     *spinbutton;

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, FALSE, 0);
      gtk_widget_set_visible (grid, TRUE);

      adj = (GtkAdjustment *)
             gtk_adjustment_new (info->hot_spot_x[layer_num], 0,
                                 G_MAXUINT16, 1, 10, 0);
      spinbutton = clay_spin_button_new (adj, 1.0, 0);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton),
                                 0, G_MAXUINT16);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("Hot spot _X:"), 0.0, 0.5,
                                spinbutton, 1);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (clay_int_adjustment_update),
                        &info->hot_spot_x[layer_num]);

      adj = (GtkAdjustment *)
             gtk_adjustment_new (info->hot_spot_y[layer_num], 0,
                                 G_MAXUINT16, 1, 10, 0);
      spinbutton = clay_spin_button_new (adj, 1.0, 0);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton),
                                 0, G_MAXUINT16);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Hot spot _Y:"), 0.0, 0.5,
                                spinbutton, 1);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (clay_int_adjustment_update),
                        &info->hot_spot_y[layer_num]);
    }
}

static void
ico_dialog_bpp_changed (GtkWidget *combo,
                        GObject   *hbox)
{
  GtkWidget    *dialog;
  ClayDrawable *layer;
  gint          layer_num;
  gint          bpp;
  IcoSaveInfo  *info;

  dialog = gtk_widget_get_toplevel (combo);

  clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo), &bpp);

  info = g_object_get_data (G_OBJECT (dialog), "save_info");
  g_assert (info);

  layer     = g_object_get_data (hbox, "icon_layer");
  layer_num = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer_num"));

  /* Update vector entry for later when we're actually saving,
     and update the preview right away ... */
  info->depths[layer_num] = bpp;
  ico_dialog_update_icon_preview (dialog, layer, bpp);
}

static void
ico_dialog_toggle_compress (GtkWidget *checkbox,
                            GObject   *hbox)
{
  GtkWidget   *dialog;
  gint         layer_num;
  IcoSaveInfo *info;

  dialog = gtk_widget_get_toplevel (checkbox);

  info = g_object_get_data (G_OBJECT (dialog), "save_info");
  g_assert (info);
  layer_num = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer_num"));

  /* Update vector entry for later when we're actually saving */
  info->compress[layer_num] =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox));

  ico_dialog_check_compat (dialog, info);
}

static void
ico_dialog_check_compat (GtkWidget   *dialog,
                         IcoSaveInfo *info)
{
  GtkWidget *warning;
  GList     *iter;
  gboolean   warn = FALSE;
  gint       i;

  for (iter = info->layers, i = 0; iter; iter = iter->next, i++)
    {
      if (clay_drawable_get_width (iter->data) > 255  ||
          clay_drawable_get_height (iter->data) > 255 ||
          info->compress[i])
        {
          warn = TRUE;
          break;
        }
    }

  warning = g_object_get_data (G_OBJECT (dialog), "warning");

  gtk_widget_set_visible (warning, warn);
}

static void
ico_dialog_ani_update_inam (GtkEntry *entry,
                            gpointer  data)
{
  AniSaveInfo *ani_info;
  GtkWidget   *dialog;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET (entry));

  ani_info = g_object_get_data (G_OBJECT (dialog), "save_ani_info");
  ani_info->inam = g_strdup_printf ("%s", gtk_entry_get_text (entry));
}

static void
ico_dialog_ani_update_iart (GtkEntry *entry,
                            gpointer  data)
{
  AniSaveInfo *ani_info;
  GtkWidget   *dialog;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET (entry));

  ani_info = g_object_get_data (G_OBJECT (dialog), "save_ani_info");
  ani_info->iart = g_strdup_printf ("%s", gtk_entry_get_text (entry));
}
