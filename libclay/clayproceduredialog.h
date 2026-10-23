/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayproceduredialog.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_PROCEDURE_DIALOG_H__
#define __CLAY_PROCEDURE_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PROCEDURE_DIALOG (clay_procedure_dialog_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayProcedureDialog, clay_procedure_dialog, CLAY, PROCEDURE_DIALOG, ClayDialog)

struct _ClayProcedureDialogClass
{
  ClayDialogClass  parent_class;

  void             (* fill_start) (ClayProcedureDialog *dialog,
                                   ClayProcedure       *procedure,
                                   ClayProcedureConfig *config);
  void             (* fill_end)   (ClayProcedureDialog *dialog,
                                   ClayProcedure       *procedure,
                                   ClayProcedureConfig *config);
  void             (* fill_list)  (ClayProcedureDialog *dialog,
                                   ClayProcedure       *procedure,
                                   ClayProcedureConfig *config,
                                   GList               *properties);

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


GtkWidget * clay_procedure_dialog_new               (ClayProcedure       *procedure,
                                                     ClayProcedureConfig *config,
                                                     const gchar         *title);
void        clay_procedure_dialog_set_ok_label      (ClayProcedureDialog *dialog,
                                                     const gchar         *ok_label);
void        clay_procedure_dialog_set_warning       (ClayProcedureDialog *dialog,
                                                     const gchar         *title,
                                                     const gchar         *msg,
                                                     gboolean             use_markup);

GtkWidget * clay_procedure_dialog_get_widget        (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     GType                widget_type);
GtkWidget * clay_procedure_dialog_get_color_widget  (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gboolean             editable,
                                                     ClayColorAreaType    type);
GtkWidget * clay_procedure_dialog_get_coordinates   (ClayProcedureDialog *dialog,
                                                     const gchar         *coordinates_id,
                                                     const gchar         *x_property,
                                                     const gchar         *y_property,
                                                     const gchar         *unit_property,
                                                     const gchar         *unit_format,
                                                     ClaySizeEntryUpdatePolicy
                                                                          update_policy,
                                                     gdouble              x_resolution,
                                                     gdouble              y_resolution);
GtkWidget * clay_procedure_dialog_get_int_combo     (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     ClayIntStore        *store);
GtkWidget * clay_procedure_dialog_get_int_radio     (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     ClayIntStore        *store);
GtkWidget * clay_procedure_dialog_get_spin_scale    (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gdouble              factor);
GtkWidget * clay_procedure_dialog_get_scale_entry   (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gdouble              factor);
GtkWidget * clay_procedure_dialog_get_size_entry    (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gboolean             property_is_pixel,
                                                     const gchar         *unit_property,
                                                     const gchar         *unit_format,
                                                     ClaySizeEntryUpdatePolicy
                                                                          update_policy,
                                                     gdouble              resolution);
GtkWidget * clay_procedure_dialog_get_label         (ClayProcedureDialog *dialog,
                                                     const gchar         *label_id,
                                                     const gchar         *text,
                                                     gboolean             is_markup,
                                                     gboolean             with_mnemonic);

GtkWidget * clay_procedure_dialog_get_drawable_preview
                                                    (ClayProcedureDialog *dialog,
                                                     const gchar         *preview_id,
                                                     ClayDrawable        *drawable);

GtkWidget * clay_procedure_dialog_fill_box          (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *first_property,
                                                     ...) G_GNUC_NULL_TERMINATED;
GtkWidget * clay_procedure_dialog_fill_box_list     (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GList               *properties);
GtkWidget * clay_procedure_dialog_fill_flowbox      (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *first_property,
                                                     ...) G_GNUC_NULL_TERMINATED;
GtkWidget * clay_procedure_dialog_fill_flowbox_list (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GList               *properties);

GtkWidget * clay_procedure_dialog_fill_frame        (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *title_id,
                                                     gboolean             invert_title,
                                                     const gchar         *contents_id);

GtkWidget * clay_procedure_dialog_fill_expander     (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *title_id,
                                                     gboolean             invert_title,
                                                     const gchar         *contents_id);

GtkWidget * clay_procedure_dialog_fill_scrolled_window
                                                    (ClayProcedureDialog  *dialog,
                                                     const gchar          *container_id,
                                                     const gchar          *contents_id);

GtkWidget * clay_procedure_dialog_fill_notebook     (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     const gchar         *label_id,
                                                     const gchar         *page_id,
                                                     ...) G_GNUC_NULL_TERMINATED;
GtkWidget * clay_procedure_dialog_fill_notebook_list
                                                    (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GList               *label_list,
                                                     GList               *page_list);

GtkWidget * clay_procedure_dialog_fill_paned        (ClayProcedureDialog *dialog,
                                                     const gchar         *container_id,
                                                     GtkOrientation       orientation,
                                                     const gchar         *child1_id,
                                                     const gchar         *child2_id);

void        clay_procedure_dialog_fill              (ClayProcedureDialog *dialog,
                                                     ...) G_GNUC_NULL_TERMINATED;
void        clay_procedure_dialog_fill_list         (ClayProcedureDialog *dialog,
                                                     GList               *properties);

void        clay_procedure_dialog_set_sensitive     (ClayProcedureDialog *dialog,
                                                     const gchar         *property,
                                                     gboolean             sensitive,
                                                     GObject             *config,
                                                     const gchar         *config_property,
                                                     gboolean             config_invert);

void        clay_procedure_dialog_set_sensitive_if_in (ClayProcedureDialog *dialog,
                                                       const gchar         *property,
                                                       GObject             *config,
                                                       const gchar         *config_property,
                                                       ClayValueArray      *values,
                                                       gboolean             in_values);

gboolean    clay_procedure_dialog_run               (ClayProcedureDialog *dialog);


G_END_DECLS

#endif /* __CLAY_PROCEDURE_DIALOG_H__ */
