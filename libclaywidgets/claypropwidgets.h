/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypropwidgets.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAYWIDGETS_PROP_WIDGETS_H__
#define __CLAYWIDGETS_PROP_WIDGETS_H__

G_BEGIN_DECLS


/*  GParamBoolean  */

GtkWidget     * clay_prop_check_button_new        (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *label);
GtkWidget     * clay_prop_switch_new              (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *label,
                                                   GtkWidget   **label_out,
                                                   GtkWidget   **switch_out);
GtkWidget *     clay_prop_toggle_new              (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *icon_name,
                                                   const gchar  *label,
                                                   GtkWidget   **image_out);
GtkWidget     * clay_prop_boolean_combo_box_new   (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *true_text,
                                                   const gchar  *false_text);
GtkWidget     * clay_prop_boolean_radio_frame_new (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *title,
                                                   const gchar  *true_text,
                                                   const gchar  *false_text);

GtkWidget     * clay_prop_expander_new            (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *label);


/*  GParamInt  */

GtkWidget     * clay_prop_int_combo_box_new       (GObject      *config,
                                                   const gchar  *property_name,
                                                   ClayIntStore *store);
GtkWidget     * clay_prop_int_radio_frame_new     (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *title,
                                                   ClayIntStore *store);

/*  GParamGType  */

GtkWidget     * clay_prop_pointer_combo_box_new   (GObject      *config,
                                                   const gchar  *property_name,
                                                   ClayIntStore *store);

/*  GParamEnum  */

GtkWidget     * clay_prop_enum_combo_box_new      (GObject      *config,
                                                   const gchar  *property_name,
                                                   gint          minimum,
                                                   gint          maximum);

GtkWidget     * clay_prop_enum_check_button_new   (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *label,
                                                   gint          false_value,
                                                   gint          true_value);

GtkWidget     * clay_prop_enum_radio_frame_new    (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *title,
                                                   gint          minimum,
                                                   gint          maximum);
GtkWidget     * clay_prop_enum_radio_box_new      (GObject      *config,
                                                   const gchar  *property_name,
                                                   gint          minimum,
                                                   gint          maximum);

GtkWidget     * clay_prop_enum_icon_box_new       (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *icon_prefix,
                                                   gint          minimum,
                                                   gint          maximum);

GtkWidget     * clay_prop_enum_label_new          (GObject      *config,
                                                   const gchar  *property_name);


/*  GParamInt, GParamUInt, GParamLong, GParamULong, GParamDouble  */

GtkWidget     * clay_prop_spin_button_new         (GObject      *config,
                                                   const gchar  *property_name,
                                                   gdouble       step_increment,
                                                   gdouble       page_increment,
                                                   gint          digits);

GtkWidget     * clay_prop_label_spin_new          (GObject      *config,
                                                   const gchar  *property_name,
                                                   gint          digits);

GtkWidget     * clay_prop_spin_scale_new          (GObject      *config,
                                                   const gchar  *property_name,
                                                   gdouble       step_increment,
                                                   gdouble       page_increment,
                                                   gint          digits);

void            clay_prop_widget_set_factor       (GtkWidget    *widget,
                                                   gdouble       factor,
                                                   gdouble       step_increment,
                                                   gdouble       page_increment,
                                                   gint          digits);

GtkWidget     * clay_prop_hscale_new              (GObject      *config,
                                                   const gchar  *property_name,
                                                   gdouble       step_increment,
                                                   gdouble       page_increment,
                                                   gint          digits);

GtkWidget     * clay_prop_scale_entry_new         (GObject      *config,
                                                   const gchar  *property_name,
                                                   const gchar  *label,
                                                   gdouble       factor,
                                                   gboolean      limit_scale,
                                                   gdouble       lower_limit,
                                                   gdouble       upper_limit);


/*  ClayParamMemsize  */

GtkWidget     * clay_prop_memsize_entry_new       (GObject       *config,
                                                   const gchar   *property_name);


/*  GParamString  */

GtkWidget     * clay_prop_label_new               (GObject       *config,
                                                   const gchar   *property_name);
GtkWidget     * clay_prop_entry_new               (GObject       *config,
                                                   const gchar   *property_name,
                                                   gint           max_len);
GtkWidget     * clay_prop_label_entry_new         (GObject       *config,
                                                   const gchar   *property_name,
                                                   gint           max_len);
GtkTextBuffer * clay_prop_text_buffer_new         (GObject       *config,
                                                   const gchar   *property_name,
                                                   gint           max_len);
GtkWidget     * clay_prop_string_combo_box_new    (GObject       *config,
                                                   const gchar   *property_name,
                                                   GtkTreeModel  *model,
                                                   gint           id_column,
                                                   gint           label_column);
GtkWidget *     clay_prop_choice_combo_box_new    (GObject       *config,
                                                   const gchar   *property_name);
GtkWidget *     clay_prop_choice_radio_frame_new  (GObject       *config,
                                                   const gchar   *property_name);


/*  ClayParamPath  */

GtkWidget     * clay_prop_file_chooser_new        (GObject               *config,
                                                   const gchar           *property_name,
                                                   const gchar           *label,
                                                   const gchar           *title);

GtkWidget     * clay_prop_file_chooser_button_new (GObject              *config,
                                                   const gchar          *property_name,
                                                   const gchar          *title,
                                                   GtkFileChooserAction  action);
GtkWidget     * clay_prop_file_chooser_button_new_with_dialog (GObject     *config,
                                                               const gchar *property_name,

                                                               GtkWidget   *dialog);
GtkWidget     * clay_prop_path_editor_new         (GObject       *config,
                                                   const gchar   *path_property_name,
                                                   const gchar   *writable_property_name,
                                                   const gchar   *filechooser_title);


/*  GParamInt, GParamUInt, GParamDouble   unit: ClayParamUnit  */

GtkWidget     * clay_prop_size_entry_new          (GObject       *config,
                                                   const gchar   *property_name,
                                                   gboolean       property_is_pixel,
                                                   const gchar   *unit_property_name,
                                                   const gchar   *unit_format,
                                                   ClaySizeEntryUpdatePolicy  update_policy,
                                                   gdouble        resolution);


/*  x,y: GParamInt, GParamDouble   unit: ClayParamUnit  */

GtkWidget     * clay_prop_coordinates_new         (GObject       *config,
                                                   const gchar   *x_property_name,
                                                   const gchar   *y_property_name,
                                                   const gchar   *unit_property_name,
                                                   const gchar   *unit_format,
                                                   ClaySizeEntryUpdatePolicy  update_policy,
                                                   gdouble        xresolution,
                                                   gdouble        yresolution,
                                                   gboolean       has_chainbutton);
gboolean        clay_prop_coordinates_connect     (GObject       *config,
                                                   const gchar   *x_property_name,
                                                   const gchar   *y_property_name,
                                                   const gchar   *unit_property_name,
                                                   GtkWidget     *sizeentry,
                                                   GtkWidget     *chainbutton,
                                                   gdouble        xresolution,
                                                   gdouble        yresolution);


/*  ClayParamColor  */

GtkWidget     * clay_prop_color_area_new          (GObject       *config,
                                                   const gchar   *property_name,
                                                   gint           width,
                                                   gint           height,
                                                   ClayColorAreaType  type);

GtkWidget     * clay_prop_color_select_new        (GObject       *config,
                                                   const gchar   *property_name,
                                                   gint           width,
                                                   gint           height,
                                                   ClayColorAreaType  type);

GtkWidget     * clay_prop_label_color_new         (GObject       *config,
                                                   const gchar   *property_name,
                                                   gboolean       editable);


/*  ClayParamUnit  */

GtkWidget     * clay_prop_unit_combo_box_new      (GObject       *config,
                                                   const gchar   *property_name);


/*  GParamString (icon name)  */

GtkWidget     * clay_prop_icon_image_new          (GObject       *config,
                                                   const gchar   *property_name,
                                                   GtkIconSize    icon_size);


G_END_DECLS

#endif /* __CLAYWIDGETS_PROP_WIDGETS_H__ */
